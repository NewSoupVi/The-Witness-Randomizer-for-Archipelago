#include "APEnergyLink.h"

void APEnergyLink::generateNewPuzzle(int id)
{
	bool successfullyGenerated = false;
	while (!successfullyGenerated) {
		try {
			generateRandomPuzzle(id);
			successfullyGenerated = true;
		}
		catch (std::exception e) {
			OutputDebugStringW(L"Problem generating a new puzzle\n");
		}
	}
}

void APEnergyLink::setSymmetry(std::shared_ptr<Generate> gen, bool rotationalAllowed, int random, bool weirdStarting) {
	int realRand = random % (rotationalAllowed ? 3 : 2);
	
	generator->setGridSize(5, 5);
	generator->setFlag(Generate::Config::StartEdgeOnly);

	if (realRand == 0) {
		generator->setSymmetry(Panel::Symmetry::Vertical);
		if (!weirdStarting) {
			generator->setSymbol(Decoration::Start, 0, 10);  generator->setSymbol(Decoration::Start, 10, 10);
			generator->setSymbol(Decoration::Exit, 0, 0);  generator->setSymbol(Decoration::Exit, 10, 0);
		}
	}
	if (realRand == 1) {
		generator->setSymmetry(Panel::Symmetry::Horizontal);
		if (!weirdStarting) {
			generator->setSymbol(Decoration::Start, 0, 10);  generator->setSymbol(Decoration::Start, 0, 0);
			generator->setSymbol(Decoration::Exit, 10, 10);  generator->setSymbol(Decoration::Exit, 10, 0);
		}
	}
	if (realRand == 2) {
		generator->setSymmetry(Panel::Symmetry::Rotational);
		if (!weirdStarting) {
			generator->setSymbol(Decoration::Start, 0, 10);  generator->setSymbol(Decoration::Start, 10, 0);
			generator->setSymbol(Decoration::Exit, 0, 0);  generator->setSymbol(Decoration::Exit, 10, 10);
		}
	}
}

