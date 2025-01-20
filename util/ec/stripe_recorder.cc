#include "stripe_recorder.h"

void StripeRecorder::AddTable(int table_id, int stripe_id) {
    table_info table(table_id, stripe_id);
    table_infos[table_id] = table;

    if (stripe_infos.find(stripe_id) == stripe_infos.end()) {
        stripe_info stripe(stripe_id);
        stripe_infos[stripe_id] = stripe;
    }
    stripe_infos[stripe_id].tables.push_back(table_id);
}

void StripeRecorder::DeleteTable(int table_id) {
    table_infos[table_id].valid = false;
    int stripe_id = table_infos[table_id].stripe_id;
    stripe_info& stripe = stripe_infos[stripe_id];
    stripe.expired_count++;
    if (stripe.expired_count >= delete_threshold) {
        for (int table_id : stripe.tables) {
            table_infos[table_id].valid = false;
        }
        stripe_infos.erase(stripe_id);
    }
}