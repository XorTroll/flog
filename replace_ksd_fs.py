import struct
import lz4.block
import hashlib
import sys

def replace(nso, ksd_fs, out_new_nso):
    nso_header_fmt = "<xxxxIxxxxIIIIIIIIIIIIIxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIxxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIIII"
    nso_header_fmt_size = struct.calcsize(nso_header_fmt)

    mod_fmt = "<xxxxIxxxxIIIIII"
    mod_fmt_size = struct.calcsize(mod_fmt)

    with open(nso, "rb") as fd:
        nso_data = fd.read()

        magic = nso_data[:4]

        if magic != b"NSO0":
            print("Invalid NSO magic")
            return

        (version, flags,
        text_f_offset, text_m_offset, text_d_size,
        mod_name_offset,
        rodata_f_offset, rodata_m_offset, rodata_d_size,
        mod_name_size,
        data_f_offset, data_m_offset, data_d_size,
        bss_size,
        text_c_size, rodata_c_size, data_c_size,
        api_info_offset, api_info_size,
        dynstr_offset, dynstr_size,
        dynsym_offset, dynsym_size) = struct.unpack(nso_header_fmt, nso_data[:nso_header_fmt_size])

        module_id = nso_data[0x40:0x60]

        mod_name = nso_data[mod_name_offset:mod_name_offset + mod_name_size]
        
        text_hash = nso_data[0xA0:0xC0]
        rodata_hash = nso_data[0xC0:0xE0]
        data_hash = nso_data[0xE0:0x100]

        if flags & (1 << 0):
            print("Decompressing .text section...")
            text = lz4.block.decompress(nso_data[text_f_offset:text_f_offset + text_c_size], uncompressed_size=text_d_size, return_bytearray=True)
        else:
            text = nso_data[text_f_offset:text_f_offset + text_c_size]

        if flags & (1 << 1):
            print("Decompressing .rodata section...")
            rodata = lz4.block.decompress(nso_data[rodata_f_offset:rodata_f_offset + rodata_c_size], uncompressed_size=rodata_d_size, return_bytearray=True)
        else:
            rodata = nso_data[rodata_f_offset:rodata_f_offset + rodata_c_size]

        if flags & (1 << 2):
            print("Decompressing .data section...")
            data = lz4.block.decompress(nso_data[data_f_offset:data_f_offset + data_c_size], uncompressed_size=data_d_size, return_bytearray=True)
        else:
            data = nso_data[data_f_offset:data_f_offset + data_c_size]

        mod_offset, = struct.unpack("<I", text[:4])
        (magic_offset,
        dynamic_offset,
        bss_start_offset, bss_end_offset,
        eh_frame_hdr_start_offset, eh_frame_hdr_end_offset,
        rt_gen_mod_obj_offset) = struct.unpack(mod_fmt, text[mod_offset:mod_offset + mod_fmt_size])

        mod_magic = text[magic_offset:magic_offset + 4]
        if mod_magic != b"MOD0":
            print("Invalid MOD magic")
            return

        find_ksd_fs_start_bytes = bytearray(b'\x20\x00\x00\x00\x01\x01\xAA\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')

        ksd_fs_offset = rodata.find(find_ksd_fs_start_bytes)
        if ksd_fs_offset < 0:
            print("Unable to locate flog's built-in KSD filesystem...")
            return
        else:
            print("Located flog's built-in KSD FS at .rodata offset " + hex(ksd_fs_offset))
            rodata_half_1 = rodata[:ksd_fs_offset]
            rodata_half_2 = rodata[ksd_fs_offset + 0x6020:]

            ksd_fs_m_offset = ksd_fs_offset + rodata_m_offset

            ksd_fs_m_offset_bytes = struct.pack("<I", ksd_fs_m_offset)

            with open(ksd_fs, "rb") as ksd_fd:
                ksd_fs_data = ksd_fd.read()
            
            rodata_new = rodata_half_1 + ksd_fs_data + rodata_half_2
            rodata = rodata_new

            size_diff = len(rodata) - rodata_d_size

            if size_diff != 0:
                print("Using different-sized ROMs is not supported yet...")
                return

                # TODO: fix this mess, need to edit all adresses past the KSD FS addr for the executable to be fine...
    
                start = 0
                while True:
                    off = rodata.find(ksd_fs_m_offset_bytes, start)
                    size_of_entry = 3 * 0x8 # .got entries
                    got_entry_value, = struct.unpack("<Q", rodata[off + size_of_entry:off + size_of_entry + 0x8])
                    diff = got_entry_value - ksd_fs_m_offset
                    if diff == 0x6020:
                        print("Found .got KSD FS addr entry at .rodata offset " + hex(off))
                    
                        new_got_entry_value = got_entry_value + size_diff
                        rodata[off + size_of_entry:off + size_of_entry + 0x8] = struct.pack("<Q", new_got_entry_value)

                        offset = off + size_of_entry
                        while True:
                            got_entry_value, = struct.unpack("<Q", rodata[offset:offset + 0x8])
                            print("Next entry " + hex(got_entry_value) + " at " + hex(offset))
                            got_p = got_entry_value - rodata_m_offset
                            if got_entry_value > ksd_fs_m_offset:
                                if got_p < len(rodata):
                                    print("Modifying entry " + hex(got_entry_value) + "(non-mem " + hex(got_p) + ")")
                                    new_got_entry_value = got_entry_value + size_diff
                                    rodata[offset:offset + 0x8] = struct.pack("<Q", new_got_entry_value)
                                    offset += size_of_entry
                                else:
                                    print("End of .got at " + hex(offset) + "?")
                                    break
                            else:
                                break
                        break

                    start = off + 1
                    if off == -1:
                        print("Could not find .got entry of KSD FS addr -> can't continue...")
                        print("Are you sure this is the original, unmodified flog binary?")
                        return

            data_m_offset += size_diff
            dynamic_offset += size_diff
            bss_start_offset += size_diff
            bss_end_offset += size_diff
            eh_frame_hdr_start_offset += size_diff
            eh_frame_hdr_end_offset += size_diff
            rt_gen_mod_obj_offset += size_diff
            print("Size diff after replacing KSD FS: " + str(size_diff))

    with open(out_new_nso, "wb") as out_fd:
        mod_offset, = struct.unpack("<I", text[:4])
        text[mod_offset:mod_offset + mod_fmt_size] = struct.pack(mod_fmt, magic_offset,
        dynamic_offset,
        bss_start_offset, bss_end_offset,
        eh_frame_hdr_start_offset, eh_frame_hdr_end_offset,
        rt_gen_mod_obj_offset)

        text[magic_offset:magic_offset + 4] = b"MOD0"

        text_d_size = len(text)
        rodata_d_size = len(rodata)
        data_d_size = len(data)

        text_hash = hashlib.sha256(text).digest()
        rodata_hash = hashlib.sha256(rodata).digest()
        data_hash = hashlib.sha256(data).digest()

        if flags & (1 << 0):
            print("Compressing .text section...")
            text = lz4.block.compress(text, store_size=False, return_bytearray=True)

        if flags & (1 << 1):
            print("Compressing .rodata section...")
            rodata = lz4.block.compress(rodata, store_size=False, return_bytearray=True)

        if flags & (1 << 2):
            print("Compressing .data section...")
            data = lz4.block.compress(data, store_size=False, return_bytearray=True)

        text_c_size = len(text)
        rodata_c_size = len(rodata)
        data_c_size = len(data)

        text_f_offset = 0x100 + mod_name_size
        rodata_f_offset = text_f_offset + text_c_size
        data_f_offset = rodata_f_offset + rodata_c_size

        new_header = bytearray(0x100)

        new_header[:nso_header_fmt_size] = struct.pack(nso_header_fmt, version, flags,
        text_f_offset, text_m_offset, text_d_size,
        mod_name_offset,
        rodata_f_offset, rodata_m_offset, rodata_d_size,
        mod_name_size,
        data_f_offset, data_m_offset, data_d_size,
        bss_size,
        text_c_size, rodata_c_size, data_c_size,
        api_info_offset, api_info_size,
        dynstr_offset, dynstr_size,
        dynsym_offset, dynsym_size)

        new_header[:4] = b"NSO0"

        new_header[0x40:0x60] = module_id

        new_header[0xA0:0xC0] = text_hash
        new_header[0xC0:0xE0] = rodata_hash
        new_header[0xE0:0x100] = data_hash

        out_fd.write(new_header)
        out_fd.write(mod_name)
        out_fd.write(text)
        out_fd.write(rodata)
        out_fd.write(data)

        print("Written NSO!")

    print("Done!")

def main():
    if len(sys.argv) < 4:
        print("Usage: " + sys.argv[0] + " <in-flog-main> <in-ksd-fs-bin> <out-new-main>")

    in_main = sys.argv[1]
    in_ksd_fs = sys.argv[2]
    out_main = sys.argv[3]

    replace(in_main, in_ksd_fs, out_main)

if __name__ == "__main__":
    main()