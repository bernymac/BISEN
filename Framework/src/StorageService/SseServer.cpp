#include "SseServer.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include "untrusted_util.h"
#include "definitions.h"

#if STORAGE == STORAGE_MAP
    #if MAP_TYPE == MAP_TYPE_UNORDERED
#include <map>
#include <unordered_map>
typedef unordered_map<void*, void*, VoidHash, VoidEqual> storage_map;
    #elif MAP_TYPE == MAP_TYPE_SPARSE
#include <sparsepp/spp.h>
typedef spp::sparse_hash_map<void*, void*, VoidHash, VoidEqual> storage_map;
    #elif MAP_TYPE == MAP_TYPE_TBB
#include "tbb/concurrent_unordered_map.h"
typedef tbb::concurrent_unordered_map<void*, void*, VoidHash, VoidEqual> storage_map;
    #endif
#endif

#if STORAGE == STORAGE_CASSANDRA
#include "cassie.h"
CassSession* session;
CassCluster* cluster;
#elif STORAGE == STORAGE_REDIS
#include <hiredis/hiredis.h>
redisContext* c;
#elif STORAGE == STORAGE_MAP
storage_map I;
#endif

using namespace std;

/*****************************************************************************/
typedef struct search_res {
    double processing = 0;
    double network = 0;
    size_t batches = 0;
    size_t nr_labels = 0;
} search_res;
vector<search_res> search_results;
size_t bytes_sent_upd = 0, bytes_received_upd = 0;
size_t bytes_sent_src = 0, bytes_received_src = 0;

/*****************************************************************************/
typedef struct client_data {
    int socket;
} client_data;
/*****************************************************************************/

void* process_client(void* args) {
    client_data* data = (client_data*)args;
    int client_socket = data->socket;
    free(data);

    struct timeval start;
    double total_add_time = 0;
    double total_add_time_network = 0;
    size_t count_adds = 0;

    while (1) {
        uint8_t op;
        untrusted_util::socket_receive(client_socket, &op, sizeof(uint8_t));

        switch (op) {
            case OP_UEE_INIT: {
#if STORAGE == STORAGE_CASSANDRA
                execute_query(session, "CREATE KEYSPACE IF NOT EXISTS isen WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor' : 1};");
                execute_query(session, "DROP TABLE IF EXISTS isen.pairs;");
                execute_query(session, "CREATE TABLE isen.pairs (key blob, value blob, PRIMARY KEY (key));");
#elif STORAGE == STORAGE_REDIS
                redisReply* reply = (redisReply*)redisCommand(c, "FLUSHALL");
                if (!reply) {
                    printf("error redis flushall!\n");
                }
#elif STORAGE == STORAGE_MAP
                // TODO fix, must deallocate "big" buffers instead, of which we must keep references to
                /*if(!I.empty()) {
                    //#ifdef VERBOSE
                    printf("Size before: %lu\n", I.size());
                    //#endif

                    uee_map::iterator it;
                    for(it = I.begin(); it != I.end(); ++it) {
                        free(it->first);
                        free(it->second);
                    }

                    I.clear();
                }*/
#endif
                #ifdef VERBOSE
                printf("Finished Setup!\n");
                #endif
                break;
            }
            case OP_UEE_ADD: {
                bytes_received_upd++;

                #ifdef VERBOSE
                printf("Started Add!\n");
                #endif

                /*if(count_adds++ % 1000 == 0)
                    printf("add %lu\n", count_adds);*/

                // receive data from iee
                start = untrusted_util::curr_time();
                size_t nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(size_t));

                uint8_t* buffer = (uint8_t*)malloc(nr_labels * pair_len);
                untrusted_util::socket_receive(client_socket, buffer, nr_labels * pair_len);
                total_add_time_network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                start = untrusted_util::curr_time();

#if STORAGE == STORAGE_CASSANDRA
                const CassPrepared* prepared = NULL;
                if (prepare_insert_into_batch(session, &prepared) == CASS_OK)
                    insert_into_batch_with_prepared(session, prepared, nr_labels, buffer);
                cass_prepared_free(prepared);
#elif STORAGE == STORAGE_REDIS
                for(size_t i = 0; i < nr_labels; i++) {
                    void* l = buffer + i * pair_len;
                    void* d = buffer + i * pair_len + l_size;
                    redisAppendCommand(c, "SET %b %b", l, (size_t)l_size, d, (size_t)d_size);
                }
#elif STORAGE == STORAGE_MAP
                for(size_t i = 0; i < nr_labels; i++) {
                    void* l = buffer + i * pair_len;
                    void* d = buffer + i * pair_len + l_size;
                    I[l] = d;
                }
#endif

#if STORAGE == STORAGE_REDIS
                for (size_t i = 0; i < nr_labels; i++) {
                    redisReply* reply;
                    redisGetReply(c,(void**)&reply);

                    freeReplyObject(reply);
                }
#endif
                total_add_time += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());
                count_adds++;
                bytes_received_upd += sizeof(size_t) + nr_labels * pair_len;

                #ifdef VERBOSE
                printf("Finished Add!\n");
                #endif
                break;
            }
            case OP_UEE_SEARCH: {
                bytes_received_src++;
                #ifdef VERBOSE
                printf("Started Search!\n");
                #endif

                // receive information from iee
                start = untrusted_util::curr_time();
                size_t nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(size_t));

                //cout << "batch_size " << nr_labels << endl;

                uint8_t* label = new uint8_t[l_size * nr_labels];
                untrusted_util::socket_receive(client_socket, label, l_size * nr_labels);
                search_results.back().network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                start = untrusted_util::curr_time();
                uint8_t* buffer = (unsigned char*)malloc(d_size * nr_labels);

#if STORAGE == STORAGE_CASSANDRA
                uint8_t* res = (uint8_t*)malloc(d_size);
                const CassPrepared* prepared = NULL;

                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    if (prepare_select_from_batch(session, &prepared) == CASS_OK)
                        select_from_batch_with_prepared(session, prepared, label + i * l_size, &res);
                    cass_prepared_free(prepared);

                    /*if(memcmp(res, I[label + i * l_size], d_size)) {
                        printf("not the same!\n");
                        exit(1);
                    }*/

                    memcpy(buffer + i * d_size, res, d_size);
                }

                free(res);
