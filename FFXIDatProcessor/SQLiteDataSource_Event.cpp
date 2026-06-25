// SQLiteDataSource_Event.cpp
// EventAdvImport / EventDbDump �� called from FFXIDatAdv.
// No FFXIDatAdv headers are included here; communication uses SQLiteDataSource::AdvActor POD structs.

#include "SQLiteDataSource.h"
#include "sqlite3/sqlite3.h"
#include "DataManager.h"
#include "xystring.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_map>

// --- helpers ----------------------------------------------------------------

static int UpsertZone(sqlite3* db, const std::string& zoneName)
{
	sqlite3_stmt* stmt = nullptr;
	int id = -1;
	if (sqlite3_prepare_v2(db,
		"INSERT INTO event_zone(zone_name) VALUES(?)"
		" ON CONFLICT(zone_name) DO UPDATE SET zone_name=zone_name RETURNING id",
		-1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, zoneName.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			id = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
	}
	return id;
}

static int UpsertText(sqlite3* db, const char * text)
{
	sqlite3_stmt* stmt = nullptr;
	int id = -1;
	if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO text(text) VALUES(?)", -1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, text, -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	if (sqlite3_prepare_v2(db, "SELECT id FROM text WHERE text = ?", -1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, text, -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			id = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
	}
	return id;
}

// Insert/update a single event_msg row.
// Rows with status=3 (user-verified) are never modified.
static void UpsertMsg(sqlite3* db, int eventId, uint32_t msgIndex,
	const std::string& speaker, int textJaId, int textEnId)
{
	sqlite3_stmt* stmt = nullptr;
	const char* sql = R"(
		INSERT INTO event_msg(event_id, msg_index, speaker, text_ja_id, text_en_id, status)
		VALUES (?, ?, ?, ?, ?, 2)
		ON CONFLICT(event_id, msg_index) DO UPDATE SET
			speaker    = CASE WHEN status = 3 THEN speaker    ELSE excluded.speaker    END,
			text_ja_id = CASE WHEN status = 3 THEN text_ja_id ELSE COALESCE(excluded.text_ja_id, text_ja_id) END,
			text_en_id = CASE WHEN status = 3 THEN text_en_id ELSE COALESCE(excluded.text_en_id, text_en_id) END,
			status     = CASE WHEN status = 3 THEN 3           ELSE 2                  END
	)";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, eventId);
		sqlite3_bind_int(stmt, 2, (int)msgIndex);
		if (!speaker.empty())
			sqlite3_bind_text(stmt, 3, speaker.c_str(), -1, SQLITE_TRANSIENT);
		else
			sqlite3_bind_null(stmt, 3);
		if (textJaId >= 0) sqlite3_bind_int(stmt, 4, textJaId); else sqlite3_bind_null(stmt, 4);
		if (textEnId >= 0) sqlite3_bind_int(stmt, 5, textEnId); else sqlite3_bind_null(stmt, 5);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

// --- EventDbImport ---------------------------------------------------------

