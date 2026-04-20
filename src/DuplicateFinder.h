#pragma once

#include <vector>

namespace dejavu
{
// Forward declarations
class FileNode;
class DirectoryNode;

// Given a list of FileNodes, return a list of groups of duplicate files.
std::vector<std::vector<FileNode*>> FindDuplicateFiles(
    std::vector<FileNode*>& files);

// Given a list of DirectoryNodes, return a list of groups of duplicate
// directories. Note: FindDuplicateFiles() must be called first to ensure that
// the appropriate metadata is available for the directories.
std::vector<std::vector<DirectoryNode*>> FindDuplicateDirectories(
    std::vector<DirectoryNode*>& directories);
} // namespace dejavu
