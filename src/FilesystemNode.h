#pragma once

#include <filesystem>
#include <limits>
#include <vector>

const auto INVALID_SIZE = std::numeric_limits<std::uintmax_t>::max();

class FilesystemNode
{
public:
  FilesystemNode(const std::filesystem::path& path);

  virtual void BuildTree() = 0;
  virtual void PrintTree(const int indent = 0) const = 0;

  const std::filesystem::path& GetPath() const { return m_path; }
  const uintmax_t GetSize() const { return m_size; }

protected:
  std::filesystem::path m_path;
  std::uintmax_t m_size = INVALID_SIZE;
};

class FileNode : public FilesystemNode
{
public:
  FileNode(const std::filesystem::path& path);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;

private:
  // add parent
};

class DirectoryNode : public FilesystemNode
{
public:
  DirectoryNode(const std::filesystem::path& path);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;

private:
  std::vector<DirectoryNode> m_child_directories;
  std::vector<FileNode> m_child_files;

  std::filesystem::path CleanPath(const std::filesystem::path& path);
};
