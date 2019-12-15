#include "SseIee.h"

#include <math.h>
#include <algorithm>
#include <vector>
#include "outside_util.h"
#include "trusted_util.h"
#include "trusted_crypto.h"
#include "definitions.h"
#include "outside_util.h"

#include "IeeUtils.h"
#include "QueryEvaluator.h"

#include "vec_int.h"
#include "vec_token.h"

#define MAX_BATCH_UPDATE 1000
#define MAX_BATCH_SEARCH 1000

using namespace std;

static void init_pipes();
//static void destroy_pipes();
static void benchmarking_print(int aggregate); // only for benchmarking, prints stats
static void setup(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void update(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void search(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void get_docs_from_server(vec_token *query, const unsigned count_words, const unsigned total_labels);

typedef struct search_res {
    double total_iee = 0;
    double wait_uee = 0;
    double boolean_resolution = 0;
    double buffer_alloc = 0;
    double sort_req = 0;
    double gen_req = 0;
    double read_req = 0;
    double scoring = 0;
    size_t docs_retrieved = 0;
} search_res;
static vector<search_res> search_results;

static int server_socket;
static unsigned last_ndocs;
static unsigned char* aux_bool;

static uint8_t* kEnc;

double total_iee_add = 0, total_server_add = 0;
size_t count_add = 0;

// IEE entry point
void f(bytes* out, size* out_len, const unsigned long long pid, uint8_t* in, const size in_len) {
    #ifdef VERBOSE
    //ocall_strprint("\n***** Entering IEE *****\n");
    #endif

    // set out variables
    *out = NULL;
    *out_len = 0;

    //setup operation
    if(in[0] == OP_SETUP)
        setup(out, out_len, in, in_len);
    //add / update operation
    else if (in[0] == OP_ADD)
        update(out, out_len, in, in_len);
    //search operation
    else if (in[0] == OP_SRC)
        search(out, out_len, in, in_len);
    else if (in[0] == OP_IEE_DUMP_BENCH)
        benchmarking_print(in[1]);

    #ifdef VERBOSE
    //ocall_strprint("\n***** Leaving IEE *****\n\n");
    #endif
}

void benchmarking_print(int aggregate) {
    // this instruction can be safely removed if wanted

    // add stats
    outside_util::printf("-- BISEN add iee: %lf ms (%lu docs)--\n", total_iee_add, count_add);
    outside_util::printf("-- BISEN add uee w/ net: %lf ms --\n", total_server_add);

    if(aggregate) {
        const size_t nr_searches = search_results.size();
        outside_util::printf("-- AVG BISEN IEE %lu SEARCHES --\n", nr_searches);
        outside_util::printf("TOTAL_iee\tTOTAL_storage_w_net\tSCORING\tBOOLEAN\tDOCS_BEFORE_TRIM\tBUF_ALLOC\tSORT_REQ\tGEN_REQ\tREAD_REQ\n");

        search_res agg;
        for (search_res r : search_results) {
            agg.total_iee += r.total_iee;
            agg.wait_uee += r.wait_uee;
            agg.scoring += r.scoring;
            agg.boolean_resolution += r.boolean_resolution;
            agg.docs_retrieved += r.docs_retrieved;
            agg.buffer_alloc += r.buffer_alloc;
            agg.sort_req += r.sort_req;
            agg.gen_req += r.gen_req;
            agg.read_req += r.read_req;
        }

        agg.total_iee /= nr_searches;
        agg.wait_uee /= nr_searches;
        agg.scoring /= nr_searches;
        agg.boolean_resolution /= nr_searches;
        agg.docs_retrieved /= nr_searches;
        agg.buffer_alloc /= nr_searches;
        agg.sort_req /= nr_searches;
        agg.gen_req /= nr_searches;
        agg.read_req /= nr_searches;

        outside_util::printf("%lf\t%lf\t%lf\t%lf\t%lu\t%lf\t%lf\t%lf\t%lf\n", agg.total_iee, agg.wait_uee, agg.scoring, agg.boolean_resolution, agg.docs_retrieved, agg.buffer_alloc, agg.sort_req, agg.gen_req, agg.read_req);
    } else {
        outside_util::printf("-- BISEN IEE SEARCHES --\n");
        outside_util::printf("NR\tTOTAL_iee\tTOTAL_storage_w_net\tSCORING\tBOOLEAN\tDOCS_BEFORE_TRIM\tBUF_ALLOC\tSORT_REQ\tGEN_REQ\tREAD_REQ\n");

        unsigned count = 0;
        for (search_res r : search_results) {
            outside_util::printf("%u\t%lf\t%lf\t%lf\t%lf\t%lu\t%lf\t%lf\t%lf\t%lf\n", count++, r.total_iee, r.wait_uee, r.scoring, r.boolean_resolution, r.docs_retrieved, r.buffer_alloc, r.sort_req, r.gen_req, r.read_req);
        }
    }

    // BENCHMARK : tell server to print statistics
    uint8_t op[2];
    op[0] = OP_UEE_DUMP_BENCH;
    op[1] = aggregate;
    outside_util::socket_send(server_socket, op, 2 * sizeof(uint8_t));

    // reset
    search_results.clear();
}

void search_start_benchmark_msg() {
    // this instruction is FOR BENCHMARKING ONLY can be safely removed if wanted
    // BENCHMARK : tell server to start search
    const uint8_t op = '5';
    outside_util::socket_send(server_socket, &op, sizeof(uint8_t));
}

void init_pipes() {
    server_socket = outside_util::open_socket("localhost", UEE_BISEN_PORT);
    outside_util::printf("Finished IEE init! Gonna start listening for client requests through bridge!\n");
}

static void setup(bytes* out, size* out_len, uint8_t* in, const size in_len) {
#ifdef VERBOSE
    ocall_print_string("IEE: Starting Setup!\n");
#endif

    init_pipes();
    void* tmp = in + 1; // exclude op

    // read kEnc
    size_t kEnc_size;
    memcpy(&kEnc_size, tmp, sizeof(size_t));
    tmp = (char*)tmp + sizeof(size_t);

    kEnc = (unsigned char*)malloc(AES_KEY_SIZE);
    memset(kEnc, 0x00, AES_KEY_SIZE);
    tmp = (char*)tmp + kEnc_size;

    //tmp = (char*)tmp + kF_size;

    /*printf("kEnc size %d\n", kEnc_size);
    printf("kF size %d\n", kF_size);*/

    // tell server to init index I
    const uint8_t op = OP_UEE_INIT;
    outside_util::socket_send(server_socket, &op, sizeof(uint8_t));

    // init aux_bool buffer
    aux_bool = NULL;
    last_ndocs = 0;

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;

#ifdef VERBOSE
    ocall_print_string("IEE: Finished Setup!\n");
#endif
}

static void update(bytes *out, size *out_len, uint8_t* in, const size in_len) {
#ifdef VERBOSE
    //ocall_print_string("IEE: Started add!\n");
#endif

    untrusted_time start = outside_util::curr_time();

    // read buffer
    void* tmp = in + 1; // exclude op
    const size_t to_recv_len = in_len - sizeof(unsigned char);

    // size of single buffers
    const size_t recv_len = SHA256_OUTPUT_SIZE + 3 * sizeof(int);
    const size_t d_unenc_len = SHA256_OUTPUT_SIZE + sizeof(size_t) + sizeof(int);
    const size_t d_enc_len = d_unenc_len;

    // size of batch requests to server
    size_t to_recv = to_recv_len / recv_len;

    // buffers for reuse while receiving
    unsigned char* unenc_data = (unsigned char*)malloc(d_unenc_len);

    // will contain at most max_batch_size (l,id*) pairs
    const size_t pair_size = SHA256_OUTPUT_SIZE + d_enc_len;
    void* batch_buffer = malloc(MAX_BATCH_UPDATE * pair_size);

    total_iee_add += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

    while(to_recv) {
        start = outside_util::curr_time();
        memset(batch_buffer, 0, MAX_BATCH_UPDATE * (SHA256_OUTPUT_SIZE + d_enc_len)); // fix syscall param write(buf) points to uninitialised byte(s)

        // calculate size and get batch from client
        size_t batch_size = std::min(to_recv, (size_t)MAX_BATCH_UPDATE);
        for (unsigned i = 0; i < batch_size; i++) {
            void* label = (uint8_t*)batch_buffer + i * pair_size;
            void* d = (uint8_t*)label + SHA256_OUTPUT_SIZE;

            //get doc_id, counter, frequency, kW from array
            int doc_id;
            memcpy(&doc_id, tmp, sizeof(int));
            tmp = ((char*)tmp) + sizeof(int);
            size_t id = (size_t)doc_id; // TODO should already come from the client as size_t

            int counter;
            memcpy(&counter, tmp, sizeof(int));
            tmp = ((char*)tmp) + sizeof(int);

            int frequency;
            memcpy(&frequency, tmp, sizeof(int));
            tmp = ((char*)tmp) + sizeof(int);

            // read kW
            uint8_t kW[SHA256_OUTPUT_SIZE];
            memcpy(kW, tmp, SHA256_OUTPUT_SIZE);
            tmp = ((char*)tmp) + SHA256_OUTPUT_SIZE;

            // calculate "label" (key) and add to batch_buffer
            tcrypto::hmac_sha256((unsigned char*)label, (unsigned char*)&counter, sizeof(int), kW, SHA256_OUTPUT_SIZE);

            // calculate "id*" (entry)
            // hmac + frequency + doc_id
            memcpy(unenc_data, label, SHA256_OUTPUT_SIZE);
            memcpy(unenc_data + SHA256_OUTPUT_SIZE, &frequency, sizeof(int));
            memcpy(unenc_data + SHA256_OUTPUT_SIZE + sizeof(int), &id, sizeof(size_t));

            // store in batch_buffer
            unsigned char ctr[AES_BLOCK_SIZE];
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            unsigned char k[AES_KEY_SIZE];
            memset(k, 0x00, AES_KEY_SIZE);
            tcrypto::encrypt(d, unenc_data, d_unenc_len, k, ctr);
        }

        total_iee_add += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

        start = outside_util::curr_time();

        // send batch to server
        const unsigned char op = OP_UEE_ADD;
        outside_util::socket_send(server_socket, &op, sizeof(unsigned char));
        outside_util::socket_send(server_socket, &batch_size, sizeof(size_t));
        outside_util::socket_send(server_socket, batch_buffer, batch_size * pair_size);

        total_server_add += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

        to_recv -= batch_size;
    }

    start = outside_util::curr_time();

    free(unenc_data);
    free(batch_buffer);

#ifdef VERBOSE
    //ocall_print_string("Finished update in IEE!\n");
#endif

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;

    total_iee_add += trusted_util::time_elapsed_ms(start, outside_util::curr_time());
    count_add++;
}

typedef struct {
    iee_token *tkn;
    unsigned counter_val;
} label_request;

// used to hold all labels in random order
label_request* labels = NULL;
size_t labels_size = 0;

void get_docs_from_server(vec_token *query, const unsigned count_words, const unsigned total_labels) {
#ifdef VERBOSE
    ocall_print_string("Requesting docs from server!\n");
#endif

    untrusted_time start, start2;
    start = outside_util::curr_time();
    start2 = outside_util::curr_time();
    if (total_labels > labels_size) {
        labels = (label_request*)realloc(labels, sizeof(label_request) * total_labels);
        labels_size = total_labels;
        //outside_util::printf("reallocate %lu %u %p\n", sizeof(label_request) * total_labels, total_labels, labels);
    }

    memset(labels, 0x00, sizeof(label_request) * total_labels);

    search_results.back().buffer_alloc += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());

    start2 = outside_util::curr_time();
    // iterate over all the needed words, and then over all its occurences (given by the counter), and fills the requests array
    int k = 0;
    for(unsigned i = 0; i < vt_size(query); i++) {
        // ignore non-word tokens
        if(query->array[i].type != WORD_TOKEN)
            continue;

        // fisher-yates shuffle
        for(unsigned j = 0; j < query->array[i].counter; j++) {
            int r = 0;//tcrypto::random_uint_range(0, k+1);
            if(r != k) {
                labels[k].tkn = labels[r].tkn;
                labels[k].counter_val = labels[r].counter_val;
            }

            labels[r].tkn = &(query->array[i]);
            labels[r].counter_val = j;

            k++;
        }
    }

    search_results.back().sort_req += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());

#ifdef VERBOSE
    ocall_print_string("Randomised positions!\n");
#endif
    start2 = outside_util::curr_time();
    /************************ ALLOCATE DATA STRUCTURES ************************/
    // buffer for server requests
    // (always max_batch_size, may not be filled if not needed)
    // op + batch_size + labels of H_BYTES len
    const size_t req_len = SHA256_OUTPUT_SIZE;
    unsigned char* req_buff = (unsigned char*)malloc(sizeof(char) + sizeof(size_t) + req_len * MAX_BATCH_SEARCH);

    // put the op code in the buffer
    const uint8_t op = OP_UEE_SEARCH;
    memcpy(req_buff, &op, sizeof(uint8_t));

    // buffer for encrypted server responses
    // contains the hmac for verif, the doc id, and the encryption's exp
    const size_t res_len = SHA256_OUTPUT_SIZE + sizeof(size_t) + sizeof(int); // 44 + H_BYTES (32)
    unsigned char* res_buff = (unsigned char*)malloc(res_len * MAX_BATCH_SEARCH);
    search_results.back().buffer_alloc += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());

    /********************** END ALLOCATE DATA STRUCTURES **********************/

    unsigned label_pos = 0;
    size_t batch_size = std::min((size_t)(total_labels - label_pos), (size_t)MAX_BATCH_SEARCH);

    search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

    // request labels to server
    while (label_pos < total_labels) {
        start = outside_util::curr_time();
        start2 = outside_util::curr_time();

        // put batch_size in buffer
        memcpy(req_buff + sizeof(uint8_t), &batch_size, sizeof(size_t));
        search_results.back().buffer_alloc += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());

        // aux pointer
        uint8_t* tmp = req_buff + sizeof(uint8_t) + sizeof(size_t);
        start2 = outside_util::curr_time();

        // fill the buffer with labels
        for(unsigned i = 0; i < batch_size; i++) {
            label_request* req = &(labels[label_pos + i]);
            tcrypto::hmac_sha256(tmp + i * req_len, (unsigned char*)&(req->counter_val), sizeof(int), req->tkn->kW, SHA256_OUTPUT_SIZE);
        }

        /*printf("%d batch\n", batch_size);
        for(unsigned x = 0; x < batch_size*req_len; x++)
            printf("%02x", tmp[x]);
        printf(" : \n");*/

        search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());
        search_results.back().gen_req += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());
        // send message to server and receive response
        start = outside_util::curr_time();
        outside_util::socket_send(server_socket, req_buff, sizeof(uint8_t) + sizeof(size_t) + req_len * batch_size);
        outside_util::socket_receive(server_socket, res_buff, res_len * batch_size);
        search_results.back().wait_uee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());
        start2 = outside_util::curr_time();
        // decrypt and fill the destination data structs
        start = outside_util::curr_time();
        for(unsigned i = 0; i < batch_size; i++) {
            uint8_t dec_buff[SHA256_OUTPUT_SIZE + sizeof(size_t) + sizeof(int)];

            label_request* req = &labels[label_pos + i];

            unsigned char ctr[AES_BLOCK_SIZE];
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            unsigned char k[AES_KEY_SIZE];
            memset(k, 0x00, AES_KEY_SIZE);

            tcrypto::decrypt(dec_buff, res_buff + (res_len * i), res_len, k, ctr);

            // aux pointer for req
            uint8_t* tmp_req = req_buff + sizeof(uint8_t) + sizeof(size_t);
            const uint8_t* label_verification = tmp_req + i * req_len;

            // verify
            if(memcmp(dec_buff, label_verification, SHA256_OUTPUT_SIZE)) {
                for(unsigned x = 0; x < SHA256_OUTPUT_SIZE; x++)
                    outside_util::printf("%02x", label_verification[x]);
                outside_util::printf(" : exp\n");

                for(unsigned x = 0; x < SHA256_OUTPUT_SIZE; x++)
                    outside_util::printf("%02x", dec_buff[x]);
                outside_util::printf(" : got\n");

                outside_util::printf("Label verification doesn't match! Exit\n");
                outside_util::exit(-1);
            }

            size_t doc_id;

            memcpy(&req->tkn->doc_frequencies.array[req->counter_val], dec_buff + SHA256_OUTPUT_SIZE, sizeof(int));
            memcpy(&doc_id, dec_buff + SHA256_OUTPUT_SIZE + sizeof(int), sizeof(size_t));

            req->tkn->docs.array[req->counter_val] = (int)doc_id; // TODO should also be size_t, this converts it to the iee representation as int

            //outside_util::printf("doc %d\n", req->tkn->docs.array[req->counter_val]);
        }
        search_results.back().read_req += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());
        label_pos += batch_size;
        batch_size = std::min(total_labels - label_pos, (unsigned)MAX_BATCH_SEARCH);

        search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());
    }

    start = outside_util::curr_time();
    //free(labels);

    memset(req_buff, 0, req_len * MAX_BATCH_SEARCH);
    memset(res_buff, 0, res_len * MAX_BATCH_SEARCH);

    free(req_buff);
    free(res_buff);

    search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

