#define DEBUG // informative print statements about what's going on
#include <pthread.h> // threads!
#include "Snake_Game.h" // main code for the game
#include "Misc.h" // random helper functions
#include <windows.h> // Need for Sleep() function rn...



int main()
{
	user_input_loop_args user_input_args;

	if (!game_init(&user_input_args))
	{
		display_error(__FILE__, __LINE__, __FUNCSIG__, true, 
			"Error initializing game resources!");
	}

	pthread_t input_thread;
	char user_key;

	// actually needed
	pthread_create(&input_thread, NULL, user_input_loop, &user_input_args);

	// just testing stuff to make sure it works...
	pthread_mutex_lock(&(user_input_args.input_store.lock));
	user_key = user_input_args.input_store.move_key;
	pthread_mutex_unlock(&(user_input_args.input_store.lock));

	Sleep(10000); // play with it for 10 seconds... then set the quit flag and make sure it joins
	
	user_input_args.game_continue = false;

	pthread_join(input_thread, NULL);

	return 0;
}

// what's next?
	// work on some clean up of the game_update() function when you're awake
	// work on some display stuff so we can start visually testing
	// decide how we're going to do the length increase (when an apple is eaten) and implement it
		// watch some yt videos and see how they do it