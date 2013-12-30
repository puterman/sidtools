/************************************************************************
 * File: sidlist.c
 * A rewrite of the Perl version of sidlist in C.
 *
 * Plays the sids in a playlist, lets you choose a playlist file, a
 *   directory or random tunes and some other stuff.
 *
 * Copyright (C) 1999-2000 Linus Åkerlund <uxm165t@tninet.se>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 **********************************************************************/

#include <config.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include "parsecfg.h"
#include "sidlist.h"
#include "sidtools.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif

int wait_flag = 0;

/*
 * main(): Parses args, parses config file, decides what sort of list
 *   to play and lets play_random(), play_list() and play_dir() take
 *   care of that.
 */

int
main (int argc, char *argv[])
{
  unsigned int wait_time = TIME_TO_WAIT, dir = 0, stdn = 0;
  DIR *directory = NULL;
  FILE *f, *db;
  int random = 0, number_of_songs, next_song, current_song = 0;
  int i, start_tune = 0, delay = 0;
  char *playlist = "", *db_file;
  char *song, *args;
  char *temp;
  char *sidplay_args = NULL;
  char *cfg_trash, *cfg_file;

  cfgStruct cfg[] =
  {
    { "SIDDB", CFG_STRING, &db_file },
    { "HVSC_BASE", CFG_STRING, &cfg_trash },
    { "SIDPLAY", CFG_STRING, &cfg_trash },
    { "SIDPLAY_ARGS", CFG_STRING, &cfg_trash },
    { NULL, CFG_END, NULL }
  };
  
  struct option opts[] =
  {
    {"random", no_argument, NULL, 'r'},
    {"start-tune", required_argument, NULL, 's'},
    {"time-to-wait", required_argument, NULL, 't'},
    {"delay", required_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {"database", required_argument, NULL, 'b'},
    {"args", required_argument, NULL, 'a'},
    {0, 0, 0, 0}
  };
  int option_index = 0;
  char *arg;

  if (!(cfg_file = malloc (sizeof (char) * (strlen (getenv ("HOME")) +
					    strlen (RC_FILE) + 2))))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }

  strcpy (cfg_file, getenv ("HOME"));
  strncat (cfg_file, RC_FILE, 255 - strlen (cfg_file));
  
  if (db = fopen (cfg_file, "r"))
    {
      cfgParse (cfg_file, cfg, CFG_SIMPLE);
      fclose (db);
    }
  else
    {
      free (cfg_file);
      cfg_file = strdup (CONFIGDIR);

      if (db = fopen (cfg_file, "r"))
	{
	  cfgParse (cfg_file, cfg, CFG_SIMPLE);
	  fclose (db);
	}
      else
	{
	  fprintf (stderr, "ERROR: No conf file in $HOME%s or %s!\n",
		   RC_FILE, CONFIGDIR);
	  exit (1);
	}
    }

  while (1)
    {
      i = getopt_long (argc, argv, "rs:t:d:hb:a:", opts, &option_index);

      if (i == -1)
	break;

      switch (i)
	{
	case 'r':
	  random = 1;
	  break;

	case 'b':
	  db_file = strdup (optarg);
	  break;

	case 's':
	  arg = strdup (optarg);
	  start_tune = (int) strtol (arg, NULL, 10);
	  break;

	case 't':
	  arg = strdup (optarg);
	  wait_time = (int) strtol (arg, NULL, 10);
	  break;

	case 'd':
	  arg = strdup (optarg);
	  delay = (int) strtol (arg, NULL, 10);
	  break;

	case 'h':
	  usage ();
	  return 0;

	case 'a':
	  sidplay_args = strdup (optarg);
	  break;

	case '?':
	  /* getopt_long already printed an error message */
	  usage ();
	  return 1;

	default:
	  abort ();
	}
    }
  while (optind < argc)
    {
      if (!strncmp (argv[optind], "-", 1))
	stdn = 1;
      else if ((directory = opendir (argv[optind])) == NULL)
	{
	  dir = 0;
	  if (!(playlist = (char *) malloc (strlen (argv[optind]) + 1)))
	    {
	      fprintf (stderr, "Can't allocate memory!\n");
	      exit (1);
	    }
	  strcpy (playlist, argv[optind]);
	}
      else
	{
	  dir = 1;
	  if (!(playlist = (char *) malloc (strlen (argv[optind]))))
	    {
	      fprintf (stderr, "Can't allocate memory!\n");
	      exit (1);
	    }
	  strcpy (playlist, argv[optind]);
	  if (playlist[strlen (playlist) - 1] == '/')
	    playlist[strlen (playlist) - 1] = '\0';
	}
      optind++;
    }

  if (random)
    {
      if (play_random (db_file, wait_time, delay, sidplay_args))
	{
	  system ("sidplayo");
	  return 1;
	}
    }
  else if (!dir)
    {
      if (play_list (playlist, stdn, wait_time, delay, start_tune,
		     sidplay_args))
	{
	  return 1;
	  system ("sidplayo");
	}
    }
  else
    {
      if (play_dir (playlist, directory, wait_time, delay, start_tune,
		    sidplay_args))
	{
	  return 1;
	  system ("sidplayo");
	}
    }

  system ("sidplayo");
  return 0;
}

