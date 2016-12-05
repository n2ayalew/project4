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
#include "ai8.bmp_out.c"
#include "ball6.bmp_out.c"
#include "player8.bmp_out.c"
#include "bg_height.bmp_out.c"
#include "bg_wide.bmp_out.c"
#include "bg_ball.bmp_out.c"
#include "court_net6.bmp_out.c"

#define __FI        1                   /* Font index 16x24                  */


OS_MUT mut_GLCD;                        /* Mutex to controll GLCD access     */

// court vars
object court_net;

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

// ball vars
object ball;
OS_MUT mut_ball;
object ball_old_loc_x;
object ball_old_loc_y;

score curr_score;
OS_MUT mut_curr_score;

// Game vars
const int g = 9;
const int base_ball_g = 9;
int ball_g = 9;
const U16 update_interval = 8;
int slime_mass;
int ball_mass;
int ball_origin;

short game_started;
short collision_detected;
float coll_rad;
float a,b,c,sx,bx,sy,by;
int ball_rad = 10;
short collision_detected_before;
short op_collision_detected_before;
short op_collision_detected;
int new_ball_vx;
int new_ball_y;
int new_ball_x;

short draw_net;
short player_jumped = 0;

int player_left_limit;
int player_right_limit;
int player_low_limit;
int player_high_limit;


// Peripherals
unsigned int ADCStat = 0;
unsigned int ADCValue = 0;
uint32_t INT0_val = 0;


void init_player() {
  player.x = 240; 
  //player.x = player.x  - 60; // subtract 60 so player starts in the middle
  player.y = 210;
  player.width = 60;
  player.height = 30;
  player.dx = 0;
  player.dy = 0;
  player.dt = 0.6;
  player.t = 0;
  player.bitmap = (unsigned char *)&pic_player8_bmp;

  player_old_loc_x = player;
  player_old_loc_x.width = 10;
  player_old_loc_x.height = 30;
  player_old_loc_x.bitmap = (unsigned char *)&pic_bg_height_bmp;

  player_old_loc_y = player;
  player_old_loc_y.width = 60;
  player_old_loc_y.height = 20;
  player_old_loc_y.bitmap = (unsigned char *)&pic_bg_wide_bmp;
}

void init_opponent() {
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

  opponent_old_loc_x.x = opponent.x;
  opponent_old_loc_x.y = opponent.y;
  opponent_old_loc_x.width = 10;
  opponent_old_loc_x.height = 30;
  opponent_old_loc_x.bitmap = (unsigned char *)&pic_bg_height_bmp;
}

void init_ball() {
  ball.x = player.x + 20;
  ball.y = player.y - ball.height;
  ball.width = width_ball_bmp;
  ball.height = height_ball_bmp;
  ball.dx = 0;
  ball.dy = -40;
  ball.dt = 0.9;
  ball.t = 0;
  ball.bitmap = (unsigned char *)&pic_ball6_bmp;
  ball_origin = y_max - ball.height;

  ball_old_loc_y = ball;
  ball_old_loc_y.x = 170;
  ball_old_loc_y.width = 20;
  ball_old_loc_y.height = 20;
  ball_old_loc_y.bitmap = (unsigned char *)&pic_bg_ball_bmp;

  // We might want to make a new one with a width equal to typical ball.dx value
  ball_old_loc_y = ball;
  ball_old_loc_x.x = 170;
  ball_old_loc_x.width = 20;
  ball_old_loc_x.height = 20;
  ball_old_loc_x.bitmap = (unsigned char *)&pic_bg_ball_bmp;
}

void reset_board() {
	
	// Reset slime and player positions
	os_mut_wait(&mut_ball, 0xffff);
	init_ball();
  os_mut_release(&mut_ball);

	os_mut_wait(&mut_player, 0xffff);
	init_player();
  os_mut_release(&mut_player);
	
	init_opponent();

	os_mut_wait(&mut_GLCD, 0xffff);
 	GLCD_Clear(Cyan);
	GLCD_Bitmap(court_net.x, court_net.y, court_net.width, court_net.height, court_net.bitmap);
	os_mut_release(&mut_GLCD);
}

void reset_game (void){
	curr_score.opponent = 0;
	curr_score.player = 0;
	LED_Out(0);
// 	os_mut_wait(&mut_GLCD, 0xffff);
// 	GLCD_SetBackColor(Blue);
//   GLCD_SetTextColor(White);
//   GLCD_DisplayString(0, 0, __FI, "Game Over");
// 	os_mut_release(&mut_GLCD);
	reset_board();
}


