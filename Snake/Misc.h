#pragma once
#include <stdio.h>
#include <stdarg.h>

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


