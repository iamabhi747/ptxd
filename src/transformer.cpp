#include <transformer.h>

#include <ranges>
#include <algorithm>

void Transformer::registerTransformer(const std::string& name, int prioroty, TransformerFactory factory)
{
    getRegistry()[name] = {factory, prioroty};
    log.logi(1, "Registered Transformer :", name, "(", prioroty, ")"); // Risky, log might not be initilized yet
}

void Transformer::enableTransformer(const std::string& name)
{
    auto& reg = getRegistry();
    if (!reg.contains(name))
    {
        log.logw(1, "Requested transformer does not exists. (", name, ")");
        return;
    }

    reg[name].second = true;
}

void Transformer::disableTransformer(const std::string& name)
{
    auto& reg = getRegistry();
    if (!reg.contains(name))
    {
        log.logw(1, "Requested transformer does not exists. (", name, ")");
        return;
    }

    reg[name].second = false;
}

bool Transformer::isEnabled(const std::string& name)
{
    auto& reg = getRegistry();
    if (!reg.contains(name))
    {
        log.logw(1, "Requested transformer does not exists. (", name, ")");
        return false;
    }

    return reg[name].second;
}

bool Transformer::run(const std::string& name)
{
    auto& reg = getRegistry();
    if (!reg.contains(name))
    {
        log.logw(1, "Requested transformer does not exists. (", name, ")");
        return false;
    }

    return reg[name].first()->run();
}



bool Transformer::run()
{
    log.logi(3, "Running", getName(), "Transformer.");

    if (ptxd == nullptr || ptxd->rawFunctions.size() == 0)
    {
        log.logw(1, "PTXD not initilized properly.");
        return false;
    }

    for (auto& func: ptxd->rawFunctions)
    {
        if (!runFunc(func))
        {
            log.logw(1, getName(), "Transformer exausted at function", func->name);
            return false;
        }
    }

    std::erase_if(ptxd->rawFunctions, [](const std::unique_ptr<FunctionBlock>& ptr) {
        return ptr == nullptr;
    });

    return true;
}

bool Transformer::runFunc(std::unique_ptr<FunctionBlock>& func)
{
    curFunc = func.get();

    int initial_size = func->blocks.size();
    for (int i = 0; i < initial_size; i++)
    {
        if (func->blocks[i] == nullptr) continue;
        
        auto block = std::move(func->blocks[i]);
        bool res = runBlock(block);
        func->blocks[i] = std::move(block);

        if (!res)
        {
            log.logw(1, "Block (", block->id, "-", block->label, ") exausted.");
            return false;
        }
    }

    curFunc = nullptr;

    std::erase_if(func->blocks, [](const std::unique_ptr<BranchBlock>& ptr) {
        return ptr == nullptr; 
    });

    return true;
}

bool Transformer::runBlock(std::unique_ptr<BranchBlock>& block)
{
    curBlock = block.get();

    int initial_size = block->statements.size();
    for (int i = 0; i < initial_size; i++)
    {
        if (block->statements[i] == nullptr) continue;

        auto stmt = std::move(block->statements[i]);
        bool res = runStmt(stmt);
        block->statements[i] = std::move(stmt);

        if (!res)
        {
            log.logw(1, "Stmt (<TODO: REPR Stmt>) exausted.");
            return false;
        }
    }

    curBlock = nullptr;

    std::erase_if(block->statements, [](const std::unique_ptr<PTXStmt>& ptr) {
        return ptr == nullptr; 
    });

    if (block->statements.size() == 0)
    {
        for (const auto& pred : block->predecessors)
        {
            auto targetptr = block.get();
            std::erase_if(pred.block->successors, [targetptr](const BranchEdge& s) {
                return s.block == targetptr;
            });
                
            pred.block->successors.insert(pred.block->successors.end(), block->successors.begin(), block->successors.end());
        }
        block.reset();
    }

    return true;
}

bool Transformer::runStmt(std::unique_ptr<PTXStmt>& stmt)
{
    log.logw(4, "Default runStmt is getting called!");
    log.logi(3, "Inst: <TODO: REPR Stmt>");
    return true;
}


void Transformer::getAllTransformers(std::vector<std::string>& transformers)
{
    auto& reg = getRegistry();

    transformers.clear();
    transformers.reserve(reg.size());

    struct PriorityData
    {
        int priority;
        std::string_view name;
    };

    std::vector<PriorityData> allTransformers;
    allTransformers.reserve(reg.size());

    for (const auto& [key, value] : reg)
    {
        allTransformers.push_back({value.second, key}); 
    }

    std::ranges::sort(allTransformers, std::less{}, &PriorityData::priority);

    auto keys_view = allTransformers 
    | std::views::filter([](const PriorityData& data) { return data.priority != -1; })
    | std::views::transform([](const PriorityData& data) { return std::string(data.name); });

    std::ranges::copy(keys_view, std::back_inserter(transformers));
}