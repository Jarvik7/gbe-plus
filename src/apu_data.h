// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu_data.h
// Date : March 04, 2015
// Description : Core data
//
// Defines the APU data structures that the MMU will update whenever values are written in memory
// Only the APU should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GBA_APU_DATA
#define GBA_APU_DATA

#include <vector>

#include "common.h"

struct apu_data
{
	//Sound Channels 1-4, nearly identical to the DMG/GBC
	struct sound_channels
	{
		u16 raw_frequency;
		double output_frequency;
		u32 duration;
		u32 volume;
		bool playing;

		u32 duty_cycle_start;
		u32 duty_cycle_end;
	} channels[4];

	bool sound_on;
	bool stereo;

	u8 channel_left_volume;
	u8 channel_right_volume;
	u8 dma_left_volume;
	u8 dma_right_volume;
};

#endif // GBA_APU_DATA
 