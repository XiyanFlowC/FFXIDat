#include "ProcessorFactory.h"
#include "Processors/XisProcessor.h"
#include "Processors/EvsbProcessor.h"
#include "Processors/DMsgProcessor.h"
#include "Processors/ItemProcessor.h"
#include "Processors/StatusDataProcessor.h"
#include "Processors/FixedPhraseProcessor.h"
#include "Processors/MonBridgeProcessor.h"
#include "Processors/RoeProcessor.h"
#include "Processors/EjrefToleranceProcessor.h"

ProcessorFactory& ProcessorFactory::Instance()
{
    static ProcessorFactory instance;
    return instance;
}

ProcessorFactory::ProcessorFactory()
{
    RegisterDefaultProcessors();
}

void ProcessorFactory::RegisterDefaultProcessors()
{
    // Register XisProcessor
    RegisterProcessor(u8"xis", std::make_shared<XisProcessor>());

    // Register EvsbProcessor
    RegisterProcessor(u8"evsb", std::make_shared<EvsbProcessor>());

    // Register DMsgProcessor
    RegisterProcessor(u8"dmsg", std::make_shared<DMsgProcessor>());

    // Register ItemProcessor for all item types
    auto itemProcessor = std::make_shared<ItemProcessor>();
    RegisterProcessor(u8"iab", itemProcessor);
    RegisterProcessor(u8"iwb", itemProcessor);
    RegisterProcessor(u8"iub", itemProcessor);
    RegisterProcessor(u8"inb", itemProcessor);
    RegisterProcessor(u8"ipb", itemProcessor);
    RegisterProcessor(u8"isb", itemProcessor);
    RegisterProcessor(u8"icb", itemProcessor);
    RegisterProcessor(u8"iib", itemProcessor);

    // Register StatusDataProcessor
    RegisterProcessor(u8"sd", std::make_shared<StatusDataProcessor>());

    // Register FixedPhraseProcessor
    RegisterProcessor(u8"fp", std::make_shared<FixedPhraseProcessor>());

    // Register MonBridgeProcessor
    RegisterProcessor(u8"mbd", std::make_shared<MonBridgeProcessor>());

    // Register RoeProcessor for both quest and category types
    auto roeProcessor = std::make_shared<RoeProcessor>();
    RegisterProcessor(u8"erq", roeProcessor);
    RegisterProcessor(u8"erc", roeProcessor);

    // Register special ejref_tolerance processor
    ejrefToleranceProcessor = std::make_shared<EjrefToleranceProcessor>();
}

void ProcessorFactory::RegisterProcessor(const std::u8string& type, std::shared_ptr<FileProcessor> processor)
{
    processors[type] = processor;
}

std::shared_ptr<FileProcessor> ProcessorFactory::GetProcessor(const std::u8string& type)
{
    auto it = processors.find(type);
    if (it != processors.end())
    {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<FileProcessor> ProcessorFactory::GetEjrefToleranceProcessor()
{
    return ejrefToleranceProcessor;
}
