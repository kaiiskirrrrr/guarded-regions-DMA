class guarded
{
public:
    uintptr_t guard_address = 0;

    static auto is_guarded(const uintptr_t& pointer) noexcept -> bool
    {
        static constexpr uintptr_t filter = 0xFFFFFFF000000000;
        return (pointer & filter) == 0x8000000000 or (pointer & filter) == 0x10000000000;
    }

    auto validate_pointer(const uintptr_t& address) -> uintptr_t
    {
        return is_guarded(address) ? guard_address + (address & 0xFFFFFF) : address;
    }

    auto is_kernal(const uintptr_t& ptr) -> bool
    {
        return (ptr & 0xFFF0000000000000) == 0xFFF0000000000000;
    }

    auto read_two(uintptr_t address) -> uintptr_t
    {
        uintptr_t buffer{};
        int pid = is_kernal(address) ? 4 : processInfo.pid;

        auto flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_NOPAGING_IO;
        VMMDLL_MemReadEx(DMA_HANDLE, pid, static_cast<ULONG64>(address), reinterpret_cast<PBYTE>(&buffer), sizeof(buffer), NULL, flags);

        return validate_pointer(buffer);
    }

    auto create_scatter_handle(int pid) const -> VMMDLL_SCATTER_HANDLE
    {
        if (pid == 0) { pid = processInfo.pid; }
        return VMMDLL_Scatter_Initialize(handle, pid, misa_flags);
    }

    auto get_guarded_region() -> ULONG64
    {
        PVMMDLL_MAP_POOL pPool;
        VMMDLL_Map_GetPool(DMA_HANDLE, &pPool, NULL);

        for (DWORD i = 0; i < pPool->cMap; ++i)
        {
            PVMMDLL_MAP_POOLENTRY pEntry = &pPool->pMap[i];

            if (pEntry->cb == 0x200000 && memcmp(pEntry->szTag, "ConT", 4) == 0)
            {
                guard_address = pEntry->va;
                VMMDLL_MemFree(pPool);
                return guard_address;
            }
        }

        VMMDLL_MemFree(pPool);
        return NULL;
    }

}; inline const auto c_guarded = std::make_unique<guarded>();
