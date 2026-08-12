// Copyright (c) 2026 Matt Borgerson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <cnode.h>
#include <toml++/toml.h>

#define DEFINE_CONFIG_TREE
#include "test_config.h"

int main() {
  config cfg{};
  std::memset(&cfg, 0, sizeof(cfg));

  config_tree.store_to_struct(&cfg);

  assert(cfg.app.name != nullptr);
  assert(std::string(cfg.app.name) == "DefaultApp");
  assert(cfg.app.version == 1);
  assert(cfg.app.feature_enabled == false);
  assert(cfg.app.log_level == CONFIG_APP_LOG_LEVEL_INFO);

  // Update from TOML string
  const char *toml_str = R"(
        [app]
        name = "GenConfig E2E Test"
        version = 2
        feature_enabled = true
        log_level = "debug"
    )";

  auto tbl = toml::parse(toml_str);
  config_tree.update_from_table(tbl);
  config_tree.store_to_struct(&cfg);

  assert(cfg.app.name != nullptr);
  assert(std::string(cfg.app.name) == "GenConfig E2E Test");
  assert(cfg.app.version == 2);
  assert(cfg.app.feature_enabled == true);
  assert(cfg.app.log_level == CONFIG_APP_LOG_LEVEL_DEBUG);

  std::cout << "End-to-End Test Passed!" << std::endl;
  return 0;
}
