#include "APRandomizer.h"

#include "../ClientWindow.h"
#include "../HUDManager.h"
#include "../Panels.h"
#include "../Randomizer.h"
#include "../Special.h"
#include "APGameData.h"
#include "APWatchdog.h"
#include "Client/apclientpp/apclient.hpp"
#include "PanelLocker.h"
#include "../DateTime.h"
#include "PanelRestore.h"

APRandomizer::APRandomizer() {
	panelLocker = new PanelLocker();
};

bool APRandomizer::Connect(std::string& server, std::string& user, std::string& password) {
	std::string uri = buildUri(server);

	if (ap) ap->reset();
	ap = new APClient("uuid", "The Witness", uri);

	try {	ap->set_data_package_from_file(DATAPACKAGE_CACHE);	}
	catch (std::exception) { /* ignore */ }

	bool connected = false;
	bool hasConnectionResult = false;

	ap->set_room_info_handler([&]() {
		const int item_handling_flags_all = 7;

		ap->ConnectSlot(user, password, item_handling_flags_all, {}, {0, 4, 2});
	});

	ap->set_location_checked_handler([&](const std::list<int64_t>& locations) {
		while (!randomizationFinished) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		for (const auto& locationId : locations)
			async->MarkLocationChecked(locationId, this->Collect);
	});

	ap->set_slot_disconnected_handler([&]() {
		async->getHudManager()->queueBannerMessage("Randomiser has been disconnected.");
		std::string message = "The randomizer seems to have unexpectedly disconnected. Please reload both the game and the randomizer.";

		MessageBoxA(GetActiveWindow(), message.c_str(), NULL, MB_OK);
	});

	ap->set_socket_disconnected_handler([&]() {
		async->getHudManager()->queueBannerMessage("Randomiser has been disconnected.");
		std::string message = "The randomizer seems to have unexpectedly disconnected. Please reload both the game and the randomizer.";

		MessageBoxA(GetActiveWindow(), message.c_str(), NULL, MB_OK);
	});

	ap->set_items_received_handler([&](const std::list<APClient::NetworkItem>& items) {
		while (!randomizationFinished) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		Memory::get()->WritePanelData<int>(0x0064, VIDEO_STATUS_COLOR + 12, {mostRecentItemId});
		
		for (const auto& item : items) {
			int realitem = item.item;
			int advancement = item.flags;

			if (progressiveItems.count(realitem)) {
				if (progressiveItems[realitem].size() == 0) {
					continue;
				}

				realitem = progressiveItems[realitem][0];
				progressiveItems[item.item].erase(progressiveItems[item.item].begin());
			}


			static const std::list<int64_t> lateUnlocks =
				{ ITEM_SPEED_BOOST, ITEM_PARTIAL_SPEED_BOOST, ITEM_MAX_SPEED_BOOST,
				  ITEM_TEMP_SPEED_REDUCTION, ITEM_POWER_SURGE };

			
			const bool unlockLater = std::find(lateUnlocks.begin(), lateUnlocks.end(), item.item) != lateUnlocks.end();
			if (!unlockLater) {
				unlockItem(realitem);
				panelLocker->UpdatePuzzleLocks(state, realitem);
			}

			if (itemIdToDoorSet.count(realitem)) {
				for (int doorHex : itemIdToDoorSet[realitem]) {
					async->UnlockDoor(doorHex);
				}
			}

			if (mostRecentItemId >= item.index + 1) continue;

			if (unlockLater) {
				unlockItem(realitem);
			}

			mostRecentItemId = item.index + 1;

			Memory::get()->WritePanelData<int>(0x0064, VIDEO_STATUS_COLOR + 12, {mostRecentItemId});
		}
	});

	ap->set_slot_connected_handler([&](const nlohmann::json& slotData) {
		Seed = slotData["seed"];
		FinalPanel = slotData["victory_location"];

		PuzzleRandomization = slotData.contains("puzzle_randomization") ? (int) slotData["puzzle_randomization"] : 0;
		UnlockSymbols = slotData.contains("shuffle_symbols") ? slotData["shuffle_symbols"] == true : true;
		EarlyUTM = slotData.contains("early_secret_area") ? slotData["early_secret_area"] == true : false;
		if (slotData.contains("mountain_lasers")) MountainLasers = slotData["mountain_lasers"];
		if (slotData.contains("challenge_lasers")) ChallengeLasers = slotData["challenge_lasers"];
		DisableNonRandomizedPuzzles = slotData.contains("disable_non_randomized_puzzles") ? slotData["disable_non_randomized_puzzles"] == true : false;
		EPShuffle = slotData.contains("shuffle_EPs") ? slotData["shuffle_EPs"] != 0 : false;
		DeathLink = slotData.contains("death_link") ? slotData["death_link"] == true : false;

		if (!UnlockSymbols) {
			state.unlockedArrows = true;
			state.unlockedColoredDots = true;
			state.unlockedColoredStones = true;
			state.unlockedDots = true;
			state.unlockedErasers = true;
			state.unlockedFullDots = true;
			state.unlockedInvisibleDots = true;
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

		state.requiredChallengeLasers = ChallengeLasers;
		state.requiredMountainLasers = MountainLasers;

		if (slotData.contains("progressive_item_lists")) {
			for (auto& [key, val] : slotData["progressive_item_lists"].items()) {
				int itemId = std::stoul(key, nullptr, 10);
				std::vector<int> v = val;

				progressiveItems.insert({ itemId, v });
			}
		}

		for (auto& [key, val] : slotData["panelhex_to_id"].items()) {
			int panelId = std::stoul(key, nullptr, 16);
			int locationId = val;

			panelIdToLocationId.insert({ panelId, locationId });
			panelIdToLocationIdReverse.insert({ locationId, panelId });
		}

		if (slotData.contains("obelisk_side_id_to_EPs")) {
			for (auto& [key, val] : slotData["obelisk_side_id_to_EPs"].items()) {
				int sideId = std::stoul(key, nullptr, 10);
				std::set<int> v = val;

				obeliskSideIDsToEPHexes.insert({ sideId, v });
			}
		}

		if (slotData.contains("ep_to_name")) {
			for (auto& [key, val] : slotData["ep_to_name"].items()) {
				int sideId = std::stoul(key, nullptr, 16);

				entityToName.insert({ sideId, val });
			}
		}

		if (slotData.contains("entity_to_name")) {
			for (auto& [key, val] : slotData["entity_to_name"].items()) {
				int sideId = std::stoul(key, nullptr, 16);

				entityToName.insert({ sideId, val });
			}
		}

		if (slotData.contains("precompleted_puzzles")) {
			for (int key : slotData["precompleted_puzzles"]) {
				precompletedLocations.insert(key);
			}
		}

		if (slotData.contains("disabled_panels")) {
			for (std::string keys : slotData["disabled_panels"]) {
				int key = std::stoul(keys, nullptr, 16);
				if (!actuallyEveryPanel.count(key)) continue;
				disabledPanels.insert(key);
			}
		}

		if (slotData.contains("item_id_to_door_hexes")) {
			for (auto& [key, val] : slotData["item_id_to_door_hexes"].items()) {
				int itemId = std::stoul(key, nullptr, 10);
				std::set<int> v = val;

				itemIdToDoorSet.insert({ itemId, v });
			}
		}

		if (slotData.contains("door_hexes_in_the_pool")) {
			for (int key : slotData["door_hexes_in_the_pool"]) {
				doorsActuallyInTheItemPool.insert(key);
			}
		}
		else
		{
			for (auto& [key, val] : itemIdToDoorSet) {
				for(int door : val){
					doorsActuallyInTheItemPool.insert(door);
				}
			}
		}

		if (slotData.contains("log_ids_to_hints")) {
			for (auto& [key, val] : slotData["log_ids_to_hints"].items()) {
				int logId = std::stoul(key, nullptr, 10);
				std::string message;
				int64_t location_id_in_this_world = -1;
				for (int i = 0; i < val.size() - 1; i++) {
					std::string line = val[i];
					if (!line.empty()) {
						message.append(line);
						message.append(" ");
					}
				}

				location_id_in_this_world = val[val.size() - 1];
				
				audioLogMessages.insert({ logId, std::pair(message, location_id_in_this_world) });
			}
		}

		if (DeathLink) {
			std::list<std::string> newTags = { "DeathLink" };

			ap->ConnectUpdate(false, 7, true, newTags);
		}

		connected = true;
		hasConnectionResult = true;
		cachedSlotData = slotData;
	});

	ap->set_slot_refused_handler([&](const std::list<std::string>& errors) {
		auto errorString = std::accumulate(errors.begin(), errors.end(), std::string(),
			[](const std::string& a, const std::string& b) -> std::string {
				return a + (a.length() > 0 ? "," : "") + b;
			});

		connected = false;
		hasConnectionResult = true;

		ClientWindow::get()->showMessageBox("Connection failed: " + errorString);
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

	ap->set_retrieved_handler([&](const std::map <std::string, nlohmann::json> response) {
		for (auto [key, value] : response) {
			if(key.find("WitnessLaser") != std::string::npos) async->HandleLaserResponse(key, value, Collect);
			if(key.find("WitnessEP") != std::string::npos) async->HandleEPResponse(key, value, Collect);
		}
	});

	ap->set_set_reply_handler([&](const std::string key, const nlohmann::json value, nlohmann::json original_value) {
		if (value != original_value) {
			if(key.find("WitnessLaser") != std::string::npos) async->HandleLaserResponse(key, value, Collect);
			if(key.find("WitnessEP") != std::string::npos) async->HandleEPResponse(key, value, Collect);
		}
	});

	ap->set_print_json_handler([&](const APClient::PrintJSONArgs& jsonArgs) {
		int localPlayerId = ap->get_player_number();
		
		if (jsonArgs.type == "ItemSend") {
			std::string itemName = ap->get_item_name(jsonArgs.item->item);
			if (jsonArgs.item->player == localPlayerId) {
				// The item was sent from the local player's world. Update the corresponding panel's color.
				const auto matchingPanel = std::find_if(std::begin(panelIdToLocationId), std::end(panelIdToLocationId),
				                                        [&](const std::pair<int, int>& pair) {
					                                        return pair.second == jsonArgs.item->location;
				                                        });
				if (matchingPanel != panelIdToLocationId.end()) {
					async->SetPanelItemTypeColor(matchingPanel->first, jsonArgs.item->flags);
				}

				if (*jsonArgs.receiving == localPlayerId) {
					if (!collectedPlayers.count(localPlayerId)) {
						// Found one of our own items. If it's a speed item, grant the corresponding amount of energy,
						//   and if it's not, grant a small fill and note that in the message.
						bool showNotification = true;
						switch (jsonArgs.item->item) {
							case ITEM_PARTIAL_SPEED_BOOST:
								async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial);
								showNotification = false;
								break;
							case ITEM_SPEED_BOOST:
								async->GrantSpeedBoostFill(SpeedBoostFillSize::Full);
								break;
							case ITEM_MAX_SPEED_BOOST:
								async->GrantSpeedBoostFill(SpeedBoostFillSize::MaxFill);
								break;
							default:
								async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial);
								break;
						}

						if (showNotification) {
							async->getHudManager()->queueItemMessage(itemName, "", jsonArgs.item->flags, false);
						}
					}
					else {
						// Collected one of our own items. If it's a speed item, grant the corresponding amount of
						//   energy directly.
						switch (jsonArgs.item->item) {
							case ITEM_PARTIAL_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial); break;
							case ITEM_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::Full); break;
							case ITEM_MAX_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::MaxFill); break;
							default: break;
						}
					}
				}
				else if (!collectedPlayers.count(*jsonArgs.receiving) && !releasedPlayers.count(localPlayerId)) {
					// The player found someone else's item.
					const std::string remotePlayerName = ap->get_player_alias(*jsonArgs.receiving);
					async->getHudManager()->queueItemMessage(itemName, remotePlayerName, jsonArgs.item->flags, true);

					// When sending an item, always grant a partial boost energy fill.
					async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial);
				}
			}
			else if (*jsonArgs.receiving == localPlayerId) {
				// This item was sent to the local player from a remote player.
				if (!collectedPlayers.count(localPlayerId)) {
					// Received one of our own items.
					switch (jsonArgs.item->item) {
						case ITEM_PARTIAL_SPEED_BOOST:
							async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial);
							break;
						case ITEM_SPEED_BOOST:
							async->GrantSpeedBoostFill(SpeedBoostFillSize::Full);
							break;
						case ITEM_MAX_SPEED_BOOST:
							async->GrantSpeedBoostFill(SpeedBoostFillSize::MaxFill);
							break;
						default:
							break;
					}

					const std::string remotePlayerName = ap->get_player_alias(jsonArgs.item->player);
					async->getHudManager()->queueItemMessage(itemName, remotePlayerName, jsonArgs.item->flags, false);
				}
				else {
					// When collecting items from other players, don't show messages.
					switch (jsonArgs.item->item) {
						case ITEM_PARTIAL_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::Partial); break;
						case ITEM_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::Full); break;
						case ITEM_MAX_SPEED_BOOST: async->GrantSpeedBoostFill(SpeedBoostFillSize::MaxFill); break;
						default: break;
					}
				}
			}
		}
		else if (jsonArgs.type == "Hint") {
			// Don't show hints that we triggered via audio log, have already shown, or are no longer relevant.
			if (async->seenAudioHintLocations.count(jsonArgs.item->location) ||
				*jsonArgs.found ||
				std::find_if(seenHints.begin(), seenHints.end(), [&jsonArgs](const std::pair<int,int>& seenHint) {
					return seenHint.first == jsonArgs.item->player && seenHint.second == jsonArgs.item->location;
				}) != seenHints.end()) return;

			seenHints.emplace_back(jsonArgs.item->player, jsonArgs.item->location);
			
			std::string itemName = ap->get_item_name(jsonArgs.item->item);
			std::string locationName = ap->get_location_name(jsonArgs.item->location);
			
			if (*jsonArgs.receiving == localPlayerId) {
				if (jsonArgs.item->player == localPlayerId) {
					async->getHudManager()->queueNotification(
						"Hint: Your " + itemName + " is at " + locationName + ".");
				}
				else {
					std::string remotePlayerName = ap->get_player_alias(jsonArgs.item->player);
					async->getHudManager()->queueNotification(
						"Hint: Your " + itemName + " is at " + locationName + " in " + remotePlayerName + "'s world.");
				}
			}
			else if (jsonArgs.item->player == localPlayerId) {
				std::string remotePlayerName = ap->get_player_alias(jsonArgs.item->player);
				async->getHudManager()->queueNotification(
					"Hint: " + remotePlayerName + "'s " + itemName + " is at " + locationName + " in your world.");
			}
		}
		else if (jsonArgs.type == "Goal") {
			if (*jsonArgs.slot == localPlayerId) {
				async->getHudManager()->queueNotification("You have reached your goal.");
			}
			else if (*jsonArgs.team == ap->get_team_number()) {
				std::string remotePlayerName = ap->get_player_alias(*jsonArgs.slot);
				async->getHudManager()->queueNotification(remotePlayerName + " has reached their goal.");
			}
		}
		else if (jsonArgs.type == "Collect") {
			collectedPlayers.insert(*jsonArgs.slot);
			if (*jsonArgs.slot == localPlayerId) {
				async->getHudManager()->queueNotification("Collected all of your items.");
			}
			else if (*jsonArgs.team == ap->get_team_number()) {
				std::string remotePlayerName = ap->get_player_alias(*jsonArgs.slot);
				async->getHudManager()->queueNotification(remotePlayerName + " collected their items from your world.");
			}
		}
		else if (jsonArgs.type == "Release") {
			releasedPlayers.insert(*jsonArgs.slot);
			if (*jsonArgs.slot == localPlayerId) {
				async->getHudManager()->queueNotification("Released remaining items in your world to their owners.");
			}
			else if (*jsonArgs.team == ap->get_team_number()) {
				std::string remotePlayerName = ap->get_player_alias(*jsonArgs.slot);
				async->getHudManager()->queueNotification(remotePlayerName + " has released your items.");
			}
		}
	});

	ap->set_data_package_changed_handler([&](const nlohmann::json& data) {
		ap->save_data_package(DATAPACKAGE_CACHE);
	});

	(new APServerPoller(ap))->start();

	auto start = std::chrono::system_clock::now();

	while (!hasConnectionResult) {
		if (DateTime::since(start).count() > 5000) { //5 seconnd timeout on waiting for response from server
			connected = false;
			hasConnectionResult = true;

			std::string errorMessage = "Timeout while connecting to server: " + uri;
			ClientWindow::get()->showMessageBox(errorMessage);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return connected;
}

void APRandomizer::unlockItem(int item) {
	switch (item) {
		// Puzzle symbols
		case ITEM_DOTS: state.unlockedDots = true; break;
		case ITEM_COLORED_DOTS: state.unlockedColoredDots = true; break;
		case ITEM_FULL_DOTS: state.unlockedFullDots = true; break;
		case ITEM_SOUND_DOTS: state.unlockedSoundDots = true; break;
		case ITEM_SYMMETRY: state.unlockedSymmetry = true; break;
		case ITEM_TRIANGLES: state.unlockedTriangles = true; break;
		case ITEM_ERASOR: state.unlockedErasers = true; break;
		case ITEM_TETRIS: state.unlockedTetris = true; break;
		case ITEM_TETRIS_ROTATED: state.unlockedTetrisRotated = true; break;
		case ITEM_TETRIS_NEGATIVE: state.unlockedTetrisNegative = true; break;
		case ITEM_STARS: state.unlockedStars = true; break;
		case ITEM_STARS_WITH_OTHER_SYMBOL: state.unlockedStarsWithOtherSimbol = true; break;
		case ITEM_B_W_SQUARES: state.unlockedStones = true; break;
		case ITEM_COLORED_SQUARES: state.unlockedColoredStones = true; break;
		case ITEM_SQUARES: state.unlockedStones = state.unlockedColoredStones = true; break;
		case ITEM_ARROWS: state.unlockedArrows = true; break;

		// Powerups. Note that speed boost items are handled on JSON message receipt, not here.
		case ITEM_PUZZLE_SKIP: async->AddPuzzleSkip(); break;
		case ITEM_BOOST_CAPACITY: async->GrantSpeedBoostCapacity(); break;

		// Traps
		case ITEM_POWER_SURGE: async->TriggerPowerSurge(); break;
		case ITEM_TEMP_SPEED_REDUCTION: async->TriggerSlownessTrap(); break;

		default: break;
	}
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

void APRandomizer::PostGeneration() {
	ClientWindow* clientWindow = ClientWindow::get();
	Memory* memory = Memory::get();

	if (precompletedLocations.size() > 0) {
		clientWindow->setStatusMessage("Precompleting EPs...");
		for (int checkID : precompletedLocations) {
			if (allEPs.count(checkID)) {
				memory->SolveEP(checkID);
				if (precompletableEpToName.count(checkID) && precompletableEpToPatternPointBytes.count(checkID)) {
					memory->MakeEPGlow(precompletableEpToName.at(checkID), precompletableEpToPatternPointBytes.at(checkID));
				}
			}
			else if (actuallyEveryPanel.count(checkID)) {
				panelLocker->PermanentlyUnlockPuzzle(checkID);
				Special::SkipPanel(checkID, "Excluded", false);
				memory->WritePanelData<float>(checkID, POWER, { 1.0f, 1.0f });
			}
		}
	}

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
	}

	// Bunker door colors
	clientWindow->setStatusMessage("Setting additional colors...");
	int num_dec = memory->ReadPanelData<int>(0x17C2E, NUM_DECORATIONS);
	if (num_dec != 1){
		std::vector<int> decorations = memory->ReadArray<int>(0x17C2E, DECORATIONS, num_dec);

		decorations[3] = 266;
		decorations[12] = 266;

		memory->WriteArray<int>(0x17C2E, DECORATIONS, decorations);
	}

	// In Vanilla, Caves Invis Symmetry 3 turns on a power cable on Symmetry Island. This is never relevant in vanilla, but doors modes make it a legitimate issue.
	memory->WritePanelData<int>(0x00029, TARGET, { 0 }); 

	// Challenge Timer Colors
	memory->WritePanelData<float>(0x0A332, PATTERN_POINT_COLOR_A, { 0.0f, 1.0f, 1.0f, 1.0f });
	memory->WritePanelData<float>(0x0A332, PATTERN_POINT_COLOR_B, { 1.0f, 1.0f, 0.0f, 1.0f });

	clientWindow->setStatusMessage("Applying progression...");
	PreventSnipes(); //Prevents Snipes to preserve progression randomizer experience

	if(MountainLasers != 7 || ChallengeLasers != 11) Special::SetRequiredLasers(MountainLasers, ChallengeLasers);

	if (FinalPanel == 0x09F7F) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Box Short", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0xFFF00) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Box Long", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0x3D9A9) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Elevator", MountainLasers, ChallengeLasers);
	}
	else if (FinalPanel == 0x0356B) {
		Special::writeGoalCondition(0x0042D, " Goal:", "Challenge", MountainLasers, ChallengeLasers);
	}

	async->SkipPreviouslySkippedPuzzles();

	for (int panel : desertPanels) {
		memory->UpdatePanelJunctions(panel);
	}
	
	setPuzzleLocks();

	async->ResetPowerSurge();

	Memory::get()->applyDestructivePatches();

	randomizationFinished = true;
	memory->showMsg = false;

	async->getHudManager()->queueBannerMessage("Randomized!");

	async->start();
}

