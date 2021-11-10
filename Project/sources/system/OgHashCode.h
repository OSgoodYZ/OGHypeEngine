/*
 * File: hashcode.h
 * ----------------
 * This file declares global hashing functions for various common data types.
 * These functions are used by the HashMap and HashSet collections, as well as
 * by other collections that wish to be used as elements within HashMaps/Sets.
 *
 * @version 2015/07/05
 * - using global hashing functions rather than global variables
 *   (hashSeed(), hashMultiplier(), and hashMask())
 */

#ifndef _OG_HASH_CODE_H_
#define _OG_HASH_CODE_H_

#include "OgPrecompile.h"
#include <string>

OG_NAMESPACE_SYSTEM_BEGIN

/*
 * Function: hashCode
 * Usage: int hash = hashCode(key);
 * --------------------------------
 * Returns a hash code for the specified key, which is always a
 * nonnegative integer.  This function is overloaded to support
 * all of the primitive types and the C++ <code>string</code> type.
 */
OG_API int hashCode(bool key);
OG_API int hashCode(char key);
OG_API int hashCode(double key);
OG_API int hashCode(float key);
OG_API int hashCode(int key);
OG_API int hashCode(long key);
OG_API int hashCode(ulong key);
OG_API int hashCode(const char* str);
OG_API int hashCode(const std::string& str);
OG_API int hashCode(void* key);

/*
 * Constants that are used to help implement these functions
 * (see hashcode.h for example usage)
 */
OG_API int hashSeed();         // Starting point for first cycle
OG_API int hashMultiplier();   // Multiplier for each cycle
OG_API int hashMask();         // All 1 bits except the sign

//#include "private/init.h"   // ensure that Stanford C++ lib is initialized


//***************** UE4 Hash Function *****************//
// https://create.stephan-brumme.com/crc32/
// http://slicing-by-8.sourceforge.net/
struct OG_API CRCHash
{
	static uint32 CRCTablesSB8[8][256];

	static uint32 MemCrc32(const void* InData, uint32 Length, uint32 CRC = 0);

	template <typename T>
	static unsigned int TypeCrc32(const T& Data, uint32 CRC = 0)
	{
		return MemCrc32(&Data, sizeof(T), CRC);
	}

	template <bool Predicate, typename Result = void> class TEnableIf;

	template <typename Result>
	class TEnableIf<true, Result>
	{
	public:
		typedef Result Type;
	};

	template <typename Result>
	class TEnableIf<false, Result>
	{ };


	template <typename CharType>
	static typename TEnableIf<sizeof(CharType) != 1, uint32>::Type StrCrc32(const CharType* Data, uint32 CRC = 0)
	{
		// We ensure that we never try to do a StrCrc32 with a CharType of more than 4 bytes.  This is because
		// we always want to treat every CRC as if it was based on 4 byte chars, even if it's less, because we
		// want consistency between equivalent strings with different character types.
		static_assert(sizeof(CharType) <= 4, "StrCrc32 only works with CharType up to 32 bits.");

		CRC = ~CRC;
		while (CharType Ch = *Data++)
		{
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			Ch >>= 8;
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			Ch >>= 8;
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			Ch >>= 8;
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
		}
		return ~CRC;
	}

	template <typename CharType>
	static typename TEnableIf<sizeof(CharType) == 1, uint32>::Type StrCrc32(const CharType* Data, uint32 CRC = 0)
	{
		/* Overload for when CharType is a byte, which causes warnings when right-shifting by 8 */
		CRC = ~CRC;
		while (CharType Ch = *Data++)
		{
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC) & 0xFF];
		}
		return ~CRC;
	}
};

/**
 * Combines two hash values to get a third.
 * Note - this function is not commutative.
 */
OG_FORCEINLINE uint32 HashCombine(uint32 A, uint32 C)
{
	uint32 B = 0x9e3779b9;
	A += B;

	A -= B; A -= C; A ^= (C >> 13);
	B -= C; B -= A; B ^= (A << 8);
	C -= A; C -= B; C ^= (B >> 13);
	A -= B; A -= C; A ^= (C >> 12);
	B -= C; B -= A; B ^= (A << 16);
	C -= A; C -= B; C ^= (B >> 5);
	A -= B; A -= C; A ^= (C >> 3);
	B -= C; B -= A; B ^= (A << 10);
	C -= A; C -= B; C ^= (B >> 15);

	return C;
}

//***************** UE4 Hash Function *****************//
template<typename T>
struct OgHashFunc
{
	static uint32 Get(const T& key)
	{
		return CRCHash::TypeCrc32(key);
	}
};
// const char* literal Hash Func
template<>
struct OgHashFunc<const char*>
{
	static uint32 Get(const char* key)
	{
		return CRCHash::StrCrc32(key);
	}
};

// const char* literal Hash Func
template<>
struct OgHashFunc<char*>
{
	static uint32 Get(char* key)
	{
		return CRCHash::StrCrc32(key);
	}
};

// uint64 Hash Func
// https://gist.github.com/badboy/6267743
template<>
struct OgHashFunc<uint64>
{
	static uint32 Get(const uint64& key)
	{
		uint64 tmp = (~key) + (key << 18);
		tmp = tmp ^ (tmp >> 31);
		tmp = tmp * 21;
		tmp = tmp ^ (tmp >> 11);
		tmp = tmp + (tmp << 6);
		tmp = tmp ^ (tmp >> 22);
		return (uint32)tmp;
	}
};

template<>
struct OgHashFunc<uint32>
{
	static uint32 Get(const uint32& key)
	{
		return key;
	}
};

// PointerHash has a HashCombine with parameter C
OG_FORCEINLINE uint32 PointerHash(const void* Key, uint32 C = 0)
{
	uint32 PtrInt = OgHashFunc<uint64>::Get(reinterpret_cast<uintptr>(Key));
	return HashCombine(PtrInt, C);
}


//***************** UE4 Hash Function *****************//
// Pointer Hash Func
template <typename T>
struct OgHashFunc<T*>
{
	static uint32 Get(const T* key)
	{
		return PointerHash(key);
	}
};

OG_NAMESPACE_SYSTEM_END

#endif 