#ifndef IMPALA_AST_H
#define IMPALA_AST_H

#include <deque>
#include <memory>

#include "impala/token.h"

namespace impala {

struct Expr;
struct Stmnt;

template<class T> using Ptr = std::unique_ptr<T>;
template<class T> using Ptrs = std::deque<std::unique_ptr<T>>;
template<class T, class... Args>
std::unique_ptr<T> make_ptr(Args... args) { return std::make_unique<T>(std::forward<Args>(args)...); }

struct Node {
    Node(Location location)
        : location(location)
    {}
    virtual ~Node() {}

    Location location;
};

struct Id : public Node {
    Id(Location location, Symbol symbol)
        : Node(location)
        , symbol(symbol)
    {}
    Id(Token token)
        : Node(token.location())
        , symbol(token.symbol())
    {}

    //std::ostream& stream(std::ostream&) const override;

    Symbol symbol;
};

/*
 * Ptrn
 */

struct Ptrn : public Node {
    Ptrn(Location location)
        : Node(location)
    {}

    Ptr<Expr> type;
};

struct IdPtrn : public Ptrn {
    IdPtrn(Location location, Ptr<Id>&& id)
        : Ptrn(location)
        , id(std::move(id))

    {}

    Ptr<Id> id;
};

struct TuplePtrn : public Ptrn {
    TuplePtrn(Location location, Ptrs<Ptrn>&& ptrns)
        : Ptrn(location)
        , ptrns(std::move(ptrns))
    {}

    Ptrs<Ptrn> ptrns;
};

/*
 * Expr
 */

struct Expr : public Node {
    Expr(Location location)
        : Node(location)
    {}
};

struct BinderExpr : public Expr {
    BinderExpr(Location location, Ptr<Id>&& id, Ptr<Expr> type)
        : Expr(location)
        , id(std::move(id))
        , type(std::move(type))
    {}

    Ptr<Id> id;
    Ptr<Expr> type;
};

struct TupleExpr : public Expr {
    TupleExpr(Location location, Ptrs<Expr>&& exprs = {})
        : Expr(location)
        , exprs(std::move(exprs))
    {}

    Ptrs<Expr> exprs;
};

struct SigmaExpr : public Expr {
    SigmaExpr(Location location, Ptrs<BinderExpr>&& binders)
        : Expr(location)
        , binders(std::move(binders))
    {}
    SigmaExpr(Location location)
        : SigmaExpr(location, {})
    {}

    Ptrs<BinderExpr> binders;
};

struct BlockExpr : public Expr {
    BlockExpr(Location location, Ptrs<Stmnt>&& stmnts, Ptr<Expr>&& expr)
        : Expr(location)
        , stmnts(std::move(stmnts))
        , expr(std::move(expr))
    {}
    /// An empty BlockExpr with no @p stmnts and an empty @p TupleExpr as @p expr.
    BlockExpr(Location location)
        : BlockExpr(location, {}, std::make_unique<TupleExpr>(location))
    {}

    Ptrs<Stmnt> stmnts;
    Ptr<Expr> expr;
};

struct IfExpr : public Expr {
    IfExpr(Location location, Ptr<Expr>&& cond, Ptr<Expr>&& then_expr, Ptr<Expr>&& else_expr)
        : Expr(location)
        , cond(std::move(cond))
        , then_expr(std::move(then_expr))
        , else_expr(std::move(else_expr))
    {}

    Ptr<Expr> cond;
    Ptr<Expr> then_expr;
    Ptr<Expr> else_expr;
};

struct MatchExpr : public Expr {
};

struct ForExpr : public Expr {
};

struct WhileExpr : public Expr {
};

/*
 * Stmnt
 */

struct Stmnt : public Node {
    Stmnt(Location location)
        : Node(location)
    {}
};

struct ExprStmnt : public Stmnt {
    ExprStmnt(Location location, Ptr<Expr>&& expr)
        : Stmnt(location)
        , expr(std::move(expr))
    {}

    Ptr<Expr> expr;
};

}

#endif
