// "C:/home/NXP/direct/flog/ksd_Nes.cpp"

#include <ksd_Nes.hpp>

namespace ksd {

    void FileSystem::Initialize(FileHeader *fs_start, size_t fs_size) {
        this->fs_start = reinterpret_cast<u8*>(fs_start);
        this->fs_size = fs_size;

        /* Ensure the fs is valid. */
        NN_ASSERT(fs_size > sizeof(*fs_start));

        /* Since all the 0x20 file headers go first, the first header's file offset will be the total size of all the headers. */
        const auto header_size = fs_start->file_offset;
        NN_ASSERT(!(header_size % sizeof(*fs_start)));
        NN_ASSERT(fs_size > header_size);

        this->file_count = header_size / sizeof(*fs_start);
        NN_ASSERT(this->file_count);

        NN_ASSERT(fs_start->file_offset < fs_size);

        for(int i = 0; i < this->file_count; i++) {
            const auto data_end = fs_start->file_offset + this->GetFileSize(i);
            NN_ASSERT(data_end <= fs_size);
        }
    }

    size_t FileSystem::GetFileSize(int id) {
        NN_ASSERT(id < this->file_count);
        const auto file_header = this->GetFileHeader(id);
        return file_header->prg_bank_count * Size16KB + file_header->chr_bank_count * Size8KB;
    }

    FileHeader *FileSystem::GetFileHeader(int id) {
        NN_ASSERT(id < this->file_count);
        const auto file_headers = reinterpret_cast<FileHeader*>(this->fs_start);
        return &file_headers[id];
    }

    u8 *FileSystem::GetFileData(int id) {
        NN_ASSERT(id < this->file_count);
        const auto file_header = this->GetFileHeader(id);
        return this->fs_start + file_header->file_offset;
    }

}