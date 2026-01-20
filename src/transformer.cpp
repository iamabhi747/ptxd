#include <transformer.h>

void Transformer::registerTransformer(const std::string& name, TransformerFactory factory)
{
    getRegistry()[name] = {factory, true};
    log.logi(1, "Registered Transformer :", name); // Risky, log might not be initilized yet
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