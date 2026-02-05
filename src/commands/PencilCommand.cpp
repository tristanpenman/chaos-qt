#include "PencilCommand.h"

#include "../Map.h"

using namespace std;

PencilCommand::PencilCommand(Map& map)
  : m_map(map)
{

}

void PencilCommand::addChange(int layer, int x, int y, int value)
{
  m_changes[{layer, x, y}] = value;
}

PencilCommand::Result PencilCommand::commit()
{
  // make undo command
  auto undoCommand = std::make_shared<PencilCommand>(m_map);

  // update the map
  vector<Change> changes;
  for (auto& entry : m_changes) {
    const auto layer = entry.first.layer;
    const auto x = entry.first.x;
    const auto y = entry.first.y;
    const auto value = entry.second;

    const auto layerValue = static_cast<uint8_t>(layer);
    const auto xValue = static_cast<uint16_t>(x);
    const auto yValue = static_cast<uint16_t>(y);
    const auto newValue = static_cast<uint8_t>(value);

    // save old value to undo command
    const auto oldValue = m_map.getValue(layerValue, xValue, yValue);
    undoCommand->addChange(layer, x, y, oldValue);

    // commit changes
    m_map.setValue(layerValue, xValue, yValue, newValue);
    changes.push_back({ layer, x, y, value });
  }

  return {
    undoCommand,
    changes
  };
}
