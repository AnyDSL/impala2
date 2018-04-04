#ifndef IMPALA_LEXER_H
#define IMPALA_LEXER_H

#include "impala/compiler.h"

#include "thorin/util/location.h"

#include "impala/token.h"

namespace impala {

class Lexer {
public:
    Lexer(Compiler& compiler, std::istream&, const char* filename);

    //@{ getters
    const char* filename() const { return filename_; }
    //@}

    Token lex(); ///< Get next \p Token in stream.

    Compiler& compiler;

private:
    Token parse_literal();

    template <typename Pred>
    std::optional<uint32_t> accept_opt(Pred pred) {
        if (pred(peek())) {
            auto ret = peek();
            next();
            return {ret};
        }
        return std::nullopt;
    }
    template <typename Pred>
    bool accept_if(Pred pred, bool append = true) {
        if (pred(peek())) {
            if (append) str_.append(peek_bytes_);
            next();
            return true;
        }
        return false;
    }
    bool accept(uint32_t val, bool append = true) {
        return accept_if([val] (uint32_t p) { return p == val; }, append);
    }
    bool accept(const char* p, bool append = true) {
        while (*p != '\0') {
            if (!accept(*p++, append)) return false;
        }
        return true;
    }

    uint32_t next();
    uint32_t peek() const { return peek_; }
    const std::string& str() const { return str_; }
    Location location() const { return {filename_, front_line_, front_col_, back_line_, back_col_}; }
    template<typename... Args>
    std::ostream& error(const char* fmt, Args... args) { return compiler.error(location(), fmt, std::forward<Args>(args)...); }

    std::istream& stream_;
    uint32_t peek_ = 0;
    char peek_bytes_[5] = {0, 0, 0, 0, 0};
    const char* filename_;
    std::string str_;
    uint32_t front_line_ = 1,
             front_col_  = 1,
             back_line_  = 1,
             back_col_   = 1,
             peek_line_  = 1,
             peek_col_   = 1;
};

}

#endif
