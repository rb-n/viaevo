// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_UTIL_RANDOM_MOCK_H_
#define VIAEVO_UTIL_RANDOM_MOCK_H_

#include "random.h"

namespace viaevo {

// Wrap a random number generator so it is easier to mock it in unit tests.
class RandomMock : public Random {
public:
  RandomMock();
  RandomMock(const std::vector<mt_type::result_type> &values);

  mt_type::result_type operator()() override;

  void set_values(const std::vector<mt_type::result_type> &values);
protected:
  int current_index_ = -1;
  std::vector<mt_type::result_type> values_;
};

} // namespace viaevo

#endif // VIAEVO_UTIL_RANDOM_MOCK_H_