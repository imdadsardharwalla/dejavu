#include "DuplicateFinder.h"

#include "FilesystemNode.h"

#include <cstdint>
#include <ranges>
#include <unordered_map>

namespace dejavu
{

namespace
{
template <typename KeyT, typename NodeT>
using NodeGroupMap = std::unordered_map<KeyT, std::vector<NodeT*>>;

template <typename KeyT> using FileNodeGroupMap = NodeGroupMap<KeyT, FileNode>;

template <typename KeyT>
using DirectoryNodeGroupMap = NodeGroupMap<KeyT, DirectoryNode>;

// Filters a NodeGroupMap to only include groups with 2 or more items.
const auto view_duplicates = std::views::filter([](const auto& pair)
                                 { return pair.second.size() >= 2; }) |
                             std::views::values;
} // namespace

std::vector<std::vector<FileNode*>> FindDuplicateFiles(
    std::vector<FileNode*>& files)
{
  // Tier 1: group files by size.
  FileNodeGroupMap<std::uintmax_t> size_map;
  for (auto* file : files)
    size_map[file->GetSize()].push_back(file);

  // Tier 2: group non-unique files by partial hash.
  FileNodeGroupMap<uint64_t> partial_hash_map;
  for (const auto& group : size_map | view_duplicates)
  {
    for (auto* file : group)
      partial_hash_map[file->GetPartialHash()].push_back(file);
  }

  // Tier 3: group non-unique files by their full hash.
  FileNodeGroupMap<uint64_t> full_hash_map;
  for (const auto& group : partial_hash_map | view_duplicates)
  {
    for (auto* file : group)
      full_hash_map[file->GetFullHash()].push_back(file);
  }

  // Return the groups of duplicate files.
  std::vector<std::vector<FileNode*>> duplicates;
  for (const auto& group : full_hash_map | view_duplicates)
    duplicates.push_back(group);

  return duplicates;
}

std::vector<std::vector<DirectoryNode*>> FindDuplicateDirectories(
    std::vector<DirectoryNode*>& directories)
{
  // Tier 1: group directories by size.
  DirectoryNodeGroupMap<std::uintmax_t> size_map;
  for (auto* directory : directories)
    size_map[directory->GetSize()].push_back(directory);

  // Tier 2: group non-unique directories by fingerprint.
  DirectoryNodeGroupMap<uint64_t> fingerprint_map;
  for (const auto& group : size_map | view_duplicates)
  {
    for (auto* directory : group)
      fingerprint_map[directory->GetFingerprint()].push_back(directory);
  }

  // Return the groups of duplicate directories.
  std::vector<std::vector<DirectoryNode*>> duplicates;
  for (const auto& group : fingerprint_map | view_duplicates)
    duplicates.push_back(group);

  return duplicates;
}

} // namespace dejavu
