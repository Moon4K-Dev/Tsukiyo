#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace Tsukiyo {

constexpr int VSLICE_PREVIEW_END = 15000;
constexpr int VSLICE_SECTION_LENGTH = 16;
const std::string VSLICE_DEFAULT_NOTE_SKIN = "funkin";
const std::string VSLICE_FOCUS_EVENT = "FocusCamera";
const std::string VSLICE_CHART_VERSION = "2.0.0";
const std::string VSLICE_META_VERSION = "2.2.4";

enum class FNFVSliceNoteType {
    DEFAULT,
    MOM
};

struct FNFVSliceNote {
    float time;
    int direction;
    float length;
    std::string kind;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["t"] = time;
        j["d"] = direction;
        j["l"] = length;
        j["k"] = kind;
        return j;
    }

    static FNFVSliceNote fromJson(const nlohmann::json& j) {
        if (!j.contains("t") || !j.contains("d")) {
            throw std::runtime_error("Missing required note fields");
        }
        return {
            j["t"].get<float>(),
            j["d"].get<int>(),
            j.value("l", 0.0f),
            j.value("k", "")
        };
    }
};

struct FNFVSliceEvent {
    float time;
    std::string event;
    nlohmann::json value;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["t"] = time;
        j["e"] = event;
        j["v"] = value;
        return j;
    }

    static FNFVSliceEvent fromJson(const nlohmann::json& j) {
        if (!j.contains("t") || !j.contains("e")) {
            throw std::runtime_error("Missing required event fields");
        }
        return {
            j["t"].get<float>(),
            j["e"].get<std::string>(),
            j.value("v", nlohmann::json::object())
        };
    }
};

struct FNFVSliceTimeChange {
    float time;
    float bpm;
    int stepsPerBeat;
    int beatsPerMeasure;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["t"] = time;
        j["bpm"] = bpm;
        j["n"] = stepsPerBeat;
        j["d"] = beatsPerMeasure;
        return j;
    }

    static FNFVSliceTimeChange fromJson(const nlohmann::json& j) {
        if (!j.contains("bpm")) {
            throw std::runtime_error("Missing required BPM field");
        }
        return {
            j.value("t", -1.0f),
            j["bpm"].get<float>(),
            j.value("n", 4),
            j.value("d", 4)
        };
    }
};

struct FNFVSliceCharacters {
    std::string player;
    std::string opponent;
    std::string girlfriend;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["player"] = player;
        j["opponent"] = opponent;
        j["girlfriend"] = girlfriend;
        return j;
    }

    static FNFVSliceCharacters fromJson(const nlohmann::json& j) {
        return {
            j.value("player", ""),
            j.value("opponent", ""),
            j.value("girlfriend", "")
        };
    }
};

struct FNFVSlicePlayData {
    std::string album;
    int previewStart;
    int previewEnd;
    std::map<std::string, int> ratings;
    FNFVSliceCharacters characters;
    std::vector<std::string> difficulties;
    std::vector<std::string> songVariations;
    std::string noteStyle;
    std::string stage;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["album"] = album;
        j["previewStart"] = previewStart;
        j["previewEnd"] = previewEnd;
        j["ratings"] = ratings;
        j["characters"] = characters.toJson();
        j["difficulties"] = difficulties;
        j["songVariations"] = songVariations;
        j["noteStyle"] = noteStyle;
        j["stage"] = stage;
        return j;
    }

    static FNFVSlicePlayData fromJson(const nlohmann::json& j) {
        FNFVSlicePlayData data;
        data.album = j.value("album", "");
        data.previewStart = j.value("previewStart", 0);
        data.previewEnd = j.value("previewEnd", VSLICE_PREVIEW_END);
        data.ratings = j.value("ratings", std::map<std::string, int>{});
        data.characters = FNFVSliceCharacters::fromJson(j.value("characters", nlohmann::json::object()));
        data.difficulties = j.value("difficulties", std::vector<std::string>{});
        data.songVariations = j.value("songVariations", std::vector<std::string>{});
        data.noteStyle = j.value("noteStyle", VSLICE_DEFAULT_NOTE_SKIN);
        data.stage = j.value("stage", "mainStage");
        return data;
    }
};

