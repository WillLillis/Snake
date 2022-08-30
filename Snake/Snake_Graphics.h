#pragma once
#include <stdio.h>
#include "Misc.h"

/*
*
* We're using a 1-D array of chars as our "frame buffer"
* We need to translate from x-y coordinates to the index in that buffer
* To be clear, we'll use the following as our coordinate system
*	(0, y_len - 1)     (x_len - 1, y_len - 1)
*	 __________________
*	|                  |
*	|                  |
*	|                  |
* 	|                  |
*	|                  |
*	|                  |
*	|__________________|
* (0,0)				  (x_len - 1 - 1, 0)
* 
* Important note, the x_len - 1 column is reserved for '\n' chars, the
* game should not access it, it's last column should only be x_len - 1
* 
*/

#define X_Y_TO_INDEX(x, y, x_len, y_len)	((y_len - 1 - y) * x_len) + x
#define BORDER_CHAR							0xDB
#define SNAKE_HEAD_CHAR						'+'
#define SNAKE_BODY_CHAR						'='
#define APPLE_CHAR							'*'
#define SPACE_CHAR							32

typedef struct display_game_args{
	bool game_over; // starts as false->true to signal to stop updating
	BOARD board;
	SNAKE_OBJ snake;
}display_game_args;

// draws the current game state to the console
void draw_game(char* frame_buff)
{
	// can maybe implement some optimizations here?
	// if not just a simple printf("%s", frame_buff); call
	printf("%s", frame_buff);
	return;
}

void display_gameover(char* frame_buff)
{

}

// driver function for displaying the game, to be used to create the graphics thread from the main thread
void* display_game(const display_game_args* argptr)
{
	// frame buffer...
	size_t x_len = argptr->board.x_len + 1;
	size_t y_len = argptr->board.y_len;

	char* frame_buff = (char*)malloc((x_len * y_len + 1) * sizeof(char)); // + 1 or \0 at end

	if (frame_buff == NULL)
	{
		clear_screen();
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Error occured while preparing display resources.");
		return NULL;
	}

	// initialize everything to a space, except the ends of the rows
	for (size_t y = 0; y < y_len; y++)
	{
		for (size_t x = 0; x < x_len - 1; x++)
		{
			frame_buff[X_Y_TO_INDEX(x,y, x_len, y_len)] = SPACE_CHAR;
		}
		frame_buff[X_Y_TO_INDEX(x_len - 1, y, x_len + 1, y_len)] = '\n';
	}
	frame_buff[x_len * y_len + 1] = '\0';

	// fill in borders here, only needs to happen once
	// top and bottom rows
	for (size_t curr_col = 0; curr_col < x_len - 1; curr_col++)
	{
		//frame_buff[curr_row] = BORDER_CHAR; // top row
		//frame_buff[((y_len - 1) * x_len) + curr_row] = BORDER_CHAR; // bottom row
		frame_buff[X_Y_TO_INDEX(curr_col, y_len - 1, x_len, y_len)] = BORDER_CHAR; // top row
		frame_buff[X_Y_TO_INDEX(curr_col, 0, x_len, y_len)] = BORDER_CHAR; // bottom row
	}

	// left and right sides
	for (size_t curr_row = 0; curr_row < y_len; curr_row++)
	{
		//frame_buff[curr_row * x_len] = 0xDB; // left side
		//frame_buff[(curr_row * x_len) + x_len - 1] = 0xDB; // right side
		frame_buff[X_Y_TO_INDEX(0, curr_row, x_len, y_len)] = BORDER_CHAR; // left side
		frame_buff[X_Y_TO_INDEX(x_len - 1 - 1, curr_row, x_len, y_len)] = BORDER_CHAR; // right side
	}

	clear_screen();

	while (!(argptr->game_over))
	{
		// need some sort of lock here?
			// reorganize so that the SNAKE struct is inside of the BOARD struct
			// add a mutex member into the board struct, make it so you have to have the lock to access the struct
				// sync between graphics thread and updating the game
		// snake head
		frame_buff[X_Y_TO_INDEX(argptr->snake.head_loc.x_coor, argptr->snake.head_loc.y_coor, x_len, y_len)] = SNAKE_HEAD_CHAR;
		// snake body
		for (size_t curr_seg = argptr->snake.tail_index; curr_seg <= argptr->snake.first_seg_index; curr_seg = (curr_seg != 0) ? curr_seg - 1 : MAX_SNAKE_LEN - 1)
		{
			frame_buff[X_Y_TO_INDEX(argptr->snake.body[curr_seg].loc.x_coor, argptr->snake.body[curr_seg].loc.y_coor, x_len, y_len)] = SNAKE_BODY_CHAR;
		}
		// apple
		frame_buff[X_Y_TO_INDEX(argptr->board.apple_loc.x_coor, argptr->board.apple_loc.y_coor, x_len, y_len)] = APPLE_CHAR;

		// and then display it to the screen
		draw_game(frame_buff);
	}

	display_gameover(frame_buff);

	if (frame_buff != NULL)
	{
		free(frame_buff);
	}
	return NULL; 
}

