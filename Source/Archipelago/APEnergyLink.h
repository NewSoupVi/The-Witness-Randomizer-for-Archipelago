#include "../Generate.h">
#include <memory>
#include "APState.h"
#include "PuzzleData.h"
#include <stdexcept>

class GenerationData {
public:
	GenerationData(std::string inputString) {
		parseInputString(inputString);
	}

	std::vector<std::pair<int, int>> symbols;

	int gridX = 4;
	int gridY = 4;

	std::vector<int> allowedSymmetryTypes = { };

	std::vector<APEnergyLink::Shape> requiredSymbols;
private:
	std::map<std::string, int> decorationsMap = {
		{"Dot", Decoration::Dot},
		{"IntersectionDot", Decoration::Dot_Intersection},
		{"ColumnDot", Decoration::Dot_Column},
		{"RowDot", Decoration::Dot_Row},
		{"Star", Decoration::Star},
		{"Square", Decoration::Stone},
		{"Eraser", Decoration::Eraser},
		{"Triangle", Decoration::Triangle},
		{"Triangle1", Decoration::Triangle1},
		{"Triangle2", Decoration::Triangle2},
		{"Triangle3", Decoration::Triangle3},
		{"Triangle4", Decoration::Triangle4},
		{"Arrow", Decoration::Arrow},
		{"Arrow1", Decoration::Arrow1},
		{"Arrow2", Decoration::Arrow2},
		{"Arrow3", Decoration::Arrow3},
		{"Rotated", Decoration::Can_Rotate},
		{"Negative", Decoration::Negative},
		{"Gap", Decoration::Gap},
		{"RowGap", Decoration::Gap_Row},
		{"ColumnGap", Decoration::Gap_Column},
		{"Start", Decoration::Start},
		{"Exit", Decoration::Exit},
		{"Empty", Decoration::Empty},
		{"Shaper", Decoration::Poly},
	};

	std::map<std::string, int> colorsMap = {
		{"Black", Decoration::Color::Black},
		{"White", Decoration::Color::White},
		{"Red", Decoration::Color::Red},
		{"Purple", Decoration::Color::Purple},
		{"Green", Decoration::Color::Green},
		{"Cyan", Decoration::Color::Cyan},
		{"Magenta", Decoration::Color::Magenta},
		{"Yellow", Decoration::Color::Yellow},
		{"Blue", Decoration::Color::Blue},
		{"Orange", Decoration::Color::Orange},
	};

	int getDecorationFlag(std::string token);
	void handleToken(std::string token);
	void parseInputString(std::string inputString);
};

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
		_memory->WriteArray<int>(0x0042D, 0x410, std::vector<int>(500, 0));
		basePuzzle->Read(_memory);
		basePuzzle->id = 0x0042D;

		parseGenerationDatas();
	}

	std::shared_ptr<Generate> generator;

	void parseGenerationDatas();
	std::vector<GenerationData> parseGenerationData(std::string data);

	void generateNewPuzzle(int id);
	void generateRandomPuzzle(int id);
	GenerationData chooseSymbolCombination(int minDifficulty, int maxDifficulty);

	int getPowerOutput();

	int countSymbols();

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

	std::map<int, std::vector<GenerationData>> choosablePuzzles;
};