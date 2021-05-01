#include <string>
#include <vector>
#include <cstdio>
#include <map>
#include "constructor.h"
#include "iguana.h"

using IC = IguanaConstructor;

IC::Token::Token(const std::string& value, const std::string& type)
    : type(type), value(value)
{}

IC::Lexer::Lexer(std::string inp) 
    : mInput(inp)
{}

IC::ParseNode::ParseNode(const std::string name, const std::string type) 
    : mName(name), mType(type)
{}

IC::ParseResult::ParseResult(std::vector<IC::ParseNode> nodes) 
    : mNodes(nodes), mIsError("")
{
    mIsError = false;
}

IC::ParseResult::ParseResult(const std::string error)
    : mErrorMsg(error)
{
    mIsError = true;
}

char IC::Lexer::current() {
    if (mPtr < mInput.length()) {
        return mInput[mPtr];
    }

    return '\0';
}

char IC::Lexer::consume() {
    char curr = current();
    ++mPtr;
    return curr;
}

char IC::Lexer::consumeNext() {
    ++mPtr;
    return current();
}

void IC::Lexer::skipWhitespace() {
    while (std::isspace(current()))
        consume();
}

bool IC::Lexer::eof() {
    skipWhitespace();
    return mPtr >= mInput.length();
}

std::string IC::Lexer::parseTill(bool (*filter)(char)) {
    std::string res = "";

    char curr = current();
    while (filter(curr)) {
        res += curr;
        curr = consumeNext();
    }

    return res;
}

IC::Token* IC::Lexer::getToken() {
    skipWhitespace();

    std::string val = parseTill([](char ch) -> bool {
            return std::isalpha(ch) || ch == '_';
        });

    char curr = current();
    if (std::isspace(curr) || curr == '\0') {
        return new Token(val, "IDENT");
    }

    val += parseTill([](char ch) -> bool {
            return !std::isspace(ch) && ch != '\0';
        });

    return new Token(val, "STR");
}

std::vector<IC::Token*> IC::Lexer::lexInput() {
    std::vector<Token*> res;

    while (true) {
        if (eof())
            break;

        res.push_back(getToken());
    }

    return res;
}

IC::Token* IC::Parser::current() {
    if (mPtr >= mLexemes.size()) {
        return nullptr;
    }

    return mLexemes[mPtr];
}

IC::Token* IC::Parser::consume() {
    Token* curr = current();
    ++mPtr;
    return curr;
}

void IC::Parser::skip(const std::string val) {
    Token* curr = current();

    if (curr == nullptr) {
        mError = true;
        mErrorMsg = "Unexpected end of input";
        return;
    }

    if (curr->value == val) {
        consume();
        return;
    }

    mError = true;
    mErrorMsg = "Expected " + val + "\n";
}

void IC::Parser::parseAllAtoms(std::vector<ParseNode>& nodes) {
    std::vector<ParseNode> atoms;
    Token* ident = current();

    while (ident != nullptr && ident->type == "IDENT") {
        consume();
        Token* val = consume();

        if (val == nullptr) {
            mError = true;
            mErrorMsg = "Unexpected end of input";
            return;
        }

        ParseNode n(ident->value, "ATOM");
        n.mValues.push_back(val->value);
        atoms.push_back(n);
        ident = current();
    }

    if (atoms.size() == 0) {
        mError = true;
        mErrorMsg = "No atoms specified";
        return;
    }

    nodes.insert(nodes.end(), atoms.begin(), atoms.end());
}

void IC::Parser::parseAllGrammars(std::vector<ParseNode>& nodes) {
    std::vector<ParseNode> grams;

    Token* ident = current();

    while (ident != nullptr && ident->type == "IDENT") {
        consume();
        std::vector<std::string> opers;
        Token* op = current();
        std::string type = "";

        if (op == nullptr && op->value != "&" && op->value != "|") {
            mError = true;
            mErrorMsg = "Expected '&' or '|'";
            return;
        }

        if (op->value == "|") {
            type = "Or";
            while (op != nullptr && op->value == "|") {
                consume();
                Token* oper = consume();

                if (oper->type != "IDENT") {
                    mError = true;
                    mErrorMsg = "Expected identifier";
                    return;
                }

                opers.push_back(oper->value);

                op = current();
            }
        } else {
            type = "And";
            while (op != nullptr && op->value == "&") {
                consume();
                Token* oper = consume();

                if (oper->type != "IDENT") {
                    mError = true;
                    mErrorMsg = "Expected identifier";
                    return;
                }

                opers.push_back(oper->value);

                op = current();
            }
        }

        skip(";");
        if (mError)
            return;

        ParseNode n(ident->value, type);
        n.mValues = opers;
        nodes.push_back(n);
        ident = current();
    }
}

