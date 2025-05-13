import std;
import Watcher;
import Ui;
import <Windows.h>;

int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, CHAR *lpCmdLine, int nShowCmd )
{
	std::thread( &watcher::c_watcher::update_process_info_buffer, watcher_.get( ) ).detach( );
	std::thread( &watcher::c_watcher::update_service_info_buffer, watcher_.get( ) ).detach( );

	const auto window_ { std::make_unique<ui::c_window>( ) };

	if ( !window_->render_window( "win-w", 1280, 720 ) )
		throw std::runtime_error( "Failed to create window" );

	return EXIT_SUCCESS;
}