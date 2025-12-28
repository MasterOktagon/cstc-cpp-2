
#include "token.hpp"

#include "../snippets.hpp"

#include <algorithm>
// #include <cstddef>
// #include <iostream>
#include "../debug.hpp"
#include "../errors/errors.hpp"

#include <iterator>
#include <memory>
#include <stack>
#include <string>
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
                    sptr<lexer::TokenStream> include) {
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
    this->was_found = true;
    this->on        = make_shared<TokenStream>(*on);
}

/// \brief construct a new Match
///
lexer::TokenStream::Match::Match(bool) {
    this->was_found = false;
    this->at        = 0;
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
    if (start < 0) start = size()+start;
    if (stop < 0) stop = size()+stop;
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

/// \brief flip values of a pair
template<typename A, typename B>
pair<B,A> flip(const pair<A,B>& p)
{
    return pair<B,A>(p.second, p.first);
}

/// \brief flip values of a std::map
template<typename A, typename B>
map<B,A> invert(const map<A,B>& src)
{
    map<B,A> dst;
    transform(src.begin(), src.end(), inserter(dst, dst.begin()), 
                   flip<A,B>);
    return dst;
}


lexer::TokenStream::Match lexer::TokenStream::splitStack(
    initializer_list<lexer::Token::Type> sep,
    uint64                               start_idx,
    map<lexer::Token::Type, lexer::Token::Type> mapping) const {

        map<lexer::Token::Type, lexer::Token::Type> mapping_rev = invert(mapping);
        stack<lexer::Token> typestack = {};

        uint32 idx = start_idx;
        for(;idx < size(); idx++){
            Token& t = tokens->at(this->start+idx);
            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t.type) != sep.end()) {
                return Match(idx, this);
            }

            if (mapping.count(t.type)){
                typestack.push(t);
            }
            else if (mapping_rev.count(t.type)){
                if (typestack.top().type != mapping_rev[t.type]){
                    parser::error({0, "Expected '"_s + to_string(mapping_rev[typestack.top().type]) + "'"},{t},
                        "Expected '"_s + to_string(mapping_rev[typestack.top().type]) + "', found " + t.value);
                    parser::note({typestack.top()}, "to match this '"_s + to_string(typestack.top().type));
                }
                typestack.pop();
            }

            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t.type) != sep.end()) {
                return Match(idx, this);
            }
        }
        while (not typestack.empty()){
            parser::error({0, "Unclosed '"_s + to_string(typestack.top().type) + "'"}, {typestack.top()},"");
            typestack.pop();
        }

        return false;
    }

TEST_CASE ("Testing lexer::TokenStream::splitStack", "[tokens]") {
    SECTION("checking positive example"){
        // create TokenStream
        vector<lexer::Token> tokens = {lexer::Token::OPEN,lexer::Token::COMMA,lexer::Token::CLOSE,lexer::Token::COMMA,lexer::Token::INT, lexer::Token::COMMA, lexer::Token::INT};
        lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
        lexer::TokenStream::Match m = t.splitStack({lexer::Token::COMMA});

        REQUIRE(m.found());
        REQUIRE((int64)m == 3);
        REQUIRE(m.before().size() == 3);
        REQUIRE(m.after().size() == 3);
    }
    SECTION("checking negaitve example"){
        vector<lexer::Token> tokens = {lexer::Token::OPEN,lexer::Token::COMMA,lexer::Token::CLOSE};
        lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
        lexer::TokenStream::Match m = t.splitStack({lexer::Token::COMMA});

        REQUIRE(not m.found());
    }
    SECTION("checking splitting at front"){
        vector<lexer::Token> tokens = {lexer::Token::COMMA,lexer::Token::INT};
        lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
        lexer::TokenStream::Match m = t.splitStack({lexer::Token::COMMA});

        REQUIRE(m.found());
        REQUIRE((int64)m == 0);
    }
}

lexer::TokenStream::Match lexer::TokenStream::rsplitStack(
    initializer_list<lexer::Token::Type> sep,
    uint64                               start_idx,
    map<lexer::Token::Type, lexer::Token::Type> mapping_rev) const {

        map<lexer::Token::Type, lexer::Token::Type> mapping = invert(mapping_rev);
        stack<lexer::Token> typestack = {};

        int32 idx = size()-1-start_idx;
        for(;idx >= 0; idx--){
            Token& t = tokens->at(this->start+idx);
            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t.type) != sep.end()) {
                return Match(idx, this);
            }

            if (mapping.count(t.type)){
                typestack.push(t);
            }
            else if (mapping_rev.count(t.type)){
                if (typestack.top().type != mapping_rev[t.type]){
                    parser::error({0, "Expected '"_s + to_string(mapping_rev[typestack.top().type]) + "'"},{t},
                        "Expected '"_s + to_string(mapping_rev[typestack.top().type]) + "', found " + t.value);
                    parser::note({typestack.top()}, "to match this '"_s + to_string(typestack.top().type));
                }
                typestack.pop();
            }

            if (typestack.size() == 0 and find(sep.begin(), sep.end(), t.type) != sep.end()) {
                return Match(idx, this);
            }
        }
        while (not typestack.empty()){
            parser::error({0, "Unopened '"_s + to_string(typestack.top().type) + "'"}, {typestack.top()}, "");
            typestack.pop();
        }

        return false;
    }

