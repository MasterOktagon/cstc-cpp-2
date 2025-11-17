#pragma once

//
// AST.hpp
//
// layouts the AST Node class
//

#include "../../helpers/csttype.hpp"
#include "../../lexer/token.hpp"
#include "../../snippets.hpp"

#include <optional>

#define PARSER_FN                                                                   \
    lexer::TokenStream, int local, symbol::Namespace *sr,                           \
        string expected_type = "@unknown" ///< used to template all parser functions
#define PARSER_FN_PARAM      lexer::TokenStream tokens, int local, symbol::Namespace *sr, string expected_type
#define PARSER_FN_NO_DEFAULT callable<sptr<AST>, lexer::TokenStream, int, symbol::Namespace*, string>
#define ERR                  sptr<AST>(new AST)
#define PUT_PT(s, a)         (a ? "("_s + s + ")" : s)

///
/// \class represents an AST node
///
class AST : public Repr {
    protected:
        ///
        /// \brief debug represenstation
        ///
        virtual string     _str() const;
        lexer::TokenStream tokens =
            lexer::TokenStream({}); ///< Tokens of this AST Node. these are mostly used for error messages

    public:
        ///
        /// \note since this class is thought as a replacement for errorneous ASTs, there is only a default constructor
        ///
        AST() = default;
        optional<string> const_value;    ///< constant value is supplied
        bool             has_pt = false; ///< whether this AST has () around it

        virtual ~AST() = default;

        lexer::TokenStream getTokens() const { return tokens; }

        void setTokens(lexer::TokenStream tokens) { this->tokens = tokens; }

        /// 
        /// \brief get the return type (C*) of this Node, or @unknown
        /// 
        virtual CstType getCstType() const;

        ///
        /// \brief make sure the expected type does match with the provided. may cast errors
        ///
        /// \param t expected type
        ///
        virtual void consume(CstType t);

        ///
        /// \brief make sure the expected type does match with the provided. may cast errors
        ///
        /// \return type this expression consumes, or "void" when not applicable
        ///
        virtual CstType provide();

        /// 
        /// \brief emit C* code
        ///
        virtual string emitCST() const;

        /// 
        /// \brief get this Nodes work size (in Nodes) for progress reports
        ///
        virtual uint64 nodeSize() const { return 0; }

        ///
        /// \brief get if this expression is constant
        /// 
        bool isConst() const {return const_value.has_value();}
};

