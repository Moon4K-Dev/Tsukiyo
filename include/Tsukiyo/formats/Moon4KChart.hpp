#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace Tsukiyo {

class Moon4KChart : public Chart {
public:
    Moon4KChart() {
        format = Format::Moon4K;
    }
    
    ~Moon4KChart() override = default;

    bool loadFromFile(const std::string& path) override {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
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

            nlohmann::json json = nlohmann::json::parse(rawJson);
            return parseFromJson(json);
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading file: " << e.what() << "\n";
            return false;
        }
    }

    bool saveToFile(const std::string& path) const override {
        try {
            std::ofstream file(path);
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

            if (songData.contains("song")) {
                if (songData["song"].is_string()) {
                    title = capitalizeFirst(songData["song"].get<std::string>());
                } else if (songData["song"].is_object() && songData["song"].contains("name")) {
                    title = capitalizeFirst(songData["song"]["name"].get<std::string>());
                } else {
                    std::cerr << "Error: Invalid song name format\n";
                    return false;
                }
            } else {
                std::cerr << "Error: Missing song name\n";
                return false;
            }

            bpm = songData.value("bpm", 100.0f);
            speed = songData.value("speed", 1.0f);
            keyCount = songData.value("keyCount", 4);

            if (!songData.contains("notes")) {
                std::cerr << "Error: Missing notes array\n";
                return false;
            }

            sections.clear();
            for (const auto& noteJson : songData["notes"]) {
                Section section;
                section.lengthInSteps = noteJson.value("lengthInSteps", 16);
                section.bpm = noteJson.value("bpm", bpm);
                section.changeBPM = noteJson.value("changeBPM", false);

                if (noteJson.contains("sectionNotes") && noteJson["sectionNotes"].is_array()) {
                    for (const auto& noteData : noteJson["sectionNotes"]) {
                        if (noteData.is_object()) {
                            float time = noteData.value("noteStrum", 0.0f);
                            int lane = noteData.value("noteData", 0);
                            float duration = noteData.value("noteSus", 0.0f);
                            section.notes.emplace_back(time, lane, duration);
                        }
                        else if (noteData.is_array()) {
                            std::vector<float> values;
                            for (const auto& value : noteData) {
                                if (value.is_number()) {
                                    values.push_back(value.get<float>());
                                } else if (value.is_string()) {
                                    try {
                                        values.push_back(std::stof(value.get<std::string>()));
                                    } catch (...) {
                                        continue;
                                    }
                                }
                            }

                            if (values.size() >= 2) {
                                float time = values[0];
                                int lane = static_cast<int>(values[1]);
                                float duration = (values.size() >= 4) ? values[3] : 0.0f;
                                section.notes.emplace_back(time, lane, duration);
                            }
                        }
                    }
                }

                sections.push_back(std::move(section));
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing chart: " << e.what() << "\n";
            return false;
        }
    }

    nlohmann::json toJson() const {
        nlohmann::json songData;
        
        songData["song"] = title;
        songData["bpm"] = bpm;
        songData["keyCount"] = keyCount;
        songData["speed"] = speed;
        
        nlohmann::json jsonSections = nlohmann::json::array();
        for (const auto& section : sections) {
            nlohmann::json jsonSection;
            jsonSection["lengthInSteps"] = section.lengthInSteps;
            jsonSection["bpm"] = section.bpm;
            jsonSection["changeBPM"] = section.changeBPM;
            jsonSection["mustHitSection"] = true;
            jsonSection["typeOfSection"] = 0;
            jsonSection["altAnim"] = false;
            
            nlohmann::json sectionNotes = nlohmann::json::array();
            for (const auto& note : section.notes) {
                nlohmann::json noteData = nlohmann::json::array();
                noteData.push_back(note.time);
                noteData.push_back(note.lane);
                noteData.push_back(0.0f);
                if (note.isHold) {
                    noteData.push_back(note.duration);
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