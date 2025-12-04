#include "errors.hpp"

#include "../debug.hpp"
#include "../helpers/string_functions.hpp"
#include "../lexer/token.hpp"
#include "../module.hpp"
#include "../snippets.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

uint64 parser::errc      = 0;
uint64 parser::warnc     = 0;
bool   parser::one_error = false;
bool   muted             = false;

// Register all known compiler error types
enum {
    COUNTER_BASE = __COUNTER__
};

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE + 1)
#define REGISTER_ERROR(name)    \
    {                           \
        name, {                 \
            LOCAL_COUNTER, name \
        }                       \
    }

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
    REGISTER_ERROR("Modifier not allowed"),
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
    REGISTER_ERROR("Module not found"),
    REGISTER_ERROR("File not found"),
};

#undef LOCAL_COUNTER

// Register all known compiler warnings types
enum {
    COUNTER_BASE_2 = __COUNTER__
};

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE_2 + 1)
#define REGISTER_WARNING(name)  \
    {                           \
        name, {                 \
            LOCAL_COUNTER, name \
        }                       \
    }

std::map<string, parser::ErrorType> parser::warnings = {
    REGISTER_WARNING("Unused variable"),
    REGISTER_WARNING("Type linearity violated"),
    REGISTER_WARNING("Wrong casing"),
    REGISTER_WARNING("Unused output"),
    REGISTER_WARNING("Variable declared as constant and static"),
    REGISTER_WARNING("Line too long"),
    REGISTER_WARNING("Unclosed multiline comment"),
    REGISTER_WARNING("No implementation file found"),
    REGISTER_WARNING("Import not at top"),
};

#undef LOCAL_COUNTER

void parser::mute() {
    muted = true;
}

void parser::unmute() {
    muted = false;
}

void showError(string               errstr,
               string               errcol,
               string               errcol_lite,
               string               name,
               string               msg,
               vector<lexer::Token> tokens,
               uint32               code,
               string               appendix) {
    if (muted) { return; }
    if (tokens.size() == 0) {
        std::cerr << "OH NO! " << errstr << " " << name << " could not be displayed:\n" << msg << "\n";
        return;
    }
    string location;
    if (tokens.size() == 1) {
        location = ":"s + to_string(tokens[0].line) + ":" + to_string(tokens[0].column);
    } else {
        location  = ":"s + to_string(tokens[0].line) + ":" + to_string(tokens[0].column);
        location += " - " + to_string(tokens.at(tokens.size() - 1).line) + ":" +
                    to_string(tokens.at(tokens.size() - 1).column + tokens.at(tokens.size() - 1).value.size() - 1);
    }

    std::cerr << "\r" << errcol << errstr << ": " << name << "\e[0m @ \e]8;;file://" << *(tokens[0].filename) << "\e\\\e[0;37m" << Module::directory.string()
              << "/\e[0m\e[1m" << tokens[0].filename->substr(Module::directory.string().size() + 1) << location
              << "\e[0m\e]8;;\e\\" << (code == 0 ? ""s : " ["s + errstr[0] + to_string(code) + "]") << ":" << std::endl;
    std::cerr << "\e[0m" << msg << "\e[0m" << std::endl;
    std::cerr << "       | " << std::endl;
    if (tokens.size() == 1) {
        std::cerr << " " << fillup(to_string(tokens[0].line), 5) << " | " << *(tokens[0].line_contents) << std::endl;
        std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column - 1)
                  << fillup("", tokens[0].value.size(), '^') << "\e[0m" << std::endl;
    } else {
        std::cerr << " " << fillup(to_string(tokens[0].line), 5) << " | " << (*(tokens[0].line_contents)) << std::endl;
        if (tokens[0].line == tokens.at(tokens.size() - 1).line) {
            std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column - 1)
                      << fillup("",
                                tokens.at(tokens.size() - 1).column - (tokens[0].column - 1) +
                                    tokens.at(tokens.size() - 1).value.size(),
                                '^')
                      << "\e[0m" << std::endl;
        } else {
            std::cerr << "       | " << errcol_lite << fillup("", tokens[0].column - 1)
                      << fillup("", (*(tokens[0].line_contents)).size() - (tokens[0].column - 1) - 1, '^') << "\e[0m"
                      << std::endl;
            if (tokens.at(tokens.size() - 1).line - tokens[0].line > 1) {
                std::cerr << "       | \t" << errcol_lite << "("
                          << to_string(tokens.at(tokens.size() - 1).line - tokens[0].line - 1) << " line"
                          << (tokens.at(tokens.size() - 1).line - tokens[0].line - 1 == 1 ? "" : "s") << " hidden)\e[0m"
                          << std::endl;
            }

            std::cerr << " " << fillup(to_string(tokens.at(tokens.size() - 1).line), 5) << " | "
                      << *(tokens.at(tokens.size() - 1).line_contents) << std::endl;
            std::cerr << "       | " << errcol_lite
                      << fillup("",
                                tokens.at(tokens.size() - 1).column + tokens.at(tokens.size() - 1).value.size() - 1,
                                '^')
                      << "\e[0m" << std::endl;
        }
    }

    std::cerr << appendix << std::endl;
}

