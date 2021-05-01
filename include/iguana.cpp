#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include "codetracker.h"
#include "iguana.h"

using namespace Iguana;

Node::Node(int lin, int col, const std::string& name)
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

void Node::display(IndentTracker* trckr) {
    std::string indent1 = trckr->getIndentStr();

    std::cout << indent1 << "{" << std::endl;

    trckr->increment();

    std::string indent2 = trckr->getIndentStr();

    std::cout << indent2 << "Name: " << this->mName << std::endl;
    std::cout << indent2 << "Pos: " << "(" << mLin << "," << mCol << ")" << std::endl;

    if (mValue != "") 
        std::cout << indent2  << "Value: " << mValue << std::endl;

    if (mNodes.size() != 0) {
        std::cout << indent2 << "Nodes :-" << std::endl;
        for (Node &n : mNodes) {
            n.display(trckr);
        }
    }

    trckr->decrement();
    std::string indent3 = trckr->getIndentStr();
    std::cout << indent3 << "}" << std::endl;
}

IndentTracker::IndentTracker(int stepSize) {
    mStepSize = stepSize;
    mCurrentMag = 0;
}

void IndentTracker::increment() {
    mCurrentMag += mStepSize;
}

void IndentTracker::decrement() {
    if (mCurrentMag == 0)
        return;

    mCurrentMag -= mStepSize;
}

std::string IndentTracker::getIndentStr() {
    return std::string(mCurrentMag, ' ');
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

    IndentTracker trckr(3); 

    this->mNode->display(&trckr);
}

Parser::Parser()
    : mToParse(""), mName(""),
    mParseFn(nullptr), mType(PTypes::Unassigned),
    mLowerAmt(0), mUpperAmt(0)
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
    trckr->skipWhitespace();
    
    int lin = trckr->mLin;
    int col = trckr->mCol;

    if (trckr->matchString(mToParse)) {
        std::string nodeName = mName;
        Node* pNode = new Node(trckr->mLin, trckr->mCol, nodeName);
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

    std::string nodeName = mName;

    Node* pNode = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* resNode = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* resNode = new Node(lin, col, nodeName);
    resNode->setNodes(nodes);

    return res->success(resNode);
}

ParseResult* Parser::parseClosure(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    ParseResult* pres = parseMany(trckr);

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);

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

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);
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

    std::string nodeName = mName;
    Node* node = new Node(lin, col, nodeName);

    return res->success(node);
}

ParseResult* Parser::parseUntil(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();
    trckr->skipWhitespace();

    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::vector<Node> nodes;

    Parser* toP = mParsers[0];
    Parser* until = mParsers[0];

    int indivLin = lin;
    int indivCol = col;

    while (true) {
        CodeTracker* trckrClone = trckr->copy();
        ParseResult* ures = (until->*until->mParseFn)(trckrClone);

        bool isError = ures->mError;

        delete trckrClone;
        delete ures->mNode;
        delete ures;

        if (!isError) 
            break;

        trckr->skipWhitespace();
        indivLin = trckr->mLin;
        indivCol = trckr->mCol;
        
        ParseResult* pres = (toP->*toP->mParseFn)(trckr);

        if (pres->mError) {
            delete pres;
            return res->failure(getError(toP->mName, lin, col));
        }

        nodes.push_back(*pres->mNode);
        delete pres;
    }

    Node* resNode = new Node(lin, col, mName);

    return res->success(resNode);
}

ParseResult* Parser::parseRegex(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();
    trckr->skipWhitespace();

    int lin = trckr->mLin;
    int col = trckr->mCol;

    std::string resstring = trckr->parseRegex(mToParse);

    if (resstring == "") {
        return res->failure(getError(mName, lin, col));
    }

    Node* resNode = new Node(lin, col, mName);
    resNode->setValue(resstring);

    return res->success(resNode);
}

ParseResult* Parser::parseNumber(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    std::vector<Node> nodes;
    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    Parser* toP = mParsers[0];

    for (int i = 0; i < mLowerAmt; i++) {
        ParseResult* pres = (toP->*toP->mParseFn)(trckr);

        if (pres->mError) {
            delete pres;
            return res->failure(getError(std::to_string(mLowerAmt) + " of " + mName, lin, col));
        }

        nodes.push_back(*pres->mNode);
        delete pres;
    }

    Node* resNode = new Node(lin, col, mName);
    resNode->setNodes(nodes);

    return res->success(resNode);
}

