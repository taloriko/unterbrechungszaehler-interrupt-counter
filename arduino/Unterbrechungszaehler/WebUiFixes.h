#pragma once

#include "WebUiBehavior.h"
#include "WebUiV2.h"

// WebService sendet diesen Baustein bereits zwischen Basis-UI und den weiteren
// Erweiterungen. Die Makroverkettung haelt die bestehende 1.0.1-Reihenfolge
// unveraendert und fuegt die 2.0-UI ohne Kopie der grossen Behavior-Datei an.
#define WEB_UI_FIXES WEB_UI_BEHAVIOR); server_.sendContent_P(WEB_UI_V2
