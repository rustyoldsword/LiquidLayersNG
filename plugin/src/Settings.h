#pragma once

// Data\SKSE\Plugins\LiquidLayersNG.ini. Everything here is also editable live on the SKSE Menu Framework page;
// "Save as defaults" writes it back.
namespace Settings
{
	struct Values
	{
		// [General]
		bool enabled{ true };
		bool player{ true };          // the player can receive layers
		bool npcs{ true };            // NPCs can receive layers
		bool creatureGivers{ true };  // a creature's climax counts too
		// [Kinds] which climaxes leave layers
		bool vaginal{ true };
		bool anal{ true };            // same layers as vaginal (Bottom)
		bool oral{ true };
		bool facial{ true };
		// [Animation] TullAnimatedCreampie pieces (animated meshes)
		float animSeconds{ 120.0f };  // real seconds the animation stays on; a new climax restarts the timer
		std::int32_t variant{ 0 };    // 0 random, 1..3 = Pussy 01 / 02 (reversed) / 03
		bool floor{ true };           // the puddle under her
		// [Liquid] [Predator] Liquid Pack layers
		bool smp{ true };             // dripping SMP version of Bottom/Face instead of the static one
		bool torso{ false };          // torso layer too
		float liquidHours{ 8.0f };    // game hours the liquid stays; 0 = until washed off
		// [Wash]
		bool washWater{ true };       // wading in deep water or swimming
		bool washBathing{ true };     // Bathing in Skyrim
		bool washRain{ false };       // standing in the rain (WetFunction NG)
		bool hideBottomDressed{ true };  // Bottom comes off when body clothing (slot 32) goes on
	};

	const Values& Get();
	Values& Mutable();  // the in-game menu edits these directly
	void Load();
	bool Save();
}
