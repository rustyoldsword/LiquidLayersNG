#include "Menu.h"

#include "Layers.h"
#include "Settings.h"

#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include <SKSEMenuFramework.h>
#pragma warning(pop)

namespace Menu
{
	namespace
	{
		std::string status;

		void Help(const char* a_text)
		{
			if (a_text && ImGuiMCP::IsItemHovered()) {
				ImGuiMCP::SetTooltip("%s", a_text);
			}
		}

		void Check(const char* a_label, bool& a_value, const char* a_help)
		{
			ImGuiMCP::Checkbox(a_label, std::addressof(a_value));
			Help(a_help);
		}

		void Slider(const char* a_label, float& a_value, float a_min, float a_max, const char* a_format, const char* a_help)
		{
			ImGuiMCP::SliderFloat(a_label, std::addressof(a_value), a_min, a_max, a_format);
			Help(a_help);
		}

		void __stdcall Page()
		{
			auto& s = Settings::Mutable();
			ImGuiMCP::TextWrapped("Changes apply at once. \"Save as defaults\" writes them to LiquidLayersNG.ini.");

			ImGuiMCP::SeparatorText("General");
			Check("Enabled", s.enabled, "Off: no new layers. Layers already on stay until their timer or a wash.");
			Check("Player can receive", s.player, nullptr);
			Check("NPCs can receive", s.npcs, nullptr);
			Check("Creature climaxes count", s.creatureGivers, nullptr);

			ImGuiMCP::SeparatorText("Which climaxes leave layers");
			Check("Vaginal", s.vaginal, "Animated creampie, puddle and Liquid Bottom.");
			Check("Anal", s.anal, "Same layers as vaginal.");
			Check("Oral", s.oral, "Animated mouth and Liquid Face.");
			Check("Facial", s.facial, "Liquid Face only.");

			ImGuiMCP::SeparatorText("Animation (TullAnimatedCreampie)");
			Slider("Duration", s.animSeconds, 10.0f, 600.0f, "%.0f s", "Real seconds the animated pieces stay on. Another climax restarts it.");
			const char* variants[] = { "Random", "Pussy 01", "Pussy 02 (reversed)", "Pussy 03" };
			ImGuiMCP::Combo("Variant", std::addressof(s.variant), variants, 4);
			Check("Floor puddle", s.floor, nullptr);

			ImGuiMCP::SeparatorText("Liquid ([Predator] Liquid Pack)");
			Check("Dripping (SMP)", s.smp, "SMP version of Bottom and Face; off = the static mesh.");
			Check("Torso layer", s.torso, nullptr);
			Slider("Stays for", s.liquidHours, 0.0f, 48.0f, "%.1f h", "Game hours. 0 = until washed off.");

			ImGuiMCP::SeparatorText("Washing off");
			Check("Deep water or swimming", s.washWater, nullptr);
			Check("Bathing in Skyrim", s.washBathing, nullptr);
			Check("Rain (WetFunction NG)", s.washRain, nullptr);
			Check("Bottom comes off under body clothing", s.hideBottomDressed, "When something goes on the body slot (32).");

			ImGuiMCP::Separator();
			if (ImGuiMCP::Button("Wash me now")) {
				Layers::WashOff(RE::PlayerCharacter::GetSingleton(), "menu");
				status = "Everything from Liquid Layers NG comes off the player.";
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("Save as defaults")) {
				status = Settings::Save() ? "Saved to LiquidLayersNG.ini." : "Could not write LiquidLayersNG.ini.";
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("Reload INI")) {
				Settings::Load();
				status = "Reloaded from LiquidLayersNG.ini.";
			}
			if (!status.empty()) {
				ImGuiMCP::TextDisabled("%s", status.c_str());
			}
		}
	}

	bool Register()
	{
		if (!GetMenuFrameworkModule()) {
			logger::warn("SKSE Menu Framework is not loaded; the in-game settings page is unavailable");
			return false;
		}
		SKSEMenuFramework::SetSection("Liquid Layers NG");
		SKSEMenuFramework::AddSectionItem("Settings", Page);
		logger::info("registered SKSE Menu Framework page (framework {})", SKSEMenuFramework::GetMenuFrameworkVersion());
		return true;
	}
}
