#include "DuplicateFinder.h"

#include "FilesystemNode.h"

#include <cstdint>
#include <ranges>
#include <unordered_map>

namespace vws = std::ranges::views;

namespace dejavu
{

namespace
{
template <typename T>
using FileGroupMap = std::unordered_map<T, std::vector<FileNode*>>;

// Filters a FileGroupMap to only include groups with 2 or more files.
const auto view_duplicates =
    vws::filter([](const auto& pair) { return pair.second.size() >= 2; }) |
    vws::values;
} // namespace

std::vector<std::vector<FileNode*>> FindDuplicateFiles(
    std::vector<FileNode*>& files)
{
  // Tier 1: group files by size.
  FileGroupMap<uint64_t> size_map;
  for (auto* file : files)
    size_map[file->GetSize()].push_back(file);

  // Tier 2: group non-unique files by partial hash.
  FileGroupMap<uint64_t> partial_hash_map;
  for (const auto& group : size_map | view_duplicates)
  {
    for (auto* file : group)
      partial_hash_map[file->GetPartialHash()].push_back(file);
  }

  // Tier 3: group non-unique files by their full hash.
  FileGroupMap<uint64_t> full_hash_map;
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

} // namespace dejavu
