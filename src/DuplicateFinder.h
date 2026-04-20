#pragma once

#include <vector>

namespace dejavu
{

class FileNode;
std::vector<std::vector<FileNode*>> FindDuplicateFiles(
    std::vector<FileNode*>& files);

} // namespace dejavu
