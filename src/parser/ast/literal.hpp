#pragma once

//
// LITERAL.hpp
//
// layouts literal parsing
//

#include "../symboltable.hpp"
#include "ast.hpp"
//#include "flow.hpp"
#include "../../snippets.hpp"

#include <vector>

extern AST* parse_neg(std::vector<lexer::Token>, int local, symbol::Namespace* sr, string expected_type = "@unknown");

class LiteralAST : public AST {
    public:
        virtual ~LiteralAST() {};
        virtual string getValue() const abstract;

        virtual uint64 nodeSize() const final { return 1; }
        virtual CstType provide() final;
};

class IntLiteralAST : public LiteralAST {
        int  bits    = 32;   //> Integer Bit size
        bool tsigned = true; //> whether this integer is signed

    protected:
        string _str() const { return "<Int: "_s + const_value.value() + " | " + to_string(bits) + ">"; }

    public:
        IntLiteralAST(int bits, string const_value, bool tsigned, lexer::TokenStream tokens);

        virtual ~IntLiteralAST() {}

        // fwd declarations @see @class AST

        CstType getCstType() const { return CstType((tsigned ? "int"s : "uint"s) + to_string(bits)); }

        string getValue() const { return const_value.value(); }

        bool sign() const { return tsigned; }

        virtual string emitCST() const;

        virtual void consume(CstType type);

        /**
         * @brief parse an int literal
         *
         * @return Int literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class BoolLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "<Bool: "s + const_value + ">"; }

    public:
        BoolLiteralAST(string const_value, lexer::TokenStream tokens);

        virtual ~BoolLiteralAST() {}

        // fwd declarations @see @class AST

        string getValue() const { return const_value; }

        virtual string emitCST() const;
        CstType getCstType() const { return "bool"_c; }

        virtual void consume(CstType type);

        /**
         * @brief parse a bool literal
         *
         * @return bool literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class FloatLiteralAST : public LiteralAST {
        int bits = 32; //> Float size (name)

    protected:
        string _str() const { return "<Float: "s + const_value + " | " + std::to_string(bits) + ">"; }

    public:
        FloatLiteralAST(int bits, string const_value, lexer::TokenStream tokens);

        virtual ~FloatLiteralAST() {}

        CstType getCstType() const { return CstType("float"s + std::to_string(bits)); }

        string getValue() const { return const_value; }

        virtual string emitCST() const;
        virtual void consume(CstType type);

        /**
         * @brief parse a float literal
         *
         * @return float literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class CharLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "<Char: '"s + const_value + "'>"; }

    public:
        CharLiteralAST(string const_value, lexer::TokenStream tokens);

        virtual ~CharLiteralAST() {}

        CstType getCstType() const { return "char"_c; }

        string         getValue() const;

        virtual string emitCST() const { return const_value; };
        virtual void consume(CstType type);

        /**
         * @brief parse a char literal
         *
         * @return char literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class StringLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "<string: \""s + const_value + "\">"; }

    public:
        StringLiteralAST(string const_value, lexer::TokenStream tokens);

        virtual ~StringLiteralAST() {}

        CstType getCstType() const { return "string"_c; }

        string getValue() const;

        virtual string emitCST() const { return const_value; };
        virtual void consume(CstType type);

        /**
         * @brief parse a string literal
         *
         * @return string literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class NullLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "<NULL>"; }

        CstType type = ""_c;

    public:
        NullLiteralAST(lexer::TokenStream tokens){this->tokens = tokens;}

        virtual ~NullLiteralAST() {}

        CstType getCstType() const { return type; }

        string getValue() const { return "null"; };

        virtual string emitCST() const { return "null"; };
        virtual void consume(CstType type);

        /**
         * @brief parse a null literal
         *
         * @return null literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class EmptyLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "< [] >"; }

        CstType type = "@unknown[]"_c;
        uint64 const_len = 0;

    public:
        EmptyLiteralAST(lexer::TokenStream tokens){this->tokens = tokens; is_const = true;}

        virtual ~EmptyLiteralAST() {}

        CstType getCstType() const { return type; }

        string getValue() const { return "[]"; };

        virtual string emitCST() const { return "[]"; };
        virtual void consume(CstType type);

        /**
         * @brief parse a null literal
         *
         * @return null literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};

class ArrayFieldMultiplierAST : public AST {
        protected:
        string _str() const { return "<" + str(content.get()) + " x " + str(amount.get()) + ">"; }

        CstType type = "@unknown"_c;
        sptr<AST> content;
        sptr<AST> amount;

        public:
        ArrayFieldMultiplierAST(lexer::TokenStream tokens, sptr<AST> content, sptr<AST> amount) {
            this->tokens = tokens;
            this->content = content;
            this->amount = amount;
        }

        virtual ~ArrayFieldMultiplierAST() {}

        CstType getCstType() const { return content->getCstType(); }

        string getValue() const { return ""; };

        virtual string emitCST() const { return content->emitCST() + " x " + amount->emitCST(); };
        virtual void consume(CstType type);
        virtual CstType provide();

        /**
         * @brief parse a null literal
         *
         * @return null literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};
class ArrayLiteralAST : public LiteralAST {
    protected:
        string _str() const { return "<Array ("s + type + ") [" + "] >"; }

        CstType type = "@unknown[]"_c;
        std::vector<sptr<AST>> contents = {};

    public:
        uint64 const_len = 0;
        ArrayLiteralAST(lexer::TokenStream tokens, std::vector<sptr<AST>> contents){this->tokens = tokens; this->contents=contents;}

        virtual ~ArrayLiteralAST() {}

        CstType getCstType() const { return type; }

        string getValue() const { return "[]"; };

        virtual string emitCST() const {
            string s = "[";
            bool   has_contents = false;
            for (sptr<AST> c : contents) {
                has_contents = true;
                s += c->emitCST() + ",";
            }
            if (has_contents) { s += "\b"; }
            return s + "]";
        };

        virtual void consume(CstType type);

        /**
         * @brief parse a null literal
         *
         * @return null literal AST or nullptr is not found
         */
        static sptr<AST> parse(PARSER_FN);
};