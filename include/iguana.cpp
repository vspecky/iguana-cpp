#include "iguana.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "codetracker.h"

using namespace Iguana;

Node::Node(int lin, int col, std::string name) {
    mLin = lin;
    mCol = col;
    mName = name;
    mValue = "";
}

void Node::setNodes(std::vector<Node> nodes) {
    mNodes = nodes;
}

void Node::setValue(std::string value) {
    mValue = value;
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

Parser::Parser() {
    mToParse = "";
    mName = "";
    mParseFn = nullptr;
    mType = PTypes::Unassigned;
}

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
        ParseResult* pres = (p->*mParseFn)(trckr);
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
        ParseResult* pres = (p->*mParseFn)(cloneTrckr);

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
        ParseResult* pres = (p->*mParseFn)(trckrClone);

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

    return res;
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
