#pragma once
#include <stdio.h>
#include <memory.h>
#include "Snake_Game.h"
#include "Misc.h"

/*
*
* We're using a 1-D array of chars as our "frame buffer"
* We need to translate from x-y coordinates to the index in that buffer
* To be clear, we'll use the following as our coordinate system
*	(0, y_len - 1)     (x_len - 1 - 1, y_len - 1)
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
* The actual buffer starts at (0, y_len - 1)
* 
* There's also one additional "block" at the end of the buffer only to hold
* the NULL terminator '\0'. It can just be accessed normally via index, but 
* its coordinate to index would be X_Y_TO_INDEX(x_len, 0, x_len, y_len)
* 
* Important note, the x_len - 1 column is reserved for '\n' chars, the
* game should not access it, it's last column should only be x_len - 1
* 
* Need to rethink how we're managing the frame buffer, this is already 
* getting fairly convoluted
* 
*/

#define X_Y_TO_INDEX(x, y, x_len, y_len)	((y_len - 1 - (y)) * x_len) + x
#define BORDER_CHAR							0xDB // ASCII white square character
#define SNAKE_HEAD_CHAR						'+'
#define SNAKE_BODY_CHAR						'='
#define APPLE_CHAR							'*'
#define SPACE_CHAR							' '

typedef struct display_game_args{
	bool game_over; // starts as false->true to signal to stop updating
	BOARD* board;
	char* frame_buff_out;
}display_game_args;

bool graphics_init(display_game_args* argptr, BOARD* board_ptr) 
{
	if (argptr == NULL || board_ptr == NULL)
	{
		return false;
	}

	argptr->game_over = false;
	argptr->board = board_ptr;
	argptr->frame_buff_out = NULL;
	return true;
}

// draws the current game state to the console
void draw_game(char* frame_buff)
{
	// can maybe implement some optimizations here?
	printf("%s", frame_buff);
	return;
}

void display_gameover(char* frame_buff)
{

}

