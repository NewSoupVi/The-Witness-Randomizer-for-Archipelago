#include "../Generate.h">
#include <memory>
#include "APState.h"
#include "PuzzleData.h"

class APEnergyLink {
private:
	std::shared_ptr<Memory> _memory;
	void setSymmetry(std::shared_ptr<Generate> gen, bool rotationalAllowed, int random, bool weirdStarting);
	void setSymmetry(std::shared_ptr<Generate> gen, bool rotationalAllowed, int random) {
		setSymmetry(gen, rotationalAllowed, random, false);
	}
public:
	APEnergyLink(APState* s) {
		state = s;
		generator = std::make_shared<Generate>();
		_memory = std::make_shared<Memory>("witness64_d3d11.exe");
		basePuzzle = new PuzzleData(0x2899C);
		_memory->WriteArray<int>(0x2899C, 0x410, std::vector<int>(500, 0));
		basePuzzle->Read(_memory);
	}

	std::shared_ptr<Generate> generator;

	void generateNewPuzzle(int id);
	void generateRandomPuzzle(int id);
	int chooseSymbolCombination();

	PuzzleData* basePuzzle;

	std::mt19937 random = std::mt19937{ std::random_device{}() };

	APState* state;

	enum Shape : int {
		Arrows = 0x1,
		BWSquare = 0x2,
		ColoredSquare = 0x4,
		Dots = 0x8,
		FullDots = 0x10,
		ColoredDots = 0x20,
		Symmetry = 0x40,
		Triangles = 0x80,
		Gaps = 0x100,
		Stars = 0x200,
		StarSameColor = 0x400,
		Shapers = 0x800,
		RotatedShapers = 0x1000,
		NegativeShapers = 0x2000,
		Eraser = 0x4000,
		WeirdStarting = 0x8000,
	};
};