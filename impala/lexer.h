#ifndef IMPALA_LEXER_H
#define IMPALA_LEXER_H

#include "impala/compiler.h"

#include "thorin/util/debug.h"

#include "impala/token.h"

namespace impala {

class Lexer {
public:
    Lexer(Compiler& compiler, std::istream&, const char* filename);

    Token lex(); ///< Get next \p Token in stream.

    Compiler& compiler;

private:
    bool eof() const { return stream_.eof(); }
    void eat_comments();
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
    Loc loc() const { return {filename_, front_line_, front_col_, back_line_, back_col_}; }
    template<class... Args>
    std::ostream& error(const char* fmt, Args... args) { return compiler.error(loc(), fmt, std::forward<Args>(args)...); }

    std::istream& stream_;
    uint32_t peek_ = 0;
    char peek_bytes_[5] = {0, 0, 0, 0, 0};
    const char* filename_;
    std::string str_;
    uint32_t front_line_, front_col_;
    uint32_t  back_line_,  back_col_;
    uint32_t  peek_line_,  peek_col_;
    std::array<std::pair<Symbol, TT>, Num_Keywords> keywords_;
};

}

#endif
