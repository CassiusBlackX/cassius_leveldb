#include <iostream>
#include "stripe_recorder.h"

void StripeRecorder::AddTable(int table_id, int stripe_id) {
  table_info table(table_id, stripe_id);
  t_mutex_.lock();
  table_infos[table_id] = table;
  t_mutex_.unlock();

  if (stripe_infos.find(stripe_id) == stripe_infos.end()) {
    stripe_info stripe(stripe_id);
    s_mutex_.lock();
    stripe_infos[stripe_id] = stripe;
    s_mutex_.unlock();
  }
  s_mutex_.lock();
  stripe_infos[stripe_id].tables.push_back(table_id);
  s_mutex_.unlock();
}

void StripeRecorder::ExpireTable(int table_id) {
  table_infos[table_id].valid = false;
  int stripe_id = table_infos[table_id].stripe_id;
  s_mutex_.lock();
  stripe_info& stripe = stripe_infos[stripe_id];
  stripe.expired_count++;
  s_mutex_.unlock();

  if (stripe.expired_count >= delete_threshold) {
    q_mutex_.lock();
    for (int table_id : stripe.tables) {
      // we would physically delete the tables from the disk
      to_be_deleted_tables.push(table_id);
    }
    q_mutex_.unlock();

    s_mutex_.lock();
    stripe_infos.erase(stripe_id);
    s_mutex_.unlock();
  }
}

void StripeRecorder::DeleteTable(std::vector<std::string>& files_names,
                                 const std::string& dbname) {
  q_mutex_.lock();
  t_mutex_.lock();
  while (!to_be_deleted_tables.empty()) {
    int table_id = to_be_deleted_tables.front();
    to_be_deleted_tables.pop();
    table_info table = table_infos[table_id];
    table_infos.erase(table_id);
    files_names.push_back(dbname + "/" + std::to_string(table_id));
  }
  t_mutex_.unlock();
  q_mutex_.unlock();
  // NOTE: log can be removed in release version
  std::cout << "files.count:" << files_names.size() <<": ";
  for (auto s : files_names) {
    std::cout << s << " ";
  }
  std::cout << "are no longer needed, therefore deleted" << std::endl;
}

bool StripeRecorder::LookUpTable(int table_id) const {
  auto it = table_infos.find(table_id);
  if (it != table_infos.end()) {
    return it->second.valid;
  }
  // the current table is not in a stirpe, therefore it must be valid!
  return true;
}
