
//
// MODULE.cpp
//
// implements the module class
//

#include "module.hpp"

#include "debug.hpp"
#include "errors/errors.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
// #include "parser/ast/ast.hpp"
#include "helpers/string_functions.hpp"
// #include "parser/ast/flow.hpp"
#include "parser/symboltable.hpp"
#include "snippets.hpp"

#include <algorithm>
#include <asm-generic/errno.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
// #include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

map<string, Module*> Module::known_modules =
    {}; ///< A map of all compile-time known modules to allow faster acces when importing
list<string> Module::unknown_modules = {}; ///< A map of all compile-time unknown modules to allow better error messages
list<Module*> Module::modules = {}; ///< A list of all modules loaded. This is used to determine the compile order
fs::path      Module::directory;    ///< Project directory

uint64 parsed_modules = 0; ///< amount of parsed modules

/**
 * @brief get the default stdlib location using the CSTC_STD environment variable
 */
string Module::stdLibLocation() {
    if (const char* p = getenv("CSTC_STD")) { return p; }
    return "";
}

/**
 * @brief replace the ~ for the home path using the HOME environment variable
 */
inline string h(string s) {
    if (s[0] == '~') { return string(getenv("HOME")) + s.substr(1); }
    return s;
}

bool Module::isHeader() const {
    return fs::exists(hst_file);
}

bool Module::isKnown() const {
    return fs::exists(cst_file);
}

/**
 * @brief return an imported module. This method checks if there already is a module of this name
 * and reuses it if possible
 *
 * @param path path to check for can be a "real path" or an C* import module path: @see @param from_path
 * @param dir  directory in which the callee is. currently unused
 * @param overpath path of the callee
 * @param is_stdlib depicts if this file is a stdlib file (for looking in CSTC_STD path)
 * @param tokens tokens of the preprocessor-parser, used for error messages
 * @param is_main_file whether this is the main file. should not be set outside the main function.
 * @param from_path create this from a "real path" instead of a module
 *
 * @return Newly created Module (Pointer) if succesful or nullptr
 */
Module* Module::create(string path,
                       string,
                       string               overpath,
                       bool                 is_stdlib,
                       vector<lexer::Token> tokens,
                       bool                 is_main_file,
                       bool                 from_path) {
    string module_name = "<unknown>";
    if (!from_path) { path = mod2Path(path); }
    usize pos = path.rfind(".");
    if (pos != string::npos) { path = path.substr(0, pos); }
    pos = overpath.rfind(".");
    if (pos != string::npos) { overpath = overpath.substr(0, pos); }

    fs::path directory = fs::current_path();
    if (is_stdlib) {
        directory   = fs::path(h(stdLibLocation()));
        module_name = path2Mod(path);
    } else {
        if (!from_path) {
            directory = fs::path(overpath).parent_path();
            module_name =
                path2Mod(fs::relative(fs::path(overpath).parent_path(), fs::current_path()).string() + "/" + path);
        } else {
            module_name = path2Mod(path);
        }
    }

    ////cout << directory << endl;
    ////cout << path << endl;
    ////cout << module_name << endl;

    if (known_modules.count(module_name) > 0) { return known_modules[module_name]; }
    if (fs::exists(directory.string() + "/" + path + ".hst")) {
        if (!fs::exists(directory.string() + "/" + path + ".cst")) {
            parser::warn(parser::warnings["No implementation file found"],
                         tokens,
                         "Missing an implementation file (\".cst\") @ "_s + directory.string() + "/" + path);
        }
        known_modules[module_name] = new Module(path, directory.string(), module_name, is_stdlib, is_main_file);
        modules.push_back(known_modules[module_name]);
        return known_modules[module_name];
    }
    if (fs::exists(directory.string() + "/" + path + ".cst")) {
        known_modules[module_name] = new Module(path, directory.string(), module_name, is_stdlib, is_main_file);
        modules.push_back(known_modules[module_name]);
        return known_modules[module_name];
    }
    if (find(unknown_modules.begin(), unknown_modules.end(), module_name) == unknown_modules.end()) {
        parser::error(parser::errors["Module not found"],
                      vector<lexer::Token>(tokens.begin() + 1, tokens.end() - 1),
                      "A module at "_s + directory.string() + "/" + path + " was not found");
        unknown_modules.push_back(module_name);
    }

    return nullptr;
}

