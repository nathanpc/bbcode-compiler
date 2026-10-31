/**
 * utils.h
 * A collection of utility functions to help us be more productive in C.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _BBCODE_UTILS_H
#define _BBCODE_UTILS_H

#include <stddef.h>

/**
 * Truncates a value to a byte (8 bits).
 */
#define TRUNC8(byte) ((byte) & 0xFF)

int chrchr(char ch, const char *chars);
size_t strcatdelim(char *buf, const char *start, const char *end);
size_t strcatdelim_alloc(char **buf, const char *start, const char *end);
int strcmpci(char const *a, char const *b);
const char* strcpyu(char *dest, const char *src, char ch);
int strcmpe(const char *str, const char *suffix);
int utfbom(const char* text);
const char* wrap_text(const char *text, int margin);
void print_text_wrap(const char *text, int margin, const char *prepend);
void print_until(const char *str, const char *until, int inclusive);

#endif /* _BBCODE_UTILS_H */
