/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2022  Daniel Khadivi <dani-kh@live.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"

#include "arch_fpga.h"
#include "libs/libarchfpga/src/read_xml_arch_file.h"

YOSYS_NAMESPACE_BEGIN

ODIN::ArchFpga::ArchFpga(std::string arch_file_path) {
    
    read_arch_file(arch_file_path);
    set_physical_lut_size();

}

void ODIN::ArchFpga::read_arch_file(std::string arch_file_path) {
    try {
        log("Reading FPGA Architecture file: %s\n", arch_file_path.c_str());
        XmlReadArch(arch_file_path.c_str(), false, &Arch, physical_tile_types, logical_block_types);
    } catch (vtr::VtrError& vtr_error) {
        log_error("Failed to load architecture file: %s\n", vtr_error.what());
    }
}

void ODIN::ArchFpga::set_physical_lut_size() {

}

YOSYS_NAMESPACE_END
