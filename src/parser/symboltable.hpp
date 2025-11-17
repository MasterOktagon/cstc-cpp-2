#pragma once

#include "../lexer/token.hpp"
#include "../snippets.hpp"
#include "ast/ast.hpp"

#include <map>
#include <optional>
#include <utility>
#include <vector>

template <typename T, typename S>
using MultiMap = map<T, vector<S>>;

///
/// \namespace that holds datastructures related to data
///
namespace symbol {
    class Namespace;

    ///
    /// \class represents a symbol that can be referenced
    ///
    class Reference : public Repr {
        protected:
            string loc;

            ///
            /// \brief represent this object
            ///
            virtual string _str() const { return "symbol::Reference"s; }

        public:
            lexer::TokenStream tokens = lexer::TokenStream({}); ///< tokens, where this symbol was found
            lexer::TokenStream last   = lexer::TokenStream({}); ///< tokens, where this symbol was last used
            Reference*         parent = nullptr;                ///< (optional) parent symbol

            ///
            /// \brief destroy a reference
            ///
            virtual ~Reference();

            ///
            /// \brief get the return type from this symbol or @unknown if not applicable
            ///
            virtual CstType getReturnType() const { return "@unknown"_c; }

            ///
            /// \brief get symbols location in namspace hierachy as a string
            ///
            string getLoc() const {
                if (parent == nullptr) { return loc; }
                return parent->getLoc() + "::"s + loc;
            }

            ///
            /// \brief get symbols location relative to parent
            ///
            virtual string getRelLoc() { return loc; }

            ///
            /// \brief how many bytes in memory
            ///
            virtual usize sizeBytes() { return 0; }

            virtual const string getName() const abstract;
    };

    /*class Container : public Reference {
        ///
        /// \brief try to add an object to this
        ///
        virtual void add(string loc, Reference* sr) abstract;
        virtual std::vector<symbol::Reference*> get(string) abstract;
        virtual std::vector<symbol::Reference*> operator[](string name) {return get(name);}
    };*/

    class Instantiatable {
            ///
            /// \brief get the CstType represented by this symbol, or void if not applicable
            ///
            virtual CstType getCstType() { return "void"_c; }
    };

    class Variable : public Reference {
            /**
             * Reference that holds data about a Variable
             */

            CstType type = ""_c;
            string  name;

        public:
            enum Status {
                UNINITIALIZED = 0,
                PROVIDED      = 1,
                CONSUMED      = 2,
                BORROWED      = 3,
            };

            bool             is_const   = false;
            bool             is_mutable = false;
            bool             is_static  = false;
            Status           status     = UNINITIALIZED;
            bool             is_free    = false;
            optional<string> const_value;

            Variable(string name, CstType type, lexer::TokenStream tokens, symbol::Reference* parent) {
                loc          = name;
                this->tokens = tokens;
                last         = tokens;
                this->parent = parent;
                this->name   = name;
                this->type   = type;
            }

            virtual ~Variable() {};

            virtual string _str() const { return "symbol::Variable "s + getLoc(); }

            string getVarName() const { return name; }

            virtual void add(string, Reference*) {}

            virtual CstType getCstType() { return type; }

            const string getName() const { return "Variable"; };
    };

    class Namespace : public Reference {
            /**
             * Reference that can hold other references
             */

        protected:
            std::map<string, string> import_from = {}; //> import-from map

            virtual string _str() const { return "symbol::Namespace "s + getLoc(); }

        public:
            std::vector<Namespace*>      include {};
            MultiMap<string, Reference*> contents     = {};
            std::vector<string>          unknown_vars = {};
            virtual void                 add(string loc, Reference* sr);
            Namespace() = default;

            Namespace(string loc) { this->loc = loc; };

            virtual ~Namespace();

            bool ALLOWS_VAR_DECL    = false;
            bool ALLOWS_VAR_SET     = false;
            bool ALLOWS_VISIBILITY  = false;
            bool ALLOWS_VIRTUAL     = false;
            bool ALLOWS_EXPRESSIONS = false;
            bool ALLOWS_FUNCTIONS   = false;
            bool ALLOWS_SUBBLOCKS   = false;
            bool ALLOWS_SUBCLASSES  = true;
            bool ALLOWS_INIT        = false;
            bool ALLOWS_INIT_CONST  = false;
            bool ALLOWS_CONST       = false;
            bool ALLOWS_STATIC      = true;
            bool ALLOWS_ENUMS       = false;
            bool ALLOWS_NON_STATIC  = false;

            virtual std::vector<symbol::Reference*> operator[](string subloc);
            virtual std::vector<symbol::Reference*> getLocal(string subloc);

            const string getName() const { return "Namespace"; }

            class LinearitySnapshot : public Repr {
                    std::map<symbol::Variable*, Variable::Status> data;
                    friend class Namespace;

