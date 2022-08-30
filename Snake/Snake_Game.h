#pragma once
#include <ctype.h> //_toupper() is broken without this...
#include <stdbool.h> // C bools
#include<pthread.h> // threads!
#include <stdio.h> // printing and such
#include <conio.h>	// getch()
#include <stdint.h> // cool types
#include <assert.h> // asserts for testing purposes
#include <stdlib.h> // srand() and rand()

#define KEY_UP		'W'
#define KEY_DOWN	'S'
#define KEY_RIGHT	'D'
#define KEY_LEFT	'A'

// We'll start by making a way to take in user input asyncronously
typedef struct USER_INPUT {
	pthread_mutex_t lock;
	char move_key;
}USER_INPUT;

typedef struct user_input_loop_args {
	bool game_continue;
	USER_INPUT input_store;
}user_input_loop_args;

// And I guess now we'll define some structs to keep track of the game state
// the basic thought is to have an array for the board, with each entry corresponding to a char on screen
// each entry will store whether nothing, an apple, or part of the snake occupies the space (do we want to track edges here too?)
// in regards to the snake, we'll need to track several things: 
	// each square's "next" square, (to follow the correct deletion path)
	// whether or not its the last square on the snake (to be deleted next game state update)
	// is that it?
// this feels pretty naive-> what's a better way to organize/ store this information?
	// linked list for the snake, don't worry about storing the board's empty squares
		// will only have to worry about when translating for printing on screen
	// don't want to be constantly creating and deleting...would be expensive
	// could figure out some max size for the snake, allocate that much space, and just mark entries as used/ unused
		// probably want to track the size of the snake so we can access the end in constant (instead of linear) time

// there has to be a better way to do the body segments with some sort of ring buffer...

// start with the tail at the very end of the array
// with each pass... 
	// go one past the old "first segment" and mark a new piece as active
	// go to the tail mark it as inactive, mark the segment adjacent to it as the new tail
	// head and tail should just chase eachother around the array
		// growing farther apart one way, closer together in the other direction-> invariant?

typedef struct COOR{
	uint_fast8_t x_coor;
	uint_fast8_t y_coor;
}COOR;

// it seems kind of silly to have this struct where there's only one member, the coordinate struct
// but I'll leave it organized this way for now in case I find the need to add more information down the line...
typedef struct SNAKE_SEG{
	COOR loc;
}SNAKE_SEG;

#define MAX_SNAKE_LEN	100
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

bool user_input_init(USER_INPUT* struct_in)
{
	if (struct_in == NULL)
	{
		return false;
	}
	struct_in->lock = PTHREAD_MUTEX_INITIALIZER;
	int init_success = pthread_mutex_init(&(struct_in->lock), NULL); // initialize struct's lock to default, unlocked state
	
	pthread_mutex_lock(&(struct_in->lock));
	struct_in->move_key = KEY_UP;
	pthread_mutex_unlock(&(struct_in->lock));

	return init_success == 0;
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

// not super sure why the asterisk is necessary
void* user_input_loop(user_input_loop_args* argptr)
{
	char user_key = KEY_UP;
	while (argptr->game_continue)
	{
		pthread_mutex_lock(&(argptr->input_store.lock));
		user_key = _toupper(_getch());
		if (user_key == KEY_UP || user_key == KEY_DOWN
			|| user_key == KEY_RIGHT || user_key == KEY_LEFT)
		{
			argptr->input_store.move_key = user_key;
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
		pthread_mutex_unlock(&(argptr->input_store.lock));
	}

	return NULL; 
}

bool inside_snake(const SNAKE_OBJ* snake, const COOR coor)
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

// move the snake's head one square in the correct direction, move the tail up accordingly...
	// handle apple eating/ spawning-> score updates
bool update_game_state(SNAKE_OBJ* snake, const char snake_dir, BOARD* board)
{
	// save the old head location before we update it
	COOR old_head_loc = {.x_coor = snake->head_loc.x_coor,.y_coor = snake->head_loc.y_coor};
	
	// then update the head's position
	assert(snake_dir == KEY_UP || snake_dir == KEY_DOWN
		|| snake_dir == KEY_RIGHT || snake_dir == KEY_LEFT);
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

	// probably save the head's location as a stack variable at this point 
	// to use going forward in the function-> avoid all that lookup overhead

	// now check if the snake's head collided with any of the walls, or itself
	if (snake->head_loc.x_coor == 0 || snake->head_loc.x_coor == board->x_len
		|| snake->head_loc.y_coor == 0 || snake->head_loc.y_coor == board->y_len
		|| inside_snake(snake, snake->head_loc))
	{
		return false;
	}

	if (board->apple_loc.x_coor == snake->head_loc.x_coor
		&& board->apple_loc.y_coor == snake->head_loc.y_coor)
	{
		// update score
		board->score += 100;

		// new apple location
		COOR new_apple_loc = old_head_loc; // old head location is now inside the snake-> guarantees the while loop executes at least once
		while (inside_snake(snake, new_apple_loc))
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

	// update the SNAKE's tail index (same deal as updating the head)
	snake->tail_index = snake->tail_index != 0 ? snake->tail_index - 1 : MAX_SNAKE_LEN - 1;

	return true;
}