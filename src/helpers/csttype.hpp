

#include "../snippets.hpp"

#include <string>

using namespace std;

/// \class that represents a C* type
///
class CstType : public string {

    public:
        /// \brief create a CstType
        ///
        CstType() { CstType(""); }

        /// \brief create a CstType
        ///
        CstType(string s) { *this = s; }

        bool operator==(string other) { return *this == CstType(other); }

        bool operator==(CstType other) {
            string a = *this;
            string b = other;

            if (a == "@unknown" || b == "@unknown") {
                return true; // @unknown should ONLY be used for temporarily unknown types
            }
            if (a == b) { return true; }
            if (a == "@int") {
                return (b == "int8" || b == "int16" || b == "int32" || b == "int64" || b == "ssize" || b == "uint8" ||
                        b == "uint16" || b == "uint32" || b == "uint64" || b == "usize" || b == "@uint");
            }
            if (b == "@int") {
                return (a == "int8" || a == "int16" || a == "int32" || a == "int64" || a == "ssize" || a == "uint8" ||
                        a == "uint16" || a == "uint32" || a == "uint64" || a == "usize" || a == "@uint");
            }
            if (a == "@uint") {
                return b == "uint8" || b == "uint16" || b == "uint32" || b == "uint64" || b == "usize" || b == "@uint";
            }
            if (b == "@uint") {
                return a == "uint8" || a == "uint16" || a == "uint32" || a == "uint64" || a == "usize" || a == "@uint";
            }
            if (a == "@float") { return b == "float16" || b == "float32" || b == "float64"; }
            if (b == "@float") { return a == "float16" || a == "float32" || a == "float64"; }

            return false;
        }

        bool operator!=(CstType other) { return !(*this == other); }

        inline string toString() const { return string(*this); }
};

inline CstType operator""_c(const char* a, usize) {
    return CstType(a);
}

