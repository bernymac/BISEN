#ifndef VISEN_UTIL_H
#define VISEN_UTIL_H

#include "untrusted_util.h"

#include <stdlib.h>
#include <vector>
#include <string>

std::vector<std::string> list_img_files(int limit, std::string dataset_path);
std::vector<std::string> list_img_files_rec(int limit, std::string dataset_path);
std::vector<std::string> list_txt_files(int limit, std::string dataset_path);

void iee_send(secure_connection* conn, const uint8_t* in, const size_t in_len);
void iee_recv(secure_connection* conn, uint8_t** out, size_t* out_len);
void iee_comm(secure_connection* conn, const void* in, const size_t in_len);
void print_bytes(const char* msg);
void reset_bytes();

#endif //VISEN_UTIL_H
