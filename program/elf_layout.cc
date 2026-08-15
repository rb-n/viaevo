// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "elf_layout.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace viaevo {

namespace {

void myfail(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

} // namespace

SymbolData ResolveElfSymbolData(int fd) {
  if (fd == -1)
    myfail("invalid fd");

  SymbolData symbol_data;

  ssize_t nread;

  // Read the ELF header.
  Elf64_Ehdr ehdr;
  off_t offset = lseek(fd, 0, SEEK_SET);
  if (offset != 0)
    myfail("lseek to 0 failed");

  nread = read(fd, &ehdr, sizeof(ehdr));
  if (nread != sizeof(ehdr))
    myfail("read ehdr failed");

  // Read section headers.
  std::vector<Elf64_Shdr> shdrs(ehdr.e_shnum);
  offset = lseek(fd, ehdr.e_shoff, SEEK_SET);
  if (offset != (off_t)ehdr.e_shoff)
    myfail("lseek to ehdr.e_shoff failed");
  nread = read(fd, shdrs.data(), ehdr.e_shnum * ehdr.e_shentsize);
  if (nread != ehdr.e_shnum * ehdr.e_shentsize)
    myfail("read shdrs failed");

  // Read the section header string table.
#define SBUF_SIZE 512
  char sbuf[SBUF_SIZE];
  if (shdrs[ehdr.e_shstrndx].sh_size >= sizeof(sbuf))
    myfail("sbuf too small for shstrtab");

  offset = lseek(fd, shdrs[ehdr.e_shstrndx].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[ehdr.e_shstrndx].sh_offset)
    myfail("lseek to shdrs[ehdr.e_shstrndx].sh_offset failed");

  nread = read(fd, sbuf, shdrs[ehdr.e_shstrndx].sh_size);
  if (nread != (off_t)shdrs[ehdr.e_shstrndx].sh_size)
    myfail("section header string table read failed");

  std::unordered_map<std::string, int> name_to_shdrs_index;
  for (int i = 0; i < (int)shdrs.size(); ++i) {
    std::string str(&sbuf[shdrs[i].sh_name]);
    name_to_shdrs_index[str] = i;
  }

  // Read the symbol table.
  if (name_to_shdrs_index.count(".symtab") < 1)
    myfail(".symtab not found");
  int symtab_index = name_to_shdrs_index[".symtab"];

  if (shdrs[symtab_index].sh_size % sizeof(Elf64_Sym) != 0)
    myfail("symtab size misunderstood");

  int symtab_num = shdrs[symtab_index].sh_size / sizeof(Elf64_Sym);
  std::vector<Elf64_Sym> syms(symtab_num);

  offset = lseek(fd, shdrs[symtab_index].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[symtab_index].sh_offset)
    myfail("lseek to shdrs[symtab_index].sh_offset failed");

  nread = read(fd, syms.data(), shdrs[symtab_index].sh_size);
  if (nread != (ssize_t)shdrs[symtab_index].sh_size)
    myfail("read symtab failed");

  // Read the symbol table string table.
  if (name_to_shdrs_index.count(".strtab") < 1)
    myfail(".strtab not found");
  int strtab_index = name_to_shdrs_index[".strtab"];

  if (shdrs[strtab_index].sh_size >= sizeof(sbuf))
    myfail("sbuf too small for strtab");

  offset = lseek(fd, shdrs[strtab_index].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[strtab_index].sh_offset)
    myfail("lseek to shdrs[ehdr.e_shstrndx].sh_offset failed");

  nread = read(fd, sbuf, shdrs[strtab_index].sh_size);
  if (nread != (ssize_t)shdrs[strtab_index].sh_size)
    myfail("symbol table string table read failed");

  std::unordered_map<std::string, int> name_to_syms_index;
  for (int i = 0; i < (int)syms.size(); ++i) {
    std::string str(&sbuf[syms[i].st_name]);
    name_to_syms_index[str] = i;
  }

  if (name_to_shdrs_index.count(".text") < 1)
    myfail("section header .text not found");
  int text_index = name_to_shdrs_index[".text"];

  if (name_to_syms_index.count("main") < 1)
    myfail("symbol main not found");
  int main_index = name_to_syms_index["main"];

  symbol_data.main_offset_in_text_ =
      syms[main_index].st_value - shdrs[text_index].sh_addr;
  // NOTE: Risk of underflow for an unsigned variable? (Same below for inputs.)
  symbol_data.main_offset_in_elf_ =
      syms[main_index].st_value -
      (shdrs[text_index].sh_addr - shdrs[text_index].sh_offset);
  symbol_data.main_st_size_ = syms[main_index].st_size;

  if (name_to_shdrs_index.count(".data") < 1)
    myfail("section header .data not found");
  int data_index = name_to_shdrs_index[".data"];

  if (name_to_syms_index.count("inputs") < 1)
    myfail("symbol inputs not found");
  int inputs_index = name_to_syms_index["inputs"];

  symbol_data.inputs_offset_in_elf_ =
      syms[inputs_index].st_value -
      (shdrs[data_index].sh_addr - shdrs[data_index].sh_offset);
  symbol_data.inputs_st_size_ = syms[inputs_index].st_size;

  if (name_to_syms_index.count("data_start") < 1)
    myfail("symbol data_start not found");
  int data_start_index = name_to_syms_index["data_start"];

  if (name_to_syms_index.count("results") < 1)
    myfail("symbol results not found");
  int results_index = name_to_syms_index["results"];
  symbol_data.results_offset_in_data_ =
      syms[results_index].st_value - syms[data_start_index].st_value;
  symbol_data.results_st_size_ = syms[results_index].st_size;

  return symbol_data;
}

} // namespace viaevo
