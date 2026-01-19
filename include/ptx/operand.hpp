#ifndef PTX_OPERAND_H
#define PTX_OPERAND_H

#include <string>
#include <vector>

enum class OperandType {
    NONE,
    REGISTER, SPECIAL_REGISTER,
    IMMEDIATE_INT, IMMEDIATE_FLOAT,
    MEMORY, LABEL
};

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
};

#endif