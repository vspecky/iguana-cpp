#include "iguana.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include "codetracker.h"

using namespace Iguana;

Node::Node(int lin, int col, std::string name)
    : mValue("")
{
    mLin = lin;
    mCol = col;
    mName = name;
}

void Node::setNodes(std::vector<Node> nodes) {
    mNodes = nodes;
}

void Node::setValue(std::string value) {
    mValue = value;
}

std::string Node::display() {
    if (mValue != "") {
        return mName + "(" + mValue + ")";
    }

    std::string res = mName + "(";

    for (Node &n : mNodes) {
        res += n.display() + ",";
    }

    res += ")";

    return res;
}

ParseResult::ParseResult() {
    mNode = nullptr;
    mError = false;
    mMsg = "";
}

ParseResult* ParseResult::success(Node* node) {
    mNode = node;
    return this;
}

ParseResult* ParseResult::failure(const std::string& msg) {
    mError = true;
    mMsg = msg;
    return this;
}

void ParseResult::displayResult() {
    if (mError) {
        std::cout << mMsg << std::endl;
        return;
    }

    std::cout << mNode->display() << std::endl;
}

Parser::Parser()
    : mToParse(""), mName(""),
    mParseFn(nullptr), mType(PTypes::Unassigned)
{}

std::string Parser::getError(const std::string& expected, int lin, int col) {
    std::ostringstream err;
    err << mName << " Parsing Error: Expected " << expected << " (" 
        << lin << ":" << col << ")";
    return err.str();
}

ParseResult* Parser::parse(CodeTracker* trckr) {
    return (this->*mParseFn)(trckr);
}

ParseResult* Parser::parseString(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();
    
    int lin = trckr->mLin;
    int col = trckr->mCol;

    if (trckr->matchString(mToParse)) {
        Node* pNode = new Node(trckr->mLin, trckr->mCol, mName);
        pNode->setValue(mToParse);
        trckr->consume(mToParse);
        return res->success(pNode);
    }

    return res->failure(this->getError("'" + mToParse + "'", lin, col));
}

ParseResult* Parser::parseAnd(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::vector<Node> nodes;
    for (Parser* p : mParsers) {
        int startLin = trckr->mLin;
        int startCol = trckr->mCol;
        ParseResult* pres = (p->*p->mParseFn)(trckr);
        if (pres->mError) {
            return res->failure(this->getError(p->mName, startLin, startCol));
        }
        nodes.push_back(*pres->mNode);
        delete pres;
    }

    Node* pNode = new Node(lin, col, mName);
    pNode->setNodes(nodes);

    return res->success(pNode);
}

ParseResult* Parser::parseOr(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    Node* node = nullptr;

    for (Parser* p : mParsers) {
        CodeTracker* cloneTrckr = trckr->copy();
        ParseResult* pres = (p->*p->mParseFn)(cloneTrckr);

        if (pres->mError) {
            delete pres;
            delete cloneTrckr;
            continue;
        }

        node = pres->mNode;
        trckr->mIdx = cloneTrckr->mIdx;
        trckr->mLin = cloneTrckr->mLin;
        trckr->mCol = cloneTrckr->mCol;
        delete pres;
        delete cloneTrckr;
        break;
    }

    if (node == nullptr) {
        std::string expected = "one of ";

        bool first = true;
        expected += "'" + mParsers[0]->mName + "'";
        for (Parser* p : mParsers) {
            if (first) {
                first = false;
                continue;
            }
            expected += ", '" + p->mName + "'";
        }

        return res->failure(this->getError(expected, lin, col));
    }

    Node* resNode = new Node(lin, col, mName);
    resNode->setNodes(std::vector<Node>{ *node });

    return res->success(resNode);
}

ParseResult* Parser::parseMany(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    std::vector<Node> nodes;
    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;


    Parser* p = mParsers[0];
    while (true) {
        CodeTracker* trckrClone = trckr->copy();
        ParseResult* pres = (p->*p->mParseFn)(trckrClone);

        if (pres->mError)
            break;

        trckr->copyInfo(trckrClone);
        nodes.push_back(*pres->mNode);
        delete pres;
        delete trckrClone;
    }

    if (nodes.size() == 0) {
        std::string expected = "one or more of '" + mName + "'";
        return res->failure(this->getError(expected, lin, col));
    }

    Node* resNode = new Node(lin, col, mName);
    resNode->setNodes(nodes);

    return res->success(resNode);
}

ParseResult* Parser::parseClosure(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    ParseResult* pres = parseMany(trckr);

    Node* node = new Node(lin, col, mName);

    if (!pres->mError)
        node->setNodes(std::vector<Node>{ *pres->mNode });

    delete pres;
    return res->success(node);;
}

ParseResult* Parser::parseAlphabetic(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::string resstr = trckr->parseKey(std::isalpha);

    if (resstr == "") {
        return res->failure(getError("alphabetic character", lin, col));
    }

    Node* node = new Node(lin, col, mName);
    node->setValue(resstr);

    return res->success(node);
}

ParseResult* Parser::parseAlphanumeric(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::string resstr = trckr->parseKey(std::isalnum);

    if (resstr == "") {
        return res->failure(getError("alphanumeric character", lin, col));
    }

    Node* node = new Node(lin, col, mName);
    node->setValue(resstr);

    return res->success(node);
}

