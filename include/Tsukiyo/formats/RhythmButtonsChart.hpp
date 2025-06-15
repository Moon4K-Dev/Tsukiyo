#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace Tsukiyo {

class RhythmButtonsChart : public Chart {
public:
    RhythmButtonsChart() {
        format = Format::RhythmButtons;
        keyCount = 6;
    }
    
    ~RhythmButtonsChart() override = default;

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
            file << json.dump();
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

private:
    bool parseFromJson(const nlohmann::json& j) {
        try {
            bpm = j.value("bpm", 120.0f);
            
            title = "Rhythm Buttons Chart";
            artist = "Unknown";
            charter = "Rhythm Buttons";
            speed = 1.0f;
            keyCount = 6;
            
            sections.clear();
            Section section(bpm, 16);
            
            if (!j.contains("beats") || !j.contains("buttons")) {
                std::cerr << "Error: Missing beats or buttons array\n";
                return false;
            }
            
            const auto& beats = j["beats"];
            const auto& buttons = j["buttons"];
            
            if (!beats.is_array() || !buttons.is_array()) {
                std::cerr << "Error: beats and buttons must be arrays\n";
                return false;
            }
            
            if (beats.size() != buttons.size()) {
                std::cerr << "Error: beats and buttons arrays must have the same length\n";
                return false;
            }
            
            float beatDuration = 60.0f / bpm / 4.0f;
            
            for (size_t i = 0; i < beats.size(); ++i) {
                try {
                    int beatNumber = std::stoi(beats[i].get<std::string>());
                    int buttonNumber = std::stoi(buttons[i].get<std::string>());
                    
                    int lane = buttonNumber - 1;
                    
                    float time = (beatNumber - 1) * beatDuration;
                    
                    section.notes.emplace_back(time, lane, 0.0f);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing beat/button at index " << i << ": " << e.what() << "\n";
                    continue;
                }
            }
            
            std::sort(section.notes.begin(), section.notes.end(), 
                     [](const Note& a, const Note& b) { return a.time < b.time; });
            
            sections.push_back(std::move(section));
            
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing chart: " << e.what() << "\n";
            return false;
        }
    }

    nlohmann::json toJson() const {
        nlohmann::json json;
        
        json["bpm"] = bpm;
        
        std::vector<std::string> beats;
        std::vector<std::string> buttons;
        
        if (!sections.empty()) {
            const auto& section = sections[0];
            float beatDuration = 60.0f / bpm / 4.0f;
            
            for (const auto& note : section.notes) {
                int beatNumber = static_cast<int>(note.time / beatDuration) + 1;
                int buttonNumber = note.lane + 1;
                
                beats.push_back(std::to_string(beatNumber));
                buttons.push_back(std::to_string(buttonNumber));
            }
        }
        
        json["beats"] = beats;
        json["buttons"] = buttons;
        
        return json;
    }
};

} // namespace Tsukiyo