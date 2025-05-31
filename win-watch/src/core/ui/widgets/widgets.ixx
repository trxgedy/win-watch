module;

#include "ext\ui\imgui\imgui.h"
#include <D3DX11.h>

import std;
import Fonts;
import Ui;
import Watcher;
import Utils;
import Process;

export module Widgets;

export namespace widgets
{
	namespace properties
	{
		std::unordered_map< std::uint32_t, bool > properties_tabs_open;

		void open( const std::uint32_t pid )
		{
			constexpr auto flags { ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar };

			ImGui::PushStyleColor( ImGuiCol_WindowBg, ImColor( 32, 32, 32 ).Value );
			ImGui::PushStyleColor( ImGuiCol_ChildBg, ImColor( 32, 32, 32 ).Value );
			ImGui::PushStyleColor( ImGuiCol_Button, ImColor( 142, 22, 22 ).Value );

			ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 5.f );

			ImGui::SetNextWindowSize( { 445, 570 } );
			ImGui::Begin( "##properties", false, flags );
			{
				ImGui::SetCursorPos( { 10, 10 } );
				ImGui::BeginChild( "properties_child", { 430, 555 } );
				{
					static auto process_info = process::core::get_process_info( pid );
					static auto process_modules = process::properties::get_process_modules( pid );

					ImGui::Text( "Properties: %s ( %d )", process_info.process_name.c_str( ), pid );

					ImGui::Separator( );
					ImGui::NewLine( );

					ImGui::Text( "Name: %s", process_info.process_name.c_str( ) );
					ImGui::Text( "PID: %d", process_info.pid );
					ImGui::Text( "Description: %s", process_info.process_description.c_str( ) );
					ImGui::Text( "Path: %s", process_modules.at( 1 ).module_path.c_str( ) );

					ImGui::NewLine( );
					ImGui::Separator( );
					ImGui::NewLine( );

					if ( ImGui::Button( "Terminate", { 200, 30 } ) )
						process::actions::terminate_process( pid );

					if ( ImGui::Button( "Suspend", { 200, 30 } ) )
						process::actions::suspend_process( pid );

					if ( ImGui::Button( "Resume", { 200, 30 } ) )
						process::actions::resume_process( pid );
				}
				ImGui::EndChild( );
			}
			ImGui::End( );

