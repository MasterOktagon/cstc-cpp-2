#pragma once

#include "../snippets.hpp"

#include <memory>
#include <optional>
#include <string>

using namespace std;
/*
namespace opt {

    class Location : public Repr {
        public:
            enum Base {
                STACK,
                REGISTER,
                STATIC,
            };

            Base  base   = STACK;
            int16 offset = 0;

        protected:
            string _str() const;

        public:
            Location(Base b, int16 offset) {
                this->base   = b;
                this->offset = offset;
            }
    };

    class RegisterManagerLayer : public Repr {
        public:
            struct Entry {
                    string content;
                    // string             type;
                    Location           reg;
                    optional<Location> loc     = {};
                    bool               cache   = false;
                    string             comment = "";

                    static Entry empty(int16 reg_num) {
                        return {"", Location(Location::REGISTER, reg_num), {}, false, "EMPTY"};
                    }
            };

        protected:
            string _str() const;

        private:
            uint16 ctr             = 0;
            uint16 register_amount = 32;

            struct Node {
                    Node*       next;
                    sptr<Entry> e;
            };

            void push(sptr<Entry>);
            Node* find(sptr<Entry>);

        public:
            struct LoadModifier {
                    string prefix_cmds;
                    string reg;
            };

            sptr<RegisterManagerLayer> parent_layer = nullptr;
            Node*                      registers    = nullptr;
            sptr<Entry>                back         = nullptr;

            void flush();

            LoadModifier getRegister(int16 reg_num);
            LoadModifier getContent(string content);
            void         setContent(string content, bool cache = false);
            void         setRegister(int16 reg_num, bool cache = false);
            void         callPrefix();
            void         returnPrefix();

            virtual ~RegisterManagerLayer();
    };
} // namespace opt
*/
