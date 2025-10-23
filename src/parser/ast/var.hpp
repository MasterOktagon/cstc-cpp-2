#pragma once
#include "../symboltable.hpp"
#include "ast.hpp"
#include "base_math.hpp"

#include <vector>

sptr<AST>     parseStatement(PARSER_FN);
extern String parse_name(lexer::TokenStream);

class VarDeclAST : public AST {
        String            name = "";
        sptr<AST>         type = nullptr;
        symbol::Variable* v    = nullptr;

    protected:
        String _str() const;

    public:
        VarDeclAST(String name, sptr<AST> type, symbol::Variable* v);

        virtual bool isConst() const { return false; } // do constant folding or not

        virtual ~VarDeclAST() {};

        virtual uint64 nodeSize() const { return 1; } // how many nodes to to do

        virtual String emitCST() const {
            return (v->isMutable ? "mut "s : ""s) + type->getCstType() + " " + name + ";";
        }

        virtual CstType getCstType() const { return type->getCstType(); }

        static sptr<AST> parse(PARSER_FN);
};

class VarInitlAST : public AST {
        String            name        = "";
        sptr<AST>         type        = nullptr;
        sptr<AST>         expression  = nullptr;
        symbol::Variable* v           = nullptr;
        bool              as_optional = false;

    protected:
        String _str() const;

    public:
        VarInitlAST(String name, sptr<AST> type, sptr<AST> expr, symbol::Variable* v, lexer::TokenStream tokens);

        virtual bool isConst() { return false; } // do constant folding or not

        virtual ~VarInitlAST() {};

        virtual uint64 nodeSize() const { return expression->nodeSize() + 1; } // how many nodes to to do

        virtual String emitCST() const {
            return (v->isMutable ? "mut "s : ""s) + (v->isConst ? "const "s : ""s) + type->getCstType() + " " + name +
                   " = " + expression->emitCST() + ";";
        }

        virtual CstType getCstType() const { return type->getCstType(); }

        static sptr<AST> parse(PARSER_FN);
};

class VarAccesAST : public AST {
        String name = "";

    protected:
        String _str() const;

    public:
        symbol::Reference* var = nullptr;
        VarAccesAST(String name, symbol::Reference* sr, lexer::TokenStream tokens);

        virtual bool isConst() { return false; } // do constant folding or not

        virtual ~VarAccesAST() {}

        virtual uint64 nodeSize() const { return 1; } // how many nodes to to do

        virtual String emitCST() const { return name; }

        virtual CstType getCstType() const { return var->getCstType(); }

        virtual void consume(CstType type);

        virtual CstType provide();

        static sptr<AST> parse(PARSER_FN);
};

class VarSetAST : public ExpressionAST {
        sptr<AST> left        = nullptr;
        sptr<AST> right       = nullptr;
        bool      as_optional = false;

    protected:
        //String _str() const;

    public:
        VarSetAST(sptr<AST> before, sptr<AST> after, lexer::TokenStream tokens) {
            left = before;
            right = after;
            this->tokens = tokens;
        }

        virtual bool isConst() { return false; } // do constant folding or not

        virtual ~VarSetAST() = default;

        virtual uint64 nodeSize() const { return left->nodeSize() + right->nodeSize() + 1; } // how many nodes to to do

        virtual String emitCST() const { return PUT_PT(left->emitCST() +" = " + right->emitCST(), this->has_pt); }

        virtual CstType getCstType() const { return right->getCstType(); }

        virtual void consume(CstType type);

        static sptr<AST> parse(PARSER_FN);
};

class DeleteAST : public AST {
        std::vector<symbol::Variable*> vars = {};

    protected:
        String _str() const {
            String s = "<DELETE ";
            for (symbol::Variable* v : vars) { s += "<"s + v->getLoc() + ">"; }
            return s + ">";
        };

    public:
        DeleteAST(std::vector<symbol::Variable*> vars, lexer::TokenStream tokens) {
            this->vars   = vars;
            this->tokens = tokens;
        }

        virtual ~DeleteAST() {};

        virtual uint64 nodeSize() const { return vars.size(); } // how many nodes to to do

        virtual String emitCST() const {
            String s = "delete ";
            for (symbol::Variable* var : vars) { s += var->getRawLoc() + ","; }
            s += "\b;";
            return s;
        }

        virtual CstType getCstType() const { return "void"_c; }

        static sptr<AST> parse(PARSER_FN);
};
