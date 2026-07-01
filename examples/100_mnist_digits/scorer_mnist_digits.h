// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EXAMPLES_100_MNIST_DIGITS_SCORER_MNIST_DIGITS_H_
#define VIAEVO_EXAMPLES_100_MNIST_DIGITS_SCORER_MNIST_DIGITS_H_

#include <vector>

// TODO: Remove relative path.
#include "../../scorer/scorer.h"
#include "../../util/random.h"

namespace viaevo {

// ScorerMnistDigits scores handwritten digit recognition using the MNIST
// library (http://yann.lecun.com/exdb/mnist/). The scorer expects the correct
// digit value (0-9) in results[1]. The scorer expects the results to be
// initialized to -1 (using e.g. the elfs/intermediate_medium elf). (The results
// in e.g. *_small elfs are initialized to 0 or 3 what may interfere with the
// scoring.)
class ScorerMnistDigits : public Scorer {
public:
  explicit ScorerMnistDigits(Random &gen, std::string images_filename,
                             std::string labels_filename);
  // Returns score for a specific Program.
  virtual long long Score(const Program &program) const override;
  // Returns score for a specific Program's results history.
  virtual long long
  ScoreResultsHistory(const std::vector<std::vector<int>> &) const override;
  // Returns maximum possible score for "perfect" results. Should be the same
  // for different values of current_inputs_.
  virtual long long MaxScore() const override;
  // Returns maximum possible score for "perfect" results history.
  virtual long long MaxScoreResultsHistory() const override;
  // Initializes new value(s) for current_inputs_.
  virtual void ResetInputs() override;

  int expected_value() { return expected_value_; }

protected:
  // Reads and validates the images and labels files once, caching their pixel
  // and label bytes into images_data_ and labels_data_. Called from the
  // constructor so that LoadSample does no file I/O.
  void LoadData();

  // Loads the image of the digit in pos position (from the cached images_data_)
  // into current_inputs_ and sets expected_value_ accordingly (from the cached
  // labels_data_).
  void LoadSample(int pos);

  // Random number generator.
  Random &gen_;
  // Filenames for images and labels of the training set.
  std::string images_filename_, labels_filename_;
  // expected_value_ holds the value expected in results[1]. In this case, it is
  // the value of the digit in current_inputs_.
  int expected_value_ = -1;
  // Number of digit samples in input data.
  static constexpr int num_samples_ = 60'000;
  // Size of a single image in bytes (28 * 28), determined when loading data.
  int image_size_ = 0;
  // Cached pixel bytes for all samples (num_samples_ * image_size_ bytes) and
  // cached label bytes (num_samples_ bytes), loaded once by LoadData.
  std::vector<unsigned char> images_data_;
  std::vector<unsigned char> labels_data_;
};

} // namespace viaevo

#endif // VIAEVO_EXAMPLES_100_MNIST_DIGITS_SCORER_MNIST_DIGITS_H_