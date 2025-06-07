#pragma once
#include "Chart.hpp"
#include "ChartLoader.hpp"
#include <memory>
#include <stdexcept>

namespace Tsukiyo {

class ChartConverter {
public:
    static std::unique_ptr<Chart> convert(const Chart& sourceChart, Chart::Format targetFormat) {
        auto targetChart = Chart::createChart(targetFormat);
        if (!targetChart) {
            throw std::runtime_error("Failed to create target chart format");
        }

        targetChart->title = sourceChart.title;
        targetChart->artist = sourceChart.artist;
        targetChart->charter = sourceChart.charter;
        targetChart->bpm = sourceChart.bpm;
        targetChart->difficulty = sourceChart.difficulty;
        targetChart->customDifficulty = sourceChart.customDifficulty;

        targetChart->speed = convertSpeed(sourceChart.format, targetFormat, sourceChart.speed);

        targetChart->keyCount = convertKeyCount(sourceChart.keyCount, targetFormat);

        targetChart->sections.clear();
        for (const auto& sourceSection : sourceChart.sections) {
            Section targetSection;
            targetSection.bpm = sourceSection.bpm;
            targetSection.lengthInSteps = sourceSection.lengthInSteps;
            targetSection.changeBPM = sourceSection.changeBPM;

            for (const auto& sourceNote : sourceSection.notes) {
                Note targetNote(
                    sourceNote.time,
                    convertLane(sourceNote.lane, sourceChart.keyCount, targetChart->keyCount),
                    sourceNote.duration
                );
                targetSection.notes.push_back(targetNote);
            }

            targetChart->sections.push_back(std::move(targetSection));
        }

        return targetChart;
    }

private:
    static float convertSpeed(Chart::Format sourceFormat, Chart::Format targetFormat, float sourceSpeed) {
        float normalizedSpeed = sourceSpeed;

        switch (sourceFormat) {
            case Chart::Format::OsuMania:
                normalizedSpeed *= OSU_SCROLL_SPEED;
                break;
            case Chart::Format::StepMania:
                normalizedSpeed *= STEPMANIA_SCROLL_SPEED;
                break;
            default:
                break;
        }

        switch (targetFormat) {
            case Chart::Format::OsuMania:
                return normalizedSpeed / OSU_SCROLL_SPEED;
            case Chart::Format::StepMania:
                return normalizedSpeed / STEPMANIA_SCROLL_SPEED;
            default:
                return normalizedSpeed;
        }
    }

    static int convertKeyCount(int sourceKeys, Chart::Format targetFormat) {
        switch (targetFormat) {
            case Chart::Format::Moon4K:
            case Chart::Format::FNFLegacy:
            case Chart::Format::FNFVSlice:
                return 4;
            case Chart::Format::OsuMania:
                return std::clamp(sourceKeys, 1, 18);
            case Chart::Format::StepMania:
                return (sourceKeys > 4) ? 8 : 4;
            default:
                return sourceKeys;
        }
    }

    static int convertLane(int sourceLane, int sourceKeys, int targetKeys) {
        if (sourceKeys == targetKeys) {
            return sourceLane;
        }
        float normalizedPos = static_cast<float>(sourceLane) / static_cast<float>(sourceKeys);
        int targetLane = static_cast<int>(normalizedPos * targetKeys);        
        return std::clamp(targetLane, 0, targetKeys - 1);
    }
};

} // namespace Tsukiyo 