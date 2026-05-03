#pragma once

#include <string>
#include <vector>

class ApSettings;

const std::string& GetCreditsHint();
const std::vector<std::string>& GetJokeHints(const ApSettings& apSettings);
