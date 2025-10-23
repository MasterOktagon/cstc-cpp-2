#include "base_math.hpp"

#include "../../build/optimizer_flags.hpp"
#include "../../debug/debug.hpp"
#include "../errors.hpp"
#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "func.hpp"
#include "literal.hpp"
#include "type.hpp"
#include "var.hpp"

// #include <catch2/catch.hpp>
#include <cmath>
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

CstType DoubleOperandAST::getCstType() const {
    return parser::hasOp(left->getCstType(), right->getCstType(), op);
}

uint64 DoubleOperandAST::nodeSize() const {
    return left->nodeSize() + right->nodeSize() + 1;
}

String DoubleOperandAST::_str() const {
    return "<"s + str(left.get()) + op_view + str(right.get()) + (is_const ? " [="s + value + "]>" : ">"s);
}

String DoubleOperandAST::emitCST() const {
    return PUT_PT(left->emitCST() + " " + op_view + " " + right->emitCST(), this->has_pt);
}

CstType DoubleOperandAST::provide() {
    parser::error(parser::errors["Expression unassignable"], tokens, "Cannot assign an expression result to a value");
    return "@unknown"_c;
}

CstType UnaryOperandAST::provide() {
    parser::error(parser::errors["Expression unassignable"], tokens, "Cannot assign an expression result to a value");
    return "@unknown"_c;
}

void DoubleOperandAST::consume(CstType type) {
    std::regex i("u?int(8|16|32|64|128)");
    std::regex f("float(16|32|64|128)");
    bool       int_required   = std::regex_match(type.toString(), i);
    bool       float_required = std::regex_match(type.toString(), f);

    // force the types of the sub-trees
    if (std::regex_match(left->getCstType(), i) && int_required) {
        left->consume(type);
        right->consume(type);
    }
    if (std::regex_match(left->getCstType(), f) && float_required) {
        left->consume(type);
        right->consume(type);
    }

    // check for operator overloading [WIP/TODO]
    CstType ret = parser::hasOp(left->getCstType(), right->getCstType(), op);
    if (ret != "") {
        if (ret != type) {
            parser::error(parser::errors["Type mismatch"],
                          tokens,
                          left->getCstType() + "::operator " + op_view + " (" + right->getCstType() + ") yields " +
                              ret + " (expected \e[1m" + type + "\e[0m)");
        }

        else if (optimizer::do_constant_folding && right->is_const && left->is_const) {
            if (const_folding_fn.count(std::make_tuple(left->getCstType(), right->getCstType()))) {
                value    = const_folding_fn[std::make_tuple(left->getCstType(), right->getCstType())](left->value,
                                                                                                   right->value);
                is_const = true;
            }
        }
    } else {
        parser::error(parser::errors["Unknown operator"],
                      tokens,
                      left->getCstType() + "::operator " + op_view + " (" + right->getCstType() +
                          ") is not implemented.");
    }
}

CstType UnaryOperandAST::getCstType() const {
    return parser::hasOp(left->getCstType(), left->getCstType(), op);
}

uint64 UnaryOperandAST::nodeSize() const {
    return left->nodeSize() + 1;
}

void UnaryOperandAST::consume(CstType type) {
    String ret = parser::hasOp(left->getCstType(), left->getCstType(), op);
    if (ret != "") {
        if (ret != type) {
            parser::error(parser::errors["Type mismatch"],
                          tokens,
                          left->getCstType() + "::operator " + op_view + " () yields " + ret + " (expected \e[1m" +
                              type + "\e[0m)");
        }

        else if (optimizer::do_constant_folding && left->is_const) {
            if (const_folding_fn.count(left->getCstType())) {
                value    = const_folding_fn[left->getCstType()](left->value);
                is_const = true;
            }
        }
    } else {
        parser::error(parser::errors["Unknown operator"],
                      tokens,
                      left->getCstType() + "::operator " + op_view + " () is not implemented.");
    }
}

String UnaryOperandAST::emitCST() const {
    return op_view + left->emitCST();
}

