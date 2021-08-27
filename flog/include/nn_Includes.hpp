
#pragma once

#define NN_ABORT(...) (void)(__VA_ARGS__)
#define NN_ASSERT(...) (void)(__VA_ARGS__)
#define NN_RESULT_ASSERT(...) (void)(__VA_ARGS__)

extern "C" {
    void _nnmusl_init_dso(/* ... */);
    void _nnmusl_fini_dso(/* ... */);
    void init_libc0(/* ... */);
    void init_libc1(/* ... */);
    void init_libc2(/* ... */);
}

template<size_t Size>
struct Stubbed {
    u8 data[Size];
};

namespace nn {

    struct Result {
        u32 value;
    };

    namespace os {

        struct MutexType : Stubbed<0x20> {};
        struct Mutex : Stubbed<0x20> {};
        struct SystemEventType : Stubbed<0x30> {};
        struct SystemEvent : Stubbed<0x30> {};

        struct MemoryInfo {
            size_t a;
            size_t b;
        };

        void InitializeMutex(MutexType *mutex, bool recursive, int lock_level);

        u64 GetSystemTick();

        void QueryMemoryInfo(MemoryInfo *out_info);

        Result SetMemoryHeapSize(size_t heap_size);
        Result AllocateMemoryBlock(void **out_block_addr, size_t size);
        void SetMemoryAllocatorForThreadLocal(void*(*alloc_fn)(size_t, size_t), void(*free_fn)(void*, size_t));

    }

    namespace oe {

        void Initialize();

    }

    namespace aoc {

        void Initialize();
        void Finalize();

    }

    namespace fs {

        void SetLocalAccessLog(bool enabled);
        void SetResultHandledByApplication(bool handled);
        void SetAllocator(void*(*alloc_fn)(size_t), void(*free_fn)(void*, size_t));
        void Unmount(const char *name);

    }

    namespace pctl {

        void Initialize();

    }

    namespace diag {

        void InitializeApplicationAbortObserver();

    }

    namespace mem {

        class StandardAllocator : Stubbed<0x20> {
            public:
                StandardAllocator();

                void Initialize(void *buf, size_t size);
                void Finalize();
                void *Allocate(size_t size, size_t align);
                void *Allocate(size_t size);
                void Free(void *ptr);
                void *Reallocate(void *ptr, size_t size);
                size_t GetAllocatableSize();
        };

    }

    namespace audio {

        struct AudioDeviceName {
            char name[0xFF];
        };

        struct AudioOutParameter : Stubbed<0x8> {};

        struct AudioOut {

        };

        int ListAudioDeviceName(AudioDeviceName *names, int name_count);
        void SetAudioDeviceOutputVolume(const AudioDeviceName *name, float volume);
        void InitializeAudioOutParameter(AudioOutParameter *out_param);
        Result OpenDefaultAudioOut(AudioOut *out, nn::os::SystemEvent *out_event, const AudioOutParameter &param);
        void StartAudioOut(AudioOut *out);

    }

}

namespace nv {

    void SetGraphicsAllocator(void*(*alloc_fn)(size_t, size_t, void*), void(*free_fn)(void*, void*), void*(*realloc_fn)(void*, size_t, void*), void *user_data);
    void SetGraphicsDevtoolsAllocator(void*(*alloc_fn)(size_t, size_t, void*), void(*free_fn)(void*, void*), void*(*realloc_fn)(void*, size_t, void*), void *user_data);
    void InitializeGraphics(void *buf, size_t size);

}