#pragma once
#ifndef _OG_VECTOR_H_
#define _OG_VECTOR_H_

#include "OgPrecompile.h"

#include <vector>

OG_NAMESPACE_SYSTEM_BEGIN

/**
* @brief: It is a simple vector wrapping class for convenience.
*/
template< typename T, class A = std::allocator< T >>
class OgVector
{
public:
	OgVector() = default;
	OgVector(const OgVector& rhs)
	{
		_data = rhs._data;
	}
	OgVector(OgVector&& rhs)
	{
		_data = rhs._data;
	}

	
	~OgVector() = default;

	void Add(const T& value)
	{
		_data.push_back(value);
	}

	// rvalue or universal reference
	void Add(T&& value)
	{
		_data.push_back(value);
	}

	void Remove(const T& value)
	{
		for (std::vector<T>::iterator it = _data.begin(); it != _data.end(); ++it)
		{
			if (*it == value)
			{
				_data.erase(it);
				break;
			}
		}
	}

	void PopBack()
	{
		_data.pop_back();
	}

	size_t Size() const
	{
		return _data.size();
	}

	void Resize(size_t size)
	{
		_data.resize(size);
	}

	void Reserve(size_t size)
	{
		_data.reserve(size);
	}

	void Clear()
	{
		_data.clear();
	}

	void Shrink()
	{
		_data.shrink_to_fit();
	}

	T* Data()
	{
		return &_data[0];
	}

	//=============		operator		==================

	OG_FORCEINLINE OgVector& operator=(OgVector&& rhs)
	{
		_data = rhs._data;

		return *this;
	}

	OG_FORCEINLINE OgVector& operator=(const OgVector& rhs)
	{
		_data = rhs._data;

		return *this;
	}

	OG_FORCEINLINE const T& operator[](size_t index) const
	{
		ASSERT(index >= 0 && index < _data.size());
		return _data[index];
	}

	OG_FORCEINLINE T& operator[](size_t index)
	{
		ASSERT(index >= 0 && index < _data.size());
		return _data[index];
	}

	// TODO: working



private:
	std::vector<T, A> _data;



};

OG_NAMESPACE_SYSTEM_END

#endif