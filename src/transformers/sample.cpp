#include <transformer.h>
#include <util/logger.h>

class SampleAnalysisTransformer : public Transformer
{
public:
    std::string getName() const override { return "Sample Analysis"; }

    bool run() override
    {
        log.logi(3, "Called Sample Run.");
        return true;
    }
};

static struct SampleAnalysisRegister
{
    SampleAnalysisRegister()
    {
        Transformer::registerTransformer("sample", -1, []() {
            return std::make_unique<SampleAnalysisTransformer>();
        });
    }
} register_sampleanalysis;