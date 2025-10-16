#pragma once

#include <set>
#include <string>

struct inGameHint {
	std::string message;
	int64_t locationID;
	int32_t playerNo;
	std::string areaHint;
	int32_t areaProgression;
	int32_t areaHuntEntities;
	bool isLaserHint;
	bool allowScout;
};

inline bool operator<(const inGameHint& lhs, const inGameHint& rhs) {
	return lhs.message < rhs.message;
}

class APState
{
public:
	bool unlockedStones = false;
	bool unlockedColoredStones = false;
	bool unlockedStars = false;
	bool unlockedStarsWithOtherSimbol = false;
	bool unlockedTetris = false;
	bool unlockedTetrisRotated = false;
	bool unlockedTetrisNegative = false;
	bool unlockedErasers = false;
	bool unlockedTriangles = false;
	bool unlockedDots = false;
	bool unlockedColoredDots = false;
	bool unlockedSoundDots = false;
	bool unlockedArrows = false;
	bool unlockedSymmetry = false;
	bool unlockedFullDots = false;

	int activeLasers = 0;
	int solvedHuntEntities = 0;

	int requiredChallengeLasers = 11;
	int requiredMountainLasers = 7;
	int requiredHuntEntities = 25;
	int totalHuntEntities = 40;

	std::set<int> keysReceived = {};
	std::set<int> keysInTheGame = {};
};

//Puzzle Symbols
#define ITEM_OFFSET 158000
const int ITEM_DOTS = ITEM_OFFSET + 0;
const int ITEM_COLORED_DOTS = ITEM_DOTS + 1;
const int ITEM_FULL_DOTS = ITEM_DOTS + 2;
const int ITEM_SOUND_DOTS = ITEM_DOTS + 5;
const int ITEM_SPARSE_DOTS = ITEM_DOTS + 7;
const int ITEM_SYMMETRY = ITEM_OFFSET + 10;
const int ITEM_TRIANGLES = ITEM_OFFSET + 20;
const int ITEM_ERASOR = ITEM_OFFSET + 30;
const int ITEM_TETRIS = ITEM_OFFSET + 40;
const int ITEM_TETRIS_ROTATED = ITEM_TETRIS + 1;
const int ITEM_TETRIS_NEGATIVE = ITEM_TETRIS + 10;
const int ITEM_STARS = ITEM_OFFSET + 60;
const int ITEM_STARS_WITH_OTHER_SYMBOL = ITEM_STARS + 1;
const int ITEM_SIMPLE_STARS = ITEM_STARS + 7;
const int ITEM_SQUARES = ITEM_OFFSET + 70;
const int ITEM_B_W_SQUARES = ITEM_SQUARES + 1;
const int ITEM_COLORED_SQUARES = ITEM_SQUARES + 2;
const int ITEM_ARROWS = ITEM_OFFSET + 80;
const int LASER_CHECK = 90832567;
const int ENTITY_HUNT_CHECK = 90832954;

//Powerups
#define ITEM_POWERUP_OFFSET ITEM_OFFSET + 500
const int ITEM_TEMP_SPEED_BOOST = ITEM_POWERUP_OFFSET + 0;
const int ITEM_PUZZLE_SKIP = ITEM_POWERUP_OFFSET + 10;

//Traps
#define ITEM_TRAP_OFFSET ITEM_OFFSET + 600
const int ITEM_TEMP_SPEED_REDUCTION = ITEM_TRAP_OFFSET + 0;
const int ITEM_POWER_SURGE = ITEM_TRAP_OFFSET + 10;
const int ITEM_BONK_TRAP = ITEM_TRAP_OFFSET + 15;
