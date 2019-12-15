#ifndef VISEN_CASSIE_H
#define VISEN_CASSIE_H

#include "cassandra.h"
#include <stdint.h>

CassError prepare_insert_into_batch(CassSession* session, const CassPrepared** prepared);
CassError prepare_select_from_batch(CassSession* session, const CassPrepared** prepared);

void insert_into_batch_with_prepared(CassSession* session, const CassPrepared* prepared, const size_t nr_pairs, const uint8_t* buffer);
CassError select_from_batch_with_prepared(CassSession* session, const CassPrepared* prepared, const uint8_t* f, uint8_t** s);
CassError execute_query(CassSession* session, const char* query);
size_t get_size(CassSession* session);
CassError connect_session(CassSession* session, const CassCluster* cluster);
CassCluster* create_cluster(const char* hosts);

#endif
