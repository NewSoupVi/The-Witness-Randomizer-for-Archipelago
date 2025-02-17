#pragma once


#include "../Watchdog.h"
#include "APGameData.h"
#include "APWitnessData.h"
#include "APState.h"
#include "nlohmann/json.hpp"
#include "Client/apclientpp/apclient.hpp"

#include <chrono>
#include <map>
#include <queue>

class Generate;
class DrawIngameManager;
class PanelLocker;
class Color;

class ApSettings {
public:
	std::map<int, int> panelIdToLocationId;
	int lastPanel;
	std::map<int, inGameHint> inGameHints;
	std::map<int, std::set<int>> obeliskHexToEPHexes;
	bool EPShuffle;
	int PuzzleRandomization;
	std::set<std::string> ElevatorsComeToYou;
	std::set<int> DisabledEntities;
	std::set<int> ExcludedEntities;
	std::set<int> huntEntites;
	std::map<int, std::set<int>> itemIdToDoorSet;
	std::map<int, int> doorToItemId;
	std::map<int, std::vector<int>> progressiveItems;
	std::vector<std::string> warps;
	int DeathLinkAmnesty;
	int EggHuntStep;
	int EggHuntDifficulty;
};

class FixedClientSettings {
public:
	std::string CollectedPuzzlesBehavior;
	std::string DisabledPuzzlesBehavior;
	std::string DisabledEPsBehavior;
	float SolveModeSpeedFactor;
	bool SyncProgress;
};


class APWatchdog : public Watchdog {
public:
	APWatchdog(APClient* client, PanelLocker* panelLocker, APState* state, ApSettings* apSettings, FixedClientSettings* fixedClientSettings);

	int DEATHLINK_DURATION = 15;

	int spentPuzzleSkips = 0;
	int foundPuzzleSkips = 0;

	APState* state;

	virtual void action();

	void MarkLocationChecked(int64_t locationId);
	void ApplyTemporarySpeedBoost();
	void ApplyTemporarySlow();
	void TriggerPowerSurge();
	void ResetPowerSurge();

	float TriggerBonk(bool fromDeathLink);
	void ResetDeathLink();

	void StartRebindingKey(enum class CustomKey key);

	void QueueItem(APClient::NetworkItem);
	void HandleReceivedItems();

	void AddPuzzleSkip();
	void SkipPuzzle();
	void SkipPreviouslySkippedPuzzles();

	void UnlockDoor(int id);
	void SeverDoor(int id);

	void SkipPanel(int id, std::string reason, bool kickOut) {
		SkipPanel(id, reason, kickOut, 0);
	}
	void SkipPanel(int id, std::string reason, bool kickOut, int cost, bool allowRecursion = true);

	void DisablePuzzle(int id);

	void DoubleDoorTargetHack(int id);

	void SetItemRewardColor(const int& id, const int& itemFlags);
	void WriteRewardColorToPanel(int id, Color color);
	bool PanelShouldPlayEpicVersion(const int& id);
	void PlaySentJingle(const int& id, const int& itemFlags);
	void PlayReceivedJingle(const int& itemFlags);
	void PlayFirstJingle();
	void PlayEntityHuntJingle(const int& huntEntity);

	bool CheckPanelHasBeenSolved(int panelId);

	void SetValueFromServer(std::string key, nlohmann::json value);
	void HandleLaserResponse(std::string laserID, nlohmann::json value);
	void HandleWarpResponse(nlohmann::json value);
	void HandleEPResponse(std::string epID, nlohmann::json value);
	void HandleAudioLogResponse(std::string logIDstr, nlohmann::json value);
	void HandleLaserHintResponse(std::string laserIDstr, nlohmann::json value);
	void HandleSolvedPanelsResponse(nlohmann::json value);
	void HandleEasterEggResponse(std::string key, nlohmann::json value);
	void UnlockEggs(std::set<int> eggs, bool local);
	void HandleHuntEntityResponse(nlohmann::json value);
	void HandleOpenedDoorsResponse(nlohmann::json value);
	void setLocationItemFlag(int64_t location, unsigned int flags);

	void PotentiallyColorPanel(int64_t location) {
		PotentiallyColorPanel(location, false);
	}
	void PotentiallyColorPanel(int64_t location, bool overrideRecolor);

	void InfiniteChallenge(bool enable);

	void ProcessDeathLink(double time, std::string cause, std::string source);

	bool newItemsJustIn = false;
	bool firstDataStoreResponse = false;

	void QueueReceivedItem(std::vector<__int64> item);

	std::set<int> seenAudioLogs;
	std::set<int> seenLasers;

