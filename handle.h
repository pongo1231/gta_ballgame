#pragma once

#define _HANDLE_FUNC [[nodiscard]] inline

typedef unsigned long long DWORD64;
typedef unsigned long DWORD;

class handle
{
public:
	handle() : address(0) {}
	handle(DWORD64 _addr) : address(_addr) {}

	_HANDLE_FUNC bool is_valid() const
	{
		return address;
	}

	_HANDLE_FUNC handle at(int _offset) const
	{
		return is_valid() ? address + _offset : 0;
	}

	template<typename t>
	_HANDLE_FUNC t *get() const
	{
		return reinterpret_cast<t*>(address);
	}

	template<typename t>
	_HANDLE_FUNC t value() const
	{
		return is_valid() ? *get<t>() : 0;
	}

	_HANDLE_FUNC DWORD64 addr() const
	{
		return address;
	}

	_HANDLE_FUNC handle into() const
	{
		if (is_valid())
		{
			auto handle = at(1);
			return handle.at(handle.value<DWORD>()).at(4);
		}

		return 0;
	}

private:
	DWORD64 address;
};