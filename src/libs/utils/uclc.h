/*
	uclc.h
*/

#ifndef __UCLC_H__
#define __UCLC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <string.h>

/* UclcToUpper
 *
 *	文字列を全て大文字にする。
 */
char *uclcToUpper( char *str );

/* UclcToLower
 *
 *	文字列を全て小文字にする。
 */
char *uclcToLower( char *str );

/* UclcToUpperFirst
 *
 *	先頭の文字だけ大文字にする。
 */
char *uclcToUpperFirst( char *str );

/* UclcToLowerFirst
 *
 *	先頭の文字だけ小文字にする。
 */
char *uclcToLowerFirst( char *str );

#ifdef __cplusplus
}
#endif

#endif
