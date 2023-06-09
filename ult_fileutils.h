#ifndef NO_INCLUDES
#include <linux/limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <dirent.h> 
#include <iterator>
#include <stdio.h>
#include "ult_structs.h"
#endif

//
// General file utils
//

inline function string
AllocateCapitalizedString(arena *Arena, char *Data)
{
    char Capitalized[strlen(Data)];
    sprintf(Capitalized, "%c%s", toupper(Data[0]), Data + 1);
    return AllocateString(Arena, Capitalized);
}

inline function string
AllocateStringApplyingSettings(ult_state *State, char *Data, bool IsFile=0)
{
  if (IsFile && State->Settings.RemoveFilesExt) {
    RemoveExtensionInLine(Data);
  }
  if (State->Settings.UnderscoresToSpaces) {
    ReplaceCharInLine(Data, '_', ' ');
  }
  if (State->Settings.ForceTitleUpperCase) {    
    return AllocateCapitalizedString(&State->Arena, Data);
  }
  else {
    return AllocateString(&State->Arena, Data);
  }
}

function int
fileutils_getDirectoryFileCount(const char *Path)
{
  DIR* D = opendir(Path);
  if (D == 0) {
    return 0;
  }

  struct dirent * dir;
  int count = 0;
  while ((dir = readdir(D)) != 0) {
      if(dir->d_type != DT_DIR) 
        count++;
  }
  closedir(D);
  return count;
}

function int
fileutils_getDirectorySubdirectoriesCount(const char *Path)
{
  DIR* D = opendir(Path);
  if (D == 0) {
    return 0;
  }

  struct dirent * DirEnt;
  int Count = 0;
  while ((DirEnt = readdir(D)) != 0) {
      if(DirEnt->d_type == DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) 
        Count++;
  }
  closedir(D);
  return Count;
}


//
// Application / Scripts config
//

ult_group*
fileutils_GetDirectoryContents(char Path[PATH_MAX], char Name[NAME_MAX], ult_state *State, ult_run_mode RunMode)
{
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    return 0;
  }

  ult_group* Group = (ult_group*)ReserveMemory(&State->Arena, sizeof(ult_group));
  Group->GroupTitle = AllocateStringApplyingSettings(State, Name);
  Group->EntriesCount = fileutils_getDirectoryFileCount(Path);
  Group->Entries = (ult_entry*)ReserveMemory(&State->Arena, sizeof(ult_entry) * Group->EntriesCount);

  struct dirent * DirEnt;
  ult_entry* BaseAddress = Group->Entries;
  while ((DirEnt = readdir(DirStream)) != 0) {
      if(DirEnt->d_type != DT_DIR) {
        char FullPath[PATH_MAX];
        sprintf(FullPath, "%s/%s", Path, DirEnt->d_name); 

        BaseAddress->EntryTitle = AllocateStringApplyingSettings(State, DirEnt->d_name, 1);
        BaseAddress->Path = AllocateString(&State->Arena, FullPath);
        BaseAddress->RunMode = RunMode;
        ++BaseAddress;
      }
  }
  closedir(DirStream);
  return Group;
}


function ult_config*
fileutils_ExploreSubDirectoriesForConfig(const char* Path, ult_state *State, ult_run_mode RunMode)
{
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    return 0;
  }

  ult_config* Subdirectories = (ult_config*)ReserveMemory(&State->Arena, sizeof(ult_config));
  struct dirent* DirEnt;
  int Index = 0;
  while ((DirEnt= readdir(DirStream)) != 0) {
      if(DirEnt->d_type == DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) {
        if (strcmp(DirEnt->d_name, "custom") == 0) 
          continue;
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);
        Subdirectories->Groups[Index] = fileutils_GetDirectoryContents(DirPath, DirEnt->d_name, State, RunMode);
        ++Index;
      }
  }
  closedir(DirStream);
  return Subdirectories;
}

//
// Custom configs
//

