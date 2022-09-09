#pragma once
#include <ctype.h> //_toupper() is broken without this...
#include <stdbool.h> // C bools
#include <pthread.h> // threads!
#include <stdio.h> // printing and such
#include <conio.h>	// getch()
#include <stdint.h> // cool types
#include <assert.h> // asserts for testing purposes
#include <stdlib.h> // srand() and rand()
#include <time.h> // time() function to seed srand()
#include "Misc.h"

#define NOOP			(void)0

#define KEY_UP			'W'
#define KEY_DOWN		'S'
#define KEY_RIGHT		'D'
#define KEY_LEFT		'A'

// arbitrary board size
#define BOARD_LEN_X		40
#define BOARD_LEN_Y		40
#define MAX_SNAKE_LEN	BOARD_LEN_X * BOARD_LEN_Y + 1 // Needs to be a function of the board size?

#ifndef X_Y_TO_INDEX
	#define X_Y_TO_INDEX(x, y, x_len, y_len)	((y_len - 1 - (y)) * x_len) + x
#endif // !X_Y_TO_INDEX

// struct to manage the user's key inputs
typedef struct USER_INPUT {
	pthread_mutex_t lock;
	char move_key;
}USER_INPUT;

// wrapper struct for USER_INPUT, allows for game_over signaling to the user input thread
typedef struct user_input_loop_args {
	bool game_over;
	USER_INPUT input_store;
}user_input_loop_args;

// struct to hold some 2D coordinates
typedef struct COOR{
	uint_fast8_t x_coor;
	uint_fast8_t y_coor;
}COOR;

// represents a snake body segment (head is managed differently)
typedef struct SNAKE_SEG{
	COOR loc;
	bool used;
}SNAKE_SEG;

// the whole snake
typedef struct SNAKE_OBJ{
	COOR head_loc;
	SNAKE_SEG body[MAX_SNAKE_LEN + 1];
	size_t first_seg_index;
	size_t tail_index;
}SNAKE_OBJ;

// game board, tracking basically everything besides user_input
typedef struct BOARD{
	pthread_mutex_t game_state_lock;
	SNAKE_OBJ snake;
	COOR apple_loc;
	uint32_t score; 
	uint_fast8_t x_len;
	uint_fast8_t y_len;
	char* frame_buff;
}BOARD;

// wrapper struct for an init function
typedef struct game_args{
	user_input_loop_args user_input;
	BOARD board;
}game_args;

// just an init function for the USER_INPUT struct
bool user_input_init(USER_INPUT* struct_in)
{
	if (struct_in == NULL)
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Recieved an invalid memory address for the USER_INPUT* arg");
		return false;
	}
	struct_in->lock = PTHREAD_MUTEX_INITIALIZER;
	int init_success = pthread_mutex_init(&(struct_in->lock), NULL); // initialize struct's lock to default, unlocked state

	if (init_success != 0) // do nothing if the function suceeded, return false otherwise
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Failed to initialize the USER_INPUT struct's mutex lock.");
		return false;
	}
	
	pthread_mutex_lock(&(struct_in->lock));
	struct_in->move_key = KEY_UP;
	pthread_mutex_unlock(&(struct_in->lock));

	return true;
}

// just an init function for the SNAKE_OBJ struct
bool snake_init(SNAKE_OBJ* snake)
{
	// initialize the head location (arbitrary start in the middle of the board)
	snake->head_loc.x_coor = BOARD_LEN_X / 2;
	snake->head_loc.y_coor = BOARD_LEN_Y / 2;;

	// initialize the first body segment
	snake->body[MAX_SNAKE_LEN].loc.x_coor = snake->head_loc.x_coor - 1;
	snake->body[MAX_SNAKE_LEN].loc.y_coor = snake->head_loc.y_coor;
	snake->first_seg_index = MAX_SNAKE_LEN;
	snake->tail_index = MAX_SNAKE_LEN;
	snake->body[MAX_SNAKE_LEN].used = true;

	// initialize rest of the body array to be marked as unused
	for (size_t curr_seg = 0; curr_seg < MAX_SNAKE_LEN; curr_seg++)
	{
		snake->body[curr_seg].used = false;
	}

	return true;
}