                protected:
                    LinearitySnapshot(std::map<Variable*, Variable::Status> idata) { idata = data; };

                    string _str() const {
                        string s = "[";
                        for (auto d : data) {
                            s += d.first->getVarName() + "=" +
                                 std::vector<string>({"UNINITIALIZED", "PROVIDED", "CONSUMED"})[d.second] + ", ";
                        }
                        s += "]";
                        return s;
                    }

                public:
                    bool operator==(LinearitySnapshot ls) const;

                    bool operator!=(LinearitySnapshot ls) const { return !(*this == ls); }

                    void traceback(LinearitySnapshot ls) const;
            };

            LinearitySnapshot snapshot() const;
    };

    class SubBlock : public Namespace {
            string name;

        public:
            SubBlock(symbol::Namespace* copyFrom) {
                ALLOWS_VAR_DECL    = copyFrom->ALLOWS_VAR_DECL;
                ALLOWS_VAR_SET     = copyFrom->ALLOWS_VAR_SET;
                ALLOWS_VISIBILITY  = copyFrom->ALLOWS_VISIBILITY;
                ALLOWS_VIRTUAL     = copyFrom->ALLOWS_VIRTUAL;
                ALLOWS_EXPRESSIONS = copyFrom->ALLOWS_EXPRESSIONS;
                ALLOWS_FUNCTIONS   = copyFrom->ALLOWS_FUNCTIONS;
                ALLOWS_SUBBLOCKS   = copyFrom->ALLOWS_SUBBLOCKS;
                ALLOWS_SUBCLASSES  = copyFrom->ALLOWS_SUBCLASSES;
                ALLOWS_INIT        = copyFrom->ALLOWS_INIT;
                ALLOWS_INIT_CONST  = copyFrom->ALLOWS_INIT_CONST;
                ALLOWS_CONST       = copyFrom->ALLOWS_CONST;
                ALLOWS_STATIC      = copyFrom->ALLOWS_STATIC;
                ALLOWS_ENUMS       = copyFrom->ALLOWS_ENUMS;
                ALLOWS_NON_STATIC  = copyFrom->ALLOWS_NON_STATIC;

                name   = copyFrom->getName();
                parent = copyFrom;
            }

            const string getName() const { return name; };

            CstType getReturnType() const { return parent->getReturnType(); }
    };

    class Function : public Namespace {
            /**
             * Reference that represents a function
             */

            string  name;
            CstType type = ""_c;

        protected:
            virtual string _str() const { return "symbol::Function "s + getLoc(); }

        public:
            std::vector<CstType>                            parameters;
            std::map<string, std::pair<CstType, sptr<AST>>> name_parameters;
            bool                                            is_method = false;
            bool                                            is_lvalue = false;

            enum Visibility {
                PUBLIC,
                PRIVATE,
                PROTECTED,
                GUARDED,
            };

            Visibility visibility = GUARDED;

            Function(symbol::Reference* parent, string name, lexer::TokenStream tokens, CstType type);

            // string getLLName() { return getLLType() + (is_method ? "mthd."s : "fn."s) + name; }

            CstType getCstType();

            CstType getReturnType() const { return type; }

            virtual ~Function() { Namespace::~Namespace(); };

            virtual usize sizeBytes() { return 8; }

            const string getName() const { return "Function"; };
    };

    class Struct : public Namespace, Instantiatable {
            /**
             * Reference that holds data for a composite type
             */

        protected:
            virtual string _str() const { return "symbol::Struct "s + getLoc(); }

        public:
            virtual ~Struct() {}

            Struct(const string& name, lexer::TokenStream tokens) {
                loc          = name;
                this->tokens = tokens;

                ALLOWS_INIT_CONST = true;
                ALLOWS_VAR_DECL   = true;
                ALLOWS_STATIC     = true;
            }

            usize sizeBytes() {
                usize s = 0;
                for (std::pair<string, std::vector<Reference*>> rs : contents) {
                    for (Reference* r : rs.second) { s += r->sizeBytes(); }
                }
                return s;
            }

            const string getName() const { return "Struct"; }
    };

    class EnumEntry : public Reference {
            virtual ~EnumEntry() = default;

            EnumEntry(string name) { loc = name; }
    };

    class Enum : public Namespace {
            /**
             * Reference that holds data for an Enumeration type
             */

        protected:
            uint64 default_value = 0;
            bool   has_default   = false;
            uint32 bits          = 8;

        public:
            bool needs_stringify   = false;
            bool needs_from_string = false;

            CstType getCstType() { return getLoc(); }

            const string getName() const { return "Enumeration"; }

            virtual ~Enum() {
                ALLOWS_ENUMS      = true;
                ALLOWS_SUBCLASSES = false;
            }

            Enum(string name) { loc = name; }
    };
} // namespace symbol
