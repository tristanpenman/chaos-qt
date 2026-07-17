#include "PencilCommand.h"


#include "../Map.h"


PencilCommand::PencilCommand(Map& map)
    : map_(map)
{

}

void PencilCommand::addChange(int layer, int x, int y, int value)
{
    changes_[{layer, x, y}] = value;
}

PencilCommand::Result PencilCommand::commit()
{
    // Make undo command
    auto undoCommand = std::make_shared<PencilCommand>(map_);

    std::vector<Change> changes;
    for (auto& entry : changes_) {
        const auto layer = entry.first.layer;
        const auto x = entry.first.x;
        const auto y = entry.first.y;
        const auto value = entry.second;

        const auto layerValue = static_cast<uint8_t>(layer);
        const auto xValue = static_cast<uint16_t>(x);
        const auto yValue = static_cast<uint16_t>(y);
        const auto newValue = static_cast<uint8_t>(value);

        const auto oldValue = map_.getValue(layerValue, xValue, yValue);
        undoCommand->addChange(layer, x, y, oldValue);

        map_.setValue(layerValue, xValue, yValue, newValue);
        changes.push_back({ layer, x, y, value });
    }

    return {
        undoCommand,
        changes
    };
}
