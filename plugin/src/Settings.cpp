#include "Settings.h"

#include <fstream>

namespace Settings
{
	namespace
	{
		Values values;

		std::filesystem::path Path()
		{
			return std::filesystem::current_path() / L"Data" / L"SKSE" / L"Plugins" / L"LiquidLayersNG.ini";
		}

		std::string String(const wchar_t* a_section, const wchar_t* a_key, const std::string& a_default)
		{
			wchar_t buffer[256]{};
			const std::wstring fallback(a_default.begin(), a_default.end());
			GetPrivateProfileStringW(a_section, a_key, fallback.c_str(), buffer, 256, Path().c_str());
			std::string out;
			for (const wchar_t* c = buffer; *c; ++c) {
				out.push_back(static_cast<char>(*c));
			}
			return out;
		}

		bool Bool(const wchar_t* a_section, const wchar_t* a_key, bool a_default)
		{
			return GetPrivateProfileIntW(a_section, a_key, a_default ? 1 : 0, Path().c_str()) != 0;
		}

		float Number(const wchar_t* a_section, const wchar_t* a_key, float a_default)
		{
			try {
				return std::stof(String(a_section, a_key, std::to_string(a_default)));
			} catch (...) {
				return a_default;
			}
		}
	}

	const Values& Get()
	{
		return values;
	}

	Values& Mutable()
	{
		return values;
	}

	void Load()
	{
		auto& v = values;
		v.enabled = Bool(L"General", L"bEnabled", v.enabled);
		v.player = Bool(L"General", L"bPlayer", v.player);
		v.npcs = Bool(L"General", L"bNPCs", v.npcs);
		v.creatureGivers = Bool(L"General", L"bCreatureGivers", v.creatureGivers);
		v.vaginal = Bool(L"Kinds", L"bVaginal", v.vaginal);
		v.anal = Bool(L"Kinds", L"bAnal", v.anal);
		v.oral = Bool(L"Kinds", L"bOral", v.oral);
		v.facial = Bool(L"Kinds", L"bFacial", v.facial);
		v.animSeconds = std::clamp(Number(L"Animation", L"fSeconds", v.animSeconds), 10.0f, 600.0f);
		v.variant = static_cast<std::int32_t>(std::clamp(Number(L"Animation", L"iVariant", static_cast<float>(v.variant)), 0.0f, 3.0f));
		v.floor = Bool(L"Animation", L"bFloor", v.floor);
		v.smp = Bool(L"Liquid", L"bSMP", v.smp);
		v.torso = Bool(L"Liquid", L"bTorso", v.torso);
		v.liquidHours = std::clamp(Number(L"Liquid", L"fHours", v.liquidHours), 0.0f, 48.0f);
		v.washWater = Bool(L"Wash", L"bWater", v.washWater);
		v.washBathing = Bool(L"Wash", L"bBathing", v.washBathing);
		v.washRain = Bool(L"Wash", L"bRain", v.washRain);
		v.hideBottomDressed = Bool(L"Wash", L"bRemoveBottomWhenDressed", v.hideBottomDressed);
		logger::info("settings: enabled {}, player {}, NPCs {}, creature givers {}; kinds vaginal {} anal {} oral {} facial {}", v.enabled,
			v.player, v.npcs, v.creatureGivers, v.vaginal, v.anal, v.oral, v.facial);
		logger::info("settings: animation {} s (variant {}, floor {}); liquid SMP {}, torso {}, {} h; wash water {} bathing {} rain {}, "
					 "bottom off when dressed {}",
			v.animSeconds, v.variant, v.floor, v.smp, v.torso, v.liquidHours, v.washWater, v.washBathing, v.washRain, v.hideBottomDressed);
	}

	bool Save()
	{
		// Plain file I/O: under Mod Organizer WritePrivateProfileString can report success without reaching the mod
		// folder (seen in Amputator NG 1.6.0), while ordinary file reads and writes go through its VFS.
		const auto path = Path();
		std::vector<std::string> lines;
		{
			std::ifstream in(path);
			for (std::string line; std::getline(in, line);) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				lines.push_back(line);
			}
		}
		auto put = [&](std::string_view a_section, std::string_view a_key, std::string a_value) {
			const auto text = std::format("{}={}", a_key, a_value);
			const auto header = std::format("[{}]", a_section);
			auto sec = std::ranges::find(lines, header);
			if (sec == lines.end()) {
				if (!lines.empty()) {
					lines.push_back("");
				}
				lines.push_back(header);
				lines.push_back(text);
				return;
			}
			auto it = std::next(sec);
			for (; it != lines.end() && !it->starts_with('['); ++it) {
				if (it->starts_with(std::format("{}=", a_key))) {
					*it = text;
					return;
				}
			}
			while (it != std::next(sec) && std::prev(it)->empty()) {
				--it;  // keep the blank line before the next section
			}
			lines.insert(it, text);
		};
		auto flag = [](bool a_value) { return std::string(a_value ? "1" : "0"); };
		auto number = [](float a_value) { return std::format("{:g}", std::round(a_value * 100.0f) / 100.0f); };
		const auto& v = values;
		put("General", "bEnabled", flag(v.enabled));
		put("General", "bPlayer", flag(v.player));
		put("General", "bNPCs", flag(v.npcs));
		put("General", "bCreatureGivers", flag(v.creatureGivers));
		put("Kinds", "bVaginal", flag(v.vaginal));
		put("Kinds", "bAnal", flag(v.anal));
		put("Kinds", "bOral", flag(v.oral));
		put("Kinds", "bFacial", flag(v.facial));
		put("Animation", "fSeconds", number(v.animSeconds));
		put("Animation", "iVariant", std::to_string(v.variant));
		put("Animation", "bFloor", flag(v.floor));
		put("Liquid", "bSMP", flag(v.smp));
		put("Liquid", "bTorso", flag(v.torso));
		put("Liquid", "fHours", number(v.liquidHours));
		put("Wash", "bWater", flag(v.washWater));
		put("Wash", "bBathing", flag(v.washBathing));
		put("Wash", "bRain", flag(v.washRain));
		put("Wash", "bRemoveBottomWhenDressed", flag(v.hideBottomDressed));
		bool ok = false;
		{
			std::ofstream out(path, std::ios::binary | std::ios::trunc);
			for (const auto& line : lines) {
				out << line << "\r\n";
			}
			ok = out.good();
		}
		logger::info("settings saved{}", ok ? "" : " (FAILED)");
		return ok;
	}
}
