#ifndef _ENCLAVE_HANDLER_H_
#define _ENCLAVE_HANDLER_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "sgx_error.h"       /* sgx_status_t */
#include "sgx_eid.h"     /* sgx_enclave_id_t */

#include "sgx_urts.h"
#include "sgx_uae_service.h"
#include "Enclave_u.h"

#include "ocall.h"

#define MAX_PATH FILENAME_MAX

#if   defined(__GNUC__)
#define TOKEN_FILENAME   ENCLAVE_TOKEN_NAME
#define ENCLAVE_FILENAME SIGNED_ENCLAVE_NAME
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void print_error_message(sgx_status_t ret);

int initialise_enclave(sgx_enclave_id_t* eid);
void destroy_enclave(sgx_enclave_id_t eid);

#if defined(__cplusplus)
}
#endif

#endif
