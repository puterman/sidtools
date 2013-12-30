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

#define DEFAULT_DB_FILE "/.sidtoolsdb"

int recurs(char *dir);
int stilparse (const char *sids_base_dir, const char *target);
int sidparse(char *current_file, int size);
void str_tolower(char *str);
void make_db(char *file_name, int number_of_files, char *rc_file,
	     char *sids_base_dir);
void usage (char *filename);
char *get_full_path (char *filename, char *full_path);
int stil_parse (char *sids_base_dir);

struct stil_entry {
  char *entry;
  struct stil_entry *next;
};

struct db_entry {
  char *title;
  char *author;
  char *copy;
  char *file;
  int nr_songs;
  int start_song;
  int load_addr;
  int init_addr;
  int play_addr;
  int size;
  struct db_entry *next;
};
