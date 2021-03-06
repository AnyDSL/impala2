#ifndef IMPALA_AST_H
#define IMPALA_AST_H

#include <deque>
#include <memory>

#include "thorin/util/cast.h"
#include "thorin/util/stream.h"

#include "impala/token.h"
#include "impala/bind.h"
#include "impala/print.h"

namespace impala {

class Emitter;
class Printer;
class Scopes;

struct Expr;
struct Stmnt;

template<class T, class... Args>
Ptr<T> make_ptr(Args... args) { return std::make_unique<const T>(std::forward<Args>(args)...); }

namespace detail {
    template<class T> void make_ptrs(Ptrs<T>&) {}
    template<class T, class A, class... Args>
    void make_ptrs(Ptrs<T>& ptrs, A&& arg, Args&&... args) {
        ptrs.emplace_back(std::forward<A>(arg));
        make_ptrs(ptrs, std::forward<Args>(args)...);
    }
}

template<class T, class... Args>
Ptrs<T> make_ptrs(Args&&... args) {
    Ptrs<T> result;
    detail::make_ptrs(result, std::forward<Args>(args)...);
    return result;
}

//------------------------------------------------------------------------------

struct Node : public thorin::Streamable<Printer> {
    Node(Loc loc)
        : loc(loc)
    {}
    virtual ~Node() {}

    std::ostream& stream_out(std::ostream&) const;

    Loc loc;
};

struct Prg : public Node {
    Prg(Loc loc, Ptrs<Stmnt>&& stmnts)
        : Node(loc)
        , stmnts(std::move(stmnts))
    {}

    void bind(Scopes&) const;
    void emit(Emitter&) const;
    Printer& stream(Printer&) const override;

    Ptrs<Stmnt> stmnts;
};

struct Id : public Node {
    Id(Token token)
        : Node(token.loc())
        , symbol(token.symbol())
    {}

    Printer& stream(Printer&) const override;

    Symbol symbol;
};

struct Item : public Node {
    Item(Loc loc, Ptr<Id>&& id, Ptr<Expr> expr)
        : Node(loc)
        , id(std::move(id))
        , expr(std::move(expr))
    {}

    const thorin::Def* def() const { return def_; }
    void bind_rec(Scopes&) const;
    void bind(Scopes&) const;
    void emit_rec(Emitter&) const;
    void emit(Emitter&) const;
    Printer& stream(Printer&) const override;

    Ptr<Id> id;
    Ptr<Expr> expr;

private:
    mutable const thorin::Def* def_ = nullptr;
};

/*
 * Ptrn
 */

struct Ptrn : public thorin::RuntimeCast<Ptrn>, public Node {
    Ptrn(Loc loc, Ptr<Expr>&& type, bool type_mandatory)
        : Node(loc)
        , type(std::move(type))
        , type_mandatory(type_mandatory)
    {}

    virtual void bind(Scopes&) const = 0;
    const thorin::Def* emit(Emitter&) const;
    virtual void emit(Emitter&, const thorin::Def*) const = 0;
    Printer& stream_ascription(Printer&) const ;

    Ptr<Expr> type;
    bool type_mandatory;
};

struct ErrorPtrn : public Ptrn {
    ErrorPtrn(Loc loc)
        : Ptrn(loc, nullptr, false)
    {}

    void bind(Scopes&) const override;
    void emit(Emitter&, const thorin::Def*) const override;
    Printer& stream(Printer&) const override;
};

struct IdPtrn : public Ptrn {
    IdPtrn(Loc loc, Ptr<Id>&& id, Ptr<Expr>&& type, bool type_mandatory)
        : Ptrn(loc, std::move(type), type_mandatory)
        , id(std::move(id))
    {}

    Symbol symbol() const { return id->symbol; }
    const thorin::Def* def() const { return def_; }

    void bind(Scopes&) const override;
    void emit(Emitter&, const thorin::Def*) const override;
    Printer& stream(Printer&) const override;

    Ptr<Id> id;

private:
    mutable const thorin::Def* def_ = nullptr;
};

struct TuplePtrn : public Ptrn {
    TuplePtrn(Loc loc, Ptrs<Ptrn>&& elems, Ptr<Expr>&& type, bool type_mandatory)
        : Ptrn(loc, std::move(type), type_mandatory)
        , elems(std::move(elems))
    {}

