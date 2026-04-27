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

  const std::vector<std::vector<FileNode*>>& FileDuplicates() const;
  const std::vector<std::vector<DirectoryNode*>>& DirectoryDuplicates() const;

  bool ResultsUpToDate() const { return m_results_valid; }

  std::vector<std::filesystem::path> InputFiles() const;
  std::vector<std::filesystem::path> InputDirectories() const;
  size_t InputFileCount() const { return m_input_files.size(); }
  size_t InputDirectoryCount() const { return m_input_directories.size(); }

private:
  std::vector<std::unique_ptr<FileNode>> m_input_files;
  std::vector<std::unique_ptr<DirectoryNode>> m_input_directories;

  std::vector<std::vector<FileNode*>> m_duplicate_files;
  std::vector<std::vector<DirectoryNode*>> m_duplicate_directories;

  bool m_results_valid = false;
};
} // namespace dejavu
