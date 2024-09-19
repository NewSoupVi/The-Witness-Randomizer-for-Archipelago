#pragma once

class Globals {
private:

	Globals();

	static Globals* _singleton;

public:
	static void create();
	static Globals* get();

	bool independentColoredDots = false;
};
