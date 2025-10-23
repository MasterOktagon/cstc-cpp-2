
//
// AST.cpp
//
// implements the default ASTs functionality
//

#include "ast.hpp"

#include "../../snippets.hpp"

string AST::_str() const {
    return "<AST>";
}

CstType AST::getCstType() const {
    return "@unknown"_c;
}

void AST::consume(CstType type) {
}

CstType AST::provide() {
    return "@unknown"_c;
}

string AST::emitCST() const {
    return "";
}
