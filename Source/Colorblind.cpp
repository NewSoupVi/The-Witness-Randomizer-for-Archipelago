#include "Colorblind.h"
#include "Memory.h"
#include "Panels.h"
#include "Archipelago/LockablePuzzle.h"
#include "DataTypes.h"
#include "Randomizer.h"

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

void ColorblindMode::writeColors(int id, std::vector<int> classes, std::map<int, RgbColor> newColorsByClass) {
	Memory* memory = Memory::get();
	std::vector<float> newColors = {};

	for (int class_number : classes) {
		newColors.push_back(newColorsByClass[class_number].R);
		newColors.push_back(newColorsByClass[class_number].G);
		newColors.push_back(newColorsByClass[class_number].B);
		newColors.push_back(newColorsByClass[class_number].A);
	}

	memory->WriteArray<float>(id, DECORATION_COLORS, newColors);
}

void ColorblindMode::makePanelColorblindFriendly(int id) {
	Memory* memory = Memory::get();

	memory->WritePanelData<float>(id, OUTER_BACKGROUND, { 0.2f, 0.2f, 0.2f, 1.0f });
	memory->WritePanelData<float>(id, PATH_COLOR, { 0.3f, 0.3f, 0.3f, 1.0f });
	memory->WritePanelData<float>(id, BACKGROUND_REGION_COLOR, { 0.4f, 0.4f, 0.4f, 1.0f });

	int num_decorations = memory->ReadPanelData<int>(id, NUM_DECORATIONS);
	std::vector<int> decorations = memory->ReadArray<int>(id, DECORATIONS, num_decorations);

	std::vector<int> classes = getClasses(decorations);
	std::vector<int> classes_sorted_by_count = sortClassesByCount(classes);

	std::map<int, RgbColor> newColorsByClass = { {-1, RgbColor(0.0f, 0.0f, 0.0f, 0.0f)}};
	std::queue<RgbColor, std::deque<RgbColor>> usableColors(std::deque<RgbColor>(color_order.begin(), color_order.end()));

	for (int class_number : classes_sorted_by_count) {
		RgbColor nextColor = usableColors.front();
		usableColors.pop();

		newColorsByClass[class_number] = nextColor;
	}

	writeColors(id, classes, newColorsByClass);
}