TEST_CASE ("Testing lexer::TokenStream::rsplitStack", "[tokens]") {
    SECTION("checking positive example"){
        // create TokenStream
        vector<lexer::Token> tokens = {lexer::Token::COMMA,lexer::Token::INT, lexer::Token::COMMA, lexer::Token::INT,lexer::Token::OPEN,lexer::Token::COMMA,lexer::Token::CLOSE};
        lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
        lexer::TokenStream::Match m = t.rsplitStack({lexer::Token::COMMA});

        REQUIRE(m.found());
        REQUIRE((int64)m == 2);
        REQUIRE(m.before().size() == 2);
        REQUIRE(m.after().size() == 4);
    }
    SECTION("checking negaitve example"){
        vector<lexer::Token> tokens = {lexer::Token::OPEN,lexer::Token::COMMA,lexer::Token::CLOSE};
        lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
        lexer::TokenStream::Match m = t.rsplitStack({lexer::Token::COMMA});

        REQUIRE(not m.found());
    }
}


/// \brief split this Tokenstream on every occurance of a specific type
///
/// \param sep list of tokens to be seperated at
/// \param allow_empty whether to allow empty blocks
vector<lexer::TokenStream> lexer::TokenStream::list(initializer_list<lexer::Token::Type> sep, bool allow_empty, string what) const {
    vector<lexer::TokenStream> streams = {}; 
    int32 start_idx = 0;
    lexer::TokenStream::Match m = splitStack(sep);

    while (m.found()){
        lexer::TokenStream tokens = slice(start_idx, m);
        if (not allow_empty and tokens.empty()){
            parser::error({0, "Expected "_s + what}, slice(max<int32>(0,start_idx-1), min<int32>(m+1, size())), "Expected "_s + what + " after '" + get(max<int32>(0,start_idx-1)).value + "', but found nothing");
        } else if (not tokens.empty()) {
            streams.push_back(tokens);
        }
        start_idx = m+1;
        m = splitStack(sep, start_idx);
    }
    lexer::TokenStream tokens = slice(start_idx, size());
    if (not allow_empty and tokens.empty()){
        parser::error({0, "Expected "_s + what}, slice(max<int32>(0,start_idx-1), size()), "Expected "_s + what + ", but found nothing");
    } else if (not tokens.empty()) {
        streams.push_back(tokens);
    }

    return streams;
}

TEST_CASE ("Testing lexer::TokenStream::list", "[tokens]") {
    vector<lexer::Token> tokens = {lexer::Token::COMMA,lexer::Token::INT,
                                    lexer::Token::COMMA,lexer::Token::INT,
                                    lexer::Token::COMMA,lexer::Token::OPEN,lexer::Token::COMMA, lexer::Token::CLOSE,
                                    lexer::Token::COMMA,lexer::Token::FLOAT};
    lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
    vector<lexer::TokenStream> result = t.list({lexer::Token::COMMA}, true);

    REQUIRE(result.size() == 4);
    REQUIRE(result[0].size() == 1);
    REQUIRE(result[0][0].type == lexer::Token::INT);
    REQUIRE(result[1].size() == 1);
    REQUIRE(result[2].size() == 3);
    REQUIRE(result[3].size() == 1);
}

void lexer::TokenStream::cut(usize from, usize to){
    tokens->erase(tokens->begin()+start+from, tokens->begin()+start+to);
    stop -= to-from;
}

TEST_CASE("Testing lexer::TokenStream::cut", "[tokens]"){
    vector<lexer::Token> tokens = {lexer::Token::COMMA,lexer::Token::INT,
                                    lexer::Token::COMMA,lexer::Token::INT,
                                    lexer::Token::COMMA,lexer::Token::OPEN,lexer::Token::COMMA, lexer::Token::CLOSE,
                                    lexer::Token::COMMA,lexer::Token::INT};
    lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));

    t.cut(0,4);
    REQUIRE(t.size() == 6);
    REQUIRE(t[1].type == lexer::Token::OPEN);
}

void lexer::TokenStream::paste(lexer::TokenStream t, usize idx){
    for (lexer::Token tok : *(t.tokens)){
        tokens->insert(tokens->begin() + (idx++), tok);
    }
    stop += t.size();
}

TEST_CASE("Testing lexer::TokenStream::include", "[tokens]"){
    vector<lexer::Token> tokens = {lexer::Token::COMMA,lexer::Token::INT,
                                    lexer::Token::COMMA,lexer::Token::INT};
    lexer::TokenStream   t      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens));
    vector<lexer::Token> tokens2 = {lexer::Token::OPEN,lexer::Token::CLOSE};
    lexer::TokenStream   t2      = lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens2));
    
    t.paste(t2, 3);
    REQUIRE(t.size() == 6);
    REQUIRE(t[3].type == lexer::Token::OPEN);
    REQUIRE(t[4].type == lexer::Token::CLOSE);
}