struct FNFVSliceOffsets {
    float instrumental;
    std::map<std::string, float> vocals;
    std::map<std::string, float> altInstrumentals;
    std::map<std::string, std::map<std::string, float>> altVocals;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["instrumental"] = instrumental;
        j["vocals"] = vocals;
        j["altInstrumentals"] = altInstrumentals;
        j["altVocals"] = altVocals;
        return j;
    }

    static FNFVSliceOffsets fromJson(const nlohmann::json& j) {
        return {
            j.value("instrumental", 0.0f),
            j.value("vocals", std::map<std::string, float>{}),
            j.value("altInstrumentals", std::map<std::string, float>{}),
            j.value("altVocals", std::map<std::string, std::map<std::string, float>>{})
        };
    }
};

class FNFVSliceChart : public Chart {
public:
    FNFVSliceChart() {
        format = Format::FNFVSlice;
        keyCount = 4;
    }

    ~FNFVSliceChart() override = default;

    const std::vector<std::string>& getAvailableDifficulties() const {
        return playData.difficulties;
    }

    bool loadFromFile(const std::string& path) override {
        try {
            std::filesystem::path chartPath(path);
            std::string basePath = chartPath.parent_path().string();
            std::string fileName = chartPath.stem().string();
            
            const std::string chartSuffix = "-chart";
            if (fileName.length() > chartSuffix.length() && 
                fileName.substr(fileName.length() - chartSuffix.length()) == chartSuffix) {
                fileName = fileName.substr(0, fileName.length() - chartSuffix.length());
            }
            
            std::ifstream chartFile(path);
            if (!chartFile.is_open()) return false;
            
            nlohmann::json chartJson = nlohmann::json::parse(chartFile);
            
            std::string metaPath = basePath + "/" + fileName + "-metadata.json";
            std::ifstream metaFile(metaPath);
            if (!metaFile.is_open()) return false;
            
            nlohmann::json metaJson = nlohmann::json::parse(metaFile);
            
            return loadFromJson(chartJson, metaJson);
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading file: " << e.what() << "\n";
            return false;
        }
    }

