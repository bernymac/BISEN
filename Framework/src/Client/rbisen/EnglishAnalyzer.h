//
//  EnglishAnalyzer.h
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 13/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __BooleanSSE__EnglishAnalyzer__
#define __BooleanSSE__EnglishAnalyzer__

#include <vector>
#include <set>
#include <string>
#include <stdlib.h>
#include <ctype.h>
#include "ClientUtils.h"
#include "PorterStemmer.c"

using namespace std;

class EnglishAnalyzer {
    #define INC 50
    char * s;
    int i_max;
    set<string> stopWords;

    void increase_s();

public:
    EnglishAnalyzer();
    ~EnglishAnalyzer();
    map<string, int> extractUniqueKeywords(string fname);
    vector<map<string, int>> extractUniqueKeywords_wiki(string fname);
    char* stemWord(string word);
    void stemWord_wiki(char* word);
    bool isStopWord(string word);

    double get_read_file_time();

private:
    double read_file_time = 0;
};
#endif /* defined(__BooleanSSE__EnglishAnalyzer__) */