lexer::TokenStream lexer::TokenStream::copy(){
    vector<lexer::Token> toks = *tokens;
    return lexer::TokenStream(make_shared<vector<lexer::Token>>(toks));
}

void lexer::TokenStream::include(int64 start, int64 stop, lexer::TokenStream tokens){
    sptr<lexer::TokenStream> t = make_shared<lexer::TokenStream>(slice(start, stop).copy());
    cut(start, stop);
    for (Token ts : *tokens.tokens){
        ts.include = t;
    }
    paste(tokens, start);
}

lexer::TokenStream lexer::TokenStream::none() {return {nullptr};}

string to_string(lexer::Token::Type t){
    switch (t) {
        default: return "symbol/none";        ///< undefined token
        case lexer::Token::END_CMD: return ";";

                    //  LITERALS  //
        case lexer::Token::NULV: return "null";

                    //    MATH    //
        case lexer::Token::SET: return "=";
        case lexer::Token::ADD: return "+";
        case lexer::Token::SUB: return "-";
        case lexer::Token::MUL: return "*";
        case lexer::Token::DIV: return "/";
        case lexer::Token::MOD: return "%";
        case lexer::Token::POW: return "**";
        case lexer::Token::INC: return "++";
        case lexer::Token::DEC: return "--";
        //NEC      ,        ///< `- <int>`

        case lexer::Token::NEG: return "~";
        case lexer::Token::AND: return "&";
        case lexer::Token::OR: return "|";
        case lexer::Token::XOR: return "^";
        case lexer::Token::SHL: return "<<";
        case lexer::Token::SHR: return ">>";
        case lexer::Token::LSHR: return ">>>";

                    //  LOGICAL   //
        case lexer::Token::LAND: return "and";
        case lexer::Token::LOR: return "or";
        case lexer::Token::NOT: return "not";

                    // COMPARISON //
        case lexer::Token::EQ: return "==";
        case lexer::Token::NEQ: return "!=";
        case lexer::Token::LT: return "<";
        case lexer::Token::GT: return ">";
        case lexer::Token::GEQ: return ">=";
        case lexer::Token::LEQ: return "<=";

                    //    FLOW    //
        case lexer::Token::QM: return "?";
        case lexer::Token::IN: return ":";
        case lexer::Token::UNPACK: return "<-";
        case lexer::Token::REF: return "#";
        case lexer::Token::RMREF: return "#!";
        case lexer::Token::RMREFT: return "&!";
                
        case lexer::Token::SUBNS: return "::";
        case lexer::Token::ACCESS: return ".";
        case lexer::Token::COMMA: return ",";
        case lexer::Token::DOTDOT: return "..";
        case lexer::Token::DOTDOTDOT: return "...";

                    // PARANTHESIS //
        case lexer::Token::OPEN: return "(";
        case lexer::Token::CLOSE: return ")";
        case lexer::Token::BLOCK_OPEN: return "{";
        case lexer::Token::BLOCK_CLOSE: return "}";
        case lexer::Token::INDEX_OPEN: return "[";
        case lexer::Token::INDEX_CLOSE: return "]";

                    //  KEYWORDS  //
        case lexer::Token::IF: return "if";
        case lexer::Token::ELSE: return "else";
        case lexer::Token::FOR: return "for";
        case lexer::Token::WHILE: return "while";
        case lexer::Token::LOOP: return "loop";
        case lexer::Token::THROW: return "throw";
        case lexer::Token::BREAK: return "break";
        case lexer::Token::CONTINUE: return "continue";
        case lexer::Token::NOIMPL: return "noimpl";
        case lexer::Token::RETURN: return "return";
        case lexer::Token::AS: return "as";
        case lexer::Token::OPERATOR: return "operator";
        case lexer::Token::SWITCH: return "switch";
        case lexer::Token::CASE: return "case";
        case lexer::Token::FINALLY: return "finally";
        case lexer::Token::NEW: return "new";
        case lexer::Token::DELETE: return "delete";

        case lexer::Token::IMPORT: return "import";
        case lexer::Token::REQ: return "req";
        case lexer::Token::QER: return "qer";
        case lexer::Token::MACRO: return "macro";
        case lexer::Token::INCLUDE: return "include";

        case lexer::Token::CLASS: return "class";
        case lexer::Token::ENUM: return "enum";
        case lexer::Token::STRUCT: return "struct";
        case lexer::Token::VIRTUAL: return "virtual";
        case lexer::Token::ABSTRACT: return "abstract";
        case lexer::Token::FINAL: return "final";
        case lexer::Token::NAMESPACE: return "namespace";
        case lexer::Token::FRIEND: return "friend";
        case lexer::Token::PROTECTED: return "protected";
        case lexer::Token::PRIVATE: return "private";
        case lexer::Token::STATIC: return "static";
        case lexer::Token::PUBLIC: return "public";
        case lexer::Token::MUT: return "mut";
        case lexer::Token::CONST: return "const";
        case lexer::Token::NOWRAP: return "nowrap";
        case lexer::Token::X: return "x";
    }
}
