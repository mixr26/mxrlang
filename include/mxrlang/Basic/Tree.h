#ifndef TREE_H
#define TREE_H

#include "Type.h"

#include "llvm/ADT/APSInt.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SMLoc.h"

#include <vector>

// This file contains definitions of abstract syntax tree expression
// and statement classes.

namespace mxrlang {

class Stmt;

using Stmts = std::vector<Stmt*>;

// The following class describes a declaration.

class Decl {
    llvm::StringRef name;
    Type* type;

public:
    Decl(llvm::StringRef name, Type* type = Type::getIntType())
        : name(name), type(type) {}

    llvm::StringRef getName() const { return name; }
    Type* getType() { return type; }
};

// The following classes describe expression nodes of the AST.

class AssignExpr;
class BoolLiteralExpr;
class IntLiteralExpr;
class VarExpr;

class ExprVisitor {
public:
    virtual void visit(AssignExpr* expr) = 0;
    virtual void visit(BoolLiteralExpr* expr) = 0;
    virtual void visit(IntLiteralExpr* expr) = 0;
    virtual void visit(VarExpr* expr) = 0;
};

class Expr {
public:
    enum class ExprKind {
        Assign,
        BoolLiteral,
        IntLiteral,
        Var
    };

private:
    ExprKind kind;
    llvm::SMLoc loc;
    // Every expression should have a type.
    Type* type;

public:
    Expr(ExprKind kind, llvm::SMLoc loc, Type* type = Type::getNoneType())
        : kind(kind), loc(loc), type(type) {}
    virtual ~Expr() = default;

    // Pure virtual accept method of the visitor pattern.
    virtual void accept(ExprVisitor* visitor) = 0;

    // Check whether this is a valid target of an assignment.
    // Can be overridden by valid targets (eg. Variable_expr) to return an
    // Assign_expr.
    virtual Expr* makeAssignExpr(Expr* source) { return nullptr; }

    ExprKind getKind() const { return kind; }
    llvm::SMLoc getLoc() const { return loc; }
    Type* getType() const { return type; }

    void setType(Type* type) { this->type = type; }
};

class AssignExpr : public Expr {
    Expr* dest;
    Expr* source;

public:
    AssignExpr(Expr* dest, Expr* source, llvm::SMLoc loc)
        : Expr(ExprKind::Assign, loc), dest(dest), source(source) {}

    Expr* getDest() { return dest; }
    Expr* getSource() { return source; }

    virtual void accept(ExprVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Expr* E) {
        return E->getKind() == ExprKind::Assign;
    }
};

class BoolLiteralExpr : public Expr {
    bool value;

public:
    BoolLiteralExpr(bool value, llvm::SMLoc loc)
        : Expr(ExprKind::BoolLiteral, loc, Type::getBoolType()),
          value(value) {}

    bool getValue() const { return value; }

    virtual void accept(ExprVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Expr* E) {
        return E->getKind() == ExprKind::BoolLiteral;
    }
};

class IntLiteralExpr : public Expr {
    llvm::APSInt value;

public:
    IntLiteralExpr(llvm::StringRef valueString, llvm::SMLoc loc)
        : Expr(ExprKind::IntLiteral, loc, Type::getIntType()) {
        value = llvm::APInt(/* numBits= */ 64, valueString, /* radix= */ 10);
        value.setIsSigned(true);
    }

    llvm::APSInt& getValue() { return value; }

    virtual void accept(ExprVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Expr* E) {
        return E->getKind() == ExprKind::IntLiteral;
    }
};

class VarExpr : public Expr {
    llvm::StringRef name;

public:
    VarExpr(llvm::StringRef name, llvm::SMLoc loc)
        : Expr(ExprKind::Var, loc), name(name) {}

    llvm::StringRef getName() { return name; }

    virtual void accept(ExprVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Expr* E) {
        return E->getKind() == ExprKind::Var;
    }

