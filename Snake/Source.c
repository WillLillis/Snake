//#define DEBUG // informative print statements about what's going on
#include <pthread.h> // threads!
#include "Snake_Graphics.h" // main code for the game
#include <windows.h> // Need for Sleep() function rn...
#include <time.h>



int main()
{
	pthread_t input_thread;
	pthread_t graphics_thread;

	game_args game_args;
	display_game_args display_args;

	if (!game_init(&game_args))
	{
		return -1;
	}
	if (!graphics_init(&display_args, &(game_args.board)))
	{
		return -1;
	}

	pthread_create(&input_thread, NULL, user_input_loop, &(game_args.user_input));
	pthread_create(&graphics_thread, NULL, display_game, &(display_args));

	Sleep(100);
	
	bool game_continue = true;
	char snake_dir = KEY_UP;
	clock_t start, diff;
	uint32_t ms = 0;
	uint32_t game_tick_rate = 1000; // once every second, to start at least...

	while (game_continue)
	{
		snake_dir = get_user_input(&(game_args.user_input));
		game_continue = update_game_state(&(game_args.board), snake_dir, game_args.board.frame_buff);

		if (!game_continue)
		{
			printf("Done!\n");
			break;
		}

		start = clock();
		ms = 0;
		while (ms < game_tick_rate)
		{
			diff = clock() - start;
			ms = (diff * 1000) / CLOCKS_PER_SEC;
		}
	}

	game_args.user_input.game_over = true;
	display_args.game_over = true;

	pthread_join(input_thread, NULL);
	pthread_join(graphics_thread, NULL);

	return 0;
}

// The game is working well enough for actual testing!!!!!
// there's clearly a ton of stuff we need to clean up, but its operating at least
// need to take a look at how the snake_obj struct is updated as it moves...
	// the head seems to be responding to movement, but the body parts behind aren't clearing properly
	// prolly just some sloppy code
// Also need to rework how the frame buffer is used-> do we need for anything besides keeping track of where the snake body is?