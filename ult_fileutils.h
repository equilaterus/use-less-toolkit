#include <linux/limits.h>
#ifndef NO_INCLUDES
#include <cstring>
#include <dirent.h> 
#include <iterator>
#include <stdio.h>
#include "ult_structs.h"
#endif

function int
fileutils_getDirectoryFileCount(char Path[PATH_MAX])
{
  DIR* D = opendir(Path);
  if (D == 0) {
    return 0;
  }

  struct dirent * dir;
  int count = 0;
  while ((dir = readdir(D)) != 0) {
      if(dir->d_type != DT_DIR) count++;
  }
  return count;
}


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
        char DirPath[PATH_MAX];
        sprintf(DirPath, "%s/%s", Path, DirEnt->d_name);
        Subdirectories->Groups[Index] = fileutils_GetDirectoryContents(DirPath, DirEnt->d_name, Arena, RunMode);
        ++Index;
      }
  }
  closedir(DirStream);
  return Subdirectories;
}