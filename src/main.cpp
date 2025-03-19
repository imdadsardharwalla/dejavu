#include <filesystem>
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
    root.PrintTree();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
