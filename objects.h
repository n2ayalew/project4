typedef struct object {
	int x;
	int y;
	int height;
	int width;
	int * bitmap[5][5]; 
};

typedef struct score {
	short player;
	short opponent;
};

// init functions
void init_slime(object slime);
void init_ball(object slime);
void init_score(score sc)
void init_court(void);

// Write bitmat from object to screen
void write_obect_lcd(object obj);
// Write bg colour over bounding box defined by obj
void write_bg(object obj);


// Write the values from the score object to the LED
void write_score(score sc);

// Graham: Ball, score
// Nattie: player, opponent, pot, lcd	
