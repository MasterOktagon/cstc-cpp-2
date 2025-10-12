#include "errors.hpp"
#include "../lexer/token.hpp"
#include "../snippets.hpp"
#include "../helpers/string_functions.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

uint64 parser::errc = 0;
uint64 parser::warnc = 0;
bool   parser::one_error = false;
bool   muted             = false;

// Register all known compiler error types
enum { COUNTER_BASE = __COUNTER__ };
#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE + 1)
#define REGISTER_ERROR(name) {name, {LOCAL_COUNTER, name}}

map<string, parser::ErrorType> parser::errors = {
    REGISTER_ERROR("Unused variable"),
    REGISTER_ERROR("Type linearity violated"),
    REGISTER_ERROR("Expression expected"),
    REGISTER_ERROR("Amount expected"),
    REGISTER_ERROR("Name expected"),
    REGISTER_ERROR("Type Expected"),
    REGISTER_ERROR("Sign mismatch"),
    REGISTER_ERROR("Type mismatch"),
    REGISTER_ERROR("Expression unassignable"),
    REGISTER_ERROR("Empty char"),
    REGISTER_ERROR("Invalid char"),
    REGISTER_ERROR("Invalid name"),
    REGISTER_ERROR("Expected Block open"),
    REGISTER_ERROR("Expected Block close"),
    REGISTER_ERROR("Unknown operator"),
    REGISTER_ERROR("Unknown function"),
    REGISTER_ERROR("Unknown variable"),
    REGISTER_ERROR("Unknown method"),
    REGISTER_ERROR("Illegal optional argument name"),
    REGISTER_ERROR("Positional argument after optional argument"),
    REGISTER_ERROR("Mismatching operands"),
    REGISTER_ERROR("Parameter name already used"),
    REGISTER_ERROR("Positional parameter after named parameter"),
    REGISTER_ERROR("Unreturned function"),
    REGISTER_ERROR("Expression forbidden"),
    REGISTER_ERROR("Function forbidden"),
    REGISTER_ERROR("return forbidden"),
    REGISTER_ERROR("Unreachable code"),
    REGISTER_ERROR("Expected ';'"),
    REGISTER_ERROR("Array type not specified"),
    REGISTER_ERROR("Expected list of names"),
    REGISTER_ERROR("Import-all must be at line End"),
    REGISTER_ERROR("Unexpected token"),
    REGISTER_ERROR("Unexpected import"),
    REGISTER_ERROR("Symbol already defined"),
    REGISTER_ERROR("Unknown variable"),
    REGISTER_ERROR("Unsupported name"),
    REGISTER_ERROR("Variable uninitilialized"),
    REGISTER_ERROR("Trying to set constant"),
    REGISTER_ERROR("Trying to set immutable"),
    REGISTER_ERROR("Const declaration without initialization"),
    REGISTER_ERROR("Immutable declaration without initialization"),
    REGISTER_ERROR("Only static variables allowed"),
    REGISTER_ERROR("Static variable requires initial value"),
    REGISTER_ERROR("Variable declared as constant and mutable"),
    REGISTER_ERROR("Non-constant value in constant variable"),
    REGISTER_ERROR("Trying to delete free variable"),
    REGISTER_ERROR("Namespace not allowed"),
    REGISTER_ERROR("Unopened multiline comment"),
    REGISTER_ERROR("Unresolved merge conflict"),
};

#undef LOCAL_COUNTER
// Register all known compiler warnings types
enum { COUNTER_BASE_2 = __COUNTER__ };
#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE_2 + 1)
#define REGISTER_WARNING(name) {name, {LOCAL_COUNTER, name}}

std::map<string, parser::ErrorType> parser::warnings = {
    REGISTER_WARNING("Unused variable"),
    REGISTER_WARNING("Type linearity violated"),
    REGISTER_WARNING("Wrong casing"),
    REGISTER_WARNING("Unused output"),
    REGISTER_WARNING("Variable declared as constant and static"),
    REGISTER_WARNING("Line too long"),
    REGISTER_WARNING("Unclosed multiline comment"),
};

#undef LOCAL_COUNTER

void parser::mute() {
    muted = true;
}
void parser::unmute() {
    muted = false;
}

