#include "Globals.h"

Globals* Globals::_singleton = nullptr;

Globals::Globals() {
}

void Globals::create() {
	if (_singleton == nullptr) {
		_singleton = new Globals();
	}
}

Globals* Globals::get() {
	return _singleton;