void SQLiteDataSource::EventDbImport(const std::vector<Actor>& actors)
{
	Execute("BEGIN;");

	std::cout << "[EventDbImport] Importing " << actors.size() << " (canonical) actors..." << std::endl;
	size_t actorCount = 0;

	// Declare prepared statements at function scope so catch can see them
	sqlite3_stmt *insCommon = nullptr, *selCommon = nullptr;
	sqlite3_stmt *insPrivate = nullptr, *selPrivate = nullptr;
	sqlite3_stmt *insAlias = nullptr;
	sqlite3_stmt *delAlias = nullptr;
	sqlite3_stmt *delDirect = nullptr;
	sqlite3_stmt *insEvt = nullptr, *selEvt = nullptr;
	sqlite3_stmt *delMsg = nullptr;
	sqlite3_stmt *insMsg = nullptr;

	try
	{
		// In-memory caches (big win for repeated zones/texts)
		std::unordered_map<std::string, int> zoneCache;
		auto getZone = [&](const std::string& zn) -> int {
			auto it = zoneCache.find(zn);
			if (it != zoneCache.end()) return it->second;
			int id = UpsertZone(db, zn);
			if (id >= 0) zoneCache[zn] = id;
			return id;
		};

		std::unordered_map<std::u8string, int> textCache;
		auto getText = [&](const std::u8string& t) -> int {
			if (t.empty()) return -1;
			auto it = textCache.find(t);
			if (it != textCache.end()) return it->second;
			int id = UpsertText(db, reinterpret_cast<const char*>(t.c_str()));
			if (id >= 0) textCache[t] = id;
			return id;
		};

		sqlite3_stmt* stmt = nullptr;

		// Prepare hot statements ONCE (huge win - avoids repeated SQL parsing)

		const char* sqlInsCommon =
			"INSERT INTO event_actor(zone_id, actor_no, actor_name) VALUES(NULL,NULL,?)"
			" ON CONFLICT(actor_name) WHERE zone_id IS NULL"
			" RETURNING id";
		sqlite3_prepare_v2(db, sqlInsCommon, -1, &insCommon, nullptr);

		const char* sqlSelCommon = "SELECT id FROM event_actor WHERE zone_id IS NULL AND actor_name = ?";
		sqlite3_prepare_v2(db, sqlSelCommon, -1, &selCommon, nullptr);

		const char* sqlInsPrivate =
			"INSERT INTO event_actor(zone_id, actor_no, actor_name) VALUES(?,?,?)"
			" ON CONFLICT(zone_id, actor_no) WHERE zone_id IS NOT NULL"
			" DO UPDATE SET actor_name=excluded.actor_name RETURNING id";
		sqlite3_prepare_v2(db, sqlInsPrivate, -1, &insPrivate, nullptr);

		const char* sqlSelPrivate = "SELECT id FROM event_actor WHERE zone_id = ? AND actor_no = ?";
		sqlite3_prepare_v2(db, sqlSelPrivate, -1, &selPrivate, nullptr);

		const char* sqlInsAlias =
			"INSERT INTO event_actor_alias(actor_id, zone_id, actor_no) VALUES(?,?,?)"
			" ON CONFLICT(zone_id, actor_no) DO UPDATE SET actor_id=excluded.actor_id";
		sqlite3_prepare_v2(db, sqlInsAlias, -1, &insAlias, nullptr);

		const char* sqlDelAlias = "DELETE FROM event_actor_alias WHERE zone_id = ? AND actor_no = ?";
		sqlite3_prepare_v2(db, sqlDelAlias, -1, &delAlias, nullptr);

		const char* sqlDelDirect = "DELETE FROM event_actor WHERE zone_id = ? AND actor_no = ?";
		sqlite3_prepare_v2(db, sqlDelDirect, -1, &delDirect, nullptr);

		const char* sqlInsEvt =
			"INSERT INTO event_event(actor_id, event_no, event_index) VALUES(?,?,?)"
			" ON CONFLICT(actor_id, event_no, event_index) DO UPDATE SET event_index=event_index RETURNING id";
		sqlite3_prepare_v2(db, sqlInsEvt, -1, &insEvt, nullptr);

		const char* sqlSelEvt = "SELECT id FROM event_event WHERE actor_id=? AND event_no=? AND event_index=?";
		sqlite3_prepare_v2(db, sqlSelEvt, -1, &selEvt, nullptr);

		const char* sqlDelExcessMsg =
			"DELETE FROM event_msg WHERE event_id = ? AND status <> 3 AND msg_index >= ?";
		sqlite3_prepare_v2(db, sqlDelExcessMsg, -1, &delMsg, nullptr);

		const char* sqlInsMsg = R"(
			INSERT INTO event_msg(event_id, msg_index, speaker, text_ja_id, text_en_id, status)
			VALUES (?, ?, ?, ?, ?, 2)
			ON CONFLICT(event_id, msg_index) DO UPDATE SET
				speaker    = CASE WHEN status = 3 THEN speaker    ELSE excluded.speaker    END,
				text_ja_id = CASE WHEN status = 3 THEN text_ja_id ELSE COALESCE(excluded.text_ja_id, text_ja_id) END,
				text_en_id = CASE WHEN status = 3 THEN text_en_id ELSE COALESCE(excluded.text_en_id, text_en_id) END,
				status     = CASE WHEN status = 3 THEN 3           ELSE 2                  END
		)";
		sqlite3_prepare_v2(db, sqlInsMsg, -1, &insMsg, nullptr);

		for (const auto& actor : actors)
		{
			bool isCommon = actor.zone_name.empty();
			int actorId = -1;

			if (isCommon)
			{
				// Upsert canonical common (reuse prepared)
				if (insCommon) {
					sqlite3_reset(insCommon);
					sqlite3_bind_text(insCommon, 1, actor.actor_name.c_str(), -1, SQLITE_TRANSIENT);
					if (sqlite3_step(insCommon) == SQLITE_ROW)
						actorId = sqlite3_column_int(insCommon, 0);
				}
				if (actorId < 0 && selCommon) {
					sqlite3_reset(selCommon);
					sqlite3_bind_text(selCommon, 1, actor.actor_name.c_str(), -1, SQLITE_TRANSIENT);
					if (sqlite3_step(selCommon) == SQLITE_ROW)
						actorId = sqlite3_column_int(selCommon, 0);
				}
				if (actorId < 0) continue;

				++actorCount;
				if (actorCount % 20 == 0 || actorCount == actors.size())
					std::cout << "  [" << actorCount << "/" << actors.size() << "] common: " << actor.actor_name << std::endl;

				// aliases + stray cleanup (use prepared + cache)
				for (size_t i = 0; i < actor.alias_zones.size(); ++i)
				{
					if (i >= actor.alias_actor_nos.size()) break;
					int zoneId = getZone(actor.alias_zones[i]);
					if (zoneId < 0) continue;

					// stray direct
					if (delDirect) {
						sqlite3_reset(delDirect);
						sqlite3_bind_int(delDirect, 1, zoneId);
						sqlite3_bind_int(delDirect, 2, (int)actor.alias_actor_nos[i]);
						sqlite3_step(delDirect);
					}

					if (insAlias) {
						sqlite3_reset(insAlias);
						sqlite3_bind_int(insAlias, 1, actorId);
						sqlite3_bind_int(insAlias, 2, zoneId);
						sqlite3_bind_int(insAlias, 3, (int)actor.alias_actor_nos[i]);
						sqlite3_step(insAlias);
					}
				}
			}
			else
			{
				// Private (zone-scoped) actor. Use prepared + cache
				int zoneId = getZone(actor.zone_name);
				if (zoneId < 0) continue;

				if (insPrivate) {
					sqlite3_reset(insPrivate);
					sqlite3_bind_int(insPrivate, 1, zoneId);
					sqlite3_bind_int(insPrivate, 2, (int)actor.actor_no);
					sqlite3_bind_text(insPrivate, 3, actor.actor_name.c_str(), -1, SQLITE_TRANSIENT);
					if (sqlite3_step(insPrivate) == SQLITE_ROW)
						actorId = sqlite3_column_int(insPrivate, 0);
				}
				if (actorId < 0 && selPrivate) {
					sqlite3_reset(selPrivate);
					sqlite3_bind_int(selPrivate, 1, zoneId);
					sqlite3_bind_int(selPrivate, 2, (int)actor.actor_no);
					if (sqlite3_step(selPrivate) == SQLITE_ROW)
						actorId = sqlite3_column_int(selPrivate, 0);
				}
				if (actorId < 0) continue;

				++actorCount;
				if (actorCount % 20 == 0 || actorCount == actors.size())
					std::cout << "  [" << actorCount << "/" << actors.size() << "] private: " << actor.actor_name << " (zone " << actor.zone_name << ")" << std::endl;

				// Remove any alias hijacking the primary slot
				if (delAlias) {
					sqlite3_reset(delAlias);
					sqlite3_bind_int(delAlias, 1, zoneId);
					sqlite3_bind_int(delAlias, 2, (int)actor.actor_no);
					sqlite3_step(delAlias);
				}

				// aliases for other slots (shared within or across zones)
				for (size_t i = 0; i < actor.alias_zones.size(); ++i)
				{
					if (i >= actor.alias_actor_nos.size()) break;
					const std::string& azone = actor.alias_zones[i];
					int32_t ano = actor.alias_actor_nos[i];
					if (azone == actor.zone_name && ano == actor.actor_no) continue;
					int azid = getZone(azone);
					if (azid < 0) continue;

					// stray direct
					if (delDirect) {
						sqlite3_reset(delDirect);
						sqlite3_bind_int(delDirect, 1, azid);
						sqlite3_bind_int(delDirect, 2, (int)ano);
						sqlite3_step(delDirect);
					}

					if (insAlias) {
						sqlite3_reset(insAlias);
						sqlite3_bind_int(insAlias, 1, actorId);
						sqlite3_bind_int(insAlias, 2, azid);
						sqlite3_bind_int(insAlias, 3, (int)ano);
						sqlite3_step(insAlias);
					}
				}
			}

			// Store events (prepared once outside)
			for (const auto& evt : actor.events)
			{
				int eventId = -1;
				if (insEvt) {
					sqlite3_reset(insEvt);
					sqlite3_bind_int(insEvt, 1, actorId);
					sqlite3_bind_int(insEvt, 2, (int)evt.event_no);
					sqlite3_bind_int(insEvt, 3, (int)evt.event_index);
					if (sqlite3_step(insEvt) == SQLITE_ROW)
						eventId = sqlite3_column_int(insEvt, 0);
				}
				if (eventId < 0 && selEvt) {
					sqlite3_reset(selEvt);
					sqlite3_bind_int(selEvt, 1, actorId);
					sqlite3_bind_int(selEvt, 2, (int)evt.event_no);
					sqlite3_bind_int(selEvt, 3, (int)evt.event_index);
					if (sqlite3_step(selEvt) == SQLITE_ROW)
						eventId = sqlite3_column_int(selEvt, 0);
				}
				if (eventId < 0) continue;

				// delete excess msgs (prepared)
				if (delMsg) {
					sqlite3_reset(delMsg);
					sqlite3_bind_int(delMsg, 1, eventId);
					sqlite3_bind_int(delMsg, 2, static_cast<int>(evt.dialogues.size()));
					sqlite3_step(delMsg);
				}

				// messages - use cached texts + prepared msg stmt
				for (size_t msgIndex = 0; msgIndex < evt.dialogues.size(); ++msgIndex)
				{
					const auto& dl = evt.dialogues[msgIndex];
					int jaId = getText(dl.text_ja);
					int enId = getText(dl.text_en);

					if (insMsg) {
						sqlite3_reset(insMsg);
						sqlite3_bind_int(insMsg, 1, eventId);
						sqlite3_bind_int(insMsg, 2, (int)msgIndex);
						if (!dl.speaker.empty())
							sqlite3_bind_text(insMsg, 3, dl.speaker.c_str(), -1, SQLITE_TRANSIENT);
						else
							sqlite3_bind_null(insMsg, 3);
						if (jaId >= 0) sqlite3_bind_int(insMsg, 4, jaId); else sqlite3_bind_null(insMsg, 4);
						if (enId >= 0) sqlite3_bind_int(insMsg, 5, enId); else sqlite3_bind_null(insMsg, 5);
						sqlite3_step(insMsg);
					}
				}
			}
		}

		std::cout << "[EventDbImport] Pruning orphan (unreferenced) actors..." << std::endl;
		// Final light orphan cleanup for unclean remains from previous dedup runs.
		// No blanket truncation - stable IDs for unchanged data are preserved.
		Execute(R"(DELETE FROM event_actor
			WHERE id NOT IN (SELECT actor_id FROM event_event)
			  AND id NOT IN (SELECT actor_id FROM event_actor_alias)
		)");

		// Finalize all prepared statements
		sqlite3_finalize(insCommon);
		sqlite3_finalize(selCommon);
		sqlite3_finalize(insPrivate);
		sqlite3_finalize(selPrivate);
		sqlite3_finalize(insAlias);
		sqlite3_finalize(delAlias);
		sqlite3_finalize(delDirect);
		sqlite3_finalize(insEvt);
		sqlite3_finalize(selEvt);
		sqlite3_finalize(delMsg);
		sqlite3_finalize(insMsg);

		std::cout << "[EventDbImport] Import complete." << std::endl;
		Execute("COMMIT;");
	}
	catch (...)
	{
		// best effort finalize on error path
		sqlite3_finalize(insCommon); sqlite3_finalize(selCommon);
		sqlite3_finalize(insPrivate); sqlite3_finalize(selPrivate);
		sqlite3_finalize(insAlias); sqlite3_finalize(delAlias);
		sqlite3_finalize(delDirect);
		sqlite3_finalize(insEvt); sqlite3_finalize(selEvt);
		sqlite3_finalize(delMsg); sqlite3_finalize(insMsg);
		Execute("ROLLBACK;");
		throw;
	}
}

