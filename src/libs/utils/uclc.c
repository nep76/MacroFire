/*
	uclc.c
*/

#include "uclc.h"

char *uclcToUpper( char *str )
{
	int i;
	
	for( i = 0; str[i]; i++ ) str[i] = toupper( str[i] );
	
	return str;
}

char *uclcToLower( char *str )
{
	int i;
	
	for( i = 0; str[i]; i++ ) str[i] = tolower( str[i] );
	
	return str;
}

char *uclcToUpperFirst( char *str )
{
	str[0] = toupper( str[0] );
	return str;
}

char *uclcToLowerFirst( char *str )
{
	str[0] = tolower( str[0] );
	return str;
}