String UnaryOperandAST::_str() const {
    return "<"s + op_view + str(left.get()) + (is_const ? " [="s + value + "]>" : ">"s);
}

//  AddAST

#define CF_FUN_INT(type1, type2, type, op)                                                         \
    {std::make_tuple(type1, type2),                                                                \
     nlambda(String v1, String v2) {return std::to_string(type(std::stoll(v1) op std::stoll(v2))); \
    }                                                                                              \
    }
#define CF_FUN_FLT(type1, type2, type, op)                                                         \
    {std::make_tuple(type1, type2),                                                                \
     nlambda(String v1, String v2) {return std::to_string(type(std::stold(v1) op std::stold(v2))); \
    }                                                                                              \
    }

AddAST::AddAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::ADD;
    this->op_view          = "+";
    this->const_folding_fn = {CF_FUN_INT("uint8"_c, "uint8"_c, uint8, +),
                              CF_FUN_INT("uint16"_c, "uint16"_c, uint16, +),
                              CF_FUN_INT("uint32"_c, "uint32"_c, uint32, +),
                              CF_FUN_INT("uint64"_c, "uint64"_c, uint64, +),
                              CF_FUN_INT("usize"_c, "usize"_c, uint64, +),

                              CF_FUN_INT("int8"_c, "int8"_c, int8, +),
                              CF_FUN_INT("int16"_c, "int16"_c, int16, +),
                              CF_FUN_INT("int32"_c, "int32"_c, int32, +),
                              CF_FUN_INT("int64"_c, "int64"_c, int64, +),
                              CF_FUN_INT("ssize"_c, "ssize"_c, int64, +),

                              CF_FUN_FLT("float16"_c, "float16"_c, float32, +),
                              CF_FUN_FLT("float32"_c, "float32"_c, float32, +),
                              CF_FUN_FLT("float64"_c, "float64"_c, float64, +),
                              CF_FUN_FLT("float80"_c, "float80"_c, float80, +),

                              {{"String"_c, "String"_c}, nlambda(String a, String b) {return a + b;
}
}
}
;
}

AddAST::~AddAST() {};

sptr<AST> AddAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying AddAST::parse");
    if (tokens.size() < 1) { return nullptr; }
    lexer::TokenStream::Match m =
        tokens.splitStack({lexer::Token::Type::ADD, lexer::Token::Type::SUB}, tokens[0].type == lexer::Token::SUB);
    if (m.found()) {
        DEBUGT(2, "\e[1mAddAST::parse\e[0m", &tokens)

        sptr<AST> left = math::parse(m.before(), local, sr, expected_type);
        if (left == nullptr) {
            parser::error(parser::errors["Expression expected"],
                          m.before(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m");
            return share<AST>(new AST());
        }

        sptr<AST> right = math::parse(m.after(), local, sr, expected_type);
        if (right == nullptr) {
            parser::error(parser::errors["Expression expected"],
                          m.after(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m");
            return share<AST>(new AST());
        }
        lexer::Token op = tokens[(uint64) m];
        DEBUG(3, "\top.type: "s + lexer::getTokenName(op.type))
        if (op.type == lexer::Token::Type::ADD) {
            return share<AST>(new AddAST(left, right, tokens));
        }

        else if (op.type == lexer::Token::Type::SUB) {
            return share<AST>(new SubAST(left, right, tokens));
        }
    }
    return nullptr;
}

// SubAST

SubAST::SubAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::SUB;
    this->op_view          = "-";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, -),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, -),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, -),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, -),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, -),

        CF_FUN_INT("int8"_c, "int8"_c, int8, -),
        CF_FUN_INT("int16"_c, "int16"_c, int16, -),
        CF_FUN_INT("int32"_c, "int32"_c, int32, -),
        CF_FUN_INT("int64"_c, "int64"_c, int64, -),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, -),

        CF_FUN_FLT("float16"_c, "float16"_c, float32, -),
        CF_FUN_FLT("float32"_c, "float32"_c, float32, -),
        CF_FUN_FLT("float64"_c, "float64"_c, float64, -),
        CF_FUN_FLT("float80"_c, "float80"_c, float80, -),
    };
}