    void bind(Scopes&) const override;
    void emit(Emitter&, const thorin::Def*) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Ptrn> elems;
};

/*
 * Expr
 */

struct Expr : public thorin::RuntimeCast<Expr>, public Node {
    Expr(Loc loc)
        : Node(loc)
    {}

    virtual void bind(Scopes&) const = 0;
    virtual const thorin::Def* emit(Emitter&) const = 0;
};

struct AppExpr : public Expr {
    AppExpr(Loc loc, Ptr<Expr>&& callee, Ptr<Expr>&& arg, bool cps)
        : Expr(loc)
        , callee(std::move(callee))
        , arg(std::move(arg))
        , cps(cps)
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> callee;
    Ptr<Expr> arg;
    bool cps;
};

struct BlockExpr : public Expr {
    BlockExpr(Loc loc, Ptrs<Stmnt>&& stmnts, Ptr<Expr>&& expr)
        : Expr(loc)
        , stmnts(std::move(stmnts))
        , expr(std::move(expr))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Stmnt> stmnts;
    Ptr<Expr> expr;
};

struct BottomExpr : public Expr {
    BottomExpr(Loc loc)
        : Expr(loc)
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;
};

struct ErrorExpr : public Expr {
    ErrorExpr(Loc loc)
        : Expr(loc)
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;
};

struct IdExpr : public Expr {
    IdExpr(Ptr<Id>&& id)
        : Expr(id->loc)
        , id(std::move(id))
    {}

    Symbol symbol() const { return id->symbol; }

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Id> id;
    mutable Decl decl;
};

struct IfExpr : public Expr {
    IfExpr(Loc loc, Ptr<Expr>&& cond, Ptr<Expr>&& then_expr, Ptr<Expr>&& else_expr)
        : Expr(loc)
        , cond(std::move(cond))
        , then_expr(std::move(then_expr))
        , else_expr(std::move(else_expr))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> cond;
    Ptr<Expr> then_expr;
    Ptr<Expr> else_expr;
};

struct InfixExpr : public Expr {
    enum class Tag {
        O_tilde      = int(Token::Tag::O_tilde),
        O_and_and    = int(Token::Tag::O_and_and),
        O_or_or      = int(Token::Tag::O_or_or),
    };

    InfixExpr(Loc loc, Ptr<Expr>&& lhs, Tag tag, Ptr<Expr>&& rhs)
        : Expr(loc)
        , lhs(std::move(lhs))
        , tag(tag)
        , rhs(std::move(rhs))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> lhs;
    Tag tag;
    Ptr<Expr> rhs;
};

struct FieldExpr : public Expr {
    FieldExpr(Loc loc, Ptr<Expr>&& lhs, Ptr<Id>&& id)
        : Expr(loc)
        , lhs(std::move(lhs))
        , id(std::move(id))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> lhs;
    Ptr<Id> id;
};

struct ForallExpr : public Expr {
    ForallExpr(Loc loc, Ptr<Ptrn>&& domain, Ptr<Expr>&& codomain)
        : Expr(loc)
        , domain(std::move(domain))
        , codomain(std::move(codomain))
    {}

    bool returns_bottom() const { return codomain->isa<BottomExpr>(); }

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Ptrn> domain;
    Ptr<Expr> codomain;
};

struct ForExpr : public Expr {
    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;
};

struct LambdaExpr : public Expr {
    LambdaExpr(Loc loc, Ptr<Ptrn>&& domain, Ptr<Expr>&& codomain, Ptr<Expr>&& body)
        : Expr(loc)
        , domain(std::move(domain))
        , codomain(std::move(codomain))
        , body(std::move(body))
    {}

    bool returns_bottom() const { return codomain->isa<BottomExpr>(); }

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    mutable const Id* id = nullptr;
    Ptr<Ptrn> domain;
    Ptr<Expr> codomain;
    Ptr<Expr> body;
};

struct MatchExpr : public Expr {
    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;
};

struct PackExpr : public Expr {
    PackExpr(Loc loc, Ptrs<Ptrn>&& domains, Ptr<Expr>&& body)
        : Expr(loc)
        , domains(std::move(domains))
        , body(std::move(body))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Ptrn> domains;
    Ptr<Expr> body;
};

struct PrefixExpr : public Expr {
    enum class Tag {
        O_inc = int(Token::Tag::O_inc),
        O_dec = int(Token::Tag::O_dec),
        O_add = int(Token::Tag::O_add),
        O_sub = int(Token::Tag::O_sub),
        O_and = int(Token::Tag::O_and),
    };

