module;

#include "ext\structs\structs.hpp"

import std;
import Process;

export module Watcher;

export namespace watcher
{
	class c_watcher
	{
	public:
		inline static bool running = true;

		std::vector<std::uint32_t> pids = process::core::get_all_pids( );

		std::vector<PROCESS_INFO> processes;
		std::vector<SERVICE_INFO> services;

		enum sort_type
		{
			SORT_BY_NAME,
			SORT_BY_PID
		};

		sort_type type;

		c_watcher( );
		~c_watcher( ) = default;

		auto update_process_info_buffer( ) -> void;	
		auto update_service_info_buffer( ) -> void;
		auto sort_it( sort_type type ) -> void;
	};
}

export inline auto watcher_ = std::make_unique<watcher::c_watcher>( );