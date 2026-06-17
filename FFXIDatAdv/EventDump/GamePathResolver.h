#pragma once

#include <string>
#include <filesystem>

class GamePathResolver
{
public:
	static void Init();

	static std::string GetGameRoot();

	static std::string ResolvePath(const std::string& romPath);

	static bool HasGameRoot();

private:
	static std::string gameRoot;
};
