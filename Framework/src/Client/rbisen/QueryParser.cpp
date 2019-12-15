#include "QueryParser.hpp"

using namespace std;

int is_operator(char c) {
    return c == '&' || c == '|' || c == '!' || c == '(' || c == ')';
}

int precedence(char op) {
    if(op == '!')
        return 3;
    else if(op == '&')
        return 2;
    else if(op == '|')
        return 1;
    else
        return -1;
}

vector<client_token> QueryParser::tokenize(string query) {
    vector <client_token> result;

    int i = 0;
    while (query[i] != '\0') {
        if (query[i] == ' ') {
            i++;
            continue;
        }

        client_token tkn;
        if(is_operator(query[i])) {
            tkn.type = query[i];

            // eliminate next character if it's the same (&& or ||)
            if((query[i] == '&' || query[i] == '|') && query[i+1] == query[i])
                i++;

            i++;
        } else {
            // it's a word
            tkn.type = WORD_TOKEN;

            while(query[i] != '\0' && query[i] != ' ' && !is_operator(query[i])){
                tkn.word += tolower(query[i]);
                i++;
            }
            /*printf("parser word\n");
            for(int i = 0; i < 5; i++)
                printf("%c", tkn.word[i]);
            printf("\n");*/
            // NULL termination is added in serialization
            //tkn.word += '\0';

            tkn.word = string(analyzer.stemWord(tkn.word));
            /*printf("parser word\n");
            for(int i = 0; i < 5; i++)
                printf("%c", tkn.word[i]);
            printf("\n");*/
        }

        result.push_back(tkn);
    }

    return result;
}

vector<client_token> QueryParser::shunting_yard(vector<client_token> infix_query) {
    stack<char> operators;
    vector<client_token> output;

    for(unsigned i = 0; i < infix_query.size(); i++) {
        client_token tkn = infix_query[i];

        if(tkn.type == WORD_TOKEN) {
            output.push_back(tkn);
        } else if(tkn.type == '&' || tkn.type == '|' || tkn.type == '!') {
            int curr_precedence = operators.empty()? -1 : precedence(operators.top());

            while (curr_precedence >= precedence(tkn.type)) {
                client_token tkn2;
                tkn2.type = operators.top();
                operators.pop();
                output.push_back(tkn2);

                curr_precedence = operators.empty()? -1 : precedence(operators.top());
            }

            operators.push(tkn.type);
        } else if(tkn.type == '(') {
            operators.push(tkn.type);
        } else if(tkn.type == ')') {
            while (operators.top() != '(') {
                client_token tkn2;
                tkn2.type = operators.top();
                operators.pop();

                output.push_back(tkn2);

                if (operators.empty())
                    throw invalid_argument("Mismatched parenthesis while parsing!");
            }

            operators.pop(); // pops the '('
        }
    }

    while (!operators.empty()) {
        char top = operators.top();
        operators.pop();

        if (top == '(' || top == ')')
            throw invalid_argument("Mismatched parenthesis!");

        client_token tkn;
        tkn.type = top;
        output.push_back(tkn);
    }

    return output;
}
