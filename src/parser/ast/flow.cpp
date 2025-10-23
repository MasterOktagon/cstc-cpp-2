#include "flow.hpp"

#include "../../debug/debug.hpp"
#include "../errors.hpp"
#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "base_math.hpp"
#include "func.hpp"
#include "import.hpp"
#include "namespace.hpp"
//#include "struct.hpp"
#include "var.hpp"

#include <memory>
#include <vector>

String SubBlockAST::emitCST() const {
    String ret = "";
    for (sptr<AST> a : contents) {
        ret += a->emitCST();
        if ( instanceOf(a, ExpressionAST)){
            ret += ";"; // Expressions don't End on semicolons, therefore add one
        }
        ret += "\n";
    }
    return ret;
}

String SubBlockAST::_str() const {
    String ret = "";
    for (sptr<AST> a : contents) {
        ret += str(a.get());
        ret += "\n";
    }
    return intab(ret);
}

sptr<AST> SubBlockAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() == 0) return share<AST>(new SubBlockAST (false));
    std::vector<sptr<AST>> contents;
    bool has_returned=false;
    sptr<AST> last_return = nullptr;
    std::vector<sptr<AST>> unreachable = {};

    while (tokens.size() > 0) {
        lexer::TokenStream::Match split =
            tokens.rsplitStack({lexer::Token::Type::END_CMD, lexer::Token::Type::BLOCK_CLOSE});
        lexer::TokenStream buffer = tokens.slice(0, (uint64)split + 1);
        DEBUGT(2, "\e[1mSubBlockAST::parse\e[0m", &buffer);
        if (!(buffer.size() == 1 && buffer[0].type == lexer::Token::END_CMD)) {
            if (buffer.size() == 1 && buffer[0].type == lexer::Token::DOTDOTDOT) continue;
            sptr<AST> expr = parser::parseOneOf(
                buffer, {
                            NamespaceAST::parse, VarInitlAST::parse,
                            VarDeclAST::parse, parseStatement, /*EnumAST::parse,*/ IfAST::parse, ReturnAST::parse, FuncDefAST::parse,
                            DeleteAST::parse,
                            ImportAST::parse
                        }, local, sr, "void");

            if (expr == nullptr){
                parser::error(parser::errors["Expression expected"], buffer, "Expected a valid expression (Did you forget a ';'?)");
            }
            else {
                if (has_returned){
                    unreachable.push_back(expr);
                }
                if(instanceOf(expr, ReturnAST)){
                    has_returned = true;
                    last_return = expr;
                    // check variables for usage
                    for (std::pair<String, std::vector<symbol::Reference*>> sr : sr->contents){
                        if (sr.second.at(0) == dynamic_cast<symbol::Variable*>(sr.second.at(0))){
                            auto var = (symbol::Variable*)sr.second.at(0);
                            fsignal<void, parser::ErrorType, std::vector<lexer::Token>, String, String> warn_error = parser::error;
                            if (var->isFree){
                                warn_error = parser::warn;
                                if (var->getVarName()[0] == '_'){continue;}
                            }
                            if (var->used == symbol::Variable::PROVIDED && !(var->isStatic)){
                                warn_error(parser::errors["Type linearity violated"], *var->last, "This variable was provided, but never consumed." + (var->isFree ? "\nIf this was intended, prefix it with an '_'."s : ""s), "");
                            }
                            if (var->used == symbol::Variable::CONSUMED && var->isStatic && !var->isFree){
                                warn_error(parser::errors["Type linearity violated"], *var->last, "This static variable was consumed, but never provided.", "");
                            }
                            if (var->used == symbol::Variable::UNINITIALIZED){
                                parser::warn(parser::warnings["Unused Variable"], var->tokens, "This variable was declared, but never used" + (var->isFree ? "\nIf this was intended, prefix it with an '_'."s : ""s), 0);
                            }
                        }
                    }
                }
                if(instanceOf(expr, ExpressionAST) && !sr->ALLOWS_EXPRESSIONS){
                    parser::error(parser::errors["Expression forbidden"], expr->getTokens(), "A Block of type "s + sr->getName() + " does not allow Expressions");
                }
                else if (instanceOf(expr, ExpressionAST) && !instanceOf(expr, VarSetAST) && expr->getCstType() != "void"){
                    if (parser::isAtomic(expr->getCstType())){
                        if (!instanceOf(expr, FuncCallAST)){
                            parser::warn(parser::warnings["Unused output"], expr->getTokens(), "The return type from this expression was discarded.");
                        }
                    } else {
                        parser::error(parser::errors["Type linearity violated"], expr->getTokens(), "The (non-atomic) return type from this expression was discarded.");
                    }
                }
                contents.push_back(expr);
            }
            if(!unreachable.empty()){
                parser::error(parser::errors["Unreachable code"], {unreachable[0]->getTokens()[0], unreachable[unreachable.size()-1]->getTokens()[-1]}, "");
                parser::note(last_return->getTokens(),"because of this return statement");
            } 
        }
        tokens = split.after();
    }

    auto b = share<SubBlockAST>(new SubBlockAST(has_returned));
    b->contents = contents;
    return b;
}


