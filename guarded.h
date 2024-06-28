	uintptr_t guard_address = 0;	

        inline static auto is_guarded(uintptr_t pointer) -> bool noexcept
	{
		static constexpr uintptr_t filter = 0xFFFFFFF000000000;
		uintptr_t result = pointer & filter;
		return result == 0x8000000000 || result == 0x10000000000;
	}

	inline auto validate_pointer(uintptr_t address) -> uintptr_t {
		return is_guarded(address) ? guard_address + (address & 0xFFFFFF) : address;
	}

	inline auto is_kernal(uintptr_t ptr) -> bool {
		return (ptr & 0xFFF0000000000000) == 0xFFF0000000000000;
	}

	inline uintptr_t read_two(uintptr_t address, bool cache = false) {
		uintptr_t buffer{};
		int pid = 4;

		if (!is_kernal(address))
			pid = processInfo.pid;

		auto flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_NOPAGING_IO;
		if (cache) {
			flags &= ~VMMDLL_FLAG_NOCACHE;
			flags &= ~VMMDLL_FLAG_NOCACHEPUT;
		}

		VMMDLL_MemReadEx(DMA_HANDLE, pid, (ULONG64)address, reinterpret_cast<PBYTE>(&buffer), sizeof(buffer), NULL, flags);

		return validate_pointer(buffer);
	}

        inline VMMDLL_SCATTER_HANDLE create_scatter_handle(int pid) const
        {

	if (pid == 0) pid = processInfo.pid;

	const VMMDLL_SCATTER_HANDLE scatter_handle = VMMDLL_Scatter_Initialize(handle, pid, misa_flags);
	return scatter_handle;
        }

	auto get_guarded_region() -> ULONG64
	{
		PVMMDLL_MAP_POOL pPool;

		VMMDLL_Map_GetPool(DMA_HANDLE, &pPool, NULL);

		for (DWORD i = 0; i < pPool->cMap; ++i) {
			PVMMDLL_MAP_POOLENTRY pEntry = &pPool->pMap[i];

			if (pEntry->cb == 0x200000 && memcmp(pEntry->szTag, "ConT", 4) == 0) {

				ULONG64 result = pEntry->va;

				VMMDLL_MemFree(pPool);

				guard_address = result;
				return result;
			}
		}

		VMMDLL_MemFree(pPool);
		return NULL;
	}