	bool DoorWasLocked(int id) {
		return lockedDoors.contains(id);
	}

private:
	APClient* ap;
	PanelLocker* panelLocker;
	std::shared_ptr<Generate> generator;
	std::map<int, int> panelIdToLocationId;
	std::map<int, int> panelIdToLocationId_READ_ONLY;
	std::map<int, int> locationIdToPanelId_READ_ONLY;
	std::map<int64_t, unsigned int> locationIdToItemFlags;
	std::set<int64_t> checkedLocations;
	std::set<int> recolorWhenSolved;
	std::set<int> neverRecolorAgain;
	std::set<int> alreadyPlayedHuntEntityJingle;
	std::set<std::pair<int, int64_t>> locationsThatContainedItemsFromOtherPlayers;
	int finalPanel;
	bool isCompleted = false;
	bool eee = false;
	bool desertLaserHasBeenUpWhileConnected = false;
	int sphereOpacityModifier = 0;

	std::queue<APClient::NetworkItem> queuedReceivedItems = {};

	std::chrono::system_clock::time_point lastFrameTime;
	float halfSecondCountdown = 0.f;
	float timePassedSinceRandomisation = 0.0f;
	float timePassedSinceFirstJinglePlayed = 0.0f;

	bool firstTimeActiveEntity = false;
	bool deathLinkFirstResponse = false;

	int halfSecondCounter = 0;

	int mostRecentItemId = -1;

	std::map<int, std::set<int>> itemIdToDoorSet;
	std::map<int, std::vector<int>> progressiveItems;

	int DeathLinkAmnesty = 0;
	int DeathLinkCount = 0;

	std::set<std::string> ElevatorsComeToYou = {};
	bool EPShuffle = false;
	int PuzzleRandomization = 0;

	std::string Collect = "Unchanged";
	std::string CollectText = "Unchanged";
	bool CollectUnlock = false;
	std::string DisabledPuzzlesBehavior = "Prevent Solve";
	std::string DisabledEPsBehavior = "Prevent Solve";
	std::set<int> DisabledEntities;
	bool SyncProgress = false;

	std::map<int, bool> huntEntityToSolveStatus;
	std::map<std::string, std::set<int>> huntEntitiesPerArea;
	std::map<int, float> huntEntitySphereOpacities;
	std::map<int, Vector3> huntEntityPositions;
	std::map<int, float> huntEntityKeepActive;

	std::map<int, bool> easterEggToSolveStatus;
	std::map<std::string, std::set<int>> unsolvedEasterEggsPerArea;
	std::map<int, std::string> easterEggToAreaName;
	bool firstEggShouldSendMessage = false;
	int EggHuntStep = 0;
	int EggHuntDifficulty = 0;
	int HighestRealEggCheck = 0; // Not excluded
	int HighestEggCheck = 0;

	std::map<int, int> doorToItemId;

	bool FirstEverLocationCheckDone = false;
	bool locationCheckInProgress = false;
	bool firstStorageCheckDone = false;
	bool firstActionDone = false;
	bool firstJinglePlayed = false;

	bool hasPowerSurge = false;

	std::chrono::system_clock::time_point powerSurgeStartTime;

	std::set<double> deathLinkTimestamps;

	std::set<int> disableCollisionList;

	std::set<int> severedDoorsList;
	std::set<int> lockedDoors;
	std::set<int> unlockedDoors;
	std::map<int, std::vector<float>> collisionPositions;
	std::set<int> alreadyTriedUpdatingNormally;

	std::map<int, Vector3> recordedEntityPositions = {};

	int storageCheckCounter = 6;

	float speedTime = 0.0f;
	float bonkTime = 0.0f;
	int amountOfBonksInCurrentBonk = 0;
	float solveModeSpeedFactor = 0.0f;

	bool infiniteChallenge = false;
	bool infiniteChallengeIsValid = false;
	bool insideChallengeBoxRange = true;

	bool symmetryMessageDelivered = false;
	bool ppMessageDelivered = false;

	std::mt19937 rng = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
	int tutorialCableFlashState = 0;
	int tutorialCableStateChangedRecently = 0;

	void HandleKeyTaps();

	void HandleInteractionState();

	bool IsPanelSolved(int id, bool ensureSet);

	std::vector<int> CheckCompletedHuntEntities();
	
	void CheckEPSkips();

	void CheckDeathLink();
	void SendDeathLink(int panelId);

	void QueueItemMessages();

	void DisableCollisions();

	void HandleInGameHints(float deltaSeconds);

	void CheckObeliskSides();
	void CheckSolvedPanels();
	void HandleMovementSpeed(float deltaSeconds);
	void HandlePowerSurge();
	void HandleKnockout(float deltaSeconds);
	bool IsEncumbered();
	void HandleEncumberment(float deltaSeconds, bool doFunctions);
	void HandleWarp(float deltaSeconds);

	void LookingAtLockedEntity();

	void UpdateCursorVisual(int cursorVisual);