/*
 * play_random(): plays random sid tunes.
 */

int
play_random (const char *db_file, int wait_time, int delay, char *sidplay_args)
{
  int current_song = 1;
  int number_of_songs, next_song, i;
  FILE *f;
  char *song_to_play;
  char command[512];
  char line[512];
  char stil_line[4097];

  srand (time (0));

  while (1)
    {
      f = fopen (db_file, "r");
      if (f == NULL)
	{
	  fprintf (stderr, "ERROR: Couldn't open database file.\n");
	  return 1;
	}
      fscanf (f, "%d\n", &number_of_songs);
      next_song = rand () % (number_of_songs + 1);
      for (i = 0; i < next_song; i++)
	{
	  fgets (line, 511, f);
	  fgets (stil_line, 4096, f);
	}

      fclose (f);
      song_to_play = strtok (line, SEP);
      song_to_play = strtok (NULL, SEP);
      song_to_play = strtok (NULL, SEP);
      song_to_play = strtok (NULL, SEP);
      song_to_play[strlen (song_to_play)] = '\0';

      strcpy (command, "sidplayo ");
      if (sidplay_args)
	{
	  strncat (command, sidplay_args, 511 - strlen (command));
	  strncat (command, " ", 511 - strlen (command));
	}
      strncat (command, "\"", 511 - strlen (command));
      strncat (command, song_to_play, 511 - strlen (command));
      strncat (command, "\"", 511 - strlen (command));

      if ((f = fopen (song_to_play, "r")) != NULL)
	{
	  fclose (f);
	  if (delay != 0)
	    {
	      system ("sidplayo");
	      sleep (delay);
	    }
	  printf ("Playing song number %d: %s\n", current_song++, song_to_play);
	  system (command);
	}
      else
	{
	  strncat (command, GZ_SUFFIX, 511 - strlen (command));
	  if (delay != 0)
	    {
	      system ("sidplayo");
	      sleep (delay);
	    }
	  printf ("Playing song number %d: %s\n", current_song++, song_to_play);
	  system (command);
	}
      if (!playwait (wait_time))
	printf ("\n");
    }
}

/*
 * play_list(): plays a list of songs (from file or stream).
 */

