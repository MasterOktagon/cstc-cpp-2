#pragma once
#include "../snippets.hpp"
#include "token.hpp"

namespace lexer {

    extern int32 pretty_size; ///< max length before LTL warning

    /// \brief get a list of tokens from a string.
    /// Might issue warnings that defer further processing.
    ///
    /// \return Vector of Tokens tokenized.
    extern TokenStream tokenize(string text, string filename);
    extern TokenStream tokenize(string text);

    /// \brief try to fit a delimiting token into a single-char buffer
    ///
    /// \return token type found or Token::Type::NONE. \see lexer::Token::Type
    extern Token::Type getSingleToken(char c);

    /// \brief try to fit a delimiting token into a string
    ///
    /// \param s string has to have length 2!
    ///
    /// \return token type found or Token::Type::NONE. \see lexer::Token::Type
    extern Token::Type getDoubleToken(string s);

    /// \brief try to find the token type of a token that does not fit getSingleToken or getDoubleToken
    ///
    /// \param c string buffer
    ///
    /// \return token type found or Token::Type::NONE. \see lexer::Token::Type
    extern Token::Type matchType(string c);

} // namespace lexer