#elif STORAGE == STORAGE_REDIS
                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    redisReply* reply = (redisReply*)redisCommand(c, "GET %b", label + i * l_size, l_size);
                    if (!reply) {
                         printf("error redis get!\n");
                    }

                    memcpy(buffer + i * d_size, reply->str, d_size);
                    freeReplyObject(reply);
                }
#elif STORAGE == STORAGE_MAP
                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    if(!(I[label + i * l_size])) {
                        printf("Label not found! Exit\n");
                        exit(1);
                    }

                    memcpy(buffer + i * d_size, I[label + i * l_size], d_size);
                }
#endif
                delete[] label;

                search_results.back().processing += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                // answer back to iee
                start = untrusted_util::curr_time();
                untrusted_util::socket_send(client_socket, buffer, d_size * nr_labels);
                free(buffer);
                search_results.back().network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                search_results.back().batches++;
                search_results.back().nr_labels += nr_labels;
                bytes_received_src += sizeof(size_t) + l_size * nr_labels;
                bytes_sent_src += d_size * nr_labels;
                break;
            }
            case OP_UEE_DUMP_BENCH: {
                // this instruction is for benchmarking only and can be safely
                // removed if wanted
                size_t db_size = 0;

#if STORAGE == STORAGE_CASSANDRA
                db_size = get_size(session);
#elif STORAGE == STORAGE_REDIS
                redisReply* reply = (redisReply*)redisCommand(c, "DBSIZE");
                if (!reply) {
                    printf("error redis getdbsize!\n");
                }

                memcpy(&db_size, (size_t*)&(reply->integer), sizeof(size_t));
#elif STORAGE == STORAGE_MAP
                db_size = I.size();
#endif
                printf("STORAGE SERVICE: total add = %6.3lf ms\n", total_add_time);
                printf("STORAGE SERVICE: total add network = %6.3lf ms\n", total_add_time_network);
                printf("STORAGE SERVICE: nr add batches = %lu\n", count_adds);
                printf("STORAGE SERVICE: size index: %lu\n", db_size);

                uint8_t aggregate;
                untrusted_util::socket_receive(client_socket, &aggregate, sizeof(uint8_t));

                if(aggregate) {
                    const size_t nr_searches = search_results.size();

                    printf("-- AVG STORAGE SERVICE %lu SEARCHES --\n", nr_searches);
                    printf("PROCESSING\tNETWORK\tBATCHES\tLABELS\n");

                    search_res agg;
                    for (search_res r : search_results) {
                        agg.processing += r.processing;
                        agg.network += r.network;
                        agg.batches += r.batches;
                        agg.nr_labels += r.nr_labels;
                    }

                    agg.processing /= nr_searches;
                    agg.network /= nr_searches;
                    agg.batches /= nr_searches;
                    agg.nr_labels /= nr_searches;

                    printf("%lf\t%lf\t%lu\t%lu\n", agg.processing, agg.network, agg.batches, agg.nr_labels);
                } else {
                    printf("-- STORAGE SERVICE SEARCHES --\n");
                    printf("NR\tPROCESSING\tNETWORK\tBATCHES\tLABELS\n");
                    unsigned count = 0;
                    for (search_res r : search_results) {
                        printf("%u\t%lf\t%lf\t%lu\t%lu\n", count++, r.processing, r.network, r.batches, r.nr_labels);
                    }
                }

                printf("Sent bytes update storage: %lu\nReceived bytes update storage: %lu\n", bytes_sent_upd, bytes_received_upd);
                printf("Sent bytes search storage: %lu\nReceived bytes search storage: %lu\n", bytes_sent_src, bytes_received_src);

                // reset
                search_results.clear();
                bytes_sent_upd = 0;
                bytes_received_upd = 0;
                bytes_sent_src = 0;
                bytes_received_src = 0;

		close(client_socket);

                break;
            }
            case '5': {
                // this instruction is for benchmarking only and can be safely
                // removed if wanted

                // adds a new search benchmarking record
                search_res r;
                search_results.push_back(r);

                break;
            }
            default: {
                printf("SseServer unkonwn command: %02x\n", op);
                exit(1);
            }
        }
    }
}

