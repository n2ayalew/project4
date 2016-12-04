#include <RTL.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include "LPC17xx.H"                    /* LPC17xx definitions               */
#include "GLCD.h"
#include "LED.h"
#include "KBD.h"
#include "ADC.h"
#include "objects.h"
#include "GLCD_Scroll.h"
#include "ai8.bmp_out.c"
#include "ball6.bmp_out.c"
#include "player8.bmp_out.c"
#include "bg_height.bmp_out.c"
#include "bg_wide.bmp_out.c"
#include "bg_ball.bmp_out.c"
#include "court_net6.bmp_out.c"

#define __FI        1                   /* Font index 16x24                  */
//#define DEBUG_PRINT 


OS_TID t_led;                           /* assigned task id of task: led */
OS_TID t_adc;                           /* assigned task id of task: adc */
OS_TID t_kbd;                           /* assigned task id of task: keyread */
OS_TID t_jst   ;                        /* assigned task id of task: joystick */
OS_TID t_clock;                         /* assigned task id of task: clock   */
OS_TID t_lcd;                           /* assigned task id of task: lcd     */
OS_TID t_keyread;
OS_TID t_player;                        /* assigned task id of task: player     */
OS_TID t_opponent;                      /* assigned task id of task: opponent     */
OS_TID t_ball;                          /* assigned task id of task: ball    */

OS_MUT mut_GLCD;                        /* Mutex to controll GLCD access     */

// court vars
object court_net;
#define NUM_BARRIERS 4
#define LEFT_WALL    0
#define RIGHT_WALL   1
#define TOP_WALL     2
#define BOTTOM_WALL  3
object barriers[NUM_BARRIERS];

// player vars
object player;
object player_old_loc_x;
object player_old_loc_y;
short player_moved_x;
short player_served;
OS_MUT mut_player;

// oppoenent vars
object opponent;
object opponent_old_loc_x;
object opponent_old_loc_y;
OS_MUT mut_opponent;

// ball vars
object ball;
OS_MUT mut_ball;
object ball_old_loc;

score curr_score;
OS_MUT mut_curr_score;

// Game vars
const float g = 9.8;
const U16 update_interval = 8;
int slime_mass;
int ball_mass;
int ball_origin;

short game_started;
short collision_detected;
short collision_detected_opponent;
float coll_rad;

float ball_g = 9.8;
float a,b,c,sx,bx,sy,by;
float theta;
int ball_rad = 10;

int new_ball_vx;
int new_ball_y;
int new_ball_x;
short collision_detected_before = 0;
short reversed;
short draw_net;

int ai_left_limit;
int ai_right_limit;
int player_left_limit;
int player_right_limit;
int player_low_limit;
int player_high_limit;

// Peripherals
unsigned int ADCStat = 0;
unsigned int ADCValue = 0;
uint32_t INT0_val = 0;



/*
  Axis Aligned Bounding Box Collision Detection
*/
// if this looks ugly lets try checking for collision  by checking if the velocity vectors intercept
short detect_slime_collision(object ball, object slime){
  bx = ball.x  + 0.5 * ball.width;
  sx = slime.x + slime.height;
  by = ball.y + 0.5 * ball.height;
  sy = slime.y + slime.height;

  a = (bx - sx) * (bx - sx);
  b = (by - sy) * (by - sy);
  c = sqrt(a + b);

  if ( (unsigned int)c <= (0.5 * ball.height + slime.height) ) {
    return 1;
  }else {
    return 0;
  }
}

// Check if the given point (x, y) is in the given rectangle
bool rect_contains(int x, int y, object rect) {
	if(x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height) {
		return true;
	} else {
		return false;
	}
}

void init_collision(collision *col) {
	col->dx_modifier = 1;
	col->dy_modifier = 1;
	col->occured     = false;
	
}

