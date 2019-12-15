#include "ssl_conn_hdlr.h"

#include "outside_util.h"
#include "extern_lib.h"

#include <exception>
#include <mbedtls/net.h>
#include <mbedtls/debug.h>
#include "Enclave.h"

TLSConnectionHandler::TLSConnectionHandler() {
    int ret;

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    unsigned char alloc_buf[100000];
#endif
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init( alloc_buf, sizeof(alloc_buf) );
#endif

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init(&cache);
#endif

    mbedtls_x509_crt_init(&srvcert);
    mbedtls_x509_crt_init(&cachain);

    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // we use only a single entropy source that is used in all the threads.
    mbedtls_entropy_init(&entropy);

    /*
     * 1. Load the certificates and private RSA key
     */
    outside_util::printf("loading the server cert and key...\n");

    /*
     * This demonstration program uses embedded test certificates.
     * Instead, you may want to use mbedtls_x509_crt_parse_file() to read the
     * server and CA certificates, as well as mbedtls_pk_parse_keyfile().
     */
    ret = mbedtls_x509_crt_parse(&srvcert, (const unsigned char*)mbedtls_test_srv_crt, mbedtls_test_srv_crt_len);
    if (ret != 0) {
        outside_util::printf("failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
        outside_util::exit(1);
    }

    ret = mbedtls_x509_crt_parse(&cachain, (const unsigned char*)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
    if (ret != 0) {
        outside_util::printf("failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
        outside_util::exit(1);
    }

    mbedtls_pk_init(&pkey);
    ret = mbedtls_pk_parse_key(&pkey, (const unsigned char*)mbedtls_test_srv_key, mbedtls_test_srv_key_len, NULL, 0);
    if (ret != 0) {
        outside_util::printf("failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
        outside_util::exit(1);
    }

    // seed the random number generator
    outside_util::printf("Seeding the random number generator\n");
    std::string pers = "ssl_pthread_server";
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers.c_str(), pers.length())) != 0) {
        outside_util::printf(" failed: mbedtls_ctr_drbg_seed returned -0x%04x\n", -ret);
        throw std::runtime_error("");
    }

    // prepare SSL configuration
    outside_util::printf("Setting up the SSL data...\n");
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        outside_util::printf(" failed: mbedtls_ssl_config_defaults returned -0x%04x\n", -ret);
        throw std::runtime_error("");
    }

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    // setup debug
    mbedtls_ssl_conf_dbg(&conf, mydebug, NULL);
    // if debug_level is not set (could be set via other constructors), set it to 0
    if (debug_level < 0) {
        debug_level = 0;
    }
    mbedtls_debug_set_threshold(debug_level);

    /* mbedtls_ssl_cache_get() and mbedtls_ssl_cache_set() are thread-safe if
     * MBEDTLS_THREADING_C is set.
     */
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_conf_session_cache(&conf, &cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
#endif

    mbedtls_ssl_conf_ca_chain(&conf, &cachain, NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey)) != 0) {
        outside_util::printf("failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        throw std::runtime_error("");
    }
}

TLSConnectionHandler::~TLSConnectionHandler() {
    mbedtls_x509_crt_free(&srvcert);
    mbedtls_pk_free(&pkey);
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free(&cache);
#endif
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_ssl_config_free(&conf);

    sgx_thread_mutex_destroy(&mutex);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_free();
#endif

}

int read_ssl(mbedtls_ssl_context ssl, void* buf, size_t len) {
    size_t read_len;
    int ret = mbedtls_ssl_read(&ssl, (unsigned char*)buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        return -1;

    if (ret <= 0) {
        switch (ret) {
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                outside_util::printf("  [ # ]  connection was closed gracefully\n");
                return -2;

            case MBEDTLS_ERR_NET_CONN_RESET:
                outside_util::printf("  [ # ]  connection was reset by peer\n");
                return -2;

            default:
                outside_util::printf("  [ # ]  mbedtls_ssl_read returned -0x%04x\n", -ret);
                return -2;
        }
    }

    return ret;
}

int write_ssl(mbedtls_ssl_context ssl, const void* buf, size_t len) {
    int ret;
    while ((ret = mbedtls_ssl_write(&ssl, (unsigned char*)buf, len)) <= 0) {
        if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
            outside_util::printf("  [ # ]  failed: peer closed the connection\n");
            return -2;
        }

        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            outside_util::printf("  [ # ]  failed: mbedtls_ssl_write returned -0x%04x\n", ret);
            return -2;
        }
    }

    return ret;
}

