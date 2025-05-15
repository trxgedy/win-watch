#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>

#include "ext\definitions\structs.hpp"

import std;
import Utils;

export import Process;

export namespace process
{
	namespace core
	{
		auto get_privileges( ) -> bool
		{
			HANDLE token_handle {};
			if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle ) )
				return false;

			LUID identifier {};
			if ( !LookupPrivilegeValueA( NULL, SE_DEBUG_NAME, &identifier ) )
				return false;

			TOKEN_PRIVILEGES token_privilege {};

			token_privilege.PrivilegeCount = 1;
			token_privilege.Privileges[ 0 ].Luid = identifier;
			token_privilege.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

			if ( !AdjustTokenPrivileges( token_handle, 0, &token_privilege, sizeof( TOKEN_PRIVILEGES ), nullptr, nullptr ) )
				return false;

			CloseHandle( token_handle );

			return true;
		}

		auto get_all_pids( ) -> std::vector<std::uint32_t>
		{
			const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

			std::vector<std::uint32_t> pids;
			pids.reserve( 256 );

			if ( snapshot != INVALID_HANDLE_VALUE )
			{
				PROCESSENTRY32 entry { .dwSize = sizeof( entry ) };

				do
				{
					if ( entry.th32ProcessID != 0 )
						pids.emplace_back( entry.th32ProcessID );

				} while ( Process32Next( snapshot, &entry ) );

				CloseHandle( snapshot );
			}

			pids.shrink_to_fit( );

			return pids;
		}

		auto get_pid( const std::string &process_name ) -> std::optional<std::uint32_t>
		{
			const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

			if ( snapshot != INVALID_HANDLE_VALUE )
			{
				PROCESSENTRY32 entry { .dwSize = sizeof( entry ) };

				do
				{
					if ( utils::string::to_lower<std::string>( process_name ).compare( utils::string::to_lower<std::string>( entry.szExeFile ) ) == 0 )
					{
						CloseHandle( snapshot );
						return entry.th32ProcessID;
					}
				} while ( Process32Next( snapshot, &entry ) );

				CloseHandle( snapshot );
			}

			return std::nullopt;
		}

		auto get_process_description( const std::uint32_t pid ) -> std::optional<std::string>
		{
			const auto process_handle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid );

			if ( !process_handle )
				return std::nullopt;

			char exe_path[ MAX_PATH ];
			if ( !GetModuleFileNameExA( process_handle, nullptr, exe_path, MAX_PATH ) )
			{
				CloseHandle( process_handle );
				return std::nullopt;
			}

			CloseHandle( process_handle );

			DWORD handle = 0;
			std::uint32_t version_info_size = GetFileVersionInfoSizeA( exe_path, &handle );

			if ( version_info_size == 0 )
				return std::nullopt;

			std::vector<BYTE> version_info( version_info_size );

			if ( !GetFileVersionInfoA( exe_path, handle, version_info_size, version_info.data( ) ) )
				return std::nullopt;

			std::uint32_t size = 0;
			void *description_buffer = nullptr;

			if ( VerQueryValueA( version_info.data( ), R"(\StringFileInfo\040904b0\FileDescription)", &description_buffer, &size ) && description_buffer )
				return std::string( static_cast< char * >( description_buffer ), size );

			return std::nullopt;
		}

		auto get_process_info( const std::uint32_t pid ) -> PROCESS_INFO
		{
			const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

			PROCESS_INFO buffer;

			if ( snapshot != INVALID_HANDLE_VALUE )
			{
				PROCESSENTRY32 entry { .dwSize = sizeof( entry ) };

				do
				{
					if ( entry.th32ProcessID == 0 || entry.th32ProcessID != pid )
						continue;

					buffer.pid = entry.th32ProcessID;
					buffer.process_name = entry.szExeFile;
					buffer.process_description = get_process_description( buffer.pid ).value_or( " " );

				} while ( Process32Next( snapshot, &entry ) );

				CloseHandle( snapshot );
			}

			return buffer;
		}

		auto get_service_start_type( const std::string &service_name ) -> std::uint32_t
		{
			const auto sc_manager = OpenSCManagerA( nullptr, nullptr, SC_MANAGER_ALL_ACCESS );
			if ( !sc_manager )
			{
				return 0;
			}

			const auto service_handle = OpenServiceA( sc_manager, service_name.c_str( ), SERVICE_ALL_ACCESS );
			if ( !service_handle )
			{
				CloseServiceHandle( sc_manager );
				return 0;
			}

			DWORD bytes_needed, start_type = 0;

			if ( !QueryServiceConfigA( service_handle, NULL, 0, &bytes_needed ) )
			{
				if ( GetLastError( ) == ERROR_INSUFFICIENT_BUFFER )
				{
					auto config = reinterpret_cast< QUERY_SERVICE_CONFIG * >( malloc( bytes_needed ) );

					if ( config && QueryServiceConfigA( service_handle, config, bytes_needed, &bytes_needed ) )
						start_type = config->dwStartType;

					free( config );
				}
			}

			CloseServiceHandle( service_handle );
			CloseServiceHandle( sc_manager );

			return start_type;
		}

		auto get_every_service_info( ) -> std::vector<SERVICE_INFO>
		{
			const auto sc_manager = OpenSCManagerA( nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE );

			std::vector<SERVICE_INFO> services;
			services.reserve( 256 );

			unsigned long bytes = 0, services_returned = 0, resume_handle = 0;

			EnumServicesStatusExA( sc_manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, nullptr, 0, &bytes, &services_returned, &resume_handle, nullptr );

			std::vector<BYTE> buffer( bytes );

			if ( !EnumServicesStatusExA( sc_manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, buffer.data( ), bytes, &bytes, &services_returned, &resume_handle, nullptr ) )
			{
				CloseServiceHandle( sc_manager );
				return {};
			}

			const auto services_info = reinterpret_cast< LPENUM_SERVICE_STATUS_PROCESS >( buffer.data( ) );

			for ( unsigned long i = 0; i < services_returned; ++i )
			{
				SERVICE_INFO service_info {};

				service_info.service_name = services_info[ i ].lpServiceName;
				service_info.display_name = services_info[ i ].lpDisplayName;
				service_info.pid = services_info[ i ].ServiceStatusProcess.dwProcessId;
				service_info.status = services_info[ i ].ServiceStatusProcess.dwCurrentState;
				service_info.start_type = get_service_start_type( service_info.service_name );

				services.emplace_back( service_info );
			}

			CloseServiceHandle( sc_manager );

			services.shrink_to_fit( );

			return services;
		}
	}

	namespace properties
	{
		auto get_process_modules( const std::uint32_t pid ) -> std::vector<PROCESS_MODULE_LIST>
		{
			const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid );

			std::vector<PROCESS_MODULE_LIST> modules;
			modules.reserve( 256 );

			if ( snapshot != INVALID_HANDLE_VALUE )
			{
				MODULEENTRY32 entry { .dwSize = sizeof( entry ) };

				do
				{
					PROCESS_MODULE_LIST buffer {};

					buffer.module_base_address = reinterpret_cast< std::uint64_t >( entry.modBaseAddr );
					buffer.module_name = entry.szModule;
					buffer.module_path = entry.szExePath;

					modules.emplace_back( buffer );

				} while ( Module32Next( snapshot, &entry ) );

				CloseHandle( snapshot );
			}

			modules.shrink_to_fit( );

			return modules;
		}

		auto get_process_threads( const std::uint32_t pid ) -> std::vector<THREAD_PROPERTIES>
		{
			const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, pid );

			std::vector<THREAD_PROPERTIES> threads;
			threads.reserve( 32 );

			if ( snapshot == INVALID_HANDLE_VALUE )
				return {};

			THREADENTRY32 entry { .dwSize = sizeof( entry ) };

			do
			{
				THREAD_PROPERTIES buffer {};

				buffer.priority = entry.tpBasePri;
				buffer.tid = entry.th32ThreadID;

				threads.emplace_back( buffer );

			} while ( Thread32Next( snapshot, &entry ) );

			CloseHandle( snapshot );

			threads.shrink_to_fit( );

			return threads;
		}

		auto get_process_windows( const std::uint32_t pid ) -> std::vector<WINDOW_PROPERTIES>
		{

			return {};
		}
	}
}