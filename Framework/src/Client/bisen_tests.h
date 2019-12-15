#ifndef VISEN_BISEN_TESTS_H
#define VISEN_BISEN_TESTS_H

#include "rbisen/SseClient.hpp"
#include "untrusted_util.h"

void bisen_setup(secure_connection* conn, SseClient* client);
void bisen_update(secure_connection* conn, SseClient* client, char* bisen_doc_type, unsigned nr_docs, std::vector<std::string> doc_paths);
void bisen_search(secure_connection* conn, SseClient* client, vector<string> queries);

#endif
