#include "EventWriter.h"
#include <fstream>
#include <sstream>

EventWriter::EventWriter(const std::filesystem::path& outputDir, bool pretty)
	: outputDir_(outputDir), pretty_(pretty)
{
	std::filesystem::create_directories(outputDir_);
}

std::string EventWriter::Indent(int level) const
{
	if (!pretty_) return "";
	return std::string(static_cast<size_t>(level) * 2, ' ');
}

std::string EventWriter::EscapeJson(const std::string& s) const
{
	std::string out;
	out.reserve(s.size() + 2);
	for (char c : s)
	{
		switch (c)
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (static_cast<unsigned char>(c) < 0x20)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
				out += buf;
			}
			else
				out += c;
		}
	}
	return out;
}

std::string EventWriter::ActorCategoryStr(ActorCategory cat) const
{
	switch (cat)
	{
	case ActorCategory::private_: return "private";
	case ActorCategory::common: return "common";
	case ActorCategory::special: return "special";
	default: return "unknown";
	}
}

std::filesystem::path EventWriter::ZoneDir(const std::string& zone_name) const
{
	return outputDir_ / "zone" / zone_name;
}

std::filesystem::path EventWriter::CommonDir() const
{
	return outputDir_ / "common";
}

std::filesystem::path EventWriter::TextDir(const std::string& lang) const
{
	return outputDir_ / "texts" / lang;
}

static std::string SafeFilename(const std::string& name)
{
	if (name.empty()) return "_";
	std::string safe;
	safe.reserve(name.size());
	for (char c : name)
	{
		if (static_cast<unsigned char>(c) < 0x20 ||
			c == '\\' || c == '/' || c == ':' || c == '*' ||
			c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			safe += '_';
		else
			safe += c;
	}
	return safe;
}

static bool IsFilesystemSafe(const std::string& name)
{
	if (name.empty()) return false;
	const std::string reserved = "\\/:*?\"<>|";
	for (char c : name)
	{
		if (static_cast<unsigned char>(c) < 0x20) return false;
		if (reserved.find(c) != std::string::npos) return false;
	}
	return true;
}

std::string EventWriter::MakeTextRefCommon(const std::string& actorName, uint32_t eventId) const
{
	return "texts/common/" + SafeFilename(actorName) + "/" + std::to_string(eventId) + ".json";
}

std::string EventWriter::MakeTextRefZone(const std::string& zoneName, const std::string& actorName, uint32_t eventId) const
{
	return "texts/zone/" + SafeFilename(zoneName) + "/" + SafeFilename(actorName) + "/" + std::to_string(eventId) + ".json";
}

std::string EventWriter::MakeActorFilename(const std::string& actorName, uint32_t actorNumber) const
{
	if (actorName.empty() || actorName == "_")
		return std::to_string(actorNumber) + ".json";

	std::string safe = SafeFilename(actorName);
	if (safe == actorName)
		return actorName + ".json";

	return safe + "_" + std::to_string(actorNumber) + ".json";
}

void EventWriter::WriteJson(const std::filesystem::path& path, const std::string& json)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream file(path);
	if (file.is_open())
		file << json;
}

