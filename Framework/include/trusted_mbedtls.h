#ifndef TRUSTED_MBEDTLS_H
#define TRUSTED_MBEDTLS_H

#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
// TODO really need this?
typedef struct {
    mbedtls_net_context client_fd;
    int thread_complete;
    mbedtls_ssl_config *config;
} thread_info_t;

#endif
