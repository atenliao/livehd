//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#ifndef PASS_CSE_H
#define PASS_CSE_H

#include <string>

#include "options.hpp"
#include "pass.hpp"

class Pass_cse_options_pack : public Options_pack {
public:
};

class and_node_for_comp;

class Pass_cse : public Pass {
private:
  //  std::map<int,and_node_for_comp> and_node_for_comp_map; // idx, node
protected:
  std::string cse_type;

  Pass_cse_options_pack opack;

public:
  Pass_cse();
  void traverse(LGraph *g, std::map<int, and_node_for_comp> &and_node_for_comp_map, int round);
  void transform(LGraph *orig) final;
};

#endif
