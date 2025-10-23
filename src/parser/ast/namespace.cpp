#include "namespace.hpp"

#include "../../lexer/token.hpp"
#include "../errors.hpp"
#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "base_math.hpp"
#include "flow.hpp"
#include "../../debug/debug.hpp"

#include <memory>
#include <vector>

String NamespaceAST::emitCST() const {
    String ret = "";
    for (sptr<AST> a : block->contents) {
        if (a != nullptr) {
            ret += a->emitCST();
            if ( instanceOf(a, ExpressionAST)){
                ret += ";"; // Expressions don't End on semicolons, therefore add one
            }
            ret += "\n";
        }
    }
    return "namespace "s + ns->getRawLoc() + " {\n" + intab(ret) + "\n}";
}

String NamespaceAST::_str() const {
    String ret = "";
    for (sptr<AST> a : block->contents) {
        if (a != nullptr){
            ret += str(a.get());
            ret += "\n";
        }
    }
    return "<namespace "s + ns->getRawLoc() + " \n" + intab(ret) + "\n>";
}

sptr<AST> NamespaceAST::parse(PARSER_FN_PARAM){
    DEBUG(5, "Trying NamespaceAST::parse");
    if (tokens.size() < 2) return nullptr;
    if (tokens[0].type == lexer::Token::Type::NAMESPACE){
        if (tokens[1].type == lexer::Token::Type::ID){
            DEBUG(3, "\e[1mNamespaceAST::parse\e[0m");
            if (!sr->ALLOWS_SUBCLASSES){
                parser::error(parser::errors["Namespace not allowed"], tokens, "A Block of type Namespace was not allowed in a Block of type "s + sr->getName());
                return share<AST>(new AST);
            }
            if ((*sr)[tokens[1].value].size() > 0){
                parser::error(parser::errors["Symbol already defined"], tokens, "A symbol with the name of "s + tokens[1].value + "was already used");
                parser::note((*sr)[tokens[1].value][0]->tokens, "defined here:");
                return share<AST>(new AST);
            }
            if (tokens.size() < 4 || tokens[2].type != lexer::Token::Type::BLOCK_OPEN){
                parser::error(parser::errors["Expected Block open"], {tokens[1]}, "A '{' was expected after this token");
                return share<AST>(new AST);
            }
            if (tokens[tokens.size()-1].type != lexer::Token::Type::BLOCK_CLOSE){
                parser::error(parser::errors["Expected Block close"], {tokens[tokens.size()-1]}, "A '}' was expected");
                return share<AST>(new AST);
            }

            symbol::Namespace* ns = new symbol::Namespace(tokens[1].value);
            ns->parent = sr;
            sr->add(tokens[1].value, ns);
            sptr<AST> a = parser::parseOneOf(tokens.slice(3,tokens.size()-1), {SubBlockAST::parse}, local+1, ns, "void");

            if (a == nullptr) return share<AST>(new AST);

            return sptr<AST>(new NamespaceAST(std::dynamic_pointer_cast<SubBlockAST>(a), ns));
        }
        else {
            parser::error(parser::errors["Name expected"], {tokens[0]}, "Expected a name after 'namespace'");
        }
    }
    return nullptr;
}

