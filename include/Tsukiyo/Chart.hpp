#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

namespace Tsukiyo {

inline std::string capitalizeFirst(std::string str) {
    if (!str.empty()) {
        str[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(str[0])));
        return str;
    }
    return str;
}

struct Note {
    float time;
    int lane;
    float duration;
    bool isHold;
    
    Note(float t, int l, float d = 0.0f) 
        : time(t), lane(l), duration(d), isHold(d > 0.0f) {}
};

struct Section {
    std::vector<Note> notes;
    float bpm;
    int lengthInSteps;
    bool changeBPM;
    
    Section(float _bpm = 120.0f, int steps = 16) 
        : bpm(_bpm), lengthInSteps(steps), changeBPM(false) {}
};

class Moon4KChart;

class Chart {
public:
    enum class Format {
        Moon4K,
        FNFLegacy,
        FNFVSlice,
        OsuMania,
        Quaver,
        StepMania,
        RhythmButtons,
        RhythmButtonsCustom
    };

    enum class Difficulty {
        Easy,
        Normal,
        Hard,
        Custom
    };

    Chart() : speed(1.0f), format(Format::Moon4K), difficulty(Difficulty::Normal) {}
    virtual ~Chart() = default;

    std::string title;
    std::string artist;
    std::string charter;
    float bpm;
    float speed;
    int keyCount;
    std::vector<Section> sections;
    Format format;
    Difficulty difficulty;
    std::string customDifficulty;

    virtual bool loadFromFile(const std::string& path) = 0;
    virtual bool saveToFile(const std::string& path) const = 0;

    static std::unique_ptr<Chart> createChart(Format format);
};

inline std::ostream& operator<<(std::ostream& os, const Chart::Format& format) {
    switch (format) {
        case Chart::Format::Moon4K: return os << "Moon4K";
        case Chart::Format::FNFLegacy: return os << "FNF Legacy";
        case Chart::Format::FNFVSlice: return os << "FNF V-Slice";
        case Chart::Format::OsuMania: return os << "Osu! Mania";
        case Chart::Format::Quaver: return os << "Quaver";
        case Chart::Format::StepMania: return os << "StepMania";
        case Chart::Format::RhythmButtons: return os << "Rhythm Buttons";
        case Chart::Format::RhythmButtonsCustom: return os << "Rhythm Buttons Custom";
        default: return os << "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, const Chart::Difficulty& difficulty) {
    switch (difficulty) {
        case Chart::Difficulty::Easy: return os << "Easy";
        case Chart::Difficulty::Normal: return os << "Normal";
        case Chart::Difficulty::Hard: return os << "Hard";
        case Chart::Difficulty::Custom: return os << "Custom";
        default: return os << "Unknown";
    }
}

} // namespace Tsukiyo 