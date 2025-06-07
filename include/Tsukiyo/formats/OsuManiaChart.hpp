#pragma once
#include "../Chart.hpp"
#include "../external/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

namespace Tsukiyo {

constexpr float OSU_SCROLL_SPEED = 0.45f;
constexpr int OSU_CIRCLE_SIZE = 512;
constexpr int OSU_SECTION_LENGTH = 16;
const std::string OSU_FORMAT_VERSION = "osu file format v14";

enum class OsuType {
    DEFAULT = 0,
    HOLD = 128,
    NO_NEW_COMBO = 1,
    NEW_COMBO = 5
};

enum class OsuHitsound {
    NORMAL = 0,
    WHISTLE = 2,
    FINISH = 4,
    WHISTLE_FINISH = 6,
    CLAP = 8,
    WHISTLE_CLAP = 10,
    FINISH_CLAP = 12
};

enum class OsuSampleset {
    AUTO = 0,
    NORMAL = 1,
    SOFT = 2,
    DRUM = 3
};

struct TimingPoint {
    float time;
    float beatLength;
    int meter;
    bool isInherited;

    TimingPoint(float t, float bl, int m, bool inherited)
        : time(t), beatLength(bl), meter(m), isInherited(inherited) {}
};

class OsuManiaChart : public Chart {
public:
    OsuManiaChart() {
        format = Format::OsuMania;
        keyCount = 4;
    }
    
    ~OsuManiaChart() override = default;

