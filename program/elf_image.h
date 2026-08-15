// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_ELF_IMAGE_H_
#define VIAEVO_PROGRAM_ELF_IMAGE_H_

#include <vector>

#include "elf_layout.h"

namespace viaevo {

// ElfImage owns an ELF executable held in an in-memory file (memfd) together
// with the resolved offsets and sizes of its main, inputs and results symbols
// (SymbolData). It provides access to the evolvable code (main) and the inputs
// variable and can save the current image to disk. Pure data + ELF knowledge:
// no process is ever spawned here (execution lives in Program).
class ElfImage {
public:
  // Creates an empty, uninitialized image (used by Program's default
  // constructor intended for unit-test mocks).
  ElfImage() = default;
  // Copies the ELF in filename into a new in-memory file and resolves its
  // symbol data.
  explicit ElfImage(const char *filename);

  ElfImage(const ElfImage &) = delete;
  ElfImage &operator=(const ElfImage &) = delete;

  ~ElfImage();

  bool IsInitialized() const;

  // File descriptor of the in-memory ELF (e.g. for execveat).
  int fd() const { return fd_; }

  // Symbol table values and sizes for main, inputs and results (see
  // elf_layout.h). Resolved on construction.
  const SymbolData &symbol_data() const { return symbol_data_; }

  // Get and set the ELF's evolvable code (main).
  std::vector<char> GetCode() const;
  // Size of code must match the size of the ELF's evolvable code (main).
  void SetCode(const std::vector<char> &code);
  // Replace all instructions in the ELF's evolvable code (main) with nop
  // instructions.
  void SetCodeToAllNops();

  // Get and set the ELF's inputs variable.
  std::vector<int> GetInputs() const;
  // Size of inputs must be smaller or equal to the size of the ELF's inputs
  // variable.
  void SetInputs(const std::vector<int> &inputs);

  // Save the current in-memory elf into filename.
  void Save(const char *filename);

private:
  // Copies the ELF from filename to the in-memory file referenced by the fd_
  // member variable.
  void SetupInMemory(const char *filename);

  // Writes the contents of one file (file descriptor fd_from) into another
  // (file descriptor fd_to). Used by SetupInMemory and Save.
  static void WriteFile(int fd_from, int fd_to);

  // File descriptor of the (in memory) file containing the ELF being evolved.
  // This is where the code modifications happen and where inputs are updated
  // between executions.
  int fd_ = -1;

  SymbolData symbol_data_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_ELF_IMAGE_H_
