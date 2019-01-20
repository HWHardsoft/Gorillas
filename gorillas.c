/*
 *  Gorillas for Uzebox - remake of the popular QBASIC game
 *  Version 1.1
 *  Copyright (C) 2011  Hartmut Wendt
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

#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "kernel/uzebox.h"
#include "data/backgroundTiles.pic.inc"
#include "data/fonts.pic.inc"
#include "data/gorillas_sprites4.inc"
#include "data/lutsin.inc"
#include "data/patches.inc"
#include "data/gorillas-tracks.inc"




/* global definitons */
// program modes
#define PM_Intro	0	// program mode intro
#define PM_Enter_Player_Count 	1	// program mode: enter the count of players in game menu
#define PM_Enter_Player1_Name 	2	// program mode: enter the name of player 1 in game menu
#define PM_Enter_Player2_Name 	3	// program mode: enter the name of player 2 in game menu
#define PM_Enter_Point_Count 	4	// program mode: enter the count of points in game menu
#define PM_Show_Gorilla_Dance 	5	// program mode: shows the gorilla dance


#define PM_Prepare_Game			9	// program mode: prepare play ground and skyline for game

#define PM_Enter_Angle_P1		11	// program mode: enter the throw angle for player 1
#define PM_Enter_Power_P1		12	// program mode: enter the throw power for player 1

#define PM_Enter_Angle_P2		16	// program mode: enter the throw angle for player 2
#define PM_Enter_Power_P2		17	// program mode: enter the throw power for player 2

#define PM_THROW_P1				10	// program mode player 1 banana throw
#define PM_THROW_P2 			15	// program mode player 2 banana throw


#define PM_Game_over  			20	// program mode for game over screen

#define PM_Crash_Skyscraper_P1  30	// banana of player 1 hits skyscraper
#define PM_Crash_Skyscraper_P2  31	// banana of player 2 hits skyscraper
#define PM_Crash_Gorilla_P1  	32	// banana of player 1 hits gorilla
#define PM_Crash_Gorilla_P2  	33	// banana of player 2 hits gorilla




// colors
#define red			0
#define cyan		1
#define grey		2

// banana hits
#define hit_nothing		0
#define hit_gorilla 	1
#define hit_skyscraper	2
#define wide_throw		3
#define hit_sun     	4



// 8-bit, 255 period LFSR (for generating pseudo-random numbers)
#define PRNG_NEXT() (prng = ((u8)((prng>>1) | ((prng^(prng>>2)^(prng>>3)^(prng>>4))<<7))))
#define max_cards 48
#define gravity 10




//#define delta_t 1 

#define MAX(x,y) ((x)>(y) ? (x) : (y))


struct EepromBlockStruct ebs;



u8 prng; // Pseudo-random number generator
int PosX=0, PosY=0;
u8 program_mode;	// program mode (intro, 1 player mode, 2 player mode, ....
u8 PLAYER;	// 0 = Player 1 / 1 = Player 2
u8 ani_count=0;		// counter for animation
//u8 Music_on = true;
u8 Ypos_gorilla1;
u8 Ypos_gorilla2;
u8 PlayerCNT = 1; // count of players
char Player1_Name[9]={"PLAYER1 "};
char Player2_Name[9]={"PLAYER2 "};
u8 ThrowAngle;
u8 ThrowPower;
u8 StartYPos;
u8 StartXPos = 0;
u8 ScoreP1;
u8 ScoreP2;
u8 Total_score_points = 3;
u8 StartPlayer = 0;
char Windspeed = 0;
char Windforce;
char CPU_Angle;
char CPU_Speed;
double t = 0;
u8 Ypos_left_skyscraper;
u8 Controller_status2;
u8 autorepeat_cnt;

//int Xold=0;
//long Yold=0;
/*
int V0V;
int V0H;
*/
double InitYVel; 
double InitXVel;



/*** function prototypes *****************************************************************/
void init(void);
void set_PM_mode(u8 mode);
void msg_window(u8 x1, u8 y1, u8 x2, u8 y2);
u8 set_def_EEPROM(void);
void load_def_EEPROM(void);
void save_def_EEPROM(void);
void create_new_skyline(void);
void draw_skyscraper(u8 position, u8 floors);
const char * get_skyscraper_map(u8 floor, u8 color);
void animate_banana(u8 xx, int yy);
void calculate_position(double dt);
void animate_star_frame(void);
bool edit_value(u8 *value, u8 min, u8 max, u8 Xpos, u8 Ypos, int *b_mode, int *b_mode_old);
void edit_name(char *value, u8 *entry, int *b_mode, int *b_mode_old);
void PrintName(int x, int y, char *s, bool Bcursor, u8 CurPos);
void animate_2gorillas_dance(void);
void clear_text(void);
u8 GetTile(u8 x, u8 y);
char get_tile_pixel(unsigned char x,unsigned char y,char tile, const char *tileset);
char get_sprite_tile(unsigned char x,unsigned char y);
char checkcollision(int xx, int yy);
void destroy_skyscraper(int xx, int yy);
void show_score(void);
void animate_gorilla_victory(u8 xx, u8 yy);
void draw_wind_arrow(void);
void AI_calculation(void);
void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ);


void init(void)
// init program
{
  // init tile table
  SetTileTable(BGTiles);
  // init font table
  SetFontTilesIndex(BGTILES_SIZE);
  // init Sprites
  SetSpritesTileTable(spriteTiles);	

  // init music	
  InitMusicPlayer(patches);
  // load into screen
  set_PM_mode(PM_Intro);
  // init random generator

  Controller_status2 = DetectControllers() & 0x0C;	

  prng = 15;
  //Use block 22
  ebs.id = 22;
  if (!isEepromFormatted())
     return;

  if (EEPROM_ERROR_BLOCK_NOT_FOUND == EepromReadBlock(22,&ebs))
  {
	set_def_EEPROM();
  }	

}