void APEnergyLink::generateRandomPuzzle(int id) {
	bool done = false;

	basePuzzle->Restore(_memory);

	done = true;
	done = false;

	while (!done) {
		generator->resetConfig();

		int combo = chooseSymbolCombination();

		done = true;

		generator->setGridSize(4, 4);
		generator->setSymmetry(Panel::Symmetry::None);

		std::wstringstream s;
		s << std::hex << combo;
		s << " - attempting to generate.\n";
		OutputDebugStringW(s.str().c_str());

		int extraRand = rand();

		switch (combo) {
		case (Gaps | WeirdStarting):
			generator->setGridSize(5, 5);
			generator->generate(id, Decoration::Gap, 24, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		case (Gaps | Dots):
			generator->generate(id, Decoration::Dot, 6, Decoration::Gap, 8);
			break;
		case (Gaps | Stars):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 4, Decoration::Star | Decoration::Color::Magenta, 4, Decoration::Gap, 8);
			break;
		case (Gaps | BWSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4, Decoration::Gap, 6);
			break;
		case (Gaps | ColoredSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Blue, 3, Decoration::Stone | Decoration::Color::Magenta, 3, Decoration::Stone | Decoration::Color::Green, 3, Decoration::Gap, 6);
			break;
		case (Gaps | Shapers):
			generator->generate(id, Decoration::Poly, 2, Decoration::Gap, 8);
			break;
		case (Gaps | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Gap, 5);
			break;
		case (Gaps | RotatedShapers):
			generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 2, Decoration::Gap, 6);
			break;
		case (Gaps | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 5, Decoration::Gap, 6);
			break;
		case (Gaps | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Gap, 13);
			break;
		case (WeirdStarting | Dots):
			generator->setGridSize(5, 5);
			generator->generate(id, Decoration::Dot, 12, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		case (WeirdStarting | Stars):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 4, Decoration::Star | Decoration::Color::Magenta, 4, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		case (WeirdStarting | BWSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		case (WeirdStarting | ColoredSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 3, Decoration::Stone | Decoration::Color::Green, 2, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		//case (WeirdStarting | Shapers): This one is really hard and unintuitive.
			//generator->generate(id, Decoration::Poly, 2, Decoration::Start, 1, Decoration::Exit, 1);
			//break;
		//case (WeirdStarting | RotatedShapers): This one is really hard and unintuitive.
			//generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 2, Decoration::Start, 1, Decoration::Exit, 1);
			//break;
		case (WeirdStarting | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 24, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		case (WeirdStarting | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 5, Decoration::Start, 1, Decoration::Exit, 1);
			break;
		//case (WeirdStarting | Symmetry): //Degenerate case
			//break;
		case (Dots | Stars):
			generator->generate(id, Decoration::Dot, 6, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 4);
			break;
		case (Dots | BWSquare):
			generator->generate(id, Decoration::Dot, 6, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4);
			break;
		case (Dots | ColoredSquare):
			generator->generate(id, Decoration::Dot, 6, Decoration::Stone | Decoration::Color::Blue, 3, Decoration::Stone | Decoration::Color::Magenta, 3, Decoration::Stone | Decoration::Color::Green, 3);
			break;
		case (Dots | Shapers):
			generator->generate(id, Decoration::Dot, 6, Decoration::Poly, 2);
			break;
		case (Dots | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 31);
			break;
		case (Dots | RotatedShapers):
			generator->generate(id, Decoration::Dot, 6, Decoration::Poly | Decoration::Can_Rotate, 2);
			break;
		case (Dots | Triangles):
			generator->generate(id, Decoration::Dot, 6, Decoration::Triangle | Decoration::Color::Yellow, 4);
			break;
		case (Dots | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Dot, 10);
			break;

		case (ColoredDots | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Dot, 4, Decoration::Dot | Decoration::Color::Cyan, 3, Decoration::Color::Yellow, 3);
			break;

		case (Stars | BWSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 2, Decoration::Stone | Decoration::Color::White, 2, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 2);
			break;
		case (Stars | ColoredSquare):
			generator->generate(id, Decoration::Star | Decoration::Color::Black, 2, Decoration::Star | Decoration::Color::White, 2, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 2, Decoration::Stone | Decoration::Color::Green, 2);
			break;
		case (Stars | Shapers):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 2, Decoration::Poly, 2);
			break;
		case (Stars | RotatedShapers):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 2, Decoration::Poly | Decoration::Can_Rotate, 2);
			break;
		case (Stars | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 4);
			break;
		case (Stars | Triangles):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 2, Decoration::Triangle | Decoration::Color::Yellow, 4);
			break;
		case (Stars | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Star | Decoration::Color::Magenta, 4, Decoration::Star | Decoration::Color::Blue, 2);
			break;


		case (StarSameColor | BWSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 1, Decoration::Stone | Decoration::Color::White, 1, Decoration::Star | Decoration::Color::Black, 2, Decoration::Star | Decoration::Color::White, 1);
			break;
		case (StarSameColor | ColoredSquare):
			generator->generate(id, Decoration::Star | Decoration::Color::Magenta, 1, Decoration::Star | Decoration::Color::Blue, 1, Decoration::Stone | Decoration::Color::Blue, 1, Decoration::Stone | Decoration::Color::Magenta, 1, Decoration::Stone | Decoration::Color::Green, 1);
			break;
		case (StarSameColor | Shapers):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 1, Decoration::Poly | Decoration::Color::Magenta, 1, Decoration::Poly | Decoration::Color::Blue, 1);
			break;
		case (StarSameColor | RotatedShapers):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 1, Decoration::Poly | Decoration::Color::Magenta | Decoration::Can_Rotate, 1, Decoration::Poly | Decoration::Color::Blue | Decoration::Can_Rotate, 1);
			break;
		case (StarSameColor | Triangles):
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 2, Decoration::Star | Decoration::Color::Magenta, 1, Decoration::Triangle | Decoration::Color::Magenta, 1, Decoration::Triangle | Decoration::Color::Blue, 1);
			break;
		case (BWSquare | ColoredSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 1, Decoration::Stone | Decoration::Color::White, 1, Decoration::Stone | Decoration::Color::Blue, 1, Decoration::Stone | Decoration::Color::Magenta, 1, Decoration::Stone | Decoration::Color::Green, 1);
			break;
		case (BWSquare | Shapers):
			generator->generate(id, Decoration::Poly, 2, Decoration::Stone | Decoration::Color::Black, 2, Decoration::Stone | Decoration::Color::White, 2);
			break;
		case (BWSquare | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4);
			break;
		case (BWSquare | RotatedShapers):
			generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 2, Decoration::Stone | Decoration::Color::Black, 2, Decoration::Stone | Decoration::Color::White, 2);
			break;
		case (BWSquare | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 5, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4);
			break;
		case (BWSquare | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 6, Decoration::Stone | Decoration::Color::White, 6);
			break;

		case (ColoredSquare | Shapers):
			generator->generate(id, Decoration::Poly, 2, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 2, Decoration::Stone | Decoration::Color::Green, 2);
			break;
		case (ColoredSquare | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 2, Decoration::Stone | Decoration::Color::Green, 2);
			break;
		case (ColoredSquare | RotatedShapers):
			generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 2, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 2, Decoration::Stone | Decoration::Color::Green, 2);
			break;
		case (ColoredSquare | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 5, Decoration::Stone | Decoration::Color::Blue, 2, Decoration::Stone | Decoration::Color::Magenta, 2, Decoration::Stone | Decoration::Color::Green, 2);
			break;
		case (ColoredSquare | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Stone | Decoration::Color::Magenta, 3, Decoration::Stone | Decoration::Color::Blue, 3, Decoration::Stone | Decoration::Color::Green, 3);
			break;

		case (Shapers | FullDots):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Poly, 2);
			break;
		case (Shapers | RotatedShapers):
			generator->generate(id, Decoration::Poly, 2, Decoration::Poly | Decoration::Can_Rotate, 1);
			break;
		case (Shapers | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 3, Decoration::Poly, 2);
			break;
		case (Shapers | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Poly, 3);
			break;

		case (FullDots | RotatedShapers):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Poly | Decoration::Can_Rotate, 2);
			break;
		case (FullDots | Triangles):
			generator->generate(id, Decoration::Dot_Intersection, 25, Decoration::Triangle | Decoration::Color::Yellow, 4);
			break;
		//case (FullDots | Symmetry): Degenerate Case

		case (RotatedShapers | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 3, Decoration::Poly | Decoration::Can_Rotate, 2);
			break;

		case (RotatedShapers | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 3);
			break;

		case (Triangles | Symmetry):
			setSymmetry(generator, true, extraRand);
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 6);
			break;

		case (Eraser | Dots):
			generator->generate(id, Decoration::Dot, 8, Decoration::Eraser, 1);
			break;
		case (Eraser | Stars): // This requires something extra, I think. Look at Quarry Boathouse
			generator->generate(id, Decoration::Star | Decoration::Color::Blue, 4, Decoration::Star | Decoration::Color::Magenta, 3, Decoration::Eraser | Decoration::Color::White, 1);
			break;
		case (Eraser | StarSameColor):
			if (extraRand % 2) {
				generator->generate(id, Decoration::Star | Decoration::Color::Blue, 5, Decoration::Star | Decoration::Color::Magenta, 6, Decoration::Eraser | Decoration::Color::Magenta, 1);
			}
			else
			{
				generator->generate(id, Decoration::Star | Decoration::Color::Blue, 6, Decoration::Star | Decoration::Color::Magenta, 5, Decoration::Eraser | Decoration::Color::Magenta, 1);
			}
			break;
		case (Eraser | BWSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Black, 4, Decoration::Stone | Decoration::Color::White, 4, Decoration::Eraser | Decoration::Color::Magenta, 1);
			break;
		case (Eraser | ColoredSquare):
			generator->generate(id, Decoration::Stone | Decoration::Color::Blue, 3, Decoration::Stone | Decoration::Color::Magenta, 3, Decoration::Stone | Decoration::Color::Green, 3, Decoration::Eraser | Decoration::Color::White, 1);
			break;
		case (Eraser | Shapers):
			generator->generate(id, Decoration::Poly, 3, Decoration::Eraser | Decoration::Color::White, 1);
			break;
		case (Eraser | RotatedShapers):
			generator->generate(id, Decoration::Poly | Decoration::Can_Rotate, 3, Decoration::Eraser | Decoration::Color::White, 1);
			break;
		case (Eraser | Triangles):
			generator->generate(id, Decoration::Triangle | Decoration::Color::Yellow, 6, Decoration::Eraser | Decoration::Color::White, 1);
			break;

		default:
			std::wstringstream s;
			s << std::hex << combo;
			s << " is not a supported symbol combination.\n";
			OutputDebugStringW(s.str().c_str());
			done = false;
		}
	}
	
}

