#include "Layers.h"

#include "Settings.h"

#include <charconv>
#include <random>
#include <unordered_map>

namespace Layers
{
	namespace
	{
		constexpr auto kPlugin = "Liquid Layers NG.esp"sv;

		// local FormIDs in Liquid Layers NG.esp, in Piece order
		constexpr std::array<RE::FormID, static_cast<std::size_t>(Piece::kCount)> kForms{
			0x828,  // TullAnimatedCreampieAR    Pussy 01         slot 59
			0x82A,  // TullAnimatedCreampie2AR   Pussy 02 reversed slot 59
			0x82B,  // TullAnimatedCreampie3AR   Pussy 03         slot 59
			0x829,  // TullAnimatedCreampieNAR   Mouth            slot 59
			0x82C,  // TullAnimatedCreampie4AR   Floor            slot 61 (was 60: HDT SMP Object base lives there)
			0x803,  // LP__Bottom_Base                            slot 47
			0x808,  // LP__Bottom_SMP_Base                        slot 43
			0x804,  // LP__Face_Base                              slot 55
			0x809,  // LP__Face_SMP_Base                          slot 57
			0xD63,  // LP__Torso_Base                             slot 46 (was 36: rings)
		};

		std::array<RE::TESObjectARMO*, static_cast<std::size_t>(Piece::kCount)> forms{};
		std::unordered_map<RE::FormID, std::vector<Worn>> worn;
		double clock{ 0.0 };
		std::mt19937 rng{ std::random_device{}() };
		// Who is in a SexLab P+ scene, from P+'s own thread events (AnimationStart / StageStart / AnimationEnd, sent with
		// SendModEvent and the thread id). The scene faction is not used: read through VisitFactions it answered "not in
		// a scene" for the player in the middle of one (seen in game - the puddle came off as it went on).
		// AnimationEnd comes after the thread has cleared its aliases, so the actors are remembered from the start.
		std::unordered_map<std::int32_t, std::vector<RE::FormID>> threadActors;
		std::unordered_map<RE::FormID, std::int32_t> sceneOf;  // actor -> thread
		// When each actor's scene ended (or it got a piece): nothing comes off until kSceneSettle after it, so a piece
		// is never taken off while SexLab redresses and rebuilds the actor's 3D.
		std::unordered_map<RE::FormID, double> lastInScene;

		bool InScene(RE::Actor* a_actor)
		{
			return sceneOf.contains(a_actor->GetFormID());
		}

		bool IsCreature(RE::Actor* a_actor)
		{
			const auto* race = a_actor ? a_actor->GetRace() : nullptr;
			return race && !race->HasKeywordString("ActorTypeNPC");
		}

		std::int32_t Count(RE::Actor* a_actor, RE::TESObjectARMO* a_armor)
		{
			const auto counts = a_actor->GetInventoryCounts([a_armor](RE::TESBoundObject& a_object) { return &a_object == a_armor; });
			const auto it = counts.find(a_armor);
			return it != counts.end() ? it->second : 0;
		}

		bool Ours(const RE::TESObjectARMO* a_armor)
		{
			return a_armor && std::ranges::find(forms, a_armor) != forms.end();
		}

		// Something that is not one of our pieces sits in one of the piece's slots: leave the actor's own item alone.
		const RE::TESObjectARMO* Blocker(RE::Actor* a_actor, const RE::TESObjectARMO* a_armor)
		{
			const auto mask = a_armor->GetSlotMask().underlying();
			for (std::uint32_t bit = 0; bit < 32; ++bit) {
				const auto slot = static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(1u << bit);
				if (!(mask & (1u << bit))) {
					continue;
				}
				auto* other = a_actor->GetWornArmor(slot);
				if (other && !Ours(other)) {
					return other;
				}
			}
			return nullptr;
		}

		Piece TullVariant()
		{
			auto variant = Settings::Get().variant;
			if (variant < 1 || variant > 3) {
				variant = std::uniform_int_distribution<std::int32_t>(1, 3)(rng);
			}
			return static_cast<Piece>(static_cast<std::int32_t>(Piece::kPussy01) + variant - 1);
		}

