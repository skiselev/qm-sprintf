/*
 * Minimalistic sprintf() function implementation
 * Copyright (c) 2017, Sergey Kiselev
 *
 * Based on Intel(r) Quark(tm) Software Interface
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>

#include "qm_common.h"

#define FORMAT_BUF_SIZE 32

static __inline__ char last_digit_to_char(unsigned int n, int base, bool upcase)
{
	char c;

	c = n % base;
	if (c <= 9)
		return c + '0';

	return upcase ? c + ('A' - 10) : c + ('a' - 10);
}

static __inline__ int str_putc(char **str, char c)
{
	**str = c;
	(*str)++;

	return 1;
}

static __inline__ int str_putint(char **str, unsigned int n, int base, bool upcase, bool sign, char pad, int pad_len)
{
	static char format_buf[FORMAT_BUF_SIZE];
	char *s = format_buf;
	int k;
	int i = 0;
	int len = 0;

	do {
		s[i++] = last_digit_to_char(n, base, upcase);
		n /= base;
	} while (n > 0);

	if (sign && pad == '0')
		len += str_putc(str, '-');

	/* print padding */
	if (sign)
		pad_len--;

	if (pad_len > i) {
		for (k = pad_len - i; k > 0; k--) {
			len += str_putc(str, pad);
		}
	}

	if (sign && pad != '0')
		len += str_putc(str, '-');

 	for (k = i - 1; k >= 0; k--)
		len += str_putc(str, s[k]);
 
	return len;
}

static __inline__ int str_putchars(char **str, const char *s)
{
	int len = 0;

	for (; *s; s++) {
		len += str_putc(str, *s);
	}

	return len;
}

int sprintf(char *str, const char *format, ...)
{
	const char *s = format;
	int len = 0;
	va_list ap;
	va_start(ap, format);

	while (*s) {
		char c = *s++;
		char pad = ' ';
		int pad_len = 0;
		if (c == '%') {
			c = *s++;

			if ('0' == c) {
				pad = '0';
				c = *s++;
			}

			while (c >= '0' && c <= '9') {
				pad_len = pad_len * 10 + c - '0';
				c = *s++;
			}

			/*
			 * Ignore 'l' length sub-specifier.
			 * This has no effect in ILP32 (4/4/4).
			 * In i586, 'int' and 'long int' are both 4 bytes long.
			 */
			if (c == 'l') {
				c = *s++;
			}
			switch (c) {
			case 'd': {
				int n;
				n = va_arg(ap, int);
				if (n >= 0) {
					len += str_putint(&str, n, 10, false, false, pad, pad_len);
				} else {
					len += str_putint(&str, -n, 10, false, true, pad, pad_len);
				}
				break;
			}
			case 'u': {
				unsigned int u;
				u = va_arg(ap, unsigned int);
				len += str_putint(&str, u, 10, false, false, pad, pad_len);
				break;
			}
			case 'X':
			case 'x': {
				unsigned int u;
				u = va_arg(ap, unsigned int);
				len += str_putint(&str, u, 16, c == 'X', false, pad, pad_len);
				break;
			}
			case 's': {
				const char *st;
				st = va_arg(ap, const char *);
				len += str_putchars(&str, st);
				break;
			}
			default:
				len += str_putc(&str, '%');
			case '%':
				len += str_putc(&str, c);
				break;
			}
			continue;
		}

		len += str_putc(&str, c);	
	}
	str_putc(&str, '\0');

	va_end(ap);
	return len;
}
