#pragma once

#include <string>
#include <vector>
#include <cstdio>
#include "iguana.h"


class IguanaConstructor {
private:

    struct Token {
        std::string type;
        std::string value;
        
        Token(const std::string&, const std::string&);
    };

    class Lexer {
    private:
        std::string mInput;
        int mPtr = 0;

        char current();
        char consume();
        char consumeNext();
        void skipWhitespace();
        bool eof();

        std::string parseTill(bool (*)(char));
        Token* getToken();

    public:
        Lexer(std::string);
        std::vector<Token*> lexInput();
    };

    struct ParseNode {
        std::string mName;
        std::string mType;
        std::vector<std::string> mValues;

        ParseNode(const std::string, const std::string);

        void display() {
            std::printf("%s %s: ", mName.c_str(), mType.c_str());

            for (std::string& s : mValues) {
                std::printf("%s ", s.c_str());
            }

            std::printf("\n");
        }
    };

    struct ParseResult {
        std::vector<ParseNode> mNodes;
        bool mIsError;
        std::string mErrorMsg;

        ParseResult(std::vector<ParseNode>);
        ParseResult(const std::string);
    };

    class Parser {
    private:
        int mPtr = 0;
        bool mError = false;
        std::string mErrorMsg;

        Token* current();
        Token* consume();
        void skip(const std::string);
        void parseAllAtoms(std::vector<ParseNode>&);
        void parseAllGrammars(std::vector<ParseNode>&);
    
    public:
        std::vector<Token*> mLexemes;
        ParseResult parse();
    };

    struct ConstructResult {
        Iguana::GlobalParserTable* mGpt;
        std::string mErrorMsg;
        bool mIsError;
    };

public:
    void testLexer(std::string);
    void testParser(std::string);

    static ConstructResult construct(std::string);
};
