#pragma once
#include <filesystem>

class SetupWizard
{
public:
	// Returns true when wizard handled startup and caller should exit.
	static bool RunIfConfigMissing(const std::filesystem::path& progRoot);
};