sptr<AST> IfAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mIfAST::parse\e[0m");
    if (tokens.size() < 3)
        return nullptr;
    if (tokens[0].type == lexer::Token::IF) {
        DEBUG(2, "IfAST::parse");
        lexer::TokenStream::Match m = tokens.rsplitStack({lexer::Token::BLOCK_OPEN});
        if (!m.found()) { // TODO maybe error
            return ERR;
        }
        if (!(tokens[-1].type == lexer::Token::BLOCK_CLOSE)) {
            parser::error(parser::errors["Expected Block close"], {tokens[-1]},
                          "Expected a '}' token after '"s + tokens[-1].value + "'");

            return ERR;
        }
        sptr<AST> condition = math::parse(m.before().slice(1, m.before().size()), local, sr);
        if (condition == nullptr) {
            parser::error(parser::errors["Expression expected"], m.before().slice(1, tokens.size()), "Expected a valid expression in nowrap block");
            return ERR;
        }
        condition->consume("bool"_c);
        DEBUG(3, "\tcondition: "s + condition->emitCST());
        symbol::Namespace::LinearitySnapshot ls = sr->snapshot();
        symbol::SubBlock* sb = new symbol::SubBlock(sr);
        sb->include.push_back(sr);
        sptr<SubBlockAST> block = cast2(SubBlockAST::parse(m.after().slice(0, -1), local + 1, sb), SubBlockAST);

        DEBUG(4, "\tls:  "s + str(&ls));
        symbol::Namespace::LinearitySnapshot ls2 = sr->snapshot();
        DEBUG(4, "\tls2: "s + str(&ls2))
        if (!block->has_returned && ls != ls2) {
            DEBUG(3, "\ttype linearity violated!");
            parser::error(parser::errors["Type linearity violated"], tokens,
                          "The variables in this Block are not in the same state as before", 0);
            ls.traceback(ls2);
        }
        
        return share<AST>(new IfAST(block, condition, tokens, sb));
    }
    
    return nullptr;
}

sptr<AST> ReturnAST::parse(PARSER_FN_PARAM){
    DEBUG(4, "Trying \e[1mReturnAST::parse\e[0m");
    if (tokens.size() == 0) return nullptr;
    if (tokens.size() == 2){
        if (tokens[0].type == lexer::Token::RETURN){
            if (tokens[1].type != lexer::Token::END_CMD){
                parser::error(parser::errors["Expected ';'"], {tokens[-1]}, "expected ';' at the end of statement");
                return ERR;
            }
            if ((symbol::Function*)sr == dynamic_cast<symbol::Function*>(sr) || sr->getName() == "Function"){
                if (((symbol::Function*)sr)->getReturnType() != "void"){
                    parser::error(parser::errors["Type mismatch"], tokens, "expected a return of type \e[0m"s + ((symbol::Function*)sr)->getReturnType() + "\e[0m got void");
                    return ERR;
                }
                return share<AST>(new ReturnAST(ERR, tokens));
            } else {
                parser::error(parser::errors["return forbidden"], tokens, "return statements are not allowed in a block of type "s + sr->getName());
                return ERR;
            }
        }
    }
    else if (tokens[0].type == lexer::Token::RETURN){
        if (tokens[-1].type != lexer::Token::END_CMD){
            parser::error(parser::errors["Expected ';'"], {tokens[-1]}, "expected ';' at the end of statement");
            return ERR;
        }
        if ((symbol::Function*)sr == dynamic_cast<symbol::Function*>(sr) || sr->getName() == "Function"){

            sptr<AST> expr = math::parse(tokens.slice(1, -1), local, sr);

            if (expr == nullptr){
                parser::error(parser::errors["Expression expected"], tokens, "expected a valid expression");
                return ERR;
            }

            expr->consume(sr->getReturnType());
            return share<AST>(new ReturnAST(expr, tokens));
        } else {
            parser::error(parser::errors["return forbidden"], tokens, "return statements are not allowed in a block of type "s + sr->getName());
            return ERR;
        }
    }
    return nullptr;
}

