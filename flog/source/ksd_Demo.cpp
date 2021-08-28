// "C:/home/NXP/direct/flog/ksd_Demo.cpp"

#include <ksd_Demo.hpp>

nn::mem::StandardAllocator g_Allocator;

nn::audio::AudioOut g_AudioOut;
nn::os::SystemEvent g_SystemEvent;
float g_SomeFloat;
void *g_MonoBuffer;
void *g_StereoBuffer;
u64 g_SystemTick;

nn::os::MutexType g_SomeMutex2;

namespace ksd {

    namespace {

        u8 *g_AllocatorBuffer = nullptr;
        size_t g_AllocatableSize = 0;

        void *FsAllocateFunction(size_t size) {
            return g_Allocator.Allocate(size, 4);
        }

        void FsFreeFunction(void *ptr, size_t size) {
            if(ptr != nullptr) {
                g_Allocator.Free(ptr);
            }
        }

        void *GraphicsAllocateFunction(size_t size, size_t align, void *user_data) {
            return aligned_alloc(align, size);
        }

        void GraphicsFreeFunction(void *ptr, void *user_data) {
            free(ptr);
        }

        void *GraphicsReallocateFunction(void *ptr, size_t size, void *user_data) {
            return realloc(ptr, size);
        }

        void InitializeImpl(float unk) {
            g_SomeFloat = unk;
            // std::memset()
            g_MonoBuffer = g_Allocator.Allocate(0x4000, 0x1000);
            NN_ASSERT(g_MonoBuffer != nullptr);

            g_StereoBuffer = g_Allocator.Allocate(0x8000, 0x1000);
            NN_ASSERT(g_StereoBuffer != nullptr);

            nn::audio::AudioDeviceName device_names[3] = {};
            const auto device_name_count = nn::audio::ListAudioDeviceName(device_names, countof(device_names));
            if(device_name_count > 0) {
                for(int i = 0; i < device_name_count; i++) {
                    const auto cur_device_name = device_names[i];
                    if(strcmp(cur_device_name.name, "AudioTvOutput") == 0) {
                        nn::audio::SetAudioDeviceOutputVolume(&cur_device_name, 1.0f);
                    }
                    else {
                        nn::audio::SetAudioDeviceOutputVolume(&cur_device_name, 2.0f);
                    }
                }
            }

            nn::audio::AudioOutParameter param = {};
            nn::audio::InitializeAudioOutParameter(&param);
            NN_RESULT_ASSERT(nn::audio::OpenDefaultAudioOut(&g_AudioOut, &g_SystemEvent, param));
            NN_RESULT_ASSERT(nn::audio::StartAudioOut(&g_AudioOut));
        }

        void NvInitialize(size_t size, void*(*alloc_fn)(size_t, size_t, void*), void(*free_fn)(void*, void*), void*(*realloc_fn)(void*, size_t, void*), void *user_data) {
            if((alloc_fn != nullptr) && (free_fn != nullptr) && (realloc_fn != nullptr)) {
                auto gfx_buf = alloc_fn(size, 1, user_data);
                nv::SetGraphicsAllocator(alloc_fn, free_fn, realloc_fn, user_data);
                nv::SetGraphicsDevtoolsAllocator(alloc_fn, free_fn, realloc_fn, user_data);
                nv::InitializeGraphics(gfx_buf, size);
            }
            else {
                auto gfx_buf = aligned_alloc(1, size);
                nv::SetGraphicsAllocator(GraphicsAllocateFunction, GraphicsFreeFunction, GraphicsReallocateFunction, nullptr);
                nv::SetGraphicsDevtoolsAllocator(GraphicsAllocateFunction, GraphicsFreeFunction, GraphicsReallocateFunction, nullptr);
                nv::InitializeGraphics(gfx_buf, size);
            }
        }

        void *NvAllocateFunction(size_t size, size_t align, void *user_data) {
            return g_Allocator.Allocate(size, align);
        }

        void NvFreeFunction(void *ptr, void *user_data) {
            if(ptr != nullptr) {
                g_Allocator.Free(ptr);
            }
        }

        void *NvReallocateFunction(void *ptr, size_t size, void *user_data) {
            return g_Allocator.Reallocate(ptr, size);
        }

        float EstimateCpuFrequencyOnce() {
            const auto freq = nn::os::GetSystemTickFrequency(); 

            const auto timer_begin = nn::os::GetSystemTick();
            auto left_iters = 1000000;
            auto keep = true;
            do {
                keep = left_iters > 2;
                left_iters -= 2;
            } while(keep);
            const auto timer_end = nn::os::GetSystemTick();
            NN_ASSERT((timer_end - timer_begin) > 0);

            return static_cast<double>(1000000 * freq) / static_cast<double>(timer_end - timer_begin);
        }

    }

    void Initialize() {
        g_SystemTick = nn::os::GetSystemTick();
        nn::oe::Initialize();

        g_AllocatorBuffer = new u8[0x20000000ul]();
        g_Allocator.Initialize(g_AllocatorBuffer, 0x20000000ul);
        g_AllocatableSize = g_Allocator.GetAllocatableSize();

        InitializeImpl(48000.0f);

        nn::os::InitializeMutex(&g_SomeMutex2, false, 0);
        nn::fs::SetAllocator(FsAllocateFunction, FsFreeFunction);

        NvInitialize(0x10000000ul, NvAllocateFunction, NvFreeFunction, NvReallocateFunction, nullptr);

        /* ... */
    }

}