// Detect if the ball is colliding with a given rectangle. Returns modifiers for dx and dy as a result of the collision
collision detect_rect_collision(object ball, object rect){
	collision col;
	bool col_top_left;
	bool col_top_right;
	bool col_bottom_left;
	bool col_bottom_right;
	
	// Initialize return value to "no change"
	init_collision(&col);
	
	// Figure out which corners of the ball are colliding
	col_top_left     = rect_contains(ball.x,              ball.y,               rect);
	col_top_right    = rect_contains(ball.x + ball.width, ball.y,               rect);
	col_bottom_left  = rect_contains(ball.x,              ball.y + ball.height, rect);
	col_bottom_right = rect_contains(ball.x + ball.width, ball.y + ball.height, rect);
	
	// Check for collision with ball going right
	if (ball.dx > 0 && ((col_top_right && !col_top_left) || (col_bottom_right && !col_bottom_left))) {
		col.dx_modifier = -1;
		col.occured = true;
	} 
	// Check for collision with ball going left
	else if (ball.dx < 0 && ((!col_top_right && col_top_left) || (!col_bottom_right && col_bottom_left))) {
		col.dx_modifier = -1;
		col.occured = true;
	}
	
	// Check for collision with ball going down
	if (ball.dy > 0 && ((col_bottom_right && !col_top_right) || (col_bottom_left && !col_top_left))) {
		col.dy_modifier = -1;
		col.occured = true;
	} 
	// Check for collision with ball going up
	else if (ball.dy < 0 && ((!col_bottom_right && col_top_right) || (!col_bottom_left && col_top_left))) {
		col.dy_modifier = -1;
		col.occured = true;
	}
	
  return col;
}

void score_player() {
	
	os_mut_wait(mut_curr_score, 0xffff);
	
	if(curr_score.player == 4) {
		// TODO: End game
	} else {
		curr_score.player++;
		LED_On(curr_score.player);
		// TODO: Reset screen
	}
	
	os_mut_release(mut_curr_score);
}

void score_opponent() {

	os_mut_wait(mut_curr_score, 0xffff);
	
	if(curr_score.opponent == 4) {
		// TODO: End game
	} else {
		curr_score.opponent++;
		LED_On(8 - curr_score.opponent);
		//TODO: Reset screen
	}
	
	os_mut_release(mut_curr_score);
}

/*----------------------------------------------------------------------------
  Task 2 'keyread': process key stroke from int0 push button
 *---------------------------------------------------------------------------*/
__task void keyread (void) {
  while (1) {                                 /* endless loop                */
    if (INT0_Get() == 0) {                    /* if key pressed              */
      LED_Toggle (7) ;												/* toggle eigth LED if pressed */
    }
    os_dly_wait (5);                          /* wait for timeout: 5 ticks   */
  }
}

/*----------------------------------------------------------------------------
  Task 3 'ADC': Read potentiometer
 *---------------------------------------------------------------------------*/
/*Value from potentiometer is used to define the pace of the game*/
__task void adc (void) {
  // 0 - 4095

  for (;;) {

    os_dly_wait (400);
  }
}

/*----------------------------------------------------------------------------
  Task 5 'lcd': LCD Control  ntask
 *---------------------------------------------------------------------------*/
__task void lcd (void) {

  for (;;) {
// 		os_mut_wait(mut_GLCD, 0xffff);
// 		
// 		os_mut_release(mut_GLCD);
    /*os_mut_wait(mut_GLCD, 0xffff);
    GLCD_SetBackColor(Blue);
    GLCD_SetTextColor(White);
    GLCD_DisplayString(0, 0, __FI, "      MTE 241        ");
    GLCD_DisplayString(1, 0, __FI, "      RTX            ");
    GLCD_DisplayString(2, 0, __FI, "  Project 4 Demo   ");
    os_mut_release(mut_GLCD);
    os_dly_wait (400);

    os_mut_wait(mut_GLCD, 0xffff);
    GLCD_SetBackColor(Blue);
    GLCD_SetTextColor(Red);
    GLCD_DisplayString(0, 0, __FI, "      MTE 241        ");
    GLCD_DisplayString(1, 0, __FI, "      Other text     ");
    GLCD_DisplayString(2, 0, __FI, "    More text        ");
    os_mut_release(mut_GLCD);
    os_dly_wait (400);*/
  }
}