		std::vector<Piece> PiecesFor(Papyrus::Kind a_kind)
		{
			const auto& s = Settings::Get();
			std::vector<Piece> pieces;
			switch (a_kind) {
			case Papyrus::Kind::kVaginal:
			case Papyrus::Kind::kAnal:
				pieces.push_back(TullVariant());
				if (s.floor) {
					pieces.push_back(Piece::kFloor);
				}
				pieces.push_back(s.smp ? Piece::kBottomSMP : Piece::kBottom);
				if (s.torso) {
					pieces.push_back(Piece::kTorso);
				}
				break;
			case Papyrus::Kind::kOral:
				pieces.push_back(Piece::kMouth);
				pieces.push_back(s.smp ? Piece::kFaceSMP : Piece::kFace);
				break;
			case Papyrus::Kind::kFacial:
				pieces.push_back(s.smp ? Piece::kFaceSMP : Piece::kFace);
				break;
			}
			return pieces;
		}

		bool KindEnabled(Papyrus::Kind a_kind)
		{
			const auto& s = Settings::Get();
			switch (a_kind) {
			case Papyrus::Kind::kVaginal:
				return s.vaginal;
			case Papyrus::Kind::kAnal:
				return s.anal;
			case Papyrus::Kind::kOral:
				return s.oral;
			case Papyrus::Kind::kFacial:
				return s.facial;
			}
			return false;
		}

		const char* KindName(Papyrus::Kind a_kind)
		{
			switch (a_kind) {
			case Papyrus::Kind::kVaginal:
				return "vaginal";
			case Papyrus::Kind::kAnal:
				return "anal";
			case Papyrus::Kind::kOral:
				return "oral";
			case Papyrus::Kind::kFacial:
				return "facial";
			}
			return "?";
		}

		double Until(Piece a_piece)
		{
			if (IsAnimation(a_piece)) {
				return clock + Settings::Get().animSeconds;
			}
			const auto hours = Settings::Get().liquidHours;
			const auto* calendar = RE::Calendar::GetSingleton();
			return hours > 0.0f && calendar ? calendar->GetDaysPassed() + hours / 24.0 : 0.0;
		}

		void Put(RE::Actor* a_actor, Piece a_piece)
		{
			auto* armor = Form(a_piece);
			if (!armor) {
				return;
			}
			if (const auto* blocker = Blocker(a_actor, armor)) {
				logger::info("  {}: skipped, {} is in its slot", PieceName(a_piece), blocker->GetName());
				return;
			}
			auto& list = worn[a_actor->GetFormID()];
			// one animated piece per slot 59: a new one replaces the old - but not inside a scene, where taking a piece
			// off races SexLab's own 3D rebuild and leaves its geometry behind; there the piece already on stays
			const auto slots = armor->GetSlotMask().underlying();
			if (InScene(a_actor)) {
				auto held = std::ranges::find_if(list, [&](const Worn& a_entry) {
					auto* other = Form(a_entry.piece);
					return a_entry.piece != a_piece && other && (other->GetSlotMask().underlying() & slots);
				});
				if (held != list.end()) {
					held->until = Until(held->piece);
					logger::info("  {}: kept {} in the scene, timer restarted", PieceName(a_piece), PieceName(held->piece));
					return;
				}
			}
			std::erase_if(list, [&](const Worn& a_entry) {
				auto* other = Form(a_entry.piece);
				if (a_entry.piece == a_piece || !other || !(other->GetSlotMask().underlying() & slots)) {
					return false;
				}
				Remove(a_actor, a_entry.piece);
				return true;
			});
			const auto until = Until(a_piece);
			auto entry = std::ranges::find(list, a_piece, &Worn::piece);
			if (entry != list.end() && a_actor->GetWornArmor(armor->GetFormID())) {
				entry->until = until;
				logger::info("  {}: timer restarted", PieceName(a_piece));
				return;
			}
			if (entry == list.end()) {
				list.push_back({ a_piece, until });
			} else {
				entry->until = until;
			}
			if (Count(a_actor, armor) < 1) {
				a_actor->AddObjectToContainer(armor, nullptr, 1, nullptr);
			}
			RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, armor, nullptr, 1, nullptr, false, true, false, true);
			logger::info("  {}: on", PieceName(a_piece));
		}

