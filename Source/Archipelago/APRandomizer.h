#pragma once

#include "APState.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <windows.h>

class ApSettings;
class FixedClientSettings;

class APRandomizer {
	public:
		APRandomizer();

		int Seed = 0;
		int FinalPanel = 0;
		bool SyncProgress = false;
		std::string CollectedPuzzlesBehavior = "Unchanged";
		std::string DisabledPanelsBehavior = "Prevent Solve";
		std::string DisabledEPsBehavior = "Prevent Solve";

		int PuzzleRandomization = 2;
		bool UnlockSymbols = false;
		std::vector<std::string> UnlockableWarps = {};
		bool EarlyUTM = false;
		bool DisableNonRandomizedPuzzles = false;
		bool EPShuffle = false;
		int MountainLasers = 7;
		int ChallengeLasers = 11;
		int RequiredHuntEntities = 0;
		int PanelHuntPostgame = 0;
		bool DeathLink;
		int DeathLinkAmnesty = 0;
		int EggHuntDifficulty = 0;
		std::set<std::string> ElevatorsComeToYou = {};

		int mostRecentItemId = -1;

		float solveModeSpeedFactor = 0.0f;

		bool connected = false;
		bool randomizationFinished = false;

		bool Connect(std::string& server, std::string& user, std::string& password);
		void PreGeneration();
		void GetOrCreateSaveGame();
		void PostGeneration();

		void GenerateNormal();
		void GenerateVariety();
		void GenerateHard();

		void HighContrastMode();
		void AdjustPP4Colors();
		void ColorBlindAdjustments();
		void DisableColorCycle();

		void PatchColorCycle();

		void SkipPuzzle();

		void SeverDoors();

		bool InfiniteChallenge(bool enable);

		void InitPanels();
		void RestoreOriginals();

	private:
		int unknownCounter = 10; //Try for 10 seconds to see if something else than "Unknown" shows up

		std::map<int, int> panelIdToLocationId;
		std::map<int, std::set<int>> itemIdToDoorSet;
		std::map<int, int> doorToItemId;
		std::set<int> doorsActuallyInTheItemPool;
		std::map<int, std::vector<int>> progressiveItems = {
			{ 158200, {158000, 158002}},
			{ 158210, {158010, 158001}},
			{ 158260, {158060, 158061}},
		};
		std::map<int, inGameHint> inGameHints;
		std::map<int, int> panelIdToLocationIdReverse;
		std::set<int> disabledEntities;
		std::set<int> doorsToSkipLocking;

		std::set<int> huntEntities;

		std::map<int, std::set<int>> obeliskSideIDsToEPHexes;
		std::set<int> precompletedLocations;
		std::map<int, std::string> entityToName;

		class APClient* ap = NULL;
		class APWatchdog* async;
		class PanelLocker* panelLocker;

		APState state = APState();

		void PreventSnipes();

		void setPuzzleLocks();

		ApSettings GetAPSettings();
		FixedClientSettings GetFixedClientSettings();

		std::string buildUri(std::string& server);

		GUID savegameGUID;
};

#define DATAPACKAGE_CACHE "ap_datapackage.json"


