#include "parser.hpp"

#include "../debug/debug.hpp"
#include "../lexer/token.hpp"
#include "../lexer/lexer.hpp"
#include "ast/ast.hpp"
#include "errors.hpp"
#include "symboltable.hpp"

#include <cmath>
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <vector>

#define INT_OPS(type)                                               \
    if (type1 == type) {                                            \
        if (op == lexer::Token::Type::NOT) return ""_c;             \
        if (op == lexer::Token::Type::NEG) return type;             \
        if (type2 == "bool"_c) {                                    \
            if (op == lexer::Token::Type::AS) return "bool"_c;      \
        }                                                           \
        if (std::regex_match(type2, int_regex)) {                   \
            if (op == lexer::Token::Type::AS) return type2;         \
        }                                                           \
        if (std::regex_match(type2, flt_regex)) {                   \
            if (op == lexer::Token::Type::AS) return type2;         \
        }                                                           \
        if (type2 == "char"_c) {                                    \
            if (op == lexer::Token::Type::AS) return "char"_c;      \
        }                                                           \
        if (type2 == type) {                                        \
            if (op == lexer::Token::Type::AND) return type;         \
            if (op == lexer::Token::Type::OR) return type;          \
            if (op == lexer::Token::Type::XOR) return type;         \
            if (op == lexer::Token::Type::ADD) return type;         \
            if (op == lexer::Token::Type::SUB) return type;         \
            if (op == lexer::Token::Type::MUL) return type;         \
            if (op == lexer::Token::Type::DIV) return type;         \
            if (op == lexer::Token::Type::MOD) return type;         \
            if (op == lexer::Token::Type::POW) return type;         \
            if (op == lexer::Token::Type::LESS) return "bool"_c;    \
            if (op == lexer::Token::Type::GREATER) return "bool"_c; \
            if (op == lexer::Token::Type::GEQ) return "bool"_c;     \
            if (op == lexer::Token::Type::LEQ) return "bool"_c;     \
            if (op == lexer::Token::Type::EQ) return "bool"_c;      \
            if (op == lexer::Token::Type::NEQ) return "bool"_c;     \
        }                                                           \
    }

#define FLT_OPS(type)                                               \
    if (type1 == type) {                                            \
        if (type2 == "bool"_c) {                                    \
            if (op == lexer::Token::Type::AS) return "bool"_c;      \
        }                                                           \
        if (std::regex_match(type2, int_regex)) {                   \
            if (op == lexer::Token::Type::AS) return type2;         \
        }                                                           \
        if (std::regex_match(type2, flt_regex)) {                   \
            if (op == lexer::Token::Type::AS) return type2;         \
        }                                                           \
        if (type2 == "char"_c) {                                    \
            if (op == lexer::Token::Type::AS) return "char"_c;      \
        }                                                           \
        if (type2 == type) {                                        \
            if (op == lexer::Token::Type::ADD) return type;         \
            if (op == lexer::Token::Type::SUB) return type;         \
            if (op == lexer::Token::Type::MUL) return type;         \
            if (op == lexer::Token::Type::DIV) return type;         \
            if (op == lexer::Token::Type::MOD) return type;         \
            if (op == lexer::Token::Type::POW) return type;         \
            if (op == lexer::Token::Type::LESS) return "bool"_c;    \
            if (op == lexer::Token::Type::GREATER) return "bool"_c; \
            if (op == lexer::Token::Type::GEQ) return "bool"_c;     \
            if (op == lexer::Token::Type::LEQ) return "bool"_c;     \
            if (op == lexer::Token::Type::EQ) return "bool"_c;      \
            if (op == lexer::Token::Type::NEQ) return "bool"_c;     \
        }                                                           \
    }

