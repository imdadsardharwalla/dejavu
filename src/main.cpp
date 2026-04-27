#include <algorithm>
#include <array>
#include <filesystem>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "DuplicateFinder.h"
#include "FilesystemNode.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "cpp-linenoise/linenoise.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace fs = std::filesystem;

namespace
{

constexpr std::array<std::string_view, 7> kCommands = {
    "add", "list", "scan", "show", "help", "quit", "exit"};

// Populate `completions` with filesystem entries that match `partial`, each
// prepended with `prefix` - note that a linenoise "completion" is the entire
// line, not just the additional text.
void CompleteFilesystemPath(const std::string& partial,
    std::vector<std::string>& completions, const std::string& prefix)
{
  fs::path partial_path(partial);
  fs::path parent;
  std::string stem;

  // Split the partial path into a parent directory to scan and a filename
  // stem to match against. If the partial ends with a separator, treat it as
  // a complete directory and list everything inside.
  if (partial.empty() || partial.back() == '/' || partial.back() == '\\')
  {
    parent = partial.empty() ? fs::current_path() : partial_path;
    stem = "";
  }
  else
  {
    parent = partial_path.has_parent_path() ? partial_path.parent_path()
                                            : fs::current_path();
    stem = partial_path.filename().string();
  }

  if (!fs::is_directory(parent))
    return;

  // Collect every entry in `parent` whose name starts with `stem`. Append a
  // separator to directories so the user can keep tabbing into them.
  for (const auto& entry : fs::directory_iterator(parent))
  {
    std::string name = entry.path().filename().string();
    if (stem.empty() || name.substr(0, stem.size()) == stem)
    {
      std::string match = entry.path().string();
      if (entry.is_directory())
        match += fs::path::preferred_separator;
      completions.push_back(prefix + match);
    }
  }
}

void PrintInputPaths(const dejavu::DuplicateFinder& finder)
{
  std::println();

  if (finder.InputFileCount() > 0)
  {
    std::println("Input Files:");
    for (const auto& path : finder.InputFiles())
      std::println("  {}", path.string());
  }
  else
  {
    std::println("Input Files: none");
  }

  std::println();

  if (finder.InputDirectoryCount() > 0)
  {
    std::println("Input Directories:");
    for (const auto& path : finder.InputDirectories())
      std::println("  {}", path.string());
  }
  else
  {
    std::println("Input Directories: none");
  }

  std::println();
}

// Print `heading` as a full-width section divider, e.g.
//   ──── Heading Name ──────────────────────────────
// The line is padded with U+2500 box-drawing dashes to the terminal width.
// std::print on a Windows console writes via WriteConsoleW, so the UTF-8
// dashes render correctly regardless of console codepage.
void PrintSectionHeading(const std::string& heading)
{
  // U+2500 BOX DRAWINGS LIGHT HORIZONTAL, encoded as UTF-8.
  static constexpr std::string_view kDash = "\xE2\x94\x80";
  static constexpr int kPrefixDashes = 4;

  int cols = linenoise::getColumns(0, 1);
  if (cols <= 0)
    cols = 80;

  const int fill =
      std::max(0, cols - kPrefixDashes - 2 - static_cast<int>(heading.size()));

  std::print("\n");
  for (int i = 0; i < kPrefixDashes; ++i)
    std::print("{}", kDash);
  std::print(" {} ", heading);
  for (int i = 0; i < fill; ++i)
    std::print("{}", kDash);
  std::println();
}

template <typename NodeT>
void PrintDuplicateGroups(
    const std::string& heading, const std::vector<std::vector<NodeT*>>& groups)
{
  PrintSectionHeading(heading);
  if (groups.empty())
  {
    std::println("\nNo duplicates found.");
    return;
  }

  for (size_t i = 0; i < groups.size(); ++i)
  {
    std::println("\nGroup {}:", i + 1);
    for (const auto* node : groups[i])
    {
      std::println("  {} ({} bytes)", node->Path().string(), node->Size());
    }
  }
}

void PrintDuplicates(const dejavu::DuplicateFinder& finder)
{
  PrintDuplicateGroups("Duplicate Directories", finder.DirectoryDuplicates());
  PrintDuplicateGroups("Duplicate Files", finder.FileDuplicates());
  std::println();
}

void PrintHelp()
{
  std::print("Commands:\n"
             "  add <path>  Add a file or directory to scan\n"
             "  list        List all files and directories added to scan\n"
             "  scan        Run (or re-run) duplicate detection\n"
             "  show        Show the last computed results\n"
             "  help        Show this message\n"
             "  quit        Exit the program\n"
             "\n");
}

void AddPathToFinder(dejavu::DuplicateFinder& finder, const std::string& path)
{
  try
  {
    finder.Add(fs::path(path));
    std::println("Added: {}", path);
  }
  catch (const std::exception& e)
  {
    std::println(stderr, "Error: {}", e.what());
  }
}

} // namespace

int main(const int argc, const char* const argv[])
{
  dejavu::DuplicateFinder finder;

  // Add any command-line arguments as files or directories to scan.
  for (int i = 1; i < argc; ++i)
    AddPathToFinder(finder, argv[i]);

  // Register a callback for tab-completion. Linenoise calls it with the
  // current input line and expects us to fill `completions` with candidate
  // replacements for the entire line.
  linenoise::SetCompletionCallback(
      [](const std::string& input, std::vector<std::string>& completions)
      {
        const auto sep = input.find(' ');

        // No space yet means the user is still typing the command name.
        // Suggest any known commands that start with what they've typed.
        if (sep == std::string::npos)
        {
          for (const auto& cmd : kCommands)
          {
            if (cmd.substr(0, input.size()) == input)
              completions.emplace_back(cmd);
          }
          return;
        }

        // Otherwise the user has typed a command and is now typing its
        // argument. For 'add', complete filesystem paths.
        const std::string command = input.substr(0, sep);
        if (command == "add")
        {
          const std::string partial = input.substr(sep + 1);
          const std::string prefix = command + " ";
          CompleteFilesystemPath(partial, completions, prefix);
        }
      });

  std::println("\nType 'help' for available commands.\n");

  std::string line;
  while (true)
  {
    auto quit = linenoise::Readline("dejavu> ", line);
    if (quit)
      break;

    auto sep = line.find(' ');
    std::string command = line.substr(0, sep);
    std::string arg =
        (sep != std::string::npos) ? line.substr(sep + 1) : std::string();

    if (command.empty())
      continue;

    linenoise::AddHistory(line.c_str());

    if (command == "quit" || command == "exit")
      break;

    if (command == "help")
    {
      PrintHelp();
    }
    else if (command == "add")
    {
      if (arg.empty())
      {
        std::println(stderr, "Usage: add <path>");
        continue;
      }

      AddPathToFinder(finder, arg);
    }
    else if (command == "list")
    {
      PrintInputPaths(finder);
    }
    else if (command == "scan")
    {
      try
      {
        finder.ComputeDuplicates();
        PrintDuplicates(finder);
      }
      catch (const std::exception& e)
      {
        std::println(stderr, "Error: {}", e.what());
      }
    }
    else if (command == "show")
    {
      if (!finder.ResultsUpToDate())
      {
        std::println("(Results may be stale - run 'scan' to refresh.)");
      }

      PrintDuplicates(finder);
    }
    else
    {
      std::println(stderr, "Unknown command: {}", command);
      std::println(stderr, "Type 'help' for available commands.");
    }
  }

  return 0;
}
