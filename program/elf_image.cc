// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "elf_image.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <type_traits>

namespace viaevo {

namespace {

void myfail(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

} // namespace

ElfImage::ElfImage(const char *filename) {
  SetupInMemory(filename);
  symbol_data_ = ResolveElfSymbolData(fd_);
}

ElfImage::~ElfImage() {
  if (fd_ != -1)
    close(fd_);
}

bool ElfImage::IsInitialized() const {
  return (fd_ != -1 && symbol_data_.main_offset_in_elf_ != (Elf64_Addr)-1 &&
          symbol_data_.main_st_size_ != (uint64_t)-1 &&
          symbol_data_.results_offset_in_data_ != (Elf64_Addr)-1 &&
          symbol_data_.results_st_size_ != (uint64_t)-1);
}

void ElfImage::SetupInMemory(const char *filename) {
  int fd_from;

  if (fd_ != -1)
    close(fd_);

  // In memory files for all instances of ElfImage have the same filename. This
  // should be ok as per memfd_create(2): "... as such multiple files can have
  // the same name without any side effects."
  fd_ = memfd_create("viaevo_program", 0);
  if (fd_ == -1)
    myfail("memfd_create failed");

  fd_from = open(filename, O_RDONLY);
  if (fd_from == -1)
    myfail("open failed");

  WriteFile(fd_from, fd_);

  close(fd_from);
}

void ElfImage::WriteFile(int fd_from, int fd_to) {
  char buffer[4096];
  ssize_t nread;

  // Based on https://stackoverflow.com/a/2180788
  while (nread = read(fd_from, buffer, sizeof buffer), nread > 0) {
    char *out_ptr = buffer;
    ssize_t nwritten;

    do {
      nwritten = write(fd_to, out_ptr, nread);
      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR) {
        myfail("write failed");
      }
    } while (nread > 0);
  }
}

void ElfImage::Save(const char *filename) {
  int fd_to;

  if (fd_ == -1)
    myfail("invalid elf image fd");

  off_t offset = lseek(fd_, 0, SEEK_SET);
  if (offset != 0)
    myfail("lseek to 0 failed");

  fd_to = creat(filename, 0666);
  if (fd_to == -1)
    myfail("creat failed");

  WriteFile(fd_, fd_to);

  close(fd_to);
}

std::vector<char> ElfImage::GetCode() const {
  if (symbol_data_.main_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to get main unknown");

  off_t offset = lseek(fd_, symbol_data_.main_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.main_offset_in_elf_)
    myfail("get elf code lseek failed");

  std::vector<char> elf_code(symbol_data_.main_st_size_);
  ssize_t nread = read(fd_, elf_code.data(), symbol_data_.main_st_size_);
  if (nread != (ssize_t)symbol_data_.main_st_size_)
    myfail("getting elf code failed");

  return elf_code;
}

void ElfImage::SetCode(const std::vector<char> &code) {
  if (code.size() != symbol_data_.main_st_size_)
    myfail("elf code to set has incorrect size");

  if (symbol_data_.main_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to set main unknown");

  off_t offset = lseek(fd_, symbol_data_.main_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.main_offset_in_elf_)
    myfail("set elf code lseek failed");

  ssize_t nwritten = write(fd_, code.data(), code.size());
  if (nwritten != (off_t)code.size())
    myfail("setting elf code failed");
}

void ElfImage::SetCodeToAllNops() {
  std::vector<char> new_elf_code(symbol_data_.main_st_size_, 0x90);
  SetCode(new_elf_code);
}

std::vector<int> ElfImage::GetInputs() const {
  if (symbol_data_.inputs_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to get inputs unknown");

  std::vector<int> elf_inputs;

  if (symbol_data_.inputs_st_size_ % sizeof(decltype(elf_inputs)::value_type) !=
      0)
    myfail("inputs_st_size_ mismatch");

  elf_inputs.resize(symbol_data_.inputs_st_size_ /
                    sizeof(decltype(elf_inputs)::value_type));

  off_t offset = lseek(fd_, symbol_data_.inputs_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.inputs_offset_in_elf_)
    myfail("get elf inputs lseek failed");

  ssize_t nread = read(fd_, elf_inputs.data(), symbol_data_.inputs_st_size_);
  if (nread != (ssize_t)symbol_data_.inputs_st_size_)
    myfail("getting elf inputs failed");

  return elf_inputs;
}

void ElfImage::SetInputs(const std::vector<int> &inputs) {
  if (symbol_data_.inputs_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to set inputs unknown");

  auto element_size =
      sizeof(std::remove_reference_t<decltype(inputs)>::value_type);

  if (symbol_data_.inputs_st_size_ % element_size != 0)
    myfail("inputs_st_size_ mismatch");

  if (inputs.size() * element_size > symbol_data_.inputs_st_size_)
    myfail("elf inputs to set are too large");

  off_t offset = lseek(fd_, symbol_data_.inputs_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.inputs_offset_in_elf_)
    myfail("set elf inputs lseek failed");

  ssize_t nwritten =
      write(fd_, inputs.data(), inputs.size() * element_size);
  if (nwritten != (off_t)(inputs.size() * element_size))
    myfail("setting elf inputs failed");
}

} // namespace viaevo
