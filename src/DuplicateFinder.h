#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace dejavu
{
// Forward declarations
class FileNode;
class DirectoryNode;

class DuplicateFinder
{
public:
  void Add(const std::filesystem::path& path);

  void ComputeDuplicates();

  std::vector<std::vector<FileNode*>> GetDuplicateFiles();
  std::vector<std::vector<DirectoryNode*>> GetDuplicateDirectories();

private:
  std::vector<std::unique_ptr<FileNode>> m_input_files;
  std::vector<std::unique_ptr<DirectoryNode>> m_input_directories;

  std::vector<std::vector<FileNode*>> m_duplicate_files;
  std::vector<std::vector<DirectoryNode*>> m_duplicate_directories;

  bool m_duplicates_computed = false;
};
} // namespace dejavu
