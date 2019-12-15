//
//  SSE_simple.cpp
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 20/03/17.
//  Copyright © 2017 Bernardo Ferreira. All rights reserved.
//

#include "SseClient.hpp"

#include "definitions.h"

using namespace std;

SseClient::SseClient() {
    // init data structures
    //openQueryResponseSocket();
    client_init_crypt();
    analyzer = new EnglishAnalyzer;
    W = new map<string,int>;    /**TODO persist W*/
    nDocs = 0;
}

SseClient::~SseClient() {
    client_destroy_crypt();
    delete analyzer;
    delete W;
}

unsigned long long SseClient::generate_setup_msg(unsigned char** data) {
    // get keys
    unsigned char* kEnc = client_get_kEnc();
    unsigned char* kF = client_get_kF();

    // pack the keys into a buffer
    unsigned long long data_size = 2 * sizeof(unsigned char) + 2 * sizeof(size_t) + client_symBlocksize + client_fBlocksize;

    *data = (unsigned char*)malloc(data_size);
    void* tmp = *data;

    // fill buffer with opcode
    const unsigned char op1 = OP_RBISEN;
    const unsigned char op2 = 0x69;
    memcpy(tmp, &op1, sizeof(unsigned char));
    tmp = (char*)tmp + sizeof(unsigned char);

    memcpy(tmp, &op2, sizeof(unsigned char));
    tmp = (char*)tmp + sizeof(unsigned char);

    // add kEnc to buffer
    memcpy(tmp, &client_symBlocksize, sizeof(size_t));
    tmp = (char*)tmp + sizeof(size_t);

    memcpy(tmp, kEnc, client_symBlocksize);
    tmp = (char*)tmp + client_symBlocksize;

    // add kF to buffer
    memcpy(tmp, &client_fBlocksize, sizeof(size_t));
    tmp = (char*)tmp + sizeof(size_t);

    memcpy(tmp, kF, client_fBlocksize);
    //tmp = (char*)tmp + client_fBlocksize;

    return data_size;
}

int SseClient::new_doc() {
    return nDocs++;
}

map<string, int> SseClient::extract_keywords_frequency(string fname) {
    return analyzer->extractUniqueKeywords(fname);
}

vector<map<string, int>> SseClient::extract_keywords_frequency_wiki(string fname) {
    return analyzer->extractUniqueKeywords_wiki(fname);
}

unsigned long long SseClient::add_new_document(map<string, int> words, unsigned char** data) {
    int id = new_doc();

    // add all words to the newly generated document
    unsigned long long data_size = generate_add_msg(id, words, data);

    //printf("Finished add document #%d (%zu words, %llu ms)\n", id, text.size(), elapsed);

    return data_size;
}

unsigned long long SseClient::generate_add_msg(int doc_id, map<string, int> words, unsigned char **data) {
    // op + all_words * (id + counter + frequency + hmac)
    unsigned long long data_size = 1 * sizeof(unsigned char) + words.size() * (3 * sizeof(int) + H_BYTES);

    // allocate data buffer
    // must be freed in calling function
    *data = (unsigned char*)malloc(sizeof(unsigned char) * data_size); /* fix mismatched free @ valgrind */
    void* tmp = *data;

    // fill buffer with opcode
    //const unsigned char op1 = OP_RBISEN;
    const unsigned char op2 = 0x61;
    //memcpy(tmp, &op1, sizeof(unsigned char));
    //tmp = (char*)tmp + sizeof(unsigned char);

    memcpy(tmp, &op2, sizeof(unsigned char));
    tmp = (char*)tmp + sizeof(unsigned char);

    // iteration to fill the buffer
    for(pair<string, int> f : words) {
        //get counter c for w
        int c = 1; // with new words, this is the first instance of it
        map<string,int>::iterator it = W->find(f.first);
        if (it != W->end())
            c = it->second + 1;

        // update counter c
        (*W)[f.first] = c;

        memcpy(tmp, &doc_id, sizeof(int));
        tmp = (char*)tmp + sizeof(int);

        int c_val = c - 1; // counter starts at 1, so -1 for indexing
        memcpy(tmp, &c_val, sizeof(int));
        tmp = (char*)tmp + sizeof(int);

        memcpy(tmp, &words[f.first], sizeof(int));
        tmp = (char*)tmp + sizeof(int);

        //calculate key kW (with hmac sha256)
        client_c_hmac((unsigned char*)tmp, (unsigned char*)f.first.c_str(), strlen(f.first.c_str()), client_get_kF());
        tmp = (char*)tmp + H_BYTES;
    }

    return data_size;
}

