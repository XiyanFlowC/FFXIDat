#pragma once
#include "FileProcessor.h"
#include <memory>
#include <map>
#include <string>

class ProcessorFactory
{
public:
    static ProcessorFactory& Instance();

    // Get processor for a given file type
    std::shared_ptr<FileProcessor> GetProcessor(const std::u8string& type);

    // Get special ejref_tolerance processor
    std::shared_ptr<FileProcessor> GetEjrefToleranceProcessor();

    // Register a processor
    void RegisterProcessor(const std::u8string& type, std::shared_ptr<FileProcessor> processor);

private:
    ProcessorFactory();
    ProcessorFactory(const ProcessorFactory&) = delete;
    ProcessorFactory& operator=(const ProcessorFactory&) = delete;

    void RegisterDefaultProcessors();

    std::map<std::u8string, std::shared_ptr<FileProcessor>> processors;
    std::shared_ptr<FileProcessor> ejrefToleranceProcessor;
};
