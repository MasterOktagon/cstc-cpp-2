#include "import.hpp"

#include "../../errors/errors.hpp"
#include "ast.hpp"

sptr<AST> ImportAST::parse(PARSER_FN_PARAM) {
    /*if (tokens.size() == 0) { return nullptr; }
    if (tokens[0].type == lexer::Token::IMPORT) {
        if (tokens[tokens.size() - 1].type == lexer::Token::END_CMD) {
            bool               errored      = false;
            vector<lexer::Token>     module_names = {};
            lexer::TokenStream contents     = tokens.slice(1, -1);

            for (lexer::TokenStream tok : contents.list({lexer::Token::SUBNS})) {
                if (tok.size() == 1) {
                    lexer::Token t = tok[0];
                    switch (t.type) {
                        case lexer::Token::MUL :   
                        case lexer::Token::SYMBOL :
                        case lexer::Token::DOTDOT :
                            module_names.push_back(t);
                            break;
                        default :
                            // ERROR
                    }

                } else if (tok.size() == 0) {
                    // ERROR
                } else {
                    // ERROR
                }
            }
            if (module_names.size() == 0) {
                // ERROR
                return ERR;
            }
            if (errored) { return ERR; }
        } else {
            parser::error(parser::errors["Expected ';'"],
                          {tokens[tokens.size() - 1]},
                          "Expected ';' after '"_s + tokens[tokens.size() - 1].value + "'");
            return ERR;
        }
    }*/
    return nullptr;
}
