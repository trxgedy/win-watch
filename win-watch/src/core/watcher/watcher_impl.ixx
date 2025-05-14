import std;
import <Windows.h>;

import Process;
import Utils;
import Ui;

export import Watcher;

namespace watcher
{
	c_watcher::c_watcher( )
	{
		if ( !process::core::get_privileges( ) )
			MessageBoxA( nullptr, "couldnt get privileges", " ", MB_ICONWARNING | MB_OK );

		processes.reserve( pids.size( ) );

		for ( const auto pid : pids )
		{
			processes.emplace_back( process::core::get_process_info( pid ) );
		}

		services.reserve( 256 );
		services = process::core::get_every_service_info( );
		services.shrink_to_fit( );

		sort_it( sort_type::SORT_BY_NAME );
	}

	auto c_watcher::update_process_info_buffer( ) -> void
	{
		while ( running )
		{
			if ( ui::c_window::current_tab != ui::c_window::tabs::TAB_PROCESSES )
			{
				std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
				continue;
			}

			const auto new_pids = process::core::get_all_pids( );

			std::unordered_set<std::uint32_t> pids_set( pids.begin( ), pids.end( ) );
			std::unordered_set<std::uint32_t> new_pids_set( new_pids.begin( ), new_pids.end( ) );

			for ( const auto new_pid : new_pids )
			{
				if ( pids_set.find( new_pid ) == pids_set.end( ) )
				{
					pids.emplace_back( new_pid );
					processes.emplace_back( process::core::get_process_info( new_pid ) );

					pids_set.insert( new_pid );

					this->sort_it( this->type );
				}
			}

			for ( auto it = pids.begin( ); it != pids.end( ); )
			{
				if ( new_pids_set.find( *it ) == new_pids_set.end( ) )
				{
					auto process_it = std::find_if( processes.begin( ), processes.end( ), [ & ]( const PROCESS_INFO &process )
					{
						return process.pid == *it;
					} );

					if ( process_it != processes.end( ) )
						processes.erase( process_it );

					it = pids.erase( it );

					continue;
				}

				++it;
			}

			if ( this->search[ 0 ] )
			{
				const std::string search_lower = utils::string::to_lower<std::string>( this->search );
				std::unordered_set<std::uint32_t> searched_pids_set( searched_pids.begin( ), searched_pids.end( ) );

				for ( const auto &p : processes )
				{
					const std::string proc_name_lower = utils::string::to_lower<std::string>( p.process_name );

					if ( proc_name_lower.find( search_lower ) != std::string::npos || std::to_string( p.pid ).find( search_lower ) != std::string::npos )
					{
						if ( !searched_pids_set.contains( p.pid ) )
						{
							searched_pids.emplace_back( p.pid );
							searched_processes.emplace_back( p );
							searched_pids_set.insert( p.pid );
						}
					}
					else
					{
						auto it = std::find( searched_pids.begin( ), searched_pids.end( ), p.pid );

						if ( it != searched_pids.end( ) )
						{
							auto index = std::distance( searched_pids.begin( ), it );

							searched_pids.erase( it );
							searched_processes.erase( searched_processes.begin( ) + index );
						}
					}
				}
			}

			std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		}
	}

	auto c_watcher::update_service_info_buffer( ) -> void
	{
		while ( running )
		{
			if ( ui::c_window::current_tab != ui::c_window::tabs::TAB_SERVICES )
			{
				std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
				continue;
			}

			const auto new_services = process::core::get_every_service_info( );

			std::unordered_map<std::string, const SERVICE_INFO *> new_services_map;

			for ( const auto &new_svc : new_services )
			{
				new_services_map[ new_svc.service_name ] = &new_svc;
			}

			for ( auto &svc : services )
			{
				auto it = new_services_map.find( svc.service_name );
				if ( it != new_services_map.end( ) )
				{
					svc.status = it->second->status;
					svc.start_type = it->second->start_type;
				}
			}

			if ( this->search[ 0 ] )
			{
				searched_services.clear( );

				std::string search_lower;
				{
					std::lock_guard<std::mutex> lock( this->search_mutex );
					search_lower = utils::string::to_lower<std::string>( this->search );
				}

				std::unordered_set<std::uint32_t> searched_services_set;

				for ( const auto &svc : services )
				{
					if ( searched_services_set.contains( svc.pid ) )
						continue;

					const auto svc_name_lower = utils::string::to_lower<std::string>( svc.service_name );
					const auto description_lower = utils::string::to_lower<std::string>( svc.display_name );

					if ( svc_name_lower.find( search_lower ) != std::string::npos || description_lower.find( search_lower ) != std::string::npos )
					{
						searched_services.emplace_back( svc );
						searched_services_set.insert( svc.pid );
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
				process::sort::sort_by_name<PROCESS_INFO>( this->processes, [ ]( const PROCESS_INFO &p )
				{
					return p.process_name;
				} );

				process::sort::sort_by_name<PROCESS_INFO>( this->searched_processes, [ ]( const PROCESS_INFO &p )
				{
					return p.process_name;
				} );

				process::sort::sort_by_name<SERVICE_INFO>( this->services, [ ]( const SERVICE_INFO &s )
				{
					return s.service_name;
				} );

				process::sort::sort_by_name<SERVICE_INFO>( this->searched_services, [ ]( const SERVICE_INFO &s )
				{
					return s.service_name;
				} );

				break;
			}

			case sort_type::SORT_BY_PID:
			{
				process::sort::sort_by_pid<PROCESS_INFO>( this->processes, [ ]( const PROCESS_INFO &p )
				{
					return p.pid;
				} );

				process::sort::sort_by_pid<PROCESS_INFO>( this->searched_processes, [ ]( const PROCESS_INFO &p )
				{
					return p.pid;
				} );

				process::sort::sort_by_pid<SERVICE_INFO>( this->services, [ ]( const SERVICE_INFO &s )
				{
					return s.pid;
				} );

				process::sort::sort_by_pid<SERVICE_INFO>( this->searched_services, [ ]( const SERVICE_INFO &s )
				{
					return s.pid;
				} );

				break;
			}
		}
	}
}