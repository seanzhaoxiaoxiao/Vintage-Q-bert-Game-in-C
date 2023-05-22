#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define BLACK 0x0000
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define DARK_BLUE 0x0AD3
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BURGANDY 0x8080
	
/* Global values*/
int MAPSTART_X = 159;
int MAPSTART_Y = 9;
int landedcubes[28];
int currentcube = 0;
int numberofslime = 0;
int enemyslime1 = 10;
int enemyslime2 = 11;
int initializeslimecounter = 0;
bool WIN;
bool DIE;
bool slime1on = false;
bool slime2on = false;
bool finalrefresh = true;


/* Starting position of player */
int startX = 151;
int startY = 4;
int sqstartX = 159;
int sqstartY = 9;
int greenstartX = 152;
int greenstartY = 6;

bool enemymove = false;

// starting pixel buffer
volatile int pixel_buffer_start; 

//Simple function to store the pixel at (x, y) into pixel buffer
void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//function two swap values of two variables
void swap(int *x, int *y){
	int temp = 0;
	temp = *x;
	*x = *y;
	*y = temp;
}

// Bresenham’s algorithm for drawing lines and also calling swap and plot_pixel
void draw_line(int x0, int y0, int x1, int y1, short int color){
	bool is_steep = false;	
	if(abs(y1 - y0) > abs(x1 - x0)){
		is_steep = true;
	}

	
	if(is_steep){
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	
	if (x0 > x1){
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
		
	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	int error = -(deltax / 2);
	int y = y0;
	int y_step;
	
	if(y0 < y1){ 
		y_step = 1;
	}
	else{
		y_step = -1;
	}
	
	for(int x = x0; x < x1; x++){
		if(is_steep){ 
			plot_pixel(y, x, color);
		}
		else{
			plot_pixel(x, y, color);			
		}
		
		error = error + deltay;
			
		if(error > 0){
			y = y + y_step;
			error = error - deltax;
		}		
	}
}

// function to draw a cube
void draw_3dcude(int X, int Y) {
	int x0 = X - 15;
	int x1 = X;
	int y0 = Y + 10;
	int y1 = Y;
	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	int error = -(deltax / 2);
	int y = y0;
	int y_step;
	
	//draw upper blue surface
	if(y0 < y1){ 
		y_step = 1;
	}
	else{
		y_step = -1;
	}
	for(int x = x0; x < x1; x++){
		draw_line(x, y, x+15, y+10, BLUE);		
		
		error = error + deltay;
			
		if(error > 0){
			y = y + y_step;
			error = error - deltax;
		}		
	}
	
	//draw lower left grey surface
	for(int tempy = Y+10; tempy <= Y+30; tempy++){
		draw_line(X-15, tempy, X, tempy+10, GREY);		
	}
	//draw lower right grey surface
	for(int tempy = Y+20; tempy <= Y+40; tempy++){
		draw_line(X, tempy, X+15, tempy-10, DARK_BLUE);		
	}
}
// function to draw the Qbert map
void draw_qbert_map_3d() {
	int X = MAPSTART_X;
	int Y = MAPSTART_Y;
    // loop through each row of cubes
    for (int row = 0; row < 7; row++) {
        // loop through each column of cubes
        for (int col = 0; col <= row; col++) {
			Y = MAPSTART_Y + (row * 30);
			if (row % 2 == 0) {
				X = MAPSTART_X + ((col - (row/2)) * 30);
			} else{
				X = MAPSTART_X + ((col - ((row + 1) / 2)) * 30) + 15;
			}
			draw_3dcude(X, Y);
		}
	}
}

// Test if qbert has been to all cubes
bool if_win(int color){
	int win_con = 0;
	for(int x = 60; x < 360; x++){
		for(int y = 40; y < 220; y++){
			if(*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) == color)
				win_con = 1;
			else win_con = 0;
		}
	}
	
	if(win_con == 0)
		return false;
	else
		return true;
}

//clear the screen
void clear_screen(){
	for(int x = 0; x <= 319; x++){
		for(int y = 0; y <= 239; y++){
			plot_pixel(x, y, 0x0);
		}
	}
}

// wait for video to be synchronized
void wait_for_vsync(){
	volatile int* pixel_ctrl_ptr = (int*) 0xFF203020; // pixel controller
	register int status;
	
	*pixel_ctrl_ptr = 1;
	
	status = *(pixel_ctrl_ptr + 3);
	while((status& 0x01) != 0){
		status = *(pixel_ctrl_ptr + 3);
	}
}

void drawbox(int x, int y, int color){
	for(int i = 0; i < 8; i++){
		for(int j = 0; j < 8; j++){
			plot_pixel(x + i, y + j, color);
		}
	}	
}


void draw_qbert(){
	int row = currentcube / 10;
	int col = currentcube  % 10;
	int y  = startY + (30 * row);
	int x = startX - (15 * row) + (30 * col);
	//draw red pixel
	for (int i = 4; i < 8; i++){
		plot_pixel(x + 1, y + i, RED);
	}
	for (int i = 2; i < 9; i++){
		plot_pixel(x + 2, y + i, RED);
	}
	plot_pixel(x + 3, y + 2, RED);
	plot_pixel(x + 3, y + 4, RED);
	plot_pixel(x + 3, y + 6, RED);
	plot_pixel(x + 3, y + 8, RED);
	plot_pixel(x + 3, y + 9, RED);
	for (int i = 7; i < 14; i++){
		plot_pixel(x + 4, y + i, RED);
	}
	plot_pixel(x + 4, y + 15, RED);
	for (int i = 2; i < 5; i++){
		plot_pixel(x + 5, y + i, RED);
	}
	for (int i = 8; i < 11; i++){
		plot_pixel(x + 5, y + i, RED);
	}
	for (int i = 7; i < 11; i++){
		plot_pixel(x + 6, y + i, RED);
	}
	plot_pixel(x + 6, y + 1, RED);
	plot_pixel(x + 6, y + 5, RED);
	for (int i = 9; i < 11; i++){
		plot_pixel(x + 7, y + i, RED);
	}
	plot_pixel(x + 7, y + 1, RED);
	plot_pixel(x + 7, y + 5, RED);
	for (int i = 1; i < 5; i++){
		plot_pixel(x + 8, y + i, RED);
	}
	for (int i = 7; i < 15; i++){
		plot_pixel(x + 8, y + i, RED);
	}
	plot_pixel(x + 9, y + 1, RED);
	plot_pixel(x + 9, y + 8, RED);
	plot_pixel(x + 10, y + 1, RED);
	plot_pixel(x + 10, y + 7, RED);
	for (int i = 7; i < 9; i++){
		plot_pixel(x + 11, y + i, RED);
	}
	for (int i = 7; i < 10; i++){
		plot_pixel(x + 12, y + i, RED);
	}
	plot_pixel(x + 13, y + 10, RED);
	plot_pixel(x + 14, y + 10, RED);
	plot_pixel(x + 15, y + 9, RED);
	
	//draw orange pixel
	for (int i = 5; i < 9; i++){
		plot_pixel(x + i, y, ORANGE);
	}
	for (int i = 3; i < 6; i++){
		plot_pixel(x + i, y + 1, ORANGE);
	}
	plot_pixel(x + 4, y + 2, ORANGE);
	plot_pixel(x + 3, y + 3, ORANGE);
	plot_pixel(x + 4, y + 3, ORANGE);
	plot_pixel(x + 4, y + 4, ORANGE);
	plot_pixel(x + 11, y + 4, ORANGE);
	for (int i = 3; i < 6; i++){
		plot_pixel(x + i, y + 5, ORANGE);
	}
	for (int i = 8; i < 14; i++){
		plot_pixel(x + i, y + 5, ORANGE);
	}
	for (int i = 4; i < 15; i++){
		plot_pixel(x + i, y + 6, ORANGE);
	}
	plot_pixel(x + 3, y + 7, ORANGE);
	plot_pixel(x + 5, y + 7, ORANGE);
	plot_pixel(x + 7, y + 7, ORANGE);
	plot_pixel(x + 9, y + 7, ORANGE);
	for (int i = 13; i < 16; i++){
		plot_pixel(x + i, y + 7, ORANGE);
	}
	plot_pixel(x + 7, y + 8, ORANGE);
	plot_pixel(x + 15, y + 8, ORANGE);
	plot_pixel(x + 3, y + 14, ORANGE);
	plot_pixel(x + 4, y + 14, ORANGE);
	plot_pixel(x + 5, y + 14, ORANGE);
	plot_pixel(x + 9, y + 14, ORANGE);
	plot_pixel(x + 10, y + 14, ORANGE);
	plot_pixel(x + 5, y + 15, ORANGE);
	plot_pixel(x + 6, y + 15, ORANGE);
	plot_pixel(x + 7, y + 15, ORANGE);
	plot_pixel(x + 11, y + 15, ORANGE);
	plot_pixel(x + 12, y + 15, ORANGE);
	
	//draw white pixel
	plot_pixel(x + 6, y + 2, WHITE);
	plot_pixel(x + 7, y + 2, WHITE);
	plot_pixel(x + 9, y + 2, WHITE);
	plot_pixel(x + 10, y + 2, WHITE);
	
	//draw black pixel
	plot_pixel(6, 3, BLACK);
	plot_pixel(7, 3, BLACK);
	plot_pixel(9, 3, BLACK);
	plot_pixel(10, 3, BLACK);
	plot_pixel(6, 4, BLACK);
	plot_pixel(7, 4, BLACK);
	plot_pixel(9, 4, BLACK);
	plot_pixel(10, 4, BLACK);
	plot_pixel(13, 8, BLACK);
	plot_pixel(14, 8, BLACK);
	plot_pixel(13, 9, BLACK);
	plot_pixel(14, 9, BLACK);
}

void drawSquare(int location){
	int row = location / 10;
	int col = location  % 10;
	int Y  = sqstartY + (30 * row);
	int X = sqstartX - (15 * row) + (30 * col);
	int x0 = X - 15;
	int x1 = X;
	int y0 = Y + 10;
	int y1 = Y;
	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	int error = -(deltax / 2);
	int y = y0;
	int y_step;
	
	//draw upper blue surface
	if(y0 < y1){ 
		y_step = 1;
	}
	else{
		y_step = -1;
	}
	for(int x = x0; x < x1; x++){
		draw_line(x, y, x+15, y+10, YELLOW);		
		
		error = error + deltay;
			
		if(error > 0){
			y = y + y_step;
			error = error - deltax;
		}		
	}
}

void draw_all_square(){
	for (int i = 0; i < 28; i++){
		if (landedcubes[i] == -2){
			continue;
		} else {
			int location = landedcubes[i];
			drawSquare(location);
		}
	}
}

void key_wait(){
	while(1){
		unsigned int key_value = *((volatile unsigned int*) KEY_BASE);
		if(!key_value)
			return;
	}
}

void check_bound(){
	int row = currentcube / 10;
	int col = currentcube % 10;
	if (currentcube < 0){
		DIE = true;
	} else if (col == 9){
		DIE = true;
	} else if (col > row){
		DIE = true;
	} else if (row > 6){
		DIE = true;
	}
	
	return;
}

bool check_bound_slime(int enemyslime){
	int row = enemyslime / 10;
	int col = enemyslime % 10;
	if (enemyslime < 0){
		return true;
	} else if (col == 9){
		return true;
	} else if (col > row){
		return true;
	} else if (row > 6){
		return true;
	}
	return false;
}

void update_loc(int row, int col){
	currentcube = currentcube + (row * 10);
	currentcube = currentcube + col;
}

void update_square(){
	for (int i = 0; i < 28; i++){
		if (landedcubes[i] == currentcube){
			return;
		}
	}
	for (int i = 0; i < 28; i++){
		if (landedcubes[i] == -2){
			landedcubes[i] = currentcube;
			break;
		}
	}
	for (int i = 0; i < 28; i++){
		if (landedcubes[i] == -2){
			return;
		}
	}
	WIN = true;
	return;
}

//draws a green slime enemy
void draw_enemy_slime(int slimeIdx){
	int row = 0, col = 0, x = 0, y = 0;
	if (slimeIdx == 1){
		row = enemyslime1 / 10;
		col = enemyslime1  % 10;
		y  = greenstartY + (30 * row);
		x = greenstartX - (15 * row) + (30 * col);
		
	} else if (slimeIdx == 2){
		row = enemyslime2 / 10;
		col = enemyslime2  % 10;
		y  = greenstartY + (30 * row);
		x = greenstartX - (15 * row) + (30 * col);
	}
	for (int i = 4; i < 10; i++){
		plot_pixel(x + i, y, GREEN);
	}
	for (int i = 3; i < 11; i++){
		plot_pixel(x + i, y + 1, GREEN);
	}
	for (int i = 2; i < 12; i++){
		plot_pixel(x + i, y + 2, GREEN);
	}
	for (int i = 1; i < 13; i++){
		plot_pixel(x + i, y + 3, GREEN);
	}
	for (int i = 0; i < 14; i++){
		plot_pixel(x + i, y + 4, GREEN);
	}
	for (int i = 0; i < 14; i++){
		plot_pixel(x + i, y + 5, GREEN);
	}
	for (int i = 0; i < 14; i++){
		plot_pixel(x + i, y + 6, GREEN);
	}
	for (int i = 0; i < 14; i++){
		plot_pixel(x + i, y + 7, GREEN);
	}
	for (int i = 0; i < 14; i++){
		plot_pixel(x + i, y + 8, GREEN);
	}
	for (int i = 1; i < 13; i++){
		plot_pixel(x + i, y + 9, GREEN);
	}
	for (int i = 2; i < 12; i++){
		plot_pixel(x + i, y + 10, GREEN);
	}
	for (int i = 3; i < 11; i++){
		plot_pixel(x + i, y + 11, GREEN);
	}
	for (int i = 4; i < 10; i++){
		plot_pixel(x + i, y + 12, GREEN);
	}
	plot_pixel(x + 4, y + 3, WHITE);
	plot_pixel(x + 5, y + 3, WHITE);
	plot_pixel(x + 3, y + 4, WHITE);
	plot_pixel(x + 4, y + 4, WHITE);
	plot_pixel(x + 3, y + 5, WHITE);
}

//run all functions related to the two green slime enemy
void enemy_slime(){
	//determine the movement of the slimes
	if (enemymove){
		if (slime1on){
			if (rand() % 2 == 1){
				enemyslime1 += 10;
				enemymove = false;
			} else{
				enemyslime1 += 11;
				enemymove = false;
			}
		}
		if (slime2on){
			if (rand() % 2 == 1){
				enemyslime2 += 10;
				enemymove = false;
			} else{
				enemyslime2 += 11;
				enemymove = false;
			}
		}
	}
	
	//determine which slime to display
	if (!slime1on && !slime2on){
		if (initializeslimecounter == 3){
			draw_enemy_slime(1);
			initializeslimecounter = 0;
			slime1on = true;
		}
	} else if (slime1on && !slime2on){
		draw_enemy_slime(1);
		if (initializeslimecounter == 3){
			draw_enemy_slime(2);
			initializeslimecounter = 0;
			slime2on = true;
		}
	} else if (!slime1on && slime2on){
		draw_enemy_slime(2);
		if (initializeslimecounter == 3){
			draw_enemy_slime(1);
			initializeslimecounter = 0;
			slime1on = true;
		}
	} else if (slime1on && slime2on){
		draw_enemy_slime(1);
		draw_enemy_slime(2);
	}
	
	//check if the slime is out of bound
	if (slime1on){
		if (check_bound_slime(enemyslime1)){
			slime1on = false;
			enemyslime1 = 10;
		}
	}
	if (slime2on){
		if (check_bound_slime(enemyslime2)){
			slime2on = false;
			enemyslime2 = 11;
		}
	}
	
	//check if qbert ran into a slime
	if (slime1on){
		if (currentcube == enemyslime1){
			DIE = true;
		}
	}
	if (slime2on){
		if (currentcube == enemyslime2){
			DIE = true;
		}
	}
}

// draws a big green check mark across the screen
void draw_checkmark(){
	int lftx0 = 55;
	int lfty0 = 105;
	int lftx1 = 125;
	int lfty1 = 205;
	int rightx0 = 125;
	int righty0 = 205;
	int rightx1 = 245;
	int righty1 = 65;
	for(int i = 0; i < 15; i++){
		draw_line(lftx0, lfty0, lftx1, lfty1, GREEN);
		lftx0++;
		lftx1++;
	}

	for(int i = 0; i < 15; i++){
		draw_line(rightx0, righty0, rightx1, righty1, GREEN);
		rightx0++;
		rightx1++;
	}
}

//reset all global variables
void resetglobal(){
	MAPSTART_X = 159;
	MAPSTART_Y = 9;
	//initialize landedcubes array
	for (int i = 0; i < 28; i++){
		landedcubes[i] = -2;
	}
	currentcube = 0;
	numberofslime = 0;
	enemyslime1 = 10;
	enemyslime2 = 11;
	initializeslimecounter = 0;
	slime1on = false;
	slime2on = false;
	enemymove = false;
	finalrefresh = true;
}
// draws a big cross
void draw_cross(int color){
	int lftx0 = 60;
	int lfty0 = 40;
	int lftx1 = 245;
	int lfty1 = 205;
	int rightx0 = 245;
	int righty0 = 40;
	int rightx1 = 60;
	int righty1 = 205;
	for(int i = 0; i < 15; i++){
		draw_line(lftx0, lfty0, lftx1, lfty1, color);
		lftx0++;
		lftx1++;
	}

	for(int i = 0; i < 15; i++){
		draw_line(rightx0, righty0, rightx1, righty1, color);
		rightx0++;
		rightx1++;
	}
}

void HEX_DISPLAY(char b1, char b2, char b3) {
	volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
	volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
	/* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
	* a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
	*/
	unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
	0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
	unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int shift_buffer, nibble;
	unsigned char code;
	int i;
	shift_buffer = (b1 << 16) | (b2 << 8) | b3;
	for (i = 0; i < 6; ++i) {
		nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
		code = seven_seg_decode_table[nibble];
		hex_segs[i] = code;
		shift_buffer = shift_buffer >> 4;
	}
	/* drive the hex displays */
	*(HEX3_HEX0_ptr) = *(int *)(hex_segs);
	*(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}

// main function
int main() {
	//set-up the timer
	volatile int * timer_ptr = (int *)0xFFFEC600;
	*timer_ptr = 500000000;
	*(timer_ptr + 2) = 3;
	//initialize landedcubes array
	for (int i = 0; i < 28; i++){
		landedcubes[i] = -2;
	}
    // initialize the VGA screen
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	*(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;// pixel_buffer_start points to the pixel buffer
	clear_screen();
	*(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	
	/* Set up the ps2 keyboard input*/
	volatile int * PS2_ptr = (int *)0xFF200100;
	int PS2_data, RVALID;
	char byte1 = 0, byte2 = 0, byte3 = 0;
	// PS/2 mouse needs to be reset (must be already plugged in)
	*(PS2_ptr) = 0xFF; // reset
	
    // draw loop
    while (1) {
		// Connect the timer
		unsigned int timer_value = *((volatile unsigned int*) timer_ptr + 3);
		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		RVALID = PS2_data & 0x8000; // extract the RVALID field
		
		if (timer_value == 1){
			enemymove = true;
			initializeslimecounter++;
			*(timer_ptr + 3) = timer_value;
		}
		
		//check for end program conditions
		if (WIN || DIE){
			clear_screen();
			draw_qbert_map_3d();
			draw_all_square();
			draw_qbert();
			if (WIN){
				draw_checkmark();
				HEX_DISPLAY(0x66, 0x66, 0x66);
			} else if (DIE){
				draw_cross(RED);
				HEX_DISPLAY(0x00, 0xFA, 0x11);
			}
			wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
			if (RVALID) {
				/* shift the next data byte into the display */
				byte1 = byte2;
				byte2 = byte3;
				byte3 = PS2_data & 0xFF;
				if ((byte2 == (char)0xAA) && (byte3 == (char)0x00))
					*(PS2_ptr) = 0xF4;
				if (byte2 == 0xF0 && byte3 == 0x5A){
					WIN = false;
					DIE = false;
					resetglobal();
				}
				continue;
			} else {
				continue;
			}
		}
		
		// if statement to control program behavior based on key value	
		if (RVALID) {
			/* shift the next data byte into the display */
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;
			if ((byte2 == (char)0xAA) && (byte3 == (char)0x00))
				*(PS2_ptr) = 0xF4;
			//move to right
			if (byte1 == 0xE0 && byte2 == 0xF0 && byte3 == 0x74){
				update_loc(1, 1);
				update_square();
				check_bound();
				clear_screen();
				draw_qbert_map_3d();
				draw_all_square();
				draw_qbert();
			} else if (byte1 == 0xE0 && byte2 == 0xF0 && byte3 == 0x75){		//move up
				update_loc(-1, 0);
				update_square();
				check_bound();
				clear_screen();
				draw_qbert_map_3d();
				draw_all_square();
				draw_qbert();
			} else if (byte1 == 0xE0 && byte2 == 0xF0 && byte3 == 0x72){		//move down
				update_loc(1, 0);
				update_square();
				check_bound();
				clear_screen();
				draw_qbert_map_3d();
				draw_all_square();
				draw_qbert();
			} else if (byte1 == 0xE0 && byte2 == 0xF0 && byte3 == 0x6B){		//move to the left
				update_loc(-1, -1);
				update_square();
				check_bound();
				clear_screen();
				draw_qbert_map_3d();
				draw_all_square();
				draw_qbert();
			} else{
				clear_screen();
				draw_qbert_map_3d();
				draw_all_square();
				draw_qbert();
			}
		} else {
			clear_screen();
			draw_qbert_map_3d();
			draw_all_square();
			draw_qbert();
		}
		
		//run the slime function
		enemy_slime();
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
	return 0;
}
