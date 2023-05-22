#ifndef NO_INCLUDES
#include <cstring>
#include <dirent.h> 
#include <iterator>
#include <stdio.h>
#include "ult_globals.h"
#include "ult_common.h"
#include "ult_memory.h"
#endif

const int PATH_SIZE = 256;
const int MAX_DIRS = 256;

struct directory_contents {
  string Name;
  string Path;
  string* Files;
  uint32 FilesCount;
};

struct subdirectories {
   directory_contents* Directories[MAX_DIRS];
};

function int
fileutils_getDirectoryFileCount(char Path[PATH_SIZE])
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


directory_contents*
fileutils_getDirectoryContents(char Path[PATH_SIZE], char Name[PATH_SIZE], arena* Arena)
{
  DIR* D = opendir(Path);
  if (D == 0) {
    return 0;
  }

  directory_contents* Directory = (directory_contents*)ReserveMemory(Arena, sizeof(directory_contents));
  Directory->Name = AllocateString(Arena, Name);
  Directory->Path = AllocateString(Arena, Path);
  Directory->FilesCount = fileutils_getDirectoryFileCount(Path);
  Directory->Files = (string*)ReserveMemory(Arena, sizeof(string) * Directory->FilesCount);

  struct dirent * dir;
  string* BaseAddress = Directory->Files;
  while ((dir = readdir(D)) != 0) {
      if(dir->d_type != DT_DIR) {
        *BaseAddress = AllocateString(Arena, dir->d_name);
        ++BaseAddress;
      }
  }
  return Directory;
}

function subdirectories*
fileutils_exploreSubDirectoriesForFiles(char Path[PATH_SIZE], arena* Arena)
{
  DIR* D = opendir(Path);
  if (D == 0) {
    return 0;
  }

  subdirectories* Subdirectories = (subdirectories*)ReserveMemory(Arena, sizeof(subdirectories));
  struct dirent * dir;
  int Index = 0;
  while ((dir = readdir(D)) != 0) {
      if(dir->d_type == DT_DIR && strcmp(dir->d_name,".") != 0 && strcmp(dir->d_name,"..") != 0) {
        char d_path[255];
        sprintf(d_path, "%s/%s", Path, dir->d_name);
        Subdirectories->Directories[Index] = fileutils_getDirectoryContents(d_path, dir->d_name, Arena);
        ++Index;
      }
  }
  closedir(D);
  return Subdirectories;
}