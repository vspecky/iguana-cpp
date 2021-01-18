#pragma once

#include <string>

class CodeTracker {
private:
    std::string* mCode;

public:
    int mIdx;
    int mLin;
    int mCol;

    CodeTracker(std::string* code);
    CodeTracker* copy();
    void skipWhitespace();
    bool matchString(std::string const& toMatch);
    void consume(std::string const& toConsume);
    void display();
    void copyInfo(CodeTracker*);
};
