// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

#include <assert.h>
#include <signal.h>

#include <cstddef>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <unordered_set>

// TODO: Remove relative path.
#include "../../scorer/scorer_util.h"

namespace viaevo {

namespace {

// Number of result slots the scorer expects (results[0..10], i.e. indices
// 1..10 are read by Score).
constexpr std::size_t kExpectedResultsSize = 11;

} // namespace

ScorerMnistDigits::ScorerMnistDigits(Random &gen, std::string images_filename,
                                     std::string labels_filename)
    : gen_(gen), images_filename_(images_filename),
      labels_filename_(labels_filename) {
  LoadData();
  ResetInputs();
}

long long ScorerMnistDigits::Score(const Program &program) const {
  if (program.last_stop_signal() == SIGPROF ||
      program.last_stop_signal() == SIGALRM) {
    // Penalize long running programs - e.g. due to an infinite loop. The
    // Sandbox arms a CPU-time timeout (SIGPROF) as the primary bound and a
    // wall-clock timeout (SIGALRM) as a backstop; either indicates a timeout.
    return 0;
  }

  const std::vector<int> &results = program.last_results();

  // last_results() can be shorter than expected (typically empty) when the ELF
  // process terminates before its results are read back. Guard the
  // results[1..10] accesses below against an out-of-bounds read and log the
  // occurrence.
  if (!ResultsHaveMinSize(results, kExpectedResultsSize,
                          "ScorerMnistDigits::Score"))
    return 0;

  long long score = 0;

  // Return maximum score if results[1] == expected_value_;
  if (results[1] == expected_value_)
    return 1'000'000'000;
  // "Reward" result[1] being in the correct range.
  if (results[1] >= 0 && results[1] <= 9)
    return 1'000'000;

  // results[0] is disregarded as it is changed in main of //elfs:simple_small
  // to 20.

  // Assuming results[1..10] are initialized to -1;

  // Score if any of results different from -1. It is assumed that this is
  // better than no change (the program evolved to at least change the values in
  // results).
  for (int i = 1; i <= 10; ++i) {
    if (results[i] != -1)
      score += 1;
  }

  return score;
}

// "Reward" different values for results[1] across executions on different
// inputs. (This should help avoid the evolved programs always computing the
// same results irrespective of the inputs.)
long long ScorerMnistDigits::ScoreResultsHistory(
    const std::vector<std::vector<int>> &results_history) const {
  std::unordered_set<int> results1_values;

  for (auto &results : results_history) {
    // Skip (and log) any history entry too short to contain results[1].
    if (!ResultsHaveMinSize(results, 2, "ScorerMnistDigits::ScoreResultsHistory"))
      continue;
    results1_values.insert(results[1]);
  }

  if (results1_values.size() < 2) {
    // If there are not at least two different values in results[1], the score
    // is 0.
    return 0;
  }

  // There are at least two differen values in results[1], start score at 1Q.
  long long score = 1'000'000'000'000'000;

  int values_in_range_count = 0;
  for (int i = 0; i <= 9; ++i) {
    if (results1_values.count(i) > 0) {
      ++values_in_range_count;
    }
  }

  // Add 1Q for each observed value in results[1] that is 0 >= and <= 9. The
  // assumption is made that each value between 0 and 9 inclusive will be
  // expected in results history.
  score += values_in_range_count * 1'000'000'000'000'000LL;

  // Add 999T if no value outside of the range is observer, add 998T if one such
  // value is observed, 997T if two, etc... This should "incentivize" the
  // evolved program to only provide values between 0 and 9 inclusive in
  // results[1].
  score +=
      (999 - std::min(999UL, results1_values.size() - values_in_range_count)) *
      1'000'000'000'000LL;

  return score;
}

long long ScorerMnistDigits::MaxScore() const { return 1'000'000'000; }

long long ScorerMnistDigits::MaxScoreResultsHistory() const {
  // 1Q for more than one value and 1Q for each valid digit value, 999T for no
  // other values outside of the range in results[1].
  return 11'999'000'000'000'000;
}

void ScorerMnistDigits::ResetInputs() {
  int pos = gen_() % num_samples_;
  LoadSample(pos);
}

namespace {
int ReadBigEndianInt(std::ifstream &ifs) {
  int result = 0;
  unsigned char bytes[4];
  ifs.read(reinterpret_cast<char *>(bytes), 4);
  result = bytes[0];
  for (int j = 1; j < 4; ++j) {
    result <<= 8;
    result += bytes[j];
  }
  return result;
}
} // namespace

void ScorerMnistDigits::LoadData() {
  // Read and validate the images file header, then cache all pixel bytes.
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
  assert(dimensions_sizes[0] == num_samples_ &&
         "Expected 60000 samples in images data.");
  assert(dimensions_sizes[1] == 28 && dimensions_sizes[2] == 28 &&
         "Expected images size to be 28x28.");
  image_size_ = dimensions_sizes[1] * dimensions_sizes[2];

  images_data_.resize((std::size_t)num_samples_ * image_size_);
  ifs_images.read(reinterpret_cast<char *>(images_data_.data()),
                  images_data_.size());
  assert(!ifs_images.bad() &&
         ifs_images.gcount() == (std::streamsize)images_data_.size() &&
         "Failed to read images data.");
  ifs_images.close();

  // Read and validate the labels file header, then cache all label bytes.
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

  std::vector<int> label_dimensions_sizes(num_dimensions);
  for (int i = 0; i < num_dimensions; ++i) {
    label_dimensions_sizes[i] = ReadBigEndianInt(ifs_labels);
  }
  assert(label_dimensions_sizes[0] == num_samples_ &&
         "Expected 60000 samples in labels data.");

  labels_data_.resize(num_samples_);
  ifs_labels.read(reinterpret_cast<char *>(labels_data_.data()),
                  labels_data_.size());
  assert(!ifs_labels.bad() &&
         ifs_labels.gcount() == (std::streamsize)labels_data_.size() &&
         "Failed to read labels data.");
  ifs_labels.close();
}

void ScorerMnistDigits::LoadSample(int pos) {
  assert(pos >= 0 && pos < num_samples_ && "Sample position out of range.");

  // Copy the image's pixel bytes from the cache into current_inputs_. The
  // buffer is sized as before (one extra int for padding) and zero-initialized
  // so any unused trailing bytes are deterministic.
  current_inputs_.assign(
      1 + image_size_ / sizeof(decltype(current_inputs_)::value_type), 0);
  std::memcpy(current_inputs_.data(),
              images_data_.data() + (std::size_t)pos * image_size_,
              image_size_);

  expected_value_ = labels_data_[pos];
}

} // namespace viaevo