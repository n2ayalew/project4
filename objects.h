typedef struct object {
	int x;
	int y;
	int height;
	int width;
	int * bitmap[5][5]; 
}

// init functions
void init_slime(object slime);
void init_ball(object slime);
void init_court(void);
