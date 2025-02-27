/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include "hermes/Support/UTF8.h"

#include "gtest/gtest.h"

#include <deque>
#include <vector>

using namespace hermes;

namespace {

// Verify correct behavior of convertUTF16ToUTF8StringWithSingleSurrogates().
TEST(StringTest, UTF16ToUTF8StringWithSingleSurrogates) {
  auto convert = [](std::initializer_list<char16_t> cs) -> std::vector<char> {
    std::string out;
    std::vector<char16_t> vec(cs);
    convertUTF16ToUTF8WithSingleSurrogates(out, cs);
    return std::vector<char>(out.begin(), out.end());
  };
  auto bytes =
      [](std::initializer_list<unsigned char> cs) -> std::vector<char> {
    return std::vector<char>(cs.begin(), cs.end());
  };
  EXPECT_EQ(bytes({}), convert({}));
  EXPECT_EQ(bytes({'a'}), convert({'a'}));
  EXPECT_EQ(bytes({'a', 'b', 'c', 0, 'd'}), convert({'a', 'b', 'c', 0, 'd'}));
  EXPECT_EQ(bytes({'a', 'b', 'c', 0, 'd'}), convert({'a', 'b', 'c', 0, 'd'}));
  EXPECT_EQ(bytes({'e', 0xCC, 0x81}), convert({'e', 0x0301}));
  EXPECT_EQ(bytes({0xE2, 0x98, 0x83}), convert({0x2603}));

  // UTF-16 encoded U+1F639
  EXPECT_EQ(
      bytes({0xED, 0xA0, 0xBD, 0xED, 0xB8, 0xB9}), convert({0xD83D, 0xDE39}));

  // Unpaired surrogate halves
  EXPECT_EQ(bytes({0xED, 0xA0, 0xBD}), convert({0xD83D}));
  EXPECT_EQ(bytes({'a', 0xED, 0xA0, 0xBD, 'b'}), convert({'a', 0xD83D, 'b'}));
  EXPECT_EQ(bytes({'a', 0xED, 0xB8, 0xB9, 'b'}), convert({'a', 0xDE39, 'b'}));

  // Unpaired trailing surrogate halves
  EXPECT_EQ(bytes({'a', 0xED, 0xA0, 0xBD}), convert({'a', 0xD83D}));
  EXPECT_EQ(bytes({'a', 0xED, 0xB8, 0xB9}), convert({'a', 0xDE39}));
}

// Verify correct behavior of convertUTF16ToUTF8StringWithReplacements().
TEST(StringTest, UTF16ToUTF8StringWithReplacements) {
  auto convert = [](std::initializer_list<char16_t> cs) -> std::vector<char> {
    std::string out;
    std::vector<char16_t> vec(cs);
    convertUTF16ToUTF8WithReplacements(out, cs);
    return std::vector<char>(out.begin(), out.end());
  };
  auto bytes =
      [](std::initializer_list<unsigned char> cs) -> std::vector<char> {
    return std::vector<char>(cs.begin(), cs.end());
  };
  EXPECT_EQ(bytes({}), convert({}));
  EXPECT_EQ(bytes({'a'}), convert({'a'}));
  EXPECT_EQ(bytes({'a', 'b', 'c', 0, 'd'}), convert({'a', 'b', 'c', 0, 'd'}));
  EXPECT_EQ(bytes({'a', 'b', 'c', 0, 'd'}), convert({'a', 'b', 'c', 0, 'd'}));
  EXPECT_EQ(bytes({'e', 0xCC, 0x81}), convert({'e', 0x0301}));
  EXPECT_EQ(bytes({0xE2, 0x98, 0x83}), convert({0x2603}));

  // UTF-16 encoded U+1F639
  EXPECT_EQ(bytes({0xF0, 0x9F, 0x98, 0xB9}), convert({0xD83D, 0xDE39}));

  // Unpaired surrogate halves
  EXPECT_EQ(bytes({0xEF, 0xBF, 0xBD}), convert({0xD83D}));
  EXPECT_EQ(bytes({'a', 0xEF, 0xBF, 0xBD, 'b'}), convert({'a', 0xD83D, 'b'}));
  EXPECT_EQ(bytes({'a', 0xEF, 0xBF, 0xBD, 'b'}), convert({'a', 0xDE39, 'b'}));

  // Unpaired trailing surrogate halves
  EXPECT_EQ(bytes({'a', 0xEF, 0xBF, 0xBD}), convert({'a', 0xD83D}));
  EXPECT_EQ(bytes({'a', 0xEF, 0xBF, 0xBD}), convert({'a', 0xDE39}));
}

TEST(StringTest, IsAllASCIITest) {
  std::deque<uint8_t> ascii = {32, 23, 18};
  std::deque<uint8_t> notAscii = {234, 1, 0};
  EXPECT_TRUE(isAllASCII(std::begin(ascii), std::end(ascii)));
  EXPECT_FALSE(isAllASCII(std::begin(notAscii), std::end(notAscii)));
  // Check overloads.
  uint8_t asciiArr[] = {1, 3, 14, 54, 19, 124, 13, 43, 127, 19, 0};
  uint8_t partialAsciiArr[] = {1, 3, 14, 54, 219, 124, 13, 43, 127, 19};
  uint8_t noAsciiArr[] = {129, 153, 175, 201, 219, 231, 214, 255, 255, 130};
  EXPECT_TRUE(isAllASCII(std::begin(asciiArr), std::end(asciiArr)));
  EXPECT_FALSE(
      isAllASCII(std::begin(partialAsciiArr), std::end(partialAsciiArr)));
  EXPECT_FALSE(isAllASCII(std::begin(noAsciiArr), std::end(noAsciiArr)));
  EXPECT_TRUE(
      isAllASCII((char *)std::begin(asciiArr), (char *)std::end(asciiArr)));
  EXPECT_FALSE(isAllASCII(
      (char *)std::begin(partialAsciiArr), (char *)std::end(partialAsciiArr)));
  EXPECT_FALSE(
      isAllASCII((char *)std::begin(noAsciiArr), (char *)std::end(noAsciiArr)));
  // Torture test of all possible alignments and lengths.
  for (size_t start = 0; start <= sizeof asciiArr / sizeof *asciiArr; start++) {
    for (size_t end = start; end <= sizeof asciiArr / sizeof *asciiArr; end++) {
      EXPECT_TRUE(isAllASCII(&asciiArr[start], &asciiArr[end]));
    }
  }
  for (size_t start = 0; start <= sizeof noAsciiArr / sizeof *noAsciiArr;
       start++) {
    for (size_t end = start; end <= sizeof noAsciiArr / sizeof *noAsciiArr;
         end++) {
      // Only zero-length substrings are ASCII.
      EXPECT_EQ(start == end, isAllASCII(&noAsciiArr[start], &noAsciiArr[end]));
    }
  }
}

} // end anonymous namespace
