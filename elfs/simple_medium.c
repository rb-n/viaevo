// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

// The ELF of this simple program is intended to serve as a template for
// evolving the program to meet a computational objective.
//
// The "simple" part of simple_small refers to only simple operations being
// present in the main function (in this case only assignments between dummy
// variables and uncoditional relative jumps). The "medium" part of
// simple_medium refers to medium sizes of the dummy, results, and inputs arrays
// and the presence of scratchspace.

#define NOPS                                                                   \
  asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")

// Define some initialized data where the inputs for the desired computational
// tasks will be placed and from where the results of the tasks will be read.
// This space may also serve as a "scratch space" for the program.
int dummy[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
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
  };
// clang-format on

int main() {
  // Change result[0] as a control to confirm (before code evolution) that this
  // code runs.
  results[0] = 20;
  // Create a sufficient number instructions in this template program to be
  // modified during the evolution of the program. Ideally(?), these would be
  // nop instuctions only, but only a short stretch of nop instructions is
  // visible in the ELF if only nop instructions are added here. The nop
  // instuctructions are therefore interspersed with other operations.
  //
  // The evolution using simple mutators may proceed via repurposing existing
  // functionality (e.g. modifying the assignents between dummy variables below
  // to affect inputs and results) rather than creating new valid instructions.
  // Adding additional different operations between dummy variables (as in e.g.
  // intermediate_small) may enable the evolution of more involved computtional
  // tasks if this is the case.
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
