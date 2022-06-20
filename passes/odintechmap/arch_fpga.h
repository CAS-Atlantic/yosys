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

#ifndef ARCH_FPGA_H
#define ARCH_FPGA_H

#include "kernel/yosys.h"

#include "libs/libarchfpga/src/read_xml_arch_file.h"

YOSYS_NAMESPACE_BEGIN

namespace ODIN
{
    class ArchFpga
    {
    public:
    ArchFpga();
    void read_arch_file(std::string arch_file_path);
    void log_arch_info();

    private:
    t_arch Arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;
    short physical_lut_size = -1;

    
    void set_physical_lut_size();
    void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode);
    void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type);
    };
}

YOSYS_NAMESPACE_END

#endif
