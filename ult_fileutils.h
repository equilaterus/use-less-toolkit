#ifndef NO_INCLUDES
#include <linux/limits.h>
#include "ult_common.h"
#include <cstring>
#include <dirent.h> 
#include <iterator>
#include <stdio.h>
#include "ult_structs.h"
#endif

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
  return Count;
}


//
// Application / Scripts config
//

ult_group*
fileutils_GetDirectoryContents(char Path[PATH_MAX], char Name[NAME_MAX], arena* Arena, ult_run_mode RunMode)
{
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    return 0;
  }

  ult_group* Group = (ult_group*)ReserveMemory(Arena, sizeof(ult_group));
  Group->GroupTitle = AllocateString(Arena, Name);
  Group->EntriesCount = fileutils_getDirectoryFileCount(Path);
  Group->Entries = (ult_entry*)ReserveMemory(Arena, sizeof(ult_entry) * Group->EntriesCount);

  struct dirent * DirEnt;
  ult_entry* BaseAddress = Group->Entries;
  while ((DirEnt = readdir(DirStream)) != 0) {
      if(DirEnt->d_type != DT_DIR) {
        char FullPath[PATH_MAX];
        sprintf(FullPath, "%s/%s", Path, DirEnt->d_name); 

        BaseAddress->EntryTitle = AllocateString(Arena, DirEnt->d_name);
        BaseAddress->Path = AllocateString(Arena, FullPath);
        BaseAddress->RunMode = RunMode;
        ++BaseAddress;
      }
  }
  closedir(DirStream);
  return Group;
}


function ult_config*
fileutils_ExploreSubDirectoriesForConfig(const char* Path, arena* Arena, ult_run_mode RunMode)
{
  DIR* DirStream = opendir(Path);
  if (DirStream == 0) {
    return 0;
  }

  ult_config* Subdirectories = (ult_config*)ReserveMemory(Arena, sizeof(ult_config));
  struct dirent* DirEnt;
  int Index = 0;
  while ((DirEnt= readdir(DirStream)) != 0) {
      if(DirEnt->d_type == DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) {
        if (strcmp(DirEnt->d_name, "custom") == 0) 
          continue;
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);
        Subdirectories->Groups[Index] = fileutils_GetDirectoryContents(DirPath, DirEnt->d_name, Arena, RunMode);
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
fileutils_ParseCustomConfigFile(char Path[PATH_MAX], arena* Arena) {
  fprintf(stdout, "Found config file %s\n", Path);
  FILE *FileStream;
  FileStream = fopen(Path,  "r");
  if(FileStream == 0) {
    fprintf(stderr, "Error opening file %s\n", Path);
    exit(1);
  }
  ult_group *Group = (ult_group*)ReserveMemory(Arena, sizeof(ult_group));
  
  // Read file
  char *FileContents = "";
  char *Buffer = 0;
  size_t Dummy = 0;
  while (getline(&Buffer, &Dummy, FileStream) != -1) {
    if (Buffer[0] == '[') {
      ++Group->EntriesCount;
    }

    if (asprintf(&FileContents, "%s%s", FileContents, Buffer) == -1) {
      fprintf(stderr, "Error reading file! asprintf failed\n");
      exit(1);
    }
  }
  free(Buffer);
  fclose(FileStream);
  
  // Parse contents
  fprintf(stdout, "File loaded, trying to parse it...\n");
  Group->Entries = (ult_entry*)ReserveMemory(Arena, sizeof(ult_entry) * Group->EntriesCount);
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
          Group->GroupTitle = AllocateString(Arena, Value);
        }
        else {
          if (strcmp(Property, "EntryTitle") == 0) {
          Group->Entries[CurrentIndex].EntryTitle = AllocateString(Arena, Value);
          } 
          else if (strcmp(Property, "Path") == 0) {
            Group->Entries[CurrentIndex].Path = AllocateString(Arena, Value);
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

  Config->ConfigTitle = AllocateString(&State->Arena, Name);

  dirent *DirEnt;
  int Index = 0;
  while ((DirEnt = readdir(DirStream)) != 0) {
      if (DirEnt->d_type != DT_DIR && strcmp(DirEnt->d_name,".") != 0 && strcmp(DirEnt->d_name,"..") != 0) {
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);
        Config->Groups[Index] = fileutils_ParseCustomConfigFile(DirPath, &State->Arena);
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

  fprintf(stdout, "Custom directories %d\n", fileutils_getDirectoryFileCount(Path));
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