SubAST::~SubAST() {};

// MulAST

MulAST::MulAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::MUL;
    this->op_view          = "*";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, *),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, *),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, *),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, *),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, *),

        CF_FUN_INT("int8"_c, "int8"_c, int8, *),
        CF_FUN_INT("int16"_c, "int16"_c, int16, *),
        CF_FUN_INT("int32"_c, "int32"_c, int32, *),
        CF_FUN_INT("int64"_c, "int64"_c, int64, *),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, *),

        CF_FUN_FLT("float16"_c, "float16"_c, float32, *),
        CF_FUN_FLT("float32"_c, "float32"_c, float32, *),
        CF_FUN_FLT("float64"_c, "float64"_c, float64, *),
        CF_FUN_FLT("float80"_c, "float80"_c, float80, *),

        {{"String"_c, "usize"_c}, nlambda(String a, String b) {unsigned long long amount = std::stoull(b);
    for (unsigned long long cntr; cntr < amount; cntr++) { a += a; }
    return a;
}
}
}
;
}

MulAST::~MulAST() {};

sptr<AST> MulAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying MulAST::parse");
    if (tokens.size() < 1) { return nullptr; }
    lexer::TokenStream::Match m = tokens.splitStack({lexer::Token::MUL, lexer::Token::DIV, lexer::Token::MOD});
    if (m.found()) {
        DEBUGT(2, "\e[1mMulAST::parse\e[0m", &tokens)

        sptr<AST> left = math::parse(m.before(), local, sr, expected_type);
        if (left == nullptr) {
            parser::error(parser::errors["Expression expected"],
                          m.before(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m");
            return share<AST>(new AST());
        }

        sptr<AST> right = math::parse(m.after(), local, sr, expected_type);
        if (right == nullptr) {
            parser::error(parser::errors["Expression expected"],
                          m.after(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m");
            return share<AST>(new AST());
        }
        lexer::Token op = tokens[(uint64) m];
        DEBUG(3, "\top.type: "s + lexer::getTokenName(op.type))
        if (op.type == lexer::Token::Type::MUL) {
            return share<AST>(new MulAST(left, right, tokens));
        }

        else if (op.type == lexer::Token::Type::DIV) {
            return share<AST>(new DivAST(left, right, tokens));
        }

        else if (op.type == lexer::Token::Type::MOD) {
            return share<AST>(new DivAST(left, right, tokens));
        }
    }
    return nullptr;
}

// DivAST

DivAST::DivAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::DIV;
    this->op_view          = "/";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, /),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, /),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, /),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, /),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, /),

        CF_FUN_INT("int8"_c, "int8"_c, int8, /),
        CF_FUN_INT("int16"_c, "int16"_c, int16, /),
        CF_FUN_INT("int32"_c, "int32"_c, int32, /),
        CF_FUN_INT("int64"_c, "int64"_c, int64, /),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, /),

        CF_FUN_FLT("float16"_c, "float16"_c, float32, /),
        CF_FUN_FLT("float32"_c, "float32"_c, float32, /),
        CF_FUN_FLT("float64"_c, "float64"_c, float64, /),
        CF_FUN_FLT("float80"_c, "float80"_c, float80, /),
    };
}

DivAST::~DivAST() {};

// ModAST

ModAST::ModAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::MOD;
    this->op_view          = "%";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, %),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, %),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, %),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, %),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, %),

        CF_FUN_INT("int8"_c, "int8"_c, int8, %),
        CF_FUN_INT("int16"_c, "int16"_c, int16, %),
        CF_FUN_INT("int32"_c, "int32"_c, int32, %),
        CF_FUN_INT("int64"_c, "int64"_c, int64, %),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, %),
    };
}

ModAST::~ModAST() {};

// PowAST

