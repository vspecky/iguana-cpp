#pragma once

#include <string>

class CodeTracker {
private:
    std::string* mCode;

public:
    int mIdx;
    int mLin;
    int mCol;

    CodeTracker(std::string*);
    CodeTracker* copy();
    void skipWhitespace();
    bool matchString(std::string const& toMatch);
    void consume(std::string const& toConsume);
    std::string parseKey(int (*)(int));
    std::string parseCustomSymbols(std::string&);
    std::string parseAnything();
    std::string parseRegex(const std::string);
    bool isEOF();
    void display();
    void copyInfo(CodeTracker*);
};