int main(){
int ibuttons=0,ibuttons_old;


u8 b = 0;



  // init program
  init();        
  // proceed game	
  while(1)
  {
    WaitVsync(3);	  
    // get controller state
    ibuttons_old = ibuttons;
	ibuttons = ReadJoypad(PLAYER);
    switch(program_mode)
	{
	  // proceed intro mode	
	  case PM_Intro:
	  // proceed game over mode	
	  case PM_Game_over:
	    animate_star_frame(); 
 		if (((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) || 
		   ((BTN_B & ibuttons) && !(ibuttons_old & BTN_B)) ||
		   ((BTN_START & ibuttons) && !(ibuttons_old & BTN_START)) ||
		   ((BTN_SELECT & ibuttons) && !(ibuttons_old & BTN_SELECT))) {
		   set_PM_mode(PM_Enter_Player_Count);	
		}

        break;


	  // proceed input of player count	
	  case PM_Enter_Player_Count:				
		

		if (edit_value(&PlayerCNT,1,2,21,6,&ibuttons,&ibuttons_old)) {
		  // wait for release of button A 
		  while (BTN_A & ReadJoypad(PLAYER)) {
		    prng++; 		
			WaitVsync(1);
		  }
      	  prng = MAX(prng,1);

		  set_PM_mode(PM_Enter_Player1_Name);
        } 
		
        break;
		

	  // proceed input of player 1 name	
	  case PM_Enter_Player1_Name:
						
		edit_name(&Player1_Name[0],&b,&ibuttons,&ibuttons_old);
		PrintName(19,9,Player1_Name,true,b);

		
		if ((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) {
		   PrintName(19,9,Player1_Name,false,b);
		   if (PlayerCNT == 2) {		     
		     set_PM_mode(PM_Enter_Player2_Name);
		   } else {
             Print(2,12,PSTR("NAME OF PLAYER2:"));			 
			 Player2_Name[0] = 'C';
			 Player2_Name[1] = 'P';
			 Player2_Name[2] = 'U';
			 Player2_Name[3] = 0;
			 PrintName(19,12,Player2_Name,false,0);			 	
			 set_PM_mode(PM_Enter_Point_Count);
		   }
        } 		
        break;


	  // proceed input of player 2 name	
	  case PM_Enter_Player2_Name:
				
		edit_name(&Player2_Name[0],&b,&ibuttons,&ibuttons_old);
		PrintName(19,12,Player2_Name,true,b);

		
		if ((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) {
		   PrintName(19,12,Player2_Name,false,0); 
		   set_PM_mode(PM_Enter_Point_Count);
        } 		
        break;


	  // proceed input total point count	
	  case PM_Enter_Point_Count:	
		
		if (edit_value(&Total_score_points,1,9,26,15,&ibuttons,&ibuttons_old)) {
					   
		   save_def_EEPROM();

		   set_PM_mode(PM_Show_Gorilla_Dance);		   
        } 		
        break;


	  // gorilla dance intro	
      case PM_Show_Gorilla_Dance:
  	  	animate_2gorillas_dance();
		if (((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) || 
		   ((BTN_B & ibuttons) && !(ibuttons_old & BTN_B)) ||
		   ((BTN_START & ibuttons) && !(ibuttons_old & BTN_START)) ||
		   ((BTN_SELECT & ibuttons) && !(ibuttons_old & BTN_SELECT))) {
		   set_PM_mode(PM_Prepare_Game);	
		}
        break;


	  // proceed input of angle for player 1	
	  case PM_Enter_Angle_P1:
	    //t = 0;			
		if (edit_value(&ThrowAngle,0,90,9,2,&ibuttons,&ibuttons_old)) {
		   set_PM_mode(PM_Enter_Power_P1);
        } 		
        break;


	  // proceed input of power for player 1	
	  case PM_Enter_Power_P1:			
		if (edit_value(&ThrowPower,1,50,9,3,&ibuttons,&ibuttons_old)) {
		   set_PM_mode(PM_THROW_P1);
        } 		
        break;


	  // proceed input of angle for player 2
	  case PM_Enter_Angle_P2:	    	
	    //t = 0;		
		if (edit_value(&ThrowAngle,0,90,28,2,&ibuttons,&ibuttons_old)) {
		   set_PM_mode(PM_Enter_Power_P2);
        } 		
        break;


	  // proceed input of power for player 2	
	  case PM_Enter_Power_P2:			
		if (edit_value(&ThrowPower,1,50,28,3,&ibuttons,&ibuttons_old)) {
		   set_PM_mode(PM_THROW_P2);
        } 		
        break;

	  // proceed player 1 throw
  	  case PM_THROW_P1:
 		
        	
		// WaitVsync(20);
		// calcuate new banana position
		calculate_position(t);
		t = t + 0.2;
		
		// to wide throw?
		if (((PosX + 8) > 240) || (PosX < 0)) {
		    // hide banana sprite
		    MapSprite2(1,banana_empty,0);	
			// change player
			set_PM_mode(PM_Enter_Angle_P2);
			break;
		}				 


		// check collision
		if (PosY < 224) {
		  	b = checkcollision(PosX,224 - PosY);		
		  	if (b == hit_skyscraper) {		    
		  		set_PM_mode(PM_Crash_Skyscraper_P1);
				break;

			} else if (b == hit_gorilla) {
				if (PosX > 120) set_PM_mode(PM_Crash_Gorilla_P2);
				else set_PM_mode(PM_Crash_Gorilla_P1);
				break;

			} else if (b == hit_sun) {
				// banana hits the sun --> draw surprised sun
				DrawMap2(13,0,sun_surprised);
			}
		}


		// draw animated banana
		if (PosY < 224) {
		  SetSpriteVisibility(true);
		  animate_banana(PosX, 224 - PosY);
		} else SetSpriteVisibility(false);

	    break;	


	  // proceed player 2 throw
  	  case PM_THROW_P2:

		// calcuate new banana position
		calculate_position(t);
		t = t + 0.2;
		
		// to wide throw?
		if (((PosX + 8) > 240) || (PosX < 0))  {
		    AI_calculation();
		    // hide banana sprite
		    MapSprite2(1,banana_empty,0);	
		  	// change player
  	  		set_PM_mode(PM_Enter_Angle_P1);
			break;
		}				 


		// check collision
		if (PosY < 224) {
			b = checkcollision(240 - PosX,224 - PosY);		
			if (b == hit_skyscraper) {	
				AI_calculation();	    
				set_PM_mode(PM_Crash_Skyscraper_P2);
				break;

			} else if (b == hit_gorilla) {
				if (PosX < 120) set_PM_mode(PM_Crash_Gorilla_P2);
				else set_PM_mode(PM_Crash_Gorilla_P1);
				break;

			} else if (b == hit_sun) {
				// banana hits the sun --> draw surprised sun
				DrawMap2(13,0,sun_surprised);
			}
		}

		// draw animated banana
		if (PosY < 224) {
		  SetSpriteVisibility(true);
		  animate_banana(240 - PosX, 224 - PosY);
		} else SetSpriteVisibility(false);

	    break;	
	    break;	


	  case PM_Crash_Gorilla_P1:
	  case PM_Crash_Gorilla_P2:
	    animate_banana(PosX, PosY);
		break;

	  // banana of player 1 hits skyscraper	
	  case PM_Crash_Skyscraper_P1:
		animate_banana(PosX - 4, 224 - PosY - 4);
		break;

	  // banana of player 2 hits skyscraper	
	  case PM_Crash_Skyscraper_P2:
		animate_banana(240 - PosX - 4, 224 - PosY - 4);
		break;

    }	

	//ani_count++;	// increase counter for animation
  }
  

} 


void set_PM_mode(u8 mode) {
// set parameters, tiles, background etc for choosed program mode
	double winkel;		
	switch (mode)
	{

	  case	PM_Intro:
   		 
	    // cursor is invisible now
	    StopSong();
		ClearVram();


        Print(4,3,PSTR("U Z E  G O R I L L A S"));

		Print(9,5,PSTR("VERSION 1.0"));
        Print(4,8,PSTR("(C) HARTMUT WENDT 2011"));
        Print(4,9,PSTR("SOUNDTRACK BY C. KUNZ"));
        Print(4,10,PSTR("WWW.HWHARDSOFT.DE.VU"));  

        Print(4,13,PSTR("YOUR MISSION IS TO HIT")); 
        Print(4,14,PSTR("YOUR OPPONENT WITH THE")); 
        Print(4,15,PSTR("EXPLODING BANANA BY")); 
        Print(4,16,PSTR("VARYING THE ANGLE AND")); 
        Print(4,17,PSTR("POWER OF YOUR THROW."));

        Print(7,20,PSTR("LICENCED UNDER"));
        Print(9,21,PSTR("GNU GPL V3"));
  
        Print(8,26,PSTR("PRESS ANY KEY"));
 
  		//start sound track
		SetMasterVolume(130);
		WaitVsync(10);
		StartSong(introtheme);		

		break;
	  
	  case	PM_Enter_Player_Count:
   		// blank screen - no music
	    StopSong();
		ClearVram();
	
		WaitVsync(2);
		// Draw help
		Print(1,26,PSTR("CONFIRM INPUT WITH BUTTON A"));

		Print(2,6,PSTR("COUNT OF PLAYERS:"));
			
		load_def_EEPROM();
		PLAYER=0; 
		StartPlayer = 0;
		break;

	  case	PM_Enter_Player1_Name:
		Print(2,9,PSTR("NAME OF PLAYER1:"));
		break;

	  case	PM_Enter_Player2_Name:
		Print(2,12,PSTR("NAME OF PLAYER2:"));
		break;

	  case	PM_Enter_Point_Count:
		Print(2,15,PSTR("HOW MANY TOTAL POINTS:"));	
		ScoreP1 = 0;
		ScoreP2 = 0;
		break;
	  
	  case	PM_Show_Gorilla_Dance:
   		// blank screen - no music	    
		ClearVram();
		
		WaitVsync(30);
		Print(3,4,PSTR("U Z E   G O R I L L A S"));
		
		Print(10,7,PSTR("STARRING:"));
		PrintName(4,10,Player1_Name,false,0); 
		Print(13,10,PSTR("AND"));
		PrintName(18,10,Player2_Name,false,0); 

		Print(8,26,PSTR("PRESS ANY KEY"));
		
		// draw gorillas
		DrawMap2(8,15,gorilla_map);
		DrawMap2(18,15,gorilla_map);
		ani_count = 1;

		//start sound track
		//SetMasterVolume(130);
		StartSong(maintheme);	
		
		break;

	  case	PM_Prepare_Game:	    
    	
        // clear screen & stop music
		StopSong();
	    ClearVram();		

		// generate playground
        create_new_skyline();

		// calculate Windspeed (values -5...5)
		Windspeed = PRNG_NEXT() % 11;
		Windspeed -= 5;
		Windspeed *= 2;
		draw_wind_arrow(); 		

		// calculate start values for AI in 1 player mode
		CPU_Speed = 20 + (PRNG_NEXT() % 11);
		if ((Ypos_left_skyscraper < Ypos_gorilla2) && ((Ypos_gorilla2 - Ypos_left_skyscraper) > 5)) CPU_Angle = 45 + (PRNG_NEXT() % 11);
		else CPU_Angle = 30 + (PRNG_NEXT() % 21);

		// show actual score
		show_score();

		if (StartPlayer) {
			// Player 2 will start
			mode = PM_Enter_Angle_P2;
			StartPlayer = 0;		

		} else {
			// Player 1 will start
			mode = PM_Enter_Angle_P1;		
			StartPlayer = 1;
		}		
        		
	
	  case	PM_Enter_Angle_P1:
	  case	PM_Enter_Angle_P2:
	  	// draw sun
		DrawMap2(13,0,sun_laughing);

		// draw gorillas
		DrawMap2(3,Ypos_gorilla1,gorilla_map);
		DrawMap2(24,Ypos_gorilla2,gorilla_map);

		// print player names
		PrintName(1,1,Player1_Name,false,0); 
		PrintName(20,1,Player2_Name,false,0); 

		if (mode == PM_Enter_Angle_P1) {
		   Print(1,2,PSTR("ANGLE:"));
		   PLAYER=0;  	
		} else {
		   Print(20,2,PSTR("ANGLE:"));
		   if (Controller_status2 == 0x04) PLAYER=1;  	
		}

		// AI??
		if ((PlayerCNT == 1) && (mode == PM_Enter_Angle_P2)) {
		   Print(20,3,PSTR("POWER:"));
		   PrintByte(28,2,CPU_Angle,false);
		   WaitVsync(60);
		   PrintByte(28,3,CPU_Speed,false);
		   ThrowAngle = CPU_Angle;
		   ThrowPower = CPU_Speed;	
		   WaitVsync(90);
		   mode = PM_THROW_P2;	
		   goto AI_THROW;
		   	
		} else {
			// reset Angle
			ThrowAngle = 0;
		}
    	break;

	  case	PM_Enter_Power_P1:

		Print(1,3,PSTR("POWER:"));	
		// reset power 
		ThrowPower = 0;
	    break;

	  case	PM_Enter_Power_P2:
		Print(20,3,PSTR("POWER:"));	
		// reset power 
		ThrowPower = 0;
	    break;

	  case	PM_THROW_P1:
	  case	PM_THROW_P2:
		
		AI_THROW:
		// delete text over the skyline
		clear_text();

		// move arm up
		if (mode == PM_THROW_P1) {
		  DrawMap2(3,Ypos_gorilla1,arm_up_left);
		} else {  
		  DrawMap2(26,Ypos_gorilla2,arm_up_right);
		}	
		
		// calculate Windforce;		
		if (mode == PM_THROW_P1) Windforce = Windspeed;
		else Windforce = Windspeed * (-1);

		
		// calculate start position for throw
		t = 0;
		if (mode == PM_THROW_P1) {
		  StartXPos = 24;
		  StartYPos = 230 - (Ypos_gorilla1 * 8);
		} else {  
		  StartXPos = 34;
		  StartYPos = 230 - (Ypos_gorilla2 * 8);
		}		

		// calculate vector Y
		memcpy_P( &winkel, &sincosLUT[ThrowAngle], sizeof( double ) );
		InitYVel = winkel * ThrowPower * 0.5; 

		// calculate vector X
		memcpy_P( &winkel, &sincosLUT[90-ThrowAngle], sizeof( double ) );
        InitXVel = winkel * ThrowPower * 0.5;

		//Sound
		TriggerNote(3,1,11,0x7f);
		
		break;

	  case  PM_Crash_Gorilla_P1:
	  	PosX = 24;		
		PosY = Ypos_gorilla1 * 8;
	    ani_count = 0;
		ScoreP2++;

		// Sound
	    TriggerNote(3,2,11,0x7f);	
		break;

	  case  PM_Crash_Gorilla_P2:
	  	PosX = 192;
		PosY = Ypos_gorilla2 * 8;
	    ani_count = 0;
		ScoreP1++;
		
		// Sound
	    TriggerNote(3,2,11,0x7f);	
		break;
	  
	  case  PM_Crash_Skyscraper_P1:
	  case  PM_Crash_Skyscraper_P2:
	    ani_count = 0;

		// Sound
	    TriggerNote(3,2,11,0x7f);	
		break;

      case	PM_Game_over:

		ClearVram();

        Print(10,9,PSTR("GAME OVER!"));
		Print(12,12,PSTR("SCORE:"));

		// print scores
		PrintByte(20,14,ScoreP1,false);
		PrintByte(20,15,ScoreP2,false);

		// print player names
		PrintName(10,14,Player1_Name,false,0); 
		PrintName(10,15,Player2_Name,false,0); 

		  
        Print(8,26,PSTR("PRESS ANY KEY"));	
		PLAYER=0;  	
		
	}
	program_mode = mode;

}



void create_new_skyline(void) {
// creates a new randomized city skyline
  u8 u1,u2;  

  for(u1 = 0; u1 < 10; u1++) {
  
    // skyscraper
    u2 = PRNG_NEXT() % 14;
	// Save heigth of 2nd and 9th skyscraper for gorillas
	if (u1 == 1) Ypos_gorilla1 = 19 - u2;
	else if (u1 == 7) Ypos_left_skyscraper = 19 - u2; // heigth of left skyscraper for AI
	else if (u1 == 8) Ypos_gorilla2 = 19 - u2;
	draw_skyscraper(u1, u2);
  }

}


void draw_skyscraper(u8 position, u8 floors) {
// draw a skyscraper at defined position
  u8 u1;
  for(u1 = 1; u1 < (floors + 6); u1++) {
    DrawMap2(position * 3, 27 - u1, get_skyscraper_map(PRNG_NEXT() % 4, position % 3));
    
  }
  
}



const char * get_skyscraper_map(u8 floor, u8 color) {
// get a map for a skyscraper
  if (color == red) {
    switch(floor) {
	  case 1:
		return(house_red2);
	  	break;

	  case 2:
		return(house_red3);   
	  	break;

	  case 3:
		return(house_red4);   
	  	break;
	  
	  default:
		return(house_red1);
	    break;	 

	}
  } else if (color == cyan) {
    switch(floor) {
	  case 1:
		return(house_cyan2);
	  	break;

	  case 2:
		return(house_cyan3);   
	  	break;

	  case 3:
		return(house_cyan4);   
	  	break;
	  
	  default:
		return(house_cyan1);
	    break;	 

	}

  } else {
    switch(floor) {
	  case 1:
		return(house_grey2);
	  	break;

	  case 2:
		return(house_grey3);   
	  	break;

	  case 3:
		return(house_grey4);   
	  	break;
	  
	  default:
		return(house_grey1);
	    break;	 

	}

  }
}


void calculate_position(double dt) {
// calculate ballistic trajectory
// this is the calculation from the original game:
//	 x# = StartXPos + (InitXVel# * t#) + (.5 * (Wind / 5) * t# ^ 2)
//   y# = StartYPos + ((-1 * (InitYVel# * t#)) + (.5 * gravity# * t# ^ 2)) * (ScrHeight / 350)
  
//  PosX = StartXPos + (0.1 * InitXVel * dt) + (0.05 * (Windspeed / 5) * dt * dt);
  PosX = StartXPos + (InitXVel * dt) + (0.1 * (Windforce / 5) * dt * dt);

  PosY = StartYPos + ((InitYVel * dt) + ((-0.05 * gravity * dt * dt))); 
			
}



void clear_text(void) {
// clear all text in upper screen
   Fill(1,1,9,3,0);	  
   Fill(20,1,9,3,0);	  	
}


u8 GetTile(u8 x, u8 y)
// get background tile from vram
{

 return (vram[(y * 30) + x] - RAM_TILES_COUNT);

}



// get the tile from the actual sprite
char get_sprite_tile(unsigned char x,unsigned char y) {
  if (x < 8) {
    if (y < 8) return(sprites[1].tileIndex);
    else return(sprites[3].tileIndex);

  } else {
    if (y < 8) return(sprites[2].tileIndex);
    else return(sprites[4].tileIndex);  
  
  }  
}


// get pixel value from a tile
char get_tile_pixel(unsigned char x,unsigned char y,char tile, const char *tileset) {
  
  return(pgm_read_byte(&(tileset[(64 * tile) + (8 * y) + x])));
}





// check the collision of the banana with the sun, skyscrapers or other gorilla
char checkcollision(int xx, int yy) {
// returns 0 - no collision
// returns 1 - gorilla
// returns 2 - skyscraper
// returns 4 - hit sun

u8 x,y;
unsigned char pix_sprite, pix_bg, tile_bg;
  
  // check collision
  for(y = 0; y < 8; y=y + 1)
  for(x = 0; x < 8; x=x + 1)
  {
    
	// banana hit the sun
	if (((xx + x)>= 104) && ((xx + x)< 136) && ((yy + y) < 32)) return(hit_sun);

    // check banana collision
    tile_bg = GetTile((xx + x) / 8, (yy + y) / 8);
	// enable this output for debugging 
	// PrintByte(5,24,tile_bg,1);
	if (tile_bg)
	{
	  pix_bg = get_tile_pixel((xx + x) % 8,(yy + y) % 8,tile_bg,BGTiles);      
	  pix_sprite = get_tile_pixel(x ,y ,sprites[1].tileIndex,spriteTiles);	  
 	/* enable this output for debugging 
	PrintByte(5,25,pix_bg,1);
	PrintByte(5,26,pix_sprite,1);
    PrintByte(5,27,sprites[1].tileIndex,1);
    */  
	  // crash?
	  if ((pix_sprite != 0xFE) && (pix_bg != 0x40)) {
	  	// banana hits gorilla
	    if ((pix_bg == 0x27) || !(pix_bg)) return(hit_gorilla);
		
		// banana hits skyscraper
	    return(hit_skyscraper); 
      }
	}
  }
  return(hit_nothing);
}



// damage a skyscraper after a collision with a banana
void destroy_skyscraper(int xx, int yy) {

//unsigned char tile_bg;
	// check left upper edge of banana tile	
	if (GetTile(xx / 8, yy / 8)) SetTile(xx / 8,yy / 8,0);

	// check right upper edge of banana tile	
	if (GetTile((xx + 8) / 8, yy / 8)) SetTile((xx + 8) / 8,yy / 8,0);

	// check left lower edge of banana tile	
	if (GetTile(xx / 8, (yy + 8) / 8)) SetTile(xx / 8,(yy + 8) / 8,0);

	// check right lower edge of banana tile	
	if (GetTile((xx + 8) / 8, (yy + 8) / 8)) SetTile((xx + 8) / 8,(yy + 8) / 8,0);

}




/**** A N I M A T I O N S ***************************************************************/

void animate_banana(u8 xx, int yy) {
// draw and animate the banana
  
  if ((program_mode == PM_Crash_Skyscraper_P1) || (program_mode == PM_Crash_Skyscraper_P2)) {
    
	// banana explosion after skyscraper hit
    switch (ani_count)
	{
		case 0: // 1st picture for explosion
		  //TriggerFx(3, 0xff, true);
        case 12: 
		  MapSprite2(1,Exp_small_0,0);	
		  break;

		case 1: // 2nd picture for explosion
		case 11:
		  MapSprite2(1,Exp_small_1,0);	
		  break;
		
		case 2: // 3rd picture for explosion
		  destroy_skyscraper(xx + 4,yy + 4); 	
        case 10:
		  MapSprite2(1,Exp_small_2,0);	
		  break;

		case 3: // 4th picture for explosion
		case 9:
		  MapSprite2(1,Exp_small_3,0);	
		  break;

		case 4: // 5th picture for explosion
		case 8:
		  MapSprite2(1,Exp_small_4,0);	
		  break;

		case 5: // 6th picture for explosion
		case 7:
		  MapSprite2(1,Exp_small_5,0);	
		  break;

		case 6: // 7th picture for explosion
		  MapSprite2(1,Exp_small_6,0);	
		  break;

		case 13: // 14th picture for explosion
		  MapSprite2(1,Exp_small_empty,0);	
		  if (program_mode == PM_Crash_Skyscraper_P1) set_PM_mode(PM_Enter_Angle_P2);	
		  else set_PM_mode(PM_Enter_Angle_P1);
		  break;
    }
	WaitVsync(3);
	MoveSprite(1,xx,yy,2,2);

  } else if ((program_mode == PM_Crash_Gorilla_P1) || (program_mode == PM_Crash_Gorilla_P2))  {
  
	// banana explosion after gorilla hit
    switch (ani_count)
	{
		case 0: // 1st picture for explosion
		case 14:
		  //TriggerFx(3, 0xff, true);
		  MapSprite2(1,Exp_big_0,0);	
		  break;

		case 1: // 2nd picture for explosion
		case 13:
		  MapSprite2(1,Exp_big_1,0);	
		  break;
		
		case 2: // 3rd picture for explosion
		case 12:
		  MapSprite2(1,Exp_big_2,0);	
		  break;
		
		case 3: // 4th picture for explosion
		case 11:
		  MapSprite2(1,Exp_big_3,0);	
		  break;

		case 4: // 5th picture for explosion
		case 10:
		  MapSprite2(1,Exp_big_4,0);	
		  break;
		
		case 5: // 6th picture for explosion
		case 9:
		  MapSprite2(1,Exp_big_5,0);	
		  break;

		case 6: // 7th picture for explosion
		case 8:
		  MapSprite2(1,Exp_big_6,0);	
		  break;

		case 7: // 8th picture for explosion
		  MapSprite2(1,Exp_big_7,0);	
		  // delete gorilla 
		  if(PosX < 120) Fill(3,Ypos_gorilla1,3,3,0);
		  else Fill(24,Ypos_gorilla2,3,3,0);
		  break;

		case 15: // end of animation
		  // delete last picture of explosion
		  MapSprite2(1,Exp_big_empty,0);			  

		  // gorilla victory dance 	
		  if(PosX > 120) animate_gorilla_victory(3,Ypos_gorilla1);
		  else animate_gorilla_victory(24,Ypos_gorilla2);
		  
		  
		  if ((ScoreP1 < Total_score_points) && (ScoreP2 < Total_score_points)) set_PM_mode(PM_Prepare_Game);
		  else set_PM_mode(PM_Game_over);
		  break;


    }
	WaitVsync(3);
	MoveSprite(1,xx,yy,3,3);
  } else { 
    // banana flight
 	switch (ani_count % 8)
	{
		case 0:	//1st banana picture 0 degree
		  MapSprite2(1,banana_0,0);
		  break;	

		case 1:	//2nd banana picture 45 degree
		  MapSprite2(1,banana_6,0);
		  break;	

		case 2:	//3rd banana picture 90 degree
		  MapSprite2(1,banana_1,0);
		  break;	

		case 3:	//4th banana picture 135 degree
		  MapSprite2(1,banana_7,0);
		  break;	
		
		case 4:	//5th banana picture 0 degree
		  MapSprite2(1,banana_2,0);
		  break;	

		case 5:	//6th banana picture 45 degree
		  MapSprite2(1,banana_5,0);
		  break;	

		case 6:	//7th banana picture 90 degree
		  MapSprite2(1,banana_3,0);
		  break;	

		case 7:	//8th banana picture 135 degree
		  MapSprite2(1,banana_4,0);
		  break;	
	}
	MoveSprite(1,xx,yy,1,1);	

  }
  
	
  ani_count++;
  if (ani_count >= 200) ani_count = 0;
 
}


void animate_star_frame(void) {
// draw and animate the frame of red stars in intro screen
  switch (ani_count) {
		case 0:	//1st step
			// top row
			Print(1,1,PSTR("  *  *  *  *  *  *  *  *  * "));

			// bottom row
			Print(1,23,PSTR("*  *  *  *  *  *  *  *  *  *"));

			// left col
			DrawMap2(1,2,STAR_ANIMATED_V1);

            // right col
			DrawMap2(28,2,STAR_ANIMATED_V3);				
			break;	

		case 1:	//2nd step			
			// top row
	        Print(1,1,PSTR(" *  *  *  *  *  *  *  *  *  "));
			

			// bottom row
			Print(1,23,PSTR(" *  *  *  *  *  *  *  *  *  "));

			// left col
			DrawMap2(1,2,STAR_ANIMATED_V2);

            // right col
			DrawMap2(28,2,STAR_ANIMATED_V2);				
			break;	

        default: //3rd step
			// top row
		    Print(1,1,PSTR("*  *  *  *  *  *  *  *  *  *"));

			// bottom row
			Print(1,23,PSTR("  *  *  *  *  *  *  *  *  * "));

			// left col
			DrawMap2(1,2,STAR_ANIMATED_V3);

            // right col
			DrawMap2(28,2,STAR_ANIMATED_V1);				
			break;
  }

  ani_count++;
  if (ani_count >= 3) ani_count = 0; 
}



void animate_2gorillas_dance(void) {
// animate the 2 gorillas in intro	

  switch (ani_count) {
		case 5:	//1st step
		case 35:	//3st step
		case 50:	//5th step			
		case 60:	//7th step			
		case 66:	//9th step			
		case 72:	//11th step			
		case 78:	//13th step			
		case 84:	//15th step			
		case 90:	//17th step			
  			// draw gorillas
  			DrawMap2(8,15,gorilla_map);
  			DrawMap2(18,15,gorilla_map);
      		// draw arms up
			DrawMap2(8,15,arm_up_left);
			DrawMap2(20,15,arm_up_right);						
			break;	

		case 20:	//2nd step			
		case 45:	//4th step			
		case 55:	//6th step			
		case 63:	//8th step			
		case 69:	//10th step			
		case 75:	//12th step			
		case 81:	//14th step			
		case 87:	//16th step			
		case 93:	//18th step			
  			// draw gorillas
  			DrawMap2(8,15,gorilla_map);
  			DrawMap2(18,15,gorilla_map);
      		// draw arms up
			DrawMap2(10,15,arm_up_right);
			DrawMap2(18,15,arm_up_left);		
			break;	

        case 100: //last step  			
			set_PM_mode(PM_Prepare_Game);		          
			break;
		

  }  
  ani_count++;
  WaitVsync(6);
  //PrintByte(1,27,ani_count,true);
}

void animate_gorilla_victory(u8 xx, u8 yy) {
// animate the gorillas victory dance
u8 u1;
  TriggerNote(3,3,11,0x9f);	
  for(u1 = 1; u1 < 7; u1++) {
    //Play Gorilla Cry
	//TriggerNote(3,1,11,0x7f);	
	// draw gorillas
  	DrawMap2(xx,yy,gorilla_map);
  	// draw left arm up
	DrawMap2(xx,yy,arm_up_left);
	WaitVsync(20);
    //Play Gorilla Cry
	//TriggerNote(3,1,11,0x7f);	
	// draw gorillas
  	DrawMap2(xx,yy,gorilla_map);
  	// draw right arm up
	DrawMap2(xx + 2,yy,arm_up_right);
	WaitVsync(20);
  }
}



/**** S T U F F ********************************************************************/

bool edit_value(u8 *value, u8 min, u8 max, u8 Xpos, u8 Ypos, int *b_mode, int *b_mode_old) {
// edit a numeric value with SNES controller
// UP - increase value
// DWN - decrease value
// LEFT - value - 10
// RIGHT - value + 10

  // Auto repeat
  if (*b_mode != *b_mode_old) autorepeat_cnt = 0;
  else {
    autorepeat_cnt++;;
    if (autorepeat_cnt > 20) {
		autorepeat_cnt = 10;
		*b_mode_old = 0;
	}
  }
  
  // proceed up & down button
  if ((*b_mode & BTN_UP) && !(*b_mode_old & BTN_UP)) {
     if (*value < max) (*value)++;
  }
  else if ((*b_mode & BTN_DOWN) && !(*b_mode_old & BTN_DOWN)) {		 
     if (*value > min) (*value)--;      
  }
  // proceed left & right button
  else if ((*b_mode & BTN_RIGHT) && !(*b_mode_old & BTN_RIGHT)) {		 
     if ((max - *value) >= 10) *value += 10;      
  }
  else if ((*b_mode & BTN_LEFT) && !(*b_mode_old & BTN_LEFT)) {		 
     if ((*value - min) >= 10) *value -= 10;      
  } 
  // proceed button A
  else if ((*b_mode & BTN_A) && !(*b_mode_old & BTN_A)) {
    PrintByte(Xpos,Ypos,*value,false);	
    return(true);
  }
  ani_count++;
  if (ani_count >= 10) ani_count = 0;  
  if (ani_count > 5) PrintByte(Xpos,Ypos,*value,false);
  else Print(Xpos - 2,Ypos,PSTR("   "));
  return(false);
}


void edit_name(char *value, u8 *entry, int *b_mode, int *b_mode_old) {
// edit a name (string) with SNES controller
// UP - increase char
// DWN - decrease char
// LEFT - edit prev char
// RIGHT - edit succ char
   
  // proceed left & right button
  if ((*b_mode & BTN_RIGHT) && !(*b_mode_old & BTN_RIGHT)) {		 
     if (*entry < 8) (*entry)++;      
  }
  else if ((*b_mode & BTN_LEFT) && !(*b_mode_old & BTN_LEFT)) {		 
     if (*entry) (*entry)--;      
  } 

  // Auto repeat
  if ((ani_count == 5) && (*b_mode == *b_mode_old) &&
  	 ((*b_mode & BTN_UP) || (*b_mode & BTN_DOWN))) *b_mode_old = 0;
  
  u8 c = *(value + (*entry)) - BGTILES_SIZE + 0x34;
  // proceed up & down button
  if ((*b_mode & BTN_UP) && !(*b_mode_old & BTN_UP)) {
     c++;
     if (c > 'Z') c = ' '; 
     else if (c == '!') c = '0';
     else if (c == ':') c = 'A';
  }
  else if ((*b_mode & BTN_DOWN) && !(*b_mode_old & BTN_DOWN)) {			 
     c--;      
     if (c < ' ') c = 'Z';
	 else if (c == '@') c = '9';
	 else if (c == '/') c = ' ';
  }

  *(value + (*entry)) = c + BGTILES_SIZE - 0x34 ;
}



void PrintName(int x, int y, char *s, bool Bcursor, u8 CurPos) {
// print text in game screens
	u8 c;
	u8 i;
	for(i=0;i < 8;i++) {	  
      c = *(s + i) - BGTILES_SIZE + 0x34; 
	  if (c) {
	    if ((Bcursor) && (CurPos == i)) {
		  if (ani_count > 5) PrintChar(x + i,y,c);
		  else PrintChar(x + i,y,'^');	
	    } else {
		  PrintChar(x + i,y,c);	
		} 
      } else break;
	}  
	ani_count++;
    if (ani_count >= 10) ani_count = 0; 
}



void msg_window(u8 x1, u8 y1, u8 x2, u8 y2) {
// draw a window with frame and black backgound on the screen

    // window backgound
	Fill(x1 + 1, y1 + 1, x2 - x1 - 1, y2 - y1 - 1,401);
	// upper frame
	Fill(x1 + 1, y1, x2 - x1 - 1, 1,411);
	// lower frame
	Fill(x1 + 1, y2, x2 - x1 - 1, 1,410);
	// left frame
	Fill(x1 , y1 + 1, 1, y2 - y1 - 1,409);
	// right frame
	Fill(x2, y1 + 1, 1 , y2 - y1 - 1,409);
	// upper, left corner
	SetTile(x1,y1,405);
	// upper, right corner
	SetTile(x2,y1,406);
	// lower, left corner
	SetTile(x1,y2,407);
	// lower, right corner
	SetTile(x2,y2,408);
}	



void show_score(void) {
// draw the score for both players on the screen
  	
	PrintChar(10,25,'0' + ScoreP1);	
	Print(11,25,PSTR(">SCORE<"));
	PrintChar(18,25,'0' + ScoreP2);	
}


void draw_wind_arrow(void) {
// draw the arrow for windspeed direction and intensity in bottom line
    
	switch(Windspeed) {
	
	case -10:
	case -8:
		Print(12,27,PSTR("_aaaaa"));
		break;		

	case -6:
	case -4:
		Print(13,27,PSTR("_aaa"));
		break;		

	case -2:
		Print(14,27,PSTR("_a"));
		break;		

	case 2:
		Print(14,27,PSTR("a`"));
		break;		

	case 6:
	case 4:
		Print(13,27,PSTR("aaa`"));
		break;		

	case 10:
	case 8:
		Print(12,27,PSTR("aaaaa`"));
		break;		

	}

}


void AI_calculation(void) {
// calculate the throw parameters with AI
   if (PlayerCNT != 1) return;
	
   if ((PosX < 0) && (CPU_Angle > 10)) CPU_Angle -= 2;
   else if ((PosX < 80) && (CPU_Angle < 80))  CPU_Angle += 4;
   else if ((PosX < 140) && (CPU_Angle < 80)) CPU_Angle += 2;

   if ((PosX > 180) && (CPU_Speed > 10)) CPU_Speed -= 2;
   else if ((PosX <= 100) && (CPU_Speed < 46)) CPU_Speed += 4;
   else if ((PosX <= 180) && (CPU_Speed < 48)) CPU_Speed += 2;
}



void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ)
// copy ucANZ Bytes from buffer BUFA to buffer BUFB
{
 for(;ucANZ>0 ; ucANZ--)
 {
  *(BUFB++) = *(BUFA++);
 }   
}


u8 set_def_EEPROM(void) {
// init the EEPROM block with default values
  // Player1 name
  ebs.data[0] = 'P';
  ebs.data[1] = 'L';
  ebs.data[2] = 'A';
  ebs.data[3] = 'Y';
  ebs.data[4] = 'E';
  ebs.data[5] = 'R';
  ebs.data[6] = '1';
  ebs.data[7] = ' ';
  ebs.data[8] = 0x00;
  ebs.data[9] = 0x55;
  // Player2 name
  ebs.data[10] = 'P';
  ebs.data[11] = 'L';
  ebs.data[12] = 'A';
  ebs.data[13] = 'Y';
  ebs.data[14] = 'E';
  ebs.data[15] = 'R';
  ebs.data[16] = '2';
  ebs.data[17] = ' ';
  ebs.data[18] = 0x00;
  // Player count
  ebs.data[19] = 1;
  // round count
  ebs.data[20] = 3;
  return(EepromWriteBlock(&ebs));
}


void load_def_EEPROM(void) {
// load the default values for variables from EEPROM
	// read the eeprom block
    if (!isEepromFormatted() || EepromReadBlock(22, &ebs)) return;
	// check indicator byte
	if (ebs.data[9] != 0x55) return;

	copy_buf(&ebs.data[0],&Player1_Name[0],9);
	copy_buf(&ebs.data[10],&Player2_Name[0],9);
	PlayerCNT = ebs.data[19];
	Total_score_points = ebs.data[20];
}


void save_def_EEPROM(void) {
// write the actual values for variables from EEPROM
	// check eeprom block    
	copy_buf(&Player1_Name[0],&ebs.data[0],9);
	if (PlayerCNT > 1) copy_buf(&Player2_Name[0],&ebs.data[10],9);
	ebs.data[19] = PlayerCNT; 
	ebs.data[20] = Total_score_points;
	ebs.data[9] = 0x55;
	EepromWriteBlock(&ebs);
}