__task void player_tsk (void) {
//   short player_jumped = 0;
//   os_itv_set (update_interval);
//   for (;;) {
//     os_itv_wait ();
//     os_mut_wait(mut_player, 0xffff);
//     KBD_val = KBD_Get();
//     player_moved_x = 0;
//     player_old_loc_x.x = player.x;
//     player_old_loc_x.y = player.y;
//     player_old_loc_y.x = player.x;
//     player_old_loc_y.y = player.y;

//      if (player.y >= player_high_limit && (player_jumped & 1) ){
//       player_jumped = 0;
//       player.y = player_high_limit;
//       player.dy = 0;

//     }

//     else if ( player.y < player_high_limit && (player_jumped) ) {      
//       player.t = player.t + player.dt; 
//       player.y =  0.5 * g * player.t * player.t - player.dy * player.t + player_high_limit;
//       if (player.y > player_high_limit){
//         player.y = player_high_limit;
//       }
//       player_jumped = 1;
//       if (player.y < player_old_loc_y.y) {
//         player_old_loc_y.y = player.y + player.height;
//       } 
//     }

//     else if ( ~KBD_val & KBD_UP){
//       player.t = 0;
//       player.dy = 30;
//       player.y = player_high_limit - 1;
//       player_jumped = 2;
//     }

//     if ( ~KBD_val & KBD_LEFT && (player.x > player_left_limit) ){
//       player_old_loc_x.x = player.x + (player.width - player_old_loc_x.width);
//       player.dx = -10;
//       player.x += player.dx;
//       if ( player.x <=  player_left_limit){
//         player.x = player_left_limit;
//       }
//       player_moved_x = 1;
//     }
//     else if ( (~KBD_val & KBD_RIGHT) && ( player.x < player_right_limit ) ){
//       if (player.x + player.dx >= player_right_limit) {
//         player.x = player_right_limit;
//       }
//       else {
//         player.dx = 10;
//         player.x += player.dx;
//       }
//       player_moved_x = 1;
//     }
//     else {
//       player.dx = 0;
//     }

//     if (player_jumped){
//       os_mut_wait(mut_GLCD, 0xffff);
//       write_obect_lcd(player_old_loc_y);
//       write_obect_lcd(player);
//       os_mut_release(mut_GLCD);
//     }
//     if (player_moved_x) {
//       os_mut_wait(mut_GLCD, 0xffff);
//       write_obect_lcd(player_old_loc_x);
//       write_obect_lcd(player);
//       os_mut_release(mut_GLCD);
//     }
//     os_mut_release(mut_player);
//   }


}

