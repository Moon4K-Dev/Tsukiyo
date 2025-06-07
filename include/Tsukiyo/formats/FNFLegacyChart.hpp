#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace Tsukiyo {

class FNFLegacyChart : public Chart {
public:
    FNFLegacyChart() {
        format = Format::FNFLegacy;
    }
    
    ~FNFLegacyChart() override = default;

    bool loadFromFile(const std::string& path) override {
        try {
            std::filesystem::path filePath(path);
            std::string fileName = filePath.stem().string();
            
            std::cerr << "Loading FNF Legacy chart: " << path << std::endl;
            
            if (fileName.ends_with("-easy")) {
                difficulty = Difficulty::Easy;
                title = capitalizeFirst(fileName.substr(0, fileName.length() - 5));
            } else if (fileName.ends_with("-hard")) {
                difficulty = Difficulty::Hard;
                title = capitalizeFirst(fileName.substr(0, fileName.length() - 5));
            } else {
                size_t dashPos = fileName.find_last_of('-');
                if (dashPos != std::string::npos && dashPos > 0 && dashPos < fileName.length() - 1) {
                    difficulty = Difficulty::Custom;
                    customDifficulty = fileName.substr(dashPos + 1);
                    title = capitalizeFirst(fileName.substr(0, dashPos));
                } else {
                    difficulty = Difficulty::Normal;
                    title = capitalizeFirst(fileName);
                }
            }

            std::cerr << "Detected title: " << title << std::endl;
            std::cerr << "Detected difficulty: " << static_cast<int>(difficulty) << std::endl;

            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << path << std::endl;
                return false;
            }

            std::string rawJson((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            while (!rawJson.empty() && std::isspace(rawJson.back())) {
                rawJson.pop_back();
            }
            while (!rawJson.empty() && rawJson.back() != '}') {
                rawJson.pop_back();
            }

            std::cerr << "Parsing JSON..." << std::endl;
            nlohmann::json json = nlohmann::json::parse(rawJson);
            return parseFromJson(json);
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading FNF Legacy chart: " << e.what() << std::endl;
            return false;
        }
    }

    bool saveToFile(const std::string& path) const override {
        try {
            std::filesystem::path filePath(path);
            std::string fileName = title;
            
            switch (difficulty) {
                case Difficulty::Easy:
                    fileName += "-easy";
                    break;
                case Difficulty::Hard:
                    fileName += "-hard";
                    break;
                case Difficulty::Custom:
                    if (!customDifficulty.empty()) {
                        fileName += "-" + customDifficulty;
                    }
                    break;
                default:
                    break;
            }
            
            std::filesystem::path newPath = filePath.parent_path() / (fileName + filePath.extension().string());
            
            std::ofstream file(newPath);
            if (!file.is_open()) {
                return false;
            }

            nlohmann::json json = toJson();
            file << json.dump(4);
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

private:
    bool parseFromJson(const nlohmann::json& j) {
        try {
            nlohmann::json songData = j;
            if (j.contains("song") && j["song"].is_object()) {
                songData = j["song"];
            }

            std::cerr << "Parsing song data..." << std::endl;

            if (!songData.contains("song") || !songData["song"].is_string()) {
                std::cerr << "Error: Missing or invalid 'song' field" << std::endl;
                return false;
            }
            
            std::string jsonTitle = songData["song"].get<std::string>();
            if (title.empty()) {
                title = capitalizeFirst(jsonTitle);
            }

            std::cerr << "Found song title: " << title << std::endl;

            if (!songData.contains("bpm") || !songData["bpm"].is_number()) {
                std::cerr << "Error: Missing or invalid 'bpm' field" << std::endl;
                return false;
            }
            bpm = static_cast<float>(songData["bpm"].get<int>());
            std::cerr << "Found BPM: " << bpm << std::endl;

            speed = songData.value("speed", 1.0f);
            std::cerr << "Found speed: " << speed << std::endl;

            if (!songData.contains("notes") || !songData["notes"].is_array()) {
                std::cerr << "Error: Missing or invalid 'notes' array" << std::endl;
                return false;
            }

            sections.clear();
            for (const auto& noteJson : songData["notes"]) {
                Section section;
                section.lengthInSteps = noteJson.value("lengthInSteps", 16);
                section.bpm = noteJson.value("bpm", bpm);
                section.changeBPM = noteJson.value("changeBPM", false);

                bool mustHitSection = noteJson.value("mustHitSection", true);
                bool altAnim = noteJson.value("altAnim", false);

                if (noteJson.contains("sectionNotes") && noteJson["sectionNotes"].is_array()) {
                    for (const auto& noteData : noteJson["sectionNotes"]) {
                        if (noteData.is_array() && noteData.size() >= 2) {
                            float time = noteData[0].get<float>();
                            int lane = noteData[1].get<int>();
                            
                            if (lane < 0) {
                                lane = -lane - 1;
                            }
                            
                            float duration = 0.0f;
                            if (noteData.size() >= 3 && noteData[2].is_number()) {
                                duration = noteData[2].get<float>();
                            }

                            section.notes.emplace_back(time, lane, duration);
                        }
                    }
                }

                sections.push_back(std::move(section));
            }

            keyCount = 4;
            std::cerr << "Successfully parsed chart with " << sections.size() << " sections" << std::endl;

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing FNF Legacy chart: " << e.what() << std::endl;
            return false;
        }
    }

    nlohmann::json toJson() const {
        nlohmann::json songData;
        
        songData["song"] = title;
        songData["bpm"] = static_cast<int>(bpm);
        songData["speed"] = speed;
        songData["needsVoices"] = true;
        songData["player1"] = "bf";
        songData["player2"] = "dad";
        
        nlohmann::json jsonSections = nlohmann::json::array();
        for (const auto& section : sections) {
            nlohmann::json jsonSection;
            jsonSection["lengthInSteps"] = section.lengthInSteps;
            jsonSection["bpm"] = section.bpm;
            jsonSection["changeBPM"] = section.changeBPM;
            jsonSection["mustHitSection"] = true;
            jsonSection["altAnim"] = false;
            
            nlohmann::json sectionNotes = nlohmann::json::array();
            for (const auto& note : section.notes) {
                nlohmann::json noteData = nlohmann::json::array();
                noteData.push_back(note.time);
                noteData.push_back(note.lane);
                if (note.isHold) {
                    noteData.push_back(note.duration);
                } else {
                    noteData.push_back(0);
                }
                sectionNotes.push_back(noteData);
            }
            jsonSection["sectionNotes"] = sectionNotes;
            
            jsonSections.push_back(std::move(jsonSection));
        }
        songData["notes"] = std::move(jsonSections);

        nlohmann::json json;
        json["song"] = songData;
        return json;
    }
};

} // namespace Tsukiyo 