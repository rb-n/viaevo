// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "random.h"

namespace viaevo {

Random::Random() {}

void Random::Seed(mt_type::result_type value) { gen_.seed(value); }

mt_type::result_type Random::operator()() { return gen_(); };

} // namespace viaevo
