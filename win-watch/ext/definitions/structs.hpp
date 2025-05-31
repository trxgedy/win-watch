#ifndef STRUCTS_HPP
#define STRUCTS_HPP

import std;

enum service_status : std::uint32_t
{
	STOPPED = 1,
	START_PENDING = 2,
	STOP_PENDING = 3,
	RUNNING = 4,
	CONTINUE_PENDING = 5,
	PAUSE_PENDING = 6,
	PAUSED = 7,
};

enum service_start_stype : std::uint32_t
{
	BOOT_START = 0,
	SYSTEM_START = 1,
	AUTO_START = 2,
	DEMAND_START = 3,
	DISABLED = 4
};

typedef struct PROCESS_INFO
{
	std::uint32_t pid {};

	std::string process_name {};
	std::string process_description {};
};

typedef struct SERVICE_INFO
{
	std::uint32_t pid;
	std::uint32_t status;
	std::uint32_t start_type;

	std::string service_name;
	std::string display_name;
};

typedef struct PROCESS_MODULE_LIST
{
	std::uint64_t module_base_address {};
	std::string module_name;
	std::string module_path;
};

typedef struct THREAD_PROPERTIES
{
	std::uint32_t tid {};
	std::uint32_t priority {};
	std::uint64_t start_address {};
};

typedef struct WINDOW_PROPERTIES
{
	std::string window_class;
	std::string window_title;
	std::string module {};

	std::uint32_t handle {};
};

#endif