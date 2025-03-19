#include "FilesystemNode.h"

#include <cassert>
#include <iostream>
#include <string>

FilesystemNode::FilesystemNode(
    const std::filesystem::path& path, DirectoryNode* parent)
    : m_path(path), m_size(INVALID_SIZE), m_parent(parent)
{
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

void FileNode::PrintTree(const int indent) const
{
  std::string indent_str(indent, ' ');
  std::cout << indent_str << m_path.filename();

  if (m_size != INVALID_SIZE)
    std::cout << " (" << m_size << " bytes)";
  else
    std::cout << " (size unknown)";

  std::cout << "\n";
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
}

void DirectoryNode::PrintTree(const int indent) const
{
  std::string indent_str(indent, ' ');
  std::cout << indent_str << m_path.filename() << "/";

  if (m_size != INVALID_SIZE)
    std::cout << " (" << m_size << " bytes)";
  else
    std::cout << " (size unknown)";
  std::cout << "\n";

  for (const auto& child_directory : m_child_directories)
    child_directory.PrintTree(indent + 2);

  for (const auto& child_file : m_child_files)
    child_file.PrintTree(indent + 2);

  if (indent == 0)
  {
    std::cout << std::flush;
  }
}

template <typename T>
uintmax_t DirectoryNode::AddChildNode(
    std::vector<T>& child_nodes, const std::filesystem::path& path)
{
  child_nodes.emplace_back(path, this);
  child_nodes.back().BuildTree();

  auto size = child_nodes.back().GetSize();
  assert(size != INVALID_SIZE);
  return size;
}

std::filesystem::path DirectoryNode::CleanPath(
    const std::filesystem::path& path)
{
  return (path / "").parent_path();
}
