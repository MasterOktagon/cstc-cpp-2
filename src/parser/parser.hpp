
#pragma once

//
// PARSER.hpp
//
// convienience functions for the parser
//

#include "../lexer/token.hpp"
#include "../snippets.h"
#include "ast/ast.hpp"
#include "symboltable.hpp"

#include <vector>

/**
 * @namespace implementing parser infrastructure
 */
namespace parser {

    /**
     * @brief split a vector of tokens until a certain token was found. uses index, block and paranthesisises as descent
     *
     * @param tokens tokens to parse
     * @param delimiter delimiters to search
     * @param local recursion level
     * @param <unused>
     *
     * @return index after which to split. returns size of tokens if nothing was found
     */
    extern uint64 splitStack(std::vector<lexer::Token>                 tokens,
                             std::initializer_list<lexer::Token::Type> delimiter,
                             int                                       local,
                             std::initializer_list<lexer::Token::Type> = {});

    /**
     * @brief split a vector of tokens until a certain token was found. uses index, block and paranthesisises as descent
     * start from the back.
     *
     * @param tokens tokens to parse
     * @param delimiter delimiters to search
     * @param local recursion level
     * @param <unused>
     *
     * @return index after which to split. returns size of tokens if nothing was found
     */
    extern uint64 rsplitStack(std::vector<lexer::Token>                 tokens,
                              std::initializer_list<lexer::Token::Type> delimiter,
                              int                                       local,
                              std::initializer_list<lexer::Token::Type> = {});

    /**
     * @brief parse one of these functions
     *
     * @param tokens tokens to parse
     * @param functions functions to try to parse. Will be parsed in this order, so be careful!
     * @param local recursion level
     * @param sr current symbol namesapce
     * @param <unused>
     *
     * @return An AST Node or nullptr if no match was found
     */
    extern sptr<AST> parseOneOf(lexer::TokenStream                tokens,
                                std::vector<PARSER_FN_NO_DEFAULT> functions,
                                int                               local,
                                symbol::Namespace*                sr,
                                String                            expected_type);

    /**
     * @brief get a (new) subvector from another vector
     */
    template <typename T>
    std::vector<T> subvector(std::vector<T> v, int start, int, int stop) {
        auto s   = v.begin() + start;
        auto end = v.begin() + stop;
        return std::vector<T>(s, end);
    }

    //extern bool   typeEq(CstType a, CstType b);
    extern CstType hasOp(CstType type1, CstType type2, lexer::Token::Type op);
    extern bool   isAtomic(CstType type);

    /**
     * @brief check if a text matches this case
     */
    extern bool is_snake_case(String text);
    extern bool isPascalCase(String text);
    extern bool IsCamelCase(String text);
    extern bool IS_UPPER_CASE(String text);

    /**
     * virtual @enum that shows what modifiers were found
     */
    enum Modifier {
        NONE    = 0,
        CONST   = 1,
        MUTABLE = 2,
        STATIC  = 4
    };

    /**
     * @brief error if any of these are not allowed.
     */
    extern void allowModifier(Modifier mod, Modifier in, lexer::TokenStream t);

    /**
     * @brief get one or more modifiers from a list of tokens and REMOVE THEM FROM THE VECTOR
     */
    extern Modifier getModifier(lexer::TokenStream* tokens);

    /**
     * @brief check if an identifier is allowed as a name
    */
    extern void checkName(String name, lexer::Token where);
} // namespace parser

