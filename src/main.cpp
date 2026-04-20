#include <filesystem>
#include <iostream>

#include "DuplicateFinder.h"
#include "FilesystemNode.h"

int main(const int argc, const char* const argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <directory>" << std::endl;
    return 1;
  }

  try
  {
    dejavu::DirectoryNode root(std::filesystem::absolute(argv[1]));
    root.BuildTree();

    auto [directories, files] = root.FlattenTree();

    std::cout << "=== Directories ===" << std::endl;
    for (auto* dir : directories)
    {
      std::cout << dir->GetPath() << "\n"
                << "  size:        " << dir->GetSize() << "\n"
                << "  fingerprint: " << std::hex << dir->GetFingerprint()
                << std::dec << "\n"
                << std::endl;
    }

    std::cout << "=== Files ===" << std::endl;
    for (auto* file : files)
    {
      std::cout << file->GetPath() << "\n"
                << "  size:         " << file->GetSize() << "\n"
                << "  partial hash: " << std::hex << file->GetPartialHash()
                << std::dec << "\n";

      if (file->HasFullHash())
        std::cout << "  full hash:    " << std::hex << file->GetFullHash()
                  << std::dec << "\n";

      std::cout << std::endl;
    }

    auto duplicate_groups = dejavu::FindDuplicateFiles(files);

    std::cout << "=== Duplicate Files ===" << std::endl;
    if (duplicate_groups.empty())
    {
      std::cout << "No duplicates found." << std::endl;
    }
    else
    {
      for (size_t i = 0; i < duplicate_groups.size(); ++i)
      {
        std::cout << "Group " << (i + 1) << ":" << std::endl;
        for (auto* file : duplicate_groups[i])
          std::cout << "  " << file->GetPath() << " (" << file->GetSize()
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