#define CF_FUN_POW(type, type2)                                                                                    \
    {std::make_tuple(type, type),                                                                                  \
     nlambda(String v1,                                                                                            \
             String v2) {return std::to_string(type2(std::pow(float80(std::stoll(v1)), float80(std::stoll(v2))))); \
    }                                                                                                              \
    }
#define CF_FUN_POW_FLT(type, type2)                                                                                \
    {std::make_tuple(type, type),                                                                                  \
     nlambda(String v1,                                                                                            \
             String v2) {return std::to_string(type2(std::pow(float80(std::stold(v1)), float80(std::stold(v2))))); \
    }                                                                                                              \
    }

PowAST::PowAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::POW;
    this->op_view          = "**";
    this->const_folding_fn = {
        CF_FUN_POW("uint8"_c, uint8),
        CF_FUN_POW("uint16"_c, uint16),
        CF_FUN_POW("uint32"_c, uint32),
        CF_FUN_POW("uint64"_c, uint64),
        CF_FUN_POW("usize"_c, uint64),

        CF_FUN_POW("int8"_c, int8),
        CF_FUN_POW("int16"_c, int16),
        CF_FUN_POW("int32"_c, int32),
        CF_FUN_POW("int64"_c, int64),
        CF_FUN_POW("ssize"_c, int64),

        CF_FUN_POW_FLT("float16"_c, float32),
        CF_FUN_POW_FLT("float32"_c, float32),
        CF_FUN_POW_FLT("float64"_c, float64),
        CF_FUN_POW_FLT("float80"_c, float80),
    };
}

PowAST::~PowAST() = default;

#define STANDARD_MATH_PARSE(tokentype, type1)                                              \
    if (tokens.size() < 1) return nullptr;                                                 \
    lexer::TokenStream::Match m = tokens.splitStack({tokentype});                          \
    if (m.found()) {                                                                       \
        DEBUG(2, "\e[1m"s + #type1 + "::parse\e[0m");                                      \
        sptr<AST> left = math::parse(m.before(), local, sr, expected_type);                \
        if (left == nullptr) {                                                             \
            parser::error(parser::errors["Expression expected"],                           \
                          m.before(),                                                      \
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m"); \
            return share<AST>(new AST());                                                  \
        }                                                                                  \
                                                                                           \
        sptr<AST> right = math::parse(m.after(), local, sr, expected_type);                \
        if (right == nullptr) {                                                            \
            parser::error(parser::errors["Expression expected"],                           \
                          m.after(),                                                       \
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m"); \
            return share<AST>(new AST());                                                  \
        }                                                                                  \
        lexer::Token op = tokens[(uint64) m];                                              \
        if (op.type == tokentype) return share<AST>(new type1(left, right, tokens));       \
    }

sptr<AST> PowAST::parse(PARSER_FN_PARAM) {
    DEBUG(2, "PowAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::POW, PowAST);
    return nullptr;
}

// LorAST

LorAST::LorAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::LOR;
    this->op_view          = "||";
    this->const_folding_fn = {
        {std::make_tuple("bool"_c, "bool"_c),
         nlambda(String v1, String v2) {return (v1 == "true"s || v2 == "true") ? "true"s : "false"s;
}
}
}
;
}

LorAST::~LorAST() {};

sptr<AST> LorAST::parse(PARSER_FN_PARAM) {
    DEBUG(2, "LorAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::LOR, LorAST);
    return nullptr;
}

// LandAST

LandAST::LandAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::LAND;
    this->op_view          = "&&";
    this->const_folding_fn = {
        {std::make_tuple("bool"_c, "bool"_c),
         nlambda(String v1, String v2) {return (v1 == "true"s && v2 == "true") ? "true"s : "false"s;
}
}
}
;
}

LandAST::~LandAST() {};

sptr<AST> LandAST::parse(PARSER_FN_PARAM) {
    DEBUG(2, "LandAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::LAND, LandAST);
    return nullptr;
}

// OrAST

OrAST::OrAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::OR;
    this->op_view          = "|";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, |),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, |),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, |),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, |),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, |),

        CF_FUN_INT("int8"_c, "int8"_c, int8, |),
        CF_FUN_INT("int16"_c, "int16"_c, int16, |),
        CF_FUN_INT("int32"_c, "int32"_c, int32, |),
        CF_FUN_INT("int64"_c, "int64"_c, int64, |),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, |),
    };
}

