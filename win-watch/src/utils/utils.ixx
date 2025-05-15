module;

#include <windows.h>
#include <TlHelp32.h>
#include <algorithm>

#include "ext\definitions\structs.hpp"

import std;

export module Utils;

export namespace utils
{
	namespace string
	{
		template <typename type>
			requires ( std::convertible_to< type, std::string > || std::convertible_to< type, std::wstring > )
		type to_lower( type str )
		{
			std::transform( str.begin( ), str.end( ), str.begin( ), ::tolower );

			return str;
		}

		auto generate_random_string( ) -> std::string
		{
			const char charset[ ] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
			const size_t charset_size = sizeof( charset ) - 1;
			const size_t length = 10;

			std::random_device rd;
			std::mt19937 generator( rd( ) );
			std::uniform_int_distribution<size_t> distribution( 0, charset_size - 1 );

			std::string random_string;
			random_string.reserve( length );

			for ( size_t i = 0; i < length; ++i )
			{
				random_string += charset[ distribution( generator ) ];
			}

			return random_string;
		}
	}

	namespace files
	{
		auto get_file_size( const std::string &file_path ) -> std::uint64_t
		{
			std::ifstream file( file_path, std::ios::binary | std::ios::ate );

			if ( !file.is_open( ) )
				return 0;

			std::streampos file_size = file.tellg( );

			file.close( );

			return static_cast< std::uint64_t >( std::ceil( static_cast< double >( file_size ) / 1024 ) ); // kb
		}

		auto parse_file_name( const std::string_view &file_path ) -> std::string_view
		{
			const auto index = file_path.find_last_of( "\\" );

			return file_path.substr( index + 1 );
		}
	}
}