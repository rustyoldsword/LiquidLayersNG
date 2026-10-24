#pragma once

#include "Papyrus.h"

// The pieces Liquid Layers NG.esp ships and who wears which. Two families:
//   animation (TullAnimatedCreampie): animated meshes, on for Settings::animSeconds real seconds;
//   liquid ([Predator] Liquid Pack): stays Settings::liquidHours game hours or until washed off.
// A piece goes on silently and only into slots nothing else of the actor occupies; another climax of the same
// kind restarts its timer instead of stacking.
namespace Layers
{
	enum class Piece : std::uint8_t
	{
		kPussy01,
		kPussy02,
		kPussy03,
		kMouth,
		kFloor,
		kBottom,
		kBottomSMP,
		kFace,
		kFaceSMP,
		kTorso,
		kCount
	};

	struct Worn
	{
		Piece  piece;
		double until;  // animation: our real-time clock (seconds); liquid: game days passed (0 = until washed off)
	};

	bool LoadForms();  // kDataLoaded; false when Liquid Layers NG.esp is not active
	void WatchScenes();  // kDataLoaded: follow SexLab P+ thread events (who is in a scene)

	// From the Papyrus listener (any thread): queues the work for the main thread.
	void Climax(RE::Actor* a_giver, RE::Actor* a_receiver, Papyrus::Kind a_kind);

	const char* PieceName(Piece a_piece);
	bool IsAnimation(Piece a_piece);
	RE::TESObjectARMO* Form(Piece a_piece);

	// What each actor wears from us, for the timers (step 4) and the co-save. Main thread only.
	std::vector<Worn>& WornBy(RE::FormID a_actor);
	std::vector<RE::FormID> Wearers();
	void Remove(RE::Actor* a_actor, Piece a_piece);
	void Forget(RE::FormID a_actor);

	double Clock();  // real seconds the game has been unpaused this session
	void Advance(double a_seconds);

	// Everything we put on the actor comes off (washing, the menu button). Any thread: queued for the main thread.
	void WashOff(RE::Actor* a_actor, const char* a_reason);
	// Main thread, a few times a second: expired timers, water, rain, body clothing.
	void Tick(double a_seconds);

	// co-save (record 'WORN') and resets
	void Save(SKSE::SerializationInterface* a_intfc);
	void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
	void Revert();
	// After a load: a piece of ours on the player that nothing tracks (co-save lost, older save) comes off.
	void SweepPlayer();
}
