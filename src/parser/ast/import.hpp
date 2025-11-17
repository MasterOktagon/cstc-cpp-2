#pragma once
#include "../../snippets.hpp"
#include "../symboltable.hpp"
#include "ast.hpp"

class ImportAST : public AST {
    public:
        /**
         * @brief parse an import statement (for debugging)
         *
         * @return Import AST or nullptr if not found
         */
        static sptr<AST> parse(PARSER_FN);
};

