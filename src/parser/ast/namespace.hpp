#pragma once

//
// NAMESPACE.hpp
//
// layouts the namespace AST
//

#include "ast.hpp"
#include "../symboltable.hpp"
#include "flow.hpp"


class NamespaceAST : public AST {

    sptr<SubBlockAST> block = nullptr;
    symbol::Namespace* ns    = nullptr;

    protected:
    String _str() const;

    public:
    NamespaceAST(sptr<SubBlockAST> a, symbol::Namespace* ns){block = a; this->ns = ns;}
    virtual bool isConst() {return false;} // do constant folding or not
    virtual ~NamespaceAST(){};
    virtual String emitCST() const;    
    virtual CstType getCstType() const {return "void"_c;}
    
    virtual void consume(CstType){}

    /**
     * @brief parse a namespace declaration
     *
     * @return namespace AST or nullptr is not found
     */
    static sptr<AST> parse(PARSER_FN);
};





