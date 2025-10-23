#include "var.hpp"
// #include "../../lexer/lexer.hpp"
#include "../../build/optimizer_flags.hpp"
#include "../../debug/debug.hpp"
#include "../errors.hpp"
#include "../parser.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"
#include "base_math.hpp"
#include "literal.hpp"
#include "type.hpp"

#include <cstdint>
#include <string>
#include <vector>

String parse_name(lexer::TokenStream tokens) {
    if (tokens.size() == 0) { return ""; }
    String             name = "";
    lexer::Token::Type last = lexer::Token::Type::SUBNS;

    if (tokens[0].type == lexer::Token::Type::SUBNS) {
        parser::error(parser::errors["Name expected"], {tokens[0]}, "module name or variable name expected");
        return "null"; // = errored already
    }
    for (lexer::Token t : *tokens.tokens) {
        if (last == lexer::Token::Type::SUBNS && t.type == lexer::Token::Type::ID) {
            name += t.value;
        } else if (last == lexer::Token::Type::ID && t.type == lexer::Token::Type::SUBNS) {
            name += "::";
        } else {
            return "";
        }
        last = t.type;
    }
    if (last == lexer::Token::Type::SUBNS) {
        parser::error(parser::errors["Name expected"],
                      {tokens[tokens.size() - 1]},
                      "module name or variable name expected");
        return "null";
    }
    return name;
}

sptr<AST> parseStatement(lexer::TokenStream tokens, int local, symbol::Namespace* sr, String expected_type) {
    if (tokens.size() == 0 || tokens[tokens.size() - 1].type != lexer::Token::Type::END_CMD) { return nullptr; }
    return math::parse(tokens.slice(0, tokens.size() - 1), local, sr, expected_type);
}

VarDeclAST::VarDeclAST(String name, sptr<AST> type, symbol::Variable* v) {
    this->name = name;
    this->type = type;
    this->v    = v;
}

String VarDeclAST::_str() const {
    return "<DECLARE "s + name + " : ?" + " MUT>";
}

sptr<AST> VarDeclAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying VarDeclAST::parse");
    if (tokens.size() < 3) { return nullptr; }
    if (tokens[tokens.size() - 1].type == lexer::Token::Type::END_CMD) {
        auto             tokens2 = tokens;
        String           name    = "";
        parser::Modifier m       = parser::getModifier(&tokens);
        if (tokens[tokens.size() - 2].type == lexer::Token::Type::ID) {
            name           = tokens[tokens.size() - 2].value;
            sptr<AST> type = Type::parse(tokens.slice(0, tokens.size() - 2), local, sr);
            if (type != nullptr) {
                // if (!parser::isAtomic(type->getCstType())){
                //     parser::error("Unknown type", {tokens[0], tokens[tokens.size()-3]}, "A type of this name is
                //     unknown in this scope", 19); return new AST;
                // }
                if ((*sr)[name].size() > 0) {
                    parser::error(parser::errors["Symbol already defined"],
                                  {tokens[tokens.size() - 2]},
                                  "A symbol of this name is already defined in this scope");
                    parser::note((*sr)[name][0]->tokens, "defined here:");
                    return ERR;
                }
                parser::checkName(name, tokens[tokens.size() - 2]);
                if (!parser::is_snake_case(name) && !(m & parser::Modifier::CONST)) {
                    parser::warn(parser::warnings["Wrong casing"],
                                 {tokens[tokens.size() - 2]},
                                 "Variable name should be snake_case");
                }
                if (m & parser::Modifier::CONST) {
                    parser::error(parser::errors["Const declaration without initialization"],
                                  tokens2,
                                  "A variable can only be const if an initialization is given");
                    parser::note(tokens2, "remove the 'const' keyword to resolve this easily");
                    return ERR;

                    if (m & parser::Modifier::STATIC) {
                        parser::warn(parser::warnings["Variable declared as constant and static"],
                                     tokens2,
                                     "declaring a constant static is ambigous");
                    }
                } else if (!(m & parser::Modifier::MUTABLE)) {
                    parser::error(parser::errors["Immutable declaration without initialization"],
                                  tokens2,
                                  "A variable can only be immutable if an initialization is given");
                    parser::noteInsert("Make this variable mutable if required", tokens2[0], "mut ", 0, true);
                    return ERR;
                }
                if (!sr->ALLOWS_NON_STATIC && !(parser::Modifier::STATIC | parser::Modifier::CONST)) {
                    parser::error(parser::errors["Only static variables allowed"],
                                  tokens2,
                                  "only static variables are allowed in a Block of type "s + sr->getName());
                }
                if (m & parser::Modifier::STATIC) {
                    parser::error(parser::errors["Static variable requires initial value"],
                                  tokens2,
                                  "a static variable requires a default value.");
                }

                symbol::Variable* v = new symbol::Variable(name, type->getCstType(), tokens2.tokens, sr);
                v->isConst          = m & parser::Modifier::CONST;
                v->isMutable        = m & parser::Modifier::MUTABLE;
                v->isStatic         = m & parser::Modifier::STATIC;
                sr->add(name, v);
                if (parser::isAtomic(type->getCstType())) { v->isFree = true; }
                return sptr<AST>(new VarDeclAST(name, type, v));
            }
        }
    }
    return nullptr;
}

