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

#define NOOP			(void)0

#define KEY_UP			'W'
#define KEY_DOWN		'S'
#define KEY_RIGHT		'D'
#define KEY_LEFT		'A'

// going to #define the board size here for now...
// might add an option to let the user choose the size later...
#define BOARD_LEN_X		40
#define BOARD_LEN_Y		40
#define MAX_SNAKE_LEN	100 // Needs to be a function of the board size?

// We'll start by making a way to take in user input asyncronously
typedef struct USER_INPUT {
	pthread_mutex_t lock;
	char move_key;
}USER_INPUT;

typedef struct user_input_loop_args {
	bool game_continue;
	USER_INPUT input_store;
}user_input_loop_args;

typedef struct COOR{
	uint_fast8_t x_coor;
	uint_fast8_t y_coor;
}COOR;

// it seems kind of silly to have this struct where there's only one member, the coordinate struct
// but I'll leave it organized this way for now in case I find the need to add more information down the line...
typedef struct SNAKE_SEG{
	COOR loc;
}SNAKE_SEG;

typedef struct SNAKE_OBJ{
	COOR head_loc;
	SNAKE_SEG body[MAX_SNAKE_LEN];
	size_t first_seg_index;
	size_t tail_index;
}SNAKE_OBJ;

// not sure if we're going to use this...
typedef struct BOARD{
	COOR apple_loc;
	uint_fast8_t score;
	uint_fast8_t x_len;
	uint_fast8_t y_len;
}BOARD;

// has to be down here, as SNAKE_OBJ and BOARD structs need to be defined
#include "Snake_Graphics.h"

bool user_input_init(USER_INPUT* struct_in)
{
	if (struct_in == NULL)
	{
		return false;
	}
	struct_in->lock = PTHREAD_MUTEX_INITIALIZER;
	int init_success = pthread_mutex_init(&(struct_in->lock), NULL); // initialize struct's lock to default, unlocked state

	if (init_success != 0) // do nothing if the function suceeded, return false otherwise
	{
		return false;
	}
	
	pthread_mutex_lock(&(struct_in->lock));
	struct_in->move_key = KEY_UP;
	pthread_mutex_unlock(&(struct_in->lock));

	return true;
}

// need to add board init here
bool game_init(user_input_loop_args* input_args)
{
	if (input_args == NULL)
	{
		return false;
	}
	input_args->game_continue = true;

	srand((unsigned int)time(NULL)); // seed the random number generator for our apple locations

	return user_input_init(&(input_args->input_store));
}

void* user_input_loop(user_input_loop_args* argptr)
{
	char user_key = KEY_UP;
	while (argptr->game_continue)
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
#ifdef DEBUG
		printf("New input: ");
		switch (user_key) {
		case KEY_UP:
			printf("KEY_UP\n");
			break;
		case KEY_DOWN:
			printf("KEY_DOWN\n");
			break;
		case KEY_RIGHT:
			printf("KEY_RIGHT\n");
			break;
		case KEY_LEFT:
			printf("KEY_LEFT\n");
			break;
		} // key press prints...
#endif
	}

	return NULL; 
}

bool is_inside_snake(const SNAKE_OBJ* snake, const COOR coor)
{
	for (size_t curr_seg = snake->tail_index; curr_seg <= snake->first_seg_index; curr_seg = (curr_seg != 0) ? curr_seg - 1 : MAX_SNAKE_LEN - 1)
	{
		if (snake->body[curr_seg].loc.x_coor == coor.x_coor // not sure if just testing equality with the COOR structs will work here
			&& snake->body[curr_seg].loc.y_coor == coor.y_coor)
		{
			return false;
		}
	}

	return true;
}

bool update_game_state(SNAKE_OBJ* snake, const char snake_dir, BOARD* board)
{
	bool ate_apple = false;
	// save the old head location before we update it
	COOR old_head_loc = { .x_coor = snake->head_loc.x_coor,.y_coor = snake->head_loc.y_coor };

	// then update the head's position
	switch (snake_dir) {
	case KEY_UP:
		snake->head_loc.y_coor++;
		break;
	case KEY_DOWN:
		snake->head_loc.y_coor--;
		break;
	case KEY_RIGHT:
		snake->head_loc.x_coor++;
		break;
	case KEY_LEFT:
		snake->head_loc.x_coor--;
		break;
	}

	COOR curr_head_loc = snake->head_loc;

	// now check if the snake's head collided with any of the walls, or itself
	if (curr_head_loc.x_coor == 0 || curr_head_loc.x_coor == board->x_len
		|| curr_head_loc.y_coor == 0 || curr_head_loc.y_coor == board->y_len
		|| is_inside_snake(snake, curr_head_loc))
	{
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
		COOR new_apple_loc = old_head_loc; // old head location is now inside the snake-> guarantees the while loop executes at least once
		while (is_inside_snake(snake, new_apple_loc) // can't spawn a new apple inside the snake's body...
			|| (curr_head_loc.x_coor == new_apple_loc.x_coor && curr_head_loc.y_coor == new_apple_loc.y_coor)) // ... or inside of its head
		{
			new_apple_loc.x_coor = (rand() % (board->x_len - 1)) + 1;
			new_apple_loc.y_coor = (rand() % (board->y_len - 1)) + 1;
		}

		board->apple_loc = new_apple_loc;
	}

	// then a new segment "one forward" from the previous first segment
	snake->first_seg_index = snake->first_seg_index != 0 ? snake->first_seg_index - 1 : MAX_SNAKE_LEN - 1;
	// and fill in the information for that entry...
	snake->body[snake->first_seg_index].loc = old_head_loc;

	if (!ate_apple) // tail doesn't move up if an apple is consumed...
	{
		// update the SNAKE's tail index (same deal as updating the head)
		snake->tail_index = snake->tail_index != 0 ? snake->tail_index - 1 : MAX_SNAKE_LEN - 1;
		// code is not executed if apple is eaten->snake's length increase
			// end of tail stays still for one "tick", head jumps forward!
				// watched this at 25% speed https://www.youtube.com/watch?v=zIkBYwdkuTk&t=57s&ab_channel=GreerViau
	}
	

	return true;
}