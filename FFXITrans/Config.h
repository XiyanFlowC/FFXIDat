#pragma once
#pragma once

#include <filesystem>
#include <string>
#include <set>

namespace fs = std::filesystem;

class Config
{
public:
    static Config& Instance();

    bool Initialize();
    bool LoadFromFile(const fs::path& configPath);

    // Path accessors
    const fs::path& GetGameRoot() const { return gameRoot; }
    const fs::path& GetProgRoot() const { return progRoot; }
    const fs::path& GetOutRoot() const { return outRoot; }
    void SetOutRoot(const fs::path& path) { outRoot = path; }

    // Mode flags
    bool IsEnglishMode() const { return englishMode; }
    bool IsInSituMode() const { return inSitu; }
    bool IsInSituNoPrompt() const { return inSituNoprompt; }
    bool IsBackupEnabled() const { return backupEnabled; }
    bool IsBackupNoPrompt() const { return backupNoprompt; }
    bool IsNoMismatchLog() const { return noMismatchLog; }
    bool IsEnAsJa() const { return enAsJa; }
    bool IsEjrefTolerance() const { return ejrefTolerance; }
    bool IsVerbose() const { return verbose; }
    bool IsNoName() const { return noname; }
    bool IsBilingual() const { return babelCurrentOriginal; }
    bool IsBabelEnabled() const { return babelCurrentOriginal || babelAlternateOriginal; }
    bool IsBabelCurrentOriginalEnabled() const { return babelCurrentOriginal; }
    bool IsBabelAlternateOriginalEnabled() const { return babelAlternateOriginal; }
	bool IsSamuraiJobTransNot() const { return samuraiJobTransNot; }
	bool IsMonkJobAbbreviated() const { return monkJobAbbreviated; }
	bool IsSamuraiJobSpecial() const { return samuraiJobSpecial; }

    // Excludes
    bool IsExcluded(const std::u8string& comment) const;

    // Mode setters
    void SetInSituMode(bool value) { inSitu = value; }
    void SetInSituNoPrompt(bool value) { inSituNoprompt = value; }
    void SetEnglishMode(bool value) { englishMode = value; }
    void SetNoMismatchLog(bool value) { noMismatchLog = value; }
    void SetEnAsJa(bool value) { enAsJa = value; }
    void SetEjrefTolerance(bool value) { ejrefTolerance = value; }
    void SetVerbose(bool value) { verbose = value; }
    void SetNoName(bool value) { noname = value; }
    void SetGameRoot(const fs::path& path) { gameRoot = path; }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    int InitializeFromRegistry();
    bool CheckConfigFileHasRequiredSettings(const fs::path& configPath, bool& hasGamePath, bool& hasEnglishMode);
    bool MatchWildcard(const std::u8string& text, const std::u8string& pattern) const;

    fs::path gameRoot;
    fs::path progRoot;
    fs::path outRoot = "./output";

    bool englishMode = false;
    bool inSitu = false;
    bool inSituNoprompt = false;
    bool backupEnabled = true;
    bool backupNoprompt = false;
    bool noMismatchLog = false;
    bool enAsJa = false;
    bool ejrefTolerance = false;
    bool verbose = false;
    bool noname = false;
    bool babelCurrentOriginal = false;
    bool babelAlternateOriginal = false;
    bool samuraiJobTransNot = true;
	bool monkJobAbbreviated = false;
	bool samuraiJobSpecial = false;

    std::set<std::u8string> excludes;
};
