
//
// STRUCT.cpp
//
// implements the struct AST class
//

#include "../../snippets.h"
#include "ast.hpp"
#include <cerrno>
#include <iostream>
#include "struct.hpp"
#include "../parser.hpp"
#include "../errors.hpp"
#include "var.hpp"

/*StructAST::StructAST(String name, symbol::Struct* st, std::vector<lexer::Token> tokens){
    this->tokens = tokens;
    this->st = st;
    this->name = name;
}

AST* StructAST::parse(std::vector<lexer::Token> tokens, int local, symbol::Namespace *sr, String expected_type){
    if (tokens.size() < 4) return nullptr;
    if (tokens.at(0).type == lexer::Token::Type::STRUCT){
        
    }

    return nullptr;
}*/

/*
sptr<AST> EnumAST::parse(PARSER_FN_PARAM){
    if (tokens.size() == 0) return nullptr;
    if (tokens[0].type == lexer::Token::Type::ENUM){
        if (tokens.size() == 1){
            parser::error("Identifier expected", {tokens[0]}, "Expected an identifier after 'enum'", 50);
            return share<AST>(new AST);
        }
        if (tokens[1].type == lexer::Token::ID){
            String name = tokens[1].value;
            if (tokens.size() == 2){
                parser::error("Expected Block Open", {tokens[1]}, "Expected a Block open after this token", 51);
                return share<AST>(new AST);
            }
            if (tokens[2].type != lexer::Token::Type::BLOCK_OPEN){
                parser::error("Expected Block Open", {tokens[2]}, "Expected a Block open", 51);
                return share<AST>(new AST);
            }
            if (tokens[tokens.size()-1].type != lexer::Token::Type::BLOCK_CLOSE){
                parser::error("Expected Block Close", {tokens[2]}, "Expected a Block close at the End of this statement", 52);
                return share<AST>(new AST);
            }

            // TODO

            if (!parser::IsCamelCase(name)){
                parser::warn("Wrong casing", {tokens[1]}, "Enum name should be CamelCase.", 16);
            }

            sr->add(name, new symbol::Enum(name));
            return share<AST>(new EnumAST(name, tokens));

        }
        else {
            parser::error("Identifier expected", {tokens[1]}, "Expected an identifier after 'enum'", 50);
            return share<AST>(new AST);
        }

    }
    return nullptr;
}
*/