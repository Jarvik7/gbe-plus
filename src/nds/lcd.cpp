// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.cpp
// Date : August 16, 2014
// Description : NDS LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include <cmath>

#include "lcd.h"

/****** LCD Constructor ******/
NTR_LCD::NTR_LCD()
{
	reset();
}

/****** LCD Destructor ******/
NTR_LCD::~NTR_LCD()
{
	screen_buffer.clear();
	scanline_buffer_a.clear();
	scanline_buffer_b.clear();
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void NTR_LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	scanline_buffer_a.clear();
	scanline_buffer_b.clear();
	screen_buffer.clear();

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	current_scanline = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x18000, 0);
	scanline_buffer_a.resize(0x100, 0);
	scanline_buffer_b.resize(0x100, 0);

	lcd_stat.bg_mode = 0;
	lcd_stat.hblank_interval_free = false;

	for(int x = 0; x < 4; x++)
	{
		lcd_stat.bg_control[x] = 0;
		lcd_stat.bg_enable[x] = false;
		lcd_stat.bg_offset_x[x] = 0;
		lcd_stat.bg_offset_y[x] = 0;
		lcd_stat.bg_priority[x] = 0;
		lcd_stat.bg_depth[x] = 4;
		lcd_stat.bg_size[x] = 0;
		lcd_stat.bg_base_tile_addr[x] = 0x6000000;
		lcd_stat.bg_base_map_addr[x] = 0x6000000;
	}
}

/****** Initialize LCD with SDL ******/
bool NTR_LCD::init()
{
	if(config::sdl_render)
	{
		if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
		{
			std::cout<<"LCD::Error - Could not initialize SDL\n";
			return false;
		}

		final_screen = SDL_SetVideoMode(256, 384, 32, SDL_SWSURFACE);

		if(final_screen == NULL) { return false; }
	}

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Updates palette entries when values in memory change ******/
void NTR_LCD::update_palettes()
{
	//Update BG palettes
	if(lcd_stat.bg_pal_update)
	{
		lcd_stat.bg_pal_update = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_pal_update_list[x])
			{
				lcd_stat.bg_pal_update_list[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000000 + (x << 1));
				lcd_stat.raw_pal[x][0] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.pal[x][0] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}
}

/****** Render the line for a BG ******/
bool NTR_LCD::render_bg_scanline(u32 bg_control)
{
	if(!lcd_stat.bg_enable[(bg_control - 0x4000008) >> 1]) { return false; }

	//Render BG pixel according to current BG Mode
	switch(lcd_stat.bg_mode)
	{
		//BG Mode 0
		case 0:
			return render_bg_mode_0(bg_control); break;

		default:
			std::cout<<"LCD::invalid or unsupported BG Mode : " << std::dec << (lcd_stat.display_control & 0x7);
			return false;
	}
}

/****** Render BG Mode 0 scanline ******/
bool NTR_LCD::render_bg_mode_0(u32 bg_control) { }

/****** Render BG Mode 1 scanline ******/
bool NTR_LCD::render_bg_mode_1(u32 bg_control) { }

/****** Render BG Mode 3 scanline ******/
bool NTR_LCD::render_bg_mode_3(u32 bg_control) { }

/****** Render BG Mode 4 scanline ******/
bool NTR_LCD::render_bg_mode_4(u32 bg_control) { }

/****** Render BG Mode 5 scanline ******/
bool NTR_LCD::render_bg_mode_5(u32 bg_control) { }

/****** Render BG Mode 6 ******/
bool NTR_LCD::render_bg_mode_6(u32 bg_control) { }

/****** Render pixels for a given scanline (per-pixel) ******/
void NTR_LCD::render_scanline()
{
	//Use BG Palette #0, Color #0 as the backdrop
}

/****** Immediately draw current buffer to the screen ******/
void NTR_LCD::update()
{
	//Use SDL
	if(config::sdl_render)
	{
		//Lock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
		u32* out_pixel_data = (u32*)final_screen->pixels;

		for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

		//Unlock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
		if(SDL_Flip(final_screen) == -1) { std::cout<<"LCD::Error - Could not blit\n"; }
	}

	//Use external rendering method (GUI)
	else { config::render_external(screen_buffer); }
}


/****** Run LCD for one cycle ******/
void NTR_LCD::step()
{
	lcd_clock++;

	//Mode 0 - Scanline rendering
	if(((lcd_clock % 2130) <= 1536) && (lcd_clock < 408960)) 
	{
		//Change mode
		if(lcd_mode != 0) 
		{
			lcd_mode = 0;
			
			current_scanline++;
			if(current_scanline == 263) { current_scanline = 0; }
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_clock % 2130) > 1536) && (lcd_clock < 408960))
	{
		//Change mode
		if(lcd_mode != 1) 
		{
			//Render scanline data
			render_scanline();

			lcd_mode = 1;

			//Push scanline data to final buffer - Only if Forced Blank is disabled
			if((lcd_stat.display_control & 0x80) == 0)
			{
				for(int x = 0, y = (256 * current_scanline); x < 256; x++, y++)
				{
					screen_buffer[y] = scanline_buffer_a[x];
					screen_buffer[y + 0xc000] = scanline_buffer_b[x];
				}
			}

			//Draw all-white during Forced Blank
			else
			{
				for(int x = 0, y = (256 * current_scanline); x < 256; x++, y++)
				{
					screen_buffer[y] = 0xFFFFFFFF;
					screen_buffer[y + 0xc000] = 0xFFFFFFFF;
				}
			}
		}
	}

	//Mode 2 - VBlank
	else
	{
		//Change mode
		if(lcd_mode != 2) 
		{
			lcd_mode = 2;

			//Increment scanline count
			current_scanline++;

			//Use SDL
			if(config::sdl_render)
			{
				//Lock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
				u32* out_pixel_data = (u32*)final_screen->pixels;

				for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

				//Unlock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
				
				if(SDL_Flip(final_screen) == -1) { std::cout<<"LCD::Error - Could not blit\n"; }
			}

			//Use external rendering method (GUI)
			else { config::render_external(screen_buffer); }

			//Limit framerate
			if(!config::turbo)
			{
				frame_current_time = SDL_GetTicks();
				if((frame_current_time - frame_start_time) < 16) { SDL_Delay(16 - (frame_current_time - frame_start_time));}
				frame_start_time = SDL_GetTicks();
			}

			//Update FPS counter + title
			fps_count++;
			if(((SDL_GetTicks() - fps_time) >= 1000) && (config::sdl_render))
			{ 
				fps_time = SDL_GetTicks(); 
				config::title.str("");
				config::title << "GBE+ " << fps_count << "FPS";
				SDL_WM_SetCaption(config::title.str().c_str(), NULL);
				fps_count = 0; 
			}
		}

		//Reset LCD clock
		else if(lcd_clock == 558060) 
		{
			lcd_clock = 0; 
		}

		//Increment Scanline after HBlank
		else if(lcd_clock % 1536 == 0)
		{
			current_scanline++;
		}
	}
}