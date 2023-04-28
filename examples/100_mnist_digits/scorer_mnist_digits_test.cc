// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../../util/random_mock.h"

#include <dirent.h>

namespace {

class ProgramMock : public viaevo::Program {
public:
  void set_last_results(const std::vector<int> &results) {
    last_results_ = results;
  }
};

TEST(ScorerMnistDigitsTest, ResetInputs) {
  viaevo::RandomMock gen({0, 14, 4, 1});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 4);
  EXPECT_EQ(gen(), 1);
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 4);
  EXPECT_EQ(gen(), 1);

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");

  // clang-format off
  std::vector<unsigned char> image0{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,  18,  18,  18, 126, 136, 175,  26, 166, 255, 247, 127,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  30,  36,  94, 154, 170, 253, 253, 253, 253, 253, 225, 172, 253, 242, 195,  64,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  49, 238, 253, 253, 253, 253, 253, 253, 253, 253, 251,  93,  82,  82,  56,  39,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  18, 219, 253, 253, 253, 253, 253, 198, 182, 247, 241,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  80, 156, 107, 253, 253, 205,  11,   0,  43, 154,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  14,   1, 154, 253,  90,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 139, 253, 190,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  11, 190, 253,  70,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  35, 241, 225, 160, 108,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  81, 240, 253, 253, 119,  25,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  45, 186, 253, 253, 150,  27,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  16,  93, 252, 253, 187,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 249, 253, 249,  64,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  46, 130, 183, 253, 253, 207,   2,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  39, 148, 229, 253, 253, 253, 250, 182,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  24, 114, 221, 253, 253, 253, 253, 201,  78,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  23,  66, 213, 253, 253, 253, 253, 198,  81,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  18, 171, 219, 253, 253, 253, 253, 195,  80,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  55, 172, 226, 253, 253, 253, 253, 244, 133,  11,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 136, 253, 253, 253, 212, 135, 132,  16,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  };
  // clang-format on

  const unsigned char *tmp =
      reinterpret_cast<const unsigned char *>(scorer.current_inputs().data());
  std::vector<unsigned char> read_image0;
  for (int i = 0; i < 28 * 28; ++i)
    read_image0.push_back(tmp[i]);

  EXPECT_EQ(image0, read_image0);
  EXPECT_EQ(scorer.expected_value(), 5);

  scorer.ResetInputs(); // pos = 14

  // clang-format off
  std::vector<unsigned char> image14{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1, 168, 242,  28,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10, 228, 254, 100,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 190, 254, 122,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  83, 254, 162,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 248,  25,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 255, 254, 103,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 255, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254,  63,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254,  28,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254,  28,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254,  35,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  29, 254, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6, 212, 254, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 203, 254, 178,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 155, 254, 190,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  32, 199, 104,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  };
  // clang-format on

  tmp = reinterpret_cast<const unsigned char *>(scorer.current_inputs().data());
  std::vector<unsigned char> read_image14;
  for (int i = 0; i < 28 * 28; ++i)
    read_image14.push_back(tmp[i]);

  EXPECT_EQ(image14, read_image14);
  EXPECT_EQ(scorer.expected_value(), 1);

  scorer.ResetInputs(); // pos = 4

  // clang-format off
  std::vector<unsigned char> image4{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  55, 148, 210, 253, 253, 113,  87, 148,  55,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  87, 232, 252, 253, 189, 210, 252, 252, 253, 168,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  57, 242, 252, 190,  65,   5,  12, 182, 252, 253, 116,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  96, 252, 252, 183,  14,   0,   0,  92, 252, 252, 225,  21,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 132, 253, 252, 146,  14,   0,   0,   0, 215, 252, 252,  79,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 126, 253, 247, 176,   9,   0,   0,   8,  78, 245, 253, 129,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  16, 232, 252, 176,   0,   0,   0,  36, 201, 252, 252, 169,  11,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  22, 252, 252,  30,  22, 119, 197, 241, 253, 252, 251,  77,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  16, 231, 252, 253, 252, 252, 252, 226, 227, 252, 231,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  55, 235, 253, 217, 138,  42,  24, 192, 252, 143,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  62, 255, 253, 109,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  71, 253, 252,  21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 253, 252,  21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  71, 253, 252,  21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 106, 253, 252,  21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  45, 255, 253,  21,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 218, 252,  56,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  96, 252, 189,  42,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  14, 184, 252, 170,  11,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  14, 147, 252,  42,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  };
  // clang-format on

  std::vector<unsigned char> read_image4;
  for (int i = 0; i < 28 * 28; ++i)
    read_image4.push_back(tmp[i]);

  EXPECT_EQ(image4, read_image4);
  EXPECT_EQ(scorer.expected_value(), 9);

  scorer.ResetInputs(); // pos = 1

  // clang-format off
  std::vector<unsigned char> image1{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  51, 159, 253, 159,  50,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  48, 238, 252, 252, 252, 237,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  54, 227, 253, 252, 239, 233, 252,  57,   6,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  60, 224, 252, 253, 252, 202,  84, 252, 253, 122,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 163, 252, 252, 252, 253, 252, 252,  96, 189, 253, 167,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  51, 238, 253, 253, 190, 114, 253, 228,  47,  79, 255, 168,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  48, 238, 252, 252, 179,  12,  75, 121,  21,   0,   0, 253, 243,  50,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  38, 165, 253, 233, 208,  84,   0,   0,   0,   0,   0,   0, 253, 252, 165,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   7, 178, 252, 240,  71,  19,  28,   0,   0,   0,   0,   0,   0, 253, 252, 195,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  57, 252, 252,  63,   0,   0,   0,   0,   0,   0,   0,   0,   0, 253, 252, 195,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 198, 253, 190,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 255, 253, 196,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  76, 246, 252, 112,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 253, 252, 148,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  85, 252, 230,  25,   0,   0,   0,   0,   0,   0,   0,   0,   7, 135, 253, 186,  12,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  85, 252, 223,   0,   0,   0,   0,   0,   0,   0,   0,   7, 131, 252, 225,  71,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  85, 252, 145,   0,   0,   0,   0,   0,   0,   0,  48, 165, 252, 173,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  86, 253, 225,   0,   0,   0,   0,   0,   0, 114, 238, 253, 162,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  85, 252, 249, 146,  48,  29,  85, 178, 225, 253, 223, 167,  56,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  85, 252, 252, 252, 229, 215, 252, 252, 252, 196, 130,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  28, 199, 252, 252, 253, 252, 252, 233, 145,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  25, 128, 252, 253, 252, 141,  37,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  };
  // clang-format on

  tmp = reinterpret_cast<const unsigned char *>(scorer.current_inputs().data());
  std::vector<unsigned char> read_image1;
  for (int i = 0; i < 28 * 28; ++i)
    read_image1.push_back(tmp[i]);

  EXPECT_EQ(image1, read_image1);
  EXPECT_EQ(scorer.expected_value(), 0);

  // tmp = reinterpret_cast<const unsigned char
  // *>(scorer.current_inputs().data());

  // for (int i = 0; i < 28; ++i) {
  //   for (int j = 0; j < 28; ++j)
  //     std::cout << std::setw(4) << (int)tmp[28 * i + j] << ",";
  //   std::cout << std::endl;
  // }
  // std::cout << std::endl;
}