void showError(string errstr, string errcol, string errcol_lite, string name, string msg, vector<lexer::Token> tokens, uint32 code, string appendix){
    if (muted) return;
    if (tokens.size() == 0) {
        std::cerr << "OH NO! " << errstr << " " << name << " could not be displayed:\n" << intab(msg) << "\n";
        return;
    }
    string location;
    if (tokens.size() == 1){
        location = ":"s + to_string(tokens[0].line) + ":" + to_string(tokens[0].column); 
    }
    else {
        location = ":"s + to_string(tokens[0].line) + ":" + to_string(tokens[0].column); 
        location += " - " + to_string(tokens.at(tokens.size()-1).line) + ":" + to_string(tokens.at(tokens.size()-1).column + tokens.at(tokens.size()-1).value.size()-1);
    }

    std::cerr << "\r" << errcol << errstr << ": " << name << "\e[0m @ \e[0m" << tokens[0].filename << "\e[1m" << location << "\e[0m" << (code == 0? ""s : " ["s + errstr[0] + to_string(code) + "]") << ":" << std::endl;
    std::cerr << msg << std::endl;
    std::cerr << "       | " << std::endl;
    if (tokens.size() == 1){
        std::cerr << " " << fillup(to_string(tokens[0].line), 5) << " | " << *(tokens[0].line_contents) << std::endl;
        std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column-1) << fillup("", tokens[0].value.size(), '^') << "\e[0m" << std::endl;
    } else {
        std::cerr << " " << fillup(to_string(tokens[0].line), 5) << " | " << (*(tokens[0].line_contents)) << std::endl;
        if (tokens[0].line == tokens.at(tokens.size()-1).line){
            std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column-1) << fillup("", tokens.at(tokens.size()-1).column-(tokens[0].column-1)+tokens.at(tokens.size()-1).value.size()-1, '^') << "\e[0m" << std::endl;
        }
        else {
            std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column-1) << fillup("", (*(tokens[0].line_contents)).size()-(tokens[0].column-1)-1, '^') << "\e[0m" << std::endl;
            if (tokens.at(tokens.size()-1).line - tokens[0].line > 1)
                std::cerr << "       | \t" << errcol_lite << "(" << to_string(tokens.at(tokens.size()-1).line - tokens[0].line - 1) << " line" << (tokens.at(tokens.size()-1).line - tokens[0].line - 1 == 1 ? "" : "s") << " hidden)\e[0m" << std::endl; 

            std::cerr << " " << fillup(to_string(tokens.at(tokens.size()-1).line), 5) << " | " << *(tokens.at(tokens.size()-1).line_contents) << std::endl;
            std::cerr << "       | " << errcol_lite  << fillup("", tokens.at(tokens.size()-1).column + tokens.at(tokens.size()-1).value.size()-1, '^') << "\e[0m" << std::endl;
        }
    }
    
    std::cerr << appendix << std::endl;
}

void parser::error(ErrorType type, lexer::TokenStream tokens, string msg, string appendix){
    showError("ERROR", "\e[1;31m", "\e[31m", type.name, msg, *tokens.tokens, type.code, appendix);
    errc++;
    if (one_error){
        std::exit(3);
    }
}
void parser::error(ErrorType type, vector<lexer::Token> tokens, string msg, string appendix){
    showError("ERROR", "\e[1;31m", "\e[31m", type.name, msg, tokens, type.code, appendix);
    errc++;
    if (one_error){
        std::exit(3);
    }
}
void parser::warn(ErrorType type, lexer::TokenStream tokens, string msg, string appendix){
    showError("WARNING", "\e[1;33m", "\e[33m", type.name, msg, *tokens.tokens, type.code, appendix);
    warnc++;
}
void parser::note(lexer::TokenStream tokens, string msg, string appendix){
    showError("NOTE", "\e[1;36m", "\e[36m", "", msg, *tokens.tokens, 0, appendix);
}

void parser::note(vector<lexer::Token> tokens, string msg, string appendix){
    showError("NOTE", "\e[1;36m", "\e[36m", "", msg, tokens, 0, appendix);
}
void parser::warn(parser::ErrorType type, vector<lexer::Token> tokens, string msg, string appendix){
    showError("WARNING", "\e[1;33m", "\e[33m", type.name, msg, tokens, type.code, appendix);
    warnc++;
}


/*
void parser::noteInsert(string msg, lexer::Token after, string insert, uint32 code, bool before, string appendix){

    string location;
    location = ":"s + to_string(after.line) + ":" + to_string(after.column); 

    std::cerr << "\r" << "\e[1;36mNote" << ": " << "\e[0m @ \e[0m" << after.filename << "\e[1m" << location << "\e[0m" << (code == 0? ""s : " [N"s + to_string(code) + "]") << ":" << std::endl;
    std::cerr << msg << std::endl;
    std::cerr << "       | " << std::endl;
    std::cerr << " " << fillup(to_string(after.line), 5) << " | " << (*(after.line_contents)).insert(after.column-1 + (before? 0 : after.value.size()), "\e[36m"s + insert + "\e[0m") << std::endl;
    std::cerr << "       | " << std::endl;
    std::cerr << appendix << std::endl;
}*/