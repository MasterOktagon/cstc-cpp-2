#pragma once

//
// BASE_MATH.hpp
//
// layouts base math ASTs
//

#include "../../lexer/token.hpp"
#include "../../snippets.h"
#include "../errors.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"

#include <map>
#include <tuple>
#include <vector>

/**
 * @namespace that includes the "default" expression parsing functions
 */
namespace math {

    /**
     * @brief parse an expression (e.g. variable call, function call, literal, math, ...)
     *
     * @return AST or nullptr if nothing matched
     */
    extern sptr<AST> parse(PARSER_FN);

    /**
     * @brief parse paranthesis (with an expression inside)
     *
     * @return AST or nullptr if nothing matched
     */
    extern sptr<AST> parse_pt(PARSER_FN);
} // namespace math

/**
 * @class for generic expression ASTs
 */
class ExpressionAST : public AST {
    public:
        ExpressionAST() {};
        virtual ~ExpressionAST() {};
};

class DoubleOperandAST : public ExpressionAST {
    protected:
        sptr<AST>                                                               left;  //> left operand
        sptr<AST>                                                               right; //> right operand
        lexer::Token::Type                                                      op               = lexer::Token::NONE;
        String                                                                  op_view          = "";
        std::map<std::tuple<CstType, CstType>, fsignal<String, String, String>> const_folding_fn = {};

        String _str() const final;

    public:
        DoubleOperandAST() {};
        virtual ~DoubleOperandAST() {};

        CstType getCstType() const final;
        void    consume(CstType) final;
        CstType provide() final;
        String  emitCST() const final;

        uint64 nodeSize() const final;
};

class UnaryOperandAST : public ExpressionAST {
    protected:
        sptr<AST>                                  left; //> left operand
        lexer::Token::Type                         op               = lexer::Token::NONE;
        String                                     op_view          = "";
        std::map<CstType, fsignal<String, String>> const_folding_fn = {};

        String _str() const final;

    public:
        UnaryOperandAST() {};
        virtual ~UnaryOperandAST() {};

        // fwd declarations. @see @class AST

        CstType getCstType() const final;
        void    consume(CstType) final;
        CstType provide() final;
        String  emitCST() const final;

        uint64 nodeSize() const final;
};

/**
 * @class that represents a '+' operation
 */
class AddAST : public DoubleOperandAST {
    public:
        AddAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~AddAST();

        // fwd declarations. @see @class AST

        /**
         * @brief parse an addition or subtraction (due to both having the same precedences)
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '-' operation
 */
class SubAST : public DoubleOperandAST {
    public:
        SubAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~SubAST();

        // fwd declarations. @see @class AST

};

/**
 * @class that represents a '*' operation
 */
class MulAST : public DoubleOperandAST {
    public:
        MulAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~MulAST();

        // fwd declarations. @see @class AST

        /**
         * @brief parse a multiplication, division or remainder (modulo) (due to them having the same precedences)
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '/' operation
 */
class DivAST : public DoubleOperandAST {
    public:
        DivAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~DivAST();

        // fwd declarations. @see @class AST
};

/**
 * @class that represents a '%' operation
 */
class ModAST : public DoubleOperandAST {
    public:
        ModAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~ModAST();

        // fwd declarations. @see @class AST
};

/**
 * @class that represents a '**' operation
 */
class PowAST : public DoubleOperandAST {
    public:
        PowAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~PowAST();

        // fwd declarations. @see @class AST

        /**
         * @brief parse a power
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '||' operation
 */
class LorAST : public DoubleOperandAST {
    public:
        LorAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~LorAST();

        // fwd declarations. @see @class AST

        /**
         * @brief parse a logical and
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '&&' operation
 */
class LandAST : public DoubleOperandAST {
    public:
        LandAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~LandAST();

        // fwd declarations. @see @class AST