TEST(ScorerMnistDigitsTest, AllLabelValues) {
  viaevo::RandomMock gen({0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                          11, 12, 13, 14, 15, 16, 17, 18, 19, 20});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 1);
  EXPECT_EQ(gen(), 2);
  EXPECT_EQ(gen(), 3);
  EXPECT_EQ(gen(), 4);
  EXPECT_EQ(gen(), 5);
  EXPECT_EQ(gen(), 6);
  EXPECT_EQ(gen(), 7);
  EXPECT_EQ(gen(), 8);
  EXPECT_EQ(gen(), 9);
  EXPECT_EQ(gen(), 10);
  EXPECT_EQ(gen(), 11);
  EXPECT_EQ(gen(), 12);
  EXPECT_EQ(gen(), 13);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 15);
  EXPECT_EQ(gen(), 16);
  EXPECT_EQ(gen(), 17);
  EXPECT_EQ(gen(), 18);
  EXPECT_EQ(gen(), 19);
  EXPECT_EQ(gen(), 20);

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");

  // hexdump of the begginning of the labels file:
  // 0000000 0000 0108 0000 60ea 0005 0104 0209 0301
  // 0000010 0401 0503 0603 0701 0802 0906 0004 0109
  // 0000020 0201 0304 0702 0803 0906 0500 0006 0607
  // 0000030 0801 0907 0903 0508 0309 0003 0407 0809

  // pos = 0
  EXPECT_EQ(scorer.expected_value(), 5);
  scorer.ResetInputs(); // pos = 1
  EXPECT_EQ(scorer.expected_value(), 0);
  scorer.ResetInputs(); // pos = 2
  EXPECT_EQ(scorer.expected_value(), 4);
  scorer.ResetInputs(); // pos = 3
  EXPECT_EQ(scorer.expected_value(), 1);
  scorer.ResetInputs(); // pos = 4
  EXPECT_EQ(scorer.expected_value(), 9);
  scorer.ResetInputs(); // pos = 5
  EXPECT_EQ(scorer.expected_value(), 2);
  scorer.ResetInputs(); // pos = 6
  EXPECT_EQ(scorer.expected_value(), 1);
  scorer.ResetInputs(); // pos = 7
  EXPECT_EQ(scorer.expected_value(), 3);
  scorer.ResetInputs(); // pos = 8
  EXPECT_EQ(scorer.expected_value(), 1);
  scorer.ResetInputs(); // pos = 9
  EXPECT_EQ(scorer.expected_value(), 4);
  scorer.ResetInputs(); // pos = 10
  EXPECT_EQ(scorer.expected_value(), 3);
  scorer.ResetInputs(); // pos = 11
  EXPECT_EQ(scorer.expected_value(), 5);
  scorer.ResetInputs(); // pos = 12
  EXPECT_EQ(scorer.expected_value(), 3);
  scorer.ResetInputs(); // pos = 13
  EXPECT_EQ(scorer.expected_value(), 6);
  scorer.ResetInputs(); // pos = 14
  EXPECT_EQ(scorer.expected_value(), 1);
  scorer.ResetInputs(); // pos = 15
  EXPECT_EQ(scorer.expected_value(), 7);
  scorer.ResetInputs(); // pos = 16
  EXPECT_EQ(scorer.expected_value(), 2);
  scorer.ResetInputs(); // pos = 17
  EXPECT_EQ(scorer.expected_value(), 8);
  scorer.ResetInputs(); // pos = 18
  EXPECT_EQ(scorer.expected_value(), 6);
  scorer.ResetInputs(); // pos = 19
  EXPECT_EQ(scorer.expected_value(), 9);
  scorer.ResetInputs(); // pos = 20
  EXPECT_EQ(scorer.expected_value(), 4);
}