    bool saveToFile(const std::string& path) const override {
        try {
            std::filesystem::path chartPath(path);
            std::string basePath = chartPath.parent_path().string();
            std::string fileName = chartPath.stem().string();

            nlohmann::json chartJson;
            chartJson["version"] = VSLICE_CHART_VERSION;
            chartJson["generatedBy"] = "Tsukiyo";
            chartJson["scrollSpeed"] = scrollSpeeds;
            
            nlohmann::json notesJson;
            for (const auto& [diff, notes] : chartNotes) {
                nlohmann::json diffNotes = nlohmann::json::array();
                for (const auto& note : notes) {
                    diffNotes.push_back(note.toJson());
                }
                notesJson[diff] = diffNotes;
            }
            chartJson["notes"] = notesJson;

            nlohmann::json eventsJson = nlohmann::json::array();
            for (const auto& event : events) {
                eventsJson.push_back(event.toJson());
            }
            chartJson["events"] = eventsJson;

            std::ofstream chartFile(path);
            if (!chartFile.is_open()) return false;
            chartFile << chartJson.dump(4);

            nlohmann::json metaJson;
            metaJson["timeFormat"] = "ms";
            metaJson["artist"] = artist;
            metaJson["charter"] = charter;
            metaJson["generatedBy"] = "Tsukiyo";
            metaJson["version"] = VSLICE_META_VERSION;
            metaJson["looped"] = false;
            metaJson["playData"] = playData.toJson();
            metaJson["songName"] = title;
            metaJson["offsets"] = offsets.toJson();

            nlohmann::json timeChangesJson = nlohmann::json::array();
            for (const auto& change : timeChanges) {
                timeChangesJson.push_back(change.toJson());
            }
            metaJson["timeChanges"] = timeChangesJson;

            std::string metaPath = basePath + "/" + fileName + "-metadata.json";
            std::ofstream metaFile(metaPath);
            if (!metaFile.is_open()) return false;
            metaFile << metaJson.dump(4);

            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

private:
    std::map<std::string, float> scrollSpeeds;
    std::map<std::string, std::vector<FNFVSliceNote>> chartNotes;
    std::vector<FNFVSliceEvent> events;
    std::vector<FNFVSliceTimeChange> timeChanges;
    FNFVSlicePlayData playData;
    FNFVSliceOffsets offsets;

    void createSectionsFromNotes(const std::vector<FNFVSliceNote>& notes, float baseBpm) {
        if (notes.empty()) return;

        std::vector<FNFVSliceNote> sortedNotes = notes;
        std::sort(sortedNotes.begin(), sortedNotes.end(), 
                 [](const FNFVSliceNote& a, const FNFVSliceNote& b) { return a.time < b.time; });

        float beatsPerMeasure = 4.0f;
        float msPerBeat = 60000.0f / baseBpm;
        float sectionLengthMs = msPerBeat * beatsPerMeasure;

        sections.clear();
        if (sortedNotes.empty()) return;

        float lastTime = sortedNotes.back().time;
        int numSections = static_cast<int>(lastTime / sectionLengthMs) + 1;

        for (int i = 0; i < numSections; i++) {
            Section section;
            section.bpm = baseBpm;
            section.lengthInSteps = VSLICE_SECTION_LENGTH;
            section.changeBPM = false;
            sections.push_back(section);
        }

        for (const auto& note : sortedNotes) {
            int sectionIndex = static_cast<int>(note.time / sectionLengthMs);
            if (sectionIndex >= 0 && sectionIndex < sections.size()) {
                sections[sectionIndex].notes.emplace_back(note.time, note.direction, note.length);
            }
        }

        for (const auto& change : timeChanges) {
            int sectionIndex = static_cast<int>(change.time / sectionLengthMs);
            if (sectionIndex >= 0 && sectionIndex < sections.size()) {
                sections[sectionIndex].bpm = change.bpm;
                sections[sectionIndex].changeBPM = true;
            }
        }
    }

    bool loadFromJson(const nlohmann::json& chartJson, const nlohmann::json& metaJson) {
        try {
            if (!chartJson.contains("scrollSpeed") || !chartJson["scrollSpeed"].is_object()) {
                std::cerr << "Error: Missing or invalid scrollSpeed field\n";
                return false;
            }
            scrollSpeeds = chartJson["scrollSpeed"].get<std::map<std::string, float>>();
            
            if (!chartJson.contains("notes") || !chartJson["notes"].is_object()) {
                std::cerr << "Error: Missing or invalid notes field\n";
                return false;
            }
            for (const auto& [diff, notes] : chartJson["notes"].items()) {
                if (!notes.is_array()) {
                    std::cerr << "Error: Notes for difficulty " << diff << " is not an array\n";
                    continue;
                }
                std::vector<FNFVSliceNote> diffNotes;
                for (const auto& note : notes) {
                    diffNotes.push_back(FNFVSliceNote::fromJson(note));
                }
                chartNotes[diff] = diffNotes;
            }

            if (chartJson.contains("events") && chartJson["events"].is_array()) {
                for (const auto& event : chartJson["events"]) {
                    events.push_back(FNFVSliceEvent::fromJson(event));
                }
            }

            if (!metaJson.contains("songName") || !metaJson["songName"].is_string()) {
                std::cerr << "Error: Missing or invalid songName field\n";
                return false;
            }
            title = metaJson["songName"].get<std::string>();
            
            if (metaJson.contains("artist") && metaJson["artist"].is_string()) {
                artist = metaJson["artist"].get<std::string>();
            }
            
            if (metaJson.contains("charter") && metaJson["charter"].is_string()) {
                charter = metaJson["charter"].get<std::string>();
            }
            
            float baseBpm = 120.0f;
            if (metaJson.contains("timeChanges") && metaJson["timeChanges"].is_array()) {
                for (const auto& change : metaJson["timeChanges"]) {
                    timeChanges.push_back(FNFVSliceTimeChange::fromJson(change));
                }
                if (!timeChanges.empty()) {
                    baseBpm = timeChanges[0].bpm;
                    bpm = baseBpm;
                }
            }

            if (metaJson.contains("playData") && metaJson["playData"].is_object()) {
                playData = FNFVSlicePlayData::fromJson(metaJson["playData"]);
            }
            
            if (metaJson.contains("offsets") && metaJson["offsets"].is_object()) {
                offsets = FNFVSliceOffsets::fromJson(metaJson["offsets"]);
            }

            keyCount = 4;

            std::string currentDiff = "normal";
            if (!playData.difficulties.empty()) {
                currentDiff = playData.difficulties[0];
            }
            
            if (scrollSpeeds.contains(currentDiff)) {
                speed = scrollSpeeds[currentDiff];
            }

            if (chartNotes.contains(currentDiff)) {
                createSectionsFromNotes(chartNotes[currentDiff], baseBpm);
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << "\n";
            return false;
        }
    }
};

} // namespace Tsukiyo 