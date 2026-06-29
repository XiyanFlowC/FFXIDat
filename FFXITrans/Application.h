#pragma once
#include <string>
#include <vector>
#include <map>
#include "ProcessorUtils.h"

#define VERSION "0.35.1"

class Application
{
public:
	static Application& Instance();

	// Main entry points
	int Run(int argc, char** argv);
	int PrepareSourceData();
	int ProcessTranslations();

private:
	Application() = default;
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// Initialization
	bool Initialize();
	bool InitializeCodePages();
	bool LoadTranslations();

	// UI helpers
	void ShowUsage();

	// File processing
  std::vector<FileProcessDef> LoadFileDefinitions(bool respectExcludes = true);
	bool ProcessSingleFile(
		const FileProcessDef& fileDef,
		const std::map<std::u8string, FileProcessDef>& jpDefsByComment,
		int fileCounter,
		int totalFiles,
		bool overwrite);
};
