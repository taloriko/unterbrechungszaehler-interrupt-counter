#pragma once

#include "interruption_types.h"

namespace DisplayViews {

void begin(const InterruptionTypes::Summary &summary);
void update(const InterruptionTypes::Summary &summary);
void notifyInterruption(bool flashEnabled);
void requestHomeRefresh();
// Applies changed display preferences without introducing another timer/task.
// The normal project update loop performs the actual redraw/contrast command.
void settingsChanged();

}  // namespace DisplayViews
