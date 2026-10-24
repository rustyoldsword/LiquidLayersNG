#pragma once

// Natives of script LiquidLayersNG (Scripts\Source\LiquidLayersNG.psc). LLNG_Listener calls Climax for every
// giver/receiver pair of a SexLab P+ orgasm.
namespace Papyrus
{
	enum class Kind : std::int32_t
	{
		kVaginal = 1,
		kAnal = 2,
		kOral = 3,
		kFacial = 4
	};

	void Install();
}
