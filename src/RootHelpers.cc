#include "podio/utilities/RootHelpers.h"

#include <ROOT/RVec.hxx>

#include <utility>

namespace podio::root_utils {
GenericParameters
loadParamsFrom(ROOT::VecOps::RVec<std::string> intKeys, ROOT::VecOps::RVec<std::vector<int>> intValues,
               ROOT::VecOps::RVec<std::string> floatKeys, ROOT::VecOps::RVec<std::vector<float>> floatValues,
               ROOT::VecOps::RVec<std::string> doubleKeys, ROOT::VecOps::RVec<std::vector<double>> doubleValues,
               ROOT::VecOps::RVec<std::string> stringKeys, ROOT::VecOps::RVec<std::vector<std::string>> stringValues) {
  GenericParameters params{};
  params.loadFrom(std::move(intKeys), std::move(intValues));
  params.loadFrom(std::move(floatKeys), std::move(floatValues));
  params.loadFrom(std::move(doubleKeys), std::move(doubleValues));
  params.loadFrom(std::move(stringKeys), std::move(stringValues));
  return params;
}

} // namespace podio::root_utils
