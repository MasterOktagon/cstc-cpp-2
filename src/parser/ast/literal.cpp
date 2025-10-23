
//
// LITERAL.cpp
//
// implements parsing literals
//

#include "literal.hpp"

#include "../../debug.hpp"
#include "../../errors/errors.hpp"
#include "../../lexer/lexer.hpp"
#include "../../lexer/token.hpp"
#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "base_math.hpp"

#include <cstdint>
#include <regex>
#include <string>
#include <vector>

bool StringIntBiggerThan(string a, string b) {
    if (a.size() > b.size()) { return true; }
    if (b.size() > a.size()) { return false; }
    for (uint32 i = 0; i < a.size(); i++) {
        if (a[i] > b[i]) { return true; }
        if (b[i] > a[i]) { return false; }
    }
    return false;
}

CstType LiteralAST::provide() {
    parser::error(parser::errors["Expression unassignable"], tokens, "Cannot assign an expression result to a value");
    return "@unknown"_c;
}

IntLiteralAST::IntLiteralAST(int bits, string value, bool tsigned, lexer::TokenStream tokens) {
    this->bits        = bits;
    this->const_value = value;
    this->tsigned     = tsigned;
    this->tokens      = tokens;

    // if constant is too big for (u)int32 upgrade to uint64
    if (bits == 32) {
        string v = value;
        if (tsigned) {
            v = v.substr(1);
            if (StringIntBiggerThan(v, "2147483648")) { bits = 64; }
        } else if (StringIntBiggerThan(v, "4294967295")) {
            bits = 64;
        }
    }
}

string IntLiteralAST::emitCST() const {
    return const_value.value();
}

sptr<AST> IntLiteralAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying IntLiteralAST::parse");
    if (tokens.size() == 0) { return nullptr; }
    lexer::TokenStream tokens2 = tokens;

    bool sign = false;
    if (tokens[0].type == lexer::Token::SUB) {
        sign   = true;
        tokens = tokens.slice(1, tokens.size());
    }
    if (tokens.size() == 1) {
        if (tokens[0].type == lexer::Token::INT) {
            return sptr<AST>(new IntLiteralAST(32, tokens[0].value, sign, tokens2));
        } else if (tokens[0].type == lexer::Token::HEX) {
            return sptr<AST>(new IntLiteralAST(32, to_string(stoll(tokens[0].value.substr(2), 0, 12)), sign, tokens2));
        } else if (tokens[0].type == lexer::Token::BINARY) {
            return sptr<AST>(new IntLiteralAST(32, to_string(stoll(tokens[0].value.substr(2), 0, 2)), sign, tokens2));
        }
    }

    // TODO parse binary integers
    return nullptr;
}

TEST_CASE ("Testing IntLiteralAST::parse", "[literal]") {
    string             val    = GENERATE("1", "2", "288", "-1024");
    lexer::TokenStream tokens = lexer::tokenize(val);
    sptr<AST>          ast    = IntLiteralAST::parse(tokens, 0, nullptr);

    SECTION ("return value") {
REQUIRE(ast.get() != nullptr);
        REQUIRE(instanceOf(ast, IntLiteralAST));
    }
    SECTION("sign recognized"){
        REQUIRE((val[0] == '-') == cast2(ast, IntLiteralAST)->sign());
    }
}

void IntLiteralAST::consume(CstType type) {
    regex r("u?int(8|16|32|64|128)");
    bool expected_int = regex_match(type.toString(), r);
    if (expected_int || type == "usize"_c || type == "ssize"_c) {
        bool sig  = type[0] != 'u';
        int  bits;
        if (type == "usize"_c || type == "ssize"_c) {
            bits=64;
        } else {
            bits = std::stoi(type.substr(3 + (!sig), type.size()));
        }

        if (value[0] == '-' && !sig) {
            parser::error(parser::errors["Sign mismatch"],
                          {tokens[0], tokens[1]},
                          "Found a signed value (expected \e[1m"s + type + "\e[0m)");
        }
        tsigned    = sig;
        this->bits = bits;

        // TODO fix this
        // if (StringIntBiggerThan(value.substr(value[0] == '-'),
        //                        std::to_string((1 << (bits - tsigned)) - (value[0] != '-')))) {
        //    parser::warn("Integer too big", tokens,
        //                 "trying to fit a number too big into "s + type + ". This will lead to information loss.",
        //                 17);
        //}
    } else if (type != "@unknown"_c) {
        parser::error(parser::errors["Type mismatch"], tokens, "expected a \e[1m"s + type + "\e[0m, found int");
    }
}