Module::Module(string path, string dir, string module_name, bool is_stdlib, bool is_main_file) {
    loc = "";
    for (uint64 i = 0; i < module_name.size(); i++) {
        if (i < module_name.size() - 2 && module_name[i] == ':' && module_name[i + 1] == ':') {
            loc += "::";
            i++;
        } else {
            loc += module_name[i];
        }
    }

    this->is_main_file = is_main_file;
    this->is_stdlib    = is_stdlib;
    this->module_name  = module_name;
    ////cout << "name: " <<  this->module_name << endl;

    directory = fs::path(dir);
    cst_file  = fs::path(dir + "/" + path + ".cst");
    hst_file  = fs::path(dir + "/" + path + ".hst");

    cout << "\rFetching modules: (" << known_modules.size() + 1 << "/?)";
    if (module_name != "lang" && known_modules.count("lang") > 0) { include.push_back(known_modules["lang"]); }

    preprocess();
}

/**
 * @brief convert a path to a module name
 */
string Module::path2Mod(string path) {
    string out;
    usize  pos = path.find("/");
    while (pos != string::npos) {
        out  += path.substr(0, pos) + "::";
        path  = path.substr(pos + 1);
        pos   = path.find("/");
    }
    out += path;
    return out;
}

/**
 * @brief convert a module name to a path
 */
string Module::mod2Path(string mod) {
    string out;

    for (uint64 i = 0; i < mod.size(); i++) {
        if (i < mod.size() - 2 && mod[i] == ':' && mod[i + 1] == ':') {
            out += '/';
            i++;
        } else {
            out += mod[i];
        }
    }
    return out;
}

/**
 * @brief get a visual representation of this Object
 */
string Module::_str() const {
    return fillup("\e[1m"s + module_name + "\e[0m", 50) + (isHeader() ? "\e[36;1m[h]\e[0m"s : "   "s) +
           (is_main_file ? "\e[32;1m[m]\e[0m"s : "   "s) + (is_stdlib ? "\e[33;1m[s]\e[0m"s : "   "s) +
           (module_name == "lang" ? "\e[1m[l]\e[0m"s : "   "s) + " @ \e]8;;file://" + (cst_file.string()) + "\e\\" +
           (cst_file.string()) + "\e]8;;\e\\";
}

/**
 * @brief compare two modules about their load order
 *
 * @return true, if a should be loaded before b
 */
bool Module::loadOrder(Module* a, Module* b) {
    for (pair<string, Module*> p : a->deps) {
        if (p.second == b) { return false; }
    }
    for (symbol::Namespace* m : a->include) {
        if (m == b) { return false; }
    }
    return true;
}

/**
 * @brief get a symbol list for an import. Meta-function. Prefer not to use.
 */
optional<vector<string>> getImportList(lexer::TokenStream t) {
    if (t.size() == 0) { return {}; }
    vector<string> out = {};

    lexer::Token::Type last = lexer::Token::Type::COMMA;

    for (lexer::Token tok : *t.tokens) {
        if (tok.type == lexer::Token::Type::COMMA) {};
        if (tok.type == lexer::Token::Type::SYMBOL) {
            if (last == lexer::Token::Type::COMMA) {
                out.push_back(tok.value);
            } else {
                return {};
            }
        } else {
            return {};
        }
        last = tok.type;
    }
    return out;
}

/**
 * @brief tokenize this module and parse for imports to include them
 */
