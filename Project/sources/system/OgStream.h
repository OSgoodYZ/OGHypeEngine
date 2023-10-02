#pragma once
#ifndef __OG_STREAM_H__
#define __OG_STREAM_H__

#include "OgPrecompile.h"
#include <vector>
#include <string>
#include "system/OgVector.h"

// TODO

class OG_API OgStream
{
public:
	virtual ~OgStream() {};

	virtual int64 GetPosition() = 0;

	virtual void SetPosition(int64 pos) = 0;

	virtual void WriteRaw(const void* ptr, size_t size) = 0;

	virtual void ReadRaw(void* ptr, size_t size) = 0;

	virtual size_t Length() const = 0;

	virtual void Close() = 0;

	/*
	* template Read/Write는 단순 sizeof(T)로 할 수 있는 것들에 대해서만 사용할 수 있다.
	* 그 외의 것들은, Base Class의 함수에 Overloading하면 안되고 따로 별개의 함수 이름을 만들어야 한다.
	* Overloading을 하게 되면 LvStream의 메소드가 가려저 접근하려면
	* ChildClass.LvStream::Read/Write<typename>(variable); 형식으로 작성해야 한다.
	*/
	template<typename T,
		typename std::enable_if<
		!std::is_pointer<T>::value &&
		!std::is_same<T, std::string>::value &&
		!std::is_same<T, std::wstring>::value
	>::type * = nullptr>
		void Read(T& data)
	{
		constexpr size_t typeSize = sizeof(T);
		ReadRaw((void*)&data, typeSize);
	}

	template<typename T,
		typename std::enable_if<
		!std::is_pointer<T>::value &&
		!std::is_same<T, std::string>::value &&
		!std::is_same<T, std::wstring>::value
	>::type * = nullptr>
		void Read(T& data, size_t count)
	{
		constexpr size_t typeSize = sizeof(T);
		ReadRaw((void*)&data, typeSize * count);
	}

	template<typename T,
		typename std::enable_if<
		!std::is_pointer<T>::value &&
		!std::is_same<T, std::string>::value &&
		!std::is_same<T, std::wstring>::value
	>::type* = nullptr>
		void Write(const T& t)
	{
		static_assert(!std::is_pointer<T>::value, "Can't write pointer type");
		constexpr size_t typeSize = sizeof(T);
		WriteRaw((void*)&t, typeSize);
	}

	template<typename T,
		typename std::enable_if<
		!std::is_pointer<T>::value &&
		!std::is_same<T, std::string>::value &&
		!std::is_same<T, std::wstring>::value
	>::type* = nullptr>
		void Write(const T& t, size_t count)
	{
		static_assert(!std::is_pointer<T>::value, "Can't write pointer type");
		constexpr size_t typeSize = sizeof(T);
		WriteRaw((void*)&t, typeSize * count);
	}
};


#endif // __OG_STREAM_H__