void net_collision(object * b){
	// check for collision with net
    if ( b->x >= 160 && b->x - court_net.x < court_net.width && b->y > court_net.y && b->dx < 0){
      b->x = court_net.x + court_net.width;
      b->dx *= -1;
      draw_net = 1;
    }
    else if (b->x <= 155 && court_net.x - b->x < b->width &&b->y > court_net.y && b->dx > 0){
      b->x = court_net.x - b->width;
      b->dx *= -1;
      draw_net = 1;
    }
    else if ( b->x < 160 && (b->x + b->width) > 155 &&  (b->y  + b->height > court_net.y)){
      b->t = 0;
      ball_origin = court_net.y - b->height;
      b->y = ball_origin;
      b->t = b->t + b->dt;
      b->y = 0.5 * ball_g *b->t * b->t + b->dy * b->t + ball_origin;
      draw_net = 1;
    }
}
/*
  Axis Aligned Bounding Box Collision Detection
*/
// if this looks ugly lets try checking for collision  by checking if the velocity vectors intercept
short detect_collision(object volleyball, object slime){
  bx = volleyball.x  + 0.5 * volleyball.width;
  sx = slime.x + slime.height;
  by = volleyball.y + 0.5 * volleyball.height;
  sy = slime.y + slime.height;

  a = (bx - sx) * (bx - sx);
  b = (by - sy) * (by - sy);
  c = sqrt(a + b);

  if ( (unsigned int)c <= (0.5 * volleyball.height + slime.height) ) {
    return 1;
  }else {
    return 0;
  }
}

void score_player() {
	
	os_mut_wait(&mut_curr_score, 0xffff);
	
	if(curr_score.player == 4) {
		reset_game();
	} else {
		LED_On(curr_score.player);
		(curr_score.player)++;
		reset_board();
	}
	
	os_mut_release(&mut_curr_score);
}

void score_opponent() {

	os_mut_wait(&mut_curr_score, 0xffff);
	
	if(curr_score.opponent == 4) {
		reset_game();
	} else {
		(curr_score.opponent)++;
		LED_On(8 - curr_score.opponent);
		reset_board();
	}
	
	os_mut_release(&mut_curr_score);
}

/*----------------------------------------------------------------------------
  Task 3 'ADC': Read potentiometer
 *---------------------------------------------------------------------------*/
/*Value from potentiometer is used to define the pace of the game*/
__task void adc_tsk (void) {
	int pot_val = 0;
	char str [10];
  // 0 - 4095
  os_itv_set (update_interval * 10);

  for (;;) {
    os_itv_wait ();
		
		ADC_StartCnv();
		pot_val = ADC_GetCnv() / 1000;
		
    sprintf(str, "ACD: %d", pot_val);       /* format text for print out     */

		os_mut_wait(&mut_GLCD, 0xffff);
		GLCD_SetBackColor(Cyan);
		GLCD_SetTextColor(Red);
		GLCD_DisplayString(0, 0, 1, (unsigned char *)str);
		os_mut_release(&mut_GLCD);
		
		ball_g = base_ball_g + pot_val;
  }
}

__task void player_tsk (void) {
  os_itv_set (update_interval);
  for (;;) {
    os_itv_wait ();
    os_mut_wait(&mut_player, 0xffff);
    KBD_val = KBD_Get();
    player_moved_x = 0;
    player_old_loc_x.x = player.x;
    player_old_loc_x.y = player.y;
    player_old_loc_y.x = player.x;
    player_old_loc_y.y = player.y;

 
    if (player.y >= player_high_limit && player_jumped ){
      player_jumped = 0;
      player.y = player_high_limit;
      player.dy = 0;
    }

    else if ( player.y < player_high_limit &&  player_jumped ) {      
      player.t = player.t + player.dt; 
      player.y =  0.5 * g * player.t * player.t - player.dy * player.t + player_high_limit;
      if (player.y > player_high_limit){
        player.y = player_high_limit;
      }

      if (player.y < player_old_loc_y.y) {
      	if ((player.y + player.height + player_old_loc_y.height) > 240){
      		player_old_loc_y.y = 240 - player_old_loc_y.height;
      	}else {
      		player_old_loc_y.y = player.y + player.height;
      	}
      	//player_old_loc_y.y = player.y + player.height;
      }
     
    }

    else if ( ~KBD_val & KBD_UP && (!player_jumped) ){
      player.t = 0;
      player.dy = 30;
      player.y = player_high_limit;
      player_jumped = 1;
      player.t = player.t + player.dt; 
      player.y =  0.5 * g * player.t * player.t - player.dy * player.t + player_high_limit;
      //player_old_loc_y.y = player.y + player.height;
    }
    if ( ~KBD_val & KBD_LEFT && (player.x > player_left_limit) ){
      player_old_loc_x.x = player.x + (player.width - player_old_loc_x.width);
      player.dx = -10;
      player.x += player.dx;
      if ( player.x <=  player_left_limit){
        player.x = player_left_limit;
      }

      player_moved_x = 1;
    }
    else if ( (~KBD_val & KBD_RIGHT) && ( player.x < player_right_limit ) ){
      if (player.x + player.dx >= player_right_limit) {
        player.x = player_right_limit;
      }
      else {
        player.dx = 10;
        player.x += player.dx;
      }
      player_moved_x = 1;
    }
    else {
      player.dx = 0;
    }

   if (player_jumped){
   	
    player_old_loc_y.y = player.y + player.height;
    
    os_mut_wait(&mut_GLCD, 0xffff);
    write_obect_lcd(player_old_loc_y);
    write_obect_lcd(player);
    os_mut_release(&mut_GLCD);

    }
    if (player_moved_x) {
      os_mut_wait(&mut_GLCD, 0xffff);
      write_obect_lcd(player_old_loc_x);
      write_obect_lcd(player);
      os_mut_release(&mut_GLCD);
    }
    os_mut_release(&mut_player);
  }


}