vector<lexer::TokenStream> splitIncluded(lexer::TokenStream tokens) {
    if (tokens.empty()) { return {}; }
    vector<lexer::TokenStream> out          = {};
    usize                      i            = 0;
    usize                      start        = 0;
    string                     last_include = tokens[0].include ? *tokens[0].include : "";

    while (i < tokens.size()) {
        while (i < tokens.size() and (tokens[i].include ? *tokens[i].include : "") == last_include) { i++; }
        if (last_include != "" and start != i) { out.push_back(tokens.slice(start, i)); }
        start = i;
        if (i < tokens.size()) { last_include = tokens[i].include ? *tokens[i].include : ""; }
    }
    if (last_include != "" and start != i) { out.push_back(tokens.slice(start, i)); }
    return out;
}

void parser::error(ErrorType type, lexer::TokenStream tokens, string msg, string appendix) {
    showError("ERROR", "\e[1;31m", "\e[31m", type.name, msg, *tokens.tokens, type.code, appendix);
    for (lexer::TokenStream t : splitIncluded(tokens)) { note(t, "included from file: "_s + *t[0].include); }
    errc++;
    if (one_error) { std::exit(3); }
}

void parser::error(ErrorType type, vector<lexer::Token> tokens, string msg, string appendix) {
    showError("ERROR", "\e[1;31m", "\e[31m", type.name, msg, tokens, type.code, appendix);
    for (lexer::TokenStream t : splitIncluded(lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens)))) {
        note(t, "included from file: "_s + *t[0].include);
    }
    errc++;
    if (one_error) { std::exit(3); }
}

void parser::warn(ErrorType type, lexer::TokenStream tokens, string msg, string appendix) {
    showError("WARNING", "\e[1;33m", "\e[33m", type.name, msg, *tokens.tokens, type.code, appendix);
    for (lexer::TokenStream t : splitIncluded(tokens)) { note(t, "included from file: "_s + *t[0].include); }
    warnc++;
}

void parser::note(lexer::TokenStream tokens, string msg, string appendix) {
    showError("NOTE", "\e[1;36m", "\e[36m", "", msg, *tokens.tokens, 0, appendix);
}

void parser::note(vector<lexer::Token> tokens, string msg, string appendix) {
    showError("NOTE", "\e[1;36m", "\e[36m", "", msg, tokens, 0, appendix);
}

void parser::warn(parser::ErrorType type, vector<lexer::Token> tokens, string msg, string appendix) {
    showError("WARNING", "\e[1;33m", "\e[33m", type.name, msg, tokens, type.code, appendix);
    for (lexer::TokenStream t : splitIncluded(lexer::TokenStream(make_shared<vector<lexer::Token>>(tokens)))) {
        note(t, "included from file: "_s + *t[0].include);
    }
    warnc++;
}

parser::HelpBuffer::HelpBuffer(string s) {
    type    = STRING;
    this->s = s;
}

parser::HelpBuffer::HelpBuffer(const char* s) {
    type    = STRING;
    this->s = string(s);
}

