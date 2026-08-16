// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

// The ELF of this program is intended to serve as a template for evolving the
// program to meet a computational objective.
//
// The "complex" part of complex_large refers to a rich and deliberately
// balanced "instruction vocabulary" being present in the main function (see
// RECOMMENDATIONS.md 4.1): loads/stores touching the inputs, results, dummy
// and scratchspace arrays, a uniform mix of arithmetic and bitwise operations,
// register-register operations, immediate-bearing instructions, control flow,
// and a few AVX2 vector operations. The "large" part of complex_large refers
// to large sizes of the dummy, inputs, and scratchspace arrays.
//
// The vocabulary is intended to be generic (not geared towards any specific
// benchmark problem): the raw material available in the evolvable region
// constrains what point mutations and recombinations can reach, so the goal is
// to provide a diverse palette of valid building blocks for evolution to
// repurpose.
//
// In contrast to the "simple" and "intermediate" templates, this template
// includes instructions that access the inputs and results variables directly
// (RECOMMENDATIONS.md 4.1). All stores into results below read from locations
// that hold -1 in the template image, so an unevolved execution still leaves
// results visibly unchanged (except for the results[0] = 20 control); during
// evolution, however, inputs hold live data and these instructions provide
// ready-made addressing modes for data flow between inputs and results.

#include <immintrin.h>

#define NOPS                                                                   \
  asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")