function ult_group*
fileutils_ParseCustomConfigFile(char Path[PATH_MAX], ult_state *State) {
  fprintf(stdout, "Found config file %s\n", Path);
  FILE *FileStream;
  FileStream = fopen(Path,  "r");
  if(FileStream == 0) {
    fprintf(stderr, "Error opening file %s\n", Path);
    exit(1);
  }
  ult_group *Group = (ult_group*)ReserveMemory(&State->Arena, sizeof(ult_group));
  
  // Read file
  struct stat FileStat;
  stat(Path, &FileStat);

  char *FileContents = (char*)calloc(1, FileStat.st_size);
  char *Buffer = 0;
  size_t Dummy = 0;
  while (getline(&Buffer, &Dummy, FileStream) != -1) {
    if (Buffer[0] == '[') {
      ++Group->EntriesCount;
    }
    strcat(FileContents, Buffer);
  }
  free(Buffer);
  fclose(FileStream);
  
  // Parse contents
  fprintf(stdout, "File loaded, trying to parse it...\n");
  Group->Entries = (ult_entry*)ReserveMemory(&State->Arena, sizeof(ult_entry) * Group->EntriesCount);
  int CurrentIndex = -1;
  char *CurrentLine = FileContents;
  while (CurrentLine) {
    char *NextLine = strchr(CurrentLine, '\n');
    if (NextLine) *NextLine = '\0';
    
    // Parse line
    if (CurrentLine[0] == '[') {
      ++CurrentIndex;
    } else {
      char *LineValue = strchr(CurrentLine, '=');
      if (LineValue) *LineValue = '\0';
      char *Property = TrimInLine(CurrentLine);
      char *Value = TrimInLine(++LineValue);
      if (Property && Value) {
        if (CurrentIndex == -1) {
          Group->GroupTitle = AllocateStringApplyingSettings(State, Value);
        }
        else {
          if (strcmp(Property, "EntryTitle") == 0) {
            Group->Entries[CurrentIndex].EntryTitle = AllocateStringApplyingSettings(State, Value);
          } 
          else if (strcmp(Property, "Path") == 0) {
            Group->Entries[CurrentIndex].Path = AllocateString(&State->Arena, Value);
          } 
          else if (strcmp(Property, "RunMode") == 0) {
            Group->Entries[CurrentIndex].RunMode = (ult_run_mode) atoi(Value);
          }
        }
      }
    }
    CurrentLine = NextLine ? (NextLine + 1) : 0;
  }
  fprintf(stdout, "File parsed successfully.\n");
  free(FileContents);
  return Group;
}


function void
fileutils_ExploreCustomSubdirectory(char Path[PATH_MAX], char Name[NAME_MAX], ult_state *State, ult_config *Config)
{
  fprintf(stdout, "Found directory %s [%s]\n", Name, Path);
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    fprintf(stdout, "Error opening directory\n");
    exit(1);
  }

  Config->ConfigTitle = AllocateStringApplyingSettings(State, Name);

  dirent *DirEnt;
  int Index = 0;
  while ((DirEnt = readdir(DirStream)) != 0) {
      if (DirEnt->d_type != DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) {
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);
        
        Config->Groups[Index] = fileutils_ParseCustomConfigFile(DirPath, State);
        ++Index;
      }
  }
  closedir(DirStream);
}


function void
fileutils_ExploreCustomDirectory(const char *Path, ult_state *State)
{
  fprintf(stdout, "Start process: Exploring custom directory\n");
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    fprintf(stderr, "Error opening custom directory\n");
    exit(1);
  }

  fprintf(stdout, "Custom directories %d\n", fileutils_getDirectorySubdirectoriesCount(Path));
  State->CustomConfigsCount = fileutils_getDirectorySubdirectoriesCount(Path);
  State->CustomConfigs = (ult_config*)ReserveMemory(&State->Arena, sizeof(ult_config) * State->CustomConfigsCount);
  
  struct dirent* DirEnt;
  int Index = 0;
  while ((DirEnt= readdir(DirStream)) != 0) {
      if(DirEnt->d_type == DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) {
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);

        fileutils_ExploreCustomSubdirectory(DirPath, DirEnt->d_name, State, &State->CustomConfigs[Index]);
        ++Index;
      }
  }
  closedir(DirStream);
  fprintf(stdout, "End process: Exploring custom directory\n");
}


//
// Settings
//
function void
fileutils_LoadSettings(ult_settings *Settings)
{
  fprintf(stdout, "Start process: loading settings.\n");
  FILE *fptr;
  if ((fptr = fopen(SETTINGS_FILE,"rb")) == NULL){
      fprintf(stderr, "Warning: could not open settings file.\n");
      return;
  }

  fread(Settings, sizeof(ult_settings), 1, fptr);
  fclose(fptr);
  fprintf(stdout, "Success: loading settings.\n");
}

function void
fileutils_SaveSettings(ult_settings *Settings)
{
  fprintf(stdout, "Start process: saving settings.\n");
  FILE *fptr;
  if ((fptr = fopen(SETTINGS_FILE,"wb")) == NULL){
      fprintf(stderr, "Error: could not save settings file.\n");
      exit(1);
  }

  fwrite(Settings, sizeof(ult_settings), 1, fptr);
  fclose(fptr);
  fprintf(stdout, "Success: saving settings.\n");
}