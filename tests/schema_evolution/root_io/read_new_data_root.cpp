#include "podio/ROOTReader.h"
#include "read_new_data.h"

int main() {
  return read_new_data<podio::ROOTReader>("example_data_old_schema.root");
}
