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
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <cnode.h>
#include <toml++/toml.h>

#define TEST(name) void name()
#define RUN_TEST(name)                                                         \
  do {                                                                         \
    std::cout << "[ RUN      ] " << #name << std::endl;                        \
    name();                                                                    \
    std::cout << "[       OK ] " << #name << std::endl;                        \
  } while (0)

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "Assertion failed: " #cond " at " __FILE__ ":" << __LINE__  \
                << std::endl;                                                  \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      std::cerr << "Assertion failed: " #a " == " #b " (" << (a) << " vs "     \
                << (b) << ") at " __FILE__ ":" << __LINE__ << std::endl;       \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_STREQ(a, b)                                                     \
  do {                                                                         \
    if (std::string(a) != std::string(b)) {                                    \
      std::cerr << "Assertion failed: " #a " == " #b " (\"" << (a)             \
                << "\" vs \"" << (b) << "\") at " __FILE__ ":" << __LINE__     \
                << std::endl;                                                  \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

struct DummyConfig {
  bool flag;
  int count;
  float ratio;
  const char *title;
  int choice;
};

struct DummyArrayConfig {
  int *items;
  int count;
};

static constexpr int TEST_CHOICE_A = 0;
static constexpr int TEST_CHOICE_B = 1;

TEST(test_primitives_store) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};

  CNode tree =
      ctab("config",
           {cbool(offsetof(DummyConfig, flag), "flag", true),
            cinteger(offsetof(DummyConfig, count), "count", 42),
            cnumber(offsetof(DummyConfig, ratio), "ratio", 3.14f),
            cstring(offsetof(DummyConfig, title), "title", "hello"),
            cenum(offsetof(DummyConfig, choice), "choice", {"A", "B"}, "B")});

  tree.store_to_struct(&cfg);
  ASSERT_TRUE(cfg.flag == true);
  ASSERT_EQ(cfg.count, 42);
  ASSERT_TRUE(cfg.ratio > 3.13f && cfg.ratio < 3.15f);
  ASSERT_STREQ(cfg.title, "hello");
  ASSERT_EQ(cfg.choice, TEST_CHOICE_B);
  tree.free_allocations(&cfg);
}

TEST(test_primitives_update) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};

  CNode tree =
      ctab("config",
           {cbool(offsetof(DummyConfig, flag), "flag", true),
            cinteger(offsetof(DummyConfig, count), "count", 42),
            cnumber(offsetof(DummyConfig, ratio), "ratio", 3.14f),
            cstring(offsetof(DummyConfig, title), "title", "hello"),
            cenum(offsetof(DummyConfig, choice), "choice", {"A", "B"}, "B")});
  tree.store_to_struct(&cfg);

  cfg.flag = false;
  cfg.count = 100;
  cfg.ratio = 2.718f;
  free((void *)cfg.title);
  cfg.title = strdup("world");
  cfg.choice = TEST_CHOICE_A;

  tree.update_from_struct(&cfg);

  DummyConfig updated_cfg = {true, 0, 0.0f, nullptr, 0};
  tree.store_to_struct(&updated_cfg);

  ASSERT_TRUE(updated_cfg.flag == false);
  ASSERT_EQ(updated_cfg.count, 100);
  ASSERT_TRUE(updated_cfg.ratio == 2.718f);
  ASSERT_STREQ(updated_cfg.title, "world");
  ASSERT_EQ(updated_cfg.choice, TEST_CHOICE_A);

  tree.free_allocations(&cfg);
  tree.free_allocations(&updated_cfg);
}

TEST(test_generate_delta_toml) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};

  CNode tree =
      ctab("config",
           {cbool(offsetof(DummyConfig, flag), "flag", true),
            cinteger(offsetof(DummyConfig, count), "count", 42),
            cnumber(offsetof(DummyConfig, ratio), "ratio", 3.14f),
            cstring(offsetof(DummyConfig, title), "title", "hello"),
            cenum(offsetof(DummyConfig, choice), "choice", {"A", "B"}, "B")});
  tree.store_to_struct(&cfg);
  cfg.count = 100;
  free((void *)cfg.title);
  cfg.title = strdup("world");
  tree.update_from_struct(&cfg);

  std::string delta = tree.generate_delta_toml();
  const char *expected_toml = "count = 100\ntitle = 'world'\n\n";
  ASSERT_STREQ(delta, expected_toml);

  tree.free_allocations(&cfg);
}

TEST(test_array_delta_toml_empty) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  std::string delta_empty = tree.generate_delta_toml();
  ASSERT_TRUE(delta_empty.empty());
}

TEST(test_array_delta_toml_single_default) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  const char *toml_defaults = "items = [0]";
  auto tbl_defaults = toml::parse(toml_defaults);
  tree.update_from_table(tbl_defaults);

  std::string delta_defaults = tree.generate_delta_toml();
  const char *expected_defaults_toml = "items = [\n    \n    ]\n\n";
  ASSERT_STREQ(delta_defaults, expected_defaults_toml);
}

TEST(test_array_delta_toml_multi_default) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  const char *toml_defaults = "items = [0, 0]";
  auto tbl_defaults = toml::parse(toml_defaults);
  tree.update_from_table(tbl_defaults);

  std::string delta_defaults = tree.generate_delta_toml();
  const char *expected_defaults_toml = "items = [\n    ,\n    \n    ]\n\n";
  ASSERT_STREQ(delta_defaults, expected_defaults_toml);
}

