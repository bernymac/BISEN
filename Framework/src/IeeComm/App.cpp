#include "App.h"

#include "untrusted_util.h"
#include "trusted_mbedtls.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <trusted_mbedtls.h>

#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"

#include "mbedtls_net.c"

// to pass data to client thread
typedef struct client_data {
    sgx_enclave_id_t eid;
    thread_info_t* info;
} client_data;

static mbedtls_net_context listen_fd;

static void close_all(int signum) {
    mbedtls_net_free(&listen_fd);
    fflush(NULL);
    exit(0);
}

void* ecall_handle_ssl_connection(void* data) {
    long thread_id = (long)pthread_self();

    client_data* cdata = (client_data*)data;
    ssl_conn_handle(cdata->eid, thread_id, cdata->info);

    printf("thread exiting for thread %ld\n", thread_id);
    mbedtls_net_free(&cdata->info->client_fd);
    return NULL;
}

int SGX_CDECL main(int argc, const char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    setvbuf(stderr, NULL, _IONBF, BUFSIZ);

    if(argc != 2) {
        printf("usage: ./IeeComm nr-threads\n");
        exit(1);
    }

    // port to start the server on
    const int server_port = IEE_PORT;

    // nr of threads in thread pool
    const unsigned nr_threads = atoi(argv[1]) - 1;
    printf("nr threads w/o main: %u\n", nr_threads);

    // register signal handler
    signal(SIGINT, close_all);

    // initialise the enclave
    sgx_enclave_id_t eid = 0;
    if (initialise_enclave(&eid) < 0) {
        printf("Problem with enclave initialisation, exiting...\n");
        return -1;
    }

    printf("Enclave id: %lu\n", eid);

    // perform internal enclave initialisation
    sgx_status_t status = ecall_init_enclave(eid, nr_threads);
    if (status != SGX_SUCCESS) {
        print_error_message(status);
        exit(-1);
    }

    // initialise thread pool
    thread_pool* pool = init_thread_pool(eid, nr_threads);

    // mbedtls
    ssl_conn_init(eid);
    printf("binding on https://localhost:%d...\n", server_port);

    // put port in char buf
    char port[5];
    sprintf(port, "%d", server_port);

    int ret;
    if ((ret = mbedtls_net_bind(&listen_fd, NULL, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        printf(" failed\n  ! mbedtls_net_bind returned %d\n\n", ret);
        exit(-1);
    }

    printf("[main] waiting for a remote connection...\n");

    while (1) {
#ifdef MBEDTLS_ERROR_C
        if (ret != 0) {
            char error_buf[100];
            mbedtls_strerror(ret, error_buf, 100);
            printf("  [ main ]  Last error was: -0x%04x - %s\n", -ret, error_buf);
        }
#endif

        // init thread data
        client_data* data = (client_data*)malloc(sizeof(client_data));
        memcpy(&data->eid, &eid, sizeof(sgx_enclave_id_t));
        data->info = (thread_info_t*)malloc(sizeof(thread_info_t));
        data->info->config = NULL;
        data->info->thread_complete = 0;

        // wait until a client connects
        /*if (0 != mbedtls_net_set_nonblock(&listen_fd))
            printf("can't set nonblock for the listen socket\n");*/

        mbedtls_net_context client_fd;
        ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            ret = 0;
            continue;
        } else if (ret != 0) {
            printf("  [ main ] failed: mbedtls_net_accept returned -0x%04x\n", ret);
            break;
        }

        printf("[main] creating a new thread for client %d\n", client_fd.fd);

        memcpy(&data->info->client_fd, &client_fd, sizeof(mbedtls_net_context));

        pthread_t tid;
        if ((ret = pthread_create(&tid, NULL, ecall_handle_ssl_connection, data)) != 0) {
            printf("[main] failed: thread_create returned %d\n", ret);
            mbedtls_net_free(&client_fd);
            continue;
        }
    }

    mbedtls_net_free(&listen_fd);

    // cleanup and exit
    destroy_enclave(eid);
    delete_thread_pool(pool);

    printf("Enclave successfully terminated.\n");
    return 0;
}
