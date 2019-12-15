#include "Client.h"

#include "untrusted_util.h"
#include "definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sodium.h>
#include <random>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <libconfig.h>

#include "bisen_tests.h"
#include "util.h"

using namespace std;

typedef struct configs {
    unsigned bisen_nr_docs = 1000;
    char* bisen_doc_type, *bisen_dataset_dir;
    vector<string> bisen_queries;
} configs;

void separated_tests(const configs* const settings, secure_connection* conn) {
    struct timeval start, end;

    //////////// BISEN ////////////
        SseClient client;
        vector<string> txt_paths = list_txt_files(-1, settings->bisen_dataset_dir); // get all paths and decide nr of docs in update (wiki mode is special)
        sort(txt_paths.begin(), txt_paths.end(), greater<string>()); // documents with more articles happen to be at the end for the wikipedia dataset

        gettimeofday(&start, NULL);
        bisen_setup(conn, &client);
        gettimeofday(&end, NULL);
        printf("-- BISEN setup: %lf ms --\n", untrusted_util::time_elapsed_ms(start, end));

        reset_bytes();

        gettimeofday(&start, NULL);
        bisen_update(conn, &client, settings->bisen_doc_type, settings->bisen_nr_docs, txt_paths);
        gettimeofday(&end, NULL);
        printf("-- BISEN TOTAL updates: %lf ms %d docs --\n", untrusted_util::time_elapsed_ms(start, end), settings->bisen_nr_docs);
        print_bytes("update_bisen");

        bisen_search(conn, &client, settings->bisen_queries);
        print_bytes("search_bisen");

        // print benchmark
        uint8_t bench_op[3];
        bench_op[0] = OP_RBISEN;
        bench_op[1] = OP_IEE_DUMP_BENCH;
        bench_op[2] = 0;
        iee_comm(conn, bench_op, 3);

        reset_bytes();

    ///////////////////////////////
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    setvbuf(stderr, NULL, _IONBF, BUFSIZ);

    const int server_port = IEE_PORT;

    configs program_configs;
    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "../isen.cfg")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(1);
    }

    // addresses
    char* server_name;
    config_lookup_string(&cfg, "iee_hostname", (const char**)&server_name);

    config_lookup_int(&cfg, "bisen.nr_docs", (int*)&program_configs.bisen_nr_docs);
    config_lookup_string(&cfg, "bisen.doc_type", (const char**)&program_configs.bisen_doc_type);
    config_lookup_string(&cfg, "bisen.dataset_dir", (const char**)&program_configs.bisen_dataset_dir);

    config_setting_t* queries_setting = config_lookup(&cfg, "bisen.queries");
    const int count = config_setting_length(queries_setting);

    for(int i = 0; i < count; ++i) {
        config_setting_t* q = config_setting_get_elem(queries_setting, i);
        program_configs.bisen_queries.push_back(string(config_setting_get_string(q)));
    }

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "hk:b:")) != -1) {
        switch (c) {
            /*case 'k':
                program_configs.visen_nr_clusters = (unsigned)std::stoi(optarg);
                break;*/
            case 'b':
                program_configs.bisen_nr_docs = (unsigned)std::stoi(optarg);
//                program_configs.visen_nr_docs = program_configs.bisen_nr_docs;
                break;
            case 'h':
                printf("Usage: ./Client [-b nr_docs]\n");
                exit(0);
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "-%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(1);
            default:
                exit(-1);
        }
    }

    // init mbedtls
    secure_connection* conn;
    untrusted_util::init_secure_connection(&conn, server_name, server_port);

    separated_tests(&program_configs, conn);

    // close ssl connection
    untrusted_util::close_secure_connection(conn);

    return 0;
}

