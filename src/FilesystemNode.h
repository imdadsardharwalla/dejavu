#pragma once

#include <filesystem>
#include <limits>
#include <vector>

const auto INVALID_SIZE = std::numeric_limits<std::uintmax_t>::max();

class DirectoryNode;
class FilesystemNode
{
public:
  FilesystemNode(const std::filesystem::path& path, DirectoryNode* parent);

  virtual void BuildTree() = 0;
  virtual void PrintTree(const int indent = 0) const = 0;

  const std::filesystem::path& GetPath() const { return m_path; }
  const uintmax_t GetSize() const { return m_size; }
  DirectoryNode* GetParent() const { return m_parent; }

protected:
  void PrintNode(const int indent) const;

  std::filesystem::path m_path;
  std::uintmax_t m_size = INVALID_SIZE;
  DirectoryNode* m_parent = nullptr;
  std::filesystem::path m_parent_path;
};

class FileNode : public FilesystemNode
{
public:
  FileNode(const std::filesystem::path& path, DirectoryNode* parent = nullptr);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;

private:
};

class DirectoryNode : public FilesystemNode
{
public:
  DirectoryNode(
      const std::filesystem::path& path, DirectoryNode* parent = nullptr);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;

private:
  std::vector<DirectoryNode> m_child_directories;
  std::vector<FileNode> m_child_files;

  template <typename T>
  uintmax_t AddChildNode(
      std::vector<T>& child_nodes, const std::filesystem::path& path);

  static std::filesystem::path CleanPath(const std::filesystem::path& path);
};
