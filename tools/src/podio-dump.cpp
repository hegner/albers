#include "argparseUtils.h"
#include "tabulate.h"

#include "podio/Frame.h"
#include "podio/Reader.h"
#include "podio/podioVersion.h"
#include "podio/utilities/MiscHelpers.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <tuple>

struct ParsedArgs {
  std::string inputFile{};
  std::string category{"events"};
  std::vector<size_t> events = std::vector<size_t>(1, 0);
  bool detailed{false};
};

constexpr auto usageMsg = R"(usage: podio-dump [-h] [-c CATEGORY] [-e ENTRIES] [-d] [--version] inputfile)";

constexpr auto helpMsg = R"(
Dump contents of a podio file to stdout

positional arguments:
  inputfile             Name of the file to dump content from

options:
  -h, --help            show this help message and exit
  -c CATEGORY, --category CATEGORY
                        Which Frame category to dump
  -e ENTRIES, --entries ENTRIES
                        Which entries to print. A single number, comma separated list of numbers or "first:last" for an inclusive range of entries. Defaults to the first entry.
  -d, --detailed        Dump the full contents not just the collection info
  --version             show program's version number and exit
)";

void printUsageAndExit() {
  std::cerr << usageMsg << std::endl;
  std::exit(1);
}

auto getArgumentValueOrExit(const std::vector<std::string>& argv, std::vector<std::string>::const_iterator it) {
  const int argc = argv.size();
  const auto index = std::distance(argv.begin(), it);
  if (index > argc - 2) {
    printUsageAndExit();
  }
  return argv[index + 1];
}

std::vector<size_t> parseEventRange(const std::string& evtRange) {
  const auto splitRange = splitString(evtRange, ',');
  const auto parseError = [&evtRange]() {
    std::cerr << "'" << evtRange << "' cannot be parsed into a list of entries" << std::endl;
    std::exit(1);
  };

  if (splitRange.size() == 1) {
    const auto colonSplitRange = splitString(evtRange, ':');
    if (colonSplitRange.size() == 1) {
      return {parseSizeOrExit(splitRange[0])};
    } else if (colonSplitRange.size() == 2) {
      // we have two numbers signifying an inclusive range
      const auto start = parseSizeOrExit(colonSplitRange[0]);
      const auto end = parseSizeOrExit(colonSplitRange[1]);
      std::vector<size_t> events(end - start + 1);
      std::iota(events.begin(), events.end(), start);
      return events;
    } else {
      parseError();
    }
  } else {
    std::vector<size_t> events;
    events.reserve(splitRange.size());
    std::transform(splitRange.begin(), splitRange.end(), std::back_inserter(events),
                   [](const auto& elem) { return parseSizeOrExit(elem); });

    return events;
  }

  parseError();
  return {};
}

ParsedArgs parseArgs(std::vector<std::string> argv) {
  // find help or version
  if (const auto it = findFlags(argv, "-h", "--help", "--version"); it != argv.end()) {
    if (*it == "--version") {
      std::cout << "podio " << podio::version::build_version << '\n';
    } else {
      std::cout << usageMsg << '\n' << helpMsg << std::flush;
    }
    std::exit(0);
  }

  ParsedArgs args;
  // detailed flag
  if (const auto it = findFlags(argv, "-d", "--detailed"); it != argv.end()) {
    args.detailed = true;
    argv.erase(it);
  }
  // category
  if (const auto it = findFlags(argv, "-c", "--category"); it != argv.end()) {
    args.category = getArgumentValueOrExit(argv, it);
    argv.erase(it, it + 2);
  }
  // event range
  if (const auto it = findFlags(argv, "-e", "--events"); it != argv.end()) {
    args.events = parseEventRange(*(it + 1));
    argv.erase(it, it + 2);
  }

  if (argv.size() != 1) {
    printUsageAndExit();
  }
  args.inputFile = argv[0];

  return args;
}

template <typename T>
void printParameterOverview(const podio::Frame& frame) {
  for (const auto& parKey : podio::utils::sortAlphabeticaly(frame.getParameterKeys<T>())) {
    std::cout << parKey << "\t" << frame.getParameter<std::vector<T>>(parKey)->size() << '\n';
  }
}

void printFrameOverview(const podio::Frame& frame) {
  std::cout << "Collections\n";
  std::cout << "Name\tValueType\tSize\tID\n";
  for (const auto& name : frame.getAvailableCollections()) {
    const auto coll = frame.get(name);
    std::cout << name << "\t" << coll->getValueTypeName() << "\t" << coll->size() << "\t" << coll->getID() << '\n';
  }

  std::cout << "\nParameters\n";
  printParameterOverview<int>(frame);
  printParameterOverview<float>(frame);
  printParameterOverview<double>(frame);
  printParameterOverview<std::string>(frame);
}

void printGeneralInfo(const podio::Reader& reader, const std::string& filename) {
  std::cout << "input file: " << filename << '\n';
  std::cout << "datamodel model definitions stored in this file: ";
  for (const auto& model : reader.getAvailableDatamodels()) {
    std::cout << model << ", ";
  }
  std::cout << "\n\n";

  std::cout << "Frame categories in this file:\nName\tEntries\n";

  std::vector<std::tuple<std::string, size_t>> rows{};
  for (const auto& cat : reader.getAvailableCategories()) {
    rows.emplace_back(cat, reader.getEntries(std::string(cat)));
  }
}

void printFrame(const podio::Frame& frame, const std::string& category, size_t iEntry, bool detailed) {
  std::cout << "################## " << category << ": " << iEntry << " ##################\n";
  if (detailed) {

  } else {
    printFrameOverview(frame);
  }
}

int main(int argc, char* argv[]) {
  // We strip the executable name off directly for parsing
  const auto args = parseArgs({argv + 1, argv + argc});

  auto reader = podio::makeReader(args.inputFile);

  printGeneralInfo(reader, args.inputFile);

  for (const auto event : args.events) {
    try {
      const auto& frame = reader.readFrame(args.category, event);
      printFrame(frame, args.category, event, args.detailed);
    } catch (std::runtime_error& err) {
      std::cerr << err.what() << std::endl;
      return 1;
    }
  }

  // const auto event = reader.readNextEvent();
  // printFrameOverview(event);

  return 0;
}