VarInitlAST::VarInitlAST(String name, sptr<AST> type, sptr<AST> expr, symbol::Variable* v, lexer::TokenStream tokens) {
    this->name       = name;
    this->type       = type;
    this->expression = expr;
    this->v          = v;
    this->tokens     = tokens;
}

String VarInitlAST::_str() const {
    return "<DECLARE "s + name + " : " + type->getCstType() + " = " + str(expression.get()) +
           (v->isConst ? " CONST"s : ""s) + (v->isMutable ? " MUT"s : ""s) + (v->isStatic ? " STATIC"s : ""s) + ">";
}

sptr<AST> VarInitlAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying VarInitlAST::parse");
    if (tokens.size() < 5) { return nullptr; }
        if (tokens[tokens.size() - 1].type == lexer::Token::Type::END_CMD) {
            String                    name    = "";
            auto                      tokens2 = tokens;
            parser::Modifier          m       = parser::getModifier(&tokens);
            lexer::TokenStream::Match split   = tokens.rsplitStack({lexer::Token::Type::SET});
            if (tokens[uint64(split) - 1].type == lexer::Token::Type::ID && uint64(split) > 1) {
                DEBUG(2, "\e[1mVarInitlAST::parse\e[0m");
                name           = tokens[uint64(split) - 1].value;
                sptr<AST> type = Type::parse(split.before(), local, sr);
                if (type == nullptr) {
                    parser::error(parser::errors["Type expected"],
                                  split.before(),
                                  "Expected a valid type before the identifier");
                    return nullptr;
                }
                sptr<AST> expr = math::parse(split.after(), local, sr, type->getCstType());
                if (expr == nullptr) {
                    parser::error(parser::errors["Expression expected"],
                                  split.after(),
                                  "Expected an expression");
                    return ERR;
                }
                // if (!parser::isAtomic(type->getCstType()) ){
                //     parser::error("Unknown type", {tokens[0], tokens[tokens.size()-3]}, "A type of this name is unknown
                //     in this scope", 19); return new AST;
                // }
                if (sr->getLocal(name).size() > 0) {
                    parser::error(parser::errors["Symbol already defined"],
                                  {tokens[uint64(split) - 1]},
                                  "A Symbol of this name is already defined in this scope");
                    parser::note(sr->getLocal(name)[0]->tokens, "defined here:", 0);
                    return ERR;
                }
                parser::checkName(name, tokens[uint64(split) - 1]);
                if (!parser::is_snake_case(name) && !(m & parser::Modifier::CONST)) {
                    parser::warn(parser::warnings["Wrong casing"],
                                 {tokens[uint64(split) - 1]},
                                 "Variable name should be snake_case");
                }
                if (!parser::IS_UPPER_CASE(name) && m & parser::Modifier::CONST) {
                    parser::warn(parser::warnings["Wrong casing"],
                                 {tokens[uint64(split) - 1]},
                                 "Constant name should be UPPER_CASE");
                }
                if (m & parser::Modifier::CONST) {
                    if (m & parser::Modifier::MUTABLE) {
                        parser::error(parser::errors["Variable declared as constant and mutable"],
                                      {tokens[uint64(split) - 1]},
                                      "This variable was declared as 'const' (unchangeable) and 'mut' (changeable) at the same time."); return ERR;
                    }
                    if (m & parser::Modifier::STATIC) {
                        parser::warn(parser::warnings["Variable declared as constant and static"],
                                     {tokens[uint64(split) - 1]},
                                     "declaring a constant static is ambigous");
                    }
                }
                expr->consume(type->getCstType());
                auto v = new symbol::Variable(name, type->getCstType(), tokens2.tokens, sr);

                if (m & parser::Modifier::CONST) {
                    if (!expr->is_const) {
                        parser::error(
                            parser::errors["Non-constant value in constant variable"],
                            expr->getTokens(),
                            "you are trying to assign a non-constant value to a constant variable. This is not "
                            "supported.\nRemove the 'const' keyword to get an immutable variable which allows that.");
                        if (!optimizer::do_constant_folding) {
                            parser::note(expr->getTokens(),
                                         "This could be a direct result of disabling --opt:constant-folding");
                        }
                        delete v;
                        return ERR;
                    }
                    v->const_value = expr->value;
                }
                if (!sr->ALLOWS_NON_STATIC && !(m & (parser::Modifier::STATIC | parser::Modifier::CONST))) {
                    parser::error(parser::errors["Only static variables allowed"],
                                  {tokens[uint64(split) - 1]},
                                  "only static variables are allowed in a Block of type "s + sr->getName());
                }

                v->isConst   = m & parser::Modifier::CONST;
                v->isMutable = m & parser::Modifier::MUTABLE;
                sr->add(name, v);
                if (parser::isAtomic(type->getCstType())) { v->isFree = true; }
                v->used = symbol::Variable::PROVIDED;

                return share<AST>(new VarInitlAST(name, type, expr, v, tokens));
            }
        }
    /*if (tokens.size() < 5) { return nullptr; }
    if (tokens[-1].type == lexer::Token::Type::END_CMD) {
        lexer::TokenStream tokens2 = tokens;
        tokens = tokens.slice(0, -1);
        parser::Modifier          m     = parser::getModifier(tokens);
        lexer::TokenStream::Match split = tokens.rsplitStack({lexer::Token::SET});
        if (split.found()) {
            lexer::Token name = tokens[int64(split)-1];

            sptr<AST> type = Type::parse(split.before().slice(0, -1), local, sr);

            sptr<AST> expr = math::parse(split.after(), local, sr);
        }
    }*/

    return nullptr;
}

