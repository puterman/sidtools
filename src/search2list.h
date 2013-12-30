/* Copyright (C) 1999-2000 Linus Åkerlund
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the file COPYING for more details.
 */

void sid_info (char *line, int stilinfo, FILE *database);
void usage (void);
void str_tolower (char *str);

/* Used for start, init, play etc., where the user can specify smallest,
   greatest or exact values to be searched for */
struct bounds {
  unsigned short least;
  unsigned short most;
  unsigned short exact;
  char least_used;
  char most_used;
  char exact_used;
};
