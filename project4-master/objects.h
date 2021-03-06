 // Inital x and y values are kind of arbitrary and can be changed if need be
// wide for up and down
// height left and right

#define width_court_net_bmp 10;//width of the net
#define height_court_net_bmp 50;//height of the net

#define width_player_bmp 60;//width of the player
#define height_player_bmp 30;//height of the player

#define width_ai_bmp 60;//width of the ai
#define height_ai_bmp 30;//height of the ai

#define width_ball_bmp 20
#define height_ball_bmp 20

// Volleyball Court defn's
#define bg_colour 0x07FF // Cyan

static const int x_court_net = 155;
static const int y_court_net = 190;
static const int y_max = 240;
static const int x_max = 320;
static const int y_min = 0;
static const int x_min = 0;


typedef struct {
	int x;
	int y;
	int height;
	int width;
	int dx; 
	int dy;
	unsigned char * bitmap;
	float t;
	float dt;
	unsigned int radius; 
} object;

typedef struct {
	short player;
	short opponent;
} score;

// init functions
void init_game(object court_net, object player, object opponent, object ball);


// Write bitmat from object to screen
void write_obect_lcd(object obj);
// Write bg colour over bounding box defined by obj
void write_bg(object obj);
void write_ball(object ball);

// Write the values from the score object to the LED
void write_score(score sc);

// Graham: Ball, score
// Nattie: player, opponent, pot, lcd	
