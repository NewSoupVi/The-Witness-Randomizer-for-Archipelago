#include "APRandomizer.h"

#include <locale>
#include <codecvt>

#include "../ClientWindow.h"
#include "../HUDManager.h"
#include "../Panels.h"
#include "../Randomizer.h"
#include "../Special.h"
#include "APGameData.h"
#include "APWatchdog.h"
#include "Client/apclientpp/apclient.hpp"
#include "Hints.h"
#include "PanelLocker.h"
#include "../DateTime.h"
#include "PanelRestore.h"
#include "ASMPayloadManager.h"
#include "LockablePuzzle.h"
#include "../Utilities.h"
#include "SkipSpecialCases.h"
#include "APAudioPlayer.h"

APRandomizer::APRandomizer() {
	panelLocker = new PanelLocker();
};

bool APRandomizer::Connect(std::string& server, std::string& user, std::string& password) {
	ClientWindow* clientWindow = ClientWindow::get();
	clientWindow->logLine("Loading custom audio files.");
	APAudioPlayer::get();

	clientWindow->logLine("Connecting");
	std::string uri = buildUri(server);

	if (ap) ap->reset();
	std::wstring uuid = Utilities::GetUUID();
	ap = new APClient(Utilities::wstring_to_utf8(uuid), "The Witness", uri);

	ClientWindow::get()->passAPClient(ap);

	bool connected = false;
	bool hasConnectionResult = false;

	clientWindow->logLine("Connect: Setting handlers.");
	ap->set_room_info_handler([&]() {
		const int item_handling_flags_all = 7;

		ap->ConnectSlot(user, password, item_handling_flags_all, {}, {0, 6, 4});
	});

	ap->set_location_checked_handler([&](const std::list<int64_t>& locations) {
		while (!randomizationFinished) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		for (const auto& locationId : locations)
			async->MarkLocationChecked(locationId);
	});

	ap->set_slot_disconnected_handler([&]() {
		HudManager::get()->queueBannerMessage("Randomiser has been disconnected.");
		std::string message = "The randomizer seems to have unexpectedly disconnected. Please reload both the game and the randomizer.";

		MessageBoxA(GetActiveWindow(), message.c_str(), NULL, MB_OK);
	});

	ap->set_socket_disconnected_handler([&]() {
		HudManager::get()->queueBannerMessage("Randomiser has been disconnected.");
		std::string message = "The randomizer seems to have unexpectedly disconnected. Please reload both the game and the randomizer.";

		MessageBoxA(GetActiveWindow(), message.c_str(), NULL, MB_OK);
	});

	ap->set_items_received_handler([&](const std::list<APClient::NetworkItem>& items) {
		while (!randomizationFinished) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		for (auto item : items) {
			async->QueueItem(item);
		}
	});

	ap->set_slot_connected_handler([&](const nlohmann::json& slotData) {
		clientWindow->logLine("Connect: Successful connection response!");
		clientWindow->logLine("Connect: Getting basic slot data information.");
		Seed = slotData["seed"];
		FinalPanel = slotData["victory_location"];

		PuzzleRandomization = slotData.contains("puzzle_randomization") ? (int) slotData["puzzle_randomization"] : 0;
		UnlockSymbols = slotData.contains("shuffle_symbols") ? slotData["shuffle_symbols"] == true : true;
		EarlyUTM = slotData.contains("early_secret_area") ? slotData["early_secret_area"] == true : false;
		if (slotData.contains("mountain_lasers")) MountainLasers = slotData["mountain_lasers"];
		if (slotData.contains("challenge_lasers")) ChallengeLasers = slotData["challenge_lasers"];
		bool unlockableWarpsIsOn = slotData.contains("unlockable_warps") ? slotData["unlockable_warps"] == true : false;
		UnlockableWarps = { 
			"Tutorial First Hallway",
			"Desert Outside",
			"Outside Keep",
			"Town",
			"Mountaintop",
			"Caves",
			"Treehouse Entry Area",
			"Quarry",
			"Outside Symmetry Island",
			"Shipwreck",
			"Swamp Platform",
			"Jungle",
			"Outside Bunker",
		};
		if (!unlockableWarpsIsOn) {
			UnlockableWarps = {};
		}

		if (slotData.contains("easter_egg_hunt")) EggHuntDifficulty = slotData["easter_egg_hunt"];

		if (slotData.contains("panel_hunt_required_absolute")) RequiredHuntEntities = slotData["panel_hunt_required_absolute"];
		PanelHuntPostgame = slotData.contains("panel_hunt_postgame") ? (int) slotData["panel_hunt_postgame"] : 0;

		DisableNonRandomizedPuzzles = slotData.contains("disable_non_randomized_puzzles") ? slotData["disable_non_randomized_puzzles"] == true : false;
		EPShuffle = slotData.contains("shuffle_EPs") ? slotData["shuffle_EPs"] != 0 : false;
		DeathLink = slotData.contains("death_link") ? slotData["death_link"] == true : false;
		DeathLinkAmnesty = slotData.contains("death_link_amnesty") ? (int) slotData["death_link_amnesty"] : 0;
		if (!DeathLink) DeathLinkAmnesty = -1;

		if (ap->get_server_version() < APClient::Version(0, 6, 4)) {
			VagueHintsLegacy = slotData.contains("vague_hints") ? (int)slotData["vague_hints"] != 0 : false;
		}
		if (slotData["elevators_come_to_you"].is_array()) {
			for (std::string key : slotData["elevators_come_to_you"]) {
				ElevatorsComeToYou.insert(key);
			}
		}
		else if (slotData["elevators_come_to_you"] == true) {
			ElevatorsComeToYou = { "Quarry Elevator", "Swamp Long Bridge", "Bunker Elevator", "Town Maze Rooftop Bridge" };
		}

		if (!UnlockSymbols) {
			state.unlockedArrows = true;
			state.unlockedColoredDots = true;
			state.unlockedColoredStones = true;
			state.unlockedDots = true;
			state.unlockedErasers = true;
			state.unlockedFullDots = true;
			state.unlockedSoundDots = true;
			state.unlockedStars = true;
			state.unlockedStarsWithOtherSimbol = true;
			state.unlockedStones = true;
			state.unlockedSymmetry = true;
			state.unlockedTetris = true;
			state.unlockedTetrisNegative = true;
			state.unlockedTetrisRotated = true;
			state.unlockedTriangles = true;
		}

		clientWindow->logLine("Connect: Getting hunt panels.");
		if (slotData.contains("hunt_entities")) {
			for (int key : slotData["hunt_entities"]) {
				huntEntities.insert(key);
			}
		}

		state.requiredChallengeLasers = ChallengeLasers;
		state.requiredMountainLasers = MountainLasers;
		state.requiredHuntEntities = RequiredHuntEntities;
		state.totalHuntEntities = huntEntities.size();

		clientWindow->logLine("Connect: Getting progressive items.");
		if (slotData.contains("progressive_item_lists")) {
			for (auto& [key, val] : slotData["progressive_item_lists"].items()) {
				int itemId = std::stoul(key, nullptr, 10);
				std::vector<int> v = val;

				progressiveItems[itemId] = v;
			}
		}

		clientWindow->logLine("Connect: Getting panelhex to id.");
		for (auto& [key, val] : slotData["panelhex_to_id"].items()) {
			int panelId = std::stoul(key, nullptr, 16);
			int locationId = val;

			panelIdToLocationId.insert({ panelId, locationId });
			panelIdToLocationIdReverse.insert({ locationId, panelId });
		}

		clientWindow->logLine("Connect: Getting Precompleted Puzzles.");
		if (slotData.contains("precompleted_puzzles")) {
			for (int key : slotData["precompleted_puzzles"]) {
				precompletedLocations.insert(key);

				//Back compat: Disabled EPs used to be "precompleted"
				if (allEPs.count(key)) disabledEntities.insert(key);
			}
		}

		clientWindow->logLine("Connect: Getting Disabled Puzzles.");
		if (slotData.contains("disabled_panels")) {
			for (std::string keys : slotData["disabled_panels"]) {
				int key = std::stoul(keys, nullptr, 16);
				disabledEntities.insert(key);
			}
		}

		if (slotData.contains("disabled_entities")) {
			for (int key : slotData["disabled_entities"]) {
				disabledEntities.insert(key);
			}
		}

		clientWindow->logLine("Connect: Getting item id to door hexes.");
		if (slotData.contains("item_id_to_door_hexes")) {
			for (auto& [key, val] : slotData["item_id_to_door_hexes"].items()) {
				int itemId = std::stoul(key, nullptr, 10);
				std::set<int> v = val;

				itemIdToDoorSet.insert({ itemId, v });
			}
		}

		clientWindow->logLine("Connect: Getting Obelisk Side to EPs.");
		if (slotData.contains("obelisk_side_id_to_EPs")) {
			for (auto& [key, val] : slotData["obelisk_side_id_to_EPs"].items()) {
				int sideId = std::stoul(key, nullptr, 10);
				std::set<int> v = val;
				std::set<int> non_disabled_ones = {};
				for (int ep : v) {
					if (disabledEntities.contains(ep)) continue;
					non_disabled_ones.insert(ep);
				}
				if (!non_disabled_ones.empty()) {
					obeliskSideIDsToEPHexes.insert({ sideId, non_disabled_ones });
				}
			}
		}

		// Backcompat to 0.4.5
		clientWindow->logLine("Connect: Getting door hexes in the pool.");
		if (slotData.contains("door_hexes_in_the_pool")) {
			// Backcompat for Obelisk Keys
			for (auto [id, doorSet] : itemIdToDoorSet) {
				for (int door : doorSet) {
					if (allEPs.contains(door)) {
						doorToItemId[door] = id;
					}
				}
			}

			for (int key : slotData["door_hexes_in_the_pool"]) {
				doorsActuallyInTheItemPool.insert(key);
			}
		}

		clientWindow->logLine("Connect: Getting door hexes in the pool.");
		if (slotData.contains("doors_that_shouldnt_be_locked")) {
			for (int key : slotData["doors_that_shouldnt_be_locked"]) {
				doorsToSkipLocking.insert(key);
			}
		}

		clientWindow->logLine("Connect: Getting item id to door hexes actually in the pool.");
		if (slotData.contains("door_items_in_the_pool")) {
			for (int doorItemId : slotData["door_items_in_the_pool"]) {
				for (int associatedDoorHex : itemIdToDoorSet[doorItemId]) {
					if (disabledEntities.count(associatedDoorHex) || doorsToSkipLocking.count(associatedDoorHex)) continue;

					doorsActuallyInTheItemPool.insert(associatedDoorHex);

					doorToItemId.insert({ associatedDoorHex, doorItemId });
				}
			}
		}

		clientWindow->logLine("Connect: Getting audio log hints.");
		if (slotData.contains("log_ids_to_hints")) {
			for (auto& [key, val] : slotData["log_ids_to_hints"].items()) {
				int logId = std::stoul(key, nullptr, 10);
				std::string message;
				int64_t location_id = -1;
				int32_t player_no = 0;
				bool extraInfoFound = false;
				std::string area = "";
				int32_t area_progression = -1;
				int32_t area_hunt_entities = -1;
				bool allowScout = true;

				for (int i = 0; i < val.size(); i++) {
					auto token = val[i];

					if (token.type() == nlohmann::json::value_t::number_integer || token.type() == nlohmann::json::value_t::number_unsigned || token.type() == nlohmann::json::value_t::null) {
						uint64_t integer = -1;
						if (token.type() != nlohmann::json::value_t::null) {
							integer = token;
						}
						if (!extraInfoFound) {
							location_id = integer;
							player_no = ap->get_player_number();
							extraInfoFound = true;
						}
						else if (area != "") {
							area_progression = integer & 0xFF;
							area_hunt_entities = integer >> 8;
						}
						else {
							player_no = integer;

							if (player_no < 0 && location_id != -1) {  // vague hint
								player_no = -player_no;
								allowScout = false;
							}
						}
					}
					else {
						std::string line = token;
						if (line.rfind("hinted_area:", 0) == 0) {
							area = line.substr(12);
							player_no = ap->get_player_number();
							extraInfoFound = true;
						}
						else if (line.rfind("containing_area:", 0) == 0) {
							area = line.substr(16);
							allowScout = false;
						}
						else if (!line.empty()) {
							if(message != "") message.append(" ");
							message.append(line);
						}
					}
				}
				
				inGameHints.insert({ logId, {message, location_id, player_no, area, area_progression, area_hunt_entities, false, allowScout} });
			}
		}

		clientWindow->logLine("Connect: Getting lasers hints.");
		if (slotData.contains("laser_ids_to_hints")) {
			for (auto& [key, val] : slotData["laser_ids_to_hints"].items()) {
				int logId = std::stoul(key, nullptr, 10);
				std::string message;
				int64_t location_id = -1;
				int32_t player_no = 0;
				bool extraInfoFound = false;

				std::string area = "";
				bool allowScout = true;

				for (int i = 0; i < val.size(); i++) {
					auto token = val[i];

					if (token.type() == nlohmann::json::value_t::number_integer || token.type() == nlohmann::json::value_t::number_unsigned || token.type() == nlohmann::json::value_t::null) {
						uint64_t integer = -1;
						if (token.type() != nlohmann::json::value_t::null) {
							integer = token;
						}
						if (!extraInfoFound) {
							location_id = integer;
							player_no = ap->get_player_number();
							extraInfoFound = true;
						}
						else {
							player_no = integer;
						}
					}
					else {
						std::string line = token;
						if (line.rfind("containing_area:", 0) == 0) {
							area = line.substr(16);
							allowScout = false;
						}
						else if (!line.empty()) {
							if (message != "") message.append(" ");
							message.append(line);
						}
					}
				}

				inGameHints.insert({ logId, {message, location_id, player_no, area, -1, -1, true, allowScout} });
			}
		}

		connected = true;
		hasConnectionResult = true;
	});

	ap->set_slot_refused_handler([&](const std::list<std::string>& errors) {
		auto errorString = std::accumulate(errors.begin(), errors.end(), std::string(),
			[](const std::string& a, const std::string& b) -> std::string {
				return a + (a.length() > 0 ? "," : "") + b;
			});

		connected = false;
		hasConnectionResult = true;

		ClientWindow::get()->showMessageBox("Connection failed: " + errorString, "Error");
	});

	ap->set_bounced_handler([&](nlohmann::json packet) {
		std::list<std::string> tags = packet["tags"];

		bool deathlink = (std::find(tags.begin(), tags.end(), "DeathLink") != tags.end());

		if (deathlink) {
			auto data = packet["data"];
			std::string cause = "";
			if (data.contains("cause")) {
				cause = data["cause"];
			}
			std::string source = "";
			if (data.contains("source")) {
				source = data["source"];
			}

			double timestamp = data["time"];

			async->ProcessDeathLink(timestamp, cause, source);
		}
	});

	ap->set_location_info_handler([&](const std::list<APClient::NetworkItem> itemlist) {
		for (APClient::NetworkItem n : itemlist) {
			async->setLocationItemFlag(n.location, n.flags);
		}
	});

	ap->set_set_reply_handler([&](const std::string key, const nlohmann::json value, nlohmann::json original_value) {
		async->firstDataStoreResponse = true;

		if (key.find("WitnessDeathLink") != std::string::npos) {
			async->SetValueFromServer(key, value);
			return;
		}
		
		if (key.find("WitnessDisabledDeathLink") != std::string::npos) {
			async->SetValueFromServer(key, value);
			return;
		}

		if (key.find("WitnessSeenLaserHints") != std::string::npos) async->HandleLaserHintResponse(key, value);
		else if (key.find("WitnessActivatedLasers") != std::string::npos) async->HandleLaserResponse(key, value);
		else if (key.find("WitnessSolvedEPs") != std::string::npos) async->HandleEPResponse(key, value);
		else if (key.find("WitnessActivatedAudioLogs") != std::string::npos) async->HandleAudioLogResponse(key, value);
		else if (key.find("WitnessSolvedPanels") != std::string::npos) async->HandleSolvedPanelsResponse(value);
		else if (key.find("WitnessHuntEntityStatus") != std::string::npos) async->HandleHuntEntityResponse(value);
		else if (key.find("WitnessEasterEggStatus") != std::string::npos) async->HandleEasterEggResponse(key, value);
		else if (key.find("WitnessOpenedDoors") != std::string::npos) async->HandleOpenedDoorsResponse(value);
		else if (key.find("WitnessUnlockedWarps") != std::string::npos) async->HandleWarpResponse(value);
	});

	ap->set_print_json_handler([&](const APClient::PrintJSONArgs jsonArgs) {
		if (!jsonArgs.receiving || !jsonArgs.item || jsonArgs.item->player != ap->get_player_number())
			return;

		const APClient::NetworkItem item = *jsonArgs.item;
		const int receiver = *jsonArgs.receiving;
		const int sender = item.player;
		const auto msg = jsonArgs.data;

		while (!randomizationFinished) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		while (ap->get_item_name(item.item, ap->get_player_game(receiver)) == "Unknown" && unknownCounter > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			unknownCounter--;
		}

		auto findResult = std::find_if(std::begin(panelIdToLocationId), std::end(panelIdToLocationId), [&](const std::pair<int, int>& pair) {
			return pair.second == item.location;
		});

		if (findResult == std::end(panelIdToLocationId)) return;

		bool receiving = receiver == ap->get_player_number();

		std::string player = ap->get_player_alias(receiver);
		std::string itemName = ap->get_item_name(item.item, ap->get_player_game(receiver));
		std::string locationName = ap->get_location_name(item.location, ap->get_player_game(sender));

		bool hint = jsonArgs.type == "Hint";
		bool found = (jsonArgs.found) ? *jsonArgs.found : false;

		if (hint) {
			for (int seenAudioLog : async->seenAudioLogs) {
				if (inGameHints[seenAudioLog].locationID == item.location && inGameHints[seenAudioLog].playerNo == ap->get_player_number()) return;
			}
			for (int seenLaser : async->seenLasers) {
				if (inGameHints[seenLaser].locationID == item.location && inGameHints[seenLaser].playerNo == ap->get_player_number()) return;
			}

			std::string isFor = "";
			if (!receiving) isFor = " for " + player;

			if(!found) HudManager::get()->queueNotification("Hint: " + itemName + isFor + " is on " + locationName + ".");
		}
		else {
			int location = item.location;

			bool panelSolved = false;
			int panelId = panelIdToLocationIdReverse[location];

			if (panelIdToLocationIdReverse.count(location) && !async->CheckPanelHasBeenSolved(panelId)) {
				HudManager::get()->queueNotification("(Collect) Sent " + itemName + " to " + player + ".", getColorByItemFlag(item.flags));
			}
			else
			{
				if (item.item == ITEM_BONK_TRAP && receiving) {
					// Bonk will be played on item receive, but we should not interrupt ramping
					if (allEPs.count(panelId)) APAudioPlayer::get()->ResetRampingCooldownEP();
					else APAudioPlayer::get()->ResetRampingCooldownPanel();
				}
				else {
					async->PlaySentJingle(findResult->first, item.flags);
				}
				if(!receiving) HudManager::get()->queueNotification("Sent " + itemName + " to " + player + ".", getColorByItemFlag(item.flags));
			}
		}
	});

	clientWindow->logLine("Connect: Starting poller.");
	(new APServerPoller(ap))->start();

	auto start = std::chrono::system_clock::now();

	clientWindow->logLine("Connect: Waiting for connection result.");
	while (!hasConnectionResult) {
		if (DateTime::since(start).count() > 10000) { //5 seconnd timeout on waiting for response from server
			connected = false;
			hasConnectionResult = true;

			std::string errorMessage = "Timeout while connecting to server: " + uri;
			ClientWindow::get()->showMessageBox(errorMessage, "Error");
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return connected;
}

std::string APRandomizer::buildUri(std::string& server)
{
	std::string uri = server;

	if (uri.find(":") == std::string::npos)
		uri = uri + ":38281";
	else if (uri.back() == ':')
		uri = uri + "38281";

	return uri;
}

void APRandomizer::PreGeneration() {
	// Generate joke hints.
	std::vector<int> jokeHintOrder(GetJokeHints().size());
	std::iota(jokeHintOrder.begin(), jokeHintOrder.end(), 0);
	std::shuffle(jokeHintOrder.begin(), jokeHintOrder.end(), Random::gen);

	int jokeIndex = 0;
	std::vector<int> jokeHints;
	for (auto& [id, hint] : inGameHints) {
		if (hint.message.empty()) {
			jokeHints.push_back(id);
			hint.message = GetJokeHints().at(jokeHintOrder.at(jokeIndex));
			jokeIndex++;
		}
	}

	// Replace one junk hint with the credits hint.
	if (!jokeHints.empty()) {
		inGameHint& hint = inGameHints.at(jokeHints.at(Random::rand() % jokeHints.size()));
		hint.message = GetCreditsHint();
	}
}


void APRandomizer::PostGeneration() {
	ClientWindow* clientWindow = ClientWindow::get();
	Memory* memory = Memory::get();

	if (huntEntities.size()) { // Panelhunt mode -> Tutorial Gate Open is repurposed
		Memory::get()->PowerNext(0x03629, 0x36);

		if (!memory->ReadPanelData<int>(0x288E8, 0x1E4))
		{
			ASMPayloadManager::get()->OpenDoor(0x288E8);
		}

		if (!memory->ReadPanelData<int>(0x288F3, 0x1E4))
		{
			ASMPayloadManager::get()->OpenDoor(0x288F3);
		}

		if (!memory->ReadPanelData<int>(0x28942, 0x1E4))
		{
			ASMPayloadManager::get()->OpenDoor(0x28942);
		}

		memory->PowerGauge(0x003C4, 0x3F, 3);
		memory->PowerGauge(0x003C4, 0x3C, 2);
		memory->PowerGauge(0x003C4, 0x51, 1);

		memory->WritePanelData(0x3F, MOUNT_PARENT_ID, 0);
		memory->WritePanelData<float>(0x3F, POSITION, { 0.0f, 0.0f, -200.0f });
		memory->WritePanelData(0x3C, MOUNT_PARENT_ID, 0);
		memory->WritePanelData<float>(0x3C, POSITION, { 0.0f, 0.0f, -200.0f });
		memory->WritePanelData(0x51, MOUNT_PARENT_ID, 0);
		memory->WritePanelData<float>(0x51, POSITION, { 0.0f, 0.0f, -200.0f });

		ASMPayloadManager::get()->UpdateEntityPosition(0x3F);
		ASMPayloadManager::get()->UpdateEntityPosition(0x3C);
		ASMPayloadManager::get()->UpdateEntityPosition(0x51);

		// Make this obviously unsolvable to the bottom //
		Panel panel = Panel(0x0A3B5);
		std::vector<Endpoint> newEndpoints = {};
		for (auto endpoint : panel._endpoints) {
			if (endpoint.GetY() < 3) {
				newEndpoints.push_back(endpoint);
			}
		}
		panel._endpoints = newEndpoints;
		panel.Write();
		if (async->DoorWasLocked(0x03BA2)) {
			Special::clearTarget(0x0A3B5);
		}
		else {
			Special::setTarget(0x0A3B5, 0x3BA7);
		}
	}

	// Write gameplay relevant client options into datastorage

	clientWindow->logLine("Putting Settings in Datastorage.");
	ap->Set("WitnessSetting" + std::to_string(ap->get_player_number()) + "-Collect", NULL, false, {{"replace", CollectedPuzzlesBehavior}});
	ap->Set("WitnessSetting" + std::to_string(ap->get_player_number()) + "-Disabled", NULL, false, {{"replace", DisabledPanelsBehavior} });
	ap->Set("WitnessSetting" + std::to_string(ap->get_player_number()) + "-DisabledEPs", NULL, false, { {"replace", DisabledEPsBehavior} });
	ap->Set("WitnessSetting" + std::to_string(ap->get_player_number()) + "-SyncProgress", NULL, false, { {"replace", SyncProgress} });

	std::string deathLinkDisabledKey = "WitnessDisabledDeathLink" + std::to_string(ap->get_player_number()) + Utilities::wstring_to_utf8(Utilities::GetUUID());
	ap->SetNotify({ deathLinkDisabledKey });
	ap->Set(deathLinkDisabledKey, false, true, { {"default", false} });

	std::set<int64_t> allLocations;
	std::set<int64_t> missingLocations = ap->get_missing_locations();
	std::set<int64_t> checkedLocations = ap->get_checked_locations();
	allLocations.insert(missingLocations.begin(), missingLocations.end());
	allLocations.insert(checkedLocations.begin(), checkedLocations.end());
	std::list<int64_t> allLocationsList(allLocations.begin(), allLocations.end());

	// :)
	if (Utilities::isAprilFools()) {
		for (int panel : trivial_panels) {
			Special::swapStartAndEnd(panel);
		}
		for (int panel : door_timers) {
			Special::flipPanelHorizontally(panel);
		}
	}

	clientWindow->logLine("Making postgeneration game modifications.");
	// EP-related slowing down of certain bridges etc.
	if (EPShuffle) {
		clientWindow->setStatusMessage("Adjusting EP element speeds...");

		memory->WritePanelData<float>(0x005A2, OPEN_RATE, { 0.02f }); // Swamp Rotating Bridge, 2x (Instead of 4x)
		
		memory->WritePanelData<float>(0x09E26, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x (Instead of 2x)
		memory->WritePanelData<float>(0x09E98, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x
		memory->WritePanelData<float>(0x09ED4, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x
		memory->WritePanelData<float>(0x09EE3, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x
		memory->WritePanelData<float>(0x09F10, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x
		memory->WritePanelData<float>(0x09F11, OPEN_RATE, { 0.25f }); // Monastery Shutters, 1x

		memory->WritePanelData<float>(0x17E74, OPEN_RATE, { 0.03f }); // Swamp Flood gate (inner), 2x (Instead of 4x)
		memory->WritePanelData<float>(0x1802C, OPEN_RATE, { 0.03f }); // Swamp Flood gate (outer), 2x
	}

	// In Vanilla, Caves Invis Symmetry 3 turns on a power cable on Symmetry Island. This is never relevant in vanilla, but doors modes make it a legitimate issue.
	memory->WritePanelData<int>(0x00029, TARGET, { 0 }); 

	// Challenge Timer Colors
	memory->WritePanelData<float>(0x0A332, PATTERN_POINT_COLOR_A, { 0.0f, 1.0f, 1.0f, 1.0f });
	memory->WritePanelData<float>(0x0A332, PATTERN_POINT_COLOR_B, { 1.0f, 1.0f, 0.0f, 1.0f });

	clientWindow->setStatusMessage("Applying progression...");
	PreventSnipes(); //Prevents Snipes to preserve progression randomizer experience

	clientWindow->logLine("Changing mounatain laser box and goal condition.");

	bool rotateBox = MountainLasers > 7 || (RequiredHuntEntities && ChallengeLasers > 7 && (PanelHuntPostgame == 1 || PanelHuntPostgame == 3));

	if(rotateBox || MountainLasers != 7 || ChallengeLasers != 11) Special::SetRequiredLasers(MountainLasers, ChallengeLasers);
	if (rotateBox) {
		memory->WritePanelData<float>(0x09f7f, POSITION, { 153.95224, -53.066, 68.343, 0.97, -0.2462272793, 0.01424658205, -0.980078876, 0.06994858384 });
		ASMPayloadManager::get()->UpdateEntityPosition(0x09f7f);
	}

	if (FinalPanel == 0x03629) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Panel Hunt", " Lasers:", MountainLasers, ChallengeLasers);
		Special::DrawSingleVerticalLine(0x03629);
		Special::DrawSingleVerticalLine(0x03505);
	}
	else if (FinalPanel == 0x09F7F) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Box Short", " Lasers:", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0xFFF00) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Box Long", " Lasers:", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0x3D9A9) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Elevator", " Lasers:", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0x0356B) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Challenge", "Lasers:", MountainLasers, ChallengeLasers);
	}

	if (FinalPanel != 0x03629) {
		Memory::get()->turnOffEEE();
	}
	if (FinalPanel != 0x3D9A9) {
		Memory::get()->turnOffElevator();
	}

	for (int panel : desertPanels) {
		memory->UpdatePanelJunctions(panel);
	}
	
	clientWindow->logLine("Setting Puzzle Locks.");
	setPuzzleLocks();

	clientWindow->logLine("Restoring previously taken actions.");
	async->SkipPreviouslySkippedPuzzles();

	async->ResetPowerSurge();

	clientWindow->logLine("Apply patches.");
	Memory::get()->applyDestructivePatches();

	randomizationFinished = true;
	memory->showMsg = false;

	if (disabledEntities.size() > 0) {
		clientWindow->setStatusMessage("Disabling Puzzles...");
		for (int entityHex : disabledEntities) {
			async->DisablePuzzle(entityHex);
		}
	}

	if (precompletedLocations.size() > 0) {
		clientWindow->setStatusMessage("Precompleting Puzzles...");
		for (int checkID : precompletedLocations) {
			if (allPanels.count(checkID)) {
				async->SkipPanel(checkID, "Excluded", false);
			}
		}
	}

	clientWindow->logLine("Randomization finished.");
	HudManager::get()->queueBannerMessage("Randomized!");
	async->PlayFirstJingle();

	clientWindow->logLine("Starting APWatchdog.");
	async->start();

	clientWindow->logLine("Firing Locationscout.");
	ap->LocationScouts(allLocationsList);
}

void APRandomizer::AdjustPP4Colors() {
	Memory* memory = Memory::get();

	int pp4NumDecorations = memory->ReadPanelData<int>(0x01D3F, NUM_DECORATIONS);
	std::vector<int> pp4Decorations = memory->ReadArray<int>(0x01D3F, DECORATIONS, pp4NumDecorations);

	bool adjustColors = false;
	for (int i = 0; i < pp4Decorations.size(); i++) {
		int decoration = pp4Decorations[i];
		if ((decoration & 0x700) == Decoration::Shape::Poly && (decoration & 0x2000) == Decoration::Negative) {
			adjustColors = true;
			decoration &= ~0xF;
			decoration |= Decoration::Color::Blue;
			pp4Decorations[i] = decoration;
		}
	}

	memory->WriteArray(0x01D3F, DECORATIONS, pp4Decorations);

	if (adjustColors) {
		std::vector<float> swampRedOuter = memory->ReadPanelData<float>(0x00001, OUTER_BACKGROUND, 4);
		std::vector<float> swampRedInner = memory->ReadPanelData<float>(0x00001, BACKGROUND_REGION_COLOR, 4);

		swampRedOuter[0] *= 1.3f;
		swampRedInner[0] *= 1.3f;

		memory->WritePanelData<float>(0x01D3F, OUTER_BACKGROUND, swampRedOuter);
		memory->WritePanelData<float>(0x01D3F, BACKGROUND_REGION_COLOR, swampRedInner);
	}

	memory->WritePanelData<int>(0x01D3F, NEEDS_REDRAW, {1});
}

void APRandomizer::ColorBlindAdjustments() {
	AdjustPP4Colors();
}

void APRandomizer::HighContrastMode() {
	Memory* memory = Memory::get();

	std::vector<float> swampRedOuter = memory->ReadPanelData<float>(0x00982, OUTER_BACKGROUND, 4);
	std::vector<float> swampRedInner = memory->ReadPanelData<float>(0x00982, BACKGROUND_REGION_COLOR, 4);
	std::vector<float> pathColor = memory->ReadPanelData<float>(0x00982, PATH_COLOR, 4);
	std::vector<float> activeColor = memory->ReadPanelData<float>(0x00982, ACTIVE_COLOR, 4);

	for (int id : notThatBadSwampPanels) {
		memory->WritePanelData<float>(id, OUTER_BACKGROUND, swampRedOuter);
		memory->WritePanelData<float>(id, BACKGROUND_REGION_COLOR, swampRedInner);
		memory->WritePanelData<float>(id, PATH_COLOR, pathColor);
		memory->WritePanelData<float>(id, ACTIVE_COLOR, activeColor);
		memory->WritePanelData<int>(id, NEEDS_REDRAW, { 1 });
	}

	for (int i = 0; i < 3; i++) {
		swampRedOuter[i] *= 0.75f;
		swampRedInner[i] *= 0.75f;
		pathColor[i] *= 0.75f;
	}

	for (int id : swampLowContrastPanels) {
		memory->WritePanelData<float>(id, OUTER_BACKGROUND, swampRedOuter);
		memory->WritePanelData<float>(id, BACKGROUND_REGION_COLOR, swampRedInner);
		memory->WritePanelData<float>(id, PATH_COLOR, pathColor);
		memory->WritePanelData<float>(id, ACTIVE_COLOR, activeColor);
		memory->WritePanelData<int>(id, NEEDS_REDRAW, { 1 });
	}

	AdjustPP4Colors();
}

void APRandomizer::DisableColorCycle() {
	Memory* memory = Memory::get();

	for (int id : allPanels) {
		memory->WritePanelData<int>(id, COLOR_CYCLE_INDEX, -1);
	}

	memory->EnableKhatzEffects(false);
}

void APRandomizer::PatchColorCycle() {
	if (Utilities::isAprilFools()) {
		Memory::get()->PatchUpdatePanelForKhatz();
	}
}

void APRandomizer::setPuzzleLocks() {
	ClientWindow* clientWindow = ClientWindow::get();
	Memory* memory = Memory::get();

	const int puzzleCount = LockablePuzzles.size();
	for (int i = 0; i < puzzleCount; i++)	{
		clientWindow->setStatusMessage("Locking puzzles: " + std::to_string(i) + "/" + std::to_string(puzzleCount));
		if (!allPanels.count(LockablePuzzles[i]) || !memory->ReadPanelData<int>(LockablePuzzles[i], SOLVED) || lockEvenIfSolved.count(LockablePuzzles[i]))
			panelLocker->UpdatePuzzleLock(state, LockablePuzzles[i]);
	}
}

void APRandomizer::InitPanels() {
	Memory* memory = Memory::get();

	for (int panel : LockablePuzzles) {
		if (allPanels.count(panel)) {
			memory->InitPanel(panel);
		}
	}

	Special::SetVanillaMetapuzzleShapes();
}

void APRandomizer::RestoreOriginals() {
	PanelRestore::RestoreOriginalPanelData();
}

ApSettings APRandomizer::GetAPSettings() {
	int EggHuntStep = 0;
	if (EggHuntDifficulty == 1 || EggHuntDifficulty == 2) EggHuntStep = 3;
	if (EggHuntDifficulty >= 3) EggHuntStep = 4;

	ApSettings apSettings = ApSettings();
	apSettings.panelIdToLocationId = panelIdToLocationId;
	apSettings.lastPanel = FinalPanel;
	apSettings.inGameHints = inGameHints;
	apSettings.obeliskHexToEPHexes = obeliskSideIDsToEPHexes;
	apSettings.EPShuffle = EPShuffle;
	apSettings.PuzzleRandomization = PuzzleRandomization;
	apSettings.ElevatorsComeToYou = ElevatorsComeToYou;
	apSettings.DisabledEntities = disabledEntities;
	apSettings.ExcludedEntities = precompletedLocations;
	apSettings.huntEntites = huntEntities;
	apSettings.itemIdToDoorSet = itemIdToDoorSet;
	apSettings.doorToItemId = doorToItemId;
	apSettings.progressiveItems = progressiveItems;
	apSettings.warps = UnlockableWarps;
	apSettings.DeathLinkAmnesty = DeathLinkAmnesty;
	apSettings.EggHuntStep = EggHuntStep;
	apSettings.EggHuntDifficulty = EggHuntDifficulty;
	apSettings.VagueHintsLegacy = VagueHintsLegacy;
	return apSettings;
}

FixedClientSettings APRandomizer::GetFixedClientSettings() {
	FixedClientSettings fixedClientSettings = FixedClientSettings();
	fixedClientSettings.CollectedPuzzlesBehavior = CollectedPuzzlesBehavior;
	fixedClientSettings.DisabledPuzzlesBehavior = DisabledPanelsBehavior;
	fixedClientSettings.DisabledEPsBehavior = DisabledEPsBehavior;
	fixedClientSettings.SolveModeSpeedFactor = solveModeSpeedFactor;
	fixedClientSettings.SyncProgress = SyncProgress;
	return fixedClientSettings;
}

void APRandomizer::GenerateNormal() {
	ApSettings apSettings = GetAPSettings();
	FixedClientSettings fixedClientSettings = GetFixedClientSettings();
	async = new APWatchdog(ap, panelLocker, &state, &apSettings, &fixedClientSettings);
	SeverDoors();

	if (DisableNonRandomizedPuzzles)
		panelLocker->DisableNonRandomizedPuzzles(doorsActuallyInTheItemPool);
}

void APRandomizer::GenerateVariety() {
	ApSettings apSettings = GetAPSettings();
	FixedClientSettings fixedClientSettings = GetFixedClientSettings();
	async = new APWatchdog(ap, panelLocker, &state, &apSettings, &fixedClientSettings);
	SeverDoors();

	Memory::get()->PowerNext(0x03629, 0x36);

	if (DisableNonRandomizedPuzzles)
		panelLocker->DisableNonRandomizedPuzzles(doorsActuallyInTheItemPool);
}

void APRandomizer::GenerateHard() {
	ApSettings apSettings = GetAPSettings();
	FixedClientSettings fixedClientSettings = GetFixedClientSettings();
	async = new APWatchdog(ap, panelLocker, &state, &apSettings, &fixedClientSettings);
	SeverDoors();

	//Mess with Town targets
	Special::copyTarget(0x03C08, 0x28A0D); Special::copyTarget(0x28A0D, 0x28998);
	
	Special::setTargetAndDeactivate(0x03C0C, 0x03C08);

	if (doorsActuallyInTheItemPool.count(0x28A0D)) {
		Special::setTarget(0x28998, 0);
	}
	else {
		Special::setTargetAndDeactivate(0x28998, 0x28A0D);
	}

	Memory::get()->PowerNext(0x03629, 0x36);

	if (DisableNonRandomizedPuzzles)
		panelLocker->DisableNonRandomizedPuzzles(doorsActuallyInTheItemPool);
}

void APRandomizer::PreventSnipes()
{
	// Distance-gate shadows laser to prevent sniping through the bars
	if (doorsActuallyInTheItemPool.count(0x19665) && doorsActuallyInTheItemPool.count(0x194B2)) {
		Memory::get()->WritePanelData<float>(0x19650, MAX_BROADCAST_DISTANCE, { 2.7 });
	}
}

void APRandomizer::SkipPuzzle() {
	async->SkipPuzzle();
}

void APRandomizer::SeverDoors() {
	for (int id : doorsActuallyInTheItemPool) {
		async->SeverDoor(id);
	}
}

bool APRandomizer::InfiniteChallenge(bool enable) {
	if(randomizationFinished) async->InfiniteChallenge(enable);
	return randomizationFinished;
}

