#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace Tsukiyo {

class RhythmButtonsChartCustom : public Chart {
public:
    RhythmButtonsChartCustom() {
        format = Format::RhythmButtonsCustom;
        keyCount = 6;
    }
    
    ~RhythmButtonsChartCustom() override = default;

    const std::vector<std::string>& getDifficultyNames() const {
        return difficultyNames;
    }

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
    std::vector<std::string> difficultyNames;

    bool parseFromJson(const nlohmann::json& j) {
        try {
            if (j.contains("song") && j["song"].is_object()) {
                const auto& songData = j["song"];
                
                title = songData.value("title", "Rhythm Buttons Chart");
                artist = songData.value("artist", "Unknown");
                charter = songData.value("charter", "Rhythm Buttons");
                bpm = songData.value("bpm", 120.0f);
                
                if (songData.contains("difficulties") && songData["difficulties"].is_array()) {
                    const auto& diffs = songData["difficulties"];
                    if (!diffs.empty() && diffs[0].is_string()) {
                        std::string diffName = diffs[0].get<std::string>();
                        if (diffName == "easy") {
                            difficulty = Difficulty::Easy;
                        } else if (diffName == "normal") {
                            difficulty = Difficulty::Normal;
                        } else if (diffName == "hard") {
                            difficulty = Difficulty::Hard;
                        } else {
                            difficulty = Difficulty::Custom;
                            customDifficulty = diffName;
                        }
                    }
                }
            } else {
                title = "Rhythm Buttons Chart";
                artist = "Unknown";
                charter = "Rhythm Buttons";
                bpm = 120.0f;
                difficulty = Difficulty::Normal;
            }
            
            speed = 1.0f;
            keyCount = 6;
            sections.clear();
            difficultyNames.clear();
            
            if (!j.contains("charts") || !j["charts"].is_object()) {
                std::cerr << "Error: Missing charts object\n";
                return false;
            }
            
            const auto& charts = j["charts"];
            float beatDuration = 60.0f / bpm / 4.0f;
            
            std::vector<std::string> difficulties;
            if (j.contains("song") && j["song"].contains("difficulties") && j["song"]["difficulties"].is_array()) {
                const auto& diffs = j["song"]["difficulties"];
                for (const auto& diff : diffs) {
                    if (diff.is_string()) {
                        difficulties.push_back(diff.get<std::string>());
                    }
                }
            }
            
            if (difficulties.empty()) {
                difficulties = {"easy", "normal", "hard"};
            }
            
            for (const auto& diffName : difficulties) {
                if (charts.contains(diffName) && charts[diffName].is_array() && !charts[diffName].empty()) {
                    const auto& chartData = charts[diffName];
                    
                    Section section(bpm, 16);
                    section.changeBPM = false;
                    
                    for (const auto& noteObj : chartData) {
                        if (!noteObj.is_object()) {
                            continue;
                        }
                        
                        if (!noteObj.contains("beat") || !noteObj.contains("button")) {
                            continue;
                        }
                        
                        try {
                            int beatNumber = noteObj["beat"].get<int>();
                            int buttonNumber = noteObj["button"].get<int>();
                            
                            if (buttonNumber < 1 || buttonNumber > 6) {
                                std::cerr << "Warning: Invalid button number " << buttonNumber << ", skipping\n";
                                continue;
                            }
                            
                            int lane = buttonNumber - 1;
                            float time = (beatNumber - 1) * beatDuration;
                            
                            section.notes.emplace_back(time, lane, 0.0f);
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Error parsing note: " << e.what() << "\n";
                            continue;
                        }
                    }
                    
                    std::sort(section.notes.begin(), section.notes.end(), 
                             [](const Note& a, const Note& b) { return a.time < b.time; });
                    
                    sections.push_back(std::move(section));
                    difficultyNames.push_back(diffName);
                }
            }
            
            if (sections.empty()) {
                std::cerr << "Error: No available difficulties found\n";
                return false;
            }
            
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing chart: " << e.what() << "\n";
            return false;
        }
    }

    nlohmann::json toJson() const {
        nlohmann::json json;
        
        nlohmann::json songData;
        songData["title"] = title;
        songData["artist"] = artist;
        songData["charter"] = charter;
        songData["bpm"] = bpm;
        
        std::vector<std::string> difficulties;
        std::vector<std::string> diffNames = {"easy", "normal", "hard"};
        
        for (size_t i = 0; i < sections.size() && i < diffNames.size(); ++i) {
            if (!sections[i].notes.empty()) {
                difficulties.push_back(diffNames[i]);
            }
        }
        
        songData["difficulties"] = difficulties;
        json["song"] = songData;
        
        nlohmann::json charts;
        float beatDuration = 60.0f / bpm / 4.0f;
        
        for (size_t i = 0; i < sections.size() && i < diffNames.size(); ++i) {
            if (!sections[i].notes.empty()) {
                nlohmann::json chartArray = nlohmann::json::array();
                
                for (const auto& note : sections[i].notes) {
                    nlohmann::json noteObj;
                    int beatNumber = static_cast<int>(note.time / beatDuration) + 1;
                    int buttonNumber = note.lane + 1;
                    
                    noteObj["beat"] = beatNumber;
                    noteObj["button"] = buttonNumber;
                    chartArray.push_back(noteObj);
                }
                
                charts[diffNames[i]] = chartArray;
            }
        }
        
        json["charts"] = charts;
        
        return json;
    }
};

} // namespace Tsukiyo 