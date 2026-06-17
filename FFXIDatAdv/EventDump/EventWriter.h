#pragma once

#include "Models.h"
#include <string>
#include <vector>
#include <filesystem>

class EventWriter
{
public:
	EventWriter(const std::filesystem::path& outputDir, bool pretty = false);

	bool WriteZoneIndex(const ZoneIndex& index, const std::string& zone_name);

	bool WriteActorFile(const ResolvedActor& actor, const std::string& zone_name, bool splitText = false);
	bool WriteActorFile(const ResolvedActor& actor, const std::string& zone_name, const std::string& explicitFilename, bool splitText = false);

	bool WriteCommonActorFile(const CommonActorData& actor);

	bool WriteTextFile(const std::string& textRef, const std::string& lang,
		const std::string& actorName, uint32_t eventId,
		const std::vector<DialogueLine>& dialogues);

	bool WriteCommonIndex(const std::vector<CommonActorData>& commonActors);

	bool WriteMasterIndex(
		const std::vector<ZoneIndex>& zones,
		const std::vector<CommonActorData>& commonActors);

	// text_ref paths are lang-agnostic: "texts/common/<Actor>/<EID>.json"
	// actual files are written to: texts/<lang>/common/<Actor>/<EID>.json
	std::string MakeTextRefCommon(const std::string& actorName, uint32_t eventId) const;
	std::string MakeTextRefZone(const std::string& zoneName, const std::string& actorName, uint32_t eventId) const;
	std::string MakeActorFilename(const std::string& actorName, uint32_t actorNumber) const;

private:
	std::filesystem::path outputDir_;
	bool pretty_;

	std::string Indent(int level) const;
	std::string EscapeJson(const std::string& s) const;
	std::string ActorCategoryStr(ActorCategory cat) const;

	std::filesystem::path ZoneDir(const std::string& zone_name) const;
	std::filesystem::path CommonDir() const;
	std::filesystem::path TextDir(const std::string& lang) const;

	void WriteJson(const std::filesystem::path& path, const std::string& json);
};
