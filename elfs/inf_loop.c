// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

// The ELF of this program with an infinite loop is only intended for unit
// testing of the Program class. SIGALRM after a Program's predefined duration
// should be delivered to the process.

// Define some initialized data for consistency to meet Program's expectation
// about the ELF. These values are a relic of early development when the values
// were intended to be easily recognizable in the ELF file or in the process
// memory. The values should not matter here.
int dummy[] = {10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int results[] = {10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};
// clang-format off
int inputs[] = {100, 
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17
  };
// clang-format on

int main() {
  while (1)
    ;
  return 0;
}
