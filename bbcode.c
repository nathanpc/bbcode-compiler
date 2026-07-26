/**
 * bbcode.c
 * Compiles BBCode into an intermediate language that can later be used by other
 * applications to convert it into other markup languages.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "atom.h"
#include "utils.h"
#include "shims.h"

/**
 * The maximum number of characters, inclusing the NUL terminator, that a tag
 * name may contain.
 */
#define MAX_TAG_NAME_LEN 15

/**
 * Lexing stages.
 */
typedef enum lexer_stage_e {
	LEXER_START = -1,
	LEXER_STRING = 0,
	LEXER_INTAG,
	LEXER_INARG,
	LEXER_OUTTAG
} lexer_stage_t;

/* Function prototypes. */
int lex(const char *str, const char **start, const char **end);
char** lexer(const char *code);
atom_t* parse_atom(char **tokens, char ***next);
char** parse_tag(atom_t *atom, char **tokens);
char** parse_prefmt(atom_t *atom, char **tokens);

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Array of command line arguments.
 *
 * @return Return code.
 */
int main(int argc, char **argv) {
	char *code;
	char *line;
	size_t len;
	char **tokens;
	char **cur;
	FILE *stream;

	/* Take a memory snapshot for memory leak detection. */
	mem_snapshot_start();

	/* Check if we should read from a file. */
	stream = stdin;
	if (argc > 1) {
		stream = fopen(argv[1], "r");
		if (stream == NULL) {
			fprintf(stderr, "error: failed to open file '%s' for reading.\n",
				argv[1]);
			return 1;
		}
	}

	/* Read entire file into memory so that it can be tokenized later. */
	line = malloc(LINE_MAX * sizeof(char));
	code = malloc(LINE_MAX * sizeof(char));
	*code = '\0';
	len = 0;
	while (fgets(line, sizeof(line), stream)) {
		size_t linelen = strlen(line);
		code = realloc(code, (len + linelen + 1) * sizeof(char));
		strcat(code + len, line);
		len += linelen;
	}

	/* Close the file stream if needed. */
	if (stream != stdin) {
		fclose(stream);
		stream = NULL;
	}

	/* Pass BBCode through the lexer to extract tokens, then free resources. */
	free(line);
	tokens = lexer(code);
	free(code);
	line = NULL;
	code = NULL;

	/* Parse atoms and print them out. */
	cur = tokens;
	while (*cur != NULL) {
		atom_t *atom = parse_atom(cur, &cur);
		if (!atom)
			continue;

		atom_print(atom);
		atom_free(atom);
		atom = NULL;
	}

	/* Free the tokens list. */
	free(tokens);

	/* Check for memory leaks. */
	mem_snapshot_report();

	return 0;
}

/**
 * Lexes the entire code provided.
 *
 * @param code   BBCode string to be lexed.
 *
 * @return List of tokens lexed from the provided string.
 */
char** lexer(const char *code) {
	lexer_stage_t stage;
	const char *start;
	const char *end;
	char **tokens;
	int count;
	int ret;

	/* Allocate our initial token list. */
	count = 1;
	tokens = malloc(sizeof(char*));
	*tokens = NULL;

	stage = LEXER_START;
	start = code;
	while ((ret = lex(start, &start, &end)) > 0) {
		switch (stage) {
		case LEXER_INARG:
			/* Opening tags always generate a new token. For code blocks. */
			if (*start == '[')
				goto new_token;

			/* Treat all tokens that are not ']' as a single token. */
			if ((*tokens[count - 2] != '=') && (*start != ']')) {
				strcatdelim_alloc(&tokens[count - 2], start, end);
				count--;
				break;
			}

			goto new_token;
		case LEXER_STRING:
			/* Newlines are always new tokens. */
			if (*start == '\n')
				goto new_token;

			/* Join all tokens until we reach a tag opening. */
			if (*start != '[') {
				strcatdelim_alloc(&tokens[count - 2], start, end);
				count--;
				break;
			}

			goto new_token;
		case LEXER_START:
			/* Start of a brand new string. */
			stage = LEXER_STRING;
			goto new_token;
		case LEXER_INTAG:
		case LEXER_OUTTAG:
		new_token:
			strcatdelim_alloc(&tokens[count - 1], start, end);
			break;
		}
		
#ifdef _DEBUG
		printf("LEXER %d \"%s\" %d\n", stage, tokens[count - 1], count);
#endif /* _DEBUG */

		/* Change lexer stage if needed. */
		if (*start == '\n') {
			stage = LEXER_START;
		} else if (*start == '[') {
			stage = LEXER_INTAG;
		} else if ((*start == '=') && (stage == LEXER_INTAG)) {
			stage = LEXER_INARG;
		} else if ((stage >= LEXER_INTAG) && (*start == ']')) {
			stage = LEXER_OUTTAG;
		} else if (stage == LEXER_OUTTAG) {
			stage = LEXER_STRING;
		}

		/* Move to the next token in the string. */
		start = end;
		count++;

		/* Reset all buffers. */
		tokens = realloc(tokens, count * sizeof(char*));
		tokens[count - 1] = NULL;
	}

	return tokens;
}

