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
short detect_collision(object ball, object slime){
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
		os_mut_wait(mut_GLCD, 0xffff);
		
		os_mut_release(mut_GLCD);
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
  short player_jumped = 0;
  os_itv_set (update_interval);
  for (;;) {
    os_itv_wait ();
    os_mut_wait(mut_player, 0xffff);
    KBD_val = KBD_Get();
    player_moved_x = 0;
    player_old_loc_x.x = player.x;
    player_old_loc_x.y = player.y;
    player_old_loc_y.x = player.x;
    player_old_loc_y.y = player.y;

     if (player.y >= player_high_limit && (player_jumped & 1) ){
      player_jumped = 0;
      player.y = player_high_limit;
      player.dy = 0;

    }

    else if ( player.y < player_high_limit && (player_jumped) ) {      
      player.t = player.t + player.dt; 
      player.y =  0.5 * g * player.t * player.t - player.dy * player.t + player_high_limit;
      if (player.y > player_high_limit){
        player.y = player_high_limit;
      }
      player_jumped = 1;
      if (player.y < player_old_loc_y.y) {
        player_old_loc_y.y = player.y + player.height;
      } 
    }

    else if ( ~KBD_val & KBD_UP){
      player.t = 0;
      player.dy = 30;
      player.y = player_high_limit - 1;
      player_jumped = 2;
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
      os_mut_wait(mut_GLCD, 0xffff);
      write_obect_lcd(player_old_loc_y);
      write_obect_lcd(player);
      os_mut_release(mut_GLCD);
    }
    if (player_moved_x) {
      os_mut_wait(mut_GLCD, 0xffff);
      write_obect_lcd(player_old_loc_x);
      write_obect_lcd(player);
      os_mut_release(mut_GLCD);
    }
    os_mut_release(mut_player);
  }


}

__task void ball_tsk (void) {

  os_itv_set (update_interval);
  for (;;){
    os_itv_wait ();
    os_mut_wait(mut_ball, 0xffff);
    ball_old_loc_y.x = ball.x;
    ball_old_loc_y.y = ball.y;
    ball_old_loc_x.x = ball.x;
    ball_old_loc_x.y = ball.y;
    draw_net = 0;
    // Update position of ball
    
    ball.t = ball.t + ball.dt;
    ball.y = 0.5 * ball_g * ball.t * ball.t + ball.dy * ball.t + ball_origin;
    ball.x += ball.dx;
    if (ball.x + ball.width >= x_max && ball.dx > 0){
        ball.x = x_max - ball.width;
        ball.dx *= -1;
    }
    if (ball.x <= x_min && ball.dx < 0) {
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
    collision_detected_opponent = 0;
    if ( ball.x > 165 ){ // half court = 155
      // check for collision with player
      os_mut_wait(mut_player, 0xffff);
      if ( !collision_detected_before ){
        collision_detected = detect_collision(ball, player);
      } else if ( !detect_collision(ball, player) ) {
        collision_detected_before = 0;
      }
      os_mut_release(mut_player);

      if (collision_detected){
        collision_detected_before = 1;

        // collision detected update ball velocity
        os_mut_wait(mut_player, 0xffff);

        // update velocities
        new_ball_vx = (((ball_mass - slime_mass) * ball.dx) + (2 * player.dx * ball_mass * slime_mass)) / (slime_mass + ball_mass);
        os_mut_release(mut_player);

        // Find Point of Collision and update ball Location
        sx = player.x + 30;
        sy = player.y + 30;
        bx = ball.x + ball_rad;
        by = ball.y + ball_rad;
        c = sqrt((sy - by) * (sy - by) +  (sx - bx) * (sx - bx));
        new_ball_y = sy - (30 + ball_rad) * ( (sy-by) / c );

        if (sx - bx < 0){
          new_ball_x = sx + (30 + ball_rad)*( (sx-bx) / c  );
        }
        else if (sx - bx  > 0){
          new_ball_x = sx - (30 + ball_rad)*( (sx-bx) / c  );
        }
        ball.y = new_ball_y - ball_rad;
        ball.x = new_ball_x - ball_rad;
        ball.dx = new_ball_vx;
        ball_origin = ball.y;
        ball.t = 0;
      }

      // check for opponent scoring
      // TODO: 1. Pause game (pause OS), 2.Increase opponent's score, 3. Restart Game
      if ( ball.y + ball.height >= y_max ){
        ball.t = 0;
        ball_origin = y_max - ball.height;
        ball.y = y_max - ball.height;
      }
    }
    else if ( ball.x < 155 ) {

      // check for collision with opponent
      os_mut_wait(mut_opponent, 0xffff);
      collision_detected_opponent = detect_collision(ball, opponent);
      os_mut_release(mut_opponent);

      if (collision_detected) {
        // collision detected update ball direction
        ball.t = 0;
      }
      // check if player scored
      // TODO: 1. Pause game (pause OS), 2.Increase player's score, 3. Restart Game
      else if (ball.y + ball.height >= y_max){  
        ball.y = y_max - ball.height;
      }
    }

    // Ideally we should draw bitmaps that have dimensions of height = ball.dy, width = ball.width
    // but this seems to look fine for now
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
    os_mut_wait(mut_GLCD, 0xffff);
    write_obect_lcd(ball_old_loc_y);
    write_obect_lcd(ball_old_loc_x);
    write_obect_lcd(ball);

    // Draw net if ball collided with net
    if (draw_net) {
      write_obect_lcd(court_net);
    }
    // Redraw slimes. We could change this so redraw only occurs when collision occurs
      os_mut_wait(mut_player, 0xffff);
      write_obect_lcd(player);
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
	ball.x = player.x;
	ball.y = player.y - ball.height;
	ball.width = width_ball_bmp;
	ball.height = height_ball_bmp;
  ball.dx = 0;
  ball.dy = -40;
  ball.dt = 0.9;
  ball.t = 0;
  ball.bitmap = (unsigned char *)&pic_ball6_bmp;
  ball_origin = y_max - ball.height;
  
  // bitmap for overwritting ball's prev vertical position
  ball_old_loc_y.x = 170;
  ball_old_loc_y.y = ball.y;
  ball_old_loc_y.width = 20;
  ball_old_loc_y.height = 20;
  ball_old_loc_y.dx = ball.dx;
  ball_old_loc_y.dy = ball.dy;
  ball_old_loc_y.dt = ball.dt;
  ball_old_loc_y.t = ball.t;
  ball_old_loc_y.bitmap = (unsigned char *)&pic_bg_ball_bmp;

  // bitmap for overwritting ball's prev horizontal position
  ball_old_loc_x.x = 170;
  ball_old_loc_x.y = player.y;
  ball_old_loc_x.width = 20;
  ball_old_loc_x.height = 20;
  ball_old_loc_x.dx = ball.dx;
  ball_old_loc_x.dy = ball.dy;
  ball_old_loc_x.dt = ball.dt;
  ball_old_loc_x.t = ball.t;
  ball_old_loc_x.bitmap = (unsigned char *)&pic_bg_ball_bmp;

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
