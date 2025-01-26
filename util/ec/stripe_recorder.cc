#include <iostream>
#include "stripe_recorder.h"

void StripeRecorder::AddTable(int table_id, int stripe_id) {
  table_info table(table_id, stripe_id);
  std::cout << "@@@adding" << table_id << " -> " << stripe_id << std::endl;
  t_mutex_.lock();
  table_infos[table_id] = table;
  t_mutex_.unlock();

  if (stripe_infos.find(stripe_id) == stripe_infos.end()) {
    std::cout << "creating new stripe: " << stripe_id << std::endl;
    stripe_info stripe(stripe_id);
    s_mutex_.lock();
    stripe_infos[stripe_id] = stripe;
    s_mutex_.unlock();
  }
  s_mutex_.lock();
  stripe_infos[stripe_id].tables.push_back(table_id);
  s_mutex_.unlock();
  std::cout << "stripe: " << stripe_id << " contains " << stripe_infos[stripe_id].tables.size() << std::endl;
}

void StripeRecorder::ExpireTable(int table_id) {
  table_infos[table_id].valid = false;
  int stripe_id = table_infos[table_id].stripe_id;
  s_mutex_.lock();
  stripe_info& stripe = stripe_infos[stripe_id];
  stripe.expired_count++;
  s_mutex_.unlock();
  std::cout << "table_id: " << table_id << " is expired, belonging to stripe_id: " << stripe_id << " , whose expired_count is: " << stripe.expired_count << std::endl;

  if (stripe.expired_count >= delete_threshold) {
    std::cout << "!!!!stripe: " << stripe_id << " is going to be deleted!!!" << std::endl;
    std::cout << "threr are " << stripe.tables.size() << " in the stripe";
    q_mutex_.lock();
    for (int id : stripe.tables) {
      // we would physically delete the tables from the disk
      to_be_deleted_tables.push(id);
      std::cout << " " << id << ",";
    }
    q_mutex_.unlock();
    std::cout << std::endl;
    std::cout << "ExpireTable::to_be_deleted_tables.size: " << to_be_deleted_tables.size() << std::endl;

    s_mutex_.lock();
    stripe_infos.erase(stripe_id);
    s_mutex_.unlock();
  }
}

void StripeRecorder::DeleteTable(std::vector<std::string>& files_names,
                                 const std::string& dbname) {
  q_mutex_.lock();
  t_mutex_.lock();
  if (!to_be_deleted_tables.empty()) {
    std::cout << "DeleteTable::to_be_deleted_tables.size: " << to_be_deleted_tables.size() << std::endl;
  }
  while (!to_be_deleted_tables.empty()) {
    int table_id = to_be_deleted_tables.front();
    to_be_deleted_tables.pop();
    table_info table = table_infos[table_id];
    table_infos.erase(table_id);
    files_names.push_back(dbname + "/" + std::to_string(table_id));
  }
  t_mutex_.unlock();
  q_mutex_.unlock();
  if (files_names.size() > 0) {
    std::cout << "the following files are going to be physically deleted: ";
    for (auto s : files_names) {
      std::cout << s << ", ";
    }
    std::cout << std::endl;
  }
}

bool StripeRecorder::LookUpTable(int table_id) const {
  auto it = table_infos.find(table_id);
  if (it != table_infos.end()) {
    return it->second.valid;
  }
  // the current table is not in a stirpe, therefore it must be valid!
  return true;
}