#ifdef VERBOSE
    ocall_print_string("Got all docs from server!\n\n");
#endif
}

#if BISEN_SCORING
#define PAIR_LEN (sizeof(int) + sizeof(double))
#else
#define PAIR_LEN (sizeof(int))
#endif

#if BISEN_SCORING
void calc_idf(vec_token *query, const unsigned total_docs) {
    for(unsigned i = 0; i < vt_size(query); i++) {
        iee_token* t = &query->array[i];

        if(t->type != 'w')
            continue;

        unsigned nr_docs = vi_size(&t->docs);
        t->idf = !nr_docs ? 0 : log10((double)total_docs / nr_docs);// nr_docs should have +1 to avoid 0 division

        //outside_util::printf("idf %c %f %lu %lu\n", t->type, t->idf, nr_docs, total_docs);
    }
}


void calc_tfidf(vec_token *query, vec_int* response_docs, void* results) {
    uint8_t* tmp = (uint8_t*)results;

    //iterate over the response docs
    for(unsigned i = 0; i < vi_size(response_docs); i++) {
        int curr_doc = response_docs->array[i];
        double tf_idf = 0;

        // for each response doc, iterate over all terms and find who uses them
        for(unsigned j = 0; j < vt_size(query); j++) {
            iee_token* t = &query->array[j];

            if(t->type != 'w')
                continue;

            int pos = vi_index_of(t->docs, curr_doc);
            //outside_util::printf("pos %d; ", pos);
            if(pos != -1)
                tf_idf += t->doc_frequencies.array[pos] * t->idf;
        }

        // store tf-idf in result buffer
        memcpy(tmp, &curr_doc, sizeof(int));
        memcpy(tmp + sizeof(int), &tf_idf, sizeof(double));
        tmp += PAIR_LEN;
    }
}

