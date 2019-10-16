//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#pragma once

//#include <string>

#include "absl/strings/substitute.h"
#include "fmt/format.h"

#include "eprp.hpp"
#include "iassert.hpp"

class Pass {
private:
  const std::string name;

protected:
  void register_pass(Eprp_method &method);
  void register_inou(Eprp_method &method);

  Pass(std::string_view name_);

  bool setup_directory(std::string_view dir) const;

public:
  static Eprp eprp;

  virtual void setup() = 0;

  static void error(std::string_view msg) { eprp.parser_error(msg); }
  static void warn(std::string_view msg) { eprp.parser_warn(msg); }
  static void info(std::string_view msg) {
#ifndef NDEBUG
    eprp.parser_info(msg);
#endif
  }

  template <typename... Args>
  static void error(const char *format, const Args &... args) {
    fmt::format_args fargs = fmt::make_format_args(args...);
    fmt::memory_buffer tmp;
    fmt::vformat_to(tmp, format, fargs);
    eprp.parser_error(std::string_view(tmp.data(), tmp.size()));
  }

  template <typename... Args>
  static void warn(std::string_view format, const Args &... args) {
    fmt::format_args fargs = fmt::make_format_args(args...);
    fmt::memory_buffer tmp;
    fmt::vformat_to(tmp, format, fargs);
    eprp.parser_warn(std::string_view(tmp.data(), tmp.size()));
  }

  template <typename... Args>
  static void info(std::string_view format, const Args &... args) {
    fmt::format_args fargs = fmt::make_format_args(args...);
    fmt::memory_buffer tmp;
    fmt::vformat_to(tmp, format, fargs);
    eprp.parser_info(std::string_view(tmp.data(), tmp.size()));
  }
};
