/*
 *  Uzebox Default Patches
 *  Copyright (C) 2008  Alec Bourque
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
	Patches are made of a PatchStruct type.
	
	Patches have 3 types defined in a PatchStruct:
		0=wave channel (i.e.; channel 0,1 and 2)
		1=noise channel (channel 3)
		2=PCM (channel 3)

	Can have a pcm data pointer (or NULL), loop start and loop end pointers. If no loop
	is required, add a zero sample (signed) at the end and set both loop start and loop end 
	on that sample's address.

	Patches can have a command/modifiers stream made of 3 bytes per command (or NULL):
		1=delta time
		2=command
		3=command parameter

		It must end with <0,PATCH_END> and this command takes only two bytes (no parameter).
*/

//External PCM samples
#include "rocketman-track.inc"


//INST: Synth Piano
const char patch00[] PROGMEM ={ 
0,PC_WAVE,6,
0,PC_ENV_SPEED,-5,
0,PATCH_END
};



const char bomb[] PROGMEM = { 
0,PC_ENV_VOL,0xff,
0,PC_WAVE,3,
0,PC_ENV_SPEED,-4,
0,PC_PITCH,40,
1,PC_NOTE_DOWN,5, 
1,PC_NOTE_DOWN,4, 
1,PC_NOTE_DOWN,3, 
1,PC_NOTE_DOWN,2, 
1,PC_NOTE_DOWN,1,
0,PC_TREMOLO_LEVEL,0x90, 
0,PC_TREMOLO_RATE,0x60, 
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_DOWN,1,
2,PC_NOTE_CUT,0,
0,PATCH_END 
};


const char coinCollect[] PROGMEM = {
0,PC_WAVE,2,
0,PC_PITCH,83,
0,PC_ENV_VOL,0xff,
0,PC_TREMOLO_LEVEL,0x90,     
0,PC_TREMOLO_RATE,60, 
2,PC_NOTE_DOWN,6,
2,PC_NOTE_UP,6,
0,PC_ENV_SPEED,-10,
0,PATCH_END
};





const struct PatchStruct patches[] PROGMEM = 
{
	{0,NULL,patch00,0,0},	
	{0,NULL,bomb,0,0},
	{2,rocketman_song,NULL,0,sizeof_rocketman_song},
	

};
