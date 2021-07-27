#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#if !defined(DEBUG) && !defined(_DEBUG) || defined(NDEBUG)
#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_METRICS_WINDOW
#endif