__task void ball_tsk (void) {
	int i;
	collision col, next_col;
	
// 	os_mut_wait(mut_GLCD, 0xffff);
// 	printf("Task started");
// 	os_mut_release(mut_GLCD);

  os_itv_set (update_interval);
  for (;;){
    os_itv_wait ();
    os_mut_wait(mut_ball, 0xffff);
    ball_old_loc.x = ball.x;
    ball_old_loc.y = ball.y;
    draw_net = 0;
	  init_collision(&col);
		
		// Check for ground collision
		next_col = detect_rect_collision(ball, barriers[BOTTOM_WALL]);
		if(next_col.occured) {
			
			if(ball.x + ball.width/2 < court_net.x + court_net.width/2) {
				score_player();
			} else {
				score_opponent();
			}
		}
		
    // Update position of ball
    ball.t   = ball.t + ball.dt;
    ball.y  += ball.dy * ball.dt;
		ball.dy += ball_g * ball.dt;
    ball.x  += ball.dx;
		
		// Check for wall collisions
		for(i = 0; i < NUM_BARRIERS; i++) {
			next_col = detect_rect_collision(ball, barriers[i]);
			
			col.dx_modifier *= next_col.dx_modifier;
			col.dy_modifier *= next_col.dy_modifier;
			col.occured |= next_col.occured;
		}
		
		// Check for net collision
		next_col = detect_rect_collision(ball, court_net);	
		col.dx_modifier *= next_col.dx_modifier;
		col.dy_modifier *= next_col.dy_modifier;
		col.occured |= next_col.occured;
		// If we hit the net, we need to remember to redraw
		if(next_col.occured) {
			draw_net = 1;
		}
		
		// If we had a colission, make sure we step back and use our original x and y
		if(col.occured) {
			ball.t = 0;
			ball.y = ball_old_loc.y;
			ball.x = ball_old_loc.x;
			
			ball.dx *= col.dx_modifier;
			ball.dy *= col.dy_modifier;
			//os_tsk_delete_self();
		}
		
//     collision_detected = 0;
//     collision_detected_opponent = 0;
//     if ( ball.x > 165 ){ // half court = 155
//       // check for collision with player
//       os_mut_wait(mut_player, 0xffff);
//       if ( !collision_detected_before ){
//         collision_detected = detect_slime_collision(ball, player);
//       } else if ( !detect_slime_collision(ball, player) ) {
//         collision_detected_before = 0;
//       }
//       os_mut_release(mut_player);

//       if (collision_detected){
//         collision_detected_before = 1;

//         // collision detected update ball velocity
//         os_mut_wait(mut_player, 0xffff);

//         // update velocities
//         new_ball_vx = (((ball_mass - slime_mass) * ball.dx) + (2 * player.dx * ball_mass * slime_mass)) / (slime_mass + ball_mass);
//         os_mut_release(mut_player);

//         // Find Point of Collision and update ball Location
//         sx = player.x + 30;
//         sy = player.y + 30;
//         bx = ball.x + ball_rad;
//         by = ball.y + ball_rad;
//         c = sqrt((sy - by) * (sy - by) +  (sx - bx) * (sx - bx));
//         new_ball_y = sy - (30 + ball_rad) * ( (sy-by) / c );

//         if (sx - bx < 0){
//           new_ball_x = sx + (30 + ball_rad)*( (sx-bx) / c  );
//         }
//         else if (sx - bx  > 0){
//           new_ball_x = sx - (30 + ball_rad)*( (sx-bx) / c  );
//         }
//         ball.y = new_ball_y - ball_rad;
//         ball.x = new_ball_x - ball_rad;
//         ball.dx = new_ball_vx;
//         ball_origin = ball.y;
//         ball.t = 0;
//       }

//       // check for opponent scoring
//       // TODO: 1. Pause game (pause OS), 2.Increase opponent's score, 3. Restart Game
//       if ( ball.y + ball.height >= y_max ){
//         ball.t = 0;
//         ball_origin = y_max - ball.height;
//         ball.y = y_max - ball.height;
//       }
//     }
//     else if ( ball.x < 155 ) {

//       // check for collision with opponent
//       os_mut_wait(mut_opponent, 0xffff);
//       collision_detected_opponent = detect_slime_collision(ball, opponent);
//       os_mut_release(mut_opponent);

//       if (collision_detected) {
//         // collision detected update ball direction
//         ball.t = 0;
//       }
//       // check if player scored
//       // TODO: 1. Pause game (pause OS), 2.Increase player's score, 3. Restart Game
//       else if (ball.y + ball.height >= y_max){  
//         ball.y = y_max - ball.height;
//       }
//     }

		

    // Draw Ball
    os_mut_wait(mut_GLCD, 0xffff);
    write_obect_lcd(ball_old_loc);
    write_obect_lcd(ball);

    // Draw net if ball collided with net
    if (draw_net) {
      write_obect_lcd(court_net);
    }
    // Redraw slimes. We could change this so redraw only occurs when collision occurs
		os_mut_wait(mut_player, 0xffff);
		//write_obect_lcd(player);
		os_mut_release(mut_player);

		os_mut_wait(mut_opponent, 0xffff);
		write_obect_lcd(opponent);
		os_mut_release(mut_opponent);

    os_mut_release(mut_GLCD);
    os_mut_release(mut_ball);

  }
}

__task void opponent_tsk (void) {
   os_itv_set (update_interval);
  for(;;){
    os_itv_wait ();
    //os_mut_wait(mut_opponent, 0xffff);
    //os_mut_release(mut_opponent);
  }

}


/*----------------------------------------------------------------------------
  Task 6 'init': Initialize
 *---------------------------------------------------------------------------*/
