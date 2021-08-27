import os
import sys
import struct

src_start = """
/* KSD filesystem data source, auto-generated with build_ksd_fs.py */

#include <ksd_Nes.hpp>

namespace ksd {

    u8 g_FileSystemData[] = {
"""
src_end = """
    };

    constexpr size_t FileSystemDataSize = sizeof(g_FileSystemData);

    FileHeader *g_FileSystemStart = reinterpret_cast<FileHeader*>(g_FileSystemData);

}
"""
tab = "    "
data_line_start = 2 * tab
bytes_per_line = 16

size_16kb = 16 * 0x400
size_8kb = 8 * 0x400

def append_line(data_str, line_str):
    data_str += data_line_start
    data_str += line_str
    data_str += "\n"
    return data_str

def gen_src(data, out_src):
    data_str = ""
    line_str = ""

    i = 0
    for byte in data:
        line_str += "0x%0.2X" % byte + ", "
        
        i += 1
        if i >= bytes_per_line:
            data_str = append_line(data_str, line_str)
            line_str = ""
            i = 0
    
    if len(line_str) > 0:
        data_str = append_line(data_str, line_str)

    src_data = src_start + data_str + src_end

    with open(out_src, "w") as out_fd:
        out_fd.write(src_data)

def bit(n, b):
    return (n >> b) & 1

def add_file(file_count, header_data, file_data, path):
    print("Adding ROM '" + path + "'...")
    file_size = os.path.getsize(path)

    with open(path, "rb") as fd:
        rom_header = fd.read(0x10)

        prg_bank_count, chr_bank_count, f6, f7 = struct.unpack("<xxxxBBBBxxxxxxxx", rom_header)
        magic = rom_header[:4]
        if magic != b"NES\x1A":
            print(" -- Invalid ROM iNES header!")
            return False

        # Has trainer -> f6 bit 2
        has_trainer = bit(f6, 2)
        if has_trainer:
            print(" -- ROMs with trainer data are not supported!")
            return False

        # PlayChoice support: f7 bit 1
        has_playchoice = bit(f7, 1)
        if has_playchoice:
            print(" |- ROM has PlayChoice INST-ROM data, this data won't be included as it won't be used...")

        header_fmt = "<IBBHxxxxxxxxxxxxxxxxxxxxxxxx"
        header_size = struct.calcsize(header_fmt)

        # Size of all headers + all files before the current one
        file_offset = header_size * file_count + len(file_data)
        unk = 0x2AA # TODO: what is this value? not setting it / zeroing it crashes flog (error related with mapper 0xAA?)
        new_header = struct.pack(header_fmt, file_offset, prg_bank_count, chr_bank_count, unk)

        write_size = prg_bank_count * size_16kb + chr_bank_count * size_8kb
        if write_size > file_size:
            print(" -- Invalid ROM size!")
            return False

        header_data += new_header

        file = fd.read()
        file_data += file[:write_size]

    return True

def print_usage():
    print("Usage:")
    print(" -- Generate C++ source file: " + sys.argv[0] + " --cpp <input_dir> <output_cpp_src>")
    print(" -- Generate binary file: " + sys.argv[0] + " <input_dir> <output_bin>")

def main():
    if len(sys.argv) < 3:
        print_usage()
        return

    if sys.argv[1] == "--cpp":
        if len(sys.argv) < 4:
            print_usage()
            return

        gen_cpp = True
        in_dir = sys.argv[2]
        out_file = sys.argv[3]
    else:
        gen_cpp = False
        in_dir = sys.argv[1]
        out_file = sys.argv[2]

    header_data = bytearray()
    file_data = bytearray()

    dir_files = os.listdir(in_dir)
    dir_file_count = len(dir_files)
    if dir_file_count == 0:
        print("The input directory is empty!")
        return

    for file in dir_files:
        if not add_file(dir_file_count, header_data, file_data, os.path.join(in_dir, file)):
            return

    fs_data = header_data + file_data
    print("Final filesystem data size: " + hex(len(fs_data)))

    if gen_cpp:
        gen_src(fs_data, out_file)
        print("Generated filesystem C++ source at '" + out_file + "'...")
    else:
        with open(out_file, "wb") as out_fd:
            out_fd.write(fs_data)
        print("Generated filesystem binary file at '" + out_file + "'...")

if __name__ == "__main__":
    main()