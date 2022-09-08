#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

/****************************************************************************
* display_error
*
* - Prints a message to the console indicating an error. Gives the file, line
* number, and function in which the error occured. Also allows the caller to
* provide a custom message to print along with the error, in a "printf style"
* using a const char* format string and variable arguments
*
* Parameters :
* - file_name : name of the file in which the error occurred, grabbed using the
* __FILE__ macro
* - line_num : the line number where the file occurred, grabbed using the
* __LINE__ macro
* - func_sig : the signature of the function in which the error occurred, grabbed
* using the __FUNCSIG__ macro (or by whatever it's being used as an alias for,
* depending on the compiler)
* - user_clear : indicates whether the user will have to provide an input in order
* to clear/ continue past the error
* - err_msg : format string for the user's custom error message
* - ... : variable number of optional arguments corresponding to the format string
*
* Returns :
* - none
****************************************************************************/
void display_error(const char* file_name, int line_num, const char* func_sig, bool user_clear, const char* err_msg, ...)
{
	printf("ERROR: ");

	va_list arg_ptr;
	va_start(arg_ptr, err_msg);
	vprintf(err_msg, arg_ptr);
	va_end(arg_ptr);

	printf("\n\t[FILE] %s\n", file_name);
	printf("\t[LINE] %d\n", line_num);
	printf("\t[FUNC] %s\n", func_sig);

	if (user_clear)
	{
		printf("Press [ENTER] to continue...\n");
		char throw_away = getchar();
	}
}

/****************************************************************************
* clear_screen
*
* - Clears the console's screen by calling the relevant OS's clear screen
* command
*
* Parameters :
* - none
*
* Returns :
* - none
****************************************************************************/
inline void clear_screen()
{
#if defined(_WIN32) || defined(_WIN64)
	system("cls");
#elif  defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
	system("clear");
#else
	display_error(__FILE__, __LINE__, __FUNCSIG__, false,
		"Failed to clear the screen. Unable to identify operating system in use.");
#endif // _WIN32 OR _WIN64
}

/****************************************************************************
* erase_lines
*
* - Clears the specified number of lines in the console
* - Uses ASCII escape sequences
* - https://copyprogramming.com/howto/c-how-do-i-erase-a-line-from-the-console
*
* Parameters :
* - num_lines : the number of lines to clear
*
* Returns :
* - none
****************************************************************************/
// does this work on Mac?
void erase_lines(uint_fast16_t num_lines)
{
	if (num_lines > 0)
	{
		printf("\x1b[2K"); // Delete current line

		for (uint_fast16_t line = 1; line < num_lines; line++) // line = 1 because we included the first line
		{
			printf("\x1b[1A"); // Move cursor up one
			printf("\x1b[2K"); // Delete the entire line
		}
		printf("\r"); // Resume the cursor at beginning of line
	}
}

// only valid with game coordinates!
inline void set_cursor_position(uint_fast16_t x, uint_fast16_t y, uint_fast16_t board_height)
{
	printf("\033[%hu;%huH", board_height - y, x);
}

// https://cboard.cprogramming.com/windows-programming/55672-maximizing-console-window-full-screen.html?highlight=alt+enter+console
void set_console_fullscreen()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD NewSBSize = GetLargestConsoleWindowSize(hOut);
	SMALL_RECT DisplayArea = { 0, 0, 0, 0 };

	SetConsoleScreenBufferSize(hOut, NewSBSize);

	DisplayArea.Right = NewSBSize.X - 1;
	DisplayArea.Bottom = NewSBSize.Y - 1;

	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
}


