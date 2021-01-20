#include "codetracker.h"

#include <string>
#include <iostream>

CodeTracker::CodeTracker(std::string* code) {
    mCode = code;
    mIdx = 0;
    mLin = 1;
    mCol = 1;
}

CodeTracker* CodeTracker::copy() {
    CodeTracker* newTracker = new CodeTracker(mCode);
    newTracker->mIdx = this->mIdx;
    newTracker->mLin = this->mLin;
    newTracker->mCol = this->mCol;

    return newTracker;
}

void CodeTracker::skipWhitespace() {
    int len = mCode->length();

    while (mIdx < len) {
        char ch = mCode->at(mIdx);

        if (!isspace(ch)) break;

        if (ch == '\n') {
            mLin++;
            mCol = 1;
        } else {
            mCol++;
        }

        mIdx++;
    }
}

bool CodeTracker::matchString(std::string const& toMatch) {
    this->skipWhitespace();

    return mCode->substr(mIdx, toMatch.length()) == toMatch;
}

void CodeTracker::consume(std::string const& toConsume) {
    int len = toConsume.length();
    mIdx += len;
    mCol += len;
}

bool CodeTracker::isEOF() {
    this->skipWhitespace();

    return mIdx >= mCode->length();
}

std::string CodeTracker::parseKey(int (*key)(int)) {
    this->skipWhitespace();
    std::string res = "";

    int len = mCode->length();

    while (mIdx < len && key(mCode->at(mIdx)) != 0) {
        res += mCode->at(mIdx);
        mIdx++;
        mCol++;
    }

    return res;
}

std::string CodeTracker::parseCustomSymbols(std::string& toParse) {
    this->skipWhitespace();
    std::string res = "";

    int len = mCode->length();

    while (mIdx < len && toParse.find(mCode->at(mIdx)) != std::string::npos) {
        res += mCode->at(mIdx);
        mIdx++;
        mCol++;
    }

    return res;
}

void CodeTracker::display() {
    std::cout << "Index " << mIdx << std::endl;
    std::cout << "Col   " << mCol << std::endl;
    std::cout << "Lin   " << mLin << std::endl;
}

void CodeTracker::copyInfo(CodeTracker* other) {
    mLin = other->mLin;
    mCol = other->mCol;
    mIdx = other->mIdx;
}
