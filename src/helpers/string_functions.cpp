#include "string_functions.hpp"

string fillup(string in, uint32 len, char with) {
    while (in.size() < len) { in += with; }
    return in;
}

string intab(string in, string with) {
    usize pos = in.find("\n");
    while (pos != string::npos) { in.replace(pos, 1, with); }
    return in;
}