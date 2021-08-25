
#pragma once
#include <base_Includes.hpp>

namespace ksd {

    constexpr size_t SizeKB = 0x400;
    constexpr size_t Size8KB = 8 * SizeKB;
    constexpr size_t Size16KB = 16 * SizeKB;

    struct FileHeader {
        u32 file_offset;
        u8 prg_bank_count;
        u8 chr_bank_count;
        u16 unk_1;
        u32 unk_2;
        u32 unk_3;
        u32 unk_4;
        u32 unk_5;
        u32 unk_6;
        u32 unk_7;
    };

    struct FileSystem {
        u8 *fs_start;
        size_t fs_size;
        int file_count;

        void Initialize(FileHeader *fs_start, size_t fs_size);
        size_t GetFileSize(int id);
        FileHeader *GetFileHeader(int id);
        u8 *GetFileData(int id);
    };

}