VarAccesAST::VarAccesAST(String name, symbol::Reference* sr, lexer::TokenStream tokens) {
    this->name   = name;
    this->var    = sr;
    this->tokens = tokens;
    if (sr == dynamic_cast<symbol::Variable*>(sr)) {
        this->is_const = ((symbol::Variable*) sr)->isConst;
        if (is_const) { value = ((symbol::Variable*) sr)->const_value; }
    }
}

String VarAccesAST::_str() const {
    return "<ACCES "s + name + (is_const ? "[="s + ((symbol::Variable*) var)->const_value + "]" : ""s) + ">";
}

sptr<AST> VarAccesAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying VarAccesAST::parse");
    if (tokens.size() == 0) { return nullptr; }
    String name = parse_name(tokens);
    DEBUG(5, "\tname: "s + name)
    if (name == "") { return nullptr; }
    if (name == "null") { return ERR; }
    if ((*sr)[name].size() == 0) {
        parser::error(parser::errors["Unknown variable"],
                      tokens,
                      "A variable of this name was not found in this scope");
        return ERR;
    }

    symbol::Reference* p = (*sr)[name].at(0);
    return share<AST>(new VarAccesAST(name, p, tokens));
}

void VarAccesAST::consume(CstType type) {
    if (var->getCstType() != type) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      String("expected a \e[1m") + type + "\e[0m, got a variable of type " + var->getCstType());
    }
    symbol::Reference* p = var;
    if (p == dynamic_cast<symbol::Variable*>(p)) {
        symbol::Variable::Status& u = ((symbol::Variable*) p)->used;
        if (u == symbol::Variable::UNINITIALIZED) {
            parser::error(
                parser::errors["Variable uninitilialized"],
                tokens,
                "Variable '\e[1m"s + name +
                    "\e[0m' is uninitilialized at this point.\nMake sure the variable holds a value to resolve.");
            parser::note(p->tokens, "declared here");
            return;
        } else if (u == symbol::Variable::CONSUMED && !((symbol::Variable*) p)->isFree) {
            parser::error(parser::errors["Type linearity violated"],
                          tokens,
                          "Variable '\e[1m"s + name +
                              "\e[0m' is consumed at this point.\nMake sure the variable holds a value to resolve.");
            parser::note(p->last, "last consumed here");
            return;
        }
        u       = symbol::Variable::CONSUMED;
        p->last = tokens.tokens;
    }
}

