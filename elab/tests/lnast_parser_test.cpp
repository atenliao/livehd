//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "gtest/gtest.h"
#include "fmt/format.h"

#include "lnast.hpp"
#include "lnast_parser.hpp"

using tuple = std::tuple<std::string, uint8_t , uint8_t>;// <node_name, node_type, scope>

int main(int argc, char **argv) {

  if(argc != 2) {
    fprintf(stderr, "Usage:\n\t%s cfg\n", argv[0]);
    exit(-3);
  }

  Lnast_parser lnast_parser;
  lnast_parser.parse_file(argv[1]);

  lnast_parser.ref_lnast()->ssa_trans();

  // FIXME: Any check?
}
