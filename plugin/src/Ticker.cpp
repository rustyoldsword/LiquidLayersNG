#include "Ticker.h"

#include "Layers.h"

#include <chrono>
#include <thread>

namespace Ticker
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		std::atomic_bool queued{ false };
		Clock::time_point last{};

		void OnMainThread()
		{
			const auto now = Clock::now();
			const double seconds = last == Clock::time_point{} ? 0.0 : std::chrono::duration<double>(now - last).count();
			last = now;
			queued = false;
			auto* ui = RE::UI::GetSingleton();
			if (!RE::PlayerCharacter::GetSingleton() || (ui && ui->GameIsPaused())) {
				return;
			}
			Layers::Tick(std::min(seconds, 1.0));  // a hitch or a long load is not "time on"
		}
	}

	void Start()
	{
		std::thread([]() {
			for (;;) {
				std::this_thread::sleep_for(250ms);
				if (!queued.exchange(true)) {
					SKSE::GetTaskInterface()->AddTask(OnMainThread);
				}
			}
		}).detach();
		logger::info("timer started");
	}
}