int
play_list (const char *playlist, int stdn, int wait_time, int delay,
	   int start_tune, char *sidplay_args)
{
  int current_song = 1, i;
  char line[512], command[512];
  char *song, *args;
  FILE *f;

  if (stdn)
    f = stdin;
  else
    {
      if ((f = fopen (playlist, "r")) == NULL)
	{
	  usage ();
	  return 1;
	}
    }
  for (i = 0; i < start_tune - 1; i++)
    {
      current_song++;
      fgets (line, 511, f);
    }

  while (!feof (f))
    {
      if (!(fgets (line, 511, f)))
	break;
      song = strtok (line, "@\n");
      args = strtok (NULL, "@\n");
      
      if (song != NULL)
	{
	  strcpy (command, "sidplayo ");
	  if (sidplay_args)
	    {
	      strncat (command, sidplay_args, 511 - strlen (command));
	      strncat (command, " ", 511 - strlen (command));
	    }
	  if (args != NULL)
	    strncat (command, args, 511 - strlen (command));
	  strncat (command, " ", 511 - strlen (command));
	  strncat (command, "\"", 511 - strlen (command));
	  strncat (command, song, 511 - strlen (command));
	  strncat (command, "\"", 511 - strlen (command));
	  if (delay != 0)
	    {
	      system ("sidplayo");
	      sleep (delay);
	    }
	  printf ("Playing song number %d: %s\n", current_song++, song);
	  system (command);
	  if (!playwait (wait_time))
	    printf ("\n");
	}
    }
}


/*
 * play_dir(): takes care of playing the songs in a directory.
 */

int
play_dir (const char *playlist, DIR * directory, int wait_time, int delay,
	  int start_tune, char *sidplay_args)
{
  int current_song = 1, i;
  struct dirent *dir_entry;
  struct stat *file_stat;
  char line[512], command[512];

  if (!(file_stat = (struct stat *) malloc (sizeof (struct stat))))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (start_tune > 1)
    {
      for (i = 1; i < start_tune; i++)
	{
	  if ((dir_entry = readdir (directory)) == NULL)
	    return 1;
	  if (!strstr (dir_entry->d_name, SID_SUFFIX))
	    i--;
	  else
	    current_song++;
	}
    }
  while ((dir_entry = readdir (directory)) != NULL)
    {
      if (dir_entry->d_name[0] != '.')
	{
	  strcpy (line, playlist);
	  strncat (line, "/", 511 - strlen (line));
	  strncat (line, dir_entry->d_name, 511 - strlen (line));
	  stat (line, file_stat);
	  if (S_ISREG (file_stat->st_mode) &&
	      ((strcmp (SID_SUFFIX,
			line + strlen (line) -
			4) == 0) ||
	       (strcmp (SID_GZ_SUFFIX,
			line + strlen (line) - 7)
		== 0)))
	    {
	      strcpy (command, "sidplayo ");
	      if (sidplay_args)
		{
		  strncat (command, sidplay_args, 511 - strlen (command));
		  strncat (command, " ", 511 - strlen (command));
		}
	      strncat (command, "\"", 511 - strlen (command));
	      strncat (command, line, 511 - strlen (command));
	      strncat (command, "\"", 511 - strlen (command));
	      if (delay != 0)
		{
		  system ("sidplayo");
		  sleep (delay);
		}
	      printf ("Playing song number %d: %s\n", current_song++, line);
	      system (command);
	      if (!playwait (wait_time))
		printf ("\n");
	    }
	}
    }
  closedir (directory);
}

/*
 * Function: playwait
 *   Waits for delay seconds, unless a key is pressed, in which case it
 *   returns immediately.
 */

int
playwait (int wait_time)
{
  fd_set set;
  struct timeval timeout;
  int ret = 0;
  char shit[2049];

  FD_ZERO (&set);
  FD_SET (STDERR_FILENO, &set);

  timeout.tv_sec = wait_time;
  timeout.tv_usec = 0;

  if ((ret = select (FD_SETSIZE, &set, NULL, NULL, &timeout)) == 0)
    return 0;
  else if (ret == 1)
    {
      read (STDERR_FILENO, shit, 2048);
      return 1;
    }
}

/*
 * Function: usage
 *   Prints out usage information.
 */

void
usage (void)
{
  printf ("USAGE: sidlist [options] <listfile|dir|->\n");
  printf ("-h | --help                   Print this message\n");
  printf ("-t | --time-to-wait=<seconds> Time to play each tune\n");
  printf ("-r | --random                 Play random tunes\n");
  printf ("-s | --start-tune=<number>    Number of tune to start with\n");
  printf ("-d | --delay=<seconds>        Delay between tunes\n");
  printf ("-a | --args=<arguments>       Arguments to SID player\n");
  printf ("-b | --database=<database>    Use database file other than default\n");
}
