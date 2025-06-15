#include <iostream>
#include <Tsukiyo/ChartLoader.hpp>
#include <fstream>

void printChartInfo(const std::unique_ptr<Tsukiyo::Chart>& chart) {
    std::cout << "Chart Information:\n";
    std::cout << "----------------\n";
    std::cout << "Chart Type: " << chart->format << "\n";
    std::cout << "Song Title: " << chart->title << "\n";
    std::cout << "Artist: " << chart->artist << "\n";
    std::cout << "Charter: " << chart->charter << "\n";

    if (chart->format == Tsukiyo::Chart::Format::FNFVSlice) {
        auto vslice = dynamic_cast<const Tsukiyo::FNFVSliceChart*>(chart.get());
        if (vslice && !vslice->getAvailableDifficulties().empty()) {
            std::cout << "Available Difficulties: ";
            const auto& diffs = vslice->getAvailableDifficulties();
            for (size_t i = 0; i < diffs.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << diffs[i];
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "Difficulty: " << chart->difficulty;
        if (chart->difficulty == Tsukiyo::Chart::Difficulty::Custom) {
            std::cout << " (" << chart->customDifficulty << ")";
        }
        std::cout << "\n";
    }
    
    std::cout << "BPM: " << chart->bpm << "\n";
    std::cout << "Speed: " << chart->speed << "\n";
    std::cout << "Key Count: " << chart->keyCount << "\n";
    std::cout << "Total Sections: " << chart->sections.size() << "\n\n";
}

bool isVSliceChart(const std::string& chartPath) {
    try {
        std::filesystem::path chartFilePath(chartPath);
        std::string basePath = chartFilePath.parent_path().string();
        std::string fileName = chartFilePath.stem().string();
        
        const std::string chartSuffix = "-chart";
        if (fileName.length() > chartSuffix.length() && 
            fileName.substr(fileName.length() - chartSuffix.length()) == chartSuffix) {
            fileName = fileName.substr(0, fileName.length() - chartSuffix.length());
        }
        
        std::string metaPath = basePath + "/" + fileName + "-metadata.json";
        
        if (!std::filesystem::exists(metaPath)) {
            return false;
        }

        std::ifstream chartFile(chartPath);
        if (!chartFile.is_open()) return false;
        nlohmann::json chartJson = nlohmann::json::parse(chartFile);
        
        if (!chartJson.contains("version") || !chartJson["version"].is_string()) {
            return false;
        }
        
        std::string version = chartJson["version"].get<std::string>();
        return version == Tsukiyo::VSLICE_CHART_VERSION;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool isRhythmButtonsChart(const std::string& chartPath) {
    try {
        std::ifstream chartFile(chartPath);
        if (!chartFile.is_open()) return false;
        
        nlohmann::json chartJson = nlohmann::json::parse(chartFile);
        
        return chartJson.contains("bpm") && 
               chartJson.contains("beats") && 
               chartJson.contains("buttons") &&
               chartJson["beats"].is_array() &&
               chartJson["buttons"].is_array();
    }
    catch (const std::exception&) {
        return false;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_chart_file>\n";
        return 1;
    }

    std::string filePath;
    for (int i = 1; i < argc; i++) {
        if (i > 1) filePath += " ";
        filePath += argv[i];
    }

    if (filePath.front() == '"' && filePath.back() == '"') {
        filePath = filePath.substr(1, filePath.length() - 2);
    }

    if (!std::filesystem::exists(filePath)) {
        std::cout << "File not found: " << filePath << "\n";
        return 1;
    }

    Tsukiyo::Chart::Format format;

    std::string ext = Tsukiyo::getFileExtension(filePath);
    if (ext == ".moon") {
        format = Tsukiyo::Chart::Format::Moon4K;
    } else if (ext == ".json") {
        if (isVSliceChart(filePath)) {
            format = Tsukiyo::Chart::Format::FNFVSlice;
        } else if (isRhythmButtonsChart(filePath)) {
            format = Tsukiyo::Chart::Format::RhythmButtons;
        } else {
            format = Tsukiyo::Chart::Format::FNFLegacy;
        }
    } else if (ext == ".osu") {
        format = Tsukiyo::Chart::Format::OsuMania;
    } else if (ext == ".sm") {
        format = Tsukiyo::Chart::Format::StepMania;
    } else {
        std::cout << "Unsupported file format. Please use .moon, .json, .osu, or .sm files.\n";
        return 1;
    }

    auto chart = Tsukiyo::Chart::createChart(format);
    if (!chart) {
        std::cout << "Failed to create chart reader!\n";
        return 1;
    }

    if (!chart->loadFromFile(filePath)) {
        std::cout << "Failed to load chart file: " << filePath << "\n";
        return 1;
    }

    printChartInfo(chart);
    return 0;
}