    bool loadFromFile(const std::string& path) override {
        try {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }

            std::string line;
            std::string currentSection;
            std::map<std::string, std::vector<std::string>> sections;

            char bom[3];
            file.read(bom, 3);
            if (!(bom[0] == (char)0xEF && bom[1] == (char)0xBB && bom[2] == (char)0xBF)) {
                file.seekg(0);
            }

            while (std::getline(file, line)) {
                if (!line.empty() && line[0] == (char)0xEF && 
                    line.length() >= 3 && 
                    line[1] == (char)0xBB && 
                    line[2] == (char)0xBF) {
                    line = line.substr(3);
                }

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (line.empty() || line.starts_with("//")) {
                    continue;
                }

                line.erase(
                    std::remove_if(line.begin(), line.end(),
                        [](unsigned char c) { 
                            return !std::isprint(static_cast<unsigned char>(c)) && !std::isspace(static_cast<unsigned char>(c));
                        }),
                    line.end()
                );

                if (line.starts_with("[") && line.ends_with("]")) {
                    currentSection = line.substr(1, line.length() - 2);
                    sections[currentSection] = std::vector<std::string>();
                    continue;
                }

                if (!currentSection.empty()) {
                    sections[currentSection].push_back(line);
                }
            }

            if (sections.contains("Metadata")) {
                for (const auto& line : sections["Metadata"]) {
                    auto [key, value] = parseKeyValue(line);
                    if (key == "Title") title = capitalizeFirst(value);
                    else if (key == "Artist") artist = value;
                    else if (key == "Creator") charter = value;
                }
            }

            if (sections.contains("Difficulty")) {
                for (const auto& line : sections["Difficulty"]) {
                    auto [key, value] = parseKeyValue(line);
                    if (key == "CircleSize") keyCount = std::stoi(value);
                    else if (key == "SliderMultiplier") speed = std::stof(value) / OSU_SCROLL_SPEED;
                }
            }

            std::vector<TimingPoint> timingPoints;
            if (sections.contains("TimingPoints")) {
                for (const auto& line : sections["TimingPoints"]) {
                    std::stringstream ss(line);
                    std::string token;
                    std::vector<std::string> tokens;
                    
                    while (std::getline(ss, token, ',')) {
                        tokens.push_back(token);
                    }
                    
                    if (tokens.size() >= 2) {
                        float time = std::stof(tokens[0]);
                        float beatLength = std::stof(tokens[1]);
                        int meter = tokens.size() >= 3 ? std::stoi(tokens[2]) : 4;
                        bool isInherited = tokens.size() >= 7 ? tokens[6] == "0" : false;
                        
                        timingPoints.emplace_back(time, beatLength, meter, isInherited);
                    }
                }
            }

            if (!timingPoints.empty()) {
                bpm = 60000.0f / timingPoints[0].beatLength;
            }

            std::vector<Note> notes;
            if (sections.contains("HitObjects")) {
                for (const auto& line : sections["HitObjects"]) {
                    std::stringstream ss(line);
                    std::string token;
                    std::vector<std::string> tokens;
                    
                    while (std::getline(ss, token, ',')) {
                        tokens.push_back(token);
                    }
                    
                    if (tokens.size() >= 4) {
                        int x = std::stoi(tokens[0]);
                        int lane = (x * keyCount) / OSU_CIRCLE_SIZE;
                        float time = std::stof(tokens[2]);
                        int type = std::stoi(tokens[3]);
                        
                        float duration = 0.0f;
                        if ((type & static_cast<int>(OsuType::HOLD)) && tokens.size() >= 6) {
                            size_t colonPos = tokens[5].find(':');
                            if (colonPos != std::string::npos) {
                                float endTime = std::stof(tokens[5].substr(0, colonPos));
                                duration = endTime - time;
                            }
                        }
                        
                        notes.emplace_back(time, lane, duration);
                    }
                }
            }

            std::sort(notes.begin(), notes.end(), 
                     [](const Note& a, const Note& b) { return a.time < b.time; });

            createSections(notes, timingPoints);

            return true;
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

            file << OSU_FORMAT_VERSION << "\n\n";

            file << "[General]\n";
            file << "AudioFilename: audio.mp3\n";
            file << "Mode: 3\n\n";

            file << "[Metadata]\n";
            file << "Title:" << title << "\n";
            file << "Artist:" << artist << "\n";
            file << "Creator:" << charter << "\n";
            file << "Version:Normal\n\n";

            file << "[Difficulty]\n";
            file << "CircleSize:" << keyCount << "\n";
            file << "SliderMultiplier:" << (speed * OSU_SCROLL_SPEED) << "\n\n";

            file << "[TimingPoints]\n";
            for (const auto& section : sections) {
                float beatLength = 60000.0f / section.bpm;
                file << "0," << beatLength << ",4,1,0,100,1,0\n";
            }
            file << "\n";

            file << "[HitObjects]\n";
            for (const auto& section : sections) {
                for (const auto& note : section.notes) {
                    int x = static_cast<int>(note.lane * OSU_CIRCLE_SIZE / keyCount);
                    int type = static_cast<int>(note.isHold ? OsuType::HOLD : OsuType::NO_NEW_COMBO);
                    
                    if (note.isHold) {
                        file << x << ",0," << note.time << "," << type << ",0,"
                             << (note.time + note.duration) << ":0:0:0:0:\n";
                    } else {
                        file << x << ",0," << note.time << "," << type << ",0,0:0:0:0:\n";
                    }
                }
            }

            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

private:
    void createSections(const std::vector<Note>& notes, const std::vector<TimingPoint>& timingPoints) {
        sections.clear();
        if (notes.empty()) return;

        float currentBPM = bpm;
        float msPerBeat = 60000.0f / currentBPM;
        float beatsPerMeasure = 4.0f;
        float sectionLengthMs = msPerBeat * beatsPerMeasure;

        float lastTime = notes.back().time;
        int numSections = static_cast<int>(lastTime / sectionLengthMs) + 1;

        for (int i = 0; i < numSections; i++) {
            Section section;
            section.bpm = currentBPM;
            section.lengthInSteps = OSU_SECTION_LENGTH;
            section.changeBPM = false;
            sections.push_back(section);
        }

        for (const auto& note : notes) {
            int sectionIndex = static_cast<int>(note.time / sectionLengthMs);
            if (sectionIndex >= 0 && sectionIndex < sections.size()) {
                sections[sectionIndex].notes.push_back(note);
            }
        }

        for (const auto& point : timingPoints) {
            if (!point.isInherited) {
                int sectionIndex = static_cast<int>(point.time / sectionLengthMs);
                if (sectionIndex >= 0 && sectionIndex < sections.size()) {
                    float newBPM = 60000.0f / point.beatLength;
                    sections[sectionIndex].bpm = newBPM;
                    sections[sectionIndex].changeBPM = true;
                    currentBPM = newBPM;
                }
            }
        }
    }

    std::pair<std::string, std::string> parseKeyValue(const std::string& line) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            return {"", ""};
        }
        
        std::string key = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        return {key, value};
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
};

} // namespace Tsukiyo 