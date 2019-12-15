#include "bisen_tests.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "definitions.h"

#include "util.h"

extern "C" {
#include "rbisen/Utils.h"
#include "rbisen/types.h"
}

void bisen_setup(secure_connection* conn, SseClient* client) {
    unsigned char* data_bisen;

    // setup
    unsigned long long data_size_bisen = client->generate_setup_msg(&data_bisen);
    iee_comm(conn, data_bisen, data_size_bisen);
    free(data_bisen);
}

void bisen_update(secure_connection* conn, SseClient* client, char* bisen_doc_type, unsigned nr_docs, std::vector<std::string> doc_paths) {
    struct timeval start, end;
    double total_client = 0, total_iee = 0, extract_keywords = 0;

    printf("Update type: %s\n", bisen_doc_type);

    size_t nr_updates = 0;
    for (const string doc : doc_paths) {
        //printf("Update (%lu/%u)\n", nr_updates, nr_docs);

        gettimeofday(&start, NULL);
        vector<map<string, int>> docs;

        if(!strcmp(bisen_doc_type, "wiki")) {
            // extract keywords from a 1M, multiple article, file
            docs = client->extract_keywords_frequency_wiki(doc);
        } else if(!strcmp(bisen_doc_type, "normal")) {
            // one file is one document
            docs.push_back(client->extract_keywords_frequency(doc));
        } else {
            printf("Document type not recognised\n");
            exit(0);
        }

        nr_updates += docs.size();
        gettimeofday(&end, NULL);
        extract_keywords += untrusted_util::time_elapsed_ms(start, end);
        total_client += untrusted_util::time_elapsed_ms(start, end);

        vector<pair<size_t, uint8_t*>> msgs;
        size_t total_size = 0;
        const size_t to_send = docs.size();

        for (const map<string, int> text : docs) {
            // generate the byte* to send to the server
            gettimeofday(&start, NULL);
            unsigned char* data_bisen;
            unsigned long long data_size_bisen = client->add_new_document(text, &data_bisen);
            gettimeofday(&end, NULL);
            total_client += untrusted_util::time_elapsed_ms(start, end);

            msgs.push_back(make_pair(data_size_bisen, data_bisen));
            total_size += data_size_bisen;
        }

        // put messages into bulk buffer
        const size_t buffer_len = sizeof(uint8_t) + sizeof(size_t) + docs.size() * sizeof(size_t) + total_size;
        uint8_t* buffer = (uint8_t*)malloc(buffer_len);
        buffer[0] = OP_IEE_BISEN_BULK;
        uint8_t* tmp = buffer + sizeof(uint8_t);

        memcpy(tmp, &to_send, sizeof(size_t));
        tmp += sizeof(size_t);

        for(pair<size_t, uint8_t*> msg : msgs) {
            memcpy(tmp, &(msg.first), sizeof(size_t));
            tmp += sizeof(size_t);

            memcpy(tmp, msg.second, msg.first);
            tmp += msg.first;

            free(msg.second);
        }

        gettimeofday(&start, NULL);
        iee_comm(conn, buffer, buffer_len);
        gettimeofday(&end, NULL);
        total_iee += untrusted_util::time_elapsed_ms(start, end);

        free(buffer);

        if (nr_docs > 0 && nr_updates >= nr_docs) {
            printf("Breaking, done enough updates (%lu/%u)\n", nr_updates, nr_docs);
            break;
        }
    }

    printf("-- BISEN TOTAL add: %lf ms (%lu docs) --\n", total_client + total_iee, nr_updates);
    printf("-- BISEN add client: %lf ms (of which %lf ms file read, %lf ms parsing) --\n", total_client, client->get_read_file_time(), extract_keywords - client->get_read_file_time());
    printf("-- BISEN add iee w/ net: %lf ms --\n", total_iee);
}

void bisen_search(secure_connection* conn, SseClient* client, vector<string> queries) {
    struct timeval start, end;

    unsigned char* data_bisen;
    size_t data_size_bisen;

    printf("-- BISEN CLIENT SEARCHES --\n");
    printf("NR\tTOTAL_client\tTOTAL_iee_w_net\tDOCS_RETURNED\n");
    for (unsigned k = 0; k < queries.size(); k++) {
        double total_client = 0, total_iee = 0;

        gettimeofday(&start, NULL);
        string query = queries[k];
#if DEBUG_PRINT_BISEN_CLIENT
        printf("\n----------------------------\n");
        printf("Query %d: %s\n", k, query.c_str());
#endif

        data_size_bisen = client->search(query, &data_bisen);
        gettimeofday(&end, NULL);
        total_client += untrusted_util::time_elapsed_ms(start, end);

        gettimeofday(&start, NULL);
        iee_send(conn, data_bisen, data_size_bisen);
        free(data_bisen);

        uint8_t* bisen_out;
        size_t bisen_out_len;
        iee_recv(conn, &bisen_out, &bisen_out_len);
        gettimeofday(&end, NULL);
        total_iee += untrusted_util::time_elapsed_ms(start, end);

        gettimeofday(&start, NULL);
        size_t n_docs;
        memcpy(&n_docs, bisen_out, sizeof(size_t));

        uint8_t has_scoring;
        memcpy(&has_scoring, bisen_out + sizeof(size_t), sizeof(uint8_t));

        int pair_len = sizeof(int);
        if(has_scoring)
            pair_len += sizeof(double);

#if DEBUG_PRINT_BISEN_CLIENT
        printf("Number of docs: %lu\n", n_docs);
#endif

        uint8_t* tmp = bisen_out + sizeof(uint8_t) + sizeof(size_t);
        for (size_t i = 0; i < n_docs; ++i) {
            if(has_scoring) {
                int d;
                double s;
                memcpy(&d, tmp + i * pair_len, sizeof(int));
                memcpy(&s, tmp + i * pair_len + sizeof(int), sizeof(double));
#if DEBUG_PRINT_BISEN_CLIENT
                printf("%d %f\n", d, s);
#endif
            } else {
                int d;
                memcpy(&d, tmp + i * pair_len, sizeof(int));
#if DEBUG_PRINT_BISEN_CLIENT
                printf("%d ", d);
#endif
            }
        }
#if DEBUG_PRINT_BISEN_CLIENT
        if(!has_scoring)
            printf("\n");
#endif
        free(bisen_out);

        gettimeofday(&end, NULL);
        total_client += untrusted_util::time_elapsed_ms(start, end);

        printf("%u\t%lf\t%lf\t%lu\n", k, total_client, total_iee, n_docs);
    }
}
