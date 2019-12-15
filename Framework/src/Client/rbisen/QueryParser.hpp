#ifndef QueryParser_hpp
#define QueryParser_hpp

#include "EnglishAnalyzer.h"
#include "Utils.h"
#include "Definitions.h"

class QueryParser {
private:
    EnglishAnalyzer analyzer;

public:
    std::vector<client_token> tokenize(std::string query);
    std::vector<client_token> shunting_yard(std::vector<client_token>);
};

#endif
