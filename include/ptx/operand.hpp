#ifndef PTX_OPERAND_H
#define PTX_OPERAND_H

#include <string>
#include <vector>
#include <string_view>

enum class OperandType {
    NONE,
    REGISTER, SPECIAL_REGISTER,
    IMMEDIATE_INT, IMMEDIATE_FLOAT,
    MEMORY, LABEL
};
constexpr std::string_view REPR(OperandType obj);
std::ostream& operator<<(std::ostream& os, OperandType obj);

struct PTXOperand {
    OperandType type = OperandType::NONE;
    std::string raw;

    std::string name; 

    int64_t immInt    = 0;
    float   immFloat  = 0.0f;
    double  immDouble = 0.0;

    std::string baseReg;
    int offset = 0;
    
    bool isVector = false;
    std::vector<std::string> vectorRegs;

    std::string_view REPR();
    friend std::ostream& operator<<(std::ostream& os, PTXOperand& obj);
};

#endif