sptr<AST> OrAST::parse(PARSER_FN_PARAM) {
    DEBUG(2, "OrAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::OR, OrAST);
    return nullptr;
}

// AndAST

AndAST::AndAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::AND;
    this->op_view          = "&";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, &),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, &),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, &),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, &),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, &),

        CF_FUN_INT("int8"_c, "int8"_c, int8, &),
        CF_FUN_INT("int16"_c, "int16"_c, int16, &),
        CF_FUN_INT("int32"_c, "int32"_c, int32, &),
        CF_FUN_INT("int64"_c, "int64"_c, int64, &),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, &),
    };
}

sptr<AST> AndAST::parse(lexer::TokenStream tokens, int local, symbol::Namespace* sr, String expected_type) {
    DEBUG(2, "AndAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::AND, AddAST);
    return nullptr;
}

// XorAST

XorAST::XorAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::XOR;
    this->op_view          = "^";
    this->const_folding_fn = {
        CF_FUN_INT("uint8"_c, "uint8"_c, uint8, ^),
        CF_FUN_INT("uint16"_c, "uint16"_c, uint16, ^),
        CF_FUN_INT("uint32"_c, "uint32"_c, uint32, ^),
        CF_FUN_INT("uint64"_c, "uint64"_c, uint64, ^),
        CF_FUN_INT("usize"_c, "usize"_c, uint64, ^),

        CF_FUN_INT("int8"_c, "int8"_c, int8, ^),
        CF_FUN_INT("int16"_c, "int16"_c, int16, ^),
        CF_FUN_INT("int32"_c, "int32"_c, int32, ^),
        CF_FUN_INT("int64"_c, "int64"_c, int64, ^),
        CF_FUN_INT("ssize"_c, "ssize"_c, int64, ^),
    };
}

sptr<AST> XorAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying XorAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::XOR, XorAST);
    return nullptr;
}

// EqAST

EqAST::EqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::EQ;
    this->op_view          = "==";
    this->const_folding_fn = {
        {std::make_tuple("bool"_c, "bool"_c), nlambda(String v1, String v2) {return (v1 == v2) ? "true"s : "false"s;
}
}
}
;
}

sptr<AST> EqAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying EqAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::EQ, EqAST);
    return nullptr;
}

// NeqAST

NeqAST::NeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::NEQ;
    this->op_view          = "!=";
    this->const_folding_fn = {
        {std::make_tuple("bool"_c, "bool"_c), nlambda(String v1, String v2) {return (v1 != v2) ? "true"s : "false"s;
}
}
}
;
}

sptr<AST> NeqAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying NeqAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::NEQ, EqAST);
    return nullptr;
}

/*
sptr<AST> GtAST::parse(PARSER_FN_PARAM) {
    lexer::Token::Type tokentype = lexer::Token::GREATER;

    if (tokens.size() < 1) { return nullptr; }
    lexer::TokenStream::Match m = tokens.splitStack({tokentype});
    if (m.found()) {
        sptr<AST> left = math::parse(m.before(), local, sr, expected_type);
        if (left == nullptr) {
            parser::error("Expression expected",
                          m.before(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m",
                          111);
            return share<AST>(new AST());
        }

        sptr<AST> right = math::parse(m.after(), local, sr, expected_type);
        if (right == nullptr) {
            parser::error("Expression expected",
                          m.after(),
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m",
                          111);
            return share<AST>(new AST());
        }
        lexer::Token op = tokens[m];
        if (op.type == tokentype) { return share<AST>(new GtAST(left, right, tokens)); }
    }
    return nullptr;
}*/

// GtAST
GtAST::GtAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::GREATER;
    this->op_view          = ">";
    this->const_folding_fn = {};
}

sptr<AST> GtAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying GtAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::GREATER, GtAST);
    return nullptr;
}

