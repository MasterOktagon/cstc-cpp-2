
#include "symboltable.hpp"

#include "../debug.hpp"
#include "../errors/errors.hpp"
#include "../snippets.hpp"

#include <map>
#include <string>
#include <vector>

void symbol::Namespace::add(string loc, symbol::Reference* sr) {
    usize pos  = loc.find("::");
    sr->parent = this;
    if (pos != string::npos && loc.substr(0, pos) != "" && dynamic_cast<Namespace*>(sr) == sr) {
        ((Namespace*) (contents.at(loc.substr(0, pos))[0]))->add(loc.substr(pos + 2), sr);
    } else {
        if (contents.count(loc) == 0) { contents[loc] = {}; }
        contents.at(loc).push_back(sr);
    }

    // debug(str(sr) + " added at "s + loc.substr(0,pos), 3);
}

symbol::Reference::~Reference() = default;

CstType symbol::Function::getCstType() {
    string s = "["s + type;
    if (parameters.size() > 0) {
        s += "<-";
        for (CstType t : parameters) { s += t + ","; }
        if (s.at(s.size() - 1) == ',') { s = s.substr(0, s.size() - 1); }
    }
    s += "]";
    return s;
}

symbol::Namespace::~Namespace() {
    for (std::pair<string, std::vector<Reference*>> v : contents) {
        for (Reference* t : v.second) { delete t; }
    }
}

std::vector<symbol::Reference*> symbol::Namespace::operator[](string subloc) {
    auto result = getLocal(subloc);
    for (uint64 i = 0; result.size() == 0 && i < include.size(); i++) { result = (*include.at(i))[subloc]; }
    return result;
}

std::vector<symbol::Reference*> symbol::Namespace::getLocal(string subloc) {
    if (subloc == "") { return {this}; }
    if (contents.count(subloc) > 0) { return contents[subloc]; }

    std::vector<Reference*> result;

    usize pos = subloc.find("::");
    if (pos != string::npos) {
        string head = subloc.substr(0, pos);
        string tail = subloc.substr(pos + 2, subloc.size() - pos - 2);
        if (contents.count(head) == 0) {
            result = {};
        } else if ((Namespace*) contents[head][0] != dynamic_cast<Namespace*>(contents[head][0])) {
            result = {};
        } else {
            result = (*((Namespace*) contents[head][0]))[tail];
        }
    }
    if (result.size() == 0 && import_from.count(subloc) > 0) { result = (*this)[import_from[subloc]]; }
    return result;
}

symbol::Namespace::LinearitySnapshot symbol::Namespace::snapshot() const {
    LinearitySnapshot l({});
    for (auto s : contents) {
        auto a   = dynamic_cast<symbol::Variable*>(s.second[0]);
        auto var = (symbol::Variable*) (s.second[0]);
        DEBUG(7, "snapshot: var? "s + std::to_string(a == var));
        if (a == var && !var->is_free) {
            DEBUG(7, "snapshot: ["s + var->getVarName() + "] used: "s + std::to_string(var->status));
            l.data[var] = var->status;
        }
    }

    return l;
}

bool symbol::Namespace::LinearitySnapshot::operator==(LinearitySnapshot ls) const {
    for (auto s : data) {
        if (ls.data[s.first] != s.second) { return false; }
    }
    return true;
}

string getStatusName(symbol::Variable::Status s) {
    switch (s) {
        default                                      : return "unknown";
        case symbol::Variable::Status::CONSUMED      : return "consumed";
        case symbol::Variable::Status::PROVIDED      : return "provided";
        case symbol::Variable::Status::UNINITIALIZED : return "uninitialized";
        case symbol::Variable::Status::BORROWED      : return "borrowed";
    }
}

void symbol::Namespace::LinearitySnapshot::traceback(LinearitySnapshot ls) const {
    for (auto s : data) {
        if (ls.data[s.first] != s.second) {
            parser::note(s.first->last,
                         "Variable \e[1m" + s.first->getVarName() + "\e[0m was " + getStatusName(s.second) +
                             " but is " + getStatusName(ls.data[s.first]) + " now.");
        }
    }
}

symbol::Function::Function(symbol::Reference* parent, string name, lexer::TokenStream tokens, CstType type) {
    this->tokens = tokens.tokens;
    this->loc    = name;
    this->parent = parent;
    this->type   = type;

    ALLOWS_NON_STATIC  = true;
    ALLOWS_EXPRESSIONS = true;
}

