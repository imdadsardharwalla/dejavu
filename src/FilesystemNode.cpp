#include "FilesystemNode.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>

#include "xxhash_cpp/xxhash.hpp"

constexpr size_t PARTIAL_HASH_SIZE = 4096;
constexpr size_t READ_CHUNK_SIZE = 65536;

FilesystemNode::FilesystemNode(
    const std::filesystem::path& path, DirectoryNode* parent)
    : m_path(path), m_size(INVALID_SIZE), m_parent(parent)
{
}

void FilesystemNode::PrintNode(const int indent) const
{
  std::string indent_str(indent, ' ');
  std::cout << indent_str << m_path.filename();

  if (GetSize() != INVALID_SIZE)
    std::cout << " (" << m_size << " bytes)";
  else
    std::cout << " (size unknown)";

  std::cout << std::endl;
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

// Read the first PARTIAL_HASH_SIZE bytes of the file and compute the partial
// hash. If the file is smaller than PARTIAL_HASH_SIZE, we can also set the full
// hash.
uint64_t FileNode::GetPartialHash()
{
  if (!m_partial_hash.has_value())
  {
    std::ifstream file(m_path, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open file: " + m_path.string());

    std::array<char, PARTIAL_HASH_SIZE> buffer{};
    file.read(buffer.data(), PARTIAL_HASH_SIZE);
    const auto bytes_read = static_cast<size_t>(file.gcount());

    m_partial_hash = xxh::xxhash<64>(buffer.data(), bytes_read);

    // If we've read the entire file, we can also set the full hash.
    if (bytes_read < PARTIAL_HASH_SIZE)
      m_full_hash = m_partial_hash.value();
  }
  return m_partial_hash.value();
}

// Read the entire file in chunks of READ_CHUNK_SIZE bytes and compute the full
// hash.
uint64_t FileNode::GetFullHash()
{
  if (!m_full_hash.has_value())
  {
    std::ifstream file(m_path, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open file: " + m_path.string());

    xxh::hash_state_t<64> hash_state;
    std::array<char, READ_CHUNK_SIZE> buffer{};

    while (file.read(buffer.data(), READ_CHUNK_SIZE) || file.gcount() > 0)
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
      std::cerr << "Skipping " << entry.path()
                << " because it is not a directory or a regular file"
                << std::endl;
    }
  }

  // Sort children by name to ensure a consistent ordering
  std::ranges::sort(m_child_directories, [](const auto& a, const auto& b)
      { return a->GetPath().filename() < b->GetPath().filename(); });
  std::ranges::sort(m_child_files, [](const auto& a, const auto& b)
      { return a->GetPath().filename() < b->GetPath().filename(); });
}

void DirectoryNode::PrintTree(const int indent) const
{
  PrintNode(indent);

  for (const auto& child_directory : m_child_directories)
    child_directory->PrintTree(indent + 2);

  for (const auto& child_file : m_child_files)
    child_file->PrintTree(indent + 2);

  if (indent == 0)
    std::cout << std::flush;
}

// Build a fingerprint of the directory from its children's names and sizes,
// along with child directory fingerprints and child file hashes (full if
// available, otherwise partial). File names are included because two
// directories are only considered duplicates if they contain the same children
// with the same names, unlike files, where only content matters.
uint64_t DirectoryNode::GetFingerprint()
{
  if (!m_fingerprint.has_value())
  {
    xxh::hash_state_t<64> hash_state;

    for (auto& child_directory : m_child_directories)
    {
      // Child directory name
      hash_state.update(child_directory->GetPath().filename().string());

      // Child directory fingerprint
      const auto fingerprint = child_directory->GetFingerprint();
      hash_state.update(&fingerprint, sizeof(fingerprint));

      // Child directory size
      const auto size = child_directory->GetSize();
      assert(size != INVALID_SIZE);
      hash_state.update(&size, sizeof(size));
    }

    for (auto& child_file : m_child_files)
    {
      // Child file name
      hash_state.update(child_file->GetPath().filename().string());

      // If the child file has a full hash, add it. Otherwise, add the partial
      // hash.
      const auto hash = child_file->HasFullHash()
                            ? child_file->GetFullHash()
                            : child_file->GetPartialHash();
      hash_state.update(&hash, sizeof(hash));

      // Child file size
      const auto size = child_file->GetSize();
      assert(size != INVALID_SIZE);
      hash_state.update(&size, sizeof(size));
    }

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

  const auto size = child_node->GetSize();
  assert(size != INVALID_SIZE);
  return size;
}

std::filesystem::path DirectoryNode::CleanPath(
    const std::filesystem::path& path)
{
  return (path / "").parent_path();
}
