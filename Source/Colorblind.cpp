#include "Colorblind.h"
#include "Memory.h"
#include "Panels.h"
#include "Archipelago/LockablePuzzle.h"
#include "Archipelago/APGameData.h"
#include "DataTypes.h"
#include "Randomizer.h"
#include "Panel.h"
#include "Special.h"

#include <algorithm>
#include <vector>
#include <map>
#include <queue>

std::vector<RgbColor> color_order = {
	RgbColor(0.0f, 0.0f, 1.0f),
	RgbColor(1.0f, 1.0f, 0.0f),
	RgbColor(0.0f, 0.0f, 0.0f),
	RgbColor(1.0f, 0.0f, 1.0f),
	RgbColor(1.0f, 1.0f, 1.0f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
	RgbColor(0.65f, 0.65f, 0.65f),
};

void ColorblindMode::makePanelsColorblindFriendly() {
	for (int panel : LockablePuzzles) {
		if (allPanels.contains(panel)) {
			ColorblindMode::makePanelColorblindFriendly(panel);
		}
	}
}

std::vector<int> ColorblindMode::getClasses(std::vector<int> decorations) {
	std::vector<int> classes = {};
	
	for (int decoration : decorations) {
		if (decoration == 0 || (decoration & 0xA00) == 0xA00) {
			classes.push_back(-1);
			continue;
		}

		classes.push_back(decoration & 0xF);
	}

	return classes;
}

std::vector<int> ColorblindMode::sortClassesByCount(std::vector<int> classes) {
	std::map<int, int> class_counts = {};
	for (int class_number : classes) {
		if (class_number == -1) continue;

		if (!class_counts.contains(class_number)) {
			class_counts[class_number] = 0;
		}
		class_counts[class_number] += 1;
	}

	std::vector<std::pair<int, int>> class_counts_vector{ class_counts.begin(), class_counts.end() };
	if (class_counts_vector.size()) {
		sort(class_counts_vector.begin(), class_counts_vector.end(), [](std::pair<int, int>& l, std::pair<int, int>& r)
			{
				return l.second > r.second;
			});
	}

	std::vector<int> ret = {};
	for (auto pair : class_counts_vector) {
		ret.push_back(pair.first);
	}

	return ret;
}

void ColorblindMode::writeColors(int id, std::vector<int> decorations, std::vector<int> classes, std::map<int, RgbColor> newColorsByClass, std::set<int> classesContainingTriangles) {
	Memory* memory = Memory::get();
	std::vector<float> newColors = {};
	std::map<int, int> pushableClasses = { 
		{1, SYMBOL_A},
		{2, SYMBOL_B},
		{4, SYMBOL_C},
		{5, SYMBOL_D},
		{6, SYMBOL_E},
	};

	bool needs_to_write_decorations = false;

	for (int triangle_class_number : classesContainingTriangles) {
		if (pushableClasses.contains(triangle_class_number)) continue;

		int dodge_class = -1;
		for (auto [good_class, _] : pushableClasses) {
			if (newColorsByClass.contains(good_class)) continue;
			dodge_class = good_class;
			break;
		}
		if (dodge_class == -1) {
			throw std::exception("Couldn't find a suitable class for triangles.");
		}

		newColorsByClass[dodge_class] = newColorsByClass[triangle_class_number];
		newColorsByClass.erase(triangle_class_number);

		needs_to_write_decorations = true;

		std::vector<int> newDecorations = {};
		for (int i = 0; i < decorations.size(); i++) {
			int decoration = decorations[i];
			int class_number = classes[i];

			if (class_number == triangle_class_number) {
				newDecorations.push_back((decoration & ~0xF) + dodge_class);
			}
			else {
				newDecorations.push_back(decoration);
			}
		}
		decorations = newDecorations;
	}

	for (int class_number : classes) {
		newColors.push_back(newColorsByClass[class_number].R);
		newColors.push_back(newColorsByClass[class_number].G);
		newColors.push_back(newColorsByClass[class_number].B);
		newColors.push_back(newColorsByClass[class_number].A);
	}

	memory->WriteArray<float>(id, DECORATION_COLORS, newColors);

	bool has_written_push_colors = false;
	for (auto [class_number, panel_offset] : pushableClasses) {
		if (!newColorsByClass.contains(class_number)) continue;

		if (!has_written_push_colors) {
			memory->WritePanelData(id, PUSH_SYMBOL_COLORS, 1);
			has_written_push_colors = true;
		}
		memory->WritePanelData(id, panel_offset, newColorsByClass[class_number]);
	}

	if (needs_to_write_decorations) {
		memory->WriteArray<int>(id, DECORATIONS, decorations);
	}
}

std::set<int> ColorblindMode::getClassesContainingTriangles(std::vector<int> decorations, std::vector<int> classes) {
	std::set<int> ret = {};

	for (int i = 0; i < decorations.size(); i++) {
		int decoration = decorations[i];
		int class_number = classes[i];

		if (!ret.contains(class_number) && (decoration & 0x700) == Decoration::Shape::Triangle) {
			ret.insert(class_number);
		}
	}

	return ret;
}

std::map<int, RgbColor> ColorblindMode::chooseNewColors(int id, std::vector<int> decorations, std::vector<int> classes, std::vector<int> classes_sorted_by_count, std::vector<RgbColor> usableColors) {
	Memory* memory = Memory::get();

	std::map<int, RgbColor> newColorsByClass = { {-1, RgbColor(0.0f, 0.0f, 0.0f, 0.0f)} };
	
	__int64 decorationsColorsPointer = memory->ReadPanelData<__int64>(id, DECORATION_COLORS);
	std::vector<float> decorationColors = {};
	if (decorationsColorsPointer) {
		decorationColors = memory->ReadArray<float>(id, DECORATION_COLORS, 4 * decorations.size());
	}

	int black_square_class = -1;
	int white_square_class = -1;
	std::set<int> colored_square_classes = {};
	int triangle_class = -1;
	int shapers_class = -1;
	int negative_shaper_class = -1;

	for (int i = 0; i < decorations.size(); i++) {
		int decoration = decorations[i];
		int class_number = classes[i];
		if (class_number == -1) continue;
		if ((decoration & 0x700) == Decoration::Shape::Stone) {
			int white_black_or_colored = Special::SquareIsWhiteBlackOrColored(id, decorations, decorationColors, i);

			if (white_black_or_colored == 0) white_square_class = class_number;
			else if (white_black_or_colored == 1) black_square_class = class_number;
			else if (!colored_square_classes.contains(class_number)) colored_square_classes.insert(class_number);
		}
		else if ((decoration & 0x700) == Decoration::Shape::Poly) {
			if ((decorations[i] & 0x2000) == Decoration::Negative) {
				negative_shaper_class = class_number;
			}
			else {
				shapers_class = class_number;
			}
		}
		else if ((decorations[i] & 0x700) == Decoration::Shape::Triangle) {
			triangle_class = class_number;
		}
	}

	std::vector<RgbColor> usedColors = {};

	// Prioritize Black/White Squares above all else
	for (int class_number : classes_sorted_by_count) {
		if (class_number == white_square_class) {
			RgbColor white = RgbColor(1.0f, 1.0f, 1.0f, 1.0f);
			newColorsByClass[class_number] = white;
			usedColors.push_back(white);
		}
		else if (class_number == black_square_class) {
			RgbColor black = RgbColor(0.0f, 0.0f, 0.0f, 1.0f);
			newColorsByClass[class_number] = black;
			usedColors.push_back(black);
		}
	}

	// Then do negative shapers
	for (int class_number : classes_sorted_by_count) {
		if (newColorsByClass.contains(class_number)) continue;

		if (class_number == negative_shaper_class) {
			RgbColor idealColor = RgbColor(0.0f, 0.0f, 1.0f);
			RgbColor closestColor = usableColors[0];
			float bestDistance = 1897598127;
			for (RgbColor possibleColor : usableColors) {
				if (std::find(usedColors.begin(), usedColors.end(), possibleColor) != usedColors.end()) continue;
				if (possibleColor.R == possibleColor.G && possibleColor.G == possibleColor.B && colored_square_classes.contains(class_number)) continue;

				float newDistance = (Vector3(idealColor.R, idealColor.B, idealColor.G) - Vector3(possibleColor.R, possibleColor.B, possibleColor.G)).length();
				if (newDistance < bestDistance) {
					bestDistance = newDistance;
					closestColor = possibleColor;
				}
			}

			newColorsByClass[class_number] = closestColor;
			usedColors.push_back(closestColor);
		}
	}

	// Then do Triangles and Shapers
	for (int class_number : classes_sorted_by_count) {
		if (newColorsByClass.contains(class_number)) continue;

		if (class_number == triangle_class) {
			RgbColor idealColor = RgbColor(1.0f, 0.84f, 0.0f);
			RgbColor closestColor = usableColors[0];
			float bestDistance = 1897598127;
			for (RgbColor possibleColor : usableColors) {
				if (std::find(usedColors.begin(), usedColors.end(), possibleColor) != usedColors.end()) continue;
				if (possibleColor.R == possibleColor.G && possibleColor.G == possibleColor.B && colored_square_classes.contains(class_number)) continue;

				float newDistance = (Vector3(idealColor.R, idealColor.B, idealColor.G) - Vector3(possibleColor.R, possibleColor.B, possibleColor.G)).length();
				if (newDistance < bestDistance) {
					bestDistance = newDistance;
					closestColor = possibleColor;
				}
			}

			newColorsByClass[class_number] = closestColor;
			usedColors.push_back(closestColor);
		}

		if (class_number == shapers_class) {
			RgbColor idealColor = RgbColor(1.0f, 0.65f, 0.0f);
			RgbColor closestColor = usableColors[0];
			float bestDistance = 1897598127;
			for (RgbColor possibleColor : usableColors) {
				if (std::find(usedColors.begin(), usedColors.end(), possibleColor) != usedColors.end()) continue;
				if (possibleColor.R == possibleColor.G && possibleColor.G == possibleColor.B && colored_square_classes.contains(class_number)) continue;

				float newDistance = (Vector3(idealColor.R, idealColor.B, idealColor.G) - Vector3(possibleColor.R, possibleColor.B, possibleColor.G)).length();
				if (newDistance < bestDistance) {
					bestDistance = newDistance;
					closestColor = possibleColor;
				}
			}

			newColorsByClass[class_number] = closestColor;
			usedColors.push_back(closestColor);
		}
	}

	// Then everything else
	for (int class_number : classes_sorted_by_count) {
		if (newColorsByClass.contains(class_number)) continue;
		for (RgbColor possibleColor : usableColors) {
			if (std::find(usedColors.begin(), usedColors.end(), possibleColor) != usedColors.end()) continue;
			if (possibleColor.R == possibleColor.G && possibleColor.G == possibleColor.B && colored_square_classes.contains(class_number)) continue;

			newColorsByClass[class_number] = possibleColor;
			usedColors.push_back(possibleColor);

			break;
		}
	}

	return newColorsByClass;
}

void ColorblindMode::makePanelColorblindFriendly(int id) {
	Memory* memory = Memory::get();

	int num_decorations = memory->ReadPanelData<int>(id, NUM_DECORATIONS);
	std::vector<int> decorations = memory->ReadArray<int>(id, DECORATIONS, num_decorations);
	std::vector<int> classes = getClasses(decorations);
	std::vector<int> classes_sorted_by_count = sortClassesByCount(classes);

	if (classes_sorted_by_count.size() <= 1 && !notThatBadSwampPanels.contains(id) && !swampLowContrastPanels.contains(id)) return;

	memory->WritePanelData<float>(id, OUTER_BACKGROUND, { 0.29f, 0.29f, 0.29f, 1.0f });
	memory->WritePanelData<float>(id, PATH_COLOR, { 0.17f, 0.17f, 0.17f, 1.0f });
	memory->WritePanelData<float>(id, BACKGROUND_REGION_COLOR, { 0.33f, 0.33f, 0.33f, 1.0f });

	std::set<int> classesContainingTriangles = getClassesContainingTriangles(decorations, classes);

	std::vector<RgbColor> usableColors = color_order;

	std::map<int, RgbColor> newColorsByClass = chooseNewColors(id, decorations, classes, classes_sorted_by_count, usableColors);

	writeColors(id, decorations, classes, newColorsByClass, classesContainingTriangles);
}