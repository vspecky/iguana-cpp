#pragma once

#include <string>
#include <vector>
#include "codetracker.h"
#include <map>

namespace Iguana {
    class IndentTracker {
    private:
        int mCurrentMag;
        int mStepSize;

    public:
        IndentTracker(int);
        void increment();
        void decrement();
        std::string getIndentStr();
    };

    class Node {
    public:
        std::vector<Node> mNodes;
        std::string mValue;
        std::string mName;
        int mLin;
        int mCol;

        Node(int, int, const std::string&);
        void setNodes(std::vector<Node>);
        void setValue(std::string);
        void display(IndentTracker*);
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
        Until,
        EndOfFile,
        Regex,
        Number,
        Range,
        MoreThan,
        LessThan,
    };

    class Parser {
    private:
        std::vector<Parser*> mParsers;
        std::string mToParse;
        std::string mName;
        std::vector<bool> toInclude;
        ParseResult* (Parser::*mParseFn)(CodeTracker*);
        PTypes mType;
        unsigned int mLowerAmt;
        unsigned int mUpperAmt;

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
        ParseResult* parseUntil(CodeTracker*);
        ParseResult* parseRegex(CodeTracker*);
        ParseResult* parseNumber(CodeTracker*);
        ParseResult* parseRange(CodeTracker*);
        ParseResult* parseMoreThan(CodeTracker*);
        ParseResult* parseLessThan(CodeTracker*);

        void assignParserFunction();

        Parser();

    public:
        void assign(Parser*);
        static Parser* Many(Parser*, const std::string&);
        static Parser* Closure(Parser*, const std::string&);
        static Parser* Alphabetic(const std::string&);
        static Parser* Alphanumeric(const std::string&);
        static Parser* Digit(const std::string&);
        static Parser* Custom(const std::string&, const std::string&);
        static Parser* EndOfFile(const std::string&);
        static Parser* Until(const std::string&, Parser*, Parser*);
        static Parser* Number(const std::string&, Parser*, unsigned int);
        static Parser* Range(const std::string&, Parser*, unsigned int, unsigned int);
        static Parser* MoreThan(const std::string&, Parser*, unsigned int);
        static Parser* LessThan(const std::string&, Parser*, unsigned int);
        static Parser* Empty();
        static Parser* String(const std::string&, const std::string&);
        static Parser* And(std::vector<Parser*>, const std::string&);
        static Parser* And(std::vector<Parser*>, const std::string&, std::vector<bool>);
        static Parser* Or(std::vector<Parser*>, const std::string&);
        static Parser* Regex(const std::string&, const std::string&);
        friend class GlobalParserTable;
    };

    class GlobalParserTable {
    private:
        std::map<std::string, Parser*> mParsers;
        std::vector<Parser*> mAnonParsers;

        static GlobalParserTable* getFileParser();

    public:
        ~GlobalParserTable();

        static GlobalParserTable* parseFromFile(const std::string&);

        Parser* String(const std::string&, const std::string&);
        Parser* And(const std::string&, std::vector<Parser*>);
        Parser* Or(const std::string&, std::vector<Parser*>);
        Parser* Many(const std::string&, Parser*);
        Parser* Closure(const std::string&, Parser*);
        Parser* Alphabetic(const std::string&);
        Parser* Alphanumeric(const std::string&);
        Parser* Digit(const std::string&);
        Parser* Custom(const std::string&, const std::string&);
        Parser* Until(const std::string&, Parser*, Parser*);
        Parser* EndOfFile(const std::string&);
        Parser* Regex(const std::string&, const std::string&);
        Parser* Number(const std::string&, Parser*, unsigned int);
        Parser* Range(const std::string&, Parser*, unsigned int, unsigned int);
        Parser* MoreThan(const std::string&, Parser*, unsigned int);
        Parser* LessThan(const std::string&, Parser*, unsigned int);
        Parser* Empty(const std::string&);

        void addAnonParser(Parser*);

        void assign(Parser*, Parser*);

        ParseResult* parse(Parser*, CodeTracker*);
        ParseResult* parseRoot(CodeTracker*);
    };

    class ParserConstructor {

    };
}
