	inline static bool isGuarded(uintptr_t pointer) noexcept
	{
		static constexpr uintptr_t filter = 0xFFFFFFF000000000;
		uintptr_t result = pointer & filter;
		return result == 0x8000000000 || result == 0x10000000000;
	}

	uintptr_t guard_address = 0;

	inline uintptr_t validatePointer(uintptr_t address) {
		return isGuarded(address) ? guard_address + (address & 0xFFFFFF) : address;
	}

	inline bool isKernel(uintptr_t ptr) {
		return (ptr & 0xFFF0000000000000) == 0xFFF0000000000000;
	}

	uintptr_t Read2(uintptr_t address, bool cache = false) {
		uintptr_t buffer{};
		int pid = 4;

		if (!isKernel(address))
			pid = processInfo.pid;

		//std::cout << "read from pid: " << std::to_string(pid).c_str() << std::endl;

		auto flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_NOPAGING_IO;
		if (cache) {
			flags &= ~VMMDLL_FLAG_NOCACHE;
			flags &= ~VMMDLL_FLAG_NOCACHEPUT;
		}

		VMMDLL_MemReadEx(DMA_HANDLE, pid, (ULONG64)address, reinterpret_cast<PBYTE>(&buffer), sizeof(buffer), NULL, flags);

		return validatePointer(buffer);
	}

	ULONG64 GetGuardedRegion()
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



		//guard_address = Read2(vgk + 0x7FCE0);

		//return guard_address;
	}
