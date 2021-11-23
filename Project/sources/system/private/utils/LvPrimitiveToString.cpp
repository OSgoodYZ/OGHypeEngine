#include "system/utils/LvPrimitiveToString.h"

#include "system/LvString.h"

#include <stdlib.h>

using namespace Lv::System::Utils;

LV_NS_SYSTEM_BEGIN

LvString Utils::to_string(int8 value)
{
    LvString ret;
    ret.FormatSelf("%d", value);
	return ret;
}

LvString Utils::to_string(int16 value)
{
    LvString ret;
    ret.FormatSelf("%d", value);
    return ret;
}

LvString Utils::to_string(int32 value)
{
    LvString ret;
    ret.FormatSelf("%d", value);
    return ret;
}

LvString Utils::to_string(int64 value)
{
    LvString ret;
    ret.FormatSelf("%lld", value);
	return ret;
}

LvString Utils::to_string(uint8 value)
{
    LvString ret;
    ret.FormatSelf("%u", value);
    return ret;
}

LvString Utils::to_string(uint16 value)
{
    LvString ret;
    ret.FormatSelf("%u", value);
    return ret;
}

LvString Utils::to_string(uint32 value)
{
    LvString ret;
    ret.FormatSelf("%u", value);
    return ret;
}

LvString Utils::to_string(uint64 value)
{
    LvString ret;
    ret.FormatSelf("%llu", value);
    return ret;
}

LvString Utils::to_string(float value)
{
    LvString ret;
    ret.FormatSelf("%f", value);
    return ret;
}

LvString Utils::to_string(double value)
{
    LvString ret;
    ret.FormatSelf("%lf", value);
    return ret;
}

int8 Utils::to_int8(const char * value)
{
	char* error = nullptr;
	int8 result = strtol(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

int16 Utils::to_int16(const char * value)
{
	char* error = nullptr;
	int16 result = strtol(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

int32 Utils::to_int32(const char * value)
{
	char* error = nullptr;
	int32 result = strtol(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

int64 Utils::to_int64(const char * value)
{
	char* error = nullptr;
	int64 result = strtoll(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

uint8 Utils::to_uint8(const char * value)
{
	char* error = nullptr;
	uint8 result = strtoul(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

uint16 Utils::to_uint16(const char * value)
{
	char* error = nullptr;
	uint16 result = strtoul(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

uint32 Utils::to_uint32(const char * value)
{
	char* error = nullptr;
	uint32 result = strtoul(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

uint64 Utils::to_uint64(const char * value)
{
	char* error = nullptr;
	uint64 result = strtoull(value, &error, 10);
	LV_CHECK(nullptr != error, "strtoul error, %s", error);
	return result;
}

float Utils::to_float(const char * value)
{
	char* end;
	float f = 0.0;
	f = strtof(value, &end);
	return f;
}

double Utils::to_double(const char * value)
{
	char* end;
	double d = 0.0;
	d = strtod(value, &end);
	return d;
}

LV_NS_SYSTEM_END
