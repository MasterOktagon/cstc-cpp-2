#include "string_functions.hpp"

string fillup(string in, uint32 len, char with) {
    while (in.size() < len) { in += with; }
    return in;
}

string intab(string i) {
    size_t pos = i.find('\n', 0);
    while (pos != string::npos) {
        i.replace(pos, 1, "\n\t");
        pos = i.find('\n', pos + 1);
    }
    return string("\t") + i;
}

string insert(string val, string target) {
    size_t pos = target.find_first_of("{}");
    if (pos != string::npos) { target.replace(pos, 2, val); }
    return target;
}

string rinsert(string val, string target) {
    size_t pos = target.rfind("{}");
    if (pos != string::npos) { target.replace(pos, 2, val); }
    return target;
}
