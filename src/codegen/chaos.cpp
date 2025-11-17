
#include "chaos.hpp"
#include <string>
#include <sstream>

using namespace std;
/*
string opt::Location::_str() const {
    string out;
    switch (base) {
        case STATIC :
            out = "STATIC";
            break;
        case STACK :
            out = "STACK";
            break;
        case REGISTER :
            out = "r";
            break;
    }

    if (base == REGISTER) {
        return out + to_string(offset);
    }
    if (offset >= 0) {
        stringstream stream;
        stream << hex << offset;
        out += "+0x"_s + stream.str();
    } else {
        stringstream stream;
        stream << hex << -offset;
        out += "-0x"_s + stream.str();
    }

    return out;
}

string opt::RegisterManagerLayer::_str() const {
    return "TODO";
}

void opt::RegisterManagerLayer::push(sptr<Entry> e) {
    Node* n = new Node {registers, e};

    registers = n;
}*/
