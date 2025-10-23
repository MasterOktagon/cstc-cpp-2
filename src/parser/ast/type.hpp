#pragma once
#include "ast.hpp"
#include <vector>
#include "../../lexer/lexer.hpp"
#include "../symboltable.hpp"
#include "../parser.hpp"
#include <string>
#include "var.hpp"

/**
 * @namespace that holds type related functions
 */
namespace Type {

    /**
     * @brief function that parses a type
     *
     * @return Type AST or nullptr if none was found
     */
    extern sptr<AST> parse(PARSER_FN);
}

class TypeAST : public AST {

    String name = "";

    public:
    TypeAST(String name);
    TypeAST() = default;
    virtual bool isConst(){return false;} // do constant folding or not
    virtual ~TypeAST(){}
    virtual String emitCST() const {return name;}
    
    virtual CstType getCstType() const {return name;}
    virtual void consume(CstType){}

    /**
     * @brief parse a simple type name
     *
     * @return Type AST or nullptr if none was found
     */
    static sptr<AST> parse(PARSER_FN);
};

class OptionalTypeAST : public TypeAST {
    sptr<TypeAST> type;

    public:
    OptionalTypeAST(sptr<TypeAST> type);
    virtual bool isConst(){return false;} // do constant folding or not
    virtual ~OptionalTypeAST(){}
    virtual String emitLL(int*, String) const {return "";}
    virtual String emitCST() const {return type->emitCST() + "?";}
    
    virtual CstType getCstType() const {return type->getCstType() + "?";}
    virtual void consume(CstType){}

    /**
     * @brief parse a simple type name
     *
     * @return Type AST or nullptr if none was found
     */
    static sptr<AST> parse(PARSER_FN);
};

class ArrayTypeAST : public TypeAST {
    sptr<TypeAST> type;

    public:
    ArrayTypeAST(sptr<TypeAST> type);
    virtual bool isConst(){return false;} // do constant folding or not
    virtual ~ArrayTypeAST(){}
    virtual String emitCST() const {return type->emitCST() + "[]";}
    
    virtual CstType getCstType() const {return type->getCstType() + "[]";}
    virtual void consume(CstType){}

    /**
     * @brief parse a simple type name
     *
     * @return Type AST or nullptr if none was found
     */
    static sptr<AST> parse(PARSER_FN);
};



