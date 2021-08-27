// "C:/home/NXP/direct/flog/flog.cpp"

#include <ksd_Nes.hpp>
#include <ksd_Demo.hpp>

// ---

namespace {

    nn::mem::StandardAllocator g_HeapAllocator;
    void *g_HeapAddress;

    void InitializeHeapAllocator(void *heap_addr, size_t heap_size) {
        g_HeapAllocator.Initialize(heap_addr, heap_size);
        g_HeapAddress = heap_addr;
    }

    void *ThreadLocalAllocate(size_t size, size_t align) {
        if(g_HeapAddress != nullptr) {
            return g_HeapAllocator.Allocate(size, align);
        }
        else {
            return nullptr;
        }
    }

    void ThreadLocalFree(void *ptr, size_t _) {
        if(ptr != nullptr) {
            g_HeapAllocator.Free(ptr);
        }
    }

}

extern "C" {

    void *__wrap_malloc(size_t size) {
        if(g_HeapAddress == nullptr) {
            /* Note: N does not set errno in this case...? */
            return nullptr;
        }

        auto ptr = g_HeapAllocator.Allocate(size);
        if(ptr == nullptr) {
            errno = ENOMEM;
        }
        return ptr;
    }

    void __wrap_free(void *ptr) {
        if(ptr != nullptr) {
            g_HeapAllocator.Free(ptr);
        }
    }

    void *__wrap_calloc(size_t count, size_t size) {
        if(g_HeapAddress == nullptr) {
            errno = ENOMEM;
            return nullptr;
        }

        const auto mem_size = count * size;
        auto ptr = g_HeapAllocator.Allocate(mem_size);
        if(ptr == nullptr) {
            errno = ENOMEM;
            return ptr;
        }
    
        memset(ptr, 0, mem_size);
        return ptr;
    }

    void *__wrap_realloc(void *ptr, size_t size) {
        if(g_HeapAddress == nullptr) {
            /* Note: N does not set errno in this case...? */
            return nullptr;
        }

        auto new_ptr = g_HeapAllocator.Reallocate(ptr, size);
        if(new_ptr == nullptr) {
            errno = ENOMEM;
        }
        return new_ptr;
    }

    void *__wrap_aligned_alloc(size_t align, size_t size) {
        if(g_HeapAddress == nullptr) {
            /* Note: N does not set errno in this case...? */
            return nullptr;
        }

        auto ptr = g_HeapAllocator.Allocate(size, align);
        if(ptr == nullptr) {
            errno = ENOMEM;
        }
        return ptr;
    }

}

extern "C" void nninitStartup() {
    nn::os::MemoryInfo mem_info;
    nn::os::QueryMemoryInfo(&mem_info);

    const auto heap_size = (mem_info.a - mem_info.b) & 0xFFFFFFFFFFE00000;
    NN_RESULT_ASSERT(nn::os::SetMemoryHeapSize(heap_size));

    void *heap_addr;
    NN_RESULT_ASSERT(nn::os::AllocateMemoryBlock(&heap_addr, heap_size));

    InitializeHeapAllocator(heap_addr, heap_size);
    nn::os::SetMemoryAllocatorForThreadLocal(ThreadLocalAllocate, ThreadLocalFree);
}

// ---

extern "C" void nninitInitializeSdkModule() {
    nn::oe::Initialize();
    nn::aoc::Initialize();
    nn::fs::SetLocalAccessLog(true);
    nn::pctl::Initialize();
    nn::fs::SetResultHandledByApplication(true);
    nn::diag::InitializeApplicationAbortObserver();
}

extern "C" void nninitFinaializeSdkModule() {
    nn::aoc::Finalize();
}

extern "C" void __nnDetailNintendoSdkRuntimeObjectFileRefer() {
}

extern "C" void __nnDetailNintendoSdkRuntimeObjectFile() {
}

extern "C" void nnMain() {
    ksd::Initialize();
    /* ... */
}