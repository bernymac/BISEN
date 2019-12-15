#include "util.h"

#include <dirent.h>
#include <algorithm>
#include <string.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

int is_dir(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

std::vector<std::string> list_img_files(int limit, string dataset_path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(dataset_path.c_str()))) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fname = dataset_path + ent->d_name;
            if (fname.find(".jpg") == string::npos || !fname.length())
                continue;

            filenames.push_back(fname);
        }
        closedir(dir);
    } else {
        printf("SseClient::listTxtFiles couldn't open dataset dir.\n");
        exit(1);
    }

    // sort alphabetically
    sort(filenames.begin(), filenames.end());

    // remove elements in excess
    if(limit > 0 && filenames.size() > (unsigned)limit)
        filenames.erase(filenames.begin() + limit, filenames.end());

    return filenames;
}

std::vector<std::string> list_img_files_rec(int limit, string dataset_path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(dataset_path.c_str()))) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fname = dataset_path + ent->d_name;
            if (is_dir(fname.c_str()) && strcmp(fname.c_str(), ".") && strcmp(fname.c_str(), "..")) {
                vector<string> nf = list_img_files(-1, fname + '/');
                filenames.insert(filenames.end(), nf.begin(), nf.end());
            }
        }
        closedir(dir);
    } else {
        printf("SseClient::listTxtFiles couldn't open dataset dir.\n");
        exit(1);
    }

    // sort alphabetically
    sort(filenames.begin(), filenames.end());

    // remove elements in excess
    if(limit > 0 && filenames.size() > (unsigned)limit)
        filenames.erase(filenames.begin() + limit, filenames.end());

    return filenames;
}

std::vector<std::string> list_txt_files(int limit, std::string path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str()))) {
        while ((ent = readdir(dir))) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || ent->d_name[0] == '.')
                continue;

            string fname = ent->d_name;
            const size_t pos = fname.find(".txt");
            if (pos != string::npos)
                filenames.push_back(path + fname);
        }
        closedir(dir);
    } else {
        printf("SseClient::listTxtFiles couldn't open dataset dir.\n");
        exit(1);
    }

    // sort alphabetically
    sort(filenames.begin(), filenames.end());

    // remove elements in excess
    if(limit > 0 && filenames.size() > (unsigned)limit)
        filenames.erase(filenames.begin() + limit, filenames.end());

    return filenames;
}

size_t bytes_sent = 0, bytes_received = 0;

void iee_send(secure_connection* conn, const uint8_t* in, const size_t in_len) {
    //printf("will send %lu\n", in_len);
    untrusted_util::socket_secure_send(conn, &in_len, sizeof(size_t));
    untrusted_util::socket_secure_send(conn, in, in_len);

    bytes_sent += sizeof(size_t) + in_len;
}

void iee_recv(secure_connection* conn, uint8_t** out, size_t* out_len) {
    untrusted_util::socket_secure_receive(conn, out_len, sizeof(size_t));

    //printf("will receive %lu\n", *out_len);

    *out = (uint8_t*)malloc(*out_len);
    untrusted_util::socket_secure_receive(conn, *out, *out_len);

    bytes_received += sizeof(size_t) + *out_len;
}

void print_bytes(const char* msg) {
    printf("Sent bytes %s client: %lu\nReceived bytes client: %lu\n", msg, bytes_sent, bytes_received);
}

void reset_bytes() {
    bytes_sent = 0;
    bytes_received = 0;
}

void iee_comm(secure_connection* conn, const void* in, const size_t in_len) {
    iee_send(conn, (uint8_t*)in, in_len);
    size_t res_len;
    unsigned char* res;
    iee_recv(conn, &res, &res_len);

    //printf("res: %lu bytes\n", res_len);
    free(res);
}