int APEnergyLink::chooseSymbolCombination() {
	std::set<int> possibleSymbols = { Gaps, WeirdStarting };

	//if (state->unlockedArrows) possibleSymbols.emplace(Arrows); NYI
	if (state->unlockedDots) possibleSymbols.emplace(Dots);
	if (state->unlockedFullDots) possibleSymbols.emplace(FullDots);
	if (state->unlockedErasers) possibleSymbols.emplace(Eraser);
	if (state->unlockedStones) possibleSymbols.emplace(BWSquare);
	if (state->unlockedColoredStones) possibleSymbols.emplace(ColoredSquare);
	if (state->unlockedStars) possibleSymbols.emplace(Stars);
	if (state->unlockedStarsWithOtherSimbol) possibleSymbols.emplace(StarSameColor);
	if (state->unlockedTriangles) possibleSymbols.emplace(Triangles);
	if (state->unlockedSymmetry) possibleSymbols.emplace(Symmetry);
	if (state->unlockedTetris) possibleSymbols.emplace(Shapers);
	if (state->unlockedTetrisRotated) possibleSymbols.emplace(RotatedShapers);
	if (state->unlockedTetrisNegative) possibleSymbols.emplace(NegativeShapers);
	if (state->unlockedColoredDots) possibleSymbols.emplace(ColoredDots);

	int chosenCombination = 0;

	for (int i = 0; i < 2; i++) {
		std::vector<int> out;
		std::sample(possibleSymbols.begin(), possibleSymbols.end(), std::back_inserter(out),
			1, random);

		int result = out[0];
		int realResult = result;

		chosenCombination |= result;

		possibleSymbols.erase(result);
	}

	return chosenCombination;
}

