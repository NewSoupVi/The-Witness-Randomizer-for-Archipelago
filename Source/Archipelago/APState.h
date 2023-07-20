#pragma once

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
		bool unlockedInvisibleDots = false;
		bool unlockedArrows = false;
		bool unlockedSymmetry = false;
		bool unlockedFullDots = false;
		int activeLasers = 0;
		int requiredChallengeLasers = 11;
		int requiredMountainLasers = 7;
};

// Puzzle Symbols
constexpr int ITEM_OFFSET = 158000;
constexpr int ITEM_DOTS = ITEM_OFFSET + 0;
constexpr int ITEM_COLORED_DOTS = ITEM_DOTS + 1;
constexpr int ITEM_FULL_DOTS = ITEM_DOTS + 2;
constexpr int ITEM_SOUND_DOTS = ITEM_DOTS + 5;
constexpr int ITEM_INVISIBLE_DOTS = ITEM_DOTS + 6;
constexpr int ITEM_SYMMETRY = ITEM_OFFSET + 10;
constexpr int ITEM_TRIANGLES = ITEM_OFFSET + 20;
constexpr int ITEM_ERASOR = ITEM_OFFSET + 30;
constexpr int ITEM_TETRIS = ITEM_OFFSET + 40;
constexpr int ITEM_TETRIS_ROTATED = ITEM_TETRIS + 1;
constexpr int ITEM_TETRIS_NEGATIVE = ITEM_TETRIS + 10;
constexpr int ITEM_STARS = ITEM_OFFSET + 60;
constexpr int ITEM_STARS_WITH_OTHER_SYMBOL = ITEM_STARS + 1;
constexpr int ITEM_SQUARES = ITEM_OFFSET + 70;
constexpr int ITEM_B_W_SQUARES = ITEM_SQUARES + 1;
constexpr int ITEM_COLORED_SQUARES = ITEM_SQUARES + 2;
constexpr int ITEM_ARROWS = ITEM_OFFSET + 80;
constexpr int LASER_CHECK = 90832567;

// Powerups
constexpr int ITEM_POWERUP_OFFSET = ITEM_OFFSET + 500;
constexpr int ITEM_SPEED_BOOST = ITEM_POWERUP_OFFSET + 0;
constexpr int ITEM_PARTIAL_SPEED_BOOST = ITEM_POWERUP_OFFSET + 1;
constexpr int ITEM_MAX_SPEED_BOOST = ITEM_POWERUP_OFFSET + 2;
constexpr int ITEM_PUZZLE_SKIP = ITEM_POWERUP_OFFSET + 10;
constexpr int ITEM_BOOST_CAPACITY = ITEM_POWERUP_OFFSET + 11;

// Traps
constexpr int ITEM_TRAP_OFFSET = ITEM_OFFSET + 600;
constexpr int ITEM_TEMP_SPEED_REDUCTION = ITEM_TRAP_OFFSET + 0;
constexpr int ITEM_POWER_SURGE = ITEM_TRAP_OFFSET + 10;
