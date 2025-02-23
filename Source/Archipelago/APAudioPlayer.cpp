#include "APAudioPlayer.h"

#include "windows.h"
#include "mmsystem.h"

#include "../../App/resource.h"
#include "CanonicalAudioFileNames.h"
#include "../Utilities.h"

#include "soloud.h"
#include "soloud_wav.h"

#include <iostream>
#include <fstream>

APAudioPlayer* APAudioPlayer::_singleton = nullptr;

APAudioPlayer::APAudioPlayer() : Watchdog(0.1f) {
	gSoloud = new SoLoud::Soloud();
	gSoloud->init();

	std::set<std::string> audioFiles = Utilities::get_all_files_with_extension(".\\Jingles\\", ".wav");
	
	for (auto [resource, possibilities] : canonicalAudioFileNames) {
		SoLoud::Wav* wav = NULL;

		for (std::string possibility : possibilities) {
			if (audioFiles.contains(possibility)) {
				wav = new SoLoud::Wav;
				wav->load(("./Jingles/" + possibility).c_str());
				break;
			}
		}

		if (wav == NULL) {
			auto hRes = FindResource(NULL, MAKEINTRESOURCE(resource), L"WAVE");

			if (!hRes) continue;

			auto hResLoad = LoadResource(NULL, hRes);
			auto size = SizeofResource(NULL, hRes);
			const unsigned char* buffer = reinterpret_cast<unsigned char*>(LockResource(hResLoad));
			wav = new SoLoud::Wav;
			wav->loadMem(buffer, size, false, false);
		}

		preloadedAudioFiles[resource] = wav;
	}
}

void APAudioPlayer::action() {
	if (QueuedAudio.size() && !gSoloud->isValidVoiceHandle(currentlyPlayingSyncHandle)) {
    	std::pair<APJingle, std::any> nextAudio = QueuedAudio.front();

		PlayAppropriateJingle(nextAudio.first, nextAudio.second, false);

		QueuedAudio.pop();
	}

	if (QueuedAsyncAudio.size()) {
		std::pair<APJingle, std::any> nextAudio = QueuedAsyncAudio.front();

		std::thread([this, nextAudio] { this->PlayAppropriateJingle(nextAudio.first, nextAudio.second, true); }).detach();

		QueuedAsyncAudio.pop();
	}

	if (QueueDirectAsyncAudio.size()) {
		int resource = QueueDirectAsyncAudio.front();

		std::thread([this, resource] { this->PlayJingleBlocking(resource); }).detach();

		QueueDirectAsyncAudio.pop();
	}
}

void APAudioPlayer::create() {
	if (_singleton == nullptr) {
		_singleton = new APAudioPlayer();
		_singleton->start();
	}
}

APAudioPlayer* APAudioPlayer::get() {
	create();
	return _singleton;
}

void APAudioPlayer::PlayAudio(APJingle jingle, APJingleBehavior queue, std::any extraInfo) {
	if (queue == APJingleBehavior::PlayImmediate) {
		QueuedAsyncAudio.push({ jingle, extraInfo });
	}

	if (queue == APJingleBehavior::Queue){
		QueuedAudio.push({ jingle, extraInfo });
	}

	if (queue == APJingleBehavior::DontQueue) {
		if (!gSoloud->isValidVoiceHandle(currentlyPlayingSyncHandle) && QueuedAudio.empty()) QueuedAudio.push({ jingle, extraInfo });
	}
}

void APAudioPlayer::PlayJingleMain(int resource) {
	if (!preloadedAudioFiles.contains(resource)) {
		return;
	}

	currentlyPlayingSyncSound = preloadedAudioFiles[resource];
	currentlyPlayingSyncHandle = gSoloud->play(*currentlyPlayingSyncSound);
}

void APAudioPlayer::PlayJingleBlocking(int resource) {
	if (!preloadedAudioFiles.contains(resource)) {
		return;
	}

	SoLoud::Wav* sound = preloadedAudioFiles[resource];
	int handle = gSoloud->play(*sound);

	while (gSoloud->isValidVoiceHandle(handle)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		continue;
	}
}

