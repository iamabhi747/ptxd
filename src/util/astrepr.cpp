#include <ast.hpp>
#include <sstream>
#include <string>

static thread_local std::string tls_ast_repr_buffer;
static std::string_view to_repr(const std::string& str) {
    tls_ast_repr_buffer = str;
    return tls_ast_repr_buffer;
}

std::string_view RegisterNode::REPR() {
    return name;
}

std::string_view VariableNode::REPR() {
    return name;
}

std::string_view ReturnNode::REPR() {
    return to_repr("return");
}

std::string_view TypeCastNode::REPR() {
    std::stringstream ss;
    ss << "(" << type << ") " << src->REPR();
    return to_repr(ss.str());
}

std::string_view IntLiteral::REPR() {
    return to_repr(std::to_string(value));
}

std::string_view FloatLiteral::REPR() {
    return to_repr(std::to_string(value));
}

std::string_view BoolLiteral::REPR() {
    return to_repr(value ? "true" : "false");
}

std::string_view AssignmentNode::REPR() {
    std::stringstream ss;
    ss << dest->REPR() << " = " << src->REPR();
    return to_repr(ss.str());
}

std::string_view UnaryOPNode::REPR() {
    std::stringstream ss;
    ss << op << right->REPR();
    return to_repr(ss.str());
}

std::string_view BinaryOPNode::REPR() {
    std::stringstream ss;
    ss << "(" << left->REPR() << " " << op << " " << right->REPR() << ")";
    return to_repr(ss.str());
}

std::string_view TernaryOPNode::REPR() {
    std::stringstream ss;
    ss << condition->REPR() << " ? " << truthy->REPR() << " : " << falsy->REPR();
    return to_repr(ss.str());
}

std::string_view AddressRefNode::REPR() {
    std::stringstream ss;
    ss << "[" << address->REPR();
    if (offset != 0) {
        ss << (offset > 0 ? " + " : " - ") << std::abs(offset);
    }
    ss << "]";
    return to_repr(ss.str());
}

std::string_view FunctionCall::REPR() {
    std::stringstream ss;
    if (ret) ss << ret->REPR() << " = ";
    ss << name << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << parameters[i]->REPR();
    }
    ss << ")";
    return to_repr(ss.str());
}

