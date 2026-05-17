#ifndef AST_HPP
#define AST_HPP

#include <ptx/inst.hpp>

class RegisterNode : public PTXStmt
{
public:
    std::string name;

    std::string getName() const override { return "RegisterNode"; };
    RegisterNode(std::string _name) : name (_name) {}
    ~RegisterNode() = default;
    std::string_view REPR() override;
};

class VariableNode : public PTXStmt
{
public:
    std::string name;

    std::string getName() const override { return "VariableNode"; };
    VariableNode(std::string _name) : name (_name) {}
    ~VariableNode() = default;
    std::string_view REPR() override;
};

class ReturnNode : public PTXStmt
{
public:
    std::string getName() const override { return "ReturnNode"; };
    std::string_view REPR() override;
};

class TypeCastNode : public PTXStmt
{
public:
    PTXDataType type;
    std::unique_ptr<PTXStmt> src;

    std::string getName() const override { return "TypeCast"; };
    TypeCastNode(PTXDataType _type, std::unique_ptr<PTXStmt> _src) : type (_type), src (std::move(_src)) {}
    ~TypeCastNode() = default;
    std::string_view REPR() override;
};

// Literals
class IntLiteral : public PTXStmt
{
public:
    PTXDataType type;
    long long int value;

    std::string getName() const override { return "IntLiteral"; };
    IntLiteral(PTXDataType _type, long long int _value) : type (_type), value (_value) {}
    ~IntLiteral() = default;
    std::string_view REPR() override;
};

class FloatLiteral : public PTXStmt
{
public:
    PTXDataType type;
    double value;

    std::string getName() const override { return "FloatLiteral"; };
    FloatLiteral(PTXDataType _type, double _value) : type (_type), value (_value) {}
    ~FloatLiteral() = default;
    std::string_view REPR() override;
};

class BoolLiteral : public PTXStmt
{
public:
    PTXDataType type;
    bool value;

    std::string getName() const override { return "BoolLiteral"; };
    BoolLiteral(PTXDataType _type, bool _value) : type (_type), value (_value) {}
    ~BoolLiteral() = default;
    std::string_view REPR() override;
};

// Expressions
class AssignmentNode : public PTXStmt
{
public:
    std::unique_ptr<PTXStmt> dest; // RegisterNode / VariableNode
    std::unique_ptr<PTXStmt> src;

    std::string getName() const override { return "AssignmentNode"; };
    AssignmentNode(std::unique_ptr<PTXStmt> _dest, std::unique_ptr<PTXStmt> _src) : dest (std::move(_dest)), src (std::move(_src)) {}
    ~AssignmentNode() = default;
    std::string_view REPR() override;
};

class UnaryOPNode : public PTXStmt
{
public:
    PTXComp op;
    std::unique_ptr<PTXStmt> right;

    std::string getName() const override { return "UnaryOPNode"; };
    UnaryOPNode(PTXComp _op, std::unique_ptr<PTXStmt> _right) : op (_op), right (std::move(_right)) {}
    ~UnaryOPNode() = default;
    std::string_view REPR() override;
};

class BinaryOPNode : public PTXStmt
{
public:
    PTXComp op;
    std::unique_ptr<PTXStmt> left;
    std::unique_ptr<PTXStmt> right;

    std::string getName() const override { return "BinaryOPNode"; };
    BinaryOPNode(PTXComp _op, std::unique_ptr<PTXStmt> _left, std::unique_ptr<PTXStmt> _right) : op (_op), left (std::move(_left)), right (std::move(_right)) {}
    ~BinaryOPNode() = default;
    std::string_view REPR() override;
};

class TernaryOPNode : public PTXStmt
{
public:
    std::unique_ptr<PTXStmt> condition;
    std::unique_ptr<PTXStmt> truthy;
    std::unique_ptr<PTXStmt> falsy;
    // condition ? truthy : falsy

    std::string getName() const override { return "TernaryOPNode"; };
    TernaryOPNode(std::unique_ptr<PTXStmt> _condition, std::unique_ptr<PTXStmt> _truthy, std::unique_ptr<PTXStmt> _falsy) : condition (std::move(_condition)), truthy (std::move(_truthy)), falsy (std::move(_falsy)) {}
    ~TernaryOPNode() = default;
    std::string_view REPR() override;
};

class AddressRefNode : public PTXStmt
{
public:
    std::unique_ptr<PTXStmt> address;
    int offset;

    std::string getName() const override { return "AddressRefNode"; };
    AddressRefNode(std::unique_ptr<PTXStmt> _address, int _offset) : address (std::move(_address)), offset (_offset) {}
    ~AddressRefNode() = default;
    std::string_view REPR() override;
};

class FunctionCall : public PTXStmt
{
public:
    std::string name;
    std::unique_ptr<PTXStmt> ret;
    std::vector<std::unique_ptr<PTXStmt>> parameters;

    std::string getName() const override { return "FunctionCall"; };
    FunctionCall(std::string _name, std::unique_ptr<PTXStmt> _ret, std::vector<std::unique_ptr<PTXStmt>> _parameters) : name (_name), ret (std::move(_ret)), parameters (std::move(_parameters)) {}
    ~FunctionCall() = default;
    std::string_view REPR() override;
};

#endif