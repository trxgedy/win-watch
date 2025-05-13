#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <Windows.h>

#include <shared_mutex>
#include <format>
#include <iostream>

namespace console
{
	enum class MessageType : std::uint32_t
	{
		EDEBG = 9,
		ESUCS = 10,
		EERRO = 12,
		EWARN = 14
	};

	inline std::ostream &operator<< ( std::ostream &os, const MessageType type )
	{
		switch ( type )
		{
			case MessageType::EDEBG:	return os << "/";
			case MessageType::ESUCS:	return os << "+";
			case MessageType::EERRO:	return os << "-";
			case MessageType::EWARN:	return os << "!";
			default: return os << "";
		}
	}

	class ConsoleLogger
	{
	private:
		std::shared_timed_mutex mutex;

	public:
		ConsoleLogger( const std::string_view title_name = {} )
		{
			AllocConsole( );
			AttachConsole( GetCurrentProcessId( ) );

			if ( !title_name.empty( ) )
			{
				SetConsoleTitleA( title_name.data( ) );
			}

			FILE *conin {}, *conout {};

			freopen_s( &conin, "conin$", "r", stdin );
			freopen_s( &conout, "conout$", "w", stdout );
			freopen_s( &conout, "conout$", "w", stderr );
		}

		~ConsoleLogger( )
		{
			const auto handle = FindWindowA( "ConsoleWindowClass", nullptr );
			ShowWindow( handle, SW_HIDE );
			FreeConsole( );
		}

		template< typename ... arg >
		void print( const MessageType type, const std::string_view &func, const std::string_view &format, arg &&... args )
		{
			static auto *h_console = GetStdHandle( STD_OUTPUT_HANDLE );
			std::unique_lock<decltype( mutex )> lock( mutex );

			SetConsoleTextAttribute( h_console, static_cast< WORD >( type ) );
			std::cout << "[" << type << "] ";

			SetConsoleTextAttribute( h_console, 15 );
			std::cout << std::vformat( format, std::make_format_args( args... ) ) << std::endl;
		}
	};
}

#ifdef _DEBUG

const auto _logger = std::make_unique<console::ConsoleLogger>( "console" );

#define log_suc(...)	_logger->print( console::MessageType::ESUCS, __FUNCTION__, __VA_ARGS__ )
#define log_dbg(...)	_logger->print( console::MessageType::EDEBG, __FUNCTION__, __VA_ARGS__ )
#define log_err(...)	_logger->print( console::MessageType::EERRO, __FUNCTION__, __VA_ARGS__ )
#define log_wrn(...)	_logger->print( console::MessageType::EWARN, __FUNCTION__, __VA_ARGS__ )

#else

#define log_dbg(...)
#define log_suc(...)
#define log_err(...)
#define log_wrn(...)

#endif

#endif