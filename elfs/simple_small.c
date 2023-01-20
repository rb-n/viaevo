// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

// The ELF of this simple program is intended to serve as a template for
// evolving the program to meet a computational objective.

#define NOPS                                                                   \
  asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")

// Define some initialized data where the inputs for the desired computational
// tasks will be placed and from where the results of the tasks will be read.
// This space may also serve as a "scratch space" for the program. Assign some
// values here so that these are easily recognizable in the ELF or in the
// process memory.
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
  // Create a suffiecient number instructions in this template program to be
  // modified during the evolution of the program. Ideally(?), these would be
  // nop instuctions only, but only a short stretch of nop instructions is
  // visible in the ELF if only nop instructions are added here. The nop
  // instuctructions are therefore interspersed with other operations.
  NOPS;
  dummy[1] = dummy[2];
  NOPS;
  dummy[2] = dummy[3];
  NOPS;
  // Add some unconditional relative jumps (and check via objdump where these
  // land) to allow for an accumulation of "silent mutations" in code that is
  // not currently executed. This code may become executed when the jumps are
  // modified or when (parts of) this non-executed code is "recombined" into an
  // executed code of another (or the same) program.
  asm("jmp . + 127");
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
  // 2nd round
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
  // 3rd round
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
  // 4th round
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
  // 5th round
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
  // 6th round
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
  // 7th round
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
  // 8th round
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
  // 9th round
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
  // 10th round
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
  // No jmp in this last round here.
  dummy[9] = dummy[10];
  NOPS;

  return 0;
}
