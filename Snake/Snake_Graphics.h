#pragma once
#include "Snake_Game.h"
#include "Misc.h"
#include <stdio.h>
#include <memory.h>


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
* The board's x_len is different from the graphics x_len. This is definitely a bit 
* of a confusing point but I'd rather not go through and reorganize everything since 
* it's working
*	- graphics x_len = board x_len + 1, to account for the '\n' character
* 
* Might need to rethink how we're managing the frame buffer, this is already 
* getting fairly convoluted
* 
*/

#define X_Y_TO_INDEX(x, y, x_len, y_len)	((y_len - 1 - (y)) * x_len) + x
#define BORDER_CHAR							0xDB // ASCII white square character
//#define SNAKE_HEAD_CHAR					'+'
#define SNAKE_HEAD_UP						'^'
#define SNAKE_HEAD_DOWN						'v'
#define SNAKE_HEAD_RIGHT					'>'
#define SNAKE_HEAD_LEFT						'<'
#define SNAKE_BODY_CHAR						'='
#define APPLE_CHAR							'*'
#define SPACE_CHAR							' '

typedef struct display_game_args{
	bool thread_running;
	bool game_over; // starts as false->true to signal to stop updating
	BOARD* board;
	USER_INPUT* user_input;
	char* frame_buff_out;
}display_game_args;

bool graphics_init(display_game_args* argptr, BOARD* board_ptr, USER_INPUT* user_input_ptr)
{
	if (argptr == NULL || board_ptr == NULL)
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Invalid memory address(es) passed to function.");
		return false;
	}
	argptr->thread_running = false;
	argptr->game_over = false;
	argptr->board = board_ptr;
	argptr->user_input = user_input_ptr;
	argptr->frame_buff_out = NULL;
	return true;
}

// used to draw the initial board
void draw_game(char* frame_buff)
{
	printf("%s", frame_buff);
	return;
}

// displays a game over message in the middle of the board
void display_gameover(char* frame_buff)
{
	// set cursor to middle of board
	set_cursor_position(BOARD_LEN_X / 2 - 5, BOARD_LEN_Y / 2, BOARD_LEN_Y); // -5 to center the message being printed (10 characters)
	printf("GAME OVER!");
	set_cursor_position(BOARD_LEN_X + 1, BOARD_LEN_Y + 1, BOARD_LEN_Y);
	Sleep(500);
}

// driver function for displaying the game, to be used to create the graphics thread from the main thread
// must be called AFTER other init functions (SNAKE_OBJ struct must already be initialized)
void* display_game(display_game_args* argptr)
{
	argptr->thread_running = true;
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
	for (size_t curr_seg = argptr->board->snake.tail_index; curr_seg >= argptr->board->snake.first_seg_index; curr_seg = (curr_seg != 0) ? curr_seg - 1 : MAX_SNAKE_LEN)
	{
		frame_buff[X_Y_TO_INDEX(argptr->board->snake.body[curr_seg].loc.x_coor, argptr->board->snake.body[curr_seg].loc.y_coor, x_len, y_len)] = SNAKE_BODY_CHAR;
	}

	argptr->frame_buff_out = frame_buff; // give the caller access to the frame buffer now that we're done initializing it

	clear_screen();

	// some variables used in the loop below...
	size_t old_tail_index = MAX_SNAKE_LEN;
	uint_fast32_t board_height = argptr->board->y_len;
	char move_key = KEY_UP;
	char snake_head_char = SNAKE_HEAD_UP;

	printf("\033[?25l"); // hides the cursor
	printf("%s", frame_buff); // puts up the game's walls

	while (!(argptr->game_over))
	{
		// so technically it's bad practice to not use these locks, as we can get outdated information from the game state
		// but realistically this game is relatively slow paced so any issues like that will likely not be noticable
			// taking off the lock seems to help make the game look less "jerky"
		//pthread_mutex_lock(&(argptr->board->game_state_lock));

		// apple
		set_cursor_position(argptr->board->apple_loc.x_coor, argptr->board->apple_loc.y_coor, board_height);
		printf("%c", APPLE_CHAR);

		// snake head
			// doing it this way does allow you to have a head face the opposite way you're going...but I don't feel like fixing that (would just be extra complication)
		set_cursor_position(argptr->board->snake.head_loc.x_coor, argptr->board->snake.head_loc.y_coor, board_height);
		pthread_mutex_lock(&(argptr->user_input->lock));
		move_key = argptr->user_input->move_key;
		pthread_mutex_unlock(&(argptr->user_input->lock));
		switch (move_key) {
		case KEY_UP:
			snake_head_char = SNAKE_HEAD_UP;
			break;
		case KEY_DOWN:
			snake_head_char = SNAKE_HEAD_DOWN;
			break;
		case KEY_LEFT:
			snake_head_char = SNAKE_HEAD_LEFT;
			break;
		case KEY_RIGHT:
			snake_head_char = SNAKE_HEAD_RIGHT;
			break;
		}
		printf("%c", snake_head_char);
		
		// snake body
			// draw the newest body segment where the head just was
		set_cursor_position(argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.x_coor, 
			argptr->board->snake.body[argptr->board->snake.first_seg_index].loc.y_coor, board_height);
		printf("%c", SNAKE_BODY_CHAR);

		// and erase the old body segment at the end of the tail
		old_tail_index = argptr->board->snake.tail_index == MAX_SNAKE_LEN ? 0 : argptr->board->snake.tail_index + 1;
		if (argptr->board->snake.body[old_tail_index].used) // only need to do this if there actually was a segment there 
		{
			set_cursor_position(argptr->board->snake.body[old_tail_index].loc.x_coor, 
				argptr->board->snake.body[old_tail_index].loc.y_coor, board_height);
			printf("%c", (char)SPACE_CHAR);
		}

		// Score
		printf("\033[%u;%zuH", (uint32_t)board_height, x_len + 1);
		printf("Score: %u", argptr->board->score);
		
		//pthread_mutex_unlock(&(argptr->board->game_state_lock));
	}
	//printf("\033[?25h"); // un-hide cursor
	

	display_gameover(frame_buff);

	if (frame_buff != NULL)
	{
		free(frame_buff);
	}
	return NULL; 
}