TEST(test_array_delta_toml_non_default) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  const char *toml_non_default = "items = [42, 0]";
  auto tbl_non_default = toml::parse(toml_non_default);
  tree.update_from_table(tbl_non_default);

  std::string delta_non_default = tree.generate_delta_toml();
  const char *expected_non_default_toml =
      "items = [\n    item = 42,\n    \n    ]\n\n";
  ASSERT_STREQ(delta_non_default, expected_non_default_toml);

  // Verify structure content via store_to_struct
  DummyArrayConfig cfg = {nullptr, 0};
  tree.store_to_struct(&cfg);
  ASSERT_EQ(cfg.count, 2);
  ASSERT_TRUE(cfg.items != nullptr);
  ASSERT_EQ(cfg.items[0], 42);
  ASSERT_EQ(cfg.items[1], 0);

  tree.free_allocations(&cfg);
}

TEST(test_array_store_to_struct) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  const char *toml_non_default = "items = [42, 0]";
  auto tbl_non_default = toml::parse(toml_non_default);
  tree.update_from_table(tbl_non_default);

  DummyArrayConfig cfg = {nullptr, 0};
  tree.store_to_struct(&cfg);
  ASSERT_EQ(cfg.count, 2);
  ASSERT_TRUE(cfg.items != nullptr);
  ASSERT_EQ(cfg.items[0], 42);
  ASSERT_EQ(cfg.items[1], 0);

  tree.free_allocations(&cfg);
}

TEST(test_toml_update) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};

  CNode tree = ctab(
      "config", {cbool(offsetof(DummyConfig, flag), "flag", false),
                 cinteger(offsetof(DummyConfig, count), "count", 10),
                 cstring(offsetof(DummyConfig, title), "title", "default")});

  const char *toml_str = R"(
        flag = true
        count = 99
        title = "updated"
    )";

  auto tbl = toml::parse(toml_str);
  tree.update_from_table(tbl);
  tree.store_to_struct(&cfg);

  ASSERT_TRUE(cfg.flag == true);
  ASSERT_EQ(cfg.count, 99);
  ASSERT_STREQ(cfg.title, "updated");
  tree.free_allocations(&cfg);
}

TEST(test_generate_delta_toml_non_empty_path) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};
  CNode tree =
      ctab("sub", {cinteger(offsetof(DummyConfig, count), "count", 42)});
  tree.store_to_struct(&cfg);
  cfg.count = 100;
  tree.update_from_struct(&cfg);

  std::string delta = tree.generate_delta_toml("parent");
  const char *expected = "[parent.sub]\ncount = 100\n\n";
  ASSERT_STREQ(delta, expected);

  tree.free_allocations(&cfg);
}

TEST(test_generate_delta_toml_inline_table) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};
  CNode tree =
      ctab("sub", {cinteger(offsetof(DummyConfig, count), "count", 42)});
  tree.store_to_struct(&cfg);
  cfg.count = 100;
  tree.update_from_struct(&cfg);

  std::string delta = tree.generate_delta_toml("", true);
  const char *expected = "sub = { count = 100}";
  ASSERT_STREQ(delta, expected);

  tree.free_allocations(&cfg);
}

TEST(test_generate_delta_toml_non_zero_depth) {
  CNode tree =
      ctab("config", {carray(offsetof(DummyArrayConfig, items),
                             offsetof(DummyArrayConfig, count), sizeof(int),
                             "items", cinteger(0, "item", 0))});

  const char *toml_input = "items = [42]";
  auto tbl = toml::parse(toml_input);
  tree.update_from_table(tbl);

  std::string delta = tree.generate_delta_toml("", false, 1);
  const char *expected = "items = [\n        item = 42\n        ]\n\n";
  ASSERT_STREQ(delta, expected);
}

TEST(test_generate_delta_toml_not_root) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};
  CNode tree =
      ctab("sub", {cinteger(offsetof(DummyConfig, count), "count", 42)});
  tree.store_to_struct(&cfg);
  cfg.count = 100;
  tree.update_from_struct(&cfg);

  std::string delta = tree.generate_delta_toml("", false, 0, false);
  const char *expected = "[sub]\ncount = 100\n\n";
  ASSERT_STREQ(delta, expected);

  tree.free_allocations(&cfg);
}

TEST(test_generate_delta_toml_all_non_default_params) {
  DummyConfig cfg = {false, 0, 0.0f, nullptr, 0};
  CNode tree =
      ctab("sub", {cinteger(offsetof(DummyConfig, count), "count", 42)});
  tree.store_to_struct(&cfg);
  cfg.count = 100;
  tree.update_from_struct(&cfg);

  std::string delta = tree.generate_delta_toml("parent", true, 1, false);
  const char *expected = "sub = { count = 100}";
  ASSERT_STREQ(delta, expected);

  tree.free_allocations(&cfg);
}

int main() {
  RUN_TEST(test_primitives_store);
  RUN_TEST(test_primitives_update);
  RUN_TEST(test_generate_delta_toml);
  RUN_TEST(test_array_delta_toml_empty);
  RUN_TEST(test_array_delta_toml_single_default);
  RUN_TEST(test_array_delta_toml_multi_default);
  RUN_TEST(test_array_delta_toml_non_default);
  RUN_TEST(test_array_store_to_struct);
  RUN_TEST(test_toml_update);
  RUN_TEST(test_generate_delta_toml_non_empty_path);
  RUN_TEST(test_generate_delta_toml_inline_table);
  RUN_TEST(test_generate_delta_toml_non_zero_depth);
  RUN_TEST(test_generate_delta_toml_not_root);
  RUN_TEST(test_generate_delta_toml_all_non_default_params);
  std::cout << "All CNode tests passed!" << std::endl;
  return 0;
}

