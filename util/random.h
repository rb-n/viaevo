// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_UTIL_RANDOM_H_
#define VIAEVO_UTIL_RANDOM_H_

#include <random>

namespace viaevo {

typedef std::mt19937 mt_type;

// Wrap a random number generator so it is easier to mock it in unit tests.
class Random {
public:
  Random();
  void Seed(mt_type::result_type value);
  mt_type::result_type operator()();

  template <class CharT, class Traits>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &ost, const Random &r);

  template <class CharT, class Traits>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &ist, Random &r);

protected:
  mt_type gen_;
};

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &ost, const Random &r) {
  ost << r.gen_;
}

template <class CharT, class Traits>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &ist, Random &r) {
  ist >> r.gen_;
}

} // namespace viaevo

#endif // VIAEVO_UTIL_RANDOM_H_