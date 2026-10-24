#pragma once

// In-game settings page in SKSE Menu Framework ("Liquid Layers NG"): every INI value, applied immediately, written to
// LiquidLayersNG.ini with "Save as defaults". Optional: without the framework the INI is the only way.
namespace Menu
{
	bool Register();  // kPostPostLoad
}