CstType VarAccesAST::provide() {
    if (var == dynamic_cast<symbol::Variable*>(var)) {
        symbol::Variable::Status& u = ((symbol::Variable*) var)->used;
        if (((symbol::Variable*) var)->isConst) {
            parser::error(parser::errors["Trying to set constant"],
                          tokens,
                          "You are trying to set a variable which has a constant value.");
            parser::note(var->tokens, "defined here:");
        }
        if (!(((symbol::Variable*) var)->isMutable)) {
            parser::error(parser::errors["Trying to set immutable"],
                          tokens,
                          "You are trying to set a variable which was declared as immutable.");
            parser::note(var->tokens, "defined here:");
        }
        if (u == symbol::Variable::PROVIDED) {
            fsignal<void, parser::ErrorType, lexer::TokenStream, String, String> warn_error = parser::error;
            if (((symbol::Variable*) var)->isFree) { warn_error = parser::warn; }

            warn_error(parser::errors["Type linearity violated"],
                       tokens,
                       "Variable '\e[1m"s + name +
                           "\e[0m' was never consumed.\nMake sure the variable is consumed before it is provided.",
                       "");
            parser::note(var->last, "last provided here");
        }
        u         = symbol::Variable::PROVIDED;
        var->last = tokens.tokens;
    } else {
        parser::error(parser::errors["Cannot assign to symbol"],
                      tokens,
                      "Cannot assign a value to a symbol of type " + var->getName());
    }
    return var->getCstType();
}

/*
VarSetAST::VarSetAST(String name, symbol::Variable* sr, sptr<AST> expr, lexer::TokenStream tokens) {
    this->name   = name;
    this->var    = sr;
    this->expr   = expr;
    this->tokens = tokens;
}

String VarSetAST::_str() const {
    return "<"s + name + " = " + str(expr.get()) + ">";
}

sptr<AST> VarSetAST::parse(PARSER_FN_PARAM) {
    DEBUG(4, "Trying \e[1mVarSetAST::parse\e[0m");
    if (tokens.size() == 0) { return nullptr; }
    String                    name  = "";
    lexer::TokenStream::Match split = tokens.rsplitStack({lexer::Token::Type::SET});
    if (split.found() && (uint64) split == 0) {
        parser::error("Expected Expression", {tokens[tokens.size() - 1]}, "Expected an expression after '='", 31);
        return ERR;
    }
    if (!split.found()) { return nullptr; }
    auto varname = split.before();
    name         = parse_name(varname);
    if (name == "") {
        parser::error("Expected Symbol", varname, "module name or variable name expected", 30);
        return nullptr;
    }

    sptr<AST> expr = math::parse(split.after(), local, sr);
    if (expr == nullptr) {
        parser::error("Expected Expression", {tokens[tokens.size() - 1]}, "Expected a valid expression after '='", 31);
        return ERR;
    }

    if ((*sr)[name].size() == 0) {
        parser::error("Unknown variable", tokens, "A variable of this name was not found in this scope", 20);
        return ERR;
    }

    expr->forceType((*sr)[name][0]->getCstType());
    symbol::Reference* p = (*sr)[name].at(0);
    if (p == dynamic_cast<symbol::Variable*>(p) && ((symbol::Variable*) p)->isConst) {
        parser::error("Trying to set constant",
                      tokens,
                      "You are trying to set a variable which has a constant value.",
                      17);
        parser::note(p->tokens, "defined here:", 0);
        return ERR;
    }
    if (!(p == dynamic_cast<symbol::Variable*>(p) && ((symbol::Variable*) p)->isMutable)) {
        parser::error("Trying to set immutable",
                      tokens,
                      "You are trying to set a variable which was declared as immutable.",
                      18);
        parser::note(p->tokens, "defined here:", 0);
        return ERR;
    }
    return share<AST>(new VarSetAST(name, (symbol::Variable*) p, expr, tokens));
}
*/
void VarSetAST::consume(CstType type) {
    if (right->getCstType() != type) {
        parser::error(parser::errors["Type mismatch"],
                      tokens,
                      String("expected a \e[1m") + type + "\e[0m, got a value of type " + right->getCstType());
    }
}

