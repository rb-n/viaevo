// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

#include <assert.h>

#include <fstream>
#include <ios>
#include <iostream>
#include <unordered_set>

namespace viaevo {

ScorerMnistDigits::ScorerMnistDigits(Random &gen, std::string images_filename,
                                     std::string labels_filename)
    : gen_(gen), images_filename_(images_filename),
      labels_filename_(labels_filename) {
  ResetInputs();
}

long long ScorerMnistDigits::Score(const Program &program) const {
  const std::vector<int> &results = program.last_results();
  long long score = 0;

  // Return maximum score if results[1] == expected_value_;
  if (results[1] == expected_value_)
    return 1'000'000'000;

  // results[0] is disregarded as it is changed in main of //elfs:simple_small
  // to 20.

  // Assuming results[1..10] are initialized to -1;

  // Score if any of results different from -1;
  // Score more if any of results between 0 and 9;
  // Score even more if any of the results equals expected_value;
  for (int i = 1; i <= 10; ++i) {
    if (results[i] != -1)
      score += 1;
    if (results[i] >= 0 && results[i] <= 9)
      score += 1'000;
    if (results[i] == expected_value_)
      score += 1'000'000;
  }

  return score;
}

// "Reward" different values for results[1] across executions on different
// inputs. (This should help avoid the evolved programs always computing the
// same results irrespective of the inputs.)
long long ScorerMnistDigits::ScoreResultsHistory(
    const std::vector<std::vector<int>> &results_history) const {
  if (results_history.size() < 1) {
    return 0;
  }

  long long score = 0;

  std::unordered_set<int> results1_values;

  for (auto &results : results_history) {
    results1_values.insert(results[1]);
  }

  if (results1_values.size() > 1) {
    // Add 1T to score if more than one value shows up in results[1].
    score += 1'000'000'000'000;
    for (int i = 0; i <= 9; ++i) {
      if (results1_values.count(i) > 0) {
        // Add 1T for each valid digit value showing up in results[1].
        score += 1'000'000'000'000;
      }
    }
  }

  return score;
}

long long ScorerMnistDigits::MaxScore() const { return 1'000'000'000; }

long long ScorerMnistDigits::MaxScoreResultsHistory() const {
  // 1T for more than one value and 1T for each valid digit value.
  return 11'000'000'000'000;
}

void ScorerMnistDigits::ResetInputs() {
  int pos = gen_() % num_samples_;
  LoadSample(pos);
}

namespace {
int ReadBigEndianInt(std::ifstream &ifs) {
  int result = 0;
  unsigned char bytes[4];
  ifs >> bytes[0] >> bytes[1] >> bytes[2] >> bytes[3];
  result = bytes[0];
  for (int j = 1; j < 4; ++j) {
    result <<= 8;
    result += bytes[j];
  }
  return result;
}
} // namespace

void ScorerMnistDigits::LoadSample(int pos) {
  // Read the image.
  std::ifstream ifs_images(images_filename_, std::ios::binary);
  assert(ifs_images.is_open() && "Failed to open images data.");
  unsigned char c;
  for (int i = 0; i < 2; ++i) {
    ifs_images >> c;
    assert(c == 0 && "Images file's first two bytes should be 0.");
  }
  unsigned char data_type, num_dimensions;
  ifs_images >> data_type >> num_dimensions;
  assert(data_type == 8 &&
         "Images data type is not the expected 'unsigned byte'.");
  assert(num_dimensions == 3 &&
         "Images dimensions do not have the expected value of 3.");

  std::vector<int> dimensions_sizes(num_dimensions);
  for (int i = 0; i < num_dimensions; ++i) {
    dimensions_sizes[i] = ReadBigEndianInt(ifs_images);
  }
  assert(dimensions_sizes[0] == 60000 &&
         "Expected 60000 samples in images data.");
  assert(dimensions_sizes[1] == 28 && dimensions_sizes[2] == 28 &&
         "Expected images size to be 28x28.");
  std::ifstream::pos_type image_size =
      dimensions_sizes[1] * dimensions_sizes[2];
  ifs_images.seekg(ifs_images.tellg() + pos * image_size);
  assert(!ifs_images.bad() && !ifs_images.eof() && "Images data seekg failed.");
  current_inputs_.resize(1 + image_size /
                                 sizeof(decltype(current_inputs_)::value_type));
  ifs_images.read(reinterpret_cast<char *>(current_inputs_.data()), image_size);
  ifs_images.close();

  // Read the label.
  std::ifstream ifs_labels(labels_filename_, std::ios::binary);
  assert(ifs_labels.is_open() && "Failed to open labels data.");
  for (int i = 0; i < 2; ++i) {
    ifs_labels >> c;
    assert(c == 0 && "Labels file's first two bytes should be 0.");
  }
  ifs_labels >> data_type >> num_dimensions;
  assert(data_type == 8 &&
         "Labels data type is not the expected 'unsigned byte'.");
  assert(num_dimensions == 1 &&
         "Labels dimensions do not have the expected value of 1.");

  for (int i = 0; i < num_dimensions; ++i) {
    dimensions_sizes[i] = ReadBigEndianInt(ifs_labels);
  }
  assert(dimensions_sizes[0] == 60000 &&
         "Expected 60000 samples in labels data.");
  ifs_labels.seekg(ifs_labels.tellg() + (std::ifstream::pos_type)pos);
  assert(!ifs_images.bad() && !ifs_images.eof() && "Images data seekg failed.");
  current_inputs_.resize(1 + image_size /
                                 sizeof(decltype(current_inputs_)::value_type));
  ifs_labels >> c;
  expected_value_ = c;
  ifs_labels.close();
}

} // namespace viaevo