void APRandomizer::setPuzzleLocks() {
	ClientWindow* clientWindow = ClientWindow::get();
	Memory* memory = Memory::get();

	const int puzzleCount = sizeof(AllPuzzles) / sizeof(AllPuzzles[0]);
	for (int i = 0; i < puzzleCount; i++)	{
		clientWindow->setStatusMessage("Locking puzzles: " + std::to_string(i) + "/" + std::to_string(puzzleCount));
		if (!memory->ReadPanelData<int>(AllPuzzles[i], SOLVED));
			panelLocker->UpdatePuzzleLock(state, AllPuzzles[i]);
	}
}

void APRandomizer::Init() {
	Memory* memory = Memory::get();

	mostRecentItemId = memory->ReadPanelData<int>(0x0064, VIDEO_STATUS_COLOR + 12);

	for (int panel : AllPuzzles) {
		memory->InitPanel(panel);
	}

	PanelRestore::RestoreOriginalPanelData();

	Special::SetVanillaMetapuzzleShapes();
}

void APRandomizer::GenerateNormal() {
	async = new APWatchdog(ap, panelIdToLocationId, FinalPanel, panelLocker, entityToName, audioLogMessages, obeliskSideIDsToEPHexes, EPShuffle, PuzzleRandomization, &state, DeathLink);
	async->SetPlaytestParameters(cachedSlotData);
	SeverDoors();

	if (DisableNonRandomizedPuzzles)
		panelLocker->DisableNonRandomizedPuzzles(disabledPanels, doorsActuallyInTheItemPool);
}

void APRandomizer::GenerateHard() {
	async = new APWatchdog(ap, panelIdToLocationId, FinalPanel, panelLocker, entityToName, audioLogMessages, obeliskSideIDsToEPHexes, EPShuffle, PuzzleRandomization, &state, DeathLink);
	async->SetPlaytestParameters(cachedSlotData);
	SeverDoors();

	//Mess with Town targets
	Special::copyTarget(0x03C08, 0x28A0D); Special::copyTarget(0x28A0D, 0x28998);
	
	Special::setTargetAndDeactivate(0x03C0C, 0x03C08);

	if (doorsActuallyInTheItemPool.count(0x28A0D)) {
		Special::setPower(0x28A0D, false);
	}
	else
	{
		Special::setTargetAndDeactivate(0x28998, 0x28A0D);
	}

	Memory::get()->PowerNext(0x03629, 0x36);

	if (DisableNonRandomizedPuzzles)
		panelLocker->DisableNonRandomizedPuzzles(disabledPanels, doorsActuallyInTheItemPool);
}

void APRandomizer::PreventSnipes()
{
	// Distance-gate shadows laser to prevent sniping through the bars
	Memory::get()->WritePanelData<float>(0x19650, MAX_BROADCAST_DISTANCE, {2.7});
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

