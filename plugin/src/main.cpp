#include "Layers.h"
#include "Menu.h"
#include "Papyrus.h"
#include "Settings.h"
#include "Ticker.h"

namespace
{
	void InitializeLogging()
	{
		auto path = logger::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Unable to lookup SKSE logs directory.");
		}
		*path /= "LiquidLayersNG.log";

		auto log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kPostPostLoad:
			Menu::Register();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			if (Layers::LoadForms()) {
				Layers::WatchScenes();
				Ticker::Start();
			}
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			SKSE::GetTaskInterface()->AddTask([]() { Layers::SweepPlayer(); });
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	InitializeLogging();

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	logger::info("{} {} is loading", plugin->GetName(), plugin->GetVersion().string());

	// own logger: keep CommonLib from re-initialising (and truncating) it
	SKSE::Init(a_skse, false);
	Settings::Load();

	if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
		SKSE::stl::report_and_fail("Unable to register the SKSE message listener.");
	}
	Papyrus::Install();

	// co-save: who wears which piece and when it comes off
	auto* serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID('LLNG');
	serialization->SetSaveCallback([](SKSE::SerializationInterface* a_intfc) { Layers::Save(a_intfc); });
	serialization->SetLoadCallback([](SKSE::SerializationInterface* a_intfc) {
		std::uint32_t type, version, length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type == 'WORN') {
				Layers::Load(a_intfc, version);
			}
		}
	});
	serialization->SetRevertCallback([](SKSE::SerializationInterface*) { Layers::Revert(); });

	logger::info("{} loaded", plugin->GetName());
	return true;
}
