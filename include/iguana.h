#pragma once

#include <string>
#include <vector>
#include "codetracker.h"

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
    };

    class ParseResult {
    public:
        Node* mNode;
        bool mError;
        std::string mMsg;

        ParseResult();
        ParseResult* success(Node*);
        ParseResult* failure(const std::string&);
    };

    enum class PTypes : char {
        Unassigned,
        String,
        And,
        Or,
        Many,
        Closure,
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

    public:
        Parser();
        ParseResult* parse(CodeTracker*);
        static Parser* String(const std::string&, const std::string&);
        static Parser* And(std::vector<Parser*>, const std::string&);
        static Parser* Or(std::vector<Parser*>, const std::string&);
        static Parser* Many(Parser*, const std::string&);
        static Parser* Closure(Parser*, const std::string&);
    };

    class GlobalParserTable {
    private:

    };
}