// just an init function for the BOARD struct
bool board_init(BOARD* board)
{
	// initialize the game_state lock for the struct
	board->game_state_lock = PTHREAD_MUTEX_INITIALIZER;
	if (pthread_mutex_init(&(board->game_state_lock), NULL) != 0)
	{
		return false;
	}

	// initialize the snake object
	if (!snake_init(&(board->snake)))
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, false,
			"snake_init function returned an error.");
		return false;
	}

	// might as well have these stored in the struct...?
	board->x_len = BOARD_LEN_X;
	board->y_len = BOARD_LEN_Y;

	// initialize first apple in upper right corner (arbitrary)
	board->apple_loc.x_coor = (3 * board->x_len) / 4;
	board->apple_loc.y_coor = (3 * board->y_len) / 4;

	// initialize the score
	board->score = 0;

	// initialize frame_buff pointer to NULL
		// the graphics thread will take care of allocating/ initializing this memory
	board->frame_buff = NULL;

	return true;
}

// main init function
bool game_init(game_args* input_args)
{
	if (input_args == NULL)
	{
		return false;
	}

	// user input stuff
	if (!user_input_init(&(input_args->user_input.input_store)))
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, false,
			"user_input_init function returned an error.");
		return false;
	}
	input_args->user_input.game_over = false;

	// seed the random number generator for our apple locations
	srand((unsigned int)time(NULL)); 

	// initialize the board struct, which in turn also initializes the snake struct within
	if (!board_init(&(input_args->board)))
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, false,
			"board_init function returned an error.");
		return false;
	}

	return true;
}

// grabs the latest stored user input key
char get_user_input(user_input_loop_args* argptr)
{
	pthread_mutex_lock(&(argptr->input_store.lock));
	char ret_key = argptr->input_store.move_key;
	pthread_mutex_unlock(&(argptr->input_store.lock));
	return ret_key;
}

// main loop for getting the user's input, spins the entire game until the game_over flag is set
void* user_input_loop(user_input_loop_args* argptr)
{
	char user_key = KEY_UP;
	while (!(argptr->game_over))
	{
		user_key = _toupper(_getch());
		// need to be careful here...make sure other thread only copies value when it has the lock and then gives it back up immediately after
		pthread_mutex_lock(&(argptr->input_store.lock));
		if (user_key == KEY_UP || user_key == KEY_DOWN
			|| user_key == KEY_RIGHT || user_key == KEY_LEFT)
		{
			argptr->input_store.move_key = user_key;
		}
		pthread_mutex_unlock(&(argptr->input_store.lock));
	}

	return NULL; 
}

// indicates whether the supplied coordinates are inside of the snake's body (NOT HEAD!)
bool is_inside_snake(const SNAKE_OBJ* snake, const COOR coor)
{
	// need to take another look here
	size_t tail_index = snake->tail_index;
	size_t first_seg_index = snake->first_seg_index;
	if (tail_index == first_seg_index) // edge case, just look at one block
	{
		if((snake->body[tail_index].loc.x_coor == coor.x_coor)
			&& (snake->body[tail_index].loc.y_coor == coor.y_coor))
		{
			return true;
		}
		return false;
	}
	// regular case where we need to check multiple segments
	for (size_t curr_seg_index = tail_index; curr_seg_index != first_seg_index; 
		curr_seg_index = ((curr_seg_index == 0) ? MAX_SNAKE_LEN : curr_seg_index - 1))
	{
		if ((snake->body[curr_seg_index].loc.x_coor == coor.x_coor) // not sure if just testing equality with the COOR structs will work here
			&& (snake->body[curr_seg_index].loc.y_coor == coor.y_coor))
		{
			return true;
		}
	}
	// need to explicitly check for collisions with the first index now
	if ((snake->body[first_seg_index].loc.x_coor == coor.x_coor)
		&& (snake->body[first_seg_index].loc.y_coor == coor.y_coor))
	{
		return true;
	}

	return false;
}