__task void ball_tsk (void) {
  short opponent_moved;

  os_itv_set (update_interval);
  for (;;){
    os_itv_wait ();
    os_mut_wait(&mut_ball, 0xffff);
    ball_old_loc_y.x = ball.x;
    ball_old_loc_y.y = ball.y;
    ball_old_loc_x.x = ball.x;
    ball_old_loc_x.y = ball.y;
    draw_net = 0;
    // Update position of ball
    
    ball.t = ball.t + ball.dt;
    ball.y = 0.5 * ball_g * ball.t * ball.t + ball.dy * ball.t + ball_origin;
    ball.x += ball.dx;

    collision_detected = 0;
    op_collision_detected = 0;

    if ( ball.x > 165 ){ // half court = 155
      // check for collision with player
      
      if ( !collision_detected_before ){
        os_mut_wait(&mut_player, 0xffff);
        collision_detected = detect_collision(ball, player);
       os_mut_release(&mut_player);
      } else {
        collision_detected_before = 0;
      }
     
      if (collision_detected){
        collision_detected_before = 1;
        // collision detected update ball velocity
        // update velocities
        os_mut_wait(&mut_player, 0xffff);
        new_ball_vx = (((ball_mass - slime_mass) * ball.dx) + (2 * player.dx * ball_mass * slime_mass)) / (slime_mass + ball_mass);
        sx = player.x + 30;
        sy = player.y + 30;
        os_mut_release(&mut_player);  
     

      }
      // check for ai scoring
      if ( ball.y + ball.height > 240 && !collision_detected){
        ball.t = 0;
        ball_origin = 240 - ball.height;
        ball.y = 240 - ball.height;
        os_mut_release(&mut_ball);
        score_opponent();
        continue;
      }
    }

    /**********************************************/
    // Update location of opponent
    opponent_old_loc_x.x = opponent.x;
    if (ball.x < 155){
    	opponent_moved = 1;
    	if (ball.x > opponent.x){
    		opponent.dx = 10;
    		opponent.x += opponent.dx;
    	}
    	if (ball.x < opponent.x){
    		opponent_old_loc_x.x = opponent.x + 50;
    		opponent.dx = -10;
    		opponent.x += opponent.dx;
    	}
    }
    else {
    	opponent_moved = 0;
    }
    if (opponent_moved){
    	if (opponent.x < 0){
    		opponent.x = 0;
    	}
    	if (opponent.x + opponent.width > court_net.x){
    		opponent.x = court_net.x - opponent.width;
    	}
    	/*os_mut_wait(mut_GLCD, 0xffff);
    	write_obect_lcd(opponent_old_loc_x);
    	write_obect_lcd(opponent);
    	os_mut_release(mut_GLCD);*/
   	}

   	/**********************************************/

    if ( ball.x < 155 ) {

      // check for collision with opponent
      if ( !op_collision_detected_before ){
    
        op_collision_detected = detect_collision(ball, opponent);
   
      } 
      else {
        op_collision_detected_before = 0;
      }

      if (op_collision_detected) {

        op_collision_detected_before = 1;
      	new_ball_vx = (((ball_mass - slime_mass) * ball.dx) + (2 * opponent.dx * ball_mass * slime_mass)) / (slime_mass + ball_mass);
        sx = opponent.x + 30;
        sy = opponent.y + 30;
      }
      // check if player scored
      else if (ball.y + ball.height > 240 && !op_collision_detected ){  
        ball.y = 240 - ball.height;
        os_mut_release(&mut_ball);
        score_player();
        continue;
      }
    }

    /**********************************************/
    // Check for collisions with walls and court net
    if ( op_collision_detected || collision_detected ){
    	bx = ball.x + ball_rad;
        by = ball.y + ball_rad;
    	if ( collision_detected && new_ball_vx > 0 && sx > bx ){
    		new_ball_vx *= -1;
    	}
    	if ( op_collision_detected && new_ball_vx > 0 && sx < bx ){
    		new_ball_vx *= -1;
    	}

    	c = sqrt((sy - by) * (sy - by) +  (sx - bx) * (sx - bx));
    	new_ball_y = sy - (30 + ball_rad) * ( (sy-by) / c );
    	
    	if (bx > sx){
    	  new_ball_x = sx + (30 + ball_rad)*( (bx-sx) / c  );
    	}
    	else if (sx > bx){
    	  new_ball_x = sx - (30 + ball_rad)*( (sx-bx) / c  );
    	}
    	else {
    		new_ball_x = sx;
    	}
    	// Make sure no boundaries were crossed after collision
    	if (new_ball_x + ball_rad >= 320){
    		new_ball_x = 320 - ball_rad; 
    	}
    	ball.y = new_ball_y - ball_rad;
    	ball.x = new_ball_x - ball_rad;
    	ball.dx = new_ball_vx;
    	ball_origin = ball.y;
    	ball.t = 0;
    }

    /**********************************************/

    /**********************************************/
    // upate location of ball if boundaries were hit
    if (ball.x + ball.width > 320 && ball.dx > 0){
        ball.x = x_max - ball.width;
        ball.dx *= -1;
    }
    if (ball.x < 0 && ball.dx < 0) {
      ball.x = 0;
      ball.dx *= -1;
    }

    if ( ball.y < y_min  && ball.dy < 0) {
      ball.t = 0;
      ball.y = y_min;
      ball_origin = y_min;
      ball.dy *= -1;
      
    }
    net_collision(&ball);
    /**********************************************/


    // Draw Ball and Opponent
    os_mut_wait(&mut_GLCD, 0xffff);

    write_obect_lcd(ball_old_loc_y);
    write_obect_lcd(ball_old_loc_x);
    write_obect_lcd(ball);
    write_obect_lcd(opponent_old_loc_x);
    write_obect_lcd(court_net);
    write_obect_lcd(opponent);

    os_mut_release(mut_GLCD);

    // Redraw both slimes if collision occured
    if ( collision_detected ){

      os_mut_wait(&mut_GLCD, 0xffff);
      os_mut_wait(&mut_player, 0xffff);
      write_obect_lcd(player);
      os_mut_release(&mut_player);
      os_mut_release(&mut_GLCD);
    }
    os_mut_release(&mut_ball);

  }
}




