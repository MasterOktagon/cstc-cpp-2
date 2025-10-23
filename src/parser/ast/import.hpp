#pragma once

//
// IMPORT.hpp
//
// import ingoring & debugging AST
//

#include "../symboltable.hpp"
#include "ast.hpp"

/**
 * @namespace that implements the Import helper parsing (W.I.P.)
 */
namespace ImportAST {
   /**
     * @brief parses (and ignores) Import statements to allow debugging import statements
     */
    extern sptr<AST> parse(PARSER_FN);
}