void APAudioPlayer::PlayAppropriateJingle(APJingle jingle, std::any extraFlag, bool async) {
	if (jingle == APJingle::Plop) {
		while (true) {
			std::uniform_int_distribution<int> uid(0, jingleVersions[jingle].size() - 1);
			int index = uid(rng); // use rng as a generator
			if (lastPlopIndex == index) continue;
			lastPlopIndex = index;
			break;
		}
		PlayJingle(jingleVersions[jingle][lastPlopIndex], async);
		return;
	}

	auto now = std::chrono::system_clock::now();

	if (jingle == APJingle::UnderstatedEntityHunt) {
		std::pair<int, int> solved_and_total = std::any_cast<std::pair<int, int>>(extraFlag);
		int solved = solved_and_total.first;
		int required = solved_and_total.second;
		if (solved == required) {
			// Special last jingle
			PlayJingle(finalPanelHuntJingleUnderstated, async);
			return;
		}
	}
	if (jingle == APJingle::EntityHunt) {
		std::pair<int, int> solved_and_total = std::any_cast<std::pair<int, int>>(extraFlag);
		int solved = solved_and_total.first;
		int required = solved_and_total.second;
		float percentage = (float)solved / (float)required;
		double bestIndex = percentage * (entityHuntJingles.size() - 1);
		if (bestIndex > entityHuntJingles.size() - 1) bestIndex = entityHuntJingles.size() - 1;

		std::normal_distribution d{ bestIndex, 1.5};
		std::uniform_int_distribution d2(1, 2);

		int index = -1;
		while (true) {
			if (solved == 1) {
				index = 0; // Always play the first jingle first
				break;
			}
			if (required > 12) {
				if (solved == required - 2) {
					// Make the third to last jingle not be the most epic one
					if (index == entityHuntJingles.size() - 1) continue;
					// Make the third to last jingle not be underwhelming
					if (index < entityHuntJingles.size() - 5) continue;
				}
			}
			if (solved == required - 1) {
				// Force the second to last jingle to be the most epic one
				index = entityHuntJingles.size() - 1;
				break;
			}
			if (solved == required) {
				// Special last jingle
				PlayJingle(finalPanelHuntJingle, async);
				return;
			}

			index = std::round(d(rng));
			if (index < 0) continue;
			if (index >= entityHuntJingles.size()) continue;
			if (index == lastEntityHuntIndex) continue;
			if (index == secondLastEntityHuntIndex) {
				if (d2(rng) == 1) continue; // Make the second last a bit less likely to prevent back and forth
			}
			break;
		}

		secondLastEntityHuntIndex = lastEntityHuntIndex;
		lastEntityHuntIndex = index;
		
		PlayJingle(entityHuntJingles[index], async);
		return;
	}

	if (understatedJingles.count(jingle)) {
		int resource = jingleVersions[jingle][0];

		PlayJingle(resource, async);
		return;
	}
	
	if ((now - lastPanelJinglePlayedTime) > std::chrono::seconds(60)) {
		panelChain = -1;
		lastPanelJinglePlayed = APJingle::None;
	}
	if ((now - lastEPJinglePlayedTime) > std::chrono::seconds(60)) {
		epChain = -1;
		lastEPJinglePlayed = APJingle::None;
	}

	int versionToPlay = 0;

	bool epicVersion = std::any_cast<bool>(extraFlag);

	if (panelJingles.count(jingle) || dogJingles.count(jingle)) {
		panelChain++;
		if (lastPanelJinglePlayed == jingle && !epicVersion) panelChain++; // If the same jingle just played, advance the counter another time so the same jingle doesn't play twice.
		
		lastPanelJinglePlayed = epicVersion ? APJingle::None : jingle;

		versionToPlay = panelChain / 2;
		ResetRampingCooldownPanel();
	}
	else if (epJingles.count(jingle)) {
		epChain++;
		if (lastEPJinglePlayed == jingle && !epicVersion) epChain++;

		lastEPJinglePlayed = epicVersion ? APJingle::None : jingle;

		versionToPlay = epChain / 2;
		ResetRampingCooldownEP();
	}

	if (epicVersion) {
		ResetRampingCooldownPanel(); // Epic doesn't interrupt a panel chain. Also, Epic should still advance the counter by one.

		int resource = jingleEpicVersions[jingle];
		PlayJingle(resource, async);
		return;
	}

	if (versionToPlay < 0) versionToPlay = 0;
	if (versionToPlay > jingleVersions[jingle].size() - 1) versionToPlay = jingleVersions[jingle].size() - 1;

	int resource = jingleVersions[jingle][versionToPlay];

	PlayJingle(resource, async);
}

void APAudioPlayer::PlayFinalRoomJingle(std::string type, APJingleBehavior queue, double finalRoomMusicTimer) {
	auto now = std::chrono::system_clock::now();

	std::string key = "E";
	if (finalRoomMusicTimer > 900000) key = "Ebm";
	if (finalRoomMusicTimer > 1800000) key = "E";
	if (finalRoomMusicTimer > 2800000) key = "B";
	if (finalRoomMusicTimer > 4200000) key = "E";

	int resource = pillarJingles.find(key)->second.find(type)->second;

	QueueDirectAsyncAudio.push(resource);
}

void APAudioPlayer::ResetRampingCooldownPanel()
{
	lastPanelJinglePlayedTime = std::chrono::system_clock::now();
}

void APAudioPlayer::ResetRampingCooldownEP()
{
	lastEPJinglePlayedTime = std::chrono::system_clock::now();
}

bool APAudioPlayer::HasCustomChallengeSong()
{
	return preloadedAudioFiles.contains(-2);
}

void APAudioPlayer::StartCustomChallengeSong()
{
	if (gSoloud->isValidVoiceHandle(challengeHandle)) return;
	SoLoud::Wav* sound = preloadedAudioFiles[-2];
	challengeHandle = gSoloud->play(*sound);
	challengeSongIsPlaying = true;
}

void APAudioPlayer::StopCustomChallengeSong()
{
	challengeSongIsPlaying = false;
	if (!gSoloud->isValidVoiceHandle(challengeHandle)) return;
	gSoloud->stop(challengeHandle);
}

bool APAudioPlayer::ChallengeSongHasEnded()
{
	return !gSoloud->isValidVoiceHandle(challengeHandle);
}
