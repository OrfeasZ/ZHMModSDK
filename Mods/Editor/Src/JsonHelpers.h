#pragma once

#include <Glacier/ZMath.h>
#include <Glacier/ZString.h>
#include <simdjson.h>

inline std::string write_json(int64_t p_Value)
{
	char s_NumberBuffer[24];
	char* s_BufferEnd = simdjson::fast_itoa(s_NumberBuffer, p_Value);
	return std::string(s_NumberBuffer, s_BufferEnd);
}

inline std::string write_json(uint64_t p_Value)
{
	char s_NumberBuffer[24];
	char* s_BufferEnd = simdjson::fast_itoa(s_NumberBuffer, p_Value);
	return std::string(s_NumberBuffer, s_BufferEnd);
}

inline std::string write_json(int8_t p_Value)
{
	return write_json(static_cast<int64_t>(p_Value));
}

inline std::string write_json(uint8_t p_Value)
{
	return write_json(static_cast<uint64_t>(p_Value));
}

inline std::string write_json(int16_t p_Value)
{
	return write_json(static_cast<int64_t>(p_Value));
}

inline std::string write_json(uint16_t p_Value)
{
	return write_json(static_cast<uint64_t>(p_Value));
}

inline std::string write_json(int32_t p_Value)
{
	return write_json(static_cast<int64_t>(p_Value));
}

inline std::string write_json(uint32_t p_Value)
{
	return write_json(static_cast<uint64_t>(p_Value));
}

inline std::string write_json(double p_Value)
{
	char s_NumberBuffer[24];
	char* s_BufferEnd = simdjson::internal::to_chars(s_NumberBuffer, nullptr, p_Value);
	return std::string(s_NumberBuffer, s_BufferEnd);
}

inline std::string write_json(float p_Value)
{
	return write_json(static_cast<double>(p_Value));
}

inline std::string write_json(std::string_view p_Value)
{
	simdjson::internal::mini_formatter s_Formatter;
	s_Formatter.string(p_Value);
	return std::string(s_Formatter.str());
}

inline std::string write_json(const char* p_Value)
{
	return write_json(std::string_view(p_Value));
}

inline std::string write_json(const std::string& p_Value)
{
	return write_json(std::string_view(p_Value));
}

inline std::string write_json(const ZString& p_Value)
{
	return write_json(std::string_view(p_Value));
}

inline std::string write_json(bool p_Value)
{
	return p_Value ? "true" : "false";
}
