#pragma once
#include "../Chart.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

namespace Tsukiyo {

constexpr float STEPMANIA_SCROLL_SPEED = 0.017775f;

enum class StepManiaNote {
    EMPTY = '0',
    NOTE = '1',
    HOLD_HEAD = '2',
    HOLD_TAIL = '3',
    ROLL_HEAD = '4',
    MINE = 'M'
};

enum class StepManiaDance {
    SINGLE = 4,
    DOUBLE = 8
};

struct StepManiaBPM {
    float beat;
    float bpm;
};

struct StepManiaStop {
    float beat;
    float duration;
};

struct StepManiaDiffData {
    std::string diff;
    std::string desc;
    StepManiaDance dance;
    std::vector<std::vector<std::string>> notes;
    std::string charter;
    int meter;
    std::array<float, 5> radar;
};

class StepManiaChart : public Chart {
public:
    StepManiaChart() {
        format = Format::StepMania;
        keyCount = 4;
    }

    ~StepManiaChart() override = default;

    bool loadFromFile(const std::string& path) override {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                return false;
            }

            std::string line;
            std::string currentTag;
            std::stringstream currentData;

            while (std::getline(file, line)) {
                line = trim(line);
                if (line.empty()) continue;

                if (line.front() == '#') {
                    if (!currentTag.empty()) {
                        processTag(currentTag, currentData.str());
                        currentData.str("");
                        currentData.clear();
                    }
                    size_t colonPos = line.find(':');
                    if (colonPos != std::string::npos) {
                        currentTag = line.substr(1, colonPos - 1);
                        std::string initialData = line.substr(colonPos + 1);
                        if (!initialData.empty()) {
                            currentData << initialData << "\n";
                        }
                    }
                } else if (!currentTag.empty()) {
                    currentData << line << "\n";
                }
            }

            if (!currentTag.empty()) {
                processTag(currentTag, currentData.str());
            }

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

            file << "#TITLE:" << title << ";\n";
            file << "#ARTIST:" << artist << ";\n";
            file << "#MUSIC:" << audioFile << ";\n";
            file << "#OFFSET:" << (offset / 1000.0f) << ";\n";

            file << "#BPMS:";
            for (size_t i = 0; i < bpmChanges.size(); ++i) {
                if (i > 0) file << ",";
                file << bpmChanges[i].beat << "=" << bpmChanges[i].bpm;
            }
            file << ";\n";

            if (!stops.empty()) {
                file << "#STOPS:";
                for (size_t i = 0; i < stops.size(); ++i) {
                    if (i > 0) file << ",";
                    file << stops[i].beat << "=" << stops[i].duration;
                }
                file << ";\n";
            }

            for (const auto& [diff, data] : diffData) {
                file << "#NOTES:\n";
                file << "     " << (data.dance == StepManiaDance::SINGLE ? "dance-single" : "dance-double") << ":\n";
                file << "     " << data.charter << ":\n";
                file << "     " << data.diff << ":\n";
                file << "     " << data.meter << ":\n";
                file << "     " << data.radar[0] << "," << data.radar[1] << "," << data.radar[2] << ","
                     << data.radar[3] << "," << data.radar[4] << ":\n";
                
                for (const auto& measure : data.notes) {
                    file << "     ";
                    for (const auto& step : measure) {
                        file << step << "\n     ";
                    }
                    file << ",\n";
                }
                file << ";\n";
            }

            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

private:
    std::string audioFile;
    std::vector<StepManiaBPM> bpmChanges;
    std::vector<StepManiaStop> stops;
    std::map<std::string, StepManiaDiffData> diffData;
    float offset = 0.0f;

    void processTag(const std::string& tag, const std::string& data) {
        std::string cleanData = data;
        while (!cleanData.empty() && (cleanData.back() == ';' || std::isspace(cleanData.back()))) {
            cleanData.pop_back();
        }

        if (tag == "TITLE") {
            title = capitalizeFirst(cleanData);
        }
        else if (tag == "ARTIST") {
            artist = cleanData;
        }
        else if (tag == "MUSIC") {
            audioFile = cleanData;
        }
        else if (tag == "OFFSET") {
            offset = std::stof(cleanData) * 1000.0f;
        }
        else if (tag == "BPMS") {
            parseBPMs(cleanData);
        }
        else if (tag == "STOPS") {
            parseStops(cleanData);
        }
        else if (tag == "NOTES") {
            parseNotes(cleanData);
        }
    }

    void parseBPMs(const std::string& data) {
        std::stringstream ss(data);
        std::string pair;
        while (std::getline(ss, pair, ',')) {
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos) {
                float beat = std::stof(pair.substr(0, equalPos));
                float bpm = std::stof(pair.substr(equalPos + 1));
                bpmChanges.push_back({beat, bpm});
            }
        }
        if (!bpmChanges.empty()) {
            bpm = bpmChanges[0].bpm;
        }
    }

    void parseStops(const std::string& data) {
        std::stringstream ss(data);
        std::string pair;
        while (std::getline(ss, pair, ',')) {
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos) {
                float beat = std::stof(pair.substr(0, equalPos));
                float duration = std::stof(pair.substr(equalPos + 1));
                stops.push_back({beat, duration});
            }
        }
    }

    void parseNotes(const std::string& data) {
        std::stringstream ss(data);
        std::string line;
        std::vector<std::string> noteData;
        
        while (std::getline(ss, line)) {
            line = trim(line);
            if (!line.empty() && line != ",") {
                noteData.push_back(line);
            }
        }

        if (noteData.size() < 6) return;

        std::string danceType = trim(noteData[0]);
        std::string charter = trim(noteData[1]);
        std::string diff = trim(noteData[2]);
        int meter = std::stoi(trim(noteData[3]));
        
        std::string radarStr = trim(noteData[4]);
        std::array<float, 5> radar = {0};
        std::stringstream radarSS(radarStr);
        std::string value;
        int i = 0;
        while (std::getline(radarSS, value, ',') && i < 5) {
            radar[i++] = std::stof(value);
        }

        StepManiaDiffData diffInfo;
        diffInfo.diff = diff;
        diffInfo.charter = charter;
        diffInfo.dance = danceType.find("double") != std::string::npos ? StepManiaDance::DOUBLE : StepManiaDance::SINGLE;
        diffInfo.meter = meter;
        diffInfo.radar = radar;

        std::vector<std::string> currentMeasure;
        for (size_t i = 5; i < noteData.size(); ++i) {
            std::string step = trim(noteData[i]);
            if (step == ",") {
                if (!currentMeasure.empty()) {
                    diffInfo.notes.push_back(currentMeasure);
                    currentMeasure.clear();
                }
            } else if (!step.empty()) {
                currentMeasure.push_back(step);
            }
        }

        diffData[diff] = diffInfo;
        keyCount = static_cast<int>(diffInfo.dance);
    }

    static std::string trim(std::string str) {
        while (!str.empty() && std::isspace(str.front())) {
            str.erase(0, 1);
        }
        while (!str.empty() && std::isspace(str.back())) {
            str.pop_back();
        }
        return str;
    }
};

} // namespace Tsukiyo 