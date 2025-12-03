#pragma once

#include "../lexer/token.hpp"
#include "../snippets.hpp"

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace parser {

    extern uint64 errc;      ///< amount of raised errors
    extern uint64 warnc;     ///< amount raised warnings
    extern bool   one_error; ///< whether the "one error mode" (compiler exit on first error) is enabled

    /// \brief structure representing a type of error
    ///
    struct ErrorType {
            uint32 code = 0;  ///< error code
            string name = ""; ///< error name
    };

    extern map<string, ErrorType> errors;   ///< all ErrorType's for errors are stored here
    extern map<string, ErrorType> warnings; ///< all ErrorType's for warnings are stored here

    /// \brief mute all error output until unmuted. used mainly for tests.
    ///
    extern void mute();

    /// \brief unmute all error output. used mainly for tests.
    ///
    extern void unmute();

    /// \brief format and output an error
    ///
    /// this function takes a TokenStream, so the error origin will be a continuos stream of tokens
    /// \param type ErrorType to be displayed
    /// \param tokens tokens that are causing this error
    /// \param msg error message
    /// \param appendix string to be displayed after this message
    extern void error(ErrorType type, lexer::TokenStream tokens, string msg, string appendix = "");

    /// \brief format and output an warning
    ///
    /// this function takes a TokenStream, so the warning origin will be a continuos stream of tokens
    /// \param type ErrorType to be displayed
    /// \param tokens tokens that are causing this warning
    /// \param msg warning message
    /// \param appendix string to be displayed after this message
    extern void warn(ErrorType type, lexer::TokenStream tokens, string msg, string appendix = "");

    /// \brief format and output a note
    ///
    /// to be used with a warning or error.
    /// this function takes a TokenStream, so the warning origin will be a continuos stream of tokens.
    /// \param tokens tokens that are showed
    /// \param msg note message
    /// \param appendix string to be displayed after this message
    extern void note(lexer::TokenStream tokens, string msg, string appendix = "");

    /// \brief format and output an error
    ///
    /// this function takes a vector, so the error origin will display all single tokens
    /// \param type ErrorType to be displayed
    /// \param tokens tokens that are causing this error
    /// \param msg error message
    /// \param appendix string to be displayed after this message
    extern void error(ErrorType type, vector<lexer::Token> tokens, string msg, string appendix = "");

    /// \brief format and output an warning
    ///
    /// this function takes a TokenStream, so the warning origin will display all single tokens
    /// \param type ErrorType to be displayed
    /// \param tokens tokens that are causing this warning
    /// \param msg warning message
    /// \param appendix string to be displayed after this message
    extern void warn(ErrorType type, vector<lexer::Token> tokens, string msg, string appendix = "");

    extern void note(vector<lexer::Token> tokens, string msg, string appendix = "");

    ///
    /// \class that represents a string that can be displayed in a help message using a linked list
    ///
    class HelpBuffer : public Repr {
            friend void help(HelpBuffer buf, string msg, string appendix);

            ///
            /// \enum internal content type
            ///
            enum ContentType {
                STRING,
                TOKEN,
                TOKEN_STREAM
            };

            ContentType type; ///< current stored type

            string             s;                               // one of these
            lexer::Token       t;                               // holds
            lexer::TokenStream st = lexer::TokenStream::none(); // the data

        protected:
            sptr<HelpBuffer> next = nullptr; ///< aka linked list

            /// \brief get a string representation
            ///
            string _str() const;

        public:
            
            /// \brief construct a HelpBuffer entry from a string
            ///
            HelpBuffer(string s);

            /// \brief construct a HelpBuffer entry from a const char*
            ///
            HelpBuffer(const char* s);

            /// \brief construct a HelpBuffer entry from a TokenStream
            ///
            HelpBuffer(lexer::TokenStream t);

            /// \brief construct a HelpBuffer entry from a Token
            ///
            HelpBuffer(lexer::Token);

            /// \brief append another HelpBuffer to this one
            ///
            HelpBuffer operator+(HelpBuffer other);

            /// \brief append another HelpBuffer to this one
            ///
            HelpBuffer operator+(string other);

            /// \brief append another HelpBuffer to this one
            ///
            HelpBuffer operator+(const char* other);

            /// \brief append another HelpBuffer to this one
            ///
            HelpBuffer operator+(lexer::Token other);

            /// \brief append another HelpBuffer to this one
            ///
            HelpBuffer operator+(lexer::TokenStream other);

            /// \brief destroy this HelpBuffer
            ///
            ~HelpBuffer();

            /// \brief append another HelpBuffer to this one
            ///
            void append(HelpBuffer buf);
    };

    /// \brief display a help message showing what can be done to improve yor code
    ///
    /// \param buf HelpBuffer that should be displayed. at least one token/-stream must be supplied
    /// \param msg warning message
    /// \param appendix string to be displayed after this message
    extern void help(HelpBuffer buf, string msg, string appendix = "");

} // namespace parser