ParseResult* Parser::parseRange(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    std::vector<Node> nodes;
    trckr->skipWhitespace();
    int lin = trckr->mLin;
    int col = trckr->mCol;

    Parser* toP = mParsers[0];

    for (int i = 0; i < mUpperAmt; i++) {
        CodeTracker* trckrClone = trckr->copy();
        ParseResult* pres = (toP->*toP->mParseFn)(trckrClone);

        if (pres->mError) {
            delete trckrClone;
            delete pres;
            break;
        }

        nodes.push_back(*pres->mNode);
        trckr->copyInfo(trckrClone);
        delete trckrClone;
        delete pres;
    }

    if (nodes.size() >= mLowerAmt) {
        Node* resNode = new Node(lin, col, mName);
        resNode->setNodes(nodes);
        return res->success(resNode);
    }

    std::string expected = std::to_string(mLowerAmt)
        + "-"
        + std::to_string(mUpperAmt)
        + " of "
        + mName;

    return res->failure(getError(expected, lin, col));
}

ParseResult* Parser::parseMoreThan(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    std::vector<Node> nodes;
    int lin = trckr->mLin;
    int col = trckr->mCol;

    Parser* toP = mParsers[0];

    while (true) {
        CodeTracker* trckrClone = trckr->copy();
        ParseResult* pres = (toP->*toP->mParseFn)(trckrClone);

        if (pres->mError) {
            delete trckrClone;
            delete pres;
            break;
        }

        nodes.push_back(*pres->mNode);
        trckr->copyInfo(trckrClone);
        delete trckrClone;
        delete pres;
    }

    if (nodes.size() > mLowerAmt) {
        Node* resNode = new Node(lin, col, mName);
        resNode->setNodes(nodes);
        return res->success(resNode);
    }

    std::string expected = "More than "
        + std::to_string(mLowerAmt)
        + " of "
        + mName;

    return res->failure(getError(expected, lin, col));
}

ParseResult* Parser::parseLessThan(CodeTracker* trckr) {
    ParseResult* res = new ParseResult();

    trckr->skipWhitespace();
    std::vector<Node> nodes;
    int lin = trckr->mLin;
    int col = trckr->mCol;

    Parser* toP = mParsers[0];

    for (int i = 1; i < mUpperAmt; i++) {
        CodeTracker* trckrClone = trckr->copy();
        ParseResult* pres = (toP->*toP->mParseFn)(trckr);

        if (pres->mError) {
            delete trckrClone;
            delete pres;
            break;
        }

        nodes.push_back(*pres->mNode);
        trckr->copyInfo(trckrClone);
        delete trckrClone;
        delete pres;
    }

    if (nodes.size() > 0) {
        Node* resNode = new Node(lin, col, mName);
        resNode->setNodes(nodes);
        return res->success(resNode);
    }

    std::string expected = "less than "
        + std::to_string(mUpperAmt)
        + " of "
        + mName;

    return res->failure(getError(expected, lin, col));
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

Parser* Parser::Until(const std::string& name, Parser* toParse, Parser* until) {
    Parser* p = new Parser();
    p->mName = name;
    p->mParsers.push_back(toParse);
    p->mParsers.push_back(until);
    p->mType = PTypes::Until;
    p->mParseFn = &Parser::parseUntil;
    return p;
}

Parser* Parser::EndOfFile(const std::string& name) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::EndOfFile;
    p->mParseFn = &Parser::parseEOF;
    return p;
}

Parser* Parser::Regex(const std::string& name, const std::string& regex) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Regex;
    p->mToParse = regex;
    p->mParseFn = &Parser::parseRegex;
    return p;
}

Parser* Parser::Number(const std::string& name, Parser* toParse, unsigned int amt) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Number;
    p->mLowerAmt = amt;
    p->mParsers.push_back(toParse);
    p->mParseFn = &Parser::parseNumber;
    return p;
}

Parser* Parser::Range(const std::string& name, Parser* toParse, unsigned int l, unsigned int h) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::Range;
    p->mLowerAmt = l;
    p->mUpperAmt = h;
    p->mParsers.push_back(toParse);
    p->mParseFn = &Parser::parseRange;
    return p;
}

Parser* Parser::MoreThan(const std::string& name, Parser* toParse, unsigned int l) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::MoreThan;
    p->mParsers.push_back(toParse);
    p->mParseFn = &Parser::parseMoreThan;
    p->mLowerAmt = l;
    return p;
}