IC::ParseResult IC::Parser::parse() {
    std::vector<ParseNode> nodes;

    skip("@@");
    if (mError)
        return ParseResult(mErrorMsg);

    parseAllAtoms(nodes);
    if (mError)
        return ParseResult(mErrorMsg);

    skip("@@");
    if (mError)
        return ParseResult(mErrorMsg);

    parseAllGrammars(nodes);
    if (mError)
        return ParseResult(mErrorMsg);

    skip("@@");
    if (mError)
        return ParseResult(mErrorMsg);

    return ParseResult(nodes);
}

void IC::testLexer(std::string input) {
    Lexer* lex = new Lexer(input);

    std::vector<Token*> toks = lex->lexInput();

    for (Token* t : toks) {
        std::printf("%-10s%-10s\n", t->type.c_str(), t->value.c_str());
    }

    delete lex;
}

void IC::testParser(std::string input) {
    Lexer* lex = new Lexer(input);

    std::vector<Token*> toks = lex->lexInput();

    for (Token* t : toks) {
        std::printf("%-10s%-10s\n", t->type.c_str(), t->value.c_str());
    }

    Parser* p = new Parser();
    p->mLexemes = toks;

    ParseResult res = p->parse();

    if (res.mIsError) {
        std::printf("%s\n", res.mErrorMsg.c_str());
    } else {
        for (ParseNode& n : res.mNodes) {
            n.display();
        }
    }

    delete p;
    delete lex;
}

IC::ConstructResult IC::construct(std::string input) {
    Lexer lex(input);
    std::vector<Token*> lexemes = lex.lexInput();

    Parser grammarParser;
    grammarParser.mLexemes = lexemes;

    ParseResult pres = grammarParser.parse();

    if (pres.mIsError) {
        ConstructResult cres;
        cres.mIsError = true;
        cres.mErrorMsg = pres.mErrorMsg;
        return cres;
    }

    std::vector<ParseNode>& nodes = pres.mNodes;

    bool rootPresent = false;
    for (ParseNode& n : nodes) {
        if (n.mName == "ROOT") {
            rootPresent = true;
            break;
        }
    }

    if (!rootPresent) {
        ConstructResult cres;
        cres.mIsError = true;
        cres.mErrorMsg = "No ROOT production given";
        return cres;
    }

    std::map<std::string, Iguana::Parser*> parsers;

    Iguana::GlobalParserTable* gpt = new Iguana::GlobalParserTable();

    for (ParseNode& n : nodes) {
        if (parsers.count(n.mName)) {
            ConstructResult cres;
            cres.mIsError = true;
            cres.mErrorMsg = "Duplicate parsers";
            delete gpt;
            return cres;
        }

        Iguana::Parser* p = gpt->Empty(n.mName);
        parsers[n.mName] = p;
    }

    for (ParseNode& n : nodes) {
        Iguana::Parser* p = parsers[n.mName];

        if (n.mType == "ATOM") {
            std::string val = n.mValues[0];
            
            Iguana::Parser* interm;
            if (val.length() > 2 && val.substr(0, 2) == "#|" && val.back() == '|') {
                interm = Iguana::Parser::Regex(n.mName, val);
                p->assign(interm); 
            } else {
                interm = Iguana::Parser::String(val, n.mName);
                p->assign(interm);
            }

            delete interm;
            continue;
        }

        std::vector<Iguana::Parser*> children;
        for (std::string& name : n.mValues) {
            if (parsers.count(name) == 0) {
                ConstructResult cres;
                cres.mIsError = true;
                cres.mErrorMsg = "Parser " + name + " not found";
                delete gpt;
                return cres;
            }

            children.push_back(parsers[name]);
        }

        Iguana::Parser* interm;
        if (n.mType == "And") {
            interm = Iguana::Parser::And(children, n.mName);
        } else if (n.mType == "Or") {
            interm = Iguana::Parser::Or(children, n.mName);
        }

        p->assign(interm);
        delete interm;
    }

    ConstructResult cres;
    cres.mGpt = gpt;
    cres.mIsError = false;

    return cres;
}
