#ifndef NO_INCLUDES
#include "imgui/imgui.h"
#include "ult_memory.h"
#endif

enum ult_run_mode {
  ult_rm_Browse,
  ult_rm_Script,
  ult_rm_Application
};

struct ult_entry {
  string EntryTitle;
  string Path;
  ult_run_mode RunMode;
};

struct ult_group {
  string GroupTitle;
  ult_entry* Entries;
  uint32 EntriesCount;
};

// todo(dacanizares): Groups as vector and add show window bool
struct ult_config {
  string ConfigTitle;
  ult_group* Groups[MAX_ENTRIES];
};

// todo(dacanizares): define persitance props.
struct ult_settings {
  bool Fullscreen = 0;
  bool Compositor = 0;
  bool Dockspace = 1;

  int MonitorIndex = 0;

  bool ShowWindow[MAX_ENTRIES] = {[0 ... MAX_ENTRIES-1] = 1};

  bool ForceTitleUpperCase = 1;
  bool RemoveApplicationExt = 1;
  bool UnderscoresToSpaces = 1;

  ImVec4 BgColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

struct ult_state {
  arena Arena;
  ult_settings Settings;
  ult_config* ScriptsConfig;
  ult_config* ApplicationsConfig;

  ult_config* CustomConfigs;
  uint32 CustomConfigsCount;  
};

void state_ShowAllWindows(ult_state *State) {
  bool *Current = State->Settings.ShowWindow;
  for (int i = 0; i < MAX_ENTRIES; ++i) {
    *Current = true;
    ++Current;
  }
}