		void Apply(RE::Actor* a_giver, RE::Actor* a_receiver, Papyrus::Kind a_kind)
		{
			const auto& s = Settings::Get();
			if (!s.enabled || !KindEnabled(a_kind)) {
				return;
			}
			const bool player = a_receiver->IsPlayerRef();
			if ((player && !s.player) || (!player && !s.npcs)) {
				return;
			}
			if (IsCreature(a_receiver) || (!s.creatureGivers && IsCreature(a_giver)) || !a_receiver->Is3DLoaded()) {
				return;
			}
			logger::info("{} -> {} ({})", a_giver->GetDisplayFullName(), a_receiver->GetDisplayFullName(), KindName(a_kind));
			lastInScene[a_receiver->GetFormID()] = clock;
			for (const auto piece : PiecesFor(a_kind)) {
				Put(a_receiver, piece);
			}
		}
	}

	bool LoadForms()
	{
		auto* data = RE::TESDataHandler::GetSingleton();
		if (!data || !data->LookupModByName(kPlugin)) {
			logger::warn("{} is not active - nothing will happen", kPlugin);
			return false;
		}
		bool ok = true;
		for (std::size_t i = 0; i < kForms.size(); ++i) {
			forms[i] = data->LookupForm<RE::TESObjectARMO>(kForms[i], kPlugin);
			if (!forms[i]) {
				logger::error("{} has no armor {:03X} ({})", kPlugin, kForms[i], PieceName(static_cast<Piece>(i)));
				ok = false;
			}
		}
		logger::info("{} pieces loaded{}", forms.size(), ok ? "" : " (some missing)");
		return ok;
	}

	namespace
	{
		enum class SceneEvent
		{
			kStart,
			kEnd
		};

		// main thread: AnimationStart / StageStart refresh the thread's actors, AnimationEnd lets them go
		void OnScene(SceneEvent a_event, std::int32_t a_tid, RE::FormID a_quest)
		{
			if (a_event == SceneEvent::kEnd) {
				const auto it = threadActors.find(a_tid);
				if (it == threadActors.end()) {
					return;
				}
				for (const auto id : it->second) {
					if (const auto own = sceneOf.find(id); own != sceneOf.end() && own->second == a_tid) {
						sceneOf.erase(own);
						lastInScene[id] = clock;
					}
				}
				logger::info("SexLab thread {} ended ({} actor(s))", a_tid, it->second.size());
				threadActors.erase(it);
				return;
			}
			auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(a_quest);
			if (!quest) {
				return;
			}
			auto& actors = threadActors[a_tid];
			const bool first = actors.empty();
			for (const auto& entry : quest->refAliasMap) {
				const auto reference = entry.second.get();
				const auto* actor = reference ? reference->As<RE::Actor>() : nullptr;
				if (actor && std::ranges::find(actors, actor->GetFormID()) == actors.end()) {
					actors.push_back(actor->GetFormID());
				}
			}
			for (const auto id : actors) {
				sceneOf[id] = a_tid;
			}
			if (first) {
				logger::info("SexLab thread {} started ({} actor(s))", a_tid, actors.size());
			}
		}

		class SceneListener final : public RE::BSTEventSink<SKSE::ModCallbackEvent>
		{
		public:
			static SceneListener* Get()
			{
				static SceneListener listener;
				return &listener;
			}

			RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override
			{
				if (!a_event || !a_event->sender || a_event->sender->GetFormType() != RE::FormType::Quest) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const std::string_view name = a_event->eventName.c_str();
				SceneEvent kind;
				if (name == "AnimationStart" || name == "StageStart") {
					kind = SceneEvent::kStart;
				} else if (name == "AnimationEnd") {
					kind = SceneEvent::kEnd;
				} else {
					return RE::BSEventNotifyControl::kContinue;
				}
				const auto* file = a_event->sender->GetFile(0);
				if (!file || file->GetFilename() != "SexLab.esm"sv) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const std::string_view text = a_event->strArg.c_str();
				std::int32_t tid = -1;
				if (std::from_chars(text.data(), text.data() + text.size(), tid).ec != std::errc{} || tid < 0) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const auto quest = a_event->sender->GetFormID();
				SKSE::GetTaskInterface()->AddTask([kind, tid, quest]() { OnScene(kind, tid, quest); });
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void WatchScenes()
	{
		if (auto* source = SKSE::GetModCallbackEventSource()) {
			source->AddEventSink(SceneListener::Get());
			logger::info("following SexLab scenes through their thread events");
		} else {
			logger::warn("SKSE mod events unavailable - pieces come off only by their timers");
		}
	}

	void Climax(RE::Actor* a_giver, RE::Actor* a_receiver, Papyrus::Kind a_kind)
	{
		if (!a_giver || !a_receiver) {
			return;
		}
		SKSE::GetTaskInterface()->AddTask([giver = a_giver->CreateRefHandle(), receiver = a_receiver->CreateRefHandle(), a_kind]() {
			auto g = giver.get();
			auto r = receiver.get();
			if (g && r) {
				Apply(g.get(), r.get(), a_kind);
			}
		});
	}

	const char* PieceName(Piece a_piece)
	{
		switch (a_piece) {
		case Piece::kPussy01:
			return "creampie 01";
		case Piece::kPussy02:
			return "creampie 02";
		case Piece::kPussy03:
			return "creampie 03";
		case Piece::kMouth:
			return "mouth";
		case Piece::kFloor:
			return "floor puddle";
		case Piece::kBottom:
			return "liquid bottom";
		case Piece::kBottomSMP:
			return "liquid bottom (SMP)";
		case Piece::kFace:
			return "liquid face";
		case Piece::kFaceSMP:
			return "liquid face (SMP)";
		case Piece::kTorso:
			return "liquid torso";
		default:
			return "?";
		}
	}

	bool IsAnimation(Piece a_piece)
	{
		return a_piece <= Piece::kFloor;
	}

	RE::TESObjectARMO* Form(Piece a_piece)
	{
		const auto i = static_cast<std::size_t>(a_piece);
		return i < forms.size() ? forms[i] : nullptr;
	}

	std::vector<Worn>& WornBy(RE::FormID a_actor)
	{
		return worn[a_actor];
	}

	std::vector<RE::FormID> Wearers()
	{
		std::vector<RE::FormID> ids;
		for (const auto& [id, list] : worn) {
			if (!list.empty()) {
				ids.push_back(id);
			}
		}
		return ids;
	}

	void Remove(RE::Actor* a_actor, Piece a_piece)
	{
		auto* armor = Form(a_piece);
		if (!a_actor || !armor) {
			return;
		}
		if (a_actor->GetWornArmor(armor->GetFormID())) {
			RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, armor, nullptr, 1, nullptr, false, true, false, true);
		}
		if (const auto count = Count(a_actor, armor); count > 0) {
			a_actor->RemoveItem(armor, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		}
		logger::info("  {}: off ({})", PieceName(a_piece), a_actor->GetDisplayFullName());
	}

	void Forget(RE::FormID a_actor)
	{
		worn.erase(a_actor);
	}

	double Clock()
	{
		return clock;
	}

	void Advance(double a_seconds)
	{
		clock += a_seconds;
	}

	namespace
	{
		constexpr double kRainSeconds = 30.0;  // standing in the rain this long washes the liquid off
		// SexLab strips and redresses at the start and end of a scene and rebuilds the actor's 3D meanwhile
		// (Update3DModel). A piece taken off in that window leaves its geometry on the skeleton for good (seen in game:
		// the floor puddle and the mouth stayed after "off"), so nothing comes off in a scene or this long after it.
		constexpr double kSceneSettle = 1.0;
		std::unordered_map<RE::FormID, double> rainFor;

		void WashNow(RE::Actor* a_actor, const char* a_reason)
		{
			auto it = worn.find(a_actor->GetFormID());
			if (it == worn.end() || it->second.empty()) {
				return;
			}
			logger::info("{}: washed off ({})", a_actor->GetDisplayFullName(), a_reason);
			for (const auto& entry : it->second) {
				Remove(a_actor, entry.piece);
			}
			worn.erase(it);
			rainFor.erase(a_actor->GetFormID());
		}

		bool InDeepWater(RE::Actor* a_actor)
		{
			if (const auto* state = a_actor->AsActorState(); state && state->IsSwimming()) {
				return true;
			}
			auto* cell = a_actor->GetParentCell();
			return cell && a_actor->GetSubmergedLevel(a_actor->GetPositionZ(), cell) >= 0.5f;
		}

		bool InRain(RE::Actor* a_actor)
		{
			const auto* sky = RE::Sky::GetSingleton();
			const auto* cell = a_actor->GetParentCell();
			return sky && sky->IsRaining() && cell && cell->IsExteriorCell();
		}

		bool DressedBody(RE::Actor* a_actor)
		{
			const auto* body = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kBody);
			return body && !Ours(body);
		}

		bool UnderClothes(Piece a_piece)
		{
			return a_piece == Piece::kBottom || a_piece == Piece::kBottomSMP || a_piece == Piece::kTorso;
		}
	}

	void WashOff(RE::Actor* a_actor, const char* a_reason)
	{
		if (!a_actor) {
			return;
		}
		SKSE::GetTaskInterface()->AddTask([handle = a_actor->CreateRefHandle(), a_reason]() {
			if (auto actor = handle.get()) {
				if (auto* a = actor->As<RE::Actor>()) {
					WashNow(a, a_reason);
				}
			}
		});
	}

	void Tick(double a_seconds)
	{
		clock += a_seconds;
		const auto& s = Settings::Get();
		const auto* calendar = RE::Calendar::GetSingleton();
		const double days = calendar ? calendar->GetDaysPassed() : 0.0;
		for (const auto id : Wearers()) {
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(id);
			if (!actor) {
				worn.erase(id);
				rainFor.erase(id);
				lastInScene.erase(id);
				continue;
			}
			// nothing comes off in a scene or right after it (see kSceneSettle); timers that run out meanwhile wait
			if (InScene(actor)) {
				lastInScene[id] = clock;
				continue;
			}
			if (const auto it = lastInScene.find(id); it != lastInScene.end()) {
				if (clock - it->second < kSceneSettle) {
					continue;
				}
				lastInScene.erase(it);
			}
			if (actor->Is3DLoaded()) {
				if (s.washWater && InDeepWater(actor)) {
					WashNow(actor, "water");
					continue;
				}
				if (s.washRain && InRain(actor)) {
					if ((rainFor[id] += a_seconds) >= kRainSeconds) {
						WashNow(actor, "rain");
						continue;
					}
				} else {
					rainFor.erase(id);
				}
			}
			auto& list = worn[id];
			const bool dressed = s.hideBottomDressed && DressedBody(actor);
			// the puddle is an item the actor wears: once the scene is over she would carry it along the floor
			std::erase_if(list, [&](const Worn& a_entry) {
				const bool expired = IsAnimation(a_entry.piece) ? a_entry.until <= clock : (a_entry.until > 0.0 && days >= a_entry.until);
				const bool covered = dressed && UnderClothes(a_entry.piece);
				const bool left = a_entry.piece == Piece::kFloor;
				if (!expired && !covered && !left) {
					return false;
				}
				Remove(actor, a_entry.piece);
				return true;
			});
			if (list.empty()) {
				worn.erase(id);
			}
		}
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc->OpenRecord('WORN', 1)) {
			logger::error("could not open co-save record WORN");
			return;
		}
		const auto ids = Wearers();
		const auto count = static_cast<std::uint32_t>(ids.size());
		a_intfc->WriteRecordData(count);
		for (const auto id : ids) {
			const auto& list = worn[id];
			a_intfc->WriteRecordData(id);
			a_intfc->WriteRecordData(static_cast<std::uint32_t>(list.size()));
			for (const auto& entry : list) {
				// animation timers are saved as the seconds left; liquid as the game day it ends (or 0)
				const double value = IsAnimation(entry.piece) ? std::max(0.0, entry.until - clock) : entry.until;
				a_intfc->WriteRecordData(static_cast<std::uint8_t>(entry.piece));
				a_intfc->WriteRecordData(value);
			}
		}
		logger::info("saved layers of {} actor(s)", count);
	}

	void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version)
	{
		if (a_version != 1) {
			logger::warn("co-save record WORN version {} is unknown - skipped", a_version);
			return;
		}
		worn.clear();
		std::uint32_t count = 0;
		a_intfc->ReadRecordData(count);
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID id = 0;
			std::uint32_t n = 0;
			a_intfc->ReadRecordData(id);
			a_intfc->ReadRecordData(n);
			std::vector<Worn> list;
			for (std::uint32_t j = 0; j < n; ++j) {
				std::uint8_t piece = 0;
				double value = 0.0;
				a_intfc->ReadRecordData(piece);
				a_intfc->ReadRecordData(value);
				if (piece >= static_cast<std::uint8_t>(Piece::kCount)) {
					continue;
				}
				const auto p = static_cast<Piece>(piece);
				list.push_back({ p, IsAnimation(p) ? clock + value : value });
			}
			RE::FormID resolved = 0;
			if (a_intfc->ResolveFormID(id, resolved) && !list.empty()) {
				worn[resolved] = std::move(list);
			}
		}
		logger::info("loaded layers of {} actor(s)", worn.size());
	}

	void Revert()
	{
		worn.clear();
		rainFor.clear();
		lastInScene.clear();
		threadActors.clear();
		sceneOf.clear();
	}

	void SweepPlayer()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		const auto& list = worn[player->GetFormID()];
		for (std::size_t i = 0; i < forms.size(); ++i) {
			const auto piece = static_cast<Piece>(i);
			if (forms[i] && Count(player, forms[i]) > 0 && std::ranges::find(list, piece, &Worn::piece) == list.end()) {
				logger::info("player carries an untracked {} - removing", PieceName(piece));
				Remove(player, piece);
			}
		}
		if (list.empty()) {
			worn.erase(player->GetFormID());
		}
	}
}
