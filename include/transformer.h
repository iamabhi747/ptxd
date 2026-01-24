#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <string>
#include <memory>
#include <functional>

#include <util/logger.h>
#include <ptxd.h>

class Transformer
{
public:
    virtual ~Transformer() = default;
    virtual std::string getName() const = 0;
    virtual bool run();
    virtual bool runFunc(std::unique_ptr<FunctionBlock>& func);
    virtual bool runBlock(std::unique_ptr<BranchBlock>& block);
    virtual bool runStmt(std::unique_ptr<PTXStmt>& stmt);

    static PTXD* ptxd;
    static inline Logger& log = Logger::getInstance();
    using TransformerFactory = std::function<std::unique_ptr<Transformer>()>;

    static void registerTransformer(const std::string& name, TransformerFactory factory);
    static void enableTransformer(const std::string& name);
    static void disableTransformer(const std::string& name);
    static bool isEnabled(const std::string& name);
    static bool run(const std::string& name);

private:
    static std::unordered_map<std::string, std::pair<TransformerFactory, bool>>& getRegistry() {
        static std::unordered_map<std::string, std::pair<TransformerFactory, bool>> registry;
        return registry;
    }

protected:
    FunctionBlock* curFunc;
    BranchBlock*   curBlock;
};

#endif