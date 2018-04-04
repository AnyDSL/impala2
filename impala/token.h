#ifndef IMPALA_TOKEN_H
#define IMPALA_TOKEN_H

#include <string>

#include "thorin/util/location.h"
#include "thorin/util/symbol.h"
#include "thorin/util/types.h"
#include "thorin/util/utility.h"

namespace thorin::fe {

#define IMPALAP_ARG_TOKENS(f) \
    f(L_Brace,          "{") \
    f(L_Paren,          "(") \
    f(L_Bracket,        "[") \
    f(L_Angle,          "<") \
    f(QualifierU,       "ᵁ") \
    f(QualifierR,       "ᴿ") \
    f(QualifierA,       "ᴬ") \
    f(QualifierL,       "ᴸ") \
    f(Backslash,        "\\") \
    f(Pi,               "\\pi") \
    f(Lambda,           "\\lambda") \
    f(Bool,             "bool") \
    f(Identifier,       "identifier") \
    f(Literal,          "literal")

#define IMPALART_TOKENS(f) \
    f(Arity_Kind,       "\\arity_kind") \
    f(Multi_Arity_Kind, "\\multi_arity_kind") \
    f(Qualifier_Type,   "\\qualifier_type") \
    f(Star,             "*")

#define IMPALA_TOKENS(f) \
    f(R_Brace,          "}") \
    f(R_Paren,          ")") \
    f(R_Bracket,        "]") \
    f(R_Angle,          ">") \
    f(Colon,            ":") \
    f(ColonColon,       "::") \
    f(ColonEqual,       ":=") \
    f(Comma,            ",") \
    f(Dot,              ".") \
    f(Equal,            "=") \
    f(Semicolon,        ";") \
    f(Sharp,            "#") \
    f(Arrow,            "<-") \
    f(Cn,               "cn") \
    f(Eof,              "eof")

struct Literal {
    enum class Tag {
#define CODE(T) Lit_##T,
        IMPALA_TYPES(CODE)
#undef CODE
        Lit_arity,
        Lit_index,
        Lit_index_arity,
        Lit_untyped
    };

    Tag tag;
    Box box;

    Literal() {}
    Literal(Tag tag, Box box)
        : tag(tag), box(box)
    {}
};

class Token {
public:
    enum class Tag {
#define CODE(T, S) T,
        IMPALA_APP_ARG_TOKENS(CODE)
        IMPALA_SORT_TOKENS(CODE)
        IMPALA_OP_TOKENS(CODE)
#undef CODE
    };

    Token() {}
    Token(Location loc, Literal lit)
        : tag_(Tag::Literal)
        , location_(loc)
        , literal_(lit)
    {}
    Token(Location loc, const std::string& identifier)
        : tag_(Tag::Identifier)
        , location_(loc)
        , symbol_(identifier)
    {}
    Token(Location loc, Tag tag)
        : tag_(tag)
        , location_(loc)
    {}

    Tag tag() const { return tag_; }
    Literal literal() const { return literal_; }
    Symbol symbol() const { return symbol_; }
    Location location() const { return location_; }

    bool isa(Tag tag) const { return tag_ == tag; }
    bool is_app_arg() const {
        switch (tag_) {
#define CODE(T, S) case Tag::T:
            IMPALA_APP_ARG_TOKENS(CODE)
#undef CODE
                return true;
            default: return false;
        }
    }

    static std::string tag_to_string(Tag tag) {
        switch (tag) {
#define CODE(T, S) case Tag::T: return S;
            IMPALA_APP_ARG_TOKENS(CODE)
            IMPALA_SORT_TOKENS(CODE)
            IMPALA_OP_TOKENS(CODE)
#undef CODE
            default: IMPALA_UNREACHABLE;
        }
    }

private:
    Tag tag_;
    Location location_;

    Literal literal_;
    Symbol symbol_;
};

std::ostream& operator<<(std::ostream& os, const Token& t);

}

#endif // TOKEN_H
