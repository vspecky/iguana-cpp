#pragma once

#include <string>
#include <vector>
#include "codetracker.h"
#include <map>

namespace Iguana {
    class Node {
    public:
        std::vector<Node> mNodes;
        std::string mValue;
        std::string mName;
        int mLin;
        int mCol;

        Node(int, int, std::string);
        void setNodes(std::vector<Node>);
        void setValue(std::string);
        std::string display();
    };

    class ParseResult {
    public:
        Node* mNode;
        bool mError;
        std::string mMsg;

        ParseResult();
        ParseResult* success(Node*);
        ParseResult* failure(const std::string&);
        void displayResult();
    };

    enum class PTypes : char {
        Unassigned,
        String,
        And,
        Or,
        Many,
        Alphabetic,
        Alphanumeric,
        Digit,
        Custom,
        Closure,
        EndOfFile,
    };

    class Parser {
    private:
        std::vector<Parser*> mParsers;
        std::string mToParse;
        std::string mName;
        ParseResult* (Parser::*mParseFn)(CodeTracker*);
        PTypes mType;

        std::string getError(const std::string&, int, int);
        ParseResult* parseString(CodeTracker*);
        ParseResult* parseAnd(CodeTracker*);
        ParseResult* parseOr(CodeTracker*);
        ParseResult* parseMany(CodeTracker*);
        ParseResult* parseClosure(CodeTracker*);
        ParseResult* parse(CodeTracker*);
        ParseResult* parseAlphabetic(CodeTracker*);
        ParseResult* parseAlphanumeric(CodeTracker*);
        ParseResult* parseDigit(CodeTracker*);
        ParseResult* parseCustom(CodeTracker*);
        ParseResult* parseEOF(CodeTracker*);

        void assign(Parser*);

        Parser();
        static Parser* String(const std::string&, const std::string&);
        static Parser* And(std::vector<Parser*>, const std::string&);
        static Parser* Or(std::vector<Parser*>, const std::string&);
        static Parser* Many(Parser*, const std::string&);
        static Parser* Closure(Parser*, const std::string&);
        static Parser* Alphabetic(const std::string&);
        static Parser* Alphanumeric(const std::string&);
        static Parser* Digit(const std::string&);
        static Parser* Custom(const std::string&, const std::string&);
        static Parser* EndOfFile(const std::string&);

    public:
        friend class GlobalParserTable;
    };

    class GlobalParserTable {
    private:
        std::map<std::string, Parser*> mParsers;

    public:
        ~GlobalParserTable();

        Parser* String(const std::string&, const std::string&);
        Parser* And(const std::string&, std::vector<Parser*>);
        Parser* Or(const std::string&, std::vector<Parser*>);
        Parser* Many(const std::string&, Parser*);
        Parser* Closure(const std::string&, Parser*);
        Parser* Alphabetic(const std::string&);
        Parser* Alphanumeric(const std::string&);
        Parser* Digit(const std::string&);
        Parser* Custom(const std::string&, const std::string&);
        Parser* EndOfFile(const std::string&);

        void assign(Parser*, Parser*);

        ParseResult* parse(Parser*, CodeTracker*);
    };
}
