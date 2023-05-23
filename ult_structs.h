#ifndef NO_INCLUDES
#include "imgui/imgui.h"
#include "ult_memory.h"
#endif

struct directory_contents {
  string Name;
  string Path;
  string* Files;
  uint32 FilesCount;
};

struct subdirectories {
   directory_contents* Directories[MAX_DIRS];
};

struct ult_settings {
  bool Fullscreen;
  bool Compositor;
  bool Dockspace;

  int MonitorIndex;

  bool ShowSettingsWindow;
  bool ShowScriptsWindow;
  bool ShowApplicationsWindow;
  bool ShowDemoWindow;

  ImVec4 BgColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

struct ult_state {
    arena Arena;
    subdirectories* Scripts;
    subdirectories* Applications;

    ult_settings Settings;
};