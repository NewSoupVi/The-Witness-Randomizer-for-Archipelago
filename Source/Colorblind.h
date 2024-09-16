#pragma once
#include <vector>
#include <map>

class RgbColor;

class ColorblindMode {
public:
	static void makePanelsColorblindFriendly();
	static std::vector<int> getClasses(std::vector<int> decorations);
	static std::vector<int> sortClassesByCount(std::vector<int> classes);
	static void writeColors(int id, std::vector<int> classes, std::map<int, RgbColor> newColorsByClass);
	static void makePanelColorblindFriendly(int id);
};