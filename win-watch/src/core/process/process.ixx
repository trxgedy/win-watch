#include "ext\structs\structs.hpp"

import std;
import Utils;

export module Process;

export namespace process
{
	namespace core
	{
		auto get_privileges( ) -> bool;
		auto get_all_pids( ) -> std::vector<std::uint32_t>;
		auto get_pid( const std::string &process_name ) -> std::optional<std::uint32_t>;
		auto get_process_description( const std::uint32_t pid ) -> std::optional<std::string>;
		auto get_process_info( const std::uint32_t pid ) -> PROCESS_INFO;
		auto get_service_start_type( const std::string &service_name ) -> std::uint32_t;
		auto get_every_service_info( ) -> std::vector<SERVICE_INFO>;
	}

	namespace properties
	{
		auto get_process_modules( const std::uint32_t pid ) -> std::vector<PROCESS_MODULE_LIST>; // done
		auto get_process_threads( const std::uint32_t pid ) -> std::vector<THREAD_PROPERTIES>; // to do
		auto get_process_windows( const std::uint32_t pid ) -> std::vector<WINDOW_PROPERTIES>; // to do
	}

	namespace sort
	{
		template <typename T>
		auto sort_by_name( std::vector<T> &data, const std::function<std::string( const T & )> &get_name ) -> void
		{
			std::sort( data.begin( ), data.end( ), [ & ]( const T &a, const T &b )
			{
				const auto first = utils::string::to_lower<std::string>( get_name( a ) );
				const auto second = utils::string::to_lower<std::string>( get_name( b ) );

				return first < second;
			} );
		}

		template <typename T>
		auto sort_by_pid( std::vector<T> &data, const std::function<std::uint32_t( const T & )> &get_pid ) -> void
		{
			std::sort( data.begin( ), data.end( ), [ & ]( const T &a, const T &b )
			{
				return get_pid( a ) < get_pid( b );
			} );
		}
	}
}