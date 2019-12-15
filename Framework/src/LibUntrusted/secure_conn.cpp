#include "untrusted_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"

typedef struct secure_conn_t {
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
} secure_conn_t;

static void my_debug(void* ctx, int level, const char* file, int line, const char* str) {
    ((void)level);
    fprintf((FILE*)ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE*)ctx);
}

static int ssl_send_all(mbedtls_ssl_context* ssl, const void* buf, size_t len) {
    size_t total = 0;       // how many bytes we've sent
    size_t bytesleft = len; // how many we have left to send
    long n = 0;

    while(total < len) {
        n = mbedtls_ssl_write(ssl, (unsigned char*)buf + total, bytesleft > 16384? 16384 : bytesleft);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n == -1 || total != len ? -1 : 0; // return -1 on failure, 0 on success
}

static ssize_t ssl_receive_all(mbedtls_ssl_context* ssl, void* buff, size_t len) {
    ssize_t r = 0;
    while ((unsigned long)r < len) {
        ssize_t n = mbedtls_ssl_read(ssl, (unsigned char*)buff + r, (len-r) > 16384? 16384 : (len-r));
        if (n < 0) {
            printf("ERROR reading from socket\n");
            exit(1);
        }
        r+=n;
    }

    return r;
}

void untrusted_util::init_secure_connection(secure_connection** connection, const char* server_name, const int server_port) {
    secure_conn_t* conn = (secure_conn_t*)malloc(sizeof(secure_conn_t));
    *connection = (secure_connection*)conn;

    // put port in char buf
    char port[5];
    sprintf(port, "%d", server_port);

    int ret;
    uint32_t flags;

    // initialise the RNG and the session data
    mbedtls_net_init(&conn->server_fd);
    mbedtls_ssl_init(&conn->ssl);
    mbedtls_ssl_config_init(&conn->conf);
    mbedtls_x509_crt_init(&conn->cacert);
    mbedtls_ctr_drbg_init(&conn->ctr_drbg);

    printf("seeding the random number generator...\n");
    mbedtls_entropy_init(&conn->entropy);
    const char* pers = "ssl_client1";
    if ((ret = mbedtls_ctr_drbg_seed(&conn->ctr_drbg, mbedtls_entropy_func, &conn->entropy, (const unsigned char*)pers, strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        exit(1);
    }

    // initialise certificates
    printf("loading the CA root certificate...\n");
    ret = mbedtls_x509_crt_parse(&conn->cacert, (const unsigned char*)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
    if (ret < 0) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        exit(1);
    }

    printf("ok (%d skipped)\n", ret);

    // start connection
    printf("connecting to tcp/%s/%s...\n", server_name, port);
    if ((ret = mbedtls_net_connect(&conn->server_fd, server_name, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        exit(1);
    }

    printf("setting up the SSL/TLS structure...\n");
    if ((ret = mbedtls_ssl_config_defaults(&conn->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        exit(1);
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&conn->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conn->conf, &conn->cacert, NULL);
    mbedtls_ssl_conf_rng(&conn->conf, mbedtls_ctr_drbg_random, &conn->ctr_drbg);
    mbedtls_ssl_conf_dbg(&conn->conf, my_debug, stdout);

    if ((ret = mbedtls_ssl_setup(&conn->ssl, &conn->conf)) != 0) {
        printf("failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        exit(1);
    }

    if ((ret = mbedtls_ssl_set_hostname(&conn->ssl, server_name)) != 0) {
        printf("failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        exit(1);
    }

    mbedtls_ssl_set_bio(&conn->ssl, &conn->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    printf("performing the SSL/TLS handshake...\n");
    while ((ret = mbedtls_ssl_handshake(&conn->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
            exit(1);
        }
    }

    // verify the server certificate
    printf("verifying peer X.509 certificate...\n");

    // in real life, we probably want to bail out when ret != 0
    if ((flags = mbedtls_ssl_get_verify_result(&conn->ssl)) != 0) {
        printf("failed\n");

        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "! ", flags);
        printf("%s\n", vrfy_buf);
    }
}

void untrusted_util::socket_secure_send(secure_connection* connection, const void* buff, size_t len) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    if (ssl_send_all(&conn->ssl, buff, len) < 0) {
        printf("ERROR writing to socket\n");
        exit(1);
    }
}

void untrusted_util::socket_secure_receive(secure_connection* connection, void* buff, size_t len) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    if (ssl_receive_all(&conn->ssl, buff, len) < 0) {
        printf("EERROR reading from socket\n");
        exit(1);
    }
}

void untrusted_util::close_secure_connection(secure_connection* connection) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    mbedtls_ssl_close_notify(&conn->ssl);

    // ssl cleanup
    mbedtls_net_free(&conn->server_fd);
    mbedtls_ssl_free(&conn->ssl);
    mbedtls_ssl_config_free(&conn->conf);
    mbedtls_ctr_drbg_free(&conn->ctr_drbg);
    mbedtls_entropy_free(&conn->entropy);

    free(connection);
}