Parser* Parser::LessThan(const std::string& name, Parser* toParse, unsigned int h) {
    Parser* p = new Parser();
    p->mName = name;
    p->mType = PTypes::LessThan;
    p->mParsers.push_back(toParse);
    p->mParseFn = &Parser::parseLessThan;
    p->mUpperAmt = h;
    return p;
}

Parser* Parser::Empty() {
    Parser* p = new Parser();
    return p;
}

void Parser::assign(Parser* other) {
    if (mType != PTypes::Unassigned)
        return;

    mParsers = other->mParsers;
    mToParse = other->mToParse;
    mName = other->mName;
    mType = other->mType;
    mParseFn = other->mParseFn;
}

void Parser::assignParserFunction() {
    switch(mType) {
        case PTypes::And:
            mParseFn = &Parser::parseAnd;
            break;

        default:
            mParseFn = &Parser::parseUntil;
            break;
    }
}


GlobalParserTable::~GlobalParserTable() {
    for (std::pair<std::string, Parser*> const &p : mParsers)
        delete p.second;
}

Parser* GlobalParserTable::String(const std::string& name, const std::string& toParse) {
    Parser* p = Parser::String(toParse, name);
    if (name == "")
        mAnonParsers.push_back(p);
    else
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

Parser* GlobalParserTable::Regex(const std::string& name, const std::string& regex) {
    Parser* p = Parser::Regex(name, regex);    
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Number(const std::string& name, Parser* toP, unsigned int num) {
    Parser* p = Parser::Number(name, toP, num);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Range(const std::string& name, Parser* toP, unsigned int l, unsigned int h) {
    Parser* p = Parser::Range(name, toP, l, h);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::MoreThan(const std::string& name, Parser* toP, unsigned int l) {
    Parser* p = Parser::MoreThan(name, toP, l);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::LessThan(const std::string& name, Parser* toP, unsigned int h) {
    Parser* p = Parser::LessThan(name, toP, h);
    mParsers.insert(std::pair<std::string, Parser*>(name, p));
    return p;
}

Parser* GlobalParserTable::Empty(const std::string& name) {
    Parser* p = Parser::Empty();
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

ParseResult* GlobalParserTable::parseRoot(CodeTracker* trckr) {
    Parser* root = mParsers["ROOT"];
    return root->parse(trckr);
}

void GlobalParserTable::assign(Parser* to, Parser* from) {
    to->assign(from);
}

GlobalParserTable* GlobalParserTable::getFileParser() {
    GlobalParserTable* gpt = new GlobalParserTable();

    Parser* dividerP = gpt->String("Divider", "@@"); 

    Parser* termIdentP = gpt->Regex("Terminal Identifier", "[a-z0-9_]+");

    Parser* colonP = gpt->String("Colon", ":");

    Parser* termRegexP = gpt->Regex("Terminal Regex", "#|\\S+|");

    Parser* termP = gpt->Regex("Terminal", "\\S+");

    Parser* ntermIdentP = gpt->Regex("Non-Terminal Identifier", "[A-Z0-9_]+");

    Parser* arrowP = gpt->String("Arrow", "=>");

    Parser* pipeP = gpt->String("Pipe", "|");

    Parser* semicolonP = gpt->String("Semicolon", ";");

    Parser* lparenP = gpt->String("Left Parenthesis", "(");

    Parser* rparenP = gpt->String("Right Parenthesis", ")");

    Parser* termDeclP = gpt->Or(
                "Terminal Declaration",
                std::vector<Parser*>{ termRegexP, termP }
            );

    Parser* termStmtP = gpt->And(
                "Terminal Statement",
                std::vector<Parser*>{ termIdentP, colonP, termDeclP }
            );

    Parser* termBlockP = gpt->Many("Terminal Block", termStmtP);

    Parser* termOrNtermP = gpt->Or(
                "Terminal or Non Terminal",
                std::vector<Parser*>{ termIdentP, ntermIdentP }
            );

    Parser* andChainP = gpt->Many(
                "And Parsers",
                termOrNtermP
            );

    Parser* andDeclP = gpt->And(
                "AND Parser Declaration",
                std::vector<Parser*>{ ntermIdentP, lparenP, andChainP, rparenP }
            );

    Parser* ntermIdentOrAndDeclP = gpt->Or(
                "Terminal or Non-Terminal or AND Declaration",
                std::vector<Parser*>{ andDeclP, termOrNtermP }
            );

    return gpt;
}

GlobalParserTable* GlobalParserTable::parseFromFile(const std::string& file) {
    GlobalParserTable* gpt = new GlobalParserTable();



    return gpt;
}