CstType parser::hasOp(CstType type1, CstType type2, lexer::Token::Type op) {
    if (type1 == "@unknown"_c || type2 == "@unknown"_c) { return "@unknown"_c; }

    if (type2 == type1 + '?' && op == lexer::Token::Type::AS) { return type2; }

    std::regex int_regex("u?int(8|16|32|64|128)");
    std::regex flt_regex("float(16|32|64|80)");
    if (type1 == "bool"_c) {
        if (op == lexer::Token::Type::NOT) { return "bool"_c; }
        if (op == lexer::Token::Type::NEG) { return ""_c; }
        if (type2 == "bool"_c) {
            if (op == lexer::Token::Type::LAND) { return "bool"_c; }
            if (op == lexer::Token::Type::LOR) { return "bool"_c; }
            if (op == lexer::Token::Type::NEQ) { return "bool"_c; }
            if (op == lexer::Token::Type::EQ) { return "bool"_c; }
            if (op == lexer::Token::Type::AND) { return "bool"_c; }
            if (op == lexer::Token::Type::OR) { return "bool"_c; }
            if (op == lexer::Token::Type::XOR) { return "bool"_c; }
        }
        if (std::regex_match(type2, int_regex)) {
            if (op == lexer::Token::Type::AS) { return type2; }
        }
    }
    INT_OPS("@int"_c);
    INT_OPS("uint8"_c);
    INT_OPS("uint16"_c);
    INT_OPS("uint32"_c);
    INT_OPS("uint64"_c);
    INT_OPS("usize"_c);

    INT_OPS("int8"_c);
    INT_OPS("int16"_c);
    INT_OPS("int32"_c);
    INT_OPS("int64"_c);
    INT_OPS("ssize"_c);

    FLT_OPS("float16"_c);
    FLT_OPS("float32"_c);
    FLT_OPS("float64"_c);
    FLT_OPS("float80"_c);

    return ""_c;
}

String match_token_clamp(lexer::Token::Type t) {
    switch (t) {
        case lexer::Token::Type::OPEN        : return "PARANTHESIS";
        case lexer::Token::Type::CLOSE       : return "PARANTHESIS";

        case lexer::Token::Type::INDEX_OPEN  : return "INDEX";
        case lexer::Token::Type::INDEX_CLOSE : return "INDEX";

        case lexer::Token::Type::BLOCK_OPEN  : return "BLOCK";
        case lexer::Token::Type::BLOCK_CLOSE : return "BLOCK";

        default                              : return "UNKNOWN";
    }
}

sptr<AST> parser::parseOneOf(lexer::TokenStream                tokens,
                             std::vector<PARSER_FN_NO_DEFAULT> functions,
                             int                               local,
                             symbol::Namespace*                sr,
                             String                            expected_type) {
    for (auto fn : functions) {
        sptr<AST> r = fn(tokens, local, sr, expected_type);
        if (r != nullptr) {
            DEBUG(2, "parser::parseOneOf: "s + r->emitCST());
            return r;
        }
    }
    return nullptr;
}

/*bool parser::typeEq(String a, String b) {
    if (a == "@unknown" || b == "@unknown") {
        return true; // @unknown should ONLY be used for temporarily unknown types
    }
    if (a == b) { return true; }
    if (a == "@int") {
        return (b == "int8" || b == "int16" || b == "int32" || b == "int64" || b == "int128" || b == "int256" ||
                b == "int512" || b == "int1024" || b == "uint8" || b == "uint16" || b == "uint32" || b == "uint64" || a
== "ssize" || b == "usize" || b == "@uint");
    }
    if (a == "@float") { return b == "float32" || b == "float64"; }
    if (b == "@int") {
        return (a == "int8" || a == "int16" || a == "int32" || a == "int64" || a == "int128" || a == "int256" ||
                a == "int512" || a == "int1024" || a == "uint8" || a == "uint16" || a == "uint32" || a == "uint64" ||
                a == "usize" || a == "ssize"|| a == "@uint");
    }
    if (a == "@uint") {
        return b == "uint8" || b == "uint16" || b == "uint32" || b == "uint64" || b == "uint128" || b == "uint256" ||
               b == "uint512" || b == "uint1024" || b == "@uint" || a == "usize";
    }
    if (b == "@uint") {
        return a == "uint8" || a == "uint16" || a == "uint32" || a == "uint64" || a == "uint128" || a == "uint256" ||
               a == "uint512" || a == "uint1024" || a == "@uint" || a == "usize";
    }
    if (b == "@float") { return a == "float32" || a == "float64"; }

    return false;
}*/