/**
 * A very simple lexer to find the beginning and the end of tokens in a string.
 *
 * @param str   String to be scanned for tokens.
 * @param start Pointer to the start of a token.
 * @param end   Pointer to the end of a token.
 *
 * @return The size of our token if successful. 0 if we've reached the end of
 *         the string. Less than 0 if an error occurred.
 */
int lex(const char *str, const char **start, const char **end) {
	const char *delim = "[]/=\n";

	/* Are we wasting our time? */
	if (str == NULL)
		return -1;

	/* Ignore CR characters. */
	str += strspn(str, "\r");

	/* Check if we've reached the end of the string. */
	if (*str == '\0') {
		*start = *end = NULL;
		return 0;
	}

	/* Read the delimiter or a string until we reach the next delimiter. */
	*start = str;
	if (chrchr(*str, delim)) {
		*end = str + 1;
	} else {
		*end = str + strcspn(str, delim);
	}

	return *end - *start;
}

/**
 * Parses the next atom from the tokens list.
 *
 * @warning This function will free unused tokens from the tokens list.
 *
 * @param tokens Tokens list.
 * @param next   Pointer to where the next token to be parsed.
 *
 * @return Parsed a dynamically allocated atom object. NULL if a token to be
 *         ignored was parsed.
 */
atom_t* parse_atom(char **tokens, char ***next) {
	atom_t *atom;
	static atom_type_t last_type;
	static int prefmt_stat;
	static char last_open_tag[MAX_TAG_NAME_LEN];

	/* Create atom object. */
	atom = atom_new(-1);

	/* Deal with code/preformatted sections. */
	if (prefmt_stat == 1) {
		/* Parse the preformatted section atom. */
		atom->type = ATOM_PREFMT;
		*next = parse_prefmt(atom, tokens);
		prefmt_stat = -1;
		goto store_atom;
	} else if (prefmt_stat == -1) {
		/* Close the code tag that was gobbled while parsing the section. */
		atom->type = ATOM_TAG_CLOSE;
		atom->value = strdup(last_open_tag);
		prefmt_stat = 0;
		goto store_atom;
	}

	/* Parse atom based on the first character of the current token. */
	switch (**tokens) {
	case '[':
		/* Tag */
		free(*tokens);
		*tokens = NULL;
		*next = parse_tag(atom, tokens + 1);
		break;
	case ']':
		/* Closing brace of tag. */
		if ((last_type >= ATOM_TAG_OPEN) || (last_type == ATOM_ESCAPED)) {
			/* Ignore this token. */
			free(*tokens);
			*tokens = NULL;
			*next = tokens + 1;
			free(atom);
			atom = NULL;
			return NULL;
		}

		/* Regular text atom that starts with an closing brace. */
		goto text_atom;
	case '=':
		/* Tag argument if last token was a tag open. */
		if (last_type == ATOM_TAG_OPEN) {
			/* Change atom type and ignore token. */
			atom->type = ATOM_TAG_ARG;
			free(*tokens);
			*tokens = NULL;

			/* Use next token as the atom's value. */
			atom->value = *(tokens + 1);

			/* Jump ahead of the value token. */
			*next = tokens + 2;
			break;
		}

		/* Regular text atom that starts with an equal sign. */
		goto text_atom;
	case '\n':
		/* Linebreak */
		atom->type = ATOM_LINEBREAK;
		atom->value = NULL;

		/* Ignore the current token and jump to the next one. */
		free(*tokens);
		*tokens = NULL;
		*next = tokens + 1;
		break;
	default:
		/* Text */
		text_atom:
		atom->type = ATOM_RAW;
		atom->value = *tokens;

		/* Jump to the next token. */
		*next = tokens + 1;
		break;
	}

	/* Determine if we are about to parse a preformatted section. */
	if ((atom->type == ATOM_TAG_OPEN) && (!strcmp(atom->value, "CODE") ||
			!strcmp(atom->value, "PRE"))) {
		/* Ignore closing brace of code tag. */
		free(**next);
		**next = NULL;
		*next = *next + 1;

		/* Mark the beginning of a preformatted section. */
		prefmt_stat = 1;
	} else {
		prefmt_stat = 0;
	}

store_atom:
	/* Store the last atom type and tag open. */
	last_type = atom->type;
	if (atom->type == ATOM_TAG_OPEN)
		strncpy(last_open_tag, atom->value, MAX_TAG_NAME_LEN);

	return atom;
}

/**
 * Parses an opening or closing of a tag.
 *
 * @param atom   Atom object to be populated.
 * @param tokens Tokens list to be used.
 *
 * @return Position of the next token after the atom.
 */
char** parse_tag(atom_t *atom, char **tokens) {
	char *tmp;
	char **next;

	/* Check if we are closing or opening a tag. */
	next = tokens;
	if (**next == '/') {
		atom->type = ATOM_TAG_CLOSE;
		free(*next);
		*next = NULL;
		next++;
	} else {
		atom->type = ATOM_TAG_OPEN;
	}

	/* Extract value and normalize tag value. */
	atom->value = *next;
	next++;
	tmp = atom->value;
	while (*tmp) {
		*tmp = toupper((unsigned char)*tmp);
		tmp++;
	}

	/* Ignore cases where the user wants to print a bracket. */
	if (*atom->value == '[')
		atom->type = ATOM_ESCAPED;

	return next;
}

/**
 * Parses an entire preformatted section of tokens.
 *
 * @param atom   Atom object to be populated.
 * @param tokens Tokens list to be used.
 *
 * @return Position of the next token after the atom.
 */
char** parse_prefmt(atom_t *atom, char **tokens) {
	int close_seq;
	char **next;
	size_t len;
	
	/* Prepare the atom's value to be populated. */
	atom->value = malloc(sizeof(char));
	*atom->value = '\0';
	len = 1;

	/* Join all tokens until we reach the '[', '/', "code", ']' sequence. */
	close_seq = 0;
	next = tokens;
	while (close_seq < 4) {
		/* Store place of NUL terminator to shortcircuit concatenation. */
		size_t lenv = len - 1;

		switch (close_seq) {
		case 0:
			/* Looking for '['. */
			if (**next == '[') {
				close_seq++;
			} else {
				close_seq = 0;
			}
			break;
		case 1:
			/* Looking for '/'. */
			if (**next == '/') {
				close_seq++;
			} else {
				close_seq = 0;
			}
			break;
		case 2:
			/* Looking for "code". */
			if (!strcmpci(*next, "code") || !strcmpci(*next, "pre")) {
				close_seq++;
			} else {
				close_seq = 0;
			}
			break;
		case 3:
			/* Looking for ']'. */
			if (**next == ']') {
				close_seq++;
			} else {
				close_seq = 0;
			}
			break;
		}

		/* Append token to value. */
		len += strlen(*next);
		atom->value = realloc(atom->value, len * sizeof(char));
		strcat(atom->value + lenv, *next);

		/* Discard token and move to the next one. */
		free(*next);
		*next = NULL;
		next++;
	}

	/* Remove the "[/code]" from the end of the preformatted section */
	len -= 7;
	atom->value[len - 1] = '\0';
	atom->value = realloc(atom->value, len * sizeof(char));

	return next;
}