BoolLiteralAST::BoolLiteralAST(string value, lexer::TokenStream tokens) {
    this->const_value = value;
    this->tokens = tokens;
}

string BoolLiteralAST::emitCST() const {
    return const_value.value();
}

sptr<AST> BoolLiteralAST::parse(lexer::TokenStream tokens, int, symbol::Namespace*, string) {
    DEBUG(4, "Trying BoolLiteralAST::parse");
    if (tokens.size() == 1) {
        if (tokens[0].value == "true" || tokens[0].value == "false") {
            return sptr<AST>(new BoolLiteralAST(tokens[0].value, tokens));
        }
    }
    return nullptr;
}

void BoolLiteralAST::consume(CstType type) {
    if (type != "bool"_c) {
        parser::error(parser::errors["Type mismatch"], tokens, "expected a \e[1m"_s + type + "\e[0m, found bool");
    }
}

TEST_CASE ("Testing BoolLiteralAST::parse", "[literal]") {
    string val = GENERATE("true", "false");
    lexer::TokenStream tokens = lexer::tokenize(val);
    sptr<AST> ast = BoolLiteralAST::parse(tokens, 0, nullptr);

    SECTION ("return value") {
        REQUIRE(ast.get() != nullptr);
        REQUIRE(instanceOf(ast, BoolLiteralAST));
    }
}


FloatLiteralAST::FloatLiteralAST(int bits, string value, lexer::TokenStream tokens) {
    this->bits   = bits;
    this->value  = value;
    this->tokens = tokens;
    is_const     = true;
}

string FloatLiteralAST::emitCST() const {
    return value;
}

sptr<AST> FloatLiteralAST::parse(lexer::TokenStream tokens, int, symbol::Namespace*, string) {
    DEBUG(4, "Trying FloatLiteralAST::parse");
    if (tokens.size() < 1) { return nullptr; }
    bool sig = false;
    auto t   = tokens;
    if (tokens[0].type == lexer::Token::Type::SUB || tokens[0].type == lexer::Token::Type::NEC) {
        sig    = true;
        tokens = tokens.slice(1, tokens.size());
    }
    if (tokens.size() < 2) { return nullptr; }
    if (tokens.size() > 3) { return nullptr; }
    if (tokens[0].type == lexer::Token::Type::ACCESS && tokens[1].type == lexer::Token::Type::INT) {
        return share<AST>(new FloatLiteralAST(32, (sig ? string("-0.") : string("0.")) + tokens[1].value + "e00", t));
    } else if (tokens[0].type == lexer::Token::Type::INT && tokens[1].type == lexer::Token::Type::ACCESS) {
        string val = (sig ? string("-") : string("")) + tokens[0].value + ".";
        if (tokens.size() == 3) {
            if (tokens[2].type == lexer::Token::Type::INT) {
                val += tokens[2].value;
            } else {
                return nullptr;
            }
        }
        val += "0e00";
        return share<AST>(new FloatLiteralAST(32, val, t));
    }
    return nullptr;
}

void FloatLiteralAST::consume(CstType type) {
    std::regex r("float(16|32|64|128)");
    bool       expected_float = std::regex_match(type, r);
    if (expected_float) {
        int bits   = std::stoi(type.substr(5, type.size()));
        this->bits = bits;
    } else if (type != "@unknown"_c) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      string("expected a \e[1m") + type + "\e[0m, found float" + std::to_string(bits));
    }
}

TEST_CASE ("Testing FloatLiteralAST::parse", "[literal]") {
    string val = GENERATE(".4", "3.4", "2.", "-5.",  "-6.7885");
    lexer::TokenStream tokens = lexer::tokenize(val);
    sptr<AST> ast = FloatLiteralAST::parse(tokens, 0, nullptr);

    SECTION ("return value") {
        REQUIRE(ast.get() != nullptr);
        REQUIRE(instanceOf(ast, FloatLiteralAST));
    }
}

CharLiteralAST::CharLiteralAST(string value, lexer::TokenStream tokens) {
    this->value  = value;
    this->tokens = tokens;
    is_const     = true;
}

