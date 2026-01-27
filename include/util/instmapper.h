#ifndef INSTMAPPER_H
#define INSTMAPPER_H

#include <ptx/inst.hpp> 
#include <ptx/op.hpp>

#include <string> 
#include <unordered_map> 
#include <functional> 

class InstMapper
{
    private: 
        using opHandler = std::function<std::string(PTXInstruction*)>;
        std::unordered_map<PTXOpcode, opHandler> handlers; 

        void registerHandlers(); 
    
        public: 
            InstMapper(); 
            std::string getOperationLogic(PTXInstruction* inst); 
};


#endif