	enum cursorVisual : int {
		normal = 0,
		recolored = 1,
		big = 2,
	};

	int PettingTheDog(float deltaSeconds);
	bool LookingAtTheDog() const;
	int LookingAtEasterEgg();
	int HandleEasterEgg();

	// Updates puzzle skip logic.
	void UpdatePuzzleSkip(float deltaSeconds);

	// Whether or not the player can use a puzzle skip. Checks if the player has a
	//   puzzle selected, if they can afford the cost, etc.
	bool CanUsePuzzleSkip() const;

	// Whether or not the puzzle is in a skippable state (unlocked, not skipped, etc.),
	//   NOT whether or not the player can skip the puzzle.
	bool PuzzleIsSkippable(int puzzleId) const;

	// Computes the cost to skip the puzzle. If the cost is unusual, will set
	//   specialMessage to indicate the reason why.
	int CalculatePuzzleSkipCost(int puzzleId, std::string& specialMessage) const;

	// Returns the number of skips currently available to the player.
	int GetAvailablePuzzleSkips() const;

	void CheckLaserHints();
	void CheckAudioLogHints();
	void CheckLasers();
	void CheckEPs();
	void CheckPanels();
	void CheckHuntEntities();
	void CheckDoors();

	void UpdateAreaEgg(int entityID);
	void ClearEmptyEggAreasAndSendNotification(int specificCollectedEggID = -1);

	void CheckImportantCollisionCubes();

	void SetStatusMessages();

	int GetActivePanel();
	void WriteMovementSpeed(float currentSpeed);

	void UpdateInfiniteChallenge();

	void unlockItem(int item);

	void CheckFinalRoom();

	void ToggleSleep();
	void TryWarp();

	void CheckUnlockedWarps();

	void UnlockWarps(std::vector<std::string> warps, bool local);

	std::string startingWarp = "Tutorial First Hallway";
	std::map<std::string, bool> unlockableWarps = {};
	std::set<std::string> badWarps = {};
	std::vector<Warp*> unlockedWarps = {};
	Warp* selectedWarp = NULL;
	bool hasTeleported = true;
	bool hasDoneTutorialMarker = true;
	bool hasTriedExitingNoclip = true;
	std::chrono::system_clock::time_point attemptedWarpCompletion;
	float warpRetryTime = 0.0f;

	void DoAprilFoolsEffects(float deltaSeconds);

	Vector3 getHuntEntitySpherePosition(int huntEntity);

	void DrawSpheres(float deltaSeconds);

	void FlickerCable();

	Vector3 getCachedEntityPosition(int id);

	Vector3 getCameraDirection();

	std::map<std::string, int> laserIDsToLasers;
	std::list<std::string> laserIDs;
	std::map<int, std::string> laserMessages;

	std::set<int> solvedPanels;
	std::set<int> solvedEPs;
	std::set<int> activatedLasers;
	std::set<int> openedDoors;
	std::set<int> solvedHuntEntitiesDataStorage;
	std::set<int> solvedEasterEggsDataStorage;
	std::map<std::string, bool> lastDeadChecks;

	std::list<std::string> EPIDs;
	std::map<int, bool> EPStates;

	std::set<int> PuzzlesSkippedThisGame = {};

	int currentHintEntity = -1;
	int lastRealHintEntity = -1;
	int lastLaser = -1;
	int lastAudioLog = -1;
	float currentHintEntityDuration = 0.f;

	int activePanelId = -1;
	int mostRecentActivePanelId = -1;
	int mostRecentPanelState = -1;
	int lookingAtLockedEntity = -1;

	std::string puzzleSkipInfoMessage;
	float skipButtonHeldTime = 0.f; // Tracks how long the skip button has been held.

	std::string DeathLinkDataStorageKey = "";

	// The cost to skip the currently-selected puzzle. -1 if the puzzle cannot be
	//   skipped for whatever reason.
	int puzzleSkipCost = -1;

	std::map<int, inGameHint> inGameHints = {};
	std::map<int, std::set<int>> obeliskHexToEPHexes = {};
	std::map<int, int> epToObeliskSides = {};
	std::map<int, int> obeliskHexToAmountOfEPs = {};

	Vector3 lastMouseDirection;
	bool letGoOfLeftClickSinceEnteringFocusMode = false;
	float dogPettingDuration = 0.f;
	float dogBarkDuration = 0.f;
	bool sentDog = false;
	bool letGoSinceInteractModeOpen = false;

	double finalRoomMusicTimer = -1;

	std::set<int> panelsThatHaveToBeSkippedForEPPurposes = {};

	bool metaPuzzleMessageHasBeenDisplayed = false;

	std::vector<std::vector<__int64>> queuedItems;
};

class APServerPoller : public Watchdog {
public:

	APServerPoller(APClient* client);
	virtual void action();

private:
	APClient* ap;
};
