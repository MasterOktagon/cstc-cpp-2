#pragma once

//
// MODULE.hpp
//
// layouts the module class
//
#include "lexer/token.hpp"
#include "parser/symboltable.hpp"

#include <filesystem>
#include <list>
#include <map>
#include <optional>
#include <vector>

namespace std {
    namespace fs = filesystem;
}

///
/// \class Module holds all information and contents of a Program module
///
class Module final : public symbol::Namespace {
        bool                 is_main_file = false;                  //> whether this is the main module
        bool                 is_stdlib    = false;                  //> whether this is a stdlib module
        map<string, Module*> deps         = {};                     //> dependency modules
        lexer::TokenStream   tokens       = lexer::TokenStream({}); //> this module's tokens

    protected:
        /**
         * @brief get a visual representation of this Object
         */
        string _str() const;

    public:
        string          module_name; //> representation module name
        static fs::path directory;   //> main program directory
        fs::path        hst_file;    //> header location (relative)
        fs::path        cst_file;    //> source location (relative)

        bool isHeader() const;
        bool isKnown() const;

        const string getName() const { return "Module"; }

        /**
         * @brief tokenize this module and parse for imports to include them
         */
        void preprocess();

        /**
         * @brief parse this module and create AST nodes
         */
        void parse();

        Module(string path, string dir, string name, bool is_stdlib = false, bool is_main_file = false);

        static map<string, Module*>
            known_modules; //> A map of all compile-time known modules to allow faster acces when importing
        static list<string>
            unknown_modules;          //> A map of all compile-time unknown modules to allow better error messages
        static list<Module*> modules; //> A list of all modules loaded. This is used to determine the compile order

        /**
         * @brief get the default stdlib location using the CSTC_STD environment variable
         */
        static string stdLibLocation();

        /**
         * @brief convert a path to a module name
         */
        static string path2Mod(string path);

        /**
         * @brief convert a module name to a path
         */
        static string mod2Path(string path);

        /**
         * @brief compare two modules about their load order
         *
         * @return true, if a should be loaded before b
         */
        static bool loadOrder(Module* a, Module* b);

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
        static Module* create(string               path,
                              string               dir,
                              string               overpath,
                              bool                 is_stdlib,
                              vector<lexer::Token> tokens       = {},
                              bool                 is_main_file = false,
                              bool                 from_path    = false);
};

/**
 * @brief get a symbol list for an import. Meta-function. Prefer not to use.
 */
extern optional<vector<string>> getImportList(lexer::TokenStream);

