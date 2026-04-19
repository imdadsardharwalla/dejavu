#include <filesystem>
#include <iomanip>
#include <iostream>

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
    DirectoryNode root(std::filesystem::absolute(argv[1]));
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
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