			ImGui::PopStyleVar( );
			ImGui::PopStyleColor( 3 );
		}
	}

	namespace table
	{
		namespace process
		{
			void begin( const char *str_id, ImVec2 size )
			{
				static bool resize_once = true;

				ImGui::BeginChild( str_id, size, 0, ImGuiWindowFlags_NoScrollbar );
				{
					ImGui::Columns( 3, "##columns", true );

					if ( resize_once )
					{
						ImGui::SetColumnWidth( 1, 80 );
						resize_once = false;
					}

					ImGui::Text( "\n\nName" );
					ImGui::SameLine( -8, 0 );

					if ( ImGui::Button( "##name", { ImGui::GetColumnWidth( ), 50 } ) )
						watcher_->sort_it( watcher::c_watcher::sort_type::SORT_BY_NAME );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nPID" );
					ImGui::SameLine( -8, 0 );

					if ( ImGui::Button( "##pid", { ImGui::GetColumnWidth( ), 50 } ) )
						watcher_->sort_it( watcher::c_watcher::sort_type::SORT_BY_PID );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nDescription" );
				}
			}

			void append( const char *name, std::uint32_t pid, const char *description )
			{
				ImGui::NextColumn( );
				ImGui::Image( reinterpret_cast< ImTextureID >( ui::c_window::exe_icon ), { 15,10 } );
				ImGui::SameLine( 0, 5 );
				ImGui::Text( "%s", name );

				ImGui::SameLine( -30 );
				if ( ImGui::Button( std::to_string( pid ).c_str( ), ImVec2( 1280, ImGui::GetTextLineHeight( ) ) ) )
					properties::properties_tabs_open[ pid ] = true;

				ImGui::NextColumn( );
				ImGui::Text( "%d", pid );

				ImGui::NextColumn( );
				ImGui::Text( "%s", description );
			}

			void end( )
			{
				ImGui::EndChild( );
			}
		}

		namespace service
		{
			constexpr std::array< const char *, 7 > service_status = { "Stopped", "Start pending", "Stop pending", "Running", "Continue pending", "Pause pending", "Paused" };
			constexpr std::array< const char *, 5 > startup_types = { "Boot start", "System start", "Auto start", "Demand start", "Disabled" };

			void begin( const char *str_id, ImVec2 size )
			{
				static bool resize_once = true;

				ImGui::BeginChild( str_id, size, 0, ImGuiWindowFlags_NoScrollbar );
				{
					ImGui::Columns( 5, "##service_columns", true );

					if ( resize_once )
					{
						ImGui::SetColumnWidth( 0, 400 );
						ImGui::SetColumnWidth( 1, 80 );
						ImGui::SetColumnWidth( 2, 420 );
						ImGui::SetColumnWidth( 3, 150 );
						ImGui::SetColumnWidth( 4, 200 );
						resize_once = false;
					}

					ImGui::Text( "\n\nName" );
					ImGui::SameLine( -8, 0 );

					if ( ImGui::Button( "##process_name", { ImGui::GetColumnWidth( ) + 10, 50 } ) )
						watcher_->sort_it( watcher::c_watcher::sort_type::SORT_BY_NAME );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nPID" );
					ImGui::SameLine( -8, 0 );

					if ( ImGui::Button( "##service_pid", { ImGui::GetColumnWidth( ) + 10, 50 } ) )
						watcher_->sort_it( watcher::c_watcher::sort_type::SORT_BY_PID );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nDisplay name" );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nStatus" );

					ImGui::NextColumn( );
					ImGui::Text( "\n\nStart type" );

					// spacing
					ImGui::NextColumn( );
					ImGui::Text( " " );
					ImGui::NextColumn( );
					ImGui::Text( " " );
					ImGui::NextColumn( );
					ImGui::Text( " " );
					ImGui::NextColumn( );
					ImGui::Text( " " );
					ImGui::NextColumn( );
					ImGui::Text( " " );
				}
			}

			void append( const char *name, std::uint32_t pid, const char *display_name, std::uint32_t status, std::uint32_t start_type )
			{
				ImGui::NextColumn( );
				ImGui::Image( reinterpret_cast< ImTextureID >( ui::c_window::svc_icon ), { 15,15 } );
				ImGui::SameLine( 0, 5 );
				ImGui::Text( "%s", name );

				ImGui::SameLine( -30 );
				ImGui::InvisibleButton( utils::string::generate_random_string( ).c_str( ), ImVec2( 1280, ImGui::GetTextLineHeight( ) ) );

				ImGui::NextColumn( );

				if ( pid )
					ImGui::Text( "%d", pid );

				ImGui::NextColumn( );
				ImGui::Text( "%s", display_name );

				ImGui::NextColumn( );
				ImGui::Text( "%s", service_status.at( status - 1 ) );

				ImGui::NextColumn( );
				ImGui::Text( "%s", startup_types.at( start_type ) );
			}

			void end( )
			{
				ImGui::EndChild( );
			}
		}
	}

	namespace tabs
	{
		void add_tab( const char *title, ui::c_window::tabs tab, bool is_active = false )
		{
			if ( is_active )
			{
				ImGui::PushStyleColor( ImGuiCol_Button, ImColor( 120, 120, 120, 100 ).Value );
				ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImColor( 120, 120, 120, 100 ).Value );
				ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImColor( 150, 150, 150, 180 ).Value );
			}

			if ( ImGui::Button( title, { 100, 25 } ) )
			{
				ui::c_window::current_tab = tab;
			}

			if ( is_active )
				ImGui::PopStyleColor( 3 );
		}
	}
}