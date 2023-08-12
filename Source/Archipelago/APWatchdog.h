#pragma once


#include "../Watchdog.h"
#include "APGameData.h"
#include "APState.h"
#include "nlohmann/json.hpp"

#include <chrono>
#include <map>

class APClient;
class Generate;
class HudManager;
class PanelLocker;


enum SpeedBoostFillSize
{
	Partial,
	Full,
	MaxFill
};


class APWatchdog : public Watchdog {
public:
	APWatchdog(APClient* client, std::map<int, int> mapping, int lastPanel, PanelLocker* p, std::map<int, std::string> epn, std::map<int, std::pair<std::string, int64_t>> a, std::map<int, std::set<int>> o, bool ep, int puzzle_rando, APState* s, bool dl);

	APState* state;

	virtual void action();

	void SetPlaytestParameters(const nlohmann::json& slotData);

	void MarkLocationChecked(int locationId, bool collect);

	void GrantSpeedBoostFill(SpeedBoostFillSize size);
	void GrantSpeedBoostCapacity();
	void TryTriggerSpeedBoost();

	void TriggerSlownessTrap();
	
	void TriggerPowerSurge();
	void ResetPowerSurge();

	void StartRebindingKey(enum class CustomKey key);

	void AddPuzzleSkip();
	void SkipPuzzle();
	void SkipPreviouslySkippedPuzzles();

	void UnlockDoor(int id);
	void SeverDoor(int id);

	void DoubleDoorTargetHack(int id);

	void SetPanelItemTypeColor(const int& id, const unsigned int& itemFlags);

	bool CheckPanelHasBeenSolved(int panelId);

	void HandleLaserResponse(std::string laserID, nlohmann::json value, bool collect);
	void HandleEPResponse(std::string epID, nlohmann::json value, bool collect);

	void InfiniteChallenge(bool enable);

	void ProcessDeathLink(double time, std::string cause, std::string source);

	HudManager* getHudManager() const { return hudManager.get(); }

	std::set<int64_t> seenAudioHintLocations;

private:
	APClient* ap;
	PanelLocker* panelLocker;
	std::shared_ptr<Generate> generator;
	std::map<int, int> panelIdToLocationId;
	std::map<int, int> locationIdToPanelId_READ_ONLY;
	int finalPanel;
	bool isCompleted = false;

	std::chrono::system_clock::time_point lastFrameTime;
	float halfSecondCountdown = 0.f;

	int halfSecondCounter = 0;

	std::shared_ptr<HudManager> hudManager;

	bool DeathLink = false;
	bool EPShuffle = false;
	int PuzzleRandomization = 0;

	bool FirstEverLocationCheckDone = false;

	bool hasPowerSurge = false;
	std::chrono::system_clock::time_point powerSurgeStartTime;

	std::set<double> deathLinkTimestamps;

	const float defaultRunSpeed = 2.f;
	const float boostedRunSpeed = 4.f;
	const float slowedRunSpeed = 1.f;

	float speedBoostTime = 0.f;
	float slownessTrapTime = 0.f;
	float solveModeSpeedBoostDecayFactor = 1.f;

	int foundPuzzleSkips = 0;
	int spentPuzzleSkips = 0;
	
	int numSpeedChargesPerBoost = 8;
	int numSpeedChargesPerSmallFillBase = 1;
	int numSpeedChargesPerSmallFillScaling = 0;
	int currentSpeedCharges = 0;
	int baseBoostCapacity = 1;
	int foundBoostCapacity = 0;
	float speedBoostDuration = 20.f;

	bool laserRequirementMet = false;

	std::set<int> disableCollisionList;

	std::set<int> severedDoorsList;
	std::map<int,int> collisionsToRefresh;
	std::map<int, std::vector<float>> collisionPositions;
	std::set<int> alreadyTriedUpdatingNormally;

	int storageCheckCounter = 6;

	bool infiniteChallenge = false;
	bool insideChallengeBoxRange = true;

	bool symmetryMessageDelivered = false;
	bool ppMessageDelivered = false;

	void HandleKeyTaps();

	void HandleInteractionState();
	
	void CheckEPSkips();

	void CheckDeathLink();
	void SendDeathLink(int panelId);

	void DisableCollisions();

	void AudioLogPlaying(float deltaSeconds);

	void RefreshDoorCollisions();

	void CheckSolvedPanels();
	void HandleMovementSpeed(float deltaSeconds);
	void HandlePowerSurge();

	void LookingAtObelisk();

	void PettingTheDog(float deltaSeconds);
	bool LookingAtTheDog() const;

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

	void CheckLasers();
	void CheckEPs();

	void CheckImportantCollisionCubes();

	void SetStatusMessages() const;

	int GetActivePanel();
	void WriteMovementSpeed(float currentSpeed);

	void UpdateInfiniteChallenge();

	std::map<std::string, int> laserIDsToLasers;
	std::list<std::string> laserIDs;
	std::map<int, bool> laserStates;

	std::map<std::string, int> EPIDsToEPs;
	std::list<std::string> EPIDs;
	std::map<int, bool> EPStates;

	int currentAudioLog = -1;
	float currentAudioLogDuration = 0.f;

	int activePanelId = -1;
	int mostRecentActivePanelId = -1;
	int mostRecentPanelState = -1;

	std::string puzzleSkipInfoMessage;
	float skipButtonHeldTime = 0.f; // Tracks how long the skip button has been held.

	// The cost to skip the currently-selected puzzle. -1 if the puzzle cannot be
	//   skipped for whatever reason.
	int puzzleSkipCost = -1;

	std::map<int, std::pair<std::string, int64_t>> audioLogMessages = {};
	std::map<int, std::set<int>> obeliskHexToEPHexes = {};
	std::map<int, int> obeliskHexToAmountOfEPs = {};
	std::map<int, std::string> entityToName = {};

	Vector3 lastMouseDirection;
	float dogPettingDuration = 0.f;
	float dogBarkDuration = 0.f;
	bool sentDog = false;
	bool letGoSinceInteractModeOpen = false;

	CollisionCube bonsaiCollisionCube = CollisionCube(18, -31.6f, 14, 21, -29, 17);
	CollisionCube riverVaultUpperCube = CollisionCube(52, -51, 19, 44, -47, 23);
	CollisionCube riverVaultLowerCube = CollisionCube(40, -56, 16, 46, -47, 20.5f);
	CollisionCube bunkerPuzzlesCube = CollisionCube(161.2f, -96.3f, 5.8f, 172.3f, -101.1f, 11.5f);
	CollisionCube tutorialPillarCube = CollisionCube(-152, -150.9f, 5, -148, -154.8f, 9);
	CollisionCube quarryLaserPanel = CollisionCube(-59, 90, 17, -67, 100, 21);
	CollisionCube symmetryUpperPanel = CollisionCube(-180, 31, 12.6f, -185, 37, 17);
	CollisionCube challengeTimer = CollisionCube(-27, -40, -20, -50, -20, -4);

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