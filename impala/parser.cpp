#include "impala/parser.h"

#include <sstream>

#define PTRN \
         Token::Tag::M_id: \
    case Token::Tag::D_l_paren

#define ITEM \
         Token::Tag::K_enum: \
    case Token::Tag::K_extern: \
    case Token::Tag::K_fn: \
    case Token::Tag::K_impl: \
    case Token::Tag::K_mod: \
    case Token::Tag::K_static: \
    case Token::Tag::K_struct: \
    case Token::Tag::K_typedef: \
    case Token::Tag::K_trait

#define EXPR \
         Token::Tag::D_l_brace: \
    case Token::Tag::D_l_bracket: \
    case Token::Tag::D_l_paren: \
    case Token::Tag::K_Cn: \
    case Token::Tag::K_Fn: \
    case Token::Tag::K_cn: \
    case Token::Tag::K_false: \
    case Token::Tag::K_fn: \
    case Token::Tag::K_for: \
    case Token::Tag::K_if: \
    case Token::Tag::K_match: \
    case Token::Tag::K_true: \
    case Token::Tag::K_while: \
    case Token::Tag::L_f: \
    case Token::Tag::L_s: \
    case Token::Tag::L_u: \
    case Token::Tag::M_id: \
    case Token::Tag::O_add: \
    case Token::Tag::O_and: \
    case Token::Tag::O_dec: \
    case Token::Tag::O_inc: \
    case Token::Tag::O_mul: \
    case Token::Tag::O_not: \
    case Token::Tag::O_sub: \
    case Token::Tag::O_tilde

#define STMNT \
         Token::Tag::K_let: \
    case ITEM: \
    case EXPR

namespace impala {

Parser::Parser(Compiler& compiler, std::istream& stream, const char* filename)
    : lexer_(compiler, stream, filename)
{
    for (int i = 0; i != max_ahead; ++i)
        lex();
    prev_ = Location(filename, 1, 1, 1, 1);
}

/*
 * helpers
 */

Token Parser::lex() {
    prev_ = ahead_[0].location();
    for (int i = 0; i < max_ahead - 1; i++)
        ahead_[i] = ahead_[i + 1];
    ahead_[max_ahead - 1] = lexer_.lex();
    return ahead();
}

bool Parser::accept(Token::Tag tag) {
    if (tag != ahead().tag())
        return false;
    lex();
    return true;
}

bool Parser::expect(Token::Tag tag, const std::string& context) {
    if (ahead().tag() == tag) {
        lex();
        return true;
    }
    std::ostringstream oss;
    thorin::streamf(oss, "'{}'", Token::tag_to_string(tag));
    error(oss.str(), context);
    return false;
}

void Parser::error(const std::string& what, const std::string& context, const Token& tok) {
    lexer_.compiler.error(tok.location(), "expected {}, got '{}'{}", what, tok,
            context.empty() ? "" : std::string(" while parsing ") + context.c_str());
}

/*
 * Ptrn
 */

Ptr<Ptrn> Parser::parse_ptrn() {
    Ptr<Ptrn> ptrn;
    switch (ahead().tag()) {
        case Token::Tag::M_id:      ptrn = parse_id_ptrn(); break;
        case Token::Tag::D_l_paren: ptrn = parse_tuple_ptrn(); break;
        default: THORIN_UNREACHABLE;
    }

    if (accept(Token::Tag::P_colon)) {
        ptrn->type = parse_expr();
        ptrn->location += prev_;
    }

    return ptrn;
}

Ptr<IdPtrn> Parser::parse_id_ptrn() {
    return nullptr;
}

Ptr<TuplePtrn> Parser::parse_tuple_ptrn() {
    return nullptr;
}

/*
 * Expr
 */

Ptr<Expr> Parser::parse_expr() {
    return nullptr;
}

Ptr<BlockExpr> Parser::parse_block_expr() {
    auto tracker = track();
    eat(Token::Tag::D_l_brace);
    Ptrs<Stmnt> stmnts;
    Ptr<Expr> final_expr;
    while (true) {
        switch (ahead().tag()) {
            case Token::Tag::P_semicolon: lex(); continue; // ignore semicolon
            //case ITEM:             stmnts.emplace_back(parse_item_stmnt()); continue;
            //case Token::LET:       stmnts.emplace_back(parse_let_stmnt()); continue;
            //case Token::ASM:       stmnts.emplace_back(parse_asm_stmnt()); continue;
            case EXPR: {
                auto tracker = track();
                Ptr<Expr> expr;
                bool stmnt_like = true;
                switch (ahead().tag()) {
                    case Token::Tag::K_if:      expr = parse_if_expr(); break;
                    case Token::Tag::K_match:   expr = parse_match_expr(); break;
                    case Token::Tag::K_for:     expr = parse_for_expr(); break;
                    case Token::Tag::K_while:   expr = parse_while_expr(); break;
                    case Token::Tag::D_l_brace: expr = parse_block_expr(); break;
                    default:                    expr = parse_expr(); stmnt_like = false;
                }

                if (accept(Token::Tag::P_semicolon) || (stmnt_like && !ahead().isa(Token::Tag::D_r_brace))) {
                    stmnts.emplace_back(make_ptr<ExprStmnt>(tracker, std::move(expr)));
                    continue;
                }

                swap(final_expr, expr);
                [[fallthrough]];
            }
            default:
                expect(Token::Tag::D_r_brace, "block expression");
                if (!final_expr)
                    final_expr = empty_expr();
                return make_ptr<BlockExpr>(tracker, std::move(stmnts), std::move(final_expr));
        }
    }
}

Ptr<BlockExpr> Parser::try_block_expr(const std::string& context) {
    switch (ahead().tag()) {
        case Token::Tag::D_l_brace:
            return parse_block_expr();
        default:
            error("block expression", context);
            return make_ptr<BlockExpr>(prev_);
    }
}

Ptr<IfExpr> Parser::parse_if_expr() {
    auto tracker = track();
    eat(Token::Tag::K_if);
    auto cond = parse_expr();
    auto then_expr = try_block_expr("consequence of an if expression");
    Ptr<Expr> else_expr;
    if (accept(Token::Tag::K_else)) {
        switch (ahead().tag()) {
            case Token::Tag::K_if:      else_expr = parse_if_expr(); break;
            case Token::Tag::D_l_brace: else_expr = parse_block_expr(); break;
            default: error("block or if-expression", "alternative of an if-expression");
        }
    }

    if (!else_expr)
        else_expr = make_ptr<BlockExpr>(prev_);
    return make_ptr<IfExpr>(tracker, std::move(cond), std::move(then_expr), std::move(else_expr));
}

Ptr<ForExpr> Parser::parse_for_expr() {
    return nullptr;
}

Ptr<MatchExpr> Parser::parse_match_expr() {
    return nullptr;
}

Ptr<TupleExpr> Parser::parse_tuple_expr() {
    auto tracker = track();
    auto exprs = parse_list("tuple elements", Token::Tag::D_l_paren, Token::Tag::D_r_paren, Token::Tag::P_semicolon,
            [&]{ return parse_expr(); });
    return make_ptr<TupleExpr>(tracker, std::move(exprs));
}

Ptr<WhileExpr> Parser::parse_while_expr() {
    return nullptr;
}

//------------------------------------------------------------------------------

Ptr<Expr> parse(Compiler& compiler, std::istream& is, const char* filename) {
    Parser parser(compiler, is, filename);
    return parser.parse_expr();
}

}
