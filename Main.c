#include <RTL.h>
#include "LPC17xx.H"                    /* LPC17xx definitions               */
#include "GLCD.h"
#include "LED.h"
#include "KBD.h"
#include "ADC.h"

#define __FI        1                   /* Font index 16x24                  */

OS_TID t_led;                           /* assigned task id of task: led */
OS_TID t_adc;                           /* assigned task id of task: adc */
OS_TID t_kbd;                           /* assigned task id of task: keyread */
OS_TID t_jst   ;                        /* assigned task id of task: joystick */
OS_TID t_clock;                         /* assigned task id of task: clock   */
OS_TID t_lcd;                           /* assigned task id of task: lcd     */

OS_MUT mut_GLCD;                        /* Mutex to controll GLCD access     */

object player;
OS_MUT mut_player;

object opponent;
OS_MUT mut_opponent;

object ball;
OS_MUT mut_ball;


unsigned int ADCStat = 0;
unsigned int ADCValue = 0;

/*----------------------------------------------------------------------------
  switch LED on
 *---------------------------------------------------------------------------*/
void LED_on  (unsigned char led) {
  LED_On (led); //turn on the physical LED
  os_mut_wait(mut_GLCD, 0xffff); //Rest of code updates graphics on the LCD
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Green);
  GLCD_DisplayChar(4, 5+led, __FI, 0x80+1); /* Circle Full                   */
  os_mut_release(mut_GLCD);
}

/*----------------------------------------------------------------------------
  switch LED off
 *---------------------------------------------------------------------------*/
void LED_off (unsigned char led) {
  LED_Off(led); //turn off the physical LED
  os_mut_wait(mut_GLCD, 0xffff); //Rest of code updates graphics on the LCD
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Green);
  GLCD_DisplayChar(4, 5+led, __FI, 0x80+0);  /* Circle Empty                 */
  os_mut_release(mut_GLCD);
}


/*----------------------------------------------------------------------------
  Task 1 'LEDs': Cycle through LEDs
 *---------------------------------------------------------------------------*/
__task void led (void) {
	int num_led = 3; //the number of LEDs to cycle through
	int on = 0;
	int off = 0;
	
	while(1)
	{
		for( on = 0; on <  num_led; on++)
		{
			
			off = on - 1; //Figure out which LED to turn off, wrap around if needed
			if(off == -1)
			{ 
				off = num_led -1;
			}
			LED_off(off);			
			LED_on (on);
			os_dly_wait (50);                      /* delay 50 clock ticks             */
		}
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
/*NOTE: You need to complete this function*/
__task void adc (void) {
  for (;;) {

  }
}

/*----------------------------------------------------------------------------
  Task 4 'joystick': process an input from the joystick
 *---------------------------------------------------------------------------*/
/*NOTE: You need to complete this function*/
__task void joystick (void) {
  for (;;) {

  }
}


/*----------------------------------------------------------------------------
  Task 5 'lcd': LCD Control task
 *---------------------------------------------------------------------------*/
__task void lcd (void) {

  for (;;) {
    os_mut_wait(mut_GLCD, 0xffff);
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
    os_dly_wait (400);
  }
}

/*----------------------------------------------------------------------------
  Task 6 'init': Initialize
 *---------------------------------------------------------------------------*/
/* NOTE: Add additional initialization calls for your tasks here */
__task void init (void) {

  os_mut_init(mut_GLCD);

  t_led 	 = os_tsk_create (led, 0);		 /* start the led task               */
	t_kbd		 = os_tsk_create (keyread, 0);     /* start the kbd task           */
	t_adc 	 = os_tsk_create (adc, 0);		 /* start the adc task               */
	t_jst		 = os_tsk_create (joystick, 0);     /* start the joystick task           */
  t_lcd    = os_tsk_create (lcd, 0);     /* start task lcd                   */
  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {

  // TODO: Init player, opponent, ball and court here
	
	NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
	
  LED_Init ();                              /* Initialize the LEDs           */
  GLCD_Init();                              /* Initialize the GLCD           */
	KBD_Init ();                              /* initialize Push Button        */
	ADC_Init ();															/* initialize the ADC            */

  GLCD_Clear(White);                        /* Clear the GLCD                */

  os_sys_init(init);                        /* Initialize RTX and start init */
}