// --- EventDbDump ------------------------------------------------------------

static std::string EscapeJson(const std::string& s)
{
	std::string r;
	for (unsigned char c : s)
	{
		switch (c)
		{
		case '"':  r += "\\\""; break;
		case '\\': r += "\\\\"; break;
		case '\n': r += "\\n";  break;
		case '\r': r += "\\r";  break;
		case '\t': r += "\\t";  break;
		default:
			if (c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); r += buf; }
			else r += (char)c;
		}
	}
	return r;
}

static void WriteJson(const std::filesystem::path& path, const std::string& json)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream f(path);
	f << json;
}

static std::string SafeName(const std::string& s)
{
	if (s.empty()) return "_";
	std::string r;
	for (char c : s)
	{
		if ((unsigned char)c < 0x20 || c == '\\' || c == '/' || c == ':' ||
			c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			r += '_';
		else
			r += c;
	}
	return r.empty() ? "_" : r;
}

void SQLiteDataSource::EventDbDump(const std::string& outputDir, const std::string& lang)
{
	namespace fs = std::filesystem;
	fs::path root(outputDir);
	fs::create_directories(root);

	// Choose which text column to use for dialogue text.
	std::string textCol = (lang == "ja") ? "text_ja_id" : "text_en_id";

	sqlite3_stmt* stmt = nullptr;

	// -- 1. Common actors (zone_id IS NULL) ----------------------------------
	{
		fs::path commonDir = root / "common";
		fs::create_directories(commonDir);

		// Collect all common actor ids + names.
		struct CommonActor { int id; std::string name; };
		std::vector<CommonActor> commonActors;

		if (sqlite3_prepare_v2(db,
			"SELECT id, actor_name FROM event_actor WHERE zone_id IS NULL ORDER BY actor_name",
			-1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				CommonActor ca;
				ca.id   = sqlite3_column_int(stmt, 0);
				ca.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
				commonActors.push_back(ca);
			}
			sqlite3_finalize(stmt);
		}

		for (const auto& ca : commonActors)
		{
			// Zone names this actor appears in (via alias).
			std::vector<std::string> zoneNames;
			if (sqlite3_prepare_v2(db,
				"SELECT z.zone_name FROM event_actor_alias a"
				" JOIN event_zone z ON z.id = a.zone_id"
				" WHERE a.actor_id = ? ORDER BY z.zone_name",
				-1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, ca.id);
				while (sqlite3_step(stmt) == SQLITE_ROW)
					zoneNames.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
				sqlite3_finalize(stmt);
			}

			// Events + messages.
			sqlite3_stmt* evStmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT id, event_no, event_index FROM event_event WHERE actor_id = ? ORDER BY event_no, event_index",
				-1, &evStmt, nullptr) != SQLITE_OK) continue;
			sqlite3_bind_int(evStmt, 1, ca.id);

			std::ostringstream json;
			json << "{\n";
			json << "  \"actor_name\": \"" << EscapeJson(ca.name) << "\",\n";
			json << "  \"category\": \"common\",\n";
			json << "  \"zones\": [";
			for (size_t i = 0; i < zoneNames.size(); ++i)
			{
				if (i) json << ", ";
				json << "\"" << EscapeJson(zoneNames[i]) << "\"";
			}
			json << "],\n";
			json << "  \"events\": [\n";

			bool firstEvent = true;
			while (sqlite3_step(evStmt) == SQLITE_ROW)
			{
				int eventId    = sqlite3_column_int(evStmt, 0);
				int eventNo    = sqlite3_column_int(evStmt, 1);
				int eventIndex = sqlite3_column_int(evStmt, 2);

				sqlite3_stmt* msgStmt = nullptr;
				std::string msgSql =
					"SELECT m.msg_index, m.speaker, t.text FROM event_msg m"
					" LEFT JOIN text t ON t.id = m." + textCol +
					" WHERE m.event_id = ? ORDER BY m.msg_index";
				if (sqlite3_prepare_v2(db, msgSql.c_str(), -1, &msgStmt, nullptr) != SQLITE_OK) continue;
				sqlite3_bind_int(msgStmt, 1, eventId);

				std::vector<std::tuple<int, std::string, std::string>> msgs;
				while (sqlite3_step(msgStmt) == SQLITE_ROW)
				{
					int idx = sqlite3_column_int(msgStmt, 0);
					std::string speaker = sqlite3_column_text(msgStmt, 1)
						? reinterpret_cast<const char*>(sqlite3_column_text(msgStmt, 1)) : "";
					std::string text = sqlite3_column_text(msgStmt, 2)
						? reinterpret_cast<const char*>(sqlite3_column_text(msgStmt, 2)) : "";
					msgs.emplace_back(idx, speaker, text);
				}
				sqlite3_finalize(msgStmt);
				if (msgs.empty()) continue;

				if (!firstEvent) json << ",\n";
				firstEvent = false;
				json << "    {\n";
				json << "      \"index\": " << eventIndex << ",\n";
				json << "      \"event_id\": " << eventNo << ",\n";
				json << "      \"dialogues\": [\n";
				for (size_t mi = 0; mi < msgs.size(); ++mi)
				{
					if (mi) json << ",\n";
					json << "        {\"speaker\": \"" << EscapeJson(std::get<1>(msgs[mi]))
						 << "\", \"text\": \"" << EscapeJson(std::get<2>(msgs[mi])) << "\"}";
				}
				json << "\n      ]\n";
				json << "    }";
			}
			sqlite3_finalize(evStmt);

			json << "\n  ]\n}\n";
			WriteJson(commonDir / (SafeName(ca.name) + ".json"), json.str());
		}

		// Write common/index.json
		{
			std::ostringstream idx;
			idx << "{\n  \"actors\": [\n";
			for (size_t i = 0; i < commonActors.size(); ++i)
			{
				if (i) idx << ",\n";
				idx << "    {\"actor_name\": \"" << EscapeJson(commonActors[i].name) << "\"}";
			}
			idx << "\n  ]\n}\n";
			WriteJson(commonDir / "index.json", idx.str());
		}
	}

	// -- 2. Per-zone actors + indices -----------------------------------------
	{
		struct ZoneInfo { int id; std::string name; };
		std::vector<ZoneInfo> zones;
		if (sqlite3_prepare_v2(db, "SELECT id, zone_name FROM event_zone ORDER BY zone_name", -1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				ZoneInfo zi;
				zi.id   = sqlite3_column_int(stmt, 0);
				zi.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
				zones.push_back(zi);
			}
			sqlite3_finalize(stmt);
		}

		for (const auto& zone : zones)
		{
			fs::path zoneDir = root / "zone" / zone.name;
			fs::create_directories(zoneDir);

			std::ostringstream zoneIndex;
			zoneIndex << "{\n";
			zoneIndex << "  \"zone_name\": \"" << EscapeJson(zone.name) << "\",\n";
			zoneIndex << "  \"actors\": [\n";
			bool firstActor = true;

			// Private actors in this zone.
			sqlite3_stmt* actStmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT id, actor_no, actor_name FROM event_actor"
				" WHERE zone_id = ? ORDER BY actor_no",
				-1, &actStmt, nullptr) != SQLITE_OK) continue;
			sqlite3_bind_int(actStmt, 1, zone.id);

			while (sqlite3_step(actStmt) == SQLITE_ROW)
			{
				int actorId     = sqlite3_column_int(actStmt, 0);
				int actorNo     = sqlite3_column_int(actStmt, 1);
				std::string actorName = reinterpret_cast<const char*>(sqlite3_column_text(actStmt, 2));
				std::string fname = SafeName(actorName) + ".json";

				if (!firstActor) zoneIndex << ",\n";
				firstActor = false;
				zoneIndex << "    {\"actor_number\": " << actorNo
					<< ", \"actor_name\": \"" << EscapeJson(actorName)
					<< "\", \"category\": \"private\", \"ref\": \"" << fname << "\"}";

				// Write actor JSON.
				sqlite3_stmt* evStmt = nullptr;
				if (sqlite3_prepare_v2(db,
					"SELECT id, event_no, event_index FROM event_event"
					" WHERE actor_id = ? ORDER BY event_no, event_index",
					-1, &evStmt, nullptr) != SQLITE_OK) continue;
				sqlite3_bind_int(evStmt, 1, actorId);

				std::ostringstream json;
				json << "{\n";
				json << "  \"actor_number\": " << actorNo << ",\n";
				json << "  \"actor_name\": \"" << EscapeJson(actorName) << "\",\n";
				json << "  \"category\": \"private\",\n";
				json << "  \"events\": [\n";
				bool firstEvent = true;

				while (sqlite3_step(evStmt) == SQLITE_ROW)
				{
					int eventId    = sqlite3_column_int(evStmt, 0);
					int eventNo    = sqlite3_column_int(evStmt, 1);
					int eventIndex = sqlite3_column_int(evStmt, 2);

					sqlite3_stmt* msgStmt = nullptr;
					std::string msgSql =
						"SELECT m.msg_index, m.speaker, t.text FROM event_msg m"
						" LEFT JOIN text t ON t.id = m." + textCol +
						" WHERE m.event_id = ? ORDER BY m.msg_index";
					if (sqlite3_prepare_v2(db, msgSql.c_str(), -1, &msgStmt, nullptr) != SQLITE_OK) continue;
					sqlite3_bind_int(msgStmt, 1, eventId);

					std::vector<std::tuple<int, std::string, std::string>> msgs;
					while (sqlite3_step(msgStmt) == SQLITE_ROW)
					{
						int idx = sqlite3_column_int(msgStmt, 0);
						std::string sp = sqlite3_column_text(msgStmt, 1)
							? reinterpret_cast<const char*>(sqlite3_column_text(msgStmt, 1)) : "";
						std::string tx = sqlite3_column_text(msgStmt, 2)
							? reinterpret_cast<const char*>(sqlite3_column_text(msgStmt, 2)) : "";
						msgs.emplace_back(idx, sp, tx);
					}
					sqlite3_finalize(msgStmt);
					if (msgs.empty()) continue;

					if (!firstEvent) json << ",\n";
					firstEvent = false;
					json << "    {\n";
					json << "      \"index\": " << eventIndex << ",\n";
					json << "      \"event_id\": " << eventNo << ",\n";
					json << "      \"dialogues\": [\n";
					for (size_t mi = 0; mi < msgs.size(); ++mi)
					{
						if (mi) json << ",\n";
						json << "        {\"speaker\": \"" << EscapeJson(std::get<1>(msgs[mi]))
							 << "\", \"text\": \"" << EscapeJson(std::get<2>(msgs[mi])) << "\"}";
					}
					json << "\n      ]\n";
					json << "    }";
				}
				sqlite3_finalize(evStmt);
				json << "\n  ]\n}\n";
				WriteJson(zoneDir / fname, json.str());
			}
			sqlite3_finalize(actStmt);

			// Common actors referenced via alias in this zone.
			if (sqlite3_prepare_v2(db,
				"SELECT a.actor_no, ea.actor_name FROM event_actor_alias a"
				" JOIN event_actor ea ON ea.id = a.actor_id"
				" WHERE a.zone_id = ? ORDER BY a.actor_no",
				-1, &actStmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(actStmt, 1, zone.id);
				while (sqlite3_step(actStmt) == SQLITE_ROW)
				{
					int actorNo       = sqlite3_column_int(actStmt, 0);
					std::string aname = reinterpret_cast<const char*>(sqlite3_column_text(actStmt, 1));
					if (!firstActor) zoneIndex << ",\n";
					firstActor = false;
					zoneIndex << "    {\"actor_number\": " << actorNo
						<< ", \"actor_name\": \"" << EscapeJson(aname)
						<< "\", \"category\": \"common\""
						<< ", \"common_ref\": \"../../common/" << SafeName(aname) << ".json\"}";
				}
				sqlite3_finalize(actStmt);
			}

			zoneIndex << "\n  ]\n}\n";
			WriteJson(zoneDir / "index.json", zoneIndex.str());
		}
	}

	// -- 3. master_index.json -------------------------------------------------
	{
		std::ostringstream master;
		master << "{\n  \"zones\": [\n";
		bool first = true;
		if (sqlite3_prepare_v2(db, "SELECT zone_name FROM event_zone ORDER BY zone_name", -1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				if (!first) master << ",\n";
				first = false;
				std::string zn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
				master << "    {\"zone_name\": \"" << EscapeJson(zn)
					<< "\", \"ref\": \"zone/" << zn << "/index.json\"}";
			}
			sqlite3_finalize(stmt);
		}
		master << "\n  ]\n}\n";
		WriteJson(root / "master_index.json", master.str());
	}

	std::cout << "[EventDbDump] Written to: " << outputDir << std::endl;
}
