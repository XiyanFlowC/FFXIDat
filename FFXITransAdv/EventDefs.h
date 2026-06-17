#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>

// Language code constants
namespace LangCode {
    inline constexpr std::string_view JA = "ja";
    inline constexpr std::string_view EN = "en";
    inline constexpr std::u8string_view JA_U8 = u8"ja";
    inline constexpr std::u8string_view EN_U8 = u8"en";

    // Normalize legacy "jp" to "ja" for backward compatibility
    inline std::string Normalize(std::string lang) {
        if (lang == "jp") return std::string(JA);
        return lang;
    }
    inline std::u8string Normalize(std::u8string lang) {
        if (lang == u8"jp") return std::u8string(JA_U8);
        return lang;
    }
}

// Zone file set: paths for a single zone's event files
struct ZoneFiles
{
	std::string zone_name;
	std::string evev_path;
	std::string evac_path;
	std::string evsb_path;      // Japanese evsb (primary, used for source text extraction)
	std::string evsb_en_path;   // English evsb
	std::string evsb_de_path;   // German evsb (future)
	std::string evsb_fr_path;   // French evsb (future)
};

// defs.csv 扩展定义
// 格式: path,type,lang,zone_name[,event_ref]
// path = ROM{vol}/{cat}/{file}
// type = evev | evac | evsb | xis | dmsg | ...
// lang = ja | en | 空（非语言特定）
// zone_name = 区域显示名称

struct FileDef
{
	std::string path;
	std::string type;
	std::string lang;
	std::string zone_name;
};

// 将 FileDef 向量合并为 ZoneFiles 映射
inline std::unordered_map<std::string, ZoneFiles> MergeToZoneSets(const std::vector<FileDef>& defs)
{
	std::unordered_map<std::string, ZoneFiles> zones;
	for (const auto& d : defs)
	{
		auto& zs = zones[d.zone_name];
		zs.zone_name = d.zone_name;
		if (d.type == "evev")
			zs.evev_path = d.path;
		else if (d.type == "evac")
			zs.evac_path = d.path;
		else if (d.type == "evsb" && (d.lang == LangCode::JA || d.lang.empty()))
			zs.evsb_path = d.path;
		else if (d.type == "evsb" && d.lang == LangCode::EN)
			zs.evsb_en_path = d.path;
		else if (d.type == "evsb" && d.lang == "de")
			zs.evsb_de_path = d.path;
		else if (d.type == "evsb" && d.lang == "fr")
			zs.evsb_fr_path = d.path;
	}
	return zones;
}
