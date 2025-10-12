
#include "token.hpp"

#include "../snippets.hpp"

#include <algorithm>
#include <memory>
#include <stack>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;

/// \brief construct a token
lexer::Token::Token(Type         type,
                    string       value,
                    uint32       line,
                    uint32       column,
                    sptr<string> filename,
                    sptr<string> line_contents,
                    sptr<Token>  expanded,
                    sptr<string> include) {
    this->line          = line;
    this->column        = column;
    this->type          = type;
    this->value         = value;
    this->line_contents = line_contents;
    this->filename      = filename;
    this->include       = include;
    this->expanded      = expanded;
}

bool lexer::Token::operator==(Token other) {
    return other.type == type and other.line == line and other.column == column;
}

bool lexer::Token::operator!=(Token other) {
    return !(other == *this);
}
/// \brief create a string representation
string lexer::Token::_str() const {
    return value;
}
/// \brief create a string representation
string lexer::TokenStream::_str() const {
    string s;
    for (uint64 t = start; t < stop; t++) { s += tokens->at(t).value + " "; }
    return s;
}

/// \brief construct a new Match
///
lexer::TokenStream::Match::Match(uint64 at, const TokenStream* on) {
    this->at        = at;
    this->was_found = false;
    this->on        = on;
}

/// \brief construct a new Match
///
lexer::TokenStream::Match::Match(bool){
    this->was_found = false;
    this->at = 0;
}

/// \brief construct a new TokenStream
///
/// \param tokens actual token data. Is shared between many TokenStreams to save space
/// \param start virtual start index on vactor
/// \param stop if stop is 0xFFFF'FFFF'FFFF'FFFF (uint64 max value), stop will be chosen as the last element of tokens
lexer::TokenStream::TokenStream(sptr<vector<Token>> tokens, uint64 start, uint64 stop) {
    this->tokens = tokens;
    if (stop == 0xFFFF'FFFF'FFFF'FFFF && tokens != nullptr) { stop = tokens->size(); }
    this->stop  = stop;
    this->start = start;
}

TEST_CASE ("Testing lexer::TokenStream::TokenStream", "[tokens]") {
    // create TokenStream
    vector<lexer::Token> tokens = {lexer::Token(), lexer::Token()};
    lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
    REQUIRE(t.size() == 2);
    REQUIRE(t.start == 0);
    REQUIRE(t.stop == 2);
}

/// \brief get a substream of this stream
///
/// slicing (extracting a substream) is a core requirement of this compiler.
/// We do this by modifying the start and stop values of a newly created TokenStream object
///
/// \param start start index offset from this start. Negative values will start from the Back
/// \param stop  stop index offset from this <u>start</u>. Negative values will start from the Back
lexer::TokenStream lexer::TokenStream::slice(int64 start, int64 stop) const {
    return lexer::TokenStream(tokens, this->start+start, this->start+stop);
}

TEST_CASE ("Testing lexer::TokenStream::slice", "[tokens]") {
    // create TokenStream
    vector<lexer::Token> tokens = {lexer::Token(), lexer::Token(), lexer::Token(), lexer::Token()};
    lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens)).slice(1,3);
    REQUIRE(t.size() == 2);
    REQUIRE(t.start == 1);
    REQUIRE(t.stop == 3);
}

/// \brief get a Token at specific position
///
/// May raise a std::range_error
///
/// \param idx relative index from start index. Negative values will start from the Back
/// \return nullToken when no token vector was set, otherwise try to return Token.
lexer::Token lexer::TokenStream::get(int64 idx) const {
    if (idx < 0) { idx += size(); }
    if (tokens == nullptr) { return nullToken; }
    return tokens->at(start + idx);
}

lexer::TokenStream::Match lexer::TokenStream::splitStack(
    initializer_list<lexer::Token::Type> sep,
    uint64                               start_idx,
    map<lexer::Token::Type, lexer::Token::Type> mapping) const {

        map<lexer::Token::Type, lexer::Token::Type> mapping_rev = mapping;
        reverse(mapping_rev.begin(), mapping_rev.end());

        stack<lexer::Token::Type> typestack = {};

        uint32 idx = 0;
        for(;idx < size(); idx++){
            Token& t = tokens->at(idx);
            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t)) {
                return Match(idx, this);
            }

            if (mapping.count(t.type)){
                typestack.push(t.type);
            }
            else if (mapping_rev.count(t.type)){
                if (typestack.top() == mapping_rev[t.type]){
                    typestack.push(t.type);
                }
                else {
                    // ERROR
                }
                typestack.pop();
            }

            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t)) {
                return Match(idx, this);
            }
        }
        while (not typestack.empty()){
            // ERROR
            typestack.pop();
        }

        return false;
    }


