#include <d3d11.h>
#include <d3dx11.h>
#include <__msvc_chrono.hpp>

#include "ext\ui\imgui\imgui.h"
#include "ext\ui\imgui\imgui_impl_win32.h"
#include "ext\ui\imgui\imgui_impl_dx11.h"

#define IMGUI_DEFINE_MATH_OPERATORS

import <Windows.h>;
import std;
import Fonts;
import Images;
import Widgets;
import Watcher;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

export import Ui;

namespace ui
{
	LRESULT WINAPI wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
			return true;

		switch ( msg )
		{
			case WM_DESTROY:
				::PostQuitMessage( 0 );
				return 0;
		}
		return ::DefWindowProcA( hWnd, msg, wParam, lParam );
	}

	bool c_window::create_device( )
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = w_context.hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[ 2 ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		HRESULT res = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &w_context.swap_chain, &w_context.g_pd3dDevice, &featureLevel, &w_context.device );
		if ( res == DXGI_ERROR_UNSUPPORTED )
			res = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &w_context.swap_chain, &w_context.g_pd3dDevice, &featureLevel, &w_context.device );
		if ( res != S_OK )
			return false;

		ID3D11Texture2D *pBackBuffer;
		w_context.swap_chain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
		w_context.g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &w_context.target_view );
		pBackBuffer->Release( );

		return true;
	}

	void c_window::cleanup_device( )
	{
		if ( w_context.target_view )
		{
			w_context.target_view->Release( );
			w_context.target_view = nullptr;
		}

		if ( w_context.swap_chain )
		{
			w_context.swap_chain->Release( );
			w_context.swap_chain = nullptr;
		}
		if ( w_context.device )
		{
			w_context.device->Release( );
			w_context.device = nullptr;
		}
		if ( w_context.g_pd3dDevice )
		{
			w_context.g_pd3dDevice->Release( );
			w_context.g_pd3dDevice = nullptr;
		}
	}

	bool c_window::create_window( )
	{
		try
		{
			w_context.wc = { sizeof( w_context.wc ), CS_CLASSDC, wnd_proc, 0L, 0L, GetModuleHandle( nullptr ), nullptr, nullptr, nullptr, nullptr, "window_class", nullptr };
			RegisterClassExA( &w_context.wc );

			w_context.hwnd = CreateWindowA( w_context.wc.lpszClassName, window_name, WS_POPUP, 100, 100, w_size.x, w_size.y, nullptr, nullptr, w_context.wc.hInstance, nullptr );

			if ( !create_device( ) )
			{
				cleanup_device( );
				UnregisterClassA( w_context.wc.lpszClassName, w_context.wc.hInstance );
				return false;
			}

			const auto init_centered = [ ]( HWND hwnd )
			{
				RECT rc;
				GetWindowRect( hwnd, &rc );

				auto size = std::make_pair( GetSystemMetrics( SM_CXSCREEN ) - rc.right, GetSystemMetrics( SM_CYSCREEN ) - rc.bottom );

				SetWindowPos( hwnd, nullptr, size.first / 2, size.second / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
			};

			init_centered( w_context.hwnd );

			ShowWindow( w_context.hwnd, SW_SHOWDEFAULT );
			UpdateWindow( w_context.hwnd );

			ImGui::CreateContext( );

			ImGui::StyleColorsDark( );

			ImGui_ImplWin32_Init( w_context.hwnd );
			ImGui_ImplDX11_Init( w_context.g_pd3dDevice, w_context.device );
		}
		catch ( std::exception &e )
		{
			cleanup_device( );

			MessageBoxA( nullptr, e.what( ), nullptr, MB_OK | MB_ICONERROR );
			return false;
		}

		return true;
	}

	bool c_window::render_window( char *name, float width, float height )
	{
		this->window_name = name;
		this->w_size = { width, height };

		if ( !create_window( ) )
			return false;

		D3DX11CreateShaderResourceViewFromMemory( w_context.g_pd3dDevice, images::exe_icon_bytes, sizeof( images::exe_icon_bytes ), NULL, NULL, &this->exe_icon, NULL );
		D3DX11CreateShaderResourceViewFromMemory( w_context.g_pd3dDevice, images::svc_icon_bytes, sizeof( images::svc_icon_bytes ), NULL, NULL, &this->svc_icon, NULL );

		menu_style( );

		MSG msg;
		std::memset( &msg, 0, sizeof( msg ) );

		while ( msg.message != WM_QUIT )
		{
			if ( !w_context.context_state )
				msg.message = WM_QUIT;

			while ( ::PeekMessageA( &msg, nullptr, 0U, 0U, PM_REMOVE ) )
			{
				::TranslateMessage( &msg );
				::DispatchMessageA( &msg );
				continue;
			}

			if ( w_context.swap_chain_oc && w_context.swap_chain->Present( 0, DXGI_PRESENT_TEST ) == DXGI_STATUS_OCCLUDED )
			{
				::Sleep( 10 );
				continue;
			}

			w_context.swap_chain_oc = false;

			if ( w_context.g_ResizeWidth != 0 && w_context.g_ResizeHeight != 0 )
			{
				if ( w_context.target_view )
				{
					w_context.target_view->Release( );
					w_context.target_view = nullptr;
				}

				w_context.swap_chain->ResizeBuffers( 0, w_context.g_ResizeWidth, w_context.g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0 );
				w_context.g_ResizeWidth = w_context.g_ResizeHeight = 0;

				ID3D11Texture2D *pBackBuffer;
				w_context.swap_chain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
				w_context.g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &w_context.target_view );
				pBackBuffer->Release( );
			}

			SetWindowLongA( w_context.hwnd, GWL_EXSTYLE, WS_EX_LAYERED );
			SetLayeredWindowAttributes( w_context.hwnd, RGB( 0, 0, 0 ), 255, LWA_COLORKEY | LWA_ALPHA );

			auto frame_start = std::chrono::high_resolution_clock::now( );

			ImGui_ImplDX11_NewFrame( );
			ImGui_ImplWin32_NewFrame( );
			ImGui::NewFrame( );
			{
				menu_design( );
			}
			ImGui::Render( );

			const float clear_color_with_alpha[ 4 ] = { 0, 0, 0, 0 };
			w_context.device->OMSetRenderTargets( 1, &w_context.target_view, nullptr );
			w_context.device->ClearRenderTargetView( w_context.target_view, clear_color_with_alpha );
			ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

			HRESULT hr = w_context.swap_chain->Present( 1, 0 );
			w_context.swap_chain_oc = ( hr == DXGI_STATUS_OCCLUDED );

			auto frame_end = std::chrono::high_resolution_clock::now( );
			float frame_time = std::chrono::duration<float, std::milli>( frame_end - frame_start ).count( );

			if ( frame_time < 16.67f )
			{
				auto sleep_time = std::chrono::duration<float, std::milli>( 16.67f - frame_time );
				std::this_thread::sleep_for( sleep_time / 2 );
			}
		}

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );

		cleanup_device( );
		DestroyWindow( w_context.hwnd );
		UnregisterClassA( w_context.wc.lpszClassName, w_context.wc.hInstance );

		return true;
	}

	void c_window::menu_style( )
	{
		static auto &io = ImGui::GetIO( );
		static auto &style = ImGui::GetStyle( );

		consolas = io.Fonts->AddFontFromMemoryCompressedBase85TTF( fonts::consolas_compressed.data( ), 14 );

		io.LogFilename = nullptr;
		io.IniFilename = nullptr;

		style.Colors[ ImGuiCol_WindowBg ] = ImColor( 43, 43, 43 );
		style.Colors[ ImGuiCol_ChildBg ] = ImColor( 43, 43, 43 );

		style.Colors[ ImGuiCol_Button ] = ImColor( 40, 40, 40, 0 );
		style.Colors[ ImGuiCol_ButtonHovered ] = ImColor( 150, 150, 150, 100 );
		style.Colors[ ImGuiCol_ButtonActive ] = ImColor( 150, 150, 150, 100 );

		style.Colors[ ImGuiCol_FrameBg ] = ImColor( 23, 23, 23 );
		style.Colors[ ImGuiCol_FrameBgHovered ] = ImColor( 25, 25, 25 );
		style.Colors[ ImGuiCol_FrameBgActive ] = ImColor( 25, 25, 25 );

		style.Colors[ ImGuiCol_TableBorderStrong ] = ImColor( 0, 0, 0, 0 );
		style.Colors[ ImGuiCol_TableBorderLight ] = ImColor( 90, 90, 90 );

		style.Colors[ ImGuiCol_ScrollbarBg ] = ImColor( 0, 0, 0, 0 );

		style.WindowPadding = { 0, 0 };
		style.WindowRounding = 4;
		style.WindowBorderSize = 0;
		style.ChildRounding = 0;
		style.FrameRounding = 0;
		style.ScrollbarRounding = 0;
		style.GrabRounding = 0;
	}

	void c_window::menu_design( )
	{
		constexpr auto flags { ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar };

		ImGui::SetNextWindowPos( { 0, 0 }, ImGuiCond_::ImGuiCond_Always );
		ImGui::SetNextWindowSize( w_size, ImGuiCond_::ImGuiCond_Always );

		ImGui::Begin( "###main_painel", nullptr, flags );
		{
			ImGui::InvisibleButton( "##move", { w_size.x - 50, 30 } );
			if ( ImGui::IsItemActive( ) )
			{
				RECT rect;
				GetWindowRect( w_context.hwnd, &rect );
				SetWindowPos( w_context.hwnd, nullptr, rect.left + ImGui::GetMouseDragDelta( ).x, rect.top + ImGui::GetMouseDragDelta( ).y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
			}

			static auto *draw = ImGui::GetWindowDrawList( );
			static auto padding = ImVec2( 0, 53 );

			draw->AddRectFilled( { 0, 0 }, { w_size.x, 53 }, ImColor( 35, 35, 35 ), 4.f, ImDrawFlags_RoundCornersTop );
			ImGui::SetCursorPos( { 10, 10 } );
			ImGui::Text( "win-watch | FPS: %d", static_cast< std::uint32_t >( ImGui::GetIO( ).Framerate ) );

			ImGui::SetCursorPos( { w_size.x - 20, 0 } );
			if ( ImGui::Button( "X", { 20,20 } ) )
			{
				watcher_->running = false;
				w_context.context_state = false;
			}

			ImGui::SetCursorPos( { 0, 28 } );
			widgets::tabs::add_tab( "Processes", tabs::TAB_PROCESSES, current_tab == tabs::TAB_PROCESSES );
			ImGui::SameLine( 0, 0 );
			widgets::tabs::add_tab( "Services", tabs::TAB_SERVICES, current_tab == tabs::TAB_SERVICES );

			ImGui::SetNextItemWidth( 180 );
			ImGui::SetCursorPos( ImVec2( w_size.x - 200, 33 ) );

			static char search_buffer[ 64 ] = { 0 };

			if ( ImGui::InputTextWithHint( "##search", "search", search_buffer, 64 ) )
			{
				std::lock_guard<std::mutex> lock( watcher_->search_mutex );
				strcpy_s( watcher_->search, search_buffer );
			}

			ImGui::SameLine( 0, 0 );

			if ( ImGui::Button( "o", { 20,20 } ) )
				strcpy_s( watcher_->search, "" );

			ImGui::SetCursorPos( padding );

			switch ( current_tab )
			{
				case tabs::TAB_PROCESSES:
				{
					widgets::table::process::begin( "process_table", { w_size.x, w_size.y - 60 } );
					{
						if ( watcher_->search[ 0 ] )
						{
							for ( const auto &search : watcher_->searched_processes )
							{
								if ( search.pid == 0 )
									continue;

								widgets::table::process::append(
									search.process_name.c_str( ),
									search.pid,
									search.process_description.c_str( )
								);
							}
						}
						else
						{
							for ( const auto &proc : watcher_->processes )
							{
								if ( proc.pid == 0 )
									continue;

								widgets::table::process::append(
									proc.process_name.c_str( ),
									proc.pid,
									proc.process_description.c_str( )
								);
							}
						}
					}
					widgets::table::process::end( );

					break;
				}

				case tabs::TAB_SERVICES:
				{
					widgets::table::service::begin( "service_table", { w_size.x, w_size.y - 60 } );
					{
						if ( watcher_->search[ 0 ] )
						{
							for ( const auto &search : watcher_->searched_services )
							{
								widgets::table::service::append( search.service_name.c_str( ),
																 search.pid,
																 search.display_name.c_str( ),
																 search.status,
																 search.start_type
								);
							}
						}
						else
						{
							for ( const auto &svc : watcher_->services )
							{
								widgets::table::service::append( svc.service_name.c_str( ),
																 svc.pid,
																 svc.display_name.c_str( ),
																 svc.status,
																 svc.start_type
								);
							}
						}
					}
					widgets::table::service::end( );

					break;
				}
			}

			ImGui::End( );
		}
	}
}