sptr<AST> CharLiteralAST::parse(lexer::TokenStream tokens, int, symbol::Namespace*, string) {
    DEBUG(4, "Trying \e[1mCharLiteralAST::parse\e[0m");
    if (tokens.size() != 1) { return nullptr; }
    if (tokens[0].type == lexer::Token::Type::CHAR) {
        if (tokens[0].value.size() == 2) {
            parser::error(parser::errors["Empty char"],
                          tokens,
                          "This char value is empty. This is not supported. Did you mean '\\u0000' ?");
            return share<AST>(new AST());
        }
        std::regex r("'\\\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]'");
        std::regex r2("'\\\\(n|a|r|t|f|v|\\\\|'|\"|)'");
        if (std::regex_match(tokens[0].value, r) || std::regex_match(tokens[0].value, r2) ||
            tokens[0].value.size() == 3) {
            // std::cout<<"skdskdl"<<std::endl;
            return share<AST>(new CharLiteralAST(tokens[0].value, tokens));
        }
        parser::error(parser::errors["Invalid char"],
                      tokens,
                      "This char value is not supported. Chars are meant to hold only one character. Did you mean to "
                      "use \"Double quotes\" ?");
        return share<AST>(new AST());
    }
    return nullptr;
}

string CharLiteralAST::getValue() const {
    if (this->value == "'\\n'") { return "u0x000A"; }
    if (this->value == "'\\t'") { return "u0x0009"; }
    if (this->value == "'\\v'") { return "u0x000B"; }
    if (this->value == "'\\f'") { return "u0x000C"; }
    if (this->value == "'\\r'") { return "u0x000D"; }
    if (this->value == "'\\a'") { return "u0x0007"; }
    if (this->value == "'\\\"'") { return "u0x0022"; }
    if (this->value == "'\\\\'") { return "u0x005C"; }
    if (this->value == "'\\\''") { return "u0x0027"; }

    if (this->value[1] == '\\' && this->value.size() == 8) {
        return string("u0x") + this->value.substr(3, 2) + this->value.substr(5, 2);
    }

    return std::to_string((uint16_t) this->value[1]);
}

void CharLiteralAST::consume(CstType type) {
    if (type != "char"_c) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      string("expected a \e[1m") + type + "\e[0m, found char");
    }
}

TEST_CASE ("Testing CharLiteralAST::parse", "[literal]") {
    string val = GENERATE("'c'", "'\\u1384'", "'\\t'");
    lexer::TokenStream tokens = lexer::tokenize(val);
    sptr<AST> ast = CharLiteralAST::parse(tokens, 0, nullptr);

    SECTION ("return value") {
        REQUIRE(ast.get() != nullptr);
        REQUIRE(instanceOf(ast, CharLiteralAST));
    }
}

TEST_CASE ("Testing CharLiteralAST::parse - disallowed chars", "[literal]") {
    string val = GENERATE("''", "'ab'");
    lexer::TokenStream tokens = lexer::tokenize(val);
    RAISED(CharLiteralAST::parse(tokens, 0, nullptr));
}

StringLiteralAST::StringLiteralAST(string value, lexer::TokenStream tokens) {
    this->value  = value;
    this->tokens = tokens;
    is_const     = true;
}

sptr<AST> StringLiteralAST::parse(lexer::TokenStream tokens, int, symbol::Namespace*, string) {
    DEBUG(4, "Trying StringLiteralAST::parse");
    if (tokens.size() != 1) { return nullptr; }
    if (tokens[0].type == lexer::Token::Type::string) {
        return share<AST>(new StringLiteralAST(tokens[0].value, tokens));
    }
    return nullptr;
}


string StringLiteralAST::getValue() const {
    return value;
}

void StringLiteralAST::consume(CstType type) {
    if (type != "string"_c && type != "char[]"_c) {
        parser::error(parser::errors["Type mismatch"], tokens, string("expected a \e[1m") + type + "\e[0m, found string");
    }
}

sptr<AST> NullLiteralAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() == 1 && tokens[0].type == lexer::Token::NULV) { return share<AST>(new NullLiteralAST(tokens)); }

    return nullptr;
}

