/**
 * This file contains several compile-time utilities that can be used in combination
 * with the reflection system to lookup, construct, or cast to engine types.
 *
 * MASSIVE thanks to black belt C++ meta-fu master ThePhD (https://thephd.github.io/)
 * for providing valuable assistance with getting this to work correctly in a constexpr
 * context, giving us some sweet sweet performance gains <3
 */

#pragma once

#include <string_view>
#include <array>

#include "Hash.h"
#include "ZResourceID.h"

namespace detail
{
	template <class T>
	constexpr std::string_view ComputeTypeName();

	template <>
	constexpr std::string_view ComputeTypeName<void>()
	{
		return "void";
	}

	using TypeNameProber = void;

	template <class T>
	constexpr std::string_view WrappedTypeName()
	{
		return __FUNCSIG__;
	}

	constexpr std::size_t WrappedTypeNamePrefixLength()
	{
		return WrappedTypeName<TypeNameProber>().find(ComputeTypeName<TypeNameProber>());
	}

	constexpr std::size_t WrappedTypeNameSuffixLength()
	{
		return WrappedTypeName<TypeNameProber>().length()
			- WrappedTypeNamePrefixLength()
			- ComputeTypeName<TypeNameProber>().length();
	}

	template <class T>
	constexpr std::string_view ComputeTypeName()
	{
		constexpr auto s_WrappedName = detail::WrappedTypeName<T>();
		constexpr auto s_PrefixLength = detail::WrappedTypeNamePrefixLength();
		constexpr auto s_SuffixLength = detail::WrappedTypeNameSuffixLength();
		constexpr auto s_TypeNameLength = s_WrappedName.length() - s_PrefixLength - s_SuffixLength;

		return s_WrappedName.substr(s_PrefixLength, s_TypeNameLength);
	}

	template <std::size_t N>
	struct ZHMTypeNameData
	{
		std::array<char, N> Data;
		std::size_t Size;
	};

	/*
	 * Compute the ZHM-specific typename of a type.
	 *
	 * This basically takes the demangled MSVC typename and removes and instances
	 * of 'class ' and 'struct ' and replaces '::' with '.'.
	 *
	 * For example "class TArray<struct SomeNamespace::SomeType>" becomes
	 * "TArray<SomeNamespace.SomeType>".
	 */
	template <std::size_t N>
	constexpr auto ComputeZHMTypeName(std::string_view p_TypeName) noexcept
	{
		constexpr auto s_ClassPrefix = std::string_view("class ");
		constexpr auto s_StructPrefix = std::string_view("struct ");
		constexpr auto s_EnumPrefix = std::string_view("enum ");

		auto s_ZHMTypeNameStorage = std::array<char, N>();

		std::size_t s_FinalSize = 0;

		for (std::size_t i = 0; i < N; ++i)
		{
			if (p_TypeName[i] == 'c' && i + s_ClassPrefix.size() < N &&
				p_TypeName.substr(i, s_ClassPrefix.size()) == s_ClassPrefix)
			{
				i += s_ClassPrefix.size() - 1;
				continue;
			}

			if (p_TypeName[i] == 's' && i + s_StructPrefix.size() < N &&
				p_TypeName.substr(i, s_StructPrefix.size()) == s_StructPrefix)
			{
				i += s_StructPrefix.size() - 1;
				continue;
			}

			if (p_TypeName[i] == 'e' && i + s_EnumPrefix.size() < N &&
				p_TypeName.substr(i, s_EnumPrefix.size()) == s_EnumPrefix)
			{
				i += s_EnumPrefix.size() - 1;
				continue;
			}

			if (p_TypeName[i] == ':' && i + 1 < N && p_TypeName[i + 1] == ':')
			{
				++i;
				s_ZHMTypeNameStorage[s_FinalSize++] = '.';
				continue;
			}

			s_ZHMTypeNameStorage[s_FinalSize++] = p_TypeName[i];
		}

		return ZHMTypeNameData<N>{ s_ZHMTypeNameStorage, s_FinalSize };
	}

	template <class T>
	inline constexpr auto ZHMTypeName_Storage = ComputeZHMTypeName<ComputeTypeName<T>().size()>(ComputeTypeName<T>());

	template <std::size_t N>
	struct StringLiteral
	{
		constexpr StringLiteral(const char(&p_Str)[N])
		{
			std::copy_n(p_Str, N, Value);
		}

		char Value[N];
	};

	template <StringLiteral Path>
	constexpr ZRuntimeResourceID IOIPathToRuntimeResourceID()
	{
		// Minus 1 here because the size includes the null terminator.
		constexpr auto s_Hash = Hash::MD5<sizeof(Path.Value) - 1>(std::string_view(Path.Value, sizeof(Path.Value) - 1));

		constexpr uint32_t s_IDHigh = ((s_Hash.A >> 24) & 0x000000FF)
			| ((s_Hash.A >> 8) & 0x0000FF00)
			| ((s_Hash.A << 8) & 0x00FF0000);

		constexpr uint32_t s_IDLow = ((s_Hash.B >> 24) & 0x000000FF)
			| ((s_Hash.B >> 8) & 0x0000FF00)
			| ((s_Hash.B << 8) & 0x00FF0000)
			| ((s_Hash.B << 24) & 0xFF000000);

		return ZRuntimeResourceID(s_IDHigh, s_IDLow);
	}
}

/**
 * The MSVC typename of a type.
 */
template <class T>
inline constexpr auto TypeName = detail::ComputeTypeName<T>();

/**
 * The name of a type as we expect to find it in ZHM type info.
 */
template <class T>
inline constexpr auto ZHMTypeName = std::string_view(detail::ZHMTypeName_Storage<T>.Data.data(), detail::ZHMTypeName_Storage<T>.Size);

// Overrides for numeric types.
template <> inline constexpr std::string_view ZHMTypeName<signed char> = "int8";
template <> inline constexpr std::string_view ZHMTypeName<unsigned char> = "uint8";
template <> inline constexpr std::string_view ZHMTypeName<short> = "int16";
template <> inline constexpr std::string_view ZHMTypeName<unsigned short> = "uint16";
template <> inline constexpr std::string_view ZHMTypeName<int> = "int32";
template <> inline constexpr std::string_view ZHMTypeName<unsigned int> = "uint32";
template <> inline constexpr std::string_view ZHMTypeName<__int64> = "int64";
template <> inline constexpr std::string_view ZHMTypeName<unsigned __int64> = "uint64";
template <> inline constexpr std::string_view ZHMTypeName<float> = "float32";
template <> inline constexpr std::string_view ZHMTypeName<double> = "float64";

/**
 * The CRC32 of a type for use in lookups with the reflection engine.
 */
template <class T>
inline constexpr uint32_t ZHMTypeId = Hash::Crc32(ZHMTypeName<T>.data(), ZHMTypeName<T>.size());

/**
 * ZRuntimeResourceID from an IOI path string.
 */
template <detail::StringLiteral Path> inline constexpr auto ResId = detail::IOIPathToRuntimeResourceID<Path>();