// main function updating the game as time progresses
bool update_game_state(BOARD* board, const char snake_dir, const char* frame_buff)
{
	bool ate_apple = false;
	// trying without the game_state_lock, see display_game() comment for justification
	//pthread_mutex_lock(&(board->game_state_lock));
	// save the old head location before we update it
	COOR old_head_loc = { .x_coor = board->snake.head_loc.x_coor,.y_coor = board->snake.head_loc.y_coor };

	// then update the head's position
	switch (snake_dir) {
	case KEY_UP:
		board->snake.head_loc.y_coor++;
		break;
	case KEY_DOWN:
		board->snake.head_loc.y_coor--;
		break;
	case KEY_RIGHT:
		board->snake.head_loc.x_coor++;
		break;
	case KEY_LEFT:
		board->snake.head_loc.x_coor--;
		break;
	}

	COOR curr_head_loc = board->snake.head_loc;
	
	// now check if the snake's head collided with any of the walls, or itself
	// some fudge factors used here for correct collision detection with walls, will investigate later...
		// just have the game print out the head's coordinates as it plays...see what's what in real time
	if (curr_head_loc.x_coor == 1 || curr_head_loc.x_coor == (board->x_len) 
		|| curr_head_loc.y_coor == 0 || curr_head_loc.y_coor == (board->y_len - 1)
		|| is_inside_snake(&(board->snake), curr_head_loc))
	{
		//pthread_mutex_unlock(&(board->game_state_lock));
		return false;
	}
	

	// collision with apple?
	if (board->apple_loc.x_coor == curr_head_loc.x_coor
		&& board->apple_loc.y_coor == curr_head_loc.y_coor)
	{
		ate_apple = true;
		// update score
		board->score += 100;

		// new apple location
		uint_fast16_t num_attempts = 0;
		COOR new_apple_loc = old_head_loc; // old head location is now inside the snake-> guarantees the while loop executes at least once
		do{
			// poor performance when snake fills up majority of board-> what's a good way to mitigate against this?
				// allow for certain number of rand attempts, then just go through frame buffer until a space char is found
			new_apple_loc.x_coor = (rand() % (board->x_len - 2)) + 2; // -2 is a bit of a fudge factor...had some apples spawning inside the walls
			new_apple_loc.y_coor = (rand() % (board->y_len - 2)) + 1; // and I haven't had the chance to actually look into what's causing that to happen
			num_attempts++;

			if (num_attempts > 10) // 10 is arbitrary
			{
				uint_fast16_t x_len = board->x_len;
				uint_fast16_t y_len = board->y_len;

				for (uint_fast16_t x = 1; x < x_len; x++)
				{
					for (uint_fast16_t y = 1; y < y_len; y++)
					{
						// need to figure out a different method here...no longer using frame_buff
						if (frame_buff[X_Y_TO_INDEX(x, y, x_len + 1, y_len)] == ' ') // + 1 needed to account for space taken up by '\n' chars
						{
							new_apple_loc.x_coor = x;
							new_apple_loc.y_coor = y;
							goto Apple_Done;
						}
					}
				}
			}
		} while (is_inside_snake(&(board->snake), new_apple_loc) // can't spawn a new apple inside the snake's body...
			|| ((curr_head_loc.x_coor == new_apple_loc.x_coor) && (curr_head_loc.y_coor == new_apple_loc.y_coor))); // ... or inside of its head
Apple_Done:
		board->apple_loc = new_apple_loc;
	}
	
	// then a new segment "one forward" from the previous first segment
	board->snake.first_seg_index = board->snake.first_seg_index != 0 ? board->snake.first_seg_index - 1 : MAX_SNAKE_LEN; 
	board->snake.body[board->snake.first_seg_index].used = true;
	// and fill in the information for that entry...
	board->snake.body[board->snake.first_seg_index].loc = old_head_loc;

	if (!ate_apple) // tail doesn't move up if an apple is consumed...
	{
		// update the SNAKE's tail index (same deal as updating the head)
		board->snake.tail_index = board->snake.tail_index != 0 ? board->snake.tail_index - 1 : MAX_SNAKE_LEN;
		// code is not executed if apple is eaten->snake's length increase
			// end of tail stays still for one "tick", head jumps forward!
				// watched this at 25% speed https://www.youtube.com/watch?v=zIkBYwdkuTk&t=57s&ab_channel=GreerViau
	}
	
	//pthread_mutex_unlock(&(board->game_state_lock));
	return true;
}