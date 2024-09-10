#include "MurmurHash3.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

auto readCollNames(const std::string& fileName) {
  std::vector<std::string> collNames{};

  std::ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    std::cerr << "Failed to open file \'" << fileName << "\' for reading collection names" << std::endl;
    return collNames;
  }

  std::string name;
  while (inputFile >> name) {
    collNames.emplace_back(std::move(name));
  }

  return collNames;
}

/// Hash all passed strings using the passed in HashFunc with an interface like
/// the MurmurHash3 methods
template <typename HashFunc, typename HashT = uint32_t>
auto hashStrings(const std::vector<std::string>& strings, HashFunc hashFunc) {
  std::vector<HashT> hashes;
  hashes.reserve(strings.size());

  for (const auto& s : strings) {
    HashT id = 0;
    hashFunc(s.c_str(), s.size(), 0, &id);
    hashes.emplace_back(id);
  }

  return hashes;
}

/// Hash all the passed in strings and check for collisions. Returns a vector of
/// Hashes and the corresponding colliding strings. Empty vector corresponds to
/// no collisions
template <typename HashFunc, typename HashT = uint32_t>
auto getCollisions(const std::vector<std::string>& strings, HashFunc hashFunc) {
  auto hashes = hashStrings<HashFunc, HashT>(strings, hashFunc);

  // Use a multimap for collision detection
  std::multimap<HashT, std::string> hashMap{};
  for (size_t i = 0; i < hashes.size(); ++i) {
    hashMap.emplace(hashes[i], strings[i]);
  }

  std::vector<std::tuple<HashT, std::vector<std::string>>> collidingStrings;
  auto firstIt = hashMap.begin();
  while (firstIt != hashMap.end()) {
    auto rangeIts = hashMap.equal_range(firstIt->first);
    if (std::distance(rangeIts.first, rangeIts.second) != 1) {
      std::vector<std::string> names;
      names.reserve(2); // Most likely case hopefully
      for (auto it = rangeIts.first; it != rangeIts.second; ++it) {
        names.emplace_back(it->second);
      }

      collidingStrings.emplace_back(rangeIts.first->first, std::move(names));
    }

    firstIt = rangeIts.second;
  }

  return collidingStrings;
}

template <typename T, typename A>
std::ostream& operator<<(std::ostream& os, const std::vector<T, A>& vec) {
  os << '[';
  if (!vec.empty()) {
    os << vec[0];
  }
  for (size_t i = 1; i < vec.size(); ++i) {
    os << ", " << vec[i];
  }
  return os << ']';
}

constexpr static auto usage = R"USAGE(usage: podio_test_hashes [-h] collNameFile)USAGE";
constexpr static auto help = R"HELP(
Check if any of the collection names provided lead to a collision in the collection IDs

positional arguments:
  collNameFile      a text file containing all collection names to be checked

optional arguments:
  -h, --help        show this help message and exit
)HELP";

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cerr << usage << std::endl;
    return 1;
  }
  if (argc == 2 && (argv[1] == std::string("-h") || argv[1] == std::string("--help"))) {
    std::cerr << usage << '\n' << help << std::endl;
    return 0;
  }

  const auto collNames = readCollNames(argv[1]);
  const auto collisions = getCollisions(collNames, MurmurHash3_x86_32);

  if (!collisions.empty()) {
    std::cerr << "Found collisions between names" << std::endl;
    std::cout << "hash: " << '\n';
    for (const auto& [hash, colls] : collisions) {
      std::cout << std::hex << std::setw(8) << std::setfill('0') << hash << ": " << colls << '\n';
    }

    return 1;
  }

  return 0;
}
