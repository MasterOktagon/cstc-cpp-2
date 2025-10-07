#pragma once

#include "../snippets.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace std;

/// The lexer namespace holds all functionality
/// related to tokenizing text into tokens.
namespace lexer {

    ///
    /// \brief Represents a single Token.
    ///
    class Token : public Repr {
        protected:
            /// \brief create a string representation
            string _str() const;

        public:
            /// \brief represents all different types a Token can be
            enum Type {
                // clang-format off
            
                    //  SPECIAL   //
                NONE     ,        ///< undefined token
                SYMBOL   ,        ///< a symbol
                EF       ,        ///< File end
                END_CMD  ,        ///< `;`

                    //  LITERALS  //
                INT      ,        ///< int literal
                HEX      ,        ///< int literal in hexadecimal
                BINARY   ,        ///< int literal in binary
                BOOL     ,        ///< bool literal
                STRING   ,        ///< string literal
                CHAR     ,        ///< char literal
                FLOAT    ,        ///< float literal
                NULV     ,        ///< `null`

                    //    MATH    //
                SET      ,        ///< `=`
                ADD      ,        ///< `+`
                SUB      ,        ///< `-`
                MUL      ,        ///< `*`
                DIV      ,        ///< `/`
                MOD      ,        ///< `%`
                POW      ,        ///< `**`
                INC      ,        ///< `++`
                DEC      ,        ///< `--`
                //NEC      ,        ///< `- <int>`

                NEG      ,        ///< `~`
                AND      ,        ///< `&`
                OR       ,        ///< `|`
                XOR      ,        ///< `^`
                SHL      ,        ///< `<<`
                SHR      ,        ///< `>>`
                LSHR     ,        ///< `>>>`

                    //  LOGICAL   //
                LAND     ,        ///< `and`
                LOR      ,        ///< `or`
                NOT      ,        ///< `not`

                    // COMPARISON //
                EQ       ,        ///< `==`
                NEQ      ,        ///< `!=`
                LT       ,        ///< `<`
                GT       ,        ///< `>`
                GEQ      ,        ///< `>=`
                LEQ      ,        ///< `<=`

                    //    FLOW    //
                QM       ,        ///< `?`
                IN       ,        ///< `:`
                UNPACK   ,        ///< `<-`
                REF      ,        ///< `#`
                RMREF    ,        ///< `#!`
                RMREFT   ,        ///< `&!` used to denote a RM-ref as a type
                
                SUBNS    ,        ///< `::`
                ACCESS   ,        ///< `.`
                COMMA    ,        ///< `,`
                DOTDOT   ,        ///< `..` Only used in 'import' for usage of "up one directory"
                DOTDOTDOT,        ///< `...` Triple dots are used for variadic functions and to denote a todo Block

                    // PARANTHESIS //
                OPEN     ,        ///< (
                CLOSE    ,        ///< )
                BLOCK_OPEN ,      ///< {
                BLOCK_CLOSE,      ///< }
                INDEX_OPEN ,      ///< [
                INDEX_CLOSE,      ///< ]

                    //  KEYWORDS  //
                IF        ,         
                ELSE      ,         
                FOR       ,   
                WHILE     ,        
                LOOP      ,    
                THROW     ,      
                BREAK     ,
                CONTINUE  ,
                NOIMPL    ,
                RETURN    ,
                AS        ,
                OPERATOR  ,
                SWITCH    ,
                CASE      ,
                FINALLY   ,
                NEW       ,
                DELETE    ,

                IMPORT    ,
                REQ       ,
                QER       ,
                MACRO     ,
                INCLUDE   ,

                CLASS     ,
                ENUM      ,
                STRUCT    ,
                VIRTUAL   ,
                ABSTRACT  ,
                FINAL     ,
                NAMESPACE ,
                FRIEND    ,
                PROTECTED ,
                PRIVATE   ,
                STATIC    ,
                PUBLIC    ,
                MUT       ,
                CONST     ,
                NOWRAP    ,
                X
                // clang-format on
            };

            Type   type;  ///< this tokens type
            string value; ///< this tokens contents

            uint32 line   = 0; ///< the line of this token in its file
            uint32 column = 0; ///< the position of this token in this line

            sptr<Token> expanded = nullptr; ///< \brief macro expanded from.
                                            ///<
                                            ///< if this token was expanded from a macro,
                                            ///< its token is referenced here for error detection

            sptr<string> filename;          ///< filename this token comes from
            sptr<string> line_contents;     ///< this tokens line's contents (for error messages)
            sptr<string> include = nullptr; ///< included file this comes from

            Token() = default;