TEST(ScorerMnistDigitsTest, Score) {
  viaevo::RandomMock gen({0, 14});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");
  EXPECT_EQ(scorer.expected_value(), 5);

  std::vector<int> results(11, -1);

  ProgramMock program;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 0);

  results[5] = 20;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1);

  results[5] = 4;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1);

  results[5] = 5;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1);

  results[1] = 5;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1'000'000'000);

  results[1] = 4;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1'000'000);

  scorer.ResetInputs();
  EXPECT_EQ(scorer.expected_value(), 1);
  EXPECT_EQ(scorer.Score(program), 1'000'000);
}

TEST(ScorerMnistDigitsTest, ScoreResultsHistory) {
  viaevo::RandomMock gen({0, 14});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 14);

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");
  EXPECT_EQ(scorer.expected_value(), 5);

  std::vector<std::vector<int>> results_history;

  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 0);

  results_history.push_back({20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 0);

  results_history.push_back({20, -1, -1, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 0);

  results_history.push_back({-9999, -1, -1, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 0);

  results_history.push_back({-9999, -5, -1, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 1'997'000'000'000'000);

  // Same results as the nearest above should not change the score.
  results_history.push_back({-9999, 5, -1, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 2'997'000'000'000'000);

  results_history.push_back({-9999, 7, -1, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 3'997'000'000'000'000);

  results_history.push_back({-9999, 7, 42, -1, -1, -1, -1, -1, -1, -1, 100});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 3'997'000'000'000'000);

  results_history.push_back({-9999, 42, -1, -1, -1, -1, -1, -1, -1, -1, -1});
  EXPECT_EQ(scorer.ScoreResultsHistory(results_history), 3'996'000'000'000'000);
}

TEST(ScorerMnistDigitsTest, MaxScore) {
  viaevo::RandomMock gen({14, 42});
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 42);

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");
  EXPECT_EQ(scorer.MaxScore(), 1'000'000'000);
  EXPECT_EQ(scorer.MaxScoreResultsHistory(), 11'999'000'000'000'000);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.MaxScore(), 1'000'000'000);
  EXPECT_EQ(scorer.MaxScoreResultsHistory(), 11'999'000'000'000'000);
}

} // namespace
