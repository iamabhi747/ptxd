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
    virtual bool run() = 0;
    virtual std::string getName() const = 0;

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
};

#endif