    PrefixExpr(Loc loc, Tag tag, Ptr<Expr>&& rhs)
        : Expr(loc)
        , tag(tag)
        , rhs(std::move(rhs))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Tag tag;
    Ptr<Expr> rhs;
};

struct PostfixExpr : public Expr {
    enum class Tag {
        O_inc = int(Token::Tag::O_inc),
        O_dec = int(Token::Tag::O_dec),
    };

    PostfixExpr(Loc loc, Ptr<Expr>&& lhs, Tag tag)
        : Expr(loc)
        , lhs(std::move(lhs))
        , tag(tag)
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> lhs;
    Tag tag;
};

struct QualifierExpr : public Expr {
    enum class Tag {
        u = int(Token::Tag::U_u),
        r = int(Token::Tag::U_r),
        a = int(Token::Tag::U_a),
        l = int(Token::Tag::U_l),
    };

    QualifierExpr(Token token)
        : Expr(token.loc())
        , tag((Tag) token.tag())
    {}

    Printer& stream(Printer&) const override;
    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;

    Tag tag;
};

struct TupleExpr : public Expr {
    struct Elem : public Node {
        Elem(Loc loc, Ptr<Id>&& id, Ptr<Expr>&& expr)
            : Node(loc)
            , id(std::move(id))
            , expr(std::move(expr))
        {}

        void bind(Scopes&) const;
        const thorin::Def* emit(Emitter&) const;
        Printer& stream(Printer&) const override;

        Ptr<Id> id;
        Ptr<Expr> expr;
    };

    TupleExpr(Loc loc, Ptrs<Elem>&& elems, Ptr<Expr>&& type)
        : Expr(loc)
        , elems(std::move(elems))
        , type(std::move(type))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Elem> elems;
    Ptr<Expr> type;
};

struct TypeExpr : public Expr {
    TypeExpr(Loc loc, Ptr<Expr>&& qualifier)
        : Expr(loc)
        , qualifier(std::move(qualifier))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> qualifier;
};

struct VariadicExpr : public Expr {
    VariadicExpr(Loc loc, Ptrs<Ptrn>&& domains, Ptr<Expr>&& body)
        : Expr(loc)
        , domains(std::move(domains))
        , body(std::move(body))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Ptrn> domains;
    Ptr<Expr> body;
};

struct SigmaExpr : public Expr {
    SigmaExpr(Loc loc, Ptrs<Ptrn>&& elems)
        : Expr(loc)
        , elems(std::move(elems))
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptrs<Ptrn> elems;
};

struct UnknownExpr : public Expr {
    UnknownExpr(Loc loc)
        : Expr(loc)
    {}

    void bind(Scopes&) const override;
    const thorin::Def* emit(Emitter&) const override;
    Printer& stream(Printer&) const override;
};

struct WhileExpr : public Expr {
};

/*
 * Stmnt
 */

struct Stmnt : public thorin::RuntimeCast<Stmnt>, public Node {
    Stmnt(Loc loc)
        : Node(loc)
    {}

    virtual void bind(Scopes&) const = 0;
    virtual void emit(Emitter&) const = 0;
};

struct ExprStmnt : public Stmnt {
    ExprStmnt(Loc loc, Ptr<Expr>&& expr)
        : Stmnt(loc)
        , expr(std::move(expr))
    {}

    void bind(Scopes&) const override;
    void emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Expr> expr;
};

struct ItemStmnt : public Stmnt {
    ItemStmnt(Loc loc, Ptr<Item>&& item)
        : Stmnt(loc)
        , item(std::move(item))
    {}

    void bind(Scopes&) const override;
    void emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Item> item;
};

struct LetStmnt : public Stmnt {
    LetStmnt(Loc loc, Ptr<Ptrn>&& ptrn, Ptr<Expr>&& init)
        : Stmnt(loc)
        , ptrn(std::move(ptrn))
        , init(std::move(init))
    {}

    void bind(Scopes&) const override;
    void emit(Emitter&) const override;
    Printer& stream(Printer&) const override;

    Ptr<Ptrn> ptrn;
    Ptr<Expr> init;
};

//------------------------------------------------------------------------------

}

#endif