void Module::preprocess() {
    ifstream f(cst_file.string());
    string   content = string(istreambuf_iterator<char>(f), istreambuf_iterator<char>());

    tokens             = lexer::tokenize(content, cst_file);
    usize macro_passes = 0;

    usize macros_edited = 1;
    usize cmd_begin     = 0;
    while (macros_edited) {
        macros_edited = 0;

        for (usize i = 0; i < tokens.size(); i++) {
            if (i < tokens.size() - 1) {
                if (tokens[i].type == lexer::Token::INCLUDE and tokens[i + 1].type == lexer::Token::STRING) {
                    std::fs::path include_file_path =
                        std::fs::path(directory.string() + "/" + mod2Path(module_name)).parent_path();
                    include_file_path += "/"_s + tokens[i + 1].value.substr(1, tokens[i + 1].value.size() - 2);
                    if (fs::exists(include_file_path)) {
                        DEBUG(4, "including: "_s + include_file_path.string());
                        ifstream           f(include_file_path.string());
                        string             c = string(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
                        lexer::TokenStream new_tokens = lexer::tokenize(c);
                        tokens.cut(i, i + 2);
                        tokens.include(new_tokens, i, include_file_path.string());

                        f.close();
                    } else {
                        parser::error(parser::errors["File not found"],
                                      tokens.slice(i, i + 2),
                                      "file at "_s + include_file_path.string() + " was not found!");
                        tokens.cut(i, i + 2);
                    }
                    macros_edited++;
                }
            }
            if (tokens[i].type == lexer::Token::BLOCK_OPEN or tokens[i].type == lexer::Token::BLOCK_CLOSE) {
                cmd_begin = i + 1;
            }
            if (tokens[i].type == lexer::Token::END_CMD) {
                lexer::TokenStream cmd = tokens.slice(cmd_begin, i);
                DEBUG(2, str(cmd));

                if (cmd[0].type == lexer::Token::IMPORT) {
                    lexer::TokenStream import_content = cmd.slice(1, cmd.size());

                    string alias = "";

                    lexer::TokenStream::Match m = import_content.splitStack({lexer::Token::AS});
                    if (m.found()){
                        DEBUG(5, "import as found at "_s + to_string(m));
                        lexer::TokenStream alias_stream = m.after();
                        import_content = m.before();
                        if (alias_stream.size() == 1 and alias_stream[0].type == lexer::Token::SYMBOL){
                            alias = alias_stream[0].value;
                            DEBUG(3, "import alias: "_s + alias);
                        }
                    }

                    vector<lexer::TokenStream> parts = import_content.list({lexer::Token::SUBNS});
                    if (parts.size() > 0) {
                        DEBUG(3, "import parts: "_s + to_string(parts.size()));
                        string         modname;
                        vector<string> includes;
                        bool           break_case = false;

                        for (usize j = 0; j < parts.size() - 1; j++) {
                            if (parts[parts.size() - 1].size() == 1) {
                                if (parts[parts.size() - 1][0].type == lexer::Token::SYMBOL ||
                                    parts[parts.size() - 1][0].type == lexer::Token::DOTDOT) {
                                    modname += parts[parts.size() - 1][0].value + "::";
                                }
                            } else {
                                break_case = true;
                            }
                        }
                        if (!break_case or parts.size() == 1) {
                            lexer::TokenStream t = parts[parts.size() - 1];
                            DEBUG(5, "import final part: "_s + str(t));
                            if (t.size() == 1) {
                                if (t[0].type == lexer::Token::SYMBOL) { modname += t[0].value; }
                            } else if (t.size() >= 3) {
                                if (t[0].type == lexer::Token::IN and t[1].type == lexer::Token::BLOCK_OPEN and
                                    t[-1].type == lexer::Token::BLOCK_CLOSE) {
                                    if (modname != "") { modname = modname.substr(2); }
                                }
                            }
                            if (modname != "") {
                                DEBUG(2, "modname: "_s + modname);
                                Module* m = Module::create(modname, "", cst_file, false, *cmd.tokens);
                                if (m != nullptr) { add(alias == ""? modname : alias, m); }
                            }
                        }
                    }
                }

                cmd_begin = i + 1;
            }
        }
        macro_passes++;
    }
    f.close();
    DEBUG(3, "preprocessor: "_s + fillup(module_name, 50) + " - macro passes:" + to_string(macro_passes));
}

/**
 * @brief parse this module and create AST nodes
 */
void Module::parse() {
    /*sptr<AST> root = SubBlockAST::parse(tokens, 0, this);
    if (root != nullptr) {
        int* i = new int;
        *i     = 0;
        // cout << str(root.get()) << endl;
        cout << root->emitCST() << endl;

        delete i;
    }*/
    cout << "\rParsing modules (" << ++parsed_modules << "/" << Module::modules.size() << ")";
}