        /**
         * @brief parse a multiplication, division or remainder (modulo) (due to them having the same precedences)
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '|' operation
 */
class OrAST : public DoubleOperandAST {
    public:
        OrAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~OrAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a bitwise or
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '&' operation
 */
class AndAST : public DoubleOperandAST {
    public:
        AndAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~AndAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a bitwise and
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '^' operation
 */
class XorAST : public DoubleOperandAST {
    public:
        XorAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~XorAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '==' operation
 */
class EqAST : public DoubleOperandAST {
    public:
        EqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~EqAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '!=' operation
 */
class NeqAST : public DoubleOperandAST {
    public:
        NeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~NeqAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '>=' operation
 */
class GeqAST : public DoubleOperandAST {
    public:
        GeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~GeqAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '<=' operation
 */
class LeqAST : public DoubleOperandAST {
    public:
        LeqAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~LeqAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '>' operation
 */
class GtAST : public DoubleOperandAST {
    public:
        GtAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~GtAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '<' operation
 */
class LtAST : public DoubleOperandAST {
    public:
        LtAST(sptr<AST> left, sptr<AST> right, lexer::TokenStream tokens);
        virtual ~LtAST() = default;

        // fwd declarations. @see @class AST

        /**
         * @brief parse a xor
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '!' operation
 */
class NotAST : public UnaryOperandAST {
    public:
        NotAST(sptr<AST> inner, lexer::TokenStream tokens);
        virtual ~NotAST() = default;

        /**
         * @brief parse a not
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents a '~' operation
 */
class NegAST : public UnaryOperandAST {
    public:
        NegAST(sptr<AST> inner, lexer::TokenStream tokens);
        virtual ~NegAST() = default;

        /**
         * @brief parse a bitwise negation
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents an 'as' cast operation
 */
class CastAST : public ExpressionAST {
        sptr<AST> from;
        sptr<AST> type;

    public:
        CastAST(sptr<AST> from, sptr<AST> type, lexer::TokenStream tokens);
        virtual ~CastAST() {};

        // fwd declarations. @see @class AST

        CstType getCstType() const { return type->getCstType(); }

        virtual uint64 nodeSize() const { return 1; }

        virtual String emitCST() const;

        virtual void consume(CstType type);
        CstType provide();

        /**
         * @brief parse a cast
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

/**
 * @class that represents an check-and-unwrap operation
 */
class CheckAST : public ExpressionAST {
        sptr<AST> of;

    protected:
        String _str() const { return "<CHECK "s + str(of.get()) + ">"; }

    public:
        CheckAST(sptr<AST> of, lexer::TokenStream tokens) {
            this->of     = of;
            this->tokens = tokens;
        }

        virtual ~CheckAST() {};

        // fwd declarations. @see @class AST

        CstType getCstType() const {
            if (of->getCstType() == ""_c) { return ""_c; }
            return of->getCstType().substr(0, of->getCstType().size() - 1);
        }

        virtual uint64 nodeSize() const { return 1; } // how many nodes to to do

        String emitCST() const { return of->emitCST() + "?"; }

        void consume(CstType type);
        CstType provide();

        /**
         * @brief parse a cast
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};

class NoWrapAST : public ExpressionAST {
        sptr<AST> of;

    protected:
        String _str() const { return "<NOWRAP "s + str(of.get()) + ">"; }

    public:
        NoWrapAST(sptr<AST> of, lexer::TokenStream tokens) {
            this->of     = of;
            this->tokens = tokens;
        }

        virtual ~NoWrapAST() {};

        // fwd declarations. @see @class AST

        CstType getCstType() const { return of->getCstType(); }

        virtual uint64 nodeSize() const { return of->nodeSize() + 1; } // how many nodes to to do

        String emitCST() const { return "nowrap("s + of->emitCST() + ")"; }

        void consume(CstType type);
        CstType provide();

        /**
         * @brief parse a nowrap call
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};


/**
 * @class
 */
class ArrayIndexAST : public ExpressionAST {
        sptr<AST> of;
        sptr<AST> idx;

    public:
        ArrayIndexAST(lexer::TokenStream tokens, sptr<AST> of, sptr<AST> idx) {
            this->of     = of;
            this->idx    = idx;
            this->tokens = tokens;
        }

        virtual ~ArrayIndexAST(){};
        virtual uint64 nodeSize() const { return of->nodeSize() + idx->nodeSize() + 1; } // how many nodes to to do

        String emitCST() const {
            return of->emitCST() + "[" + idx->emitCST() + "]";
        }

        CstType getCstType() const { return of->getCstType().substr(of->getCstType().size()-2); }

        void consume(CstType type);
        CstType provide();

        /**
         * @brief parse a nowrap call
         *
         * @return AST or nullptr if no match
         */
        static sptr<AST> parse(PARSER_FN);
};