/* NOTE: Add additional initialization calls for your tasks here */
__task void init (void) {

  os_mut_init(mut_GLCD);
  os_mut_init(mut_player);
  os_mut_init(mut_opponent);
  os_mut_init(mut_curr_score);
  os_mut_init(mut_ball);

  t_player = os_tsk_create(player_tsk, 0);
  t_opponent = os_tsk_create(opponent_tsk, 0);
  t_ball = os_tsk_create(ball_tsk, 0);
  t_keyread = os_tsk_create(keyread,0);

  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
	
	//printf("Program started");
	
	#ifdef DEBUG_PRINT
	printf("Program started");
	#endif
	
  // TODO: Init player, opponent, ball and court here
	
	NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
	NVIC_EnableIRQ( EINT3_IRQn );

  LED_Init ();                              /* Initialize the LEDs           */
  GLCD_Init();                              /* Initialize the GLCD           */
	KBD_Init ();                              /* initialize Push Button        */
	ADC_Init ();															/* initialize the ADC            */
	init_scroll();                            /* initialize scrolling */


  /* 
    Inital game state is defined here. Bitmap files are included and are assinged here.

  */
  
  // Physical limits of the court used to limit motion of player, opponent and ball
  ai_left_limit = x_min;
  ai_right_limit = x_court_net;
  player_left_limit = x_court_net + 10;
  player_right_limit = 320 - width_player_bmp;
  player_high_limit = y_max - height_player_bmp;
  player_low_limit = y_min;
  
  // Court net placed in the center of the court
  court_net.x = x_court_net;
  court_net.y = y_court_net;
  court_net.width = width_court_net_bmp;
  court_net.height = height_court_net_bmp;
  court_net.bitmap = (unsigned char *)&pic_court_net6_bmp;
  court_net.dx = 0;
  court_net.dy = 0;
	
	// Make bounding boxes for all the edges
	// Left wall
	barriers[LEFT_WALL].x = -20;
	barriers[LEFT_WALL].y = -20;
	barriers[LEFT_WALL].width = 20;
	barriers[LEFT_WALL].height = y_max + 40;
	// Right wall
	barriers[RIGHT_WALL].x = x_max;
	barriers[RIGHT_WALL].y = -20;
	barriers[RIGHT_WALL].width = 20;
	barriers[RIGHT_WALL].height = y_max + 40;
	// Top wall
	barriers[TOP_WALL] = barriers[LEFT_WALL]; // Same starting x, y
	barriers[TOP_WALL].width = x_max + 40;
	barriers[TOP_WALL].height = 20;
	// Bottom wall
	barriers[BOTTOM_WALL].x = -20;
	barriers[BOTTOM_WALL].y = y_max;
	barriers[BOTTOM_WALL].width = x_max + 40;
	barriers[BOTTOM_WALL].height = 100;

	// Inital player values. Player must begin stationary
  player.x = x_max - width_player_bmp; 
  player.y = y_max - height_player_bmp;
	player.width = width_player_bmp;
	player.height = height_player_bmp;
  player.dx = 0;
  player.dy = 0;
  player.dt = 0.6;
  player.t =0;
	player.bitmap = (unsigned char *)&pic_player8_bmp;

  /*
    A completly cyan coloured bitmap. This bitmap is drawn over a slime's
    last horizontal location.
  */ 
  player_old_loc_x.x = player.x;
  player_old_loc_x.y = player.y;
  player_old_loc_x.width = 10;
  player_old_loc_x.height = 30;
  player_old_loc_x.bitmap = (unsigned char *)&pic_bg_height_bmp;

  /*
    This bitmap serves the same purpose as player_old_loc_x but overwrites
    the last vertical location. These should be able to be used for both slimes,
    otherwise we can just define another pair of bitmaps for the opponent using pic_bg_wide_bmp.
  */ 
  player_old_loc_y.x = player.x;
  player_old_loc_y.y = player.y;
  player_old_loc_y.width = 60;
  player_old_loc_y.height = 20;
  player_old_loc_y.bitmap = (unsigned char *)&pic_bg_wide_bmp;
	
	// init opponent values
	opponent.x = x_min + 40; // add 40 so ai starts in the middle sort of
	opponent.y = y_max - height_ai_bmp;
	opponent.width = width_ai_bmp;
	opponent.height = height_ai_bmp;
  opponent.dx = 0;
  opponent.dy = 0;
  opponent.dt = 0.6;
  opponent.t = 0;
	opponent.bitmap = (unsigned char *)&pic_ai8_bmp;
	
	/*
    Init ball. Ball begins over players head and continues to bounce on it until player moves.
    The ball always bounces off a surface with ball.dy = -40 unless it bounces of the top of the court
  */ 
	ball.x = 1;//player.x + player.width / 2 - ball.width / 2;
	ball.y = 1;//player.y - ball.height;
	ball.width = width_ball_bmp;
	ball.height = height_ball_bmp;
  ball.dx = 0;
  ball.dy = 0;
  ball.dt = 0.9;
  ball.t = 0;
  ball.bitmap = (unsigned char *)&pic_ball6_bmp;
  ball_origin = y_max - ball.height;
  
  // bitmap for overwritting ball's prev vertical position
  ball_old_loc = ball; // Start with same values, different image
  ball_old_loc.bitmap = (unsigned char *)&pic_bg_ball_bmp;
	
	// Initialize score
	curr_score.player = 0;
	curr_score.opponent = 0;

  // Changing this ratio changes the difficulty of the game. The larger
  // this ratio is the greater the increase in the ball's velocity after a collision
  // with a slime.
  slime_mass = 3;
  ball_mass = 1;
  collision_detected = 0;

  // Draw inital bitmaps
	init_game(court_net, player, opponent, ball);
	
	os_sys_init(init);                        /* Initialize RTX and start init*/ 

}
