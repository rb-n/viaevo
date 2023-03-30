// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

// The ELF of this program is intended to serve as a template for evolving the
// program to meet a computational objective.
//
// The "intermediate" part of intermediate_medium refers to a small set of
// different operations between dummy variables being present in the main
// function. The "medium" part of intermediate_medium refers to medium sizes of
// the dummy, results, and inputs arrays.

#define NOPS                                                                   \
  asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")

// Define some initialized data where the inputs for the desired computational
// tasks will be placed and from where the results of the tasks will be read.
// This space may also serve as a "scratch space" for the program. Assign some
// values here so that these are easily recognizable in the ELF or in the
// process memory.
int dummy[] = {10, 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
               11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
int results[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
// clang-format off
int inputs[] = {200, 
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
  42, 17, 42, 17, 42, 17, 42, 17, 42, 17,
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

// TODO: The code below is super ad hoc. Review, expand, consolidate,
// systematize, polish, etc.
int main() {
  // Change result[0] as a control to confirm (before code evolution) that this
  // code runs.
  results[0] = 20;
  // Create a suffiecient number instructions in this template program to be
  // modified during the evolution of the program. Ideally(?), these would be
  // nop instuctions only, but only a short stretch of nop instructions is
  // visible in the ELF if only nop instructions are added here. The nop
  // instuctructions are therefore interspersed with other operations.
  //
  // The evolution using simple mutators may proceed via repurposing existing
  // functionality (e.g. modifying the operations between dummy variables below
  // to affect inputs and results) rather than creating new valid instructions -
  // as this may be unlikely to achieve via e.g. random bitflips. Adding
  // additional different operations between dummy variables (as opposed to e.g.
  // simple_small) may enable the evolution of more involved computtional tasks
  // if this is the case.
  NOPS;
  dummy[1] = dummy[2];
  dummy[2] = 2 * dummy[1];
  if (dummy[1] > dummy[4]) {
    dummy[4] = dummy[1];
  }
  dummy[9] = dummy[8] / dummy[7];
  NOPS;
  dummy[2] = dummy[3] + dummy[5];
  dummy[6] <<= 4;
  for (int i = 0; i < 5; ++i) {
    dummy[7] += i;
    if (i % 2 == 0) {
      dummy[8] *= dummy[6];
      dummy[5] = dummy[4] - 100;
    }
  }
  NOPS;
  // Add some unconditional relative jumps (and check via objdump where these
  // land) to allow for an accumulation of "silent mutations" in code that is
  // not currently executed. This code may become executed when the jumps are
  // modified or when (parts of) this non-executed code is "recombined" into an
  // executed code of another (or the same) program.
  asm("jmp . + 127");
  dummy[3] = dummy[4];
  dummy[6] = 4 * dummy[2];
  if (dummy[3] > dummy[5]) {
    dummy[5] = dummy[3];
  }
  dummy[7] = dummy[8] / dummy[9];
  NOPS;
  dummy[4] = dummy[5];
  dummy[8] = dummy[1] + dummy[4];
  dummy[3] <<= 2;
  for (int i = 0; i < 7; ++i) {
    dummy[4] -= i * 2;
    if (i % 2 == 0) {
      dummy[3] -= dummy[8];
      dummy[2] = dummy[3] - 10;
    }
  }
  NOPS;
  dummy[1] = 7 * dummy[9];
  if (dummy[2] > dummy[7]) {
    dummy[7] = dummy[3];
  }
  dummy[3] = dummy[5] / dummy[1];
  NOPS;
  dummy[4] = dummy[5];
  dummy[8] = dummy[3] - dummy[2];
  dummy[5] <<= 4;
  for (int i = 0; i < 3; ++i) {
    if (i % 2 == 0) {
      dummy[7] -= dummy[4];
      dummy[3] = dummy[2] / 10;
    }
    dummy[5] *= i * 4;
  }
  dummy[5] = dummy[6];
  NOPS;
  dummy[6] = dummy[7];
  int j = 5;
  while (j--) {
    dummy[9] = dummy[3] % 17;
  }
  if (dummy[8] % 3 == 0) {
    ++dummy[8];
  }
  dummy[3] = dummy[2] / dummy[1];
  dummy[4] -= dummy[5];
  NOPS;
  int sum = 0;
  for (int i = 0; i < 10; ++i) {
    sum += dummy[i];
  }
  dummy[1] = sum;
  dummy[2] = sum % 2;
  if (sum > 0) {
    dummy[3] = 1;
  } else {
    dummy[3] = 0;
  }
  dummy[7] = dummy[8];
  NOPS;
  dummy[8] = dummy[9];
  int tmp = 0;
  while (dummy[4] != -1 && dummy[4]) {
    tmp += dummy[4] & 1;
    dummy[4] >>= 1;
  }
  dummy[9] = tmp;
  for (int i = 5; i < 8; ++i) {
    dummy[4] += dummy[i];
  }
  NOPS;
  dummy[9] = dummy[10];
  NOPS;
  // 2nd round
  dummy[1] = dummy[2];
  dummy[11] = dummy[19] & 0xFF;
  dummy[12] = (dummy[19] & 0xFF00) >> 8;
  dummy[13] = (dummy[19] & 0xFF0000) >> 16;
  dummy[14] = (dummy[19] & 0xFF000000) >> 24;
  NOPS;
  dummy[2] = dummy[3];
  dummy[15] = dummy[11] > 128 ? 1 : 0;
  dummy[16] = dummy[12] > 128 ? 1 : 0;
  dummy[17] = dummy[13] > 128 ? 1 : 0;
  dummy[18] = dummy[14] > 128 ? 1 : 0;
  NOPS;
  // asm("jmp . + 127");
  dummy[3] = dummy[4];
  NOPS;
  dummy[4] = dummy[5];
  NOPS;
  dummy[5] = dummy[6];
  NOPS;
  dummy[6] = dummy[7];
  NOPS;
  dummy[7] = dummy[8];
  NOPS;
  dummy[8] = dummy[9];
  NOPS;
  dummy[9] = dummy[10];
  NOPS;
  return 0;
}