int compare_results_rbisen(const void *a, const void *b) {
    double d_a = *((const double*) ((uint8_t*)a + sizeof(int)));
    double d_b = *((const double*) ((uint8_t*)b + sizeof(int)));

    if (d_a == d_b)
        return 0;
    else
        return d_a < d_b ? 1 : -1;
}
#endif

void search(bytes* out, size* out_len, uint8_t* in, const size in_len) {
    //outside_util::printf("## BISEN Search %lu ##\n", search_results.size());
    search_start_benchmark_msg();

    search_res current_src;
    search_results.push_back(current_src);

    untrusted_time start;
    start = outside_util::curr_time();

    vec_token query;
    vt_init(&query, MAX_QUERY_TOKENS);

    unsigned nDocs = 0;
    unsigned count_words = 0; // useful for get_docs_from_server
    unsigned count_labels = 0; // useful for get_docs_from_server

    //read in
    int pos = 1;
    while(pos < in_len) {
        iee_token tkn;
        tkn.kW = NULL;

        iee_readFromArr(&tkn.type, 1, in, &pos);

        if(tkn.type == WORD_TOKEN) {
            count_words++;

            // read counter
            tkn.counter = (unsigned)iee_readIntFromArr(in, &pos);
            count_labels += tkn.counter;

            // read kW
            tkn.kW = (unsigned char*)malloc(SHA256_OUTPUT_SIZE);
            iee_readFromArr(tkn.kW, SHA256_OUTPUT_SIZE, in, &pos);

            // create the vector that will hold the docs
            vi_init(&tkn.docs, tkn.counter);
            vi_init(&tkn.doc_frequencies, tkn.counter);
            tkn.docs.counter = tkn.counter;
        } else if(tkn.type == META_TOKEN) {
            nDocs = (unsigned)iee_readIntFromArr(in, &pos);
            continue;
        }

        vt_push_back(&query, &tkn);
    }

    search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

    // get documents from uee
    get_docs_from_server(&query, count_words, count_labels);

#ifdef VERBOSE
    /*ocall_strprint("parsed: ");
    for(unsigned i = 0; i < vt_size(query); i++) {
        iee_token x = query.array[i];
        if(x.type == WORD_TOKEN) {
            ocall_printf("%s (", x.word);
            for(unsigned i = 0; i < vi_size(x.docs); i++) {
                if(i < vi_size(x.docs) - 1)
                    ocall_printf("%i,", x.docs.array[i]);
                else
                    ocall_printf("%i); ", x.docs.array[i]);
            }
        } else {
            ocall_printf("%c ", x.type);
        }
    }
    ocall_print_string("\n\n");*/
#endif

    start = outside_util::curr_time();

    // ensure aux_bool has space for all docs
    if(nDocs > last_ndocs) {
        //ocall_print_string("Realloc aux bool buffer\n");
        aux_bool = (unsigned char *)realloc(aux_bool, sizeof(unsigned char) * nDocs);
        last_ndocs = nDocs;
    }

    //calculate boolean formula
    untrusted_time startb = outside_util::curr_time();
    vec_int response_docs = evaluate(query, nDocs, aux_bool);
    search_results.back().boolean_resolution += trusted_util::time_elapsed_ms(startb, outside_util::curr_time());

    const size_t original_res_size = vi_size(&response_docs);
    search_results.back().docs_retrieved = original_res_size;
    void* results_buffer = NULL;
    if(original_res_size) {
        results_buffer = malloc(original_res_size * PAIR_LEN);

#if BISEN_SCORING
        untrusted_time starts = outside_util::curr_time();
        // calculate idf for each term
        calc_idf(&query, nDocs);

        // calculate tf-idf for each document
        calc_tfidf(&query, &response_docs, results_buffer);
        qsort(results_buffer, original_res_size, PAIR_LEN, compare_results_rbisen);
        search_results.back().scoring += trusted_util::time_elapsed_ms(starts, outside_util::curr_time());
#else
        // iterate over the response docs
        memcpy(results_buffer, response_docs.array, original_res_size * sizeof(int));
#endif
    }

#if BISEN_SCORING
    const size_t threshold = 10;
#else
    const size_t threshold = original_res_size;
#endif
    untrusted_time start2 = outside_util::curr_time();
    // return query results
    *out_len = sizeof(size_t) + sizeof(uint8_t) + threshold * PAIR_LEN;
    *out = (unsigned char*)malloc(*out_len);
    memset(*out, 0, *out_len);

    // add nr of elements at beginning, useful if they are less than threshold
    const size_t elements = std::min(threshold, original_res_size);
    memcpy(*out, &elements, sizeof(size_t));

    const uint8_t score = BISEN_SCORING;
    memcpy(*out + sizeof(size_t), &score, sizeof(uint8_t));
    memcpy(*out + sizeof(size_t) + sizeof(uint8_t), results_buffer, elements * PAIR_LEN);

    // free the buffers in iee_tokens
    for(unsigned i = 0; i < vt_size(&query); i++) {
        iee_token t = query.array[i];

        //vi_destroy(&t.doc_frequencies);
        //vi_destroy(&t.docs);
        free(t.kW);
    }

    vt_destroy(&query);
    vi_destroy(&response_docs);

    if(results_buffer)
        free(results_buffer);
    search_results.back().buffer_alloc += trusted_util::time_elapsed_ms(start2, outside_util::curr_time());
    search_results.back().total_iee += trusted_util::time_elapsed_ms(start, outside_util::curr_time());

    //outside_util::printf("Finished Search!\n-----------------\n");
}
