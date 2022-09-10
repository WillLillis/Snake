#include "Snake_Graphics.h" // main code for the game
#include <pthread.h> // threads!
#include <windows.h> // Need for Sleep() function, window control
#include <time.h> // main game loop needs some sort of timekeeping

int main()
{
	set_console_fullscreen();

	pthread_t input_thread;
	pthread_t graphics_thread;

	game_args game_args;
	display_game_args display_args;

	if (!game_init(&game_args))
	{
		return -1;
	}
	if (!graphics_init(&display_args, &(game_args.board), &(game_args.user_input.input_store)))
	{
		return -1;
	}

	pthread_create(&input_thread, NULL, user_input_loop, &(game_args.user_input));
	pthread_create(&graphics_thread, NULL, display_game, &(display_args));

	Sleep(100); // fudge factor... give the threads some time to create and such 

	bool game_continue = true;
	char snake_dir = KEY_UP;
	char snake_dir_temp;
	clock_t start, diff;
	uint32_t ms = 0;
	uint32_t game_tick_rate = 125; // need to play with this...

	while (game_continue)
	{
		start = clock();
		ms = 0;
		snake_dir_temp = get_user_input(&(game_args.user_input));
		if (snake_dir_temp == KEY_UP && snake_dir == KEY_DOWN // can't move opposite your current direction of motion
			|| snake_dir_temp == KEY_DOWN && snake_dir == KEY_UP
			|| snake_dir_temp == KEY_RIGHT && snake_dir == KEY_LEFT
			|| snake_dir_temp == KEY_LEFT && snake_dir == KEY_RIGHT)
		{
			;
		}
		else
		{
			snake_dir = snake_dir_temp;
		}
		game_continue = update_game_state(&(game_args.board), snake_dir, game_args.board.frame_buff);

		if (!game_continue)
		{
			break;
		}

		// start = clock() and ms = 0 have to be here if we're using the game_state_lock mutex,
		// but without them they're fine up at the top of the loop, which should make the game's 
		// timing more consistent

		do {
			diff = clock() - start;
			ms = (diff * 1000) / CLOCKS_PER_SEC;
		} while (ms < game_tick_rate);
	}

	game_args.user_input.game_over = true;
	display_args.game_over = true;

	pthread_join(input_thread, NULL);
	pthread_join(graphics_thread, NULL);

	return 0;
}

// take another look at usage of frame_buff
	// still need something for efficient apple random placement when snake is filling up board, but frame_buff no longer needed
// go through and comment/ clean up everything
// need to check for winning conditions