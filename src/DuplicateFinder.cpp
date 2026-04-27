#include "DuplicateFinder.h"

#include "FilesystemNode.h"

#include <cstdint>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

namespace dejavu
{

void DuplicateFinder::Add(const std::filesystem::path& path)
{
  if (std::filesystem::is_regular_file(path))
  {
    auto file_node = std::make_unique<FileNode>(path, nullptr);
    file_node->BuildTree();
    m_input_files.push_back(std::move(file_node));
  }
  else if (std::filesystem::is_directory(path))
  {
    auto directory_node = std::make_unique<DirectoryNode>(path, nullptr);
    directory_node->BuildTree();
    m_input_directories.push_back(std::move(directory_node));
  }
  else
  {
    throw std::invalid_argument(
        "Path is not a file or directory: " + path.string());
  }

  m_results_valid = false;
}

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

std::vector<std::vector<FileNode*>> FindDuplicateFiles(
    std::vector<FileNode*>& files)
{
  // Tier 1: group files by size.
  FileNodeGroupMap<std::uintmax_t> size_map;
  for (auto* file : files)
    size_map[file->Size()].push_back(file);

  // Tier 2: group non-unique files by partial hash.
  FileNodeGroupMap<uint64_t> partial_hash_map;
  for (const auto& group : size_map | view_duplicates)
  {
    for (auto* file : group)
      partial_hash_map[file->PartialHash()].push_back(file);
  }

  // Tier 3: group non-unique files by their full hash.
  FileNodeGroupMap<uint64_t> full_hash_map;
  for (const auto& group : partial_hash_map | view_duplicates)
  {
    for (auto* file : group)
      full_hash_map[file->FullHash()].push_back(file);
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
    size_map[directory->Size()].push_back(directory);

  // Tier 2: group non-unique directories by fingerprint.
  DirectoryNodeGroupMap<uint64_t> fingerprint_map;
  for (const auto& group : size_map | view_duplicates)
  {
    for (auto* directory : group)
      fingerprint_map[directory->Fingerprint()].push_back(directory);
  }

  // Return the groups of duplicate directories.
  std::vector<std::vector<DirectoryNode*>> duplicates;
  for (const auto& group : fingerprint_map | view_duplicates)
    duplicates.push_back(group);

  return duplicates;
}

// Retain only the highest-level duplicates. This is done by removing duplicate
// groups for which every member's parent directory is also a duplicate. There
// is a subtlety here that is worth noting. Consider the following structure:
//
// * A/foo.txt
// * B/foo.txt
// * foo.txt
//
// We will have the following duplicate groups:
//
// * {A/foo.txt, B/foo.txt, foo.txt}
// * {A/, B/}
//
// There are two ways to handle this:
//
// 1. Remove the file group {A/foo.txt, B/foo.txt, foo.txt} since A/foo.txt and
//    B/foo.txt are nested, leaving foo.txt with no reported duplicates;
// 2. Report {A/, B/} and {A/foo.txt, B/foo.txt, foo.txt} as duplicates.
//
// We choose the second option to avoid any duplicates being silently omitted.
void RemoveNestedDuplicateGroups(
    std::vector<std::vector<FileNode*>>& duplicate_files,
    std::vector<std::vector<DirectoryNode*>>& duplicate_directories)
{
  // Create a (flattened) set of all duplicate directories.
  std::unordered_set<DirectoryNode*> dup_dirs;
  for (auto& group : duplicate_directories)
  {
    for (auto* directory : group)
      dup_dirs.insert(directory);
  }

  // Check if a duplicate group is a nested duplicate (every member's
  // parent directory is also a duplicate).
  auto is_group_duplicate = [&dup_dirs](auto& group) -> bool
  {
    for (auto* filesystem_node : group)
    {
      if (!filesystem_node->Parent())
        return false;

      if (!dup_dirs.contains(filesystem_node->Parent()))
        return false;
    }
    return true;
  };

  std::erase_if(duplicate_files, is_group_duplicate);
  std::erase_if(duplicate_directories, is_group_duplicate);
}

} // namespace

void DuplicateFinder::ComputeDuplicates()
{
  std::vector<DirectoryNode*> directories;
  std::vector<FileNode*> files;

  // Copy the standalone input files into the list of files.
  for (auto& file : m_input_files)
    files.push_back(file.get());

  // Flatten the input directories into a list of files and directories. Note:
  // the root directory is automatically included in the flattened list.
  for (auto& directory : m_input_directories)
    directory->FlattenTree(directories, files);

  m_duplicate_files = FindDuplicateFiles(files);
  m_duplicate_directories = FindDuplicateDirectories(directories);

  RemoveNestedDuplicateGroups(m_duplicate_files, m_duplicate_directories);

  m_results_valid = true;
}

const std::vector<std::vector<FileNode*>>&
DuplicateFinder::FileDuplicates() const
{
  return m_duplicate_files;
}

const std::vector<std::vector<DirectoryNode*>>&
DuplicateFinder::DirectoryDuplicates() const
{
  return m_duplicate_directories;
}

std::vector<std::filesystem::path> DuplicateFinder::InputFiles() const
{
  std::vector<std::filesystem::path> paths;
  for (auto& file : m_input_files)
    paths.push_back(file->Path());
  return paths;
}

std::vector<std::filesystem::path>
DuplicateFinder::InputDirectories() const
{
  std::vector<std::filesystem::path> paths;
  for (auto& directory : m_input_directories)
    paths.push_back(directory->Path());
  return paths;
}

} // namespace dejavu