parser::HelpBuffer::HelpBuffer(lexer::Token t) {
    type    = TOKEN;
    this->t = t;
}

parser::HelpBuffer::HelpBuffer(lexer::TokenStream st) {
    type     = TOKEN_STREAM;
    this->st = st;
}

void parser::HelpBuffer::append(parser::HelpBuffer buf) {
    if (next == nullptr) {
        next = make_shared<HelpBuffer>(buf);
        return;
    }
    next->append(buf);
}

parser::HelpBuffer parser::HelpBuffer::operator+(HelpBuffer h) {
    append(h);
    return *this;
}

parser::HelpBuffer parser::HelpBuffer::operator+(const char* h) {
    append(h);
    return *this;
}

parser::HelpBuffer parser::HelpBuffer::operator+(string h) {
    append(h);
    return *this;
}

parser::HelpBuffer parser::HelpBuffer::operator+(lexer::Token h) {
    append(h);
    return *this;
}

parser::HelpBuffer parser::HelpBuffer::operator+(lexer::TokenStream h) {
    append(h);
    return *this;
}

parser::HelpBuffer operator""_h(const char* c, usize s) {
    return parser::HelpBuffer(c);
}

parser::HelpBuffer::~HelpBuffer() {
}

string parser::HelpBuffer::_str() const {
    string out;
    if (type == STRING) {
        out += "s:" + s;
    } else if (type == TOKEN) {
        out += "t:" + t.value;
    } else if (type == TOKEN_STREAM) {
        lexer::TokenStream t  = st;
        out                  += "st:" + str(t);
    }
    if (next == nullptr) { return out; }
    return out += " -> " + next->_str();
}

TEST_CASE ("testing HelpBuffer creation", "[errors]") { parser::HelpBuffer h = "Hello"_h + lexer::Token(); }

struct HelpLineInfo {
        uint64                     line;
        string                     prefix;
        vector<parser::HelpBuffer> contents;
        string                     suffix;
};

#define addHelpBuffer(line, prefix) lines.push_back({line, prefix, {}, ""});

#define handleHelpBufferString(idx)                                                    \
    usize  found = cached_prefix.find("\n");                                           \
    uint32 _a    = idx;                                                                \
    while (found != string::npos) {                                                    \
        addHelpBuffer(current_line++ + ++line_offset, "");                             \
        lines.at(_a + line_offset).contents.push_back(cached_prefix.substr(0, found)); \
        cached_prefix = cached_prefix.substr(found + 1);                               \
        found         = cached_prefix.find("\n");                                      \
    }                                                                                  \
    lines.at(_a + line_offset).contents.push_back(cached_prefix);                      \
    cached_prefix = "";

