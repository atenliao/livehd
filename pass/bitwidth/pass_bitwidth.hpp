//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#ifndef PASS_BITWIDTH_H
#define PASS_BITWIDTH_H

#include <string>

//#include "bm.h"

#include "pass.hpp"

class Pass_bitwidth : public Pass {
protected:
  class Pass_bitwidth_options_pack {
  public:
    int max_iterations;
  };
  Pass_bitwidth_options_pack opack;

  //bm::bvector<> pending;
  //bm::bvector<> next_pending;
  std::vector<Node_pin> pending;
  std::vector<Node_pin> next_pending;

  void mark_all_outputs(const LGraph *lg, Node_pin &pin);

  /*void iterate_graphio(const LGraph *lg, Index_ID idx);
  void iterate_logic(const LGraph *lg, Index_ID idx);*/
  void iterate_arith(const LGraph *lg, Node_pin &pin, Node_Type_Op op);
  /*void iterate_shift(const LGraph *lg, Index_ID idx);
  void iterate_comparison(const LGraph *lg, Index_ID idx);
  void iterate_pick(const LGraph *lg, Index_ID idx);
  void iterate_mux(const LGraph *lg, Index_ID idx);
  void iterate_equals(const LGraph *lg, Index_ID idx);
  void iterate_subgraph(const LGraph *lg, Index_ID idx);*/

  void iterate_node(LGraph *lg, Index_ID idx);
  void iterate_driver_pin(LGraph *lg, Node_pin &pin);

  void bw_pass_setup(LGraph *lg);
  void bw_pass_dump(LGraph *lg);
  bool bw_pass_iterate(LGraph *lg);

  static void trans(Eprp_var &var);
  void        do_trans(LGraph *orig);

public:
  Pass_bitwidth();

  void setup() final;
};

#endif