void NullLiteralAST::consume(CstType type) {
    DEBUG(4, "Trying \e[1mNullLiteralAST::parse\e[0m");
    if (type[type.size() - 1] == '?') {
        this->type = type;
    } else if (type != "@unknown"_c) {
        parser::error(parser::errors["Unknown operator"], tokens, "\e[1m"s + type + "::operator null()\e[0m is not defined");
    }
}

sptr<AST> EmptyLiteralAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mEmptyLiteralAST::parse\e[0m");
    if (tokens.size() != 2) { return nullptr; }
    if (tokens[0].type == lexer::Token::INDEX_OPEN && tokens[1].type == lexer::Token::INDEX_CLOSE) {
        return share<AST>(new EmptyLiteralAST(tokens));
    }
    return nullptr;
}

void EmptyLiteralAST::consume(CstType type) {
    if (type.size() < 2 || type.substr(type.size() - 2) != "[]") {
        parser::error(parser::errors["Type mismatch"], tokens, string("expected a \e[1m") + type + "\e[0m, found an empty array");
    } else if (type != "@unknown") {
        this->type = type;
    }
}

sptr<AST> ArrayFieldMultiplierAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mArrayFieldMultiplierAST::parse\e[0m");
    lexer::TokenStream::Match m = tokens.rsplitStack({lexer::Token::X});
    if (m.found()) {
        DEBUG(3, "ArrayFieldMultiplierAST::parse");
        sptr<AST> content = math::parse(m.before(), local, sr);
        if (content == nullptr) {
            parser::error(parser::errors["Expression expected"], m.after(), "Expected a valid expression before 'x'", 0);
            return ERR;
        }
        sptr<AST> amount  = math::parse(m.after(), local, sr);
        if (amount == nullptr) {
            parser::error(parser::errors["Amount expected"], m.after(), "Expected an amount after 'x'", 0);
            return ERR;
        }
        amount->consume("usize"_c);

        DEBUG(5, "\tDone!");
        return share<AST>(new ArrayFieldMultiplierAST(tokens,content, amount));
    }
    return nullptr;
}

void ArrayFieldMultiplierAST::consume(CstType type) {
    content->consume(type);
    is_const = content->is_const && amount->is_const;
    if (is_const) {
        value = std::stoll(amount->value);
    }
}

CstType ArrayFieldMultiplierAST::provide() {
    parser::error(parser::errors["Expression unassignable"], tokens, "Cannot assign an expression result to a value");
    return "@unknown"_c;
}

sptr<AST> ArrayLiteralAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mArrayLiteralAST::parse\e[0m");
    if (tokens.size() < 2) { return nullptr; }
    if (tokens[0].type == lexer::Token::INDEX_OPEN && tokens[-1].type == lexer::Token::INDEX_CLOSE) {
        DEBUG(2, "ArrayLiteralAST::parse");
        lexer::TokenStream tokens2 = tokens.slice(1, -1);
        lexer::TokenStream buffer  = lexer::TokenStream({});
        std::vector<sptr<AST>> contents = {};
        while (!tokens2.empty()) {
            DEBUG(5, "\tbuffer: "s + str(&buffer));
            lexer::TokenStream::Match next = tokens2.rsplitStack({lexer::Token::COMMA});
            if (next.found()) {
                buffer = next.before();
                tokens2 = next.after();
            } else {
                buffer = tokens2;
                tokens2 = lexer::TokenStream({});
                DEBUG(3, "\tready!");
            }
            if (buffer.empty()) { continue; }

            sptr<AST> expr = parser::parseOneOf(buffer,{ArrayFieldMultiplierAST::parse, math::parse}, local + 1, sr, "");
            if (expr == nullptr) {
                parser::error(parser::errors["Expression expected"],buffer,"expected a valid expression");
                continue;
            }
            contents.push_back(expr);
        }
        return share<AST>(new ArrayLiteralAST(tokens, contents));
    }
    return nullptr;
}

void ArrayLiteralAST::consume(CstType type) {
    if ((type.size() < 2 || type.substr(type.size() - 2) != "[]") && type != "@unknown") {
        parser::error(parser::errors["Type mismatch"], tokens, string("expected a \e[1m") + type + "\e[0m, found an array");
        return;
    }
    is_const = true;
    for (sptr<AST> a : contents) {
        a->consume(type.substr(0, type.size() - 2));
        is_const = is_const && a->is_const;
    }
}