int APEnergyLink::getPowerOutput() {
	int symbolCount = countSymbols();

	return 10000000 * (symbolCount + 1);
}

int APEnergyLink::countSymbols() {
	int unlockedSymbols = 0;

	if (state->unlockedDots) unlockedSymbols++;
	if (state->unlockedFullDots) unlockedSymbols++;
	if (state->unlockedErasers) unlockedSymbols++;
	if (state->unlockedStones) unlockedSymbols++;
	if (state->unlockedColoredStones) unlockedSymbols++;
	if (state->unlockedStars) unlockedSymbols++;
	if (state->unlockedStarsWithOtherSimbol) unlockedSymbols++;
	if (state->unlockedTriangles) unlockedSymbols++;
	if (state->unlockedSymmetry) unlockedSymbols++;
	if (state->unlockedTetris) unlockedSymbols++;
	if (state->unlockedTetrisRotated) unlockedSymbols++;
	if (state->unlockedTetrisNegative) unlockedSymbols++;
	if (state->unlockedColoredDots) unlockedSymbols++;
	if (state->unlockedSoundDots) unlockedSymbols++;
	if (state->unlockedArrows) unlockedSymbols++;

	return unlockedSymbols;
}

void GenerationData::parseInputString(std::string input) {
	std::string delimiter = ", ";

	size_t pos = 0;
	std::string token;
	while ((pos = input.find(delimiter)) != std::string::npos) {
		token = input.substr(0, pos);

		handleToken(token);

		input.erase(0, pos + delimiter.length());
	}
}

int GenerationData::getDecorationFlag(std::string token) {
	if (decorationsMap.count(token)) return decorationsMap[token];
	if (colorsMap.count(token)) return colorsMap[token];
	throw std::invalid_argument("Unknown symbol/color: " + token);
	return -1;
}

void GenerationData::handleToken(std::string token) {
	if (token == "Normal Symmetry") {
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelH);
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelV);
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::Rotational);
		return;
	}
	if (token == "Rotational Symmetry") {
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::Rotational);
		return;
	}
	if (token == "Vertical Parallel Symmetry") {
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelV);
		return;
	}
	if (token == "Horizontal Parallel Symmetry") {
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelH);
		return;
	}
	if (token == "Parallel Symmetry") {
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelH);
		this->allowedSymmetryTypes.push_back(Panel::Symmetry::ParallelV);
		return;
	}

	std::string delimiter = ", ";

	size_t pos = 0;
	std::string subtoken;

	int decoration = 0;
	int amount = -1;

	while ((pos = token.find(delimiter)) != std::string::npos) {
		subtoken = token.substr(0, pos);
		
		if (is_number(subtoken)) {
			amount = atoi(subtoken.c_str());
			continue;
		}

		decoration |= getDecorationFlag(subtoken);
	}

	if (amount == -1) {
		throw std::invalid_argument("Missing symbol amount: " + token);
	}

	if (decoration == 0) {
		throw std::invalid_argument("Unknown / Incomplete token: " + token);
	}

	symbols.push_back({ decoration, amount });
}

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}