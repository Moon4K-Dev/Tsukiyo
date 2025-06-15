#pragma once
#include "Chart.hpp"
#include "formats/Moon4KChart.hpp"
#include "formats/FNFLegacyChart.hpp"
#include "formats/FNFVSliceChart.hpp"
#include "formats/OsuManiaChart.hpp"
#include "formats/StepManiaChart.hpp"
#include "formats/RhythmButtonsChart.hpp"
#include <filesystem>
#include <string>
#include <algorithm>

namespace Tsukiyo {

inline std::string getFileExtension(const std::string& path) {
    std::string cleanPath = path;
    if (cleanPath.front() == '"' && cleanPath.back() == '"') {
        cleanPath = cleanPath.substr(1, cleanPath.length() - 2);
    }
    
    std::filesystem::path fsPath(cleanPath);
    std::string ext = fsPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

inline std::unique_ptr<Chart> Chart::createChart(Format format) {
    switch (format) {
        case Format::Moon4K:
            return std::make_unique<Moon4KChart>();
        case Format::FNFLegacy:
            return std::make_unique<FNFLegacyChart>();
        case Format::FNFVSlice:
            return std::make_unique<FNFVSliceChart>();
        case Format::OsuMania:
            return std::make_unique<OsuManiaChart>();
        case Format::StepMania:
            return std::make_unique<StepManiaChart>();
        case Format::RhythmButtons:
            return std::make_unique<RhythmButtonsChart>();
        default:
            return nullptr;
    }
}

} // namespace Tsukiyo 