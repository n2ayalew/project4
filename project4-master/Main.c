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
object ball_old_loc_x;
object ball_old_loc_y;

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
float coll_rad;

float ball_g = 9.8;
float a,b,c,sx,bx,sy,by;
float theta;
int ball_rad = 10;

int new_ball_vx;
int new_ball_y;
int new_ball_x;
short collision_detected_before;
short reversed;
short draw_net;
short player_jumped = 0;

int ai_left_limit;
int ai_right_limit;
int player_left_limit;
int player_right_limit;
int player_low_limit;
int player_high_limit;
int ball_max_y = 240;
int ball_max_x = 320;

// Peripherals
unsigned int ADCStat = 0;
unsigned int ADCValue = 0;
uint32_t INT0_val = 0;


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
    else if ( b->x < 160 && (b->x + b->width) > 155 &&  (b->y - court_net.y) < b->height){
      b->t = 0;
      ball_origin = court_net.y - b->height;
      b->y = ball_origin;
      b->t = b->t + b->dt;
      b->y = 0.5 * ball_g *b->t * b->t + b->dy * b->t + ball_origin;
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
		//os_mut_wait(mut_GLCD, 0xffff);
		
		//os_mut_release(mut_GLCD);
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
      // else {
      //   player.dx = -10;
      //   player.x += player.dx;
      // }
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
    
    //os_mut_wait(&mut_GLCD, 0xffff);
    //write_obect_lcd(player_old_loc_y);
    //os_mut_release(&mut_GLCD);
    
    os_mut_wait(&mut_GLCD, 0xffff);
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

  //int ball_rad = 10;
  //int bcx, bcy,scy, scx, collid_rad; // used for circle centers for easier math
  //float theta;
  //unsigned int collision_location;

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
    if (ball.x + ball.width >= ball_max_x && ball.dx > 0){
        ball.x = x_max - ball.width;
        ball.dx *= -1;
    }
    if (ball.x <= 0 && ball.dx < 0) {
      ball.x = x_min;
      ball.dx *= -1;
    }
    // check for collision with net
    if ( ball.x >= 160 && ball.x - court_net.x < court_net.width && ball.y > court_net.y && ball.dx < 0){
      ball.x = court_net.x + court_net.width;
      ball.dx *= -1;
      draw_net = 1;
    }
    else if (ball.x <= 155 && court_net.x - ball.x < ball.width && ball.y > court_net.y && ball.dx > 0){
      ball.x = court_net.x - ball.width;
      ball.dx *= -1;
      draw_net = 1;
    }
    else if ( ball.x < 160 && (ball.x + ball.width) > 155 &&  (ball.y - court_net.y) < ball.height){
      ball.t = 0;
      ball_origin = court_net.y - ball.height;
      ball.y = ball_origin;

      ball.t = ball.t + ball.dt;
      ball.y = 0.5 * ball_g * ball.t * ball.t + ball.dy * ball.t + ball_origin;
      //ball.x += ball.dx;
    }
    collision_detected = 0;
    if ( ball.x > 165 ){ // half court = 155
      // check for collision with player
      
      if ( !collision_detected_before ){
        os_mut_wait(&mut_player, 0xffff);
        collision_detected = detect_collision(ball, player);
       os_mut_release(&mut_player);
      } else {//if (!collision_detected){
        collision_detected_before = 0;
      }
      

      if (collision_detected){
        collision_detected_before = 1;

        // collision detected update ball velocity

        os_mut_wait(&mut_player, 0xffff);
        // update velocities
        new_ball_vx = (((ball_mass - slime_mass) * ball.dx) + (2 * player.dx * ball_mass * slime_mass)) / (slime_mass + ball_mass);
        //printf("%d\n", new_ball_vx);
        sx = player.x + 30;
        sy = player.y + 30;
        os_mut_release(&mut_player);  
        bx = ball.x + ball_rad;
        by = ball.y + ball_rad;
        // Find Point of Collision and update ball Location
        if ( new_ball_vx > 0 && sx > bx ){
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
        if (new_ball_x + ball_rad >= ball_max_x){
        	new_ball_x = ball_max_x - ball_rad; 
        }
        
        ball.y = new_ball_y - ball_rad;
        ball.x = new_ball_x - ball_rad;
        ball.dx = new_ball_vx;
        ball_origin = ball.y;
        ball.t = 0;

        net_collision(&ball);
      }
      // check for ai scoring
      if ( ball.y + ball.height >= ball_max_y ){
        ball.t = 0;
        ball_origin = ball_max_y - ball.height;
        ball.y = ball_max_y - ball.height;
      }
    }
    else if ( ball.x < 155 ) {

      // check for collision with opponent
      os_mut_wait(&mut_opponent, 0xffff);
      collision_detected = detect_collision(ball, opponent);
      os_mut_release(&mut_opponent);

      if (collision_detected) {
        // collision detected update ball direction
        ball.t = 0;
      }
      // check if player scored
      else if (ball.y + ball.height >= ball_max_y){  
        ball.y = ball_max_y - ball.height;
      }
    }

    // this is temporary we should be able to draw bitmaps that have dimensions of height = ball.dy, width = ball.width
    if (ball.y < ball_old_loc_y.y) {
      ball_old_loc_y.y = ball.y + ball.height;
    }

    if ( ball.y <= y_min  && ball.dy < 0) {
      ball.t = 0;
      ball.y = y_min;
      ball_origin = y_min;
      ball.dy *= -1;
    } 

    // Draw Ball
    os_mut_wait(&mut_GLCD, 0xffff);
    write_obect_lcd(ball_old_loc_y);
    write_obect_lcd(ball_old_loc_x);
    write_obect_lcd(ball);
    // Draw net if ball collided with net
    if (draw_net) {
      write_obect_lcd(court_net);
    }
    // Redraw both slimes if collision occured
    if (collision_detected){
      os_mut_wait(&mut_player, 0xffff);
      write_obect_lcd(player);
      os_mut_release(&mut_player);

      os_mut_wait(&mut_opponent, 0xffff);
      write_obect_lcd(opponent);
      os_mut_release(&mut_opponent);
    }
    os_mut_release(&mut_GLCD);
    os_mut_release(&mut_ball);

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
	
  // TODO: Init player, opponent, ball and court here
	
	NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
	NVIC_EnableIRQ( EINT3_IRQn );

  LED_Init ();                              /* Initialize the LEDs           */
  GLCD_Init();                              /* Initialize the GLCD           */
	KBD_Init ();                              /* initialize Push Button        */
	ADC_Init ();															/* initialize the ADC            */
	init_scroll();                            /* initialize scrolling */
  // Inital x and y values are kind of arbitrary and can be changed if need be

  ai_left_limit = x_min;
  ai_right_limit = x_court_net;
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

	// init player values
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

  player_old_loc_x.x = player.x;
  player_old_loc_x.y = player.y;
  player_old_loc_x.width = 10;
  player_old_loc_x.height = 30;
  player_old_loc_x.bitmap = (unsigned char *)&pic_bg_height_bmp;

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
	
	// init ball values
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

  ball_old_loc_y.x = 170;
  ball_old_loc_y.y = ball.y;
  ball_old_loc_y.width = 20;
  ball_old_loc_y.height = 20;
  ball_old_loc_y.dx = ball.dx;
  ball_old_loc_y.dy = ball.dy;
  ball_old_loc_y.dt = ball.dt;
  ball_old_loc_y.t = ball.t;
  ball_old_loc_y.bitmap = (unsigned char *)&pic_bg_ball_bmp;

  // We might want to make a new one with a width equal to typical ball.dx value
  ball_old_loc_x.x = 170;
  ball_old_loc_x.y = player.y;
  ball_old_loc_x.width = 20;
  ball_old_loc_x.height = 20;
  ball_old_loc_x.dx = ball.dx;
  ball_old_loc_x.dy = ball.dy;
  ball_old_loc_x.dt = ball.dt;
  ball_old_loc_x.t = ball.t;
  ball_old_loc_x.bitmap = (unsigned char *)&pic_bg_ball_bmp;

  slime_mass = 3;
  ball_mass = 1;
  collision_detected = 0;
  collision_detected_before = 0;
	init_game(court_net, player, opponent, ball);
	
	os_sys_init(init);                        /* Initialize RTX and start init*/ 
}