// LtAST
LtAST::LtAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::LESS;
    this->op_view          = "<";
    this->const_folding_fn = {};
}


sptr<AST> LtAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying LtAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::LESS, LtAST);
    return nullptr;
}

// GeqAST
GeqAST::GeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::GEQ;
    this->op_view          = ">=";
    this->const_folding_fn = {};
}

sptr<AST> GeqAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying GeqAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::GEQ, GeqAST);
    return nullptr;
}

// LeqAST
LeqAST::LeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens) {
    this->left             = left;
    this->right            = right;
    this->tokens           = tokens;
    this->op               = lexer::Token::LEQ;
    this->op_view          = "<=";
    this->const_folding_fn = {};
}

sptr<AST> LeqAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying LeqAST::parse");
    STANDARD_MATH_PARSE(lexer::Token::LEQ, LeqAST);
    return nullptr;
}

// NotAST

NotAST::NotAST(sptr<AST> inner, lexer::TokenStream tokens) {
    this->left             = inner;
    this->tokens           = tokens;
    this->op               = lexer::Token::NOT;
    this->op_view          = "!";
    this->const_folding_fn = {{"bool"_c, nlambda(String v) {return (v == "true" ? "false"s : "true"s);
}
}
}
;
}

#define UNARY_MATH_PARSE(tokentype, type1, after)                                                         \
    if (tokens.size() == 0) return nullptr;                                                               \
    if (tokens[0].type == tokentype) {                                                                    \
        DEBUG(4, "\e[1m"s + #type1 + "\e[0m");                                                            \
        sptr<AST> of = math::parse(tokens.slice(1, tokens.size()), local, sr);                            \
        if (of == nullptr) {                                                                              \
            parser::error(parser::errors["Expression expected"],                                          \
                          tokens.slice(1, 1),                                                             \
                          "Expected espression of type \e[1m"s + expected_type + "\e[0m after " + after); \
            return share<AST>(new AST);                                                                   \
        }                                                                                                 \
        return share<AST>(new type1(of, tokens));                                                         \
    }

sptr<AST> NotAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying NotAST::parse");
    UNARY_MATH_PARSE(lexer::Token::NOT, NotAST, '!');

    return nullptr;
}

// NegAST

NegAST::NegAST(sptr<AST> inner, lexer::TokenStream tokens) {
    this->left             = inner;
    this->tokens           = tokens;
    this->op               = lexer::Token::NEG;
    this->op_view          = "~";
    this->const_folding_fn = {{"uint8"s, nlambda(String v) {return std::to_string(~(uint8) (std::stoi(v)));
}
}
, {"uint16"s, nlambda(String v) {return std::to_string(~(uint16) (std::stoi(v)));
}
}
, {"uint32"s, nlambda(String v) {return std::to_string(~(uint32) (std::stol(v)));
}
}
, {"uint64"s, nlambda(String v) {return std::to_string(~(uint64) (std::stoll(v)));
}
}
,

    {"int8"s, nlambda(String v) {return std::to_string(~(int8) (std::stoi(v)));
}
}
, {"int16"s, nlambda(String v) {return std::to_string(~(int16) (std::stoi(v)));
}
}
, {"int32"s, nlambda(String v) {return std::to_string(~(int32) (std::stol(v)));
}
}
, {"int64"s, nlambda(String v) {return std::to_string(~(int64) (std::stoll(v)));
}
}
,
}
;
}

sptr<AST> NegAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying NegAST::parse");
    UNARY_MATH_PARSE(lexer::Token::NEG, NegAST, '~');

    return nullptr;
}

sptr<AST> math::parse_pt(PARSER_FN_PARAM) {
    if (tokens.size() < 2) { return nullptr; }
    if (tokens[0].type == lexer::Token::Type::OPEN) {
        if (tokens[-1].type == lexer::Token::Type::CLOSE) {
            DEBUG(2, "PT::parse");
            auto a    = math::parse(tokens.slice(1, tokens.size() - 1), local + 1, sr, expected_type);
            a->has_pt = true;
            a->setTokens(tokens);
            return a;
        }
    }

    return nullptr;
}