// driver function for displaying the game, to be used to create the graphics thread from the main thread
// must be called AFTER other init functions (SNAKE_OBJ struct must already be initialized)
void* display_game(display_game_args* argptr)
{
	// frame buffer...
	size_t x_len = argptr->board->x_len + 1; // + 1 needed to store '\n' chars at end of each line
	size_t y_len = argptr->board->y_len;

	char* frame_buff = (char*)malloc((x_len * y_len + 1) * sizeof(char)); // + 1 for \0 at end of the buffer

	if (frame_buff == NULL)
	{
		clear_screen();
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Error occured while preparing graphics resources.");
		return NULL;
	}

	memset(frame_buff, SPACE_CHAR, x_len * y_len * sizeof(char));
	frame_buff[x_len * y_len] = '\0';
	for (size_t i = 1; i < y_len; i++)
	{
		frame_buff[i * x_len - 1] = '\n';
	}

	// fill in borders here, only needs to happen once
	// top and bottom rows
	for (size_t curr_col = 0; curr_col < x_len - 1; curr_col++)
	{
		frame_buff[X_Y_TO_INDEX(curr_col, y_len - 1, x_len, y_len)] = BORDER_CHAR; // top row
		frame_buff[X_Y_TO_INDEX(curr_col, 0, x_len, y_len)] = BORDER_CHAR; // bottom row
	}

	// left and right sides
	for (size_t curr_row = 0; curr_row < y_len; curr_row++)
	{
		frame_buff[X_Y_TO_INDEX(0, curr_row, x_len, y_len)] = BORDER_CHAR; // left side
		frame_buff[X_Y_TO_INDEX(x_len - 1 - 1, curr_row, x_len, y_len)] = BORDER_CHAR; // right side
	}

	// draw initial snake body here as well...
	for (size_t curr_seg = argptr->board->snake.tail_index; curr_seg >= argptr->board->snake.first_seg_index; curr_seg = (curr_seg != 0) ? curr_seg - 1 : MAX_SNAKE_LEN - 1)
	{
		size_t test_index = X_Y_TO_INDEX(argptr->board->snake.body[curr_seg].loc.x_coor, argptr->board->snake.body[curr_seg].loc.y_coor, x_len, y_len);
		frame_buff[X_Y_TO_INDEX(argptr->board->snake.body[curr_seg].loc.x_coor, argptr->board->snake.body[curr_seg].loc.y_coor, x_len, y_len)] = SNAKE_BODY_CHAR;
	}

	argptr->frame_buff_out = frame_buff; // give the caller access to the frame buffer now that we're done initializing it

	clear_screen();

	size_t old_tail_index;
	uint_fast32_t board_height = argptr->board->y_len;

	printf("\033[?25l"); // hides the cursor
	printf("\n%s", frame_buff);

	while (!(argptr->game_over))
	{
		pthread_mutex_lock(&(argptr->board->game_state_lock));

		// apple
		//frame_buff[X_Y_TO_INDEX(argptr->board->apple_loc.x_coor, argptr->board->apple_loc.y_coor, x_len, y_len)] = APPLE_CHAR;
		//printf("\033[%u;%uH", board_height + 1 - argptr->board->apple_loc.y_coor, argptr->board->apple_loc.x_coor); // columns line up, need to flip line numbers...
		set_cursor_position(argptr->board->apple_loc.x_coor, argptr->board->apple_loc.y_coor, board_height + 1);
		printf("%c", APPLE_CHAR);

		// snake head
		//frame_buff[X_Y_TO_INDEX(argptr->board->snake.head_loc.x_coor, argptr->board->snake.head_loc.y_coor, x_len, y_len)] = SNAKE_HEAD_CHAR;
		//printf("\033[%u;%uH", board_height + 1 - argptr->board->snake.head_loc.y_coor, argptr->board->snake.head_loc.x_coor);
		set_cursor_position(argptr->board->snake.head_loc.x_coor, argptr->board->snake.head_loc.y_coor, board_height + 1);
		printf("%c", SNAKE_HEAD_CHAR);
		
		// snake body
		//frame_buff[X_Y_TO_INDEX(argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.x_coor,
		//	argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.y_coor, x_len, y_len)] = SNAKE_BODY_CHAR;
		/*printf("\033[%u;%uH", 
			board_height + 1 - argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.y_coor,
			argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.x_coor);*/
		set_cursor_position(argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.x_coor, 
			argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.y_coor, board_height + 1);
		printf("%c", SNAKE_BODY_CHAR);

		// clear spot where end of tail used to be
		// issues here with uninitialized memory on first pass through
		old_tail_index = argptr->board->snake.tail_index == MAX_SNAKE_LEN ? 0 : argptr->board->snake.tail_index + 1;
		if ((argptr->board->snake.body[old_tail_index].used))
		{
			//frame_buff[X_Y_TO_INDEX(argptr->board->snake.body[old_tail_index].loc.x_coor,
			//	argptr->board->snake.body[old_tail_index].loc.y_coor, x_len, y_len)] = SPACE_CHAR;
			//printf("\033[%u;%uH", board_height + 1 - argptr->board->snake.body[old_tail_index].loc.y_coor, argptr->board->snake.body[old_tail_index].loc.x_coor);
			set_cursor_position(argptr->board->snake.body[old_tail_index].loc.x_coor, 
				argptr->board->snake.body[old_tail_index].loc.y_coor, board_height);
			printf("%c", SPACE_CHAR);
		}
		
		pthread_mutex_unlock(&(argptr->board->game_state_lock));
	}
	printf("\033[?25h"); // un-hide cursor
	printf("\033[%u;%uH", board_height + 1, argptr->board->x_len + 1);

	display_gameover(frame_buff);

	if (frame_buff != NULL)
	{
		free(frame_buff);
	}
	return NULL; 
}

