/**
 * shims.h
 * A collection of shims and Windows-specific functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _BBCODE_SHIMS_H
#define _BBCODE_SHIMS_H

#include <limits.h>

/**
 * Definition of the maximum number of characters a line may contain, including
 * the NUL terminator.
 */
#ifndef LINE_MAX
	#define LINE_MAX 2048
#endif /* LINE_MAX */

#endif /* _BBCODE_SHIMS_H */
