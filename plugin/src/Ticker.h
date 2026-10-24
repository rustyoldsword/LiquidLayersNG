#pragma once

// Drives Layers::Tick on the main thread about four times a second. Paused time (menus, the console) is not counted,
// so the animation timer measures time the player actually saw. A thread, not a frame hook: during a SexLab P+ scene
// PlayerCharacter::Update does not run (found with Fluid Motion), and the timers must keep going there.
namespace Ticker
{
	void Start();  // kDataLoaded
}
