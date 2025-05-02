import std;
import Process;
import <Windows.h>;

export import Watcher;

namespace watcher
{
	c_watcher::c_watcher()
	{
		if( !process::core::get_privileges( ) )
			MessageBoxA( nullptr, "couldnt get privileges", " ", MB_ICONWARNING | MB_OK );

		//processes

		processes.reserve( pids.size( ) );

		for ( auto i = 0; i < pids.size( ); i++ )
		{
			processes.emplace_back( process::core::get_process_info( pids.at( i ) ) );
		}

		// services

		services.reserve( 256 );
		services = process::core::get_every_service_info( );
		services.shrink_to_fit( );

		type = sort_type::SORT_BY_NAME;
	}

	auto c_watcher::update_process_info_buffer( ) -> void
	{
		while ( true )
		{
			const auto new_pids = process::core::get_all_pids( );

			for ( const auto &new_pid : new_pids )
			{
				if ( std::find( pids.begin( ), pids.end( ), new_pid ) == pids.end( ) )
				{
					pids.emplace_back( new_pid );
					processes.emplace_back( process::core::get_process_info( new_pid ) );

					this->sort_it( this->type );
				}
			}

			for ( auto i = 0; i < pids.size( ); ++i )
			{
				if ( std::find( new_pids.begin( ), new_pids.end( ), pids.at( i ) ) == new_pids.end( ) )
				{
					for ( auto j = 0; j < processes.size( ); ++j )
					{
						if ( processes.at( j ).pid == pids.at( i ) )
						{
							processes.erase( processes.begin( ) + j );
							break;
						}
					}

					pids.erase( pids.begin( ) + i );
				}
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
		}
	}

	auto c_watcher::update_service_info_buffer( ) -> void
	{
		while ( true )
		{
			const auto new_services = process::core::get_every_service_info( );

			for ( auto i = 0; i < services.size( ); ++i )
			{
				for ( auto j = 0; j < new_services.size( ); ++j )
				{
					if ( services.at( i ).pid == new_services.at( j ).pid )
					{
						services.at( i ).status = new_services.at( j ).status;
						services.at( i ).start_type = new_services.at( j ).start_type;
						continue;
					}
				}
			}
			
			std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		}
	}

	auto c_watcher::sort_it( sort_type t ) -> void
	{
		this->type = t;

		switch ( this->type )
		{
			case sort_type::SORT_BY_NAME:
			{
				process::sort::sort_by_name<PROCESS_INFO>( this->processes, [ ] ( const PROCESS_INFO &p ) { return p.process_name; } );
				process::sort::sort_by_name<SERVICE_INFO>( this->services, [ ]( const SERVICE_INFO &s ) { return s.service_name; } ); 
				break;
			}

			case sort_type::SORT_BY_PID:
			{
				process::sort::sort_by_pid<PROCESS_INFO>( this->processes, [ ]( const PROCESS_INFO &p ) { return p.pid; } );
				process::sort::sort_by_pid<SERVICE_INFO>( this->services, [ ]( const SERVICE_INFO &s ) { return s.pid; } );
				break;
			}
		}
	}
}