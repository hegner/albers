#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

template <typename... Types>
void printTable(const std::vector<std::tuple<Types...>>& rows, const std::vector<std::string>& headers) {
  // Simply assume that all rows have the same widths
  const auto nCols = rows[0].size();
  // First figure out how large each column has to be to fit all the content
  std::vector<size_t> colWidths{0, nCols};

  std::vector<std::vector<std::string>> stringRows;
  stringRows.reserve(rows.size());

  for (size_t i = 0; i < nCols; ++i) {
    colWidths[i] = headers[i].size();
  }
  for (const auto& row : rows) {
    for (size_t iCol = 0; iCol < nCols; ++iCol) {
      colWidths[iCol] = std::max(row[iCol].size(), colWidths[iCol]);
    }
  }

  for (size_t iCol = 0; iCol < nCols; ++iCol) {
    std::cout << std::setw(colWidths[iCol] + 1) << headers[iCol];
  }
  std::cout << '\n';
  for (size_t iCol = 0; iCol < nCols; ++iCol) {
    std::cout << std::setw(colWidths[iCol] + 1) << std::setfill('-') << " ";
  }
  std::cout << '\n';
}
