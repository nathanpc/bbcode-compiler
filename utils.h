/**
 * utils.h
 * A collection of utility functions to help us be more productive in C.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _BBCODE_UTILS_H
#define _BBCODE_UTILS_H

int chrchr(char ch, const char *chars);
int strcatdelim(char *buf, const char *start, const char *end);
int strcatdelim_alloc(char **buf, const char *start, const char *end);
int strcmpci(char const *a, char const *b);

#endif /* _BBCODE_UTILS_H */
