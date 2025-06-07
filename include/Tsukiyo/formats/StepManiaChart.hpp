#pragma once
#include "../Chart.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>

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

            if (!diffData.empty()) {
                std::vector<std::string> preferredDiffs = {"Hard", "Normal", "Easy"};
                const StepManiaDiffData* selectedDiff = nullptr;

                for (const auto& diff : preferredDiffs) {
                    auto it = diffData.find(diff);
                    if (it != diffData.end()) {
                        selectedDiff = &it->second;
                        difficulty = diff == "Hard" ? Difficulty::Hard :
                                   diff == "Normal" ? Difficulty::Normal :
                                   diff == "Easy" ? Difficulty::Easy :
                                   Difficulty::Custom;
                        break;
                    }
                }

                if (!selectedDiff && !diffData.empty()) {
                    selectedDiff = &diffData.begin()->second;
                    difficulty = Difficulty::Custom;
                    customDifficulty = diffData.begin()->first;
                }

                if (selectedDiff) {
                    convertNotesToSections(*selectedDiff);
                }
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
            if (!line.empty()) {
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
        int measureCount = 0;
        
        for (size_t i = 5; i < noteData.size(); ++i) {
            std::string step = trim(noteData[i]);
            
            if (step == "," || step == ";") {
                if (!currentMeasure.empty()) {
                    diffInfo.notes.push_back(currentMeasure);
                    currentMeasure.clear();
                    measureCount++;
                }
            } 
            else if (!step.empty()) {
                currentMeasure.push_back(step);
            }
        }

        if (!currentMeasure.empty()) {
            diffInfo.notes.push_back(currentMeasure);
            measureCount++;
        }

        diffData[diff] = diffInfo;
        keyCount = static_cast<int>(diffInfo.dance);
    }

    void convertNotesToSections(const StepManiaDiffData& diffInfo) {
        sections.clear();
        if (diffInfo.notes.empty()) return;

        float currentBPM = bpm;
        float beatsPerMeasure = 4.0f;
        float msPerBeat = 60000.0f / currentBPM;
        float measureLengthMs = msPerBeat * beatsPerMeasure;

        int totalNotes = 0;
        float currentTime = 0.0f;

        for (size_t measureIndex = 0; measureIndex < diffInfo.notes.size(); measureIndex++) {
            const auto& measure = diffInfo.notes[measureIndex];
            if (measure.empty()) {
                currentTime += measureLengthMs;
                continue;
            }

            Section section;
            section.bpm = currentBPM;
            section.lengthInSteps = 16;
            section.changeBPM = false;

            float measureStartBeat = measureIndex * beatsPerMeasure;
            for (const auto& bpmChange : bpmChanges) {
                if (bpmChange.beat >= measureStartBeat && bpmChange.beat < measureStartBeat + beatsPerMeasure) {
                    section.bpm = bpmChange.bpm;
                    section.changeBPM = true;
                    currentBPM = bpmChange.bpm;
                    msPerBeat = 60000.0f / currentBPM;
                    measureLengthMs = msPerBeat * beatsPerMeasure;
                    break;
                }
            }

            float stepTimeIncrement = measureLengthMs / measure.size();
            std::vector<float> holdStartTimes(keyCount, -1.0f);

            for (size_t stepIndex = 0; stepIndex < measure.size(); stepIndex++) {
                const std::string& step = measure[stepIndex];
                float stepTime = currentTime + (stepIndex * stepTimeIncrement);

                for (size_t lane = 0; lane < std::clamp(step.length(), size_t{0}, static_cast<size_t>(keyCount)); lane++) {
                    char noteType = step[lane];

                    switch (static_cast<StepManiaNote>(noteType)) {
                        case StepManiaNote::NOTE:
                            section.notes.emplace_back(stepTime, lane, 0.0f);
                            totalNotes++;
                            break;

                        case StepManiaNote::HOLD_HEAD:
                            holdStartTimes[lane] = stepTime;
                            break;

                        case StepManiaNote::HOLD_TAIL:
                            if (holdStartTimes[lane] >= 0.0f) {
                                float duration = stepTime - holdStartTimes[lane];
                                section.notes.emplace_back(holdStartTimes[lane], lane, duration);
                                holdStartTimes[lane] = -1.0f;
                                totalNotes++;
                            }
                            break;

                        case StepManiaNote::ROLL_HEAD:
                            holdStartTimes[lane] = stepTime;
                            break;

                        default:
                            break;
                    }
                }
            }

            for (size_t lane = 0; lane < holdStartTimes.size(); lane++) {
                if (holdStartTimes[lane] >= 0.0f) {
                    float duration = currentTime + measureLengthMs - holdStartTimes[lane];
                    section.notes.emplace_back(holdStartTimes[lane], lane, duration);
                    totalNotes++;
                }
            }

            if (!section.notes.empty()) {
                sections.push_back(std::move(section));
            }

            currentTime += measureLengthMs;
        }

        for (auto& section : sections) {
            std::sort(section.notes.begin(), section.notes.end(),
                     [](const Note& a, const Note& b) { return a.time < b.time; });
        }
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