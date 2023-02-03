// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "random_mock.h"

namespace viaevo {

RandomMock::RandomMock() : Random() {}

RandomMock::RandomMock(const std::vector<mt_type::result_type> &values)
    : Random() {
  set_values(values);
}

mt_type::result_type RandomMock::operator()() {
  // TODO: Add myfail for values_.size() == 1 after myfail is moved to //util.
  // Also update the EXPECT_DEATH message in random_mock_test.cc.
  current_index_ = (current_index_ + 1) % values_.size();
  return values_[current_index_];
};

void RandomMock::set_values(const std::vector<mt_type::result_type> &values) {
  current_index_ = -1;
  values_ = values;
}

} // namespace viaevo
