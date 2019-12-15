#include "cassie.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassandra.h>

#include "SseServer.hpp"

void print_error(CassFuture* future) {
    const char* message;
    size_t message_length;
    cass_future_error_message(future, &message, &message_length);
    fprintf(stderr, "Error: %.*s\n", (int)message_length, message);
}

CassCluster* create_cluster(const char* hosts) {
    CassCluster* cluster = cass_cluster_new();
    cass_cluster_set_contact_points(cluster, hosts);
    return cluster;
}

CassError connect_session(CassSession* session, const CassCluster* cluster) {
    CassFuture* future = cass_session_connect(session, cluster);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        print_error(future);
    }
    cass_future_free(future);

    return rc;
}

CassError execute_query(CassSession* session, const char* query) {
    CassFuture* future = NULL;
    CassStatement* statement = cass_statement_new(query, 0);

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK)
        print_error(future);

    cass_future_free(future);
    cass_statement_free(statement);

    return rc;
}

size_t get_size(CassSession* session) {
    size_t size = 0;

    CassFuture* future = NULL;
    CassStatement* statement = cass_statement_new("SELECT count(*) FROM isen.pairs;", 0);

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        print_error(future);
    } else {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            const CassRow* row = cass_iterator_get_row(iterator);
            cass_value_get_int64(cass_row_get_column(row, 0), (cass_int64_t*)&size);
        }

        cass_result_free(result);
        cass_iterator_free(iterator);
    }

    cass_future_free(future);
    cass_statement_free(statement);

    return size;
}

CassError prepare_insert_into_batch(CassSession* session, const CassPrepared** prepared) {
    const char* query = "INSERT INTO isen.pairs (key, value) VALUES (?, ?)";

    CassFuture* future = cass_session_prepare(session, query);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK)
        print_error(future);
    else
        *prepared = cass_future_get_prepared(future);

    cass_future_free(future);

    return rc;
}

CassError prepare_select_from_batch(CassSession* session, const CassPrepared** prepared) {
    const char* query = "SELECT value FROM isen.pairs WHERE key = ?";

    CassFuture* future = cass_session_prepare(session, query);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK)
        print_error(future);
    else
        *prepared = cass_future_get_prepared(future);

    cass_future_free(future);

    return rc;
}

CassError select_from_batch_with_prepared(CassSession* session, const CassPrepared* prepared, const uint8_t* f, uint8_t** s) {
    CassStatement* statement = cass_prepared_bind(prepared);
    cass_statement_bind_bytes(statement, 0, f, l_size);

    CassFuture* future = cass_session_execute(session, statement);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        print_error(future);
    } else {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            size_t len;
            const CassRow* row = cass_iterator_get_row(iterator);
            cass_value_get_bytes(cass_row_get_column(row, 0), (const cass_byte_t**)s, &len);
        }

        cass_result_free(result);
        cass_iterator_free(iterator);
    }

    cass_future_free(future);
    cass_statement_free(statement);

    return rc;
}

void insert_into_batch_with_prepared(CassSession* session, const CassPrepared* prepared, const size_t nr_pairs, const uint8_t* buffer) {
    CassBatch* batch = cass_batch_new(CASS_BATCH_TYPE_LOGGED);

    for (size_t i = 0; i < nr_pairs; ++i) {
        CassStatement* statement = cass_prepared_bind(prepared);
        cass_statement_bind_bytes(statement, 0, buffer + (i * pair_len), l_size);
        cass_statement_bind_bytes(statement, 1, buffer + (i * pair_len) + l_size, d_size);
        cass_batch_add_statement(batch, statement);
        cass_statement_free(statement);
    }

    CassFuture* future = cass_session_execute_batch(session, batch);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        print_error(future);
    }

    cass_future_free(future);
    cass_batch_free(batch);
}