/*----------------------------------------------------------------------------
  Task 6 'init': Initialize
 *---------------------------------------------------------------------------*/
/* NOTE: Add additional initialization calls for your tasks here */
__task void init (void) {

  os_mut_init(&mut_GLCD);
  os_mut_init(&mut_player);
  os_mut_init(&mut_curr_score);
  os_mut_init(&mut_ball);

  os_tsk_create(player_tsk, 0);
  os_tsk_create(ball_tsk,   0);
  os_tsk_create(adc_tsk,    0);


  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
	
  // TODO: Init player, opponent, ball and court here
	
  NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
  NVIC_EnableIRQ( EINT3_IRQn );

  LED_Init ();                              /* Initialize the LEDs           */
  GLCD_Init();                              /* Initialize the GLCD           */
  KBD_Init ();                              /* initialize Push Button        */
  ADC_Init ();															/* initialize the ADC            */
	
  init_player();
  init_opponent();
  init_ball();
	
  // Inital x and y values are kind of arbitrary and can be changed if need be

  player_left_limit = x_court_net + 10;
  player_right_limit = 320 - width_player_bmp;
  player_high_limit = 210;
  player_low_limit = y_min;
  
  court_net.x = x_court_net;
  court_net.y = y_court_net;
  court_net.width = width_court_net_bmp;
  court_net.height = height_court_net_bmp;
  court_net.bitmap = (unsigned char *)&pic_court_net6_bmp;
  court_net.dx = 0;
  court_net.dy = 0;
	
  // Initialize score
  curr_score.player = 0;
  curr_score.opponent = 0;

  slime_mass = 3;
  ball_mass = 1;
  collision_detected = 0;
  collision_detected_before = 0;
  op_collision_detected_before = 0;
	init_game(court_net, player, opponent, ball);
	
	os_sys_init(init);                        /* Initialize RTX and start init*/ 
}
