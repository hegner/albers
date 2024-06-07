#include <fmt/core.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

std::string rowFormatString(const std::vector<size_t>& colWidths) {
  std::string rowFmt = "";
  rowFmt.reserve(colWidths.size() * 6); // heuristic working for columns that are 2 digit numbers wide

  for (const auto w : colWidths) {
    rowFmt.append(fmt::format("{{:<{}}}", w));
  }

  return rowFmt;
}

template <typename... Types>
void printTable(const std::vector<std::tuple<Types...>>& rows, const std::vector<std::string>& headers) {
  // Simply assume that all rows have the same widths
  const auto nCols = headers.size();
  constexpr auto nColsFromRows = std::tuple_size_v<std::tuple<Types...>>;
  if (nCols != nColsFromRows) {
    throw std::invalid_argument("headers and rows have to have the same size");
  }

  // Transform all elements into strings first to determine column widths
  std::vector<std::vector<std::string>> stringRows;
  stringRows.reserve(rows.size());
  std::transform(rows.begin(), rows.end(), std::back_inserter(stringRows), [&nCols](const auto& elem) {
    std::vector<std::string> strs;
    strs.reserve(nCols);
    std::apply([&strs](auto&&... args) { (strs.emplace_back(fmt::format("{}", args)), ...); }, elem);
    return strs;
  });

  // First figure out how large each column has to be to fit all the content
  std::vector<size_t> colWidths(nCols, 0);
  for (size_t i = 0; i < nCols; ++i) {
    colWidths[i] = headers[i].size();
  }
  for (const auto& row : stringRows) {
    for (size_t iCol = 0; iCol < nCols; ++iCol) {
      colWidths[iCol] = std::max(row[iCol].size(), colWidths[iCol]);
    }
  }

  // print the table header
  for (size_t iCol = 0; iCol < nCols; ++iCol) {
    fmt::print("{:<{}}  ", headers[iCol], colWidths[iCol]);
  }
  fmt::print("\n");
  std::cout << '\n';
  for (size_t iCol = 0; iCol < nCols; ++iCol) {
    fmt::print("{:->{}}  ", "", colWidths[iCol]);
  }
  fmt::print("\n");

  // and the contents
  for (const auto& row : stringRows) {
    for (size_t iCol = 0; iCol < nCols; ++iCol) {
      fmt::print("{:<{}}  ", row[iCol], colWidths[iCol]);
    }
    fmt::print("\n");
  }
}
