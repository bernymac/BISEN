//
//  SSE_simple.hpp
//  BooleanSSE
//
//  A simple SSE scheme, with all required functionality. Includes both client and server functionality
//  in one class, for simplification. Inline comments differentiate respective parts of the code.
//
//  Created by Bernardo Ferreira on 20/03/17.
//  Copyright © 2017 Bernardo Ferreira. All rights reserved.
//

#ifndef SseClient_hpp
#define SseClient_hpp

#include "ClientUtils.h"
#include "EnglishAnalyzer.h"
#include "QueryParser.hpp"

extern "C" {
    #include "ClientCrypt.h"
}

using namespace std;

class SseClient {

protected:
    EnglishAnalyzer* analyzer;
    QueryParser* parser;
    map<string,int>* W;
    int querySocket;
    int nDocs;

public:
    SseClient();
    ~SseClient();

    unsigned long long generate_setup_msg(unsigned char** data);
    unsigned long long add_new_document(map<string, int> words, unsigned char** data);
    //unsigned long long add_words(int doc_id, set<string> words, unsigned char** data);
    size_t search(string query, unsigned char** data);

    // functions for testing purposes
    //string generate_random_query(vector<string> all_words, const int size, const int not_prob, const int and_prob);
    vector<map<string, int>> extract_keywords_frequency_wiki(string fname);
    map<string, int> extract_keywords_frequency(string fname);

    void list_words();

    //const unsigned long count_articles(string dataset_dir, vector<string> vector);

    double get_read_file_time();

private:
    int new_doc();
    unsigned long long generate_add_msg(int doc_id, map<string, int> words, unsigned char **data);
    //string get_random_segment(vector<string> segments);
};

#endif /* SseClient_hpp */