CastAST::CastAST(sptr<AST> from, sptr<AST> type, lexer::TokenStream tokens) {
    this->from   = from;
    this->type   = type;
    this->tokens = tokens;
}

CstType CastAST::provide() {
    parser::error(parser::errors["Expression unassignable"], tokens, "Cannot assign an expression result to a value");
    return "@unknown"_c;
}

sptr<AST> CastAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mCastAST::parse\e[0m");
    if (tokens.size() < 3) { return nullptr; }
    lexer::TokenStream::Match m = tokens.splitStack({lexer::Token::AS});
    if (m.found()) {
        DEBUG(2, "CastAST::parse");
        if ((uint64) m == tokens.size() - 1) {
            parser::error(parser::errors["Type expected"], {tokens[(uint64) m]}, "Expected a type after 'as'");
            return share<AST>(new AST);
        }
        if ((uint64) m == 0) {
            parser::error(parser::errors["Expression expected"], {tokens[0]}, "Expected a valid expression");
            return share<AST>(new AST());
        }
        sptr<AST> type = Type::parse(m.after(), local, sr);
        if (type == nullptr) {
            parser::error(parser::errors["Type expected"], m.after(), "Expected a type after 'as'");
            return share<AST>(new AST);
        }
        sptr<AST> expr = math::parse(m.before(), local, sr);
        if (expr == nullptr) {
            parser::error(parser::errors["Expression expected"], m.before(), "Expected a valid expression");
            return share<AST>(new AST());
        }
        return share<AST>(new CastAST(expr, type, tokens));
    }
    return nullptr;
}

String CastAST::emitCST() const {
    return from->emitCST() + " as " + type->emitCST();
}

void CastAST::consume(CstType t) {
    if (getCstType() != t) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      String("expected a \e[1m") + t + "\e[0m, got a variable cast returning " + getCstType());
    }
}

sptr<AST> CheckAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() == 0) { return nullptr; }
    if (tokens[-1].type == lexer::Token::QM) {
        DEBUG(2, "CheckAST::parse");
        sptr<AST> of = math::parse(tokens.slice(0, -1), local, sr, expected_type);
        if (of == nullptr) {
            if (tokens.size() == 1) {
                parser::error(parser::errors["Expression expected"], tokens, "expected an expression before '?'");
            } else {
                parser::error(parser::errors["Expression expected"],
                              tokens.slice(0, -1),
                              "expected a valid expression before '?'");
            }
            return nullptr;
        }
        return share<AST>(new CheckAST(of, tokens));
    }
    return nullptr;
}

void CheckAST::consume(CstType type) {
    if (!(type != CstType(of->getCstType() + '?'))) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      "expected a \e[1m"s + type + "\e[0m, got an optional check returning " + getCstType());
    }
}

CstType CheckAST::provide() {
    of->consume("@unknown?"_c);
    CstType t = of->getCstType();
    return t.substr(std::max<int64>(t.size()-1, 0));
}

sptr<AST> NoWrapAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mNoWrapAST::parse\e[0m");
    if (tokens.size() < 3) { return nullptr; }
    if (tokens[0].type == lexer::Token::NOWRAP) {
        DEBUGT(2, "NoWrapAST::parse", &tokens);
        if (tokens[1].type != lexer::Token::OPEN) {
            parser::error(parser::errors["Expected Block open"], {tokens[0]}, "Expected a '(' token after 'nowrap'");
            return share<AST>(new AST);
        }
        if (tokens[-1].type != lexer::Token::CLOSE) {
            parser::error(parser::errors["Expected Block close"],
                          {tokens[-1]},
                          "Expected a ')' token after '"s + tokens[-1].value + "'");
            return share<AST>(new AST);
        }
        sptr<AST> a = math::parse(tokens.slice(2, -1), local + 1, sr);
        if (a == nullptr) {
            parser::error(parser::errors["Expression expected"],
                          tokens.slice(2, -1),
                          "Expected a valid expression in nowrap block");
            return share<AST>(new AST);
        }
        if (! instanceOf(a, DoubleOperandAST)) { // currently, nowrap can only be used on operators. Unary operands (~ and !) never wrap and only double have to be checked
            return a;
        }
        return share<AST>(new NoWrapAST(a, tokens));
    }
    return nullptr;
}

