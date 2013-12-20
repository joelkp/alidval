/* alidval: program to calculate "alphabetical id values" for text strings
 *
 * Copyright (c) 2013 Joel K. Pettersson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * Declarations & definitions for the simple algorithm.
 */
/** Used as the base of the divisor. */
#define SIMPLE_DIV_BASE 27.0

/*
 * Option flags & variables.
 */
enum {
	FIRST_CHAR_AL = 1<<0,
	SCALE_OUTPUT_RANGE = 1<<1,
};
unsigned option_flags = 0;

double original_lb = 0.0,
       original_ub = 1.0;
double scaled_lb,
       scaled_ub;

/**
 * Print help/usage message.
 */
static void print_help(void)
{
	puts(
"usage: alidval [options] <string(s)>\n"
"\n"
"    <string(s)> is one or more strings, for each of which an \"alphabetical\n"
"    id value\" will be produced independently.\n"
"        The algorithm is limited in precision to the first 11 characters\n"
"    of a string. It is case-insensitive. Further, it only recognizes English\n"
"    alphabet ASCII characters as being letters. By default, all other char-\n"
"    acters are treated as being identical, and jointly given priority before\n"
"    'A'.\n"
"        The output id value ranges from 0.0 to 1.0 by default.\n"
"\n"
"    Options:\n"
"        -A  Make strings that begin with an alphabetical character fill up\n"
"            the whole output id range; other leading characters are made\n"
"            equal to 'A' or 'Z', depending on which priority they would\n"
"            ordinarily take.\n"
"        -r  Map the output id value onto a specified range. The range is\n"
"            specified in the format: <number>,<number>\n"
"                The numbers are the lower and upper bound, respectively; if\n"
"            omitted, the default for the number is used. If the lower bound\n"
"            exceeds the upper, the numbering order is reversed.\n"
);
}

/**
 * Parse a program invocation option argument (along with any arguments
 * belonging to the option(s)) in the passed strings.
 * \return On success, the number of strings parsed; on failure, 0.
 */
static int parse_option(int argc, const char *const*args)
{
	const char *arg = *args;
	int parse_count = 1;
	if (arg == NULL || *arg != '-') return 0;
	++arg;
	while (*arg != '\0') {
		switch (*arg++) {
		case 'A':
			option_flags |= FIRST_CHAR_AL;
			break;
		case 'r':
			if (*arg == '\0') {
				if (argc < 2) return 0;
				arg = *++args;
				++parse_count;
			}
			errno = 0;
			scaled_lb = original_lb;
			scaled_ub = original_ub;
			if (*arg != ',') {
				scaled_lb = strtod(arg, (char**)&arg);
				if (errno != 0 || scaled_lb != scaled_lb ||
				    *arg != ',') return 0;
			}
			++arg;
			if (*arg != '\0') {
				scaled_ub = strtod(arg, (char**)&arg);
				if (errno != 0 || scaled_ub != scaled_ub ||
				    *arg != '\0') return 0;
			}
			option_flags |= SCALE_OUTPUT_RANGE;
			return parse_count;
		default:
			return 0;
		}
	}
	return parse_count;
}

/**
 * Calculate alphabetical id for the passed string using a very simple
 * algorithm. Ignores case and treats all non-ASCII-alphabetical characters
 * as being identical. Precision in calculating the id value is limited to
 * the first 11 characters; the twelvth is not fully (nor, in many cases,
 * even partially) distintinguished, and any further characters leave the
 * output unaffected.
 * \return alphabetical string id ranging from 0.0 to 1.0, inclusive.
 */
static double calc_string_id_simple_nocase(const char *str)
{
	double id = 0.0,
	       divisor = SIMPLE_DIV_BASE;
	char c;
	int c_id;
	if (str == NULL) return id;
	/*
	 * Proceed through the characters; each is assigned a value from
	 * 0 (non-alphabetical characters) or ranging from 1 ('A') to
	 * 26 ('Z').
	 *
	 * If the "-A" switch was used, the first character is handled
	 * differently, and assigned a value from 0 ('A' and non-alphabetic
	 * characters) to 25 ('Z'). The divisor is also adjusted to fit the
	 * smaller range (i.e. set to 26.0 instead 27.0).
	 *
	 * The divisor corresponds to the place-value of the value of the
	 * present character; it is multiplied by 27.0 for each new character.
	 * By adding the result of the division for each character, the output
	 * is produced.
	 */
	if (option_flags & FIRST_CHAR_AL) {
		c = *str++;
		c_id = c;
		if (c_id > 'Z')
			c_id -= 'a' - 'A';
		c_id = (c_id >= 'A' && c_id <= 'Z') ?
			c_id - 'A' :
			0;
		divisor -= 1.0; /* adjust to smaller range */
		id = ((double)c_id) / divisor;
		divisor *= SIMPLE_DIV_BASE;
	}
	/*
	 * Remaining characters.
	 */
	while ((c = *str++) != '\0') {
		c_id = c;
		if (c_id > 'Z')
			c_id -= 'a' - 'A';
		c_id = (c_id >= 'A' && c_id <= 'Z') ?
			c_id - 'A' + 1 :
			0;
		id += ((double)c_id) / divisor;
		divisor *= SIMPLE_DIV_BASE; /* increment divisor exponent */
	}

	return id;
}

/**
 * Translate the id value from the original range to the scaled range.
 * \return The scaled string id.
 */
static double scale_string_id(double id)
{
	if (scaled_lb > scaled_ub) {
		id = (1.0 -
		      ((id - original_lb) / (original_ub - original_lb))) *
		     (scaled_lb - scaled_ub) +
		     scaled_ub;
		return id;
	}
	id = ((id - original_lb) / (original_ub - original_lb)) *
	     (scaled_ub - scaled_lb) +
	     scaled_lb;
	return id;
}

/**
 * Print the id value.
 */
static void print_string_id(double id)
{
	printf("%.20f\n", id);
}

int main(int argc, const char *const*argv)
{
	double string_id;

	/*
	 * Check/parse arguments.
	 */
	while (--argc > 0) {
		if (**++argv == '-') {
			int parse_count = parse_option(argc, argv);
			if (parse_count == 0) {
				print_help();
				return 0;
			}
			argc -= (parse_count - 1);
			argv += (parse_count - 1);
		} else {
			break;
		}
	}
	if (argc == 0) {
		print_help();
		return 0;
	}

	/*
	 * Calculate, optionally scale, and print alphabetical id for
	 * string argument(s).
	 */
	while (argc-- > 0) {
		string_id = calc_string_id_simple_nocase(*argv);
		if (option_flags & SCALE_OUTPUT_RANGE)
			string_id = scale_string_id(string_id);

		print_string_id(string_id);

		++argv;
	}

	return 0;
}

/* EOF */
