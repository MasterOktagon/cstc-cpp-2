#pragma once

//
// FLOW.hpp
//
// layouts the flow ASTs
//

#include "../symboltable.hpp"
#include "ast.hpp"

#include <vector>

class SubBlockAST : public AST {
    protected:
        String _str() const;

    public:
        std::vector<sptr<AST>> contents = {}; //> block comments
        symbol::Namespace*     parent   = nullptr;
        bool                   has_returned = false;

        SubBlockAST(bool has_returned) {
            this->has_returned = has_returned;
        }

        virtual ~SubBlockAST() {}

        // fwd declarations @see @class AST

        virtual bool isConst() { return false; }

        virtual String emitCST() const;

        virtual CstType getCstType() const { return "void"_c; }

        virtual uint64 nodeSize() const { return contents.size(); };

        virtual void consume(CstType) {}

        /**
         * @brief parse a Subblock
         */
        static sptr<AST> parse(PARSER_FN);
};

class IfAST : public AST {
    public:
        sptr<SubBlockAST> block;
        sptr<AST>         cond;
        symbol::Namespace* sb;

        IfAST(sptr<SubBlockAST> block, sptr<AST> cond, lexer::TokenStream t, symbol::Namespace* sb) {
            tokens      = t;
            this->block = block;
            this->cond  = cond;
            this->sb    = sb;
        }

        virtual ~IfAST() {delete sb;}

        // fwd declarations @see @class AST

        virtual bool isConst() { return false; }

        // virtual String emitLL(int* locc, String inp) const;

        virtual String emitCST() const {
            return "if "s + cond->emitCST() + " {\n" + intab(block->emitCST()) + "\n}\n";
        };

        virtual CstType getCstType() const { return "void"_c; }

        virtual uint64 nodeSize() const { return block->contents.size() + 1; };

        virtual void consume(CstType) {}

        /**
         * @brief parse a Subblock
         */
        static sptr<AST> parse(PARSER_FN);
};

class ReturnAST : public AST {
    public:
        sptr<AST> expr;
        CstType   type = ""_c;

        ReturnAST(sptr<AST> expr, lexer::TokenStream tokens) {
            this->expr   = expr;
            this->tokens = tokens;
        }

        virtual ~ReturnAST() {}

        virtual bool isConst() { return false; }

        virtual String emitCST() const { return "return"s + (expr != nullptr ? " "s + expr->emitCST() : ""s) + ";"; };

        virtual CstType getCstType() const { return "void"_c; }

        virtual uint64 nodeSize() const { return expr->nodeSize() + 1; };

        virtual void consume(CstType) {}

        /**
         * @brief parse a Subblock
         */
        static sptr<AST> parse(PARSER_FN);
};