// Define some initialized data where the inputs for the desired computational
// tasks will be placed and from where the results of the tasks will be read.
// This space may also serve as a "scratch space" for the program.
int dummy[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
int results[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
// clang-format off
int inputs[] = {-1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
int scratchspace[] = {-1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
// clang-format on

int main() {
  // Change result[0] as a control to confirm (before code evolution) that this
  // code runs.
  results[0] = 20;
  // Create a sufficient number of instructions in this template program to be
  // modified during the evolution of the program. The nop instructions
  // (mutation "landing pads") are interspersed with the vocabulary sections
  // below.
  //
  // The evolution using simple mutators may proceed via repurposing existing
  // functionality (e.g. modifying the operations and addressing modes below to
  // affect different locations in inputs and results) rather than creating new
  // valid instructions from scratch - as the latter may be unlikely to achieve
  // via e.g. random bitflips.

  // --------------------------------------------------------------------------
  // Section 1: arithmetic and control flow between dummy variables (carried
  // over from the intermediate templates).
  // --------------------------------------------------------------------------
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
  // land - each must land inside the nop sled that follows the skipped block,
  // never in the middle of an instruction) to allow for an accumulation of
  // "silent mutations" in code that is not currently executed. This code may
  // become executed when the jumps are modified or when (parts of) this
  // non-executed code is "recombined" into an executed code of another (or the
  // same) program.
  asm("jmp . + 145");
  dummy[3] = dummy[4];
  dummy[6] = 4 * dummy[2];
  if (dummy[3] > dummy[5]) {
    dummy[5] = dummy[3];
  }
  dummy[7] = dummy[8] / dummy[9];
  NOPS;
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
  dummy[3] = dummy[4];
  if (dummy[22] && dummy[23] && dummy[24]) {
    dummy[21] = !dummy[21];
  }
  NOPS;
  dummy[4] = dummy[5];
  if (dummy[25] || dummy[26] || dummy[27]) {
    dummy[28] &= dummy[21];
    dummy[29] |= dummy[21];
    dummy[30] = dummy[28] ^ dummy[29];
  }
  NOPS;

  // --------------------------------------------------------------------------
  // Section 2: loads and stores touching inputs, results, and scratchspace
  // (RECOMMENDATIONS.md 4.1: I/O-touching vocabulary). All stores into results
  // read from locations holding -1 in the template image (dummy[113],
  // dummy[121], scratchspace[123] and inputs[13] are not written anywhere in
  // this template), so an unevolved execution leaves results visibly unchanged
  // - the instructions are "neutral vocabulary" for evolution to repurpose.
  // --------------------------------------------------------------------------
  dummy[33] = inputs[5];
  dummy[34] = inputs[42];
  scratchspace[10] = inputs[100];
  scratchspace[11] = dummy[34];
  NOPS;
  inputs[199] = dummy[35];
  inputs[198] = scratchspace[12];
  NOPS;
  results[2] = dummy[113];
  results[5] = scratchspace[123];
  results[7] = inputs[13];
  results[8] = dummy[121];
  NOPS;
  dummy[36] = results[3];
  scratchspace[13] = results[6];
  NOPS;
  asm("jmp . + 129");
  dummy[33] = inputs[57];
  scratchspace[14] = inputs[123];
  results[4] = dummy[115];
  results[9] = scratchspace[125];
  NOPS;
  NOPS;
  NOPS;
  dummy[35] = results[1];
  NOPS;

  // --------------------------------------------------------------------------
  // Section 3: a balanced mix of arithmetic and bitwise operations
  // (RECOMMENDATIONS.md 4.1: uniform coverage of + - * & | ^ << >>, plus the
  // division and modulo already present in Section 1).
  // --------------------------------------------------------------------------
  dummy[40] = dummy[41] + dummy[42];
  dummy[43] = dummy[44] - dummy[45];
  dummy[46] = dummy[47] * dummy[48];
  dummy[49] = dummy[50] & dummy[51];
  NOPS;
  dummy[52] = dummy[53] | dummy[54];
  dummy[55] = dummy[56] ^ dummy[57];
  dummy[58] = dummy[59] << 3;
  dummy[60] = dummy[61] >> 2;
  NOPS;
  scratchspace[20] = scratchspace[21] + inputs[7];
  scratchspace[22] = inputs[8] - scratchspace[23];
  scratchspace[24] = inputs[9] & 0xFF;
  scratchspace[25] = inputs[10] ^ scratchspace[26];
  NOPS;

  // --------------------------------------------------------------------------
  // Section 4: register-register operations (RECOMMENDATIONS.md 4.1). The
  // inline asm with register constraints emits genuine reg-reg encodings so
  // mutations can stumble into register-based computation, which is cheaper
  // than memory round-trips.
  // --------------------------------------------------------------------------
  int reg_a = dummy[37];
  int reg_b = scratchspace[30];
  asm("add %1, %0" : "+r"(reg_a) : "r"(reg_b));
  asm("xor %1, %0" : "+r"(reg_b) : "r"(reg_a));
  asm("sub %1, %0" : "+r"(reg_a) : "r"(reg_b));
  NOPS;
  asm("and %1, %0" : "+r"(reg_b) : "r"(reg_a));
  asm("or %1, %0" : "+r"(reg_a) : "r"(reg_b));
  asm("imul %1, %0" : "+r"(reg_b) : "r"(reg_a));
  dummy[38] = reg_a;
  scratchspace[31] = reg_b;
  NOPS;

  // --------------------------------------------------------------------------
  // Section 5: immediate-bearing instructions (RECOMMENDATIONS.md 4.1). The
  // full 4-byte immediate fields are smooth mutation targets: flipping a bit
  // in an immediate changes the constant while keeping the instruction valid.
  // --------------------------------------------------------------------------
  dummy[39] = 0x55AA33CC;
  scratchspace[32] = 0x0F0F0F0F;
  dummy[31] += 0x00010001;
  NOPS;
  scratchspace[33] ^= 0x33333333;
  scratchspace[34] &= 0x7FFFFFFF;
  scratchspace[35] |= 0x40404040;
  NOPS;
  int reg_imm;
  asm("mov $0x5A5A5A5A, %0" : "=r"(reg_imm));
  dummy[32] = reg_imm;
  NOPS;

  // --------------------------------------------------------------------------
  // Section 6: AVX2 vector operations.
  // --------------------------------------------------------------------------
  dummy[70] = 1; dummy[71] = 2; dummy[72] = 3; dummy[74] = 4; dummy[75] = 5; dummy[77] = 9;
  __m256i vec_a = _mm256_loadu_si256((__m256i*)&dummy[62]);
  __m256i vec_b = _mm256_loadu_si256((__m256i*)&dummy[70]);
  __m256i vec_add = _mm256_add_epi32(vec_a, vec_b);
  _mm256_storeu_si256((__m256i*)&dummy[78], vec_add);
  NOPS;
  dummy[86] = 1; dummy[87] = -2; dummy[88] = 3; dummy[90] = -4; dummy[91] = 5; dummy[93] = -9;
  __m256i vec_x = _mm256_loadu_si256((__m256i*)&dummy[86]);
  __m256i vec_y = _mm256_loadu_si256((__m256i*)&dummy[94]);
  __m256i vec_mul = _mm256_mullo_epi32(vec_x, vec_y);
  _mm256_storeu_si256((__m256i*)&dummy[102], vec_mul);
  NOPS;
  __m256i vec_p = _mm256_loadu_si256((__m256i*)&scratchspace[40]);
  __m256i vec_q = _mm256_loadu_si256((__m256i*)&scratchspace[48]);
  __m256i vec_xor = _mm256_xor_si256(vec_p, vec_q);
  _mm256_storeu_si256((__m256i*)&scratchspace[56], vec_xor);
  NOPS;

  // --------------------------------------------------------------------------
  // Section 7: nop-dense rounds interspersed with simple assignments and
  // unreachable stretches behind unconditional jumps ("junk DNA" for silent
  // mutations and recombination material).
  // --------------------------------------------------------------------------
  dummy[5] = dummy[6];
  NOPS;
  dummy[6] = dummy[7];
  NOPS;
  dummy[7] = dummy[8];
  NOPS;
  dummy[8] = dummy[9];
  NOPS;
  asm("jmp . + 127");
  dummy[9] = dummy[10];
  NOPS;
  dummy[1] = dummy[2];
  NOPS;
  dummy[2] = dummy[3];
  NOPS;
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
  asm("jmp . + 127");
  dummy[9] = dummy[10];
  NOPS;
  dummy[1] = dummy[2];
  NOPS;
  dummy[2] = dummy[3];
  NOPS;
  dummy[3] = dummy[4];
  NOPS;
  dummy[4] = dummy[5];
  NOPS;
  dummy[5] = dummy[6];
  NOPS;

  return 0;
}
