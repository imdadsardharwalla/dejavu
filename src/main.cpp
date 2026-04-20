#include <filesystem>
#include <iostream>

#include "DuplicateFinder.h"
#include "FilesystemNode.h"

int main(const int argc, const char* const argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <path> [<path> ...]" << std::endl;
    return 1;
  }

  try
  {
    dejavu::DuplicateFinder finder;
    for (int i = 1; i < argc; ++i)
      finder.Add(std::filesystem::absolute(argv[i]));

    auto duplicate_file_groups = finder.GetDuplicateFiles();

    std::cout << "=== Duplicate Files ===" << std::endl;
    if (duplicate_file_groups.empty())
    {
      std::cout << "No duplicates found." << std::endl;
    }
    else
    {
      for (size_t i = 0; i < duplicate_file_groups.size(); ++i)
      {
        std::cout << "Group " << (i + 1) << ":" << std::endl;
        for (auto* file : duplicate_file_groups[i])
          std::cout << "  " << file->GetPath() << " (" << file->GetSize()
                    << " bytes)" << std::endl;
        std::cout << std::endl;
      }
    }

    auto duplicate_dir_groups = finder.GetDuplicateDirectories();

    std::cout << "=== Duplicate Directories ===" << std::endl;
    if (duplicate_dir_groups.empty())
    {
      std::cout << "No duplicates found." << std::endl;
    }
    else
    {
      for (size_t i = 0; i < duplicate_dir_groups.size(); ++i)
      {
        std::cout << "Group " << (i + 1) << ":" << std::endl;
        for (auto* dir : duplicate_dir_groups[i])
          std::cout << "  " << dir->GetPath() << " (" << dir->GetSize()
                    << " bytes)" << std::endl;
        std::cout << std::endl;
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
