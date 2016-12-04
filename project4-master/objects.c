#include <RTL.h>
#include "objects.h"
#include "LPC17xx.H"
#include "GLCD.h"
#include "LED.h"

void init_game(object court_net, object player, object opponent, object ball){
	
	
    // Fill background
	GLCD_Clear(Cyan);
	GLCD_Bitmap(court_net.x,court_net.y,court_net.width,court_net.height, court_net.bitmap); // only need to draw court net once 
	GLCD_Bitmap(opponent.x,opponent.y,opponent.width,opponent.height, opponent.bitmap);
	GLCD_Bitmap(player.x,player.y,player.width,player.height, player.bitmap);
  GLCD_Bitmap(ball.x, ball.y, ball.width, ball.height, ball.bitmap);
}


void write_obect_lcd(object obj){
    GLCD_Bitmap((unsigned int)(obj.x),(unsigned int)(obj.y),obj.width,obj.height, obj.bitmap);
}


void write_ball(object ball){
    int i, j;
    for ( i = 0; i < ball.height; i++){
        for ( j = 0; j < (ball.width); j++){
            if(*((unsigned short *)ball.bitmap + (i*ball.width) + j) == Yellow){
                GLCD_SetTextColor(0);
                GLCD_PutPixel(ball.x+j,ball.y+i);
            }
        }
    }

}
