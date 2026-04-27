#include "FilesystemNode.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <print>
#include <string>

#include "xxhash_cpp/xxhash.hpp"

namespace dejavu
{

constexpr size_t kPartialHashSize = 4096;
constexpr size_t kReadChunkSize = 65536;

FilesystemNode::FilesystemNode(
    const std::filesystem::path& path, DirectoryNode* parent)
    : m_path(path), m_size(kInvalidSize), m_parent(parent)
{
  m_path.make_preferred();
}

void FilesystemNode::PrintNode(const int indent) const
{
  const std::string indent_str(indent, ' ');
  std::print("{} {} ", indent_str, m_path.filename().string());

  if (Size() != kInvalidSize)
    std::print(" ({})", m_size);
  else
    std::print(" (size unknown)");

  std::println();
}

FileNode::FileNode(const std::filesystem::path& path, DirectoryNode* parent)
    : FilesystemNode(path, parent)
{
  if (!std::filesystem::is_regular_file(m_path))
  {
    throw std::invalid_argument("Path is not a regular file");
  }
}

void FileNode::BuildTree() { m_size = std::filesystem::file_size(m_path); }

void FileNode::PrintTree(const int indent) const { PrintNode(indent); }

// Read the first kPartialHashSize bytes of the file and compute the partial
// hash. If the file is smaller than kPartialHashSize, we can also set the full
// hash.
uint64_t FileNode::PartialHash()
{
  if (!m_partial_hash.has_value())
  {
    std::ifstream file(m_path, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open file: " + m_path.string());

    std::array<char, kPartialHashSize> buffer{};
    file.read(buffer.data(), kPartialHashSize);
    const auto bytes_read = static_cast<size_t>(file.gcount());

    m_partial_hash = xxh::xxhash<64>(buffer.data(), bytes_read);

    // If we've read the entire file, we can also set the full hash.
    if (bytes_read < kPartialHashSize)
      m_full_hash = m_partial_hash.value();
  }
  return m_partial_hash.value();
}

// Read the entire file in chunks of kReadChunkSize bytes and compute the full
// hash.
uint64_t FileNode::FullHash()
{
  if (!m_full_hash.has_value())
  {
    std::ifstream file(m_path, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open file: " + m_path.string());

    xxh::hash_state_t<64> hash_state;
    std::array<char, kReadChunkSize> buffer{};

    while (file.read(buffer.data(), kReadChunkSize) || file.gcount() > 0)
    {
      hash_state.update(buffer.data(), static_cast<size_t>(file.gcount()));
      if (file.eof())
        break;
    }

    m_full_hash = hash_state.digest();
  }
  return m_full_hash.value();
}

DirectoryNode::DirectoryNode(
    const std::filesystem::path& path, DirectoryNode* parent)
    : FilesystemNode(CleanPath(path), parent)
{
  if (!std::filesystem::is_directory(m_path))
  {
    throw std::invalid_argument("Path is not a directory");
  }
}

void DirectoryNode::BuildTree()
{
  m_size = 0;
  for (const auto& entry : std::filesystem::directory_iterator(m_path))
  {
    if (entry.is_directory())
      m_size += AddChildNode(m_child_directories, entry.path());
    else if (entry.is_regular_file())
      m_size += AddChildNode(m_child_files, entry.path());
    else
    {
      std::println(stderr,
          "Skipping {} because it is not a directory or a regular file",
          entry.path().string());
    }
  }

  // Sort children by name to ensure a consistent ordering
  std::ranges::sort(m_child_directories, [](const auto& a, const auto& b)
      { return a->Path().filename() < b->Path().filename(); });
  std::ranges::sort(m_child_files, [](const auto& a, const auto& b)
      { return a->Path().filename() < b->Path().filename(); });
}

void DirectoryNode::PrintTree(const int indent) const
{
  PrintNode(indent);

  for (const auto& child_directory : m_child_directories)
    child_directory->PrintTree(indent + 2);

  for (const auto& child_file : m_child_files)
    child_file->PrintTree(indent + 2);
}

void DirectoryNode::FlattenTree(
    std::vector<DirectoryNode*>& directories, std::vector<FileNode*>& files)
{
  directories.push_back(this);

  for (auto& child_directory : m_child_directories)
    child_directory->FlattenTree(directories, files);

  for (auto& child_file : m_child_files)
    files.push_back(child_file.get());
}

// Build a fingerprint of the directory from its children's names and sizes,
// along with child directory fingerprints and child file hashes (full if
// available, otherwise partial). File names are included because two
// directories are only considered duplicates if they contain the same children
// with the same names, unlike files, where only content matters.
uint64_t DirectoryNode::Fingerprint()
{
  if (!m_fingerprint.has_value())
  {
    xxh::hash_state_t<64> hash_state;

    for (auto& child_directory : m_child_directories)
    {
      // Include child directory name
      hash_state.update(child_directory->Path().filename().string());

      // Include child directory fingerprint
      const auto fingerprint = child_directory->Fingerprint();
      hash_state.update(&fingerprint, sizeof(fingerprint));

      // Include child directory size
      const auto size = child_directory->Size();
      assert(size != kInvalidSize);
      hash_state.update(&size, sizeof(size));
    }

    for (auto& child_file : m_child_files)
    {
      // Include child file name
      hash_state.update(child_file->Path().filename().string());

      // If the child file has a full hash, include it. Otherwise, include the
      // partial hash.
      const auto hash = child_file->HasFullHash() ? child_file->FullHash()
                                                  : child_file->PartialHash();
      hash_state.update(&hash, sizeof(hash));

      // Include child file size
      const auto size = child_file->Size();
      assert(size != kInvalidSize);
      hash_state.update(&size, sizeof(size));
    }

    // Include size of the directory
    hash_state.update(&m_size, sizeof(m_size));

    m_fingerprint = hash_state.digest();
  }

  return m_fingerprint.value();
}

template <typename NodeT>
uintmax_t DirectoryNode::AddChildNode(
    std::vector<std::unique_ptr<NodeT>>& child_nodes,
    const std::filesystem::path& path)
{
  auto& child_node =
      child_nodes.emplace_back(std::make_unique<NodeT>(path, this));
  child_node->BuildTree();

  const auto size = child_node->Size();
  assert(size != kInvalidSize);
  return size;
}

std::filesystem::path DirectoryNode::CleanPath(
    const std::filesystem::path& path)
{
  return (path / "").parent_path();
}

} // namespace dejavu
