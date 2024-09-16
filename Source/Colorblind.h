#pragma once
#include <vector>
#include <map>
#include <set>

class RgbColor;

class ColorblindMode {
public:
	static void makePanelsColorblindFriendly();
	static std::vector<int> getClasses(std::vector<int> decorations);
	static std::vector<int> sortClassesByCount(std::vector<int> classes);
	static void writeColors(int id, std::vector<int> classes, std::vector<int> decorations, std::map<int, RgbColor> newColorsByClass, std::set<int> classesContainingTriangles);
	static std::set<int> getClassesContainingTriangles(std::vector<int> decorations, std::vector<int> classes);
	static std::map<int, RgbColor> chooseNewColors(int id, std::vector<int> decorations, std::vector<int> classes, std::vector<int> classes_sorted_by_count, std::vector<RgbColor> usableColors);
	static void makePanelColorblindFriendly(int id);
};