void TLSConnectionHandler::handle(long thread_id, thread_info_t* thread_info) {
    mbedtls_net_context* client_fd = &thread_info->client_fd;
    //unsigned char buf[1024];
    mbedtls_ssl_context ssl;

    // thread local data
    mbedtls_ssl_config conf;
    memcpy(&conf, &this->conf, sizeof(mbedtls_ssl_config));
    thread_info->config = &conf;
    thread_info->thread_complete = 0;

    // make sure memory references are valid
    mbedtls_ssl_init(&ssl);

    outside_util::printf("  [ #%ld ]  Setting up SSL/TLS data\n", thread_id);

    // get the SSL context ready
    int ret;
    if ((ret = mbedtls_ssl_setup(&ssl, thread_info->config)) != 0) {
        outside_util::printf("  [ #%ld ]  failed: mbedtls_ssl_setup returned -0x%04x\n", thread_id, -ret);
        goto thread_exit;
    }

    outside_util::printf("client_fd is %d\n", client_fd->fd);
    mbedtls_ssl_set_bio(&ssl, client_fd, mbedtls_net_send_ocall, mbedtls_net_recv_ocall, NULL);

    // handshake
    outside_util::printf("performing the SSL/TLS handshake\n");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            outside_util::printf("  [ #%ld ]  failed: mbedtls_ssl_handshake returned -0x%04x\n", thread_id, -ret);
            goto thread_exit;
        }
    }

    // start processing client requests
    while (1) {
        size_t in_len;
        int len = read_ssl(ssl, &in_len, sizeof(size_t));
        if (len == -1) {
            continue;
        } else if (len == -2) {
            outside_util::printf("end of client thread\n");
            break;
        }

        //outside_util::printf("received length %lu\n", in_len);

        void* in_buffer = malloc(in_len);
        //outside_util::printf("%p\n", in_buffer);

        size_t to_read = in_len;
        size_t read = 0;
        while(to_read) {
            len = read_ssl(ssl, (uint8_t*)in_buffer + read, (to_read > 16384? 16384 : to_read));
            if (len < 0) {
                outside_util::printf("error\n");
                outside_util::exit(1);
            }

            to_read -= len;
            read += len;
        }

        // execute extern lib processing
        uint8_t* out;
        size_t out_len;
        extern_lib::process_message(&out, &out_len, (const uint8_t *)in_buffer, (const size_t)in_len);
        free(in_buffer);

        // send to client
        len = write_ssl(ssl, &out_len, sizeof(size_t));
        if (len < 0) {
            outside_util::printf("error\n");
            outside_util::exit(1);
        }

        //outside_util::printf("sent len %lu\n", out_len);

        size_t to_send = out_len;
        size_t sent = 0;
        while(to_send) {
            len = write_ssl(ssl, out + sent, (to_send > 16384? 16384 : to_send));
            if (len < 0) {
                outside_util::printf("error\n");
                outside_util::exit(1);
            }

            to_send -= len;
            sent += len;
        }

        free(out);
    }

    while ((ret = mbedtls_ssl_close_notify(&ssl)) < 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            outside_util::printf("  [ #%ld ]  failed: mbedtls_ssl_close_notify returned -0x%04x\n", thread_id, ret);
            goto thread_exit;
        }
    }

    ret = 0;

    thread_exit:

#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        outside_util::printf("  [ #%ld ]  Last error was: -0x%04x - %s\n\n", thread_id, -ret, error_buf);
    }
#endif

    mbedtls_ssl_free(&ssl);

    thread_info->config = NULL;
    thread_info->thread_complete = 1;
}

sgx_thread_mutex_t TLSConnectionHandler::mutex = SGX_THREAD_MUTEX_INITIALIZER;

void TLSConnectionHandler::mydebug(void* ctx, int level, const char* file, int line, const char* str) {
    (void)ctx;
    (void)level;
    long int thread_id = 0;
    sgx_thread_mutex_lock(&mutex);

    outside_util::printf("%s:%04d: [ #%ld ] %s", file, line, thread_id, str);

    sgx_thread_mutex_unlock(&mutex);
}