    // VarExpr is a valid assignment destination.
    Expr* makeAssignExpr(Expr* source) override {
        return new AssignExpr(this, source, this->getLoc());
    }
};

// The following classes describe statement nodes of the AST.

class ExprStmt;
class FunStmt;
class IfStmt;
class ModuleStmt;
class ReturnStmt;
class VarStmt;

class StmtVisitor {
public:
    virtual void visit(ExprStmt* stmt) = 0;
    virtual void visit(FunStmt* stmt) = 0;
    virtual void visit(IfStmt* stmt) = 0;
    virtual void visit(ModuleStmt* stmt) = 0;
    virtual void visit(ReturnStmt* stmt) = 0;
    virtual void visit(VarStmt* stmt) = 0;
};

class Stmt {
public:
    enum class StmtKind {
        Expr,
        Fun,
        If,
        Module,
        Return,
        Var
    };

private:
    StmtKind kind;
    llvm::SMLoc loc;

public:
    Stmt(StmtKind kind, llvm::SMLoc loc) : kind(kind), loc(loc) {}
    virtual ~Stmt() = default;

    // Pure virtual accept method of the visitor pattern.
    virtual void accept(StmtVisitor* visitor) = 0;

    StmtKind getKind() const { return kind; }
    llvm::SMLoc getLoc() const { return loc; }
};

// Statement node describing an expression statement.
class ExprStmt : public Stmt {
    Expr* expr;

public:
    ExprStmt(Expr* expr, llvm::SMLoc loc)
        : Stmt(StmtKind::Expr, loc), expr(expr) {}

    Expr* getExpr() { return expr; }

    virtual void accept(StmtVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt* S) {
        return S->getKind() == StmtKind::Expr;
    }
};

// Statement node describing a function definition.
// Currently it only holds the top level "main" function.
class FunStmt : public Stmt,
                public Decl {
    llvm::StringRef name;
    Stmts body;

public:
    FunStmt(llvm::StringRef name, Stmts&& body, llvm::SMLoc loc)
        : Stmt(StmtKind::Fun, loc), Decl(name),
          name(name), body(std::move(body)) {}

    llvm::StringRef getName() { return name; }
    Stmts& getBody() { return body; }

    virtual void accept(StmtVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt* S) {
      return S->getKind() == StmtKind::Fun;
    }
};


// Statement node describing an IF statement.
class IfStmt : public Stmt {
    Expr* cond;
    Stmts thenStmts;
    Stmts elseStmts;

public:
    IfStmt(Expr* cond, Stmts&& thenStmts,
           Stmts&& elseStmts, llvm::SMLoc loc)
        : Stmt(StmtKind::If, loc), cond(cond),
          thenStmts(std::move(thenStmts)),
          elseStmts(std::move(elseStmts)) {}

    Expr* getCond() { return cond; }
    Stmts& getThenStmts() { return thenStmts; }
    Stmts& getElseStmts() { return elseStmts; }

    virtual void accept(StmtVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt* S) {
        return S->getKind() == StmtKind::If;
    }
};


// Statement node describing a module.
// Currently only one module supported per program.
class ModuleStmt : public Stmt,
                   public Decl {
    llvm::StringRef name;
    Stmts body;

public:
    ModuleStmt(llvm::StringRef name, Stmts&& body, llvm::SMLoc loc)
        : Stmt(StmtKind::Module, loc), Decl(name),
          name(name), body(std::move(body)) {}

    llvm::StringRef getName() { return name; }
    Stmts& getBody() { return body; }

    virtual void accept(StmtVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt* S) {
      return S->getKind() == StmtKind::Module;
    }
};

// Statement node describing a return statement.
class ReturnStmt : public Stmt {
    Expr* retExpr;

public:
    ReturnStmt(Expr* retExpr, llvm::SMLoc loc)
        : Stmt(StmtKind::Return, loc), retExpr(retExpr) {}

    Expr* getRetExpr() { return retExpr; }

    virtual void accept(StmtVisitor *visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt* S) {
        return S->getKind() == StmtKind::Return;
    }
};

// Statement node decribing a variable declaration/definition.
class VarStmt : public Stmt,
                public Decl {
    llvm::StringRef name;
    Expr* initializer;

public:
    VarStmt(llvm::StringRef name, Expr* initializer, Type* type,
            llvm::SMLoc loc)
        : Stmt(StmtKind::Var, loc), Decl(name, type),
          name(name), initializer(initializer) {}

    llvm::StringRef getName() { return name; }
    Expr* getInitializer() { return initializer; }

    virtual void accept(StmtVisitor* visitor) override {
        visitor->visit(this);
    }

    static bool classof(const Stmt *S) {
      return S->getKind() == StmtKind::Var;
    }
};

} // namespace mxrlang

#endif // TREE_H
