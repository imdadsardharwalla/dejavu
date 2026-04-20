#pragma once

#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace dejavu
{

const auto INVALID_SIZE = std::numeric_limits<std::uintmax_t>::max();

class DirectoryNode;
class FilesystemNode
{
public:
  FilesystemNode(const std::filesystem::path& path, DirectoryNode* parent);

  virtual void BuildTree() = 0;
  virtual void PrintTree(const int indent = 0) const = 0;

  const std::filesystem::path& GetPath() const { return m_path; }
  uintmax_t GetSize() const { return m_size; }
  DirectoryNode* GetParent() const { return m_parent; }

protected:
  void PrintNode(const int indent) const;

  std::filesystem::path m_path;
  std::uintmax_t m_size = INVALID_SIZE;
  DirectoryNode* m_parent = nullptr;
};

class FileNode : public FilesystemNode
{
public:
  FileNode(const std::filesystem::path& path, DirectoryNode* parent = nullptr);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;

  uint64_t GetPartialHash();
  bool HasPartialHash() const { return m_partial_hash.has_value(); }

  uint64_t GetFullHash();
  bool HasFullHash() const { return m_full_hash.has_value(); }

private:
  std::optional<uint64_t> m_partial_hash = std::nullopt;
  std::optional<uint64_t> m_full_hash = std::nullopt;
};

class DirectoryNode : public FilesystemNode
{
public:
  DirectoryNode(
      const std::filesystem::path& path, DirectoryNode* parent = nullptr);

  void BuildTree() override;
  void PrintTree(const int indent = 0) const override;
  std::pair<std::vector<DirectoryNode*>, std::vector<FileNode*>> FlattenTree();

  uint64_t GetFingerprint();

private:
  std::vector<std::unique_ptr<DirectoryNode>> m_child_directories;
  std::vector<std::unique_ptr<FileNode>> m_child_files;

  std::optional<uint64_t> m_fingerprint = std::nullopt;

  void FlattenChildren(std::vector<DirectoryNode*>& child_directories,
      std::vector<FileNode*>& child_files) const;

  template <typename T>
  uintmax_t AddChildNode(std::vector<std::unique_ptr<T>>& child_nodes,
      const std::filesystem::path& path);

  static std::filesystem::path CleanPath(const std::filesystem::path& path);
};

} // namespace dejavu