void parser::help(HelpBuffer buf, string msg, string appendix) {
    if (buf.next == nullptr) {
        DEBUG(1, "Help message "_s + msg + " could not be displayed");
        return;
    }

    string filename;
    uint64 line_start   = 0;
    uint64 column_start = 0;

    std::vector<HelpLineInfo> lines = {
        {0, "", {}, ""}
    };

    uint64 current_line = 0;
    uint64 line_offset  = 0;
    string cached_prefix;

    HelpBuffer* p = &buf;

    // cout << "input: " << str(buf) << endl;

    while (p != nullptr) {
        if (p->type == HelpBuffer::STRING) {
            if (lines.at(lines.size() - 1).line == 0) {
                cached_prefix += p->s;
            } else {
                lines.at(lines.size() - 1).contents.push_back(*p);
            }
        }
        if (p->type == HelpBuffer::TOKEN) {
            if (filename == "") {
                filename   = *(p->t.filename);
                line_start = current_line = p->t.line;
                column_start              = p->t.column;
                lines.at(0).prefix        = p->t.line_contents->substr(0, p->t.column - 1);
                lines.at(0).line          = line_start;
                handleHelpBufferString(0)
            }
            if (p->t.line + line_offset != current_line) {
                current_line = p->t.line + line_offset;

                lines.push_back({p->t.line + line_offset, p->t.line_contents->substr(0, p->t.column - 1), {}, ""});
                handleHelpBufferString(lines.size() - 1);
            }
            lines.at(lines.size() - 1).contents.push_back(*p);
        }
        if (p->type == HelpBuffer::TOKEN_STREAM) {
            for (lexer::Token t : *(p->st.tokens)) {
                if (filename == "") {
                    filename   = *t.filename;
                    line_start = current_line = t.line;
                    column_start              = t.column;
                    lines.at(0).prefix        = t.line_contents->substr(0, t.column - 1);
                    lines.at(0).line          = line_start;
                    handleHelpBufferString(0)
                }
                if (t.line + line_offset != current_line) {
                    current_line = t.line + line_offset;

                    lines.push_back({t.line + line_offset, t.line_contents->substr(0, t.column - 1), {}, ""});
                    handleHelpBufferString(lines.size() - 1);
                }
                lines.at(lines.size() - 1).contents.push_back(t);
            }
        }
        p = p->next.get();
    }

    cerr << "\e[1;32mHELP:\e[0m @ \e]8;;file://" << filename << "\e\\\e[0;37m" << Module::directory.string() << "/\e[0m\e[1m"
         << filename.substr(Module::directory.string().size() + 1) << ":" << line_start << ":" << column_start
         << "\e[0m\e]8;;\e\\" << endl;
    cerr << msg << endl;
    cerr << "       | " << endl;

    uint64                  last_line = line_start;
    HelpBuffer::ContentType last_type = HelpBuffer::STRING;
    for (HelpLineInfo info : lines) {
        uint32 last_column = 0;
        if (info.line == 0) { continue; }
        if (info.line - last_line > 1) { cerr << "  \e[37m...\e[0m  |" << endl; }
        cerr << " " << fillup(to_string(info.line), 6) << "| " << info.prefix;
        for (HelpBuffer h : info.contents) {
            switch (h.type) {
                default :
                case parser::HelpBuffer::STRING :
                    {
                        cerr << "\e[32m" << h.s << "\e[0m";
                        break;
                    }
                case parser::HelpBuffer::TOKEN :
                    {
                        if (last_type != HelpBuffer::STRING) { cerr << fillup("", h.t.column - last_column); }
                        cerr << h.t.value;
                        last_column = h.t.column + h.t.value.size();
                        break;
                    }
            }
            last_type = h.type;
        }
        last_line = info.line;
        cerr << endl;
    }

    cerr << "       | " << endl;
    cerr << appendix << endl;
}
/*
TEST_CASE ("testing help message", "[errors]") {
    parser::help("Hello"_h + "h" +
                     lexer::TokenStream(make_shared<vector<lexer::Token>>(vector<lexer::Token>(
                         {lexer::Token(lexer::Token::OPEN,
                                       "(",
                                       1,
                                       3,
                                       make_shared<string>(Module::directory.string() + "/test.cst"),
                                       make_shared<string>("a ( );")),
                          lexer::Token(lexer::Token::CLOSE,
                                       ")",
                                       1,
                                       5,
                                       make_shared<string>(Module::directory.string() + "/test.cst"),
                                       make_shared<string>("a ( );")),
                          lexer::Token(lexer::Token::END_CMD,
                                       ";",
                                       1,
                                       6,
                                       make_shared<string>(Module::directory.string() + "/test.cst"),
                                       make_shared<string>("a ( );"))}))),
                 "BlaBla");
}*/

/*
void parser::noteInsert(string msg, lexer::Token after, string insert, uint32 code, bool before, string appendix){

    string location;
    location = ":"s + to_string(after.line) + ":" + to_string(after.column);

    std::cerr << "\r" << "\e[1;36mNote" << ": " << "\e[0m @ \e[0m" << after.filename << "\e[1m" << location << "\e[0m"
<< (code == 0? ""s : " [N"s + to_string(code) + "]") << ":" << std::endl; std::cerr << msg << std::endl; std::cerr << "
| " << std::endl; std::cerr << " " << fillup(to_string(after.line), 5) << " | " <<
(*(after.line_contents)).insert(after.column-1 + (before? 0 : after.value.size()), "\e[36m"s + insert + "\e[0m") <<
std::endl; std::cerr << "       | " << std::endl; std::cerr << appendix << std::endl;
}*/
