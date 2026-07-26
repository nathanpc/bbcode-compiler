/**
 * utils.h
 * A collection of utility functions to help us be more productive in C.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _BBCODE_UTILS_H
#define _BBCODE_UTILS_H

#include <stddef.h>

int chrchr(char ch, const char *chars);
size_t strcatdelim(char *buf, const char *start, const char *end);
size_t strcatdelim_alloc(char **buf, const char *start, const char *end);
int strcmpci(char const *a, char const *b);
const char* strcpyu(char *dest, const char *src, char ch);
int strcmpe(const char *str, const char *suffix);
const char* wrap_text(const char *text, int margin);
void print_until(const char *str, const char *until, int inclusive);

#endif /* _BBCODE_UTILS_H */