int main(int argc, const char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    setvbuf(stderr, NULL, _IONBF, BUFSIZ);

    if(argc != 2) {
        printf("Usage: ./Storage port\n");
        exit(1);
    }

    const int server_port = atoi(argv[1]);

#if STORAGE == STORAGE_CASSANDRA
    char* hosts = "127.0.0.1";

    session = cass_session_new();
    cluster = create_cluster(hosts);

    if (connect_session(session, cluster) != CASS_OK) {
        cass_cluster_free(cluster);
        cass_session_free(session);
        exit(1);
    }
#elif STORAGE == STORAGE_REDIS
    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
#endif
    struct sockaddr_in server_addr;
    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listen_socket;
    if ((listen_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Could not create socket!\n");
        exit(1);
    }

    int res = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &res, sizeof(res)) == -1) {
        printf("Could not set socket options!\n");
        exit(1);
    }

    if ((bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        printf("Could not bind socket!\n");
        exit(1);
    }

    if (listen(listen_socket, 16) < 0) {
        printf("Could not open socket for listening!\n");
        exit(1);
    }

    //start listening for iee calls
    printf("Finished Server init! Gonna start listening for IEE requests!\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    printf("Listening for requests...\n");

    while (true) {
        client_data* data = (client_data*)malloc(sizeof(client_data));

        if ((data->socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            printf("Accept failed!\n");
            exit(1);
        }

        int flag = 1;
        setsockopt(data->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

        int iMode = 0;
        ioctl(data->socket, FIONBIO, &iMode);

        printf("------------------------------------------\nClient connected (%s)\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        pthread_create(&tid, NULL, process_client, data);
    }
#if STORAGE == STORAGE_CASSANDRA
    CassFuture* close_future = cass_session_close(session);
    cass_future_wait(close_future);
    cass_future_free(close_future);

    cass_cluster_free(cluster);
    cass_session_free(session);
#endif
}
