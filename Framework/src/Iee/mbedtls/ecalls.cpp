#include "Enclave_t.h"
#include "enc.h"
#include "ssl_conn_hdlr.h"

#ifdef __cplusplus
extern "C" {
#endif

void ssl_conn_init();
void ssl_conn_teardown();
void ssl_conn_handle(long int thread_id, thread_info_t *thread_info);

#ifdef __cplusplus
}
#endif

TLSConnectionHandler* connectionHandler;

void ssl_conn_init(void) {
  connectionHandler = new TLSConnectionHandler();
}

void ssl_conn_handle(long int thread_id, thread_info_t* thread_info) {
  connectionHandler->handle(thread_id, thread_info);
}

void ssl_conn_teardown(void) {
  delete connectionHandler;
}
