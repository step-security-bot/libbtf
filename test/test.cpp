// Copyright (c) Prevail Verifier contributors.
// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>

#include <map>
#include <string>
#include <vector>

#if !defined(MAX_PATH)
#define MAX_PATH (256)
#endif

#include "btf.h"
#include "btf_json.h"
#include "btf_parse.h"
#include "btf_type_data.h"
#include "btf_write.h"
#include "elfio/elfio.hpp"

#define TEST_OBJECT_FILE_DIRECTORY "external/ebpf-samples/build/"
#define TEST_JSON_FILE_DIRECTORY "external/ebpf-samples/json/"
#define BTF_CASE(file)                                                         \
  TEST_CASE("BTF suite: " #file, "[BTF]") { verify_BTF_json(#file); }

void verify_BTF_json(const std::string &file) {
  std::stringstream generated_output;
  auto reader = ELFIO::elfio();
  REQUIRE(reader.load(std::string(TEST_OBJECT_FILE_DIRECTORY) + file + ".o"));

  auto btf = reader.sections[".BTF"];

  libbtf::btf_type_data btf_data = std::vector<std::byte>(
      {reinterpret_cast<const std::byte *>(btf->get_data()),
       reinterpret_cast<const std::byte *>(btf->get_data() + btf->get_size())});

  btf_data.to_json(generated_output);

  // Pretty print the JSON output.
  std::string pretty_printed_json =
      libbtf::pretty_print_json(generated_output.str());

  // Read the expected output from the .json file.
  std::ifstream expected_stream(std::string(TEST_JSON_FILE_DIRECTORY) + file +
                                std::string(".json"));
  std::stringstream generated_stream(pretty_printed_json);

  // Compare each line of the expected output with the actual output.
  std::string expected_line;
  std::string actual_line;
  while (std::getline(expected_stream, expected_line)) {
    bool has_more = (bool)std::getline(generated_stream, actual_line);
    REQUIRE(has_more);
    REQUIRE(expected_line == actual_line);
  }
  bool has_more = (bool)std::getline(expected_stream, actual_line);
  REQUIRE_FALSE(has_more);

  // Verify that encoding the BTF data and parsing it again results in the same
  // JSON.
  libbtf::btf_type_data btf_data_round_trip = btf_data.to_bytes();

  std::stringstream generated_output_round_trip;
  btf_data_round_trip.to_json(generated_output_round_trip);

  // Pretty print the JSON output.
  std::string pretty_printed_json_round_trip =
      libbtf::pretty_print_json(generated_output_round_trip.str());

  // Verify that the pretty printed JSON is the same as the original.
  REQUIRE(pretty_printed_json == pretty_printed_json_round_trip);
}

BTF_CASE(byteswap)
BTF_CASE(ctxoffset)
BTF_CASE(exposeptr)
BTF_CASE(exposeptr2)
BTF_CASE(map_in_map)
BTF_CASE(mapoverflow)
BTF_CASE(mapunderflow)
BTF_CASE(mapvalue - overrun)
BTF_CASE(nullmapref)
BTF_CASE(packet_access)
BTF_CASE(packet_overflow)
BTF_CASE(packet_reallocate)
BTF_CASE(packet_start_ok)
BTF_CASE(stackok)
BTF_CASE(tail_call)
BTF_CASE(tail_call_bad)
BTF_CASE(twomaps)
BTF_CASE(twostackvars)
BTF_CASE(twotypes)

TEST_CASE("validate-parsing-simple-loop", "[BTF]") {
  libbtf::btf_type_data btf_data_loop;
  btf_data_loop.append(libbtf::btf_kind_ptr{.type = 1});

  REQUIRE_THROWS(
      [&] { libbtf::btf_type_data btf_data = btf_data_loop.to_bytes(); }());
}

TEST_CASE("validate-parsing-large-loop", "[BTF]") {
  libbtf::btf_type_data btf_data_loop;

  // Each PTR points to the next PTR.
  for (uint32_t i = 0; i < 10; i++) {
    btf_data_loop.append(libbtf::btf_kind_ptr{.type = i + 1});
  }
  // Last PTR points to itself.
  btf_data_loop.append(libbtf::btf_kind_ptr{.type = 1});

  REQUIRE_THROWS(
      [&] { libbtf::btf_type_data btf_data = btf_data_loop.to_bytes(); }());
}
