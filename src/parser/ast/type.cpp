#include "type.hpp"

#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "../../debug/debug.hpp"

#include <memory>
#include <vector>

TypeAST::TypeAST(String name) {
    this->name = name;
}

sptr<AST> TypeAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() != 1) { return nullptr; }
    if (tokens[0].type == lexer::Token::Type::ID) { return share<AST>(new TypeAST(tokens[0].value)); }
    return nullptr;
}

sptr<AST> Type::parse(PARSER_FN_PARAM) {
    DEBUGT(2, "Type::parse", &tokens);
    return parser::parseOneOf(tokens, {OptionalTypeAST::parse, TypeAST::parse, ArrayTypeAST::parse}, local, sr, expected_type);
}

OptionalTypeAST::OptionalTypeAST(sptr<TypeAST> t) {
    this->type = t;
}

sptr<AST> OptionalTypeAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() < 2) { return nullptr; }
    if (tokens[tokens.size() - 1].type == lexer::Token::Type::QM) {
        sptr<AST> t = Type::parse(tokens.slice(0, tokens.size() - 1), local, sr);
        if (t == nullptr) { return nullptr; }
        if (! instanceOf(t, TypeAST)){
            return nullptr;
        }
        return share<AST>(new OptionalTypeAST(std::dynamic_pointer_cast<TypeAST>(t)));
    }
    return nullptr;
}

ArrayTypeAST::ArrayTypeAST(sptr<TypeAST> t){
    this->type = t;
}

sptr<AST> ArrayTypeAST::parse(PARSER_FN_PARAM){
    if (tokens.size() < 2) return nullptr;
    if (tokens[-1].type == lexer::Token::INDEX_CLOSE && tokens[-2].type == lexer::Token::INDEX_OPEN){
        if (tokens.size() == 2){
            parser::error(parser::errors["Array type not specified"], tokens, "The type of this array was not specified");
            return ERR;
        }
        sptr<AST> type = Type::parse(tokens.slice(0, -2), local, sr);
        if (type == nullptr){
            parser::error(parser::errors["Type expected"], tokens.slice(0, -2), "expected a valid type");
            return ERR;
        }
        return share<AST>(new ArrayTypeAST(cast2(type, TypeAST)));
    }
    return nullptr;
}