ParseResult* Parser::parseDigit(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::string resstr = trckr->parseKey(std::isdigit);

    if (resstr == "") {
        return res->failure(getError("digit", lin, col));
    }

    Node* node = new Node(lin, col, mName);
    node->setValue(resstr);

    return res->success(node);
}

ParseResult* Parser::parseCustom(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::string resstr = trckr->parseCustomSymbols(mToParse);

    if (resstr == "") {
        return res->failure(getError("one of " + mToParse, lin, col));
    }

    Node* node = new Node(lin, col, mName);
    node->setValue(resstr);

    return res->success(node);
}

ParseResult* Parser::parseEOF(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();
    trckr->skipWhitespace();

    int lin = trckr->mLin;
    int col = trckr->mCol;

    if (!trckr->isEOF()) 
        return res->failure(getError("end of file", lin, col));

    Node* node = new Node(lin, col, mName);

    return res->success(node);
}

Parser* Parser::String(const std::string& toParse, const std::string& name) {
    Parser* p = new Parser();
    p->mToParse = toParse;
    p->mName = name;
    p->mType = PTypes::String;
    p->mParseFn = &Parser::parseString;
    return p;
}

Parser* Parser::And(std::vector<Parser*> toParse, const std::string& name) {
    Parser* p = new Parser();
    p->mParsers = toParse;
    p->mName = name;
    p->mType = PTypes::And;
    p->mParseFn = &Parser::parseAnd;
    return p;
}

Parser* Parser::Or(std::vector<Parser*> toParse, const std::string& name) {
    Parser* p = new Parser();
    p->mParsers = toParse;
    p->mName = name;
    p->mType = PTypes::Or;
    p->mParseFn = &Parser::parseOr;
    return p;
}

Parser* Parser::Many(Parser* toParse, const std::string& name) {
    Parser* p = new Parser();
    p->mParsers.push_back(toParse);
    p->mName = name;
    p->mType = PTypes::Many;
    p->mParseFn = &Parser::parseMany;
    return p;
}

Parser* Parser::Closure(Parser* toParse, const std::string& name) {
    Parser* p = new Parser();
    p->mParsers.push_back(toParse);
    p->mName = name;
    p->mType = PTypes::Closure;
    p->mParseFn = &Parser::parseClosure;
    return p;
}

Parser* Parser::Alphabetic(const std::string& name) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Alphabetic;
    p->mParseFn = &Parser::parseAlphabetic;
    return p;
}

Parser* Parser::Alphanumeric(const std::string& name) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Alphanumeric;
    p->mParseFn = &Parser::parseAlphanumeric;
    return p;
}

Parser* Parser::Digit(const std::string& name) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Digit;
    p->mParseFn = &Parser::parseDigit;
    return p;
}

Parser* Parser::Custom(const std::string& name, const std::string& toParse) {
    Parser* p = new Parser();
    p->mName = name;
    p->mToParse = toParse;
    p->mType = PTypes::Custom;
    p->mParseFn = &Parser::parseCustom;
    return p;
}

Parser* Parser::EndOfFile(const std::string& name) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::EndOfFile;
    p->mParseFn = &Parser::parseEOF;
    return p;
}

void Parser::assign(Parser* other) {
    if (mType != PTypes::Unassigned)
        return;

    mParsers = other->mParsers;
    mToParse = other->mToParse;
    mName = other->mName;
    mParseFn = other->mParseFn;
    mType = other->mType;
    delete other;
}



GlobalParserTable::~GlobalParserTable() {
    for (std::pair<std::string, Parser*> const &p : mParsers)
        delete p.second;
}

Parser* GlobalParserTable::String(const std::string& name, const std::string& toParse) {
    Parser* p = Parser::String(toParse, name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::And(const std::string& name, std::vector<Parser*> parsers) {
    Parser* p = Parser::And(parsers, name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Or(const std::string& name, std::vector<Parser*> parsers) {
    Parser* p = Parser::Or(parsers, name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Many(const std::string& name, Parser* toParse) {
    Parser* p = Parser::Many(toParse, name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Closure(const std::string& name, Parser* toParse) {
    Parser* p = Parser::Closure(toParse, name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Alphabetic(const std::string& name) {
    Parser* p = Parser::Alphabetic(name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Alphanumeric(const std::string& name) {
    Parser* p = Parser::Alphanumeric(name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Digit(const std::string& name) {
    Parser* p = Parser::Digit(name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Custom(const std::string& name, const std::string& toParse) {
    Parser* p = Parser::Custom(name, toParse);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::EndOfFile(const std::string& name) {
    Parser* p = Parser::EndOfFile(name);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

ParseResult* GlobalParserTable::parse(Parser* mainP, CodeTracker* trckr) {
    for (std::pair<std::string, Parser*> const &p : mParsers) {
        if (p.second->mType == PTypes::Unassigned) {
            ParseResult* res = new ParseResult();
            return res->failure(p.second->mName + " Parser is unassigned");
        }
    }

    return mainP->parse(trckr);
}

void GlobalParserTable::assign(Parser* to, Parser* from) {
    mParsers.erase(from->mName);
    to->assign(from);
}