bool parser::isAtomic(CstType type) {
    if (type == "uint8"_c) { return true; }
    if (type == "uint16"_c) { return true; }
    if (type == "uint32"_c) { return true; }
    if (type == "uint64"_c) { return true; }
    if (type == "usize"_c) { return true; }

    if (type == "int8"_c) { return true; }
    if (type == "int16"_c) { return true; }
    if (type == "int32"_c) { return true; }
    if (type == "int64"_c) { return true; }
    if (type == "ssize"_c) { return true; }

    if (type == "char"_c) { return true; }
    if (type == "float16"_c) { return true; }
    if (type == "float32"_c) { return true; }
    if (type == "float64"_c) { return true; }
    if (type == "float80"_c) { return true; }
    if (type[type.size() - 1] == '&') { return true; }
    if (type.size() > 1 && type.substr(type.size() - 2) == "&!") { return true; }
    // if(type[type.size()-1] == '?') return true;
    return false;
}

bool parser::is_snake_case(String text) {
    const std::regex rx("[a-z\\_][a-z0-9\\_]*");
    std::cmatch      m;
    return std::regex_match(text.c_str(), m, rx);
}

bool parser::isPascalCase(String text) {
    const std::regex rx("\\_?[a-z][a-z0-9A-Z]*");
    std::cmatch      m;
    return std::regex_match(text.c_str(), m, rx);
}

bool parser::IsCamelCase(String text) {
    const std::regex rx("\\_?[A-Z][a-z0-9A-Z]*");
    std::cmatch      m;
    return std::regex_match(text.c_str(), m, rx);
}

bool parser::IS_UPPER_CASE(String text) {
    const std::regex rx("[A-Z\\_][A-Z0-9\\_]*");
    std::cmatch      m;
    return std::regex_match(text.c_str(), m, rx);
}

parser::Modifier parser::getModifier(lexer::TokenStream* tok) {
    Modifier m = Modifier::NONE;
    lexer::TokenStream tokens = *tok;

    uint64 i;
    for (i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == lexer::Token::Type::CONST) {
            m = Modifier(m | Modifier::CONST);
        } else if (tokens[i].type == lexer::Token::Type::MUT) {
            m = Modifier(m | Modifier::MUTABLE);
        } else if (tokens[i].type == lexer::Token::Type::STATIC) {
            m = Modifier(m | Modifier::STATIC);
        } else {
            i++;
            break;
        }
    }
    DEBUG(3, "parser::getModifier: "s + std::to_string(i) + "/" + std::to_string(tokens.size()));

    if (i == tokens.size()) {
        tok->tokens = {};
    } else if (tokens.size() > 0) {
        tok->start += i; // not nice
    }
    return m;
}

TEST_CASE ("Testing parser::getModifier", "[util]") {
    SECTION ("slicing ability") {
        lexer::TokenStream t = lexer::tokenize("const ab");
        uint64             l = t.size();
        parser::Modifier   m = parser::getModifier(&t);
        REQUIRE (l == (t.size() - 1));
        REQUIRE (m & parser::Modifier::CONST);
        if (l){};
        if (m){};
    }

}

void parser::checkName(String name, lexer::Token where) {
    if (isAtomic(name)) {
        error(parser::errors["Invalid name"],
              {where},
              "the name "s + name + " is a atomic type and therefore not allowed as name.");
    }
}

/*void parser::allowModifier(Modifier mod, Modifier in, lexer::TokenStream t) {
    for (Modifier i = Modifier::CONST; i < Modifier::END; i = Modifier(uint32(i) * 2)) { // iterate over all modifiers
        if (!(mod & in)) {
            //parser::error("Qualifier not allowed", )
        }
    }
}*/
