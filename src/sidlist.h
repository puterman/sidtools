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

#define GZ_SUFFIX ".gz"
#define SID_GZ_SUFFIX ".sid.gz"
#define TIME_TO_WAIT 180

int play_random (const char *db_file, int time, int delay, char *sidplay_args);
int play_list (const char *playlist, int stdn, int time, int delay,
		int start_tune, char *sidplay_args);
int play_dir (const char *playlist, DIR *directory, int time, int delay,
	      int start_tune, char *sidplay_args);
void usage(void);
int playwait (int wait_time);
