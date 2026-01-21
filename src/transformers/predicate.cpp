#include <transformer.h>
#include <util/logger.h>

class PredicateTransformer : public Transformer
{
public:
    std::string getName() const override { return "Predicate"; }

    bool run() override
    {
        log.logi(3, "Called Predicate Transformer Run.");

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

        return true;
    }

    bool runFunc(std::unique_ptr<FunctionBlock>& func)
    {
        for (auto& block : func->blocks)
        {
            if (!runBlock(block))
            {
                log.logw(1, "Block (", block->id, "-", block->label, ") exausted.");
                return false;
            }
        }

        return true;
    }

    bool runBlock(std::unique_ptr<BranchBlock>& block)
    {
        for (auto& stmt : block->statements)
        {
            if (!runStmt(stmt))
            {
                log.logw(1, "Stmt (<TODO: REPR Stmt>) exausted.");
                return false;
            }
        }

        return true;
    }

    bool runStmt(std::unique_ptr<PTXStmt>& stmt)
    {
        if (!stmt->predicate.empty())
        {
            log.logi(1, "Found predicate (", stmt->predicate, ")");

            bool isNot = stmt->predicate[0] == '!';
            std::string reg = stmt->predicate.substr(stmt->predicate.find("%"));

            // TODOOOOO
        }

        return true;
    }
};

static struct PredicateRegister
{
    PredicateRegister()
    {
        Transformer::registerTransformer("predicate", []() {
            return std::make_unique<PredicateTransformer>();
        });
    }
} register_pridicate;