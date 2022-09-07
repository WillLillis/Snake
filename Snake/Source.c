#define DEBUG // informative print statements about what's going on
#include <pthread.h> // threads!
#include "Snake_Game.h" // main code for the game
#include "Misc.h" // random helper functions
#include <windows.h> // Need for Sleep() function rn...



int main()
{
	//user_input_loop_args user_input_args;

	//if (!game_init(&user_input_args))
	//{
	//	display_error(__FILE__, __LINE__, __FUNCSIG__, true, 
	//		"Error initializing game resources!");
	//}

	//pthread_t input_thread;
	//char user_key;

	//// actually needed
	//pthread_create(&input_thread, NULL, user_input_loop, &user_input_args);

	//// just testing stuff to make sure it works...
	//pthread_mutex_lock(&(user_input_args.input_store.lock));
	//user_key = user_input_args.input_store.move_key;
	//pthread_mutex_unlock(&(user_input_args.input_store.lock));

	//Sleep(10000); // play with it for 10 seconds... then set the quit flag and make sure it joins
	//
	//user_input_args.game_continue = false;

	//pthread_join(input_thread, NULL);

	// going to start testing the graphics....
	// let's try just seeing if the borders are saved in the frame buffer correctly

	size_t x_len = 40 + 1;
	size_t y_len = 40;

	char* frame_buff = (char*)malloc((x_len * y_len + 1) * sizeof(char)); // + 1 for \0 at end of the buffer

	if (frame_buff == NULL)
	{
		clear_screen();
		display_error(__FILE__, __LINE__, __FUNCSIG__, true,
			"Error occured while preparing display resources.");
		return NULL;
	}

	// initialize everything to a space, except the ends of the rows
	/*for (size_t y = 0; y < y_len; y++)
	{
		for (size_t x = 0; x < x_len - 1; x++)
		{
			frame_buff[X_Y_TO_INDEX(x, y, x_len, y_len)] = SPACE_CHAR;
		}
		frame_buff[X_Y_TO_INDEX(x_len - 1, y, x_len + 1, y_len)] = '\n';
	}
	frame_buff[x_len * y_len + 1] = '\0';*/
	memset(frame_buff, SPACE_CHAR, x_len * y_len * sizeof(char));
	frame_buff[x_len * y_len] = '\0';
	for (size_t i = 1; i < y_len; i++)
	{
		frame_buff[i * x_len - 1] = '\n';
	}

	//// fill in borders here, only needs to happen once
	//// top and bottom rows
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

	printf("%s", frame_buff);



	return 0;
}

// what's next?
	// need to modify init function so that all resources are set up at the start
	// winning conditions?
	// properly calculate max snake length?
	// work on some display stuff so we can start visually testing
	// for purposes of apple spawning...
		// use frame_buffer with update_game_state-> after certain number of failed random placements of apple, just iterate through until a spot is found?