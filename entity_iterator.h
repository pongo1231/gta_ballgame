#pragma once

#include "natives.h"
#include "memory.h"

// Thanks to menyoo for most of these!!

// Pool Interator class to iterate over pools. Has just enough operators defined to be able to be used in a for loop, not suitable for any other iterating.
template<typename t>
class pool_iterator
{
public:
	t* pool = nullptr;
	int32_t index = 0;

	explicit pool_iterator(t *_pool, int32_t _index = 0)
    {
        this->pool  = _pool;
        this->index = _index;
    }

	pool_iterator& operator++()
	{
		for (index++; index < pool->m_ulSize; index++)
			{
				if (pool->IsValid (index))
					{
						return *this;
					}
			}

		index = pool->m_ulSize;
		return *this;
	}

	Entity operator*()
        {
		static int(*_addEntityToPoolFunc)(__int64) = []
		{
			auto handle = memory::find_pattern("48 F7 F9 49 8B 48 08 48 63 D0 C1 E0 08 0F B6 1C 11 03 D8");
			return handle.at(-0x68).get<int(__int64)>();
		}();

		__int64 ullAddr = pool->GetAddress(index);
		int iHandle = _addEntityToPoolFunc(ullAddr);
		return iHandle;
        }

	bool operator!=(const pool_iterator &other) const
        {
		return this->index != other.index;
        }
};

// Common functions for VehiclePool and GenericPool
template<typename T>
class pool_utils
{
public:
	inline auto to_array()
	{
		std::vector<Entity> arr;
		for (auto entity : *static_cast<T*>(this))
			{
				arr.push_back (entity);
			}

		return arr;
	}

	auto begin ()
	{
		return ++pool_iterator<T> (static_cast<T*>(this), -1);
	}

	auto end ()
	{
		return ++pool_iterator<T> (static_cast<T*>(this), static_cast<T*>(this)->m_ulSize);
	}
};

class vehicle_pool : public pool_utils<vehicle_pool>
{
public:
	UINT64* m_pullPoolAddress;
	UINT32 m_ulSize;
	char _Padding2[36];
	UINT32* m_pulBitArray;
	char _Padding3[40];
	UINT32 m_ulItemCount;

	inline bool IsValid(UINT32 i)
	{
		return (m_pulBitArray[i >> 5] >> (i & 0x1F)) & 1;
	}

	inline UINT64 GetAddress(UINT32 i)
	{
		return m_pullPoolAddress[i];
	}
};

class generic_pool : public pool_utils<generic_pool>
{
public:
	UINT64 m_ullPoolStartAddress;
	BYTE* m_ucByteArray;
	UINT32 m_ulSize;
	UINT32 m_ulItemSize;

	inline bool IsValid(UINT32 i)
	{
		return Mask(i) != 0;
	}

	inline UINT64 GetAddress(UINT32 i)
	{
		return Mask(i) & (m_ullPoolStartAddress + i * m_ulItemSize);
	}

private:
	inline long long Mask(UINT32 i)
	{
		long long num1 = m_ucByteArray[i] & 0x80;
		return ~((num1 | -num1) >> 63);
	}
};


inline auto& get_all_peds()
{
	static generic_pool* pPedPool = [] {
		auto handle = memory::find_pattern("48 8B 05 ?? ?? ?? ?? 41 0F BF C8 0F BF 40 10");
		return handle.at(2).into().value<generic_pool*>();
	} ();

	return *pPedPool;
}

inline auto& get_all_vehs()
{
	static vehicle_pool* pVehPool = [] {
		auto handle = memory::find_pattern("48 8B 05 ?? ?? ?? ?? F3 0F 59 F6 48 8B 08");
		return *handle.at(2).into().value<vehicle_pool**>();
	} ();

	return *pVehPool;
}

inline auto& get_all_props()
{
	static generic_pool* pPropPool = [] {
		auto handle = memory::find_pattern("48 8B 05 ?? ?? ?? ?? 8B 78 10 85 FF");
		return handle.at(2).into().value<generic_pool*>();
	} ();

	return *pPropPool;
}

inline auto get_all_peds_array()
{
	return get_all_peds().to_array ();
}

inline auto get_all_vehs_array()
{
	return get_all_vehs().to_array ();
}

inline auto get_all_props_array()
{
	return get_all_props().to_array ();
}