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

#include "utils.h"

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

/**
 * Types of atoms.
 */
typedef enum atom_type_e {
	ATOM_RAW = 0,
	ATOM_PRE,
	ATOM_TAG_OPEN,
	ATOM_TAG_ARG,
	ATOM_TAG_CLOSE
} atom_type_t;

/**
 * Atom object definition.
 */
typedef struct atom_s {
	atom_type_t type;
	char *value;
} atom_t;

/* Function prototypes. */
int lex(const char *str, const char **start, const char **end);
char** lexer(const char *code);
atom_t* parse_atom(char **tokens, char ***next);
void atom_free(atom_t *atom);
void atom_print(const atom_t *atom);

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Array of command line arguments.
 *
 * @return Return code.
 */
int main(int argc, char **argv) {
	char *code = "This is an example of [b]BBCode[/b]!\n\nIt's very simple, "
		"[url=https://nathancampos.me/assets/images/http-status/404.jpg]but "
		"it's honest work[/url].\n\n[code]This is [b]BBCode[/b] that\nis "
		"ignored. Hopefully [this= is does not break things.[/code]\n\nThis "
		"is something/somewhere, and 2+2=4.";
	char **tokens;
	char **cur;

	/* Pass the BBCode through the lexer and extract tokens. */
	tokens = lexer(code);

	/* Print all tokens. */
	cur = tokens;
	printf("All lexed tokens:\n");
	while (*cur != NULL) {
		printf("\"%s\"\n", *cur);
		cur++;
	}

	printf("Parsing atoms...\n");
	cur = tokens;
	while (*cur != NULL) {
		atom_t *atom;
		atom = parse_atom(cur, &cur);
		if (atom) {
			atom_print(atom);
			atom_free(atom);
			atom = NULL;
		}
	}

#if 0
	/* Free all tokens. */
	cur = tokens;
	do {
		if (*cur)
			free(*cur);
		cur++;
	} while (*cur != NULL);
#endif
	free(tokens);

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
		
		printf("LEXER %d \"%s\" %d\n", stage, tokens[count - 1], count);

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

char** parse_tag(atom_t *atom, char **tokens) {
	char **next = tokens;

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
	/* TODO: Make value lowercase. */
	next++;

	return next;
}

/**
 * Parses the next atom from the tokens list.
 *
 * @warning This function will free unused tokens from the tokens list.
 *
 * @param tokens Tokens list.
 * @param next   Pointer to where the next token to be parsed.
 *
 * @return Parsed a dynamically allocated atom object.
 */
atom_t* parse_atom(char **tokens, char ***next) {
	atom_t *atom;
	static atom_type_t last_type;

	atom = malloc(sizeof(atom_t));
	atom->value = NULL;
	switch (**tokens) {
	case '[':
		/* Tag */
		free(*tokens);
		*tokens = NULL;
		*next = parse_tag(atom, tokens + 1);
		break;
	case ']':
		/* Closing brace of tag. */
		if ((last_type == ATOM_TAG_OPEN) || (last_type == ATOM_TAG_CLOSE)) {
			free(*tokens);
			*tokens = NULL;
			*next = tokens + 1;
			return NULL;
		}
		goto text_atom;
	case '=':
		if (last_type == ATOM_TAG_OPEN) {
			atom->type = ATOM_TAG_ARG;
			free(*tokens);
			*tokens = NULL;
			atom->value = *(tokens + 1);
			*next = tokens + 2;
			break;
		}
		goto text_atom;
	default:
		/* Text */
		text_atom:
		atom->type = ATOM_RAW;
		atom->value = *tokens;
		*next = tokens + 1;
		break;
	}

	last_type = atom->type;
	return atom;
}

/**
 * Frees up any resources taken by an atom object.
 *
 * @param atom Atom object to be completely freed.
 */
void atom_free(atom_t *atom) {
	/* Invalidate type and free value string. */
	atom->type = -1;
	if (atom->value) {
		free(atom->value);
		atom->value = NULL;
	}

	/* Free the atom object itself. */
	free(atom);
}

/**
 * Prints out the contents of an atom in the format of an intermediate output
 * language for formatters to use.
 *
 * @param atom Atom to be printed out.
 */
void atom_print(const atom_t *atom) {
	/* Type */
	switch (atom->type) {
	case ATOM_RAW:
		printf("TEXT");
		break;
	case ATOM_PRE:
		printf("PREFMT");
		break;
	case ATOM_TAG_OPEN:
		printf("TAG_OPEN");
		break;
	case ATOM_TAG_ARG:
		printf("TAG_ARG");
		break;
	case ATOM_TAG_CLOSE:
		printf("TAG_CLOSE");
		break;
	default:
		printf("UNKNOWN");
	}
	putchar('\t');

	/* Value */
#ifdef _DEBUG
	putchar('"');
#endif
	printf("%s", atom->value);
#ifdef _DEBUG
	putchar('"');
#endif
	putchar('\n');
}
