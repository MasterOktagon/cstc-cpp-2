//
// MAIN.cpp
//
// main porgram file
//

// #include "build/optimizer_flags.hpp"
#include "errors/errors.hpp"
#include "helpers/string_functions.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "module.hpp"
#include "snippets.hpp"
// #include "build/targets.hpp"
#include "../lib/argparse/include/argparse/argparse.hpp"

#include <filesystem>
#include <iostream>
#include <ostream>

// EXIT CODE NAMES
#define PROGRAM_EXIT      0
#define EXIT_ARG_FAILURE  1
#define EXIT_NO_MAIN_FILE 3

int32 main(int32 argc, const char** argv) {
    /**
     * @brief main function
     */

    // setup argument parsing
    argparse::ArgumentParser argparser("cstc"s, "c0.01"s, argparse::default_arguments::help);
    argparser.add_argument("file").help("main file of your program");
    argparser.add_argument("-l", "--list-modules").help("list all loaded modules").flag();
    argparser.add_argument("-1", "--one-error").help("exit at first error").flag();
    argparser.add_argument("-p", "--punish").help("treat warnings as errors").flag();
    argparser.add_argument("--version").help("display version info and exit").flag();
    argparser.add_argument("--max-line-len")
        .help("maximum line length before LTL warning (-1 to disable)")
        .scan<'d', int32>()
        .default_value<int32>(100);
    argparser.add_argument("--target").help("target for cross-compiler").default_value<string>("linux:x86:64:llvm");
    argparser.add_argument("--entrypoint").help("entrypoint function").default_value<string>("main");
    argparser.add_argument("--no-std-lang").help("disable autoloading lang module").flag();
    argparser.add_argument("--list-targets").help("list all available targets and exit").flag();
    argparser.add_argument("--opt").help("choose optimizer preset [none|disable|all]").default_value<string>("all");
    argparser.add_argument("--opt:constant-folding")
        .help("enable or disable constant folding optimization")
        .default_value<string>("true");
    argparser.add_argument("--opt:chaos")
        .help("enable or disable register chaos optimizer (good for pipelining) [W.I.P.]")
        .default_value<string>("true");

    // try to parse arguments
    try {
        argparser.parse_args(argc, argv);
    } catch (const exception& err) {
        cerr << err.what() << endl;
        cerr << argparser;
        return EXIT_ARG_FAILURE;
    }

    /*if (target::isValid(argparser.get("--target"))) {
        target::set(argparser.get("--target"));
    } else {
        cerr << "\e[1;31mERROR:\e[0m target '" << argparser.get("--target") << "' not found. Get a list of available
    targets with --list-targets. Defaulting to linux:x86:64:llvm" << endl; target::set("linux:x86:64:llvm");
    }

    // check optimizer features
    if (argparser["--opt"] == "all"s) {
        optimizer::do_constant_folding = true;
    }
    else if (argparser["--opt"] == "disable"s) {
        optimizer::do_constant_folding = true;
        optimizer::do_chaos = false;
    }
    else if (argparser["--opt"] == "none"s) {
        optimizer::do_constant_folding = false;
        optimizer::do_chaos = false;
    }
    else {
        cerr << "\e[1;31mERROR:\e[0m --opt only allows options 'all', 'disable' and 'none'. Defaulting to none." <<
    endl;
    }

    if (argparser["--opt:constant-folding"] == "true"s) {
        optimizer::do_constant_folding = true;
    }
    else if (argparser["--opt:constant-folding"] == "false"s) {
        optimizer::do_constant_folding = false;
    }
    else {
        cerr << "\e[1;31mERROR:\e[0m --opt:constant-folding only allows options 'true' and 'false'. Defaulting to true."
    << endl;
    }

    if (argparser["--opt:chaos"] == "true"s) {
        optimizer::do_chaos = true;
    }
    else if (argparser["--opt:chaos"] == "false"s) {
        optimizer::do_chaos = false;
    }
    else {
        cerr << "\e[1;31mERROR:\e[0m --opt:chaos only allows options 'true' and 'false'. Defaulting to true." << endl;
    }

    if (argparser["--list-targets"] == true) {
        target::list();
        exit(0);
    }*/

    // check for std environment variable
    if (Module::stdLibLocation() == "") {
        cerr << "\e[1;33mWARNING: \e[0m\e[1mCSTC_STD\e[0m environment variable could not be found.\n"
                "This may cause problems with std::* modules.\n"
             << endl;
    }

    if (argparser["--version"] == true) {
        cout << "CSTC v0.01 - cst25" << endl;

        exit(0);
    }

    parser::one_error  = argparser["-1"] == true;
    lexer::pretty_size = argparser.get<int32>("--max-line-len");
    if (lexer::pretty_size < -1) { lexer::pretty_size = -1; }

    // try to load the main file
    string main_file = argparser.get("file");
    if (!filesystem::exists(filesystem::u8path(main_file))) {
        cout << "\e[1;31mERROR:\e[0m main file at \e[1m"s + main_file + "\e[0m not found!" << endl;
        return EXIT_NO_MAIN_FILE;
    }
    // Load the main module (which autoloads all necessary modules)
    cout << "Fetching modules: (0/?)";
    if (argparser["--no-std-lang"] == false) {
        if (Module::create("lang", "", "", true) == nullptr) {
            cout << "\e[1;31mERROR:\e[0m \e[1mstd::lang\e[0m was not found! This may be a corruption of your standard "
                    "library"
                 << endl;
        }
    }
    Module::create(main_file, fs::current_path().string(), "", false, lexer::TokenStream::none(), true, true);

    // sort modules for parsing
    Module::modules.sort(Module::loadOrder);
    cout << "\r\e[32mFetching modules: (" << Module::known_modules.size() << "/"
         << Module::known_modules.size() + Module::unknown_modules.size() << ")\e[0m" << endl;

    if (argparser["-l"] == true) {
        // display a list of Modules loaded
        cout << "\e[36;1mINFO: " << Module::known_modules.size() << " Module"
             << (Module::known_modules.size() == 1 ? ""s : "s"s) << " loaded\e[0m" << endl;
        cout << "\t\e[36;1m[h]\e[0m - Header        \e[32;1m[m]\e[0m - Main file        \e[33;1m[s]\e[0m - STDlib      "
                "  \e[31;1m[t]\e[0m - target depending        \e[1m[l]\e[0m - autoload"
             << endl
             << endl;
        for (Module* m : Module::modules) { cout << "\t" << str(m) << endl; }
        for (string m : Module::unknown_modules) {
            cout << "\t\e[31m" << fillup(m, 60) << "missing" << "\e[0m" << endl;
        }
    }
    cout << endl;

    cout << "Parsing modules (0/" << Module::modules.size() << ")";

    for (Module* m : Module::modules) { m->parse(); }

    cout << "\r\e[32mParsing modules (" << Module::modules.size() << "/" << Module::modules.size() << ")\e[0m" << endl;

    if (parser::errc > 0 || parser::warnc > 0) {
        cout << "\n";
        cout << parser::errc << " error" << (parser::errc == 1 ? ", " : "s, ") << parser::warnc << " warning"
             << (parser::warnc == 1 ? "" : "s") << " generated\n";
        if (parser::warnc > 0 && argparser["-p"] == true) {
            cout << "Treating warnings as errors (--punish)\n\e[1;31mCompilation aborted\e[0m\n";
            exit(2);
        } else if (parser::errc > 0) {
            cout << "\e[1;31mCompilation aborted\e[0m\n";
            exit(2);
        }
    }
    
    cout << "Complete!" << endl;

    return PROGRAM_EXIT;
}