//boolean operands: AND, OR, NOT, (, )
size_t SseClient::search(string query, unsigned char** data) {
    // parse the query into token structs and apply the shunting yard algorithm
    vector<client_token> infix_query = parser->tokenize(query);
    vector<client_token> rpn = parser->shunting_yard(infix_query);

    size_t data_size = 2 * sizeof(unsigned char); // char from op

    // first query iteration: to get needed size and counters
    for(unsigned i = 0; i < rpn.size(); i++) {
        client_token *tkn = &rpn[i];

        if(tkn->type == WORD_TOKEN) {
            map<string,int>::iterator counterIt = W->find(tkn->word);
            if(counterIt != W->end())
                tkn->counter = counterIt->second;
            else
                tkn->counter = 0;

            //printf("counter %s %d\n", tkn->word.c_str(), tkn->counter);
            data_size += sizeof(unsigned char) + sizeof(int) + (H_BYTES * sizeof(unsigned char));
        } else {
            data_size += sizeof(unsigned char);
        }
    }

    // add number of documents to the data structure, needed for NOT
    client_token t;
    t.type = META_TOKEN;
    t.counter = nDocs;

    rpn.push_back(t);
    data_size += sizeof(unsigned char) + sizeof(int);

    //prepare query
    *data = (unsigned char*)malloc(sizeof(unsigned char) * data_size);
    int pos = 0;

    unsigned char op1 = OP_RBISEN;
    unsigned char op2 = 0x73;
    addToArr(&op1, sizeof(unsigned char), *data, &pos);
    addToArr(&op2, sizeof(unsigned char), *data, &pos);

    // second query iteration: to fill "data" buffer
    for(vector<client_token>::iterator it = rpn.begin(); it != rpn.end(); ++it) {
        client_token tkn = *it;

        addToArr(&(tkn.type), sizeof(unsigned char), *data, &pos);

        if(tkn.type == WORD_TOKEN) {
            addIntToArr(tkn.counter, *data, &pos);

            //calculate key kW (with hmac sha256)
            //struct timeval start, end;
            //gettimeofday(&start, NULL);
            client_c_hmac((*data)+pos, (unsigned char*)tkn.word.c_str(), strlen(tkn.word.c_str()), client_get_kF());
            pos += H_BYTES * sizeof(unsigned char);
            //gettimeofday(&end, NULL);
            //printf("hmac = %6.6lf s!\n", timeElapsed(start, end)/1000000.0 );

        } else if(tkn.type == META_TOKEN) {
            addIntToArr(tkn.counter, *data, &pos);
        }
    }

    return data_size;
}

/*
string SseClient::get_random_segment(vector<string> segments) {
    return segments[client_c_random_uint_range(0, segments.size())];
}

string SseClient::generate_random_query(vector<string> all_words, const int size, const int not_prob, const int and_prob) {
    const int lone_word_prob = 50;
    const int par_prob = 50;

    // generate small segments
    queue<string> segments;
    for(int i = 0; i < size; i++) {
        int lone_word_rand = client_c_random_uint_range(0, 100);

        if(lone_word_rand < lone_word_prob) {
            string word = get_random_segment(all_words);

            int not_rand = client_c_random_uint_range(0, 100);
            if(not_rand < not_prob)
                segments.push("!" + word);
            else
                segments.push(word);
        } else {
            // AND or OR
            string word1 = get_random_segment(all_words);
            string word2 = get_random_segment(all_words);

            // choose operator
            int op_prob = client_c_random_uint_range(0, 100);
            string op = op_prob < and_prob ? " && " : " || ";

            // form either a simple, parenthesis or negated segment
            int par_prob_rand = client_c_random_uint_range(0, 100);
            if(par_prob_rand < par_prob) {
                int not_rand = client_c_random_uint_range(0, 100);
                if(not_rand < not_prob)
                    segments.push("!(" + word1 + op + word2 + ")");
                else
                    segments.push("(" + word1 + op + word2 + ")");
            } else {
                segments.push(word1 + op + word2);
            }
        }
    }

    while(segments.size() > 1) {
        // pop two segments from the queue
        string seg1 = segments.front();
        segments.pop();
        string seg2 = segments.front();
        segments.pop();

        // choose operator
        int op_prob = client_c_random_uint_range(0, 100);
        string op = op_prob < and_prob  ? " && " : " || ";

        // form either a simple, parenthesis or negated segment
        int par_prob_rand = client_c_random_uint_range(0, 100);
        if(par_prob_rand < par_prob) {
            int not_rand = client_c_random_uint_range(0, 100);
            if(not_rand < not_prob)
                segments.push("!(" + seg1 + op + seg2 + ")");
            else
                segments.push("(" + seg1 + op + seg2 + ")");
        } else {
            segments.push(seg1 + op + seg2);
        }
    }

    return segments.front();
}*/

// http://thispointer.com/how-to-sort-a-map-by-value-in-c/
typedef function<bool(pair<string, int>, pair<string, int>)> Comparator;
Comparator compFunctor = [](pair<string, int> elem1 ,pair<string, int> elem2) {
    return elem1.second < elem2.second;
};

void SseClient::list_words() {
    set<pair<string, int>, Comparator> word_set(W->begin(), W->end(), compFunctor);

    map<string,int>::iterator it;
    for (pair<string, int> el : word_set)
        printf("%s %d\n", el.first.c_str(), el.second);

}

double SseClient::get_read_file_time() {
    return analyzer->get_read_file_time();
}