bool EventWriter::WriteZoneIndex(const ZoneIndex& index, const std::string& zone_name)
{
	std::filesystem::create_directories(ZoneDir(zone_name));
	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"zone_id\": " << index.zone_id << ",\n";
	json << Indent(1) << "\"zone_name\": \"" << EscapeJson(index.zone_name) << "\",\n";
	json << Indent(1) << "\"actors\": [\n";

	for (size_t i = 0; i < index.entries.size(); ++i)
	{
		const auto& entry = index.entries[i];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"actor_number\": " << entry.actor_number << ",\n";
		json << Indent(3) << "\"actor_name\": \"" << EscapeJson(entry.actor_name) << "\",\n";
		json << Indent(3) << "\"category\": \"" << ActorCategoryStr(entry.category) << "\"";

		if (entry.category == ActorCategory::common)
		{
			json << ",\n" << Indent(3) << "\"common_ref\": \"../../common/" << SafeFilename(entry.actor_name) << ".json\"";
		}
		else
		{
			// Use local_ref if set (for deduplicated filenames), otherwise compute from name+number
			std::string ref = entry.local_ref.empty()
				? MakeActorFilename(entry.actor_name, entry.actor_number)
				: entry.local_ref;
			json << ",\n" << Indent(3) << "\"ref\": \"" << ref << "\"";
		}

		json << "\n" << Indent(2) << "}";
		if (i + 1 < index.entries.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "]\n";
	json << "}\n";

	WriteJson(ZoneDir(index.zone_name) / "index.json", json.str());
	return true;
}

bool EventWriter::WriteTextFile(const std::string& textRef, const std::string& lang,
	const std::string& actorName, uint32_t eventId,
	const std::vector<DialogueLine>& dialogues)
{
	// textRef is lang-agnostic: "texts/common/<Actor>/<EID>.json"
	// The lang suffix goes right after "texts/":
	//   lang=="na"  → "texts/na/common/<Actor>/<EID>.json"
	//   lang==""    → "texts/common/<Actor>/<EID>.json"  (for backward compat)
	std::string actualRef;
	if (lang.empty())
		actualRef = textRef;
	else
		actualRef = "texts/" + lang + textRef.substr(5); // skip "texts"

	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"actor_name\": \"" << EscapeJson(actorName) << "\",\n";
	json << Indent(1) << "\"event_id\": " << eventId << ",\n";
	json << Indent(1) << "\"dialogues\": [\n";

	for (size_t j = 0; j < dialogues.size(); ++j)
	{
		const auto& dl = dialogues[j];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"speaker\": \"" << EscapeJson(dl.speaker) << "\",\n";
		json << Indent(3) << "\"text\": \"" << EscapeJson(std::string(dl.text.begin(), dl.text.end())) << "\"\n";
		json << Indent(2) << "}";
		if (j + 1 < dialogues.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "]\n";
	json << "}\n";

	WriteJson(outputDir_ / actualRef, json.str());
	return true;
}

bool EventWriter::WriteActorFile(const ResolvedActor& actor, const std::string& zone_name, const std::string& explicitFilename, bool splitText)
{
	if (actor.events.empty())
		return false;

	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"actor_number\": " << actor.actor_number << ",\n";
	json << Indent(1) << "\"actor_name\": \"" << EscapeJson(actor.actor_name) << "\",\n";
	json << Indent(1) << "\"category\": \"" << ActorCategoryStr(actor.category) << "\",\n";

	// imed_data
	json << Indent(1) << "\"imed_data\": [";
	for (size_t i = 0; i < actor.imed_data.size(); ++i)
	{
		if (i > 0) json << ", ";
		json << actor.imed_data[i];
	}
	json << "],\n";

	// events
	json << Indent(1) << "\"events\": [\n";
	for (size_t i = 0; i < actor.events.size(); ++i)
	{
		const auto& evt = actor.events[i];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"index\": " << evt.array_index << ",\n";
		json << Indent(3) << "\"event_id\": " << evt.event_id << ",\n";
		json << Indent(3) << "\"size\": " << evt.byte_size << ",\n";

		if (splitText && !evt.text_ref.empty())
		{
			json << Indent(3) << "\"text_ref\": \"" << EscapeJson(evt.text_ref) << "\"\n";
		}
		else
		{
			json << Indent(3) << "\"dialogues\": [\n";
			for (size_t j = 0; j < evt.dialogues.size(); ++j)
			{
				const auto& dl = evt.dialogues[j];
				json << Indent(4) << "{\n";
				json << Indent(5) << "\"speaker\": \"" << EscapeJson(dl.speaker) << "\",\n";
				json << Indent(5) << "\"text\": \"" << EscapeJson(std::string(dl.text.begin(), dl.text.end())) << "\"\n";
				json << Indent(4) << "}";
				if (j + 1 < evt.dialogues.size())
					json << ",";
				json << "\n";
			}
			json << Indent(3) << "]\n";
		}

		if (!evt.opcodes.empty())
		{
			json << ",\n" << Indent(3) << "\"opcodes\": [\n";
			for (size_t j = 0; j < evt.opcodes.size(); ++j)
			{
				json << Indent(4) << "\"" << EscapeJson(evt.opcodes[j]) << "\"";
				if (j + 1 < evt.opcodes.size())
					json << ",";
				json << "\n";
			}
			json << Indent(3) << "]\n";
		}

		json << Indent(2) << "}";
		if (i + 1 < actor.events.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "]\n";
	json << "}\n";

	auto filepath = ZoneDir(zone_name) / explicitFilename;
	WriteJson(filepath, json.str());
	return true;
}

bool EventWriter::WriteActorFile(const ResolvedActor& actor, const std::string& zone_name, bool splitText)
{
	return WriteActorFile(actor, zone_name, MakeActorFilename(actor.actor_name, actor.actor_number), splitText);
}

bool EventWriter::WriteCommonActorFile(const CommonActorData& actor)
{
	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"actor_name\": \"" << EscapeJson(actor.actor_name) << "\",\n";
	json << Indent(1) << "\"category\": \"common\",\n";
	json << Indent(1) << "\"verified\": " << (actor.verified ? "true" : "false") << ",\n";

	json << Indent(1) << "\"zones\": [";
	for (size_t i = 0; i < actor.zone_names.size(); ++i)
	{
		if (i > 0) json << ", ";
		json << "\"" << EscapeJson(actor.zone_names[i]) << "\"";
	}
	json << "],\n";

	json << Indent(1) << "\"events\": [\n";
	for (size_t i = 0; i < actor.events.size(); ++i)
	{
		const auto& evt = actor.events[i];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"index\": " << evt.array_index << ",\n";
		json << Indent(3) << "\"event_id\": " << evt.event_id << ",\n";
		json << Indent(3) << "\"size\": " << evt.byte_size << ",\n";

		if (!evt.text_ref.empty())
		{
			json << Indent(3) << "\"text_ref\": \"" << EscapeJson(evt.text_ref) << "\"";
		}
		else
		{
			json << Indent(3) << "\"dialogues\": [\n";
			for (size_t j = 0; j < evt.dialogues.size(); ++j)
			{
				const auto& dl = evt.dialogues[j];
				json << Indent(4) << "{\n";
				json << Indent(5) << "\"speaker\": \"" << EscapeJson(dl.speaker) << "\",\n";
				json << Indent(5) << "\"text\": \"" << EscapeJson(std::string(dl.text.begin(), dl.text.end())) << "\"\n";
				json << Indent(4) << "}";
				if (j + 1 < evt.dialogues.size())
					json << ",";
				json << "\n";
			}
			json << Indent(3) << "]";
		}

		if (!evt.opcodes.empty())
		{
			if (!evt.text_ref.empty())
				json << ",\n";
			else
				json << ",\n";
			json << Indent(3) << "\"opcodes\": [\n";
			for (size_t j = 0; j < evt.opcodes.size(); ++j)
			{
				json << Indent(4) << "\"" << EscapeJson(evt.opcodes[j]) << "\"";
				if (j + 1 < evt.opcodes.size())
					json << ",";
				json << "\n";
			}
			json << Indent(3) << "]\n";
		}
		else
		{
			json << "\n";
		}

		json << Indent(2) << "}";
		if (i + 1 < actor.events.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "]\n";
	json << "}\n";

	auto filepath = CommonDir() / MakeActorFilename(actor.actor_name, 0);
	WriteJson(filepath, json.str());
	return true;
}

bool EventWriter::WriteCommonIndex(const std::vector<CommonActorData>& commonActors)
{
	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"format_version\": 1,\n";
	json << Indent(1) << "\"actors\": [\n";

	for (size_t i = 0; i < commonActors.size(); ++i)
	{
		const auto& actor = commonActors[i];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"actor_name\": \"" << EscapeJson(actor.actor_name) << "\",\n";
		json << Indent(3) << "\"verified\": " << (actor.verified ? "true" : "false") << ",\n";
		json << Indent(3) << "\"zone_count\": " << actor.zone_names.size() << "\n";
		json << Indent(2) << "}";
		if (i + 1 < commonActors.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "]\n";
	json << "}\n";

	std::filesystem::create_directories(CommonDir());
	WriteJson(CommonDir() / "index.json", json.str());
	return true;
}

bool EventWriter::WriteMasterIndex(
	const std::vector<ZoneIndex>& zones,
	const std::vector<CommonActorData>& commonActors)
{
	std::ostringstream json;
	json << "{\n";
	json << Indent(1) << "\"format_version\": 1,\n";
	json << Indent(1) << "\"zones\": [\n";

	for (size_t i = 0; i < zones.size(); ++i)
	{
		const auto& z = zones[i];
		json << Indent(2) << "{\n";
		json << Indent(3) << "\"zone_id\": " << z.zone_id << ",\n";
		json << Indent(3) << "\"zone_name\": \"" << EscapeJson(z.zone_name) << "\",\n";
		json << Indent(3) << "\"actor_count\": " << z.entries.size() << ",\n";
		json << Indent(3) << "\"index_ref\": \"zone/" << z.zone_name << "/index.json\"\n";
		json << Indent(2) << "}";
		if (i + 1 < zones.size())
			json << ",";
		json << "\n";
	}

	json << Indent(1) << "],\n";
	json << Indent(1) << "\"common_actor_count\": " << commonActors.size() << "\n";
	json << "}\n";

	WriteJson(outputDir_ / "index.json", json.str());
	return true;
}