            /// \brief construct a token
            Token(Type         type,
                  string       value,
                  uint32       line,
                  uint32       column,
                  sptr<string> filename,
                  sptr<string> line_contents,
                  sptr<Token>  expanded = nullptr,
                  sptr<string> include  = nullptr);

            virtual ~Token() = default;

            /// \brief compare two Tokens
            /// \return true if tokens type and position is the same
            bool operator==(Token other);

            /// \brief compare two Tokens
            /// \return false if tokens type and position is the same
            bool operator!=(Token other);
    };
} // namespace lexer

const lexer::Token nullToken = lexer::Token(lexer::Token::NONE,
                                            "",
                                            0,
                                            0,
                                            sptr<string>(new string),
                                            nullptr); ///< Token constant representing an empty token

namespace lexer {

    /// \brief represents a stream (array/list/...) of tokens.
    ///
    /// Because vectors and arrays cannot be searched or sliced easyly (something the compiler needs),
    /// TokenStream does this by manipulating start and stop variables. a TokenStream window spans from start to stop.
    class TokenStream final : public Repr {
        public:
            uint64              start = 0; ///< virtual start index
            uint64              stop  = 0; ///< virtual stop index
            sptr<vector<Token>> tokens;    ///< actual data

        protected:
            /// \brief create a string representation
            string _str() const;

        public:
            /// \brief represents a TokenStream search result
            ///
            /// Since we often need to search a token and then slice at its position, this class handles this.
            class Match final {
                    uint64             at        = 0;       ///< where this result was found (relative index)
                    bool               was_found = false;   ///< whether something was found
                    const TokenStream* on        = nullptr; ///< on what stream it was found

                public:
                    /// \brief construct a new Match
                    ///
                    Match(uint64 at, bool found, const TokenStream* on);

                    /// \brief destroy this match
                    ///
                    ~Match() = default;

                    /// \brief get whether this search was succesful
                    ///
                    /// \return true if search was succesful
                    bool found() const { return was_found; }

                    /// \brief get all tokens before this match
                    ///
                    /// \return TokenStream with all tokens before this match, or empty tokenstream if failed
                    TokenStream before() const { return found() ? on->slice(0, at) : TokenStream({}); }

                    /// \brief get all tokens after this match
                    ///
                    /// \return TokenStream with all tokens after this match, or empty tokenstream if failed
                    TokenStream after() const { return found() ? on->slice(at + 1, on->size()) : TokenStream({}); }

                    /// \return position of this match (0 if not succeded)
                    ///
                    operator int64() { return at; }
            };

            /// \brief construct a new TokenStream
            ///
            /// \param tokens actual token data. Is shared between many TokenStreams to save space
            /// \param start virtual start index on vactor
            /// \param stop if stop is 0xFFFF'FFFF'FFFF'FFFF (uint64 max value), stop will be chosen as the last element
            /// of tokens
            TokenStream(sptr<vector<Token>> tokens, uint64 start = 0, uint64 stop = 0xFFFF'FFFF'FFFF'FFFF);

            /// \brief get a substream of this stream
            ///
            /// slicing (extracting a substream) is a core requirement of this compiler.
            /// We do this by modifying the start and stop values of a newly created TokenStream object
            ///
            /// \param start start index offset from this start. Negative values will start from the Back
            /// \param stop  stop index offset from this <u>start</u>. Negative values will start from the Back
            TokenStream slice(int64 start, int64 stop) const;

            /// \brief get a Token at specific position
            ///
            /// May raise a std::range_error
            ///
            /// \param idx relative index from start index. Negative values will start from the Back
            /// \return nullToken when no token vector was set, otherwise try to return Token.
            Token get(int64 idx) const;

            /// \brief overloads to get(int64)
            ///
            Token operator[](int64 idx) const { return get(idx); }

            /// \brief get the virtual size of this stream
            ///
            uint64 size() const noexcept { return stop - start; }

            /// \brief check if this stream is empty
            ///
            bool empty() const { return size() == 0; }

            /// \brief 
            Match splitStack(std::initializer_list<lexer::Token::Type>, uint64 start_idx = 0) const;
            Match rsplitStack(std::initializer_list<lexer::Token::Type>, uint64 start_idx = 0) const;

            /*
            Match split(std::initializer_list<lexer::Token::Type>, uint64 start_idx = 0) const;
            Match rsplit(std::initializer_list<lexer::Token::Type>, uint64 start_idx = 0) const;

            std::vector<lexer::TokenStream> list(std::initializer_list<lexer::Token::Type>, bool allow_empty=false)
            const;

             Match operator[](lexer::Token::Type type) { return split({type}); }

            */
    };
} // namespace lexer