void NoWrapAST::consume(CstType type) {
    of->consume(type);
}
CstType NoWrapAST::provide(){
    return of->provide();
}

sptr<AST> ArrayIndexAST::parse(PARSER_FN_PARAM){
    DEBUG(4, "Trying \e[1mArrayIndexAST::parse\e[0m");
    if (tokens.size() < 1) return nullptr;
    lexer::TokenStream::Match m = tokens.splitStack({lexer::Token::INDEX_OPEN});
    if (m.found() && (uint64)m != 0){
        if (tokens[-1].type == lexer::Token::INDEX_CLOSE){
            DEBUG(2, "ArrayIndexAST::parse");
            lexer::TokenStream tok2 = m.after().slice(0, -1);
            DEBUGT(3, "\ttok: ", &tok2);
            sptr<AST> idx = math::parse(tok2, local+1, sr);
            if (idx == nullptr){
                parser::error(parser::errors["Expression expected"], tok2, "expected a valid expression");
                return ERR;
            }
            idx->consume("usize"_c);

            lexer::TokenStream tok = m.before();
            DEBUGT(3, "\ttok: ", &tok);
            sptr<AST> of = math::parse(tok, local+1, sr);
            if (of == nullptr){
                parser::error(parser::errors["Expression expected"], tok, "expected a valid expression");
                return ERR;
            }
            return share<AST>(new ArrayIndexAST(tokens, of, idx));
        }
    }
    return nullptr;
}

void ArrayIndexAST::consume(CstType type){
    if (instanceOf(of, VarAccesAST)){
        if (cast2(of, VarAccesAST)->var == dynamic_cast<symbol::Variable*>(cast2(of, VarAccesAST)->var)){
            ((symbol::Variable*)(cast2(of, VarAccesAST)->var))->used = symbol::Variable::PROVIDED;
        }

        // TODO Require setting this after it

    }
    of->consume(type + "[]");
}
CstType ArrayIndexAST::provide(){
    of->consume("@unknown[]"_c);
    if (instanceOf(of, VarAccesAST)){
        if (cast2(of, VarAccesAST)->var == dynamic_cast<symbol::Variable*>(cast2(of, VarAccesAST)->var)){
            ((symbol::Variable*)(cast2(of, VarAccesAST)->var))->used = symbol::Variable::PROVIDED;
        }

        // TODO Require using this before it

    }
    CstType ret = of->getCstType();
    return ret.substr(std::max<int64>(ret.size()-2, 0));
}

sptr<AST> math::parse(lexer::TokenStream tokens, int local, symbol::Namespace* sr, String expected_type) {
    DEBUGT(2, "math::parse", &tokens);
    return parser::parseOneOf(tokens,
                              {parse_pt, IntLiteralAST::parse, FloatLiteralAST::parse, BoolLiteralAST::parse,
                               CharLiteralAST::parse, StringLiteralAST::parse, EmptyLiteralAST::parse, NullLiteralAST::parse, VarAccesAST::parse, VarSetAST::parse, ArrayIndexAST::parse, ArrayLiteralAST::parse,

                               NegAST::parse, LandAST::parse, LorAST::parse, EqAST::parse, NeqAST::parse, GeqAST::parse,LeqAST::parse, GtAST::parse, LtAST::parse, AddAST::parse, MulAST::parse, PowAST::parse, NotAST::parse, NegAST::parse,
                              AndAST::parse, OrAST::parse, XorAST::parse,

                               NoWrapAST::parse, CastAST::parse, CheckAST::parse,ArrayLengthAST::parse, FuncCallAST::parse},
                              local, sr, expected_type);
}
