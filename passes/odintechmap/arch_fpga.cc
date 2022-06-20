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

ODIN::ArchFpga::ArchFpga() {
    
}

void ODIN::ArchFpga::log_arch_info() {
    t_model* hb = Arch.models;
    int i=1;
    while (hb) {
        log("hard block #%d: %s\n", i++, hb->name);
        hb = hb->next;
    }
}

void ODIN::ArchFpga::read_arch_file(std::string arch_file_path) {
    try {
        log("Reading FPGA Architecture file: %s\n", arch_file_path.c_str());
        XmlReadArch(arch_file_path.c_str(), false, &Arch, physical_tile_types, logical_block_types);
    } catch (vtr::VtrError& vtr_error) {
        log_error("Failed to load architecture file: %s\n", vtr_error.what());
    }

    set_physical_lut_size();
    log("Using Lut input width of: %d\n", physical_lut_size);
}

void ODIN::ArchFpga::set_physical_lut_size() {
    std::vector<t_pb_type*> pb_lut_list;

    for (t_logical_block_type& logical_block : logical_block_types) {
        if (logical_block.index != EMPTY_TYPE_INDEX) {
            get_physical_luts(pb_lut_list, logical_block.pb_type);
        }
    }
    for (t_pb_type* pb_lut : pb_lut_list) {
        if (pb_lut) {
            if (pb_lut->num_input_pins < physical_lut_size || physical_lut_size < 1) {
                physical_lut_size = pb_lut->num_input_pins;
            }
        }
    }
}

void ODIN::ArchFpga::get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode) {
    for (int i = 0; i < mode->num_pb_type_children; i++) {
        get_physical_luts(pb_lut_list, &mode->pb_type_children[i]);
    }
}

void ODIN::ArchFpga::get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type) {
    if (pb_type) {
        if (pb_type->class_type == LUT_CLASS) {
            pb_lut_list.push_back(pb_type);
        } else {
            for (int i = 0; i < pb_type->num_modes; i++) {
                get_physical_luts(pb_lut_list, &pb_type->modes[i]);
            }
        }
    }
}

YOSYS_NAMESPACE_END
