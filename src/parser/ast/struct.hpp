
#pragma once

//
// STRUCT.hpp
//
// layouts the struct AST class
//

#include "../../snippets.h"
#include "../../lexer/token.hpp"
#include "ast.hpp"
#include <vector>
#include "../symboltable.hpp"

#if 0
class StructAST : public AST {

    String name;
    symbol::Struct* st;

    protected:
    String _str(){
        return "<Struct "s + name + ">";
    }

    public:
    StructAST(String name, symbol::Struct* st, std::vector<lexer::Token> tokens={});
    virtual ~StructAST(){}
    String getCstType(){return "void"; /* struct definiton is void*/}
    String getLLType(){return "";}
    String getValue(){return "";}
    virtual uint64 nodeSize(){return 1;} // how many nodes to to do
    virtual String emit_ll(int*, String);
    /*
        Emit llvm IR code in human-readable form

        [param locc] local variable name counter
    */

    //virtual llvm::Value* codegen();
    /*
        Emit llvm-bitcode to be compiled later
    */

    String emit_cst();
    /*
        Emit C* code
    */

    virtual void forceType(String type);

    static AST* parse(std::vector<lexer::Token> tokens, int local, symbol::Namespace* sr, String expected_type);
};
#endif

// TODO/WIP
/*
class EnumAST : public AST {
    String name;       //> enum name
    symbol::Enum* enu; //> enum symbol entry

    protected:
    String _str() const{
        return "<Enum "s + name + ">";
    }

    public:
    EnumAST(String name, lexer::TokenStream tokens){
        this->name = name;
        this->tokens = tokens;
    }
    virtual ~EnumAST(){};
    CstType getCstType() const {return "void"_c;}
    String getValue(){return "";}
    virtual uint64 nodeSize() const {return enu->contents.size() + 1;}
    String emit_cst() const {return "";};

    virtual void forceType(String type){};

    static sptr<AST> parse(PARSER_FN);
};
*/