sptr<AST> VarSetAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() < 1) { return nullptr; }
    lexer::TokenStream::Match split = tokens.rsplitStack({lexer::Token::SET});
    if (split.found()) {
        sptr<AST> before = math::parse(split.before(), local, sr);
        if (before == nullptr) {
            parser::error(parser::errors["Expression expected"], split.before(), "Expected an expression");
            return ERR;
        }
        sptr<AST> after = math::parse(split.before(), local, sr);
        if (after == nullptr) {
            parser::error(parser::errors["Expression expected"], split.after(), "Expected an expression");
            return ERR;
        }
        after->consume(before->provide());
        return share<AST>(new VarSetAST(before, after, tokens));
    }
    return nullptr;
}

sptr<AST> DeleteAST::parse(PARSER_FN_PARAM) {
    if (tokens.size() < 2) { return nullptr; }
    if (tokens[0].type == lexer::Token::DELETE) {
        if (tokens[-1].type != lexer::Token::END_CMD) {
            parser::error(parser::errors["Expected ';'"], {tokens[-1]}, "expected a ';' at the end of this statement");
            return ERR;
        }
        std::vector<symbol::Variable*> vars    = {};
        lexer::TokenStream             tokens2 = tokens.slice(1, -1);
        lexer::TokenStream             buffer  = lexer::TokenStream({});
        while (!tokens2.empty()) {
            lexer::TokenStream::Match m = tokens2.rsplitStack({lexer::Token::COMMA});
            if (m.found()) {
                buffer  = m.before();
                tokens2 = m.after();
            } else {
                buffer  = tokens2;
                tokens2 = lexer::TokenStream({});
            }
            if (buffer.empty()) { continue; }

            sptr<AST>         var = VarAccesAST::parse(buffer, local, sr);
            sptr<VarAccesAST> vaast;
            if (var != nullptr&& instanceOf(var, VarAccesAST)){
                vaast = cast2(var, VarAccesAST); 
            }
            if (var == nullptr){
                parser::error(parser::errors["Name expected"], buffer, "Expected a valid variable name");
                continue;
            }
            else if (instanceOf(var, VarAccesAST)){
                if (vaast->var == dynamic_cast<symbol::Variable*>(vaast->var)){
                    if (((symbol::Variable*)(vaast->var))->isFree){
                        parser::error(parser::errors["Trying to delete free variable"], buffer, "Variables of atomics cannot be deleted");
                        continue;
                    } else {
                        vars.push_back((symbol::Variable*)(vaast->var));
                    }
                }
                else {
                    parser::error(parser::errors["Symbol undeleteable"],buffer, "Symbol of type " + cast2(var, VarAccesAST)->var->getName() + " cannot be deleted");
                }
            }
        }
        return share<AST>(new DeleteAST(vars, tokens));
    }
    return nullptr;
}
