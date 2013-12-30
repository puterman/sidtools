/*************************************************************************
 * search2list by Linus Åkerlund, uxm165t@tninet.se
 *
 * Very much inspired by Per Bolmstedt's CGI sid search.
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
 **************************************************************************/

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <regex.h>
#include "parsecfg.h"
#include "sidtools.h"
#include "search2list.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif

/*
 * Function: main
 *   Parses arguments, searches the database, prints out the hits to stdout.
 */

int
main (int argc, char **argv)
{
  regex_t *r_author, *r_title, *r_copy;
  int i, j, fields = 0, hit, db_size_i, db_songs_i;
  int db_start_i, db_load_i, db_init_i, db_play_i;
  struct bounds songs = {0, 0, 0, 0, 0, 0},
			  size = {0, 0, 0, 0, 0, 0};
  struct bounds start = {0, 0, 0, 0, 0, 0},
			  load = {0, 0, 0, 0, 0, 0};
  struct bounds init = {0, 0, 0, 0, 0, 0},
			 play = {0, 0, 0, 0, 0, 0};
  int db_fields = 0, sidinfo = 0, stilinfo = 0, cfg_trash_int;
  FILE *database, *db;
  char *db_file;
  char *db_author, *db_title, *db_copy, *db_path;
  char *db_number, *db_start, *db_load, *db_init, *db_play, *db_size;
  char *db_line, *db_line_2;
  size_t n = 500;
  char *temp, *cfg_trash, *cfg_file;
  char temp_stil[4097];

  cfgStruct cfg[] =
  {
    { "SIDDB", CFG_STRING, &db_file },
    { "HVSC_BASE", CFG_STRING, &cfg_trash },
    { "SIDPLAY", CFG_STRING, &cfg_trash },
    { "SIDPLAY_ARGS", CFG_STRING, &cfg_trash },
    { "PORT", CFG_INT, &cfg_trash_int },
    { NULL, CFG_END, NULL }
  };
  
  struct option opts[] =
  {
    {"author", required_argument, NULL, 'a'},
    {"title", required_argument, NULL, 't'},
    {"copyright", required_argument, NULL, 'c'},
    {"database", required_argument, NULL, 'd'},
    {"number-of-tunes", required_argument, NULL, 'n'},
    {"start-tune", required_argument, NULL, 's'},
    {"load-address", required_argument, NULL, 'l'},
    {"init-address", required_argument, NULL, 'i'},
    {"play-address", required_argument, NULL, 'p'},
    {"file-size", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"extra-information", no_argument, NULL, 'e'},
    {"stil-information", no_argument, NULL, 'z'},
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

  

  /*
  cfg_file = strdup (getenv ("HOME"));

  if (!(cfg_file = realloc (cfg_file, sizeof (char) * strlen (cfg_file) +
			    strlen (RC_FILE))))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }
  
  strncat (cfg_file, RC_FILE, 255 - strlen (cfg_file));

  cfgParse (cfg_file, cfg, CFG_SIMPLE);
  */
  
  while (1)
    {
      i = getopt_long (argc, argv, "a:t:c:d:n:s:l:i:p:f:hez",
		       opts, &option_index);

      if (i == -1)
	break;

      switch (i)
	{
	case 'h':
	  usage ();
	  return 1;

	case 'e':
	  sidinfo = 1;
	  break;

	case 'z':
	  stilinfo = 1;
	  break;

	case 'a':
	  arg = strdup (optarg);
	  if (!(r_author = (regex_t *) malloc (sizeof (regex_t))))
	    {
	      fprintf (stderr, "Can't allocate memory!\n");
	      exit (1);
	    }
	  if (regcomp (r_author, arg, REG_EXTENDED | REG_ICASE))
	    {
	      fprintf (stderr, "Error compiling regexp!\n");
	      return 1;
	    }
	  if (db_fields < 2)
	    db_fields = 2;
	  fields = fields | 1;
	  break;

	case 't':
	  arg = strdup (optarg);
	  if (!(r_title = (regex_t *) malloc (sizeof (regex_t))))
	    {
	      fprintf (stderr, "Can't allocate memory!\n");
	      exit (1);
	    }
	  if (regcomp (r_title, arg, REG_EXTENDED | REG_ICASE))
	    {
	      fprintf (stderr, "Error compiling regexp!\n");
	      return 1;
	    }
	  if (db_fields < 3)
	    db_fields = 3;
	  fields = fields | 2;
	  break;

	case 'c':
	  arg = strdup (optarg);
	  if (!(r_copy = (regex_t *) malloc (sizeof (regex_t))))
	    {
	      fprintf (stderr, "Can't allocate memory!\n");
	      exit (1);
	    }
	  if (regcomp (r_copy, arg, REG_EXTENDED | REG_ICASE))
	    {
	      fprintf (stderr, "Error compiling regexp!\n");
	      return 1;
	    }
	  if (db_fields < 4)
	    db_fields = 4;
	  fields = fields | 4;
	  break;

	case 'd':
	  db_file = strdup (optarg);
	  if (db_fields < 1)
	    db_fields = 1;
	  break;

	case 'n':
	  arg = strdup (optarg);
	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      songs.least = strtol (arg + 1, NULL, 10);
	      songs.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      songs.most = strtol (arg + 1, NULL, 10);
	      songs.most_used = 1;
	    }
	  else
	    {
	      songs.exact = strtol (arg, NULL, 10);
	      songs.exact_used = 1;
	    }
	  if (db_fields < 5)
	    db_fields = 5;
	  break;

	case 's':
	  arg = strdup (optarg);
	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      start.least = strtol (arg + 1, NULL, 10);
	      start.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      start.most = strtol (arg + 1, NULL, 10);
	      start.most_used = 1;
	    }
	  else
	    {
	      start.exact = strtol (arg, NULL, 10);
	      start.exact_used = 1;
	    }
	  if (db_fields < 6)
	    db_fields = 6;
	  break;

	case 'l':
	  arg = strdup (optarg);

	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      load.least = strtol (arg + 1, NULL, 16);
	      load.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      load.most = strtol (arg + 1, NULL, 16);
	      load.most_used = 1;
	    }
	  else
	    {
	      load.exact = strtol (arg, NULL, 16);
	      load.exact_used = 1;
	    }
	  if (db_fields < 7)
	    db_fields = 7;
	  break;

	case 'i':
	  arg = strdup (optarg);

	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      init.least = strtol (arg + 1, NULL, 16);
	      init.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      init.most = strtol (arg + 1, NULL, 16);
	      init.most_used = 1;
	    }
	  else
	    {
	      init.exact = strtol (arg, NULL, 16);
	      init.exact_used = 1;
	    }
	  if (db_fields < 8)
	    db_fields = 8;
	  break;

	case 'p':
	  arg = strdup (optarg);
	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      play.least = strtol (arg + 1, NULL, 16);
	      play.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      play.most = strtol (arg + 1, NULL, 16);
	      play.most_used = 1;
	    }
	  else
	    {
	      play.exact = strtol (arg, NULL, 16);
	      play.exact_used = 1;
	    }
	  if (db_fields < 9)
	    db_fields = 9;
	  break;

	case 'f':
	  arg = strdup (optarg);
	  if (strlen (arg) > 0 && (arg[0]) == '>')
	    {
	      size.least = strtol (arg + 1, NULL, 10);
	      size.least_used = 1;
	    }
	  else if (strlen (arg) > 0 && (arg[0]) == '<')
	    {
	      size.most = strtol (arg + 1, NULL, 10);
	      size.most_used = 1;
	    }
	  else
	    {
	      size.exact = strtol (arg, NULL, 10);
	      size.exact_used = 1;
	    }
	  if (db_fields < 10)
	    db_fields = 10;
	  break;

	case '?':
	  /* getopt_long has already printed an error message */
	  usage ();
	  return 1;

	default:
	  abort ();
	}
    }

  if (argc < 2)
    {
      usage ();
      exit (1);
    }

  if (db_fields == 0)
    {
      usage ();
      exit (1);
    }

  if ((database = fopen (db_file, "r")) == NULL)
    {
      fprintf (stderr, "Error opening database file! Check if the path is\n");
      fprintf (stderr, "correctly set in search2list.h and recompile,\n");
      fprintf (stderr, "or specify the location of the database file with\n");
      fprintf (stderr, "-d <path>.\n");
      exit (1);
    }
  if (!(db_line = (char *) malloc (512)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(db_line_2 = (char *) malloc (512)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(db_path = (char *) malloc (256)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(db_author = (char *) malloc (38)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(db_title = (char *) malloc (38)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(db_copy = (char *) malloc (38)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }

  if (db_fields > 4)
    {
      if (!(db_number = (char *) malloc (5)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
      if (!(db_start = (char *) malloc (5)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
      if (!(db_load = (char *) malloc (5)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
      if (!(db_init = (char *) malloc (5)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
      if (!(db_play = (char *) malloc (5)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
      if (!(db_size = (char *) malloc (8)))
	{
	  fprintf (stderr, "Can't allocate memory!\n");
	  exit (1);
	}
    }

  /* Reads and throws away the first line (number of tunes in database). */
  fgets (db_line, n, database);

  while (!feof (database))
    {
      hit = 0;
      fgets (db_line, n, database);
      fgets (temp_stil, 4096, database);
      strcpy (db_line_2, db_line);
      if ((db_title = strtok (db_line, SEP)) == NULL)
	break;
      if ((db_author = strtok (NULL, SEP)) == NULL)
	break;
      if ((db_copy = strtok (NULL, SEP)) == NULL)
	break;
      if ((db_path = strtok (NULL, SEP)) == NULL)
	break;
      if (db_path == NULL)
	continue;

      if (db_fields > 4)
	{
	  db_number = strtok (NULL, SEP);
	  db_songs_i = strtol (db_number, NULL, 10);
	  db_start = strtok (NULL, SEP);
	  db_start_i = strtol (db_start, NULL, 16);
	  db_load = strtok (NULL, SEP);
	  db_load_i = strtol (db_load, NULL, 16);
	  db_init = strtok (NULL, SEP);
	  db_init_i = strtol (db_init, NULL, 16);
	  db_play = strtok (NULL, SEP);
	  db_play_i = strtol (db_play, NULL, 16);
	  db_size = strtok (NULL, SEP);
	  db_size_i = strtol (db_size, NULL, 10);
	}

      /*
       * This is where most searches will happen, so do as little work as
       *   possible.
       */

      if (db_fields < 5)
	{
	  switch (fields)
	    {
	    case 1:
	      if (!regexec (r_author, db_author, 0, NULL, 0))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 2:
	      if (!regexec (r_title, db_title, 0, NULL, 0))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 3:
	      if ((!regexec (r_title, db_title, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 4:
	      if (!regexec (r_copy, db_copy, 0, NULL, 0))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 5:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 6:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_title, db_title, 0, NULL, 0)))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    case 7:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_title, db_title, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		{
		  printf ("%s\n", db_path);
		  if (sidinfo)
		    sid_info (db_line_2, stilinfo, database);
		  hit = 1;
		}
	      break;
	    default:
	      printf ("This is a bug, please report to uxm165t@tninet.se\n");
	      break;
	    }
	}
      else
	{
	  switch (fields)
	    {
	    case 0:
	      hit = 1;
	      break;
	    case 1:
	      if (!regexec (r_author, db_author, 0, NULL, 0))
		hit = 1;
	      break;
	    case 2:
	      if (!regexec (r_title, db_title, 0, NULL, 0))
		hit = 1;
	      break;
	    case 3:
	      if ((!regexec (r_title, db_title, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		hit = 1;
	      break;
	    case 4:
	      if (!regexec (r_copy, db_copy, 0, NULL, 0))
		hit = 1;
	      break;
	    case 5:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		hit = 1;
	      break;
	    case 6:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_title, db_title, 0, NULL, 0)))
		hit = 1;
	      break;
	    case 7:
	      if ((!regexec (r_copy, db_copy, 0, NULL, 0)) &&
		  (!regexec (r_title, db_title, 0, NULL, 0)) &&
		  (!regexec (r_author, db_author, 0, NULL, 0)))
		hit = 1;
	      break;
	    default:
	      printf ("This is a bug, please report to uxm165t@tninet.se\n");
	      break;
	    }
	  /* This isn't pretty, and will be rewritten sometime... */
	  if ((hit == 1) &&
	      ((db_start_i == start.exact) || !(start.exact_used)) &&
	      ((db_start_i <= start.most) || !(start.most_used)) &&
	      ((db_start_i >= start.least) || !(start.least_used)) &&
	      ((db_load_i == load.exact) || !(load.exact_used)) &&
	      ((db_load_i <= load.most) || !(load.most_used)) &&
	      ((db_load_i >= load.least) || !(load.least_used)) &&
	      ((db_init_i == init.exact) || !(init.exact_used)) &&
	      ((db_init_i <= init.most) || !(init.most_used)) &&
	      ((db_init_i >= init.least) || !(init.least_used)) &&
	      ((db_play_i == play.exact) || !(play.exact_used)) &&
	      ((db_play_i <= play.most) || !(play.most_used)) &&
	      ((db_play_i >= play.least) || !(play.least_used)) &&
	      ((db_songs_i == songs.exact) || !(songs.exact_used)) &&
	      ((db_songs_i <= songs.most) || !(songs.most_used)) &&
	      ((db_songs_i >= songs.least) || !(songs.least_used)) &&
	      ((db_size_i == size.exact) || !(size.exact_used)) &&
	      ((db_size_i <= size.most) || !(size.most_used)) &&
	      ((db_size_i >= size.least) || !(size.least_used)))
	    {
	      printf ("%s\n", db_path);
	      if (sidinfo)
		sid_info (db_line_2, stilinfo, database);
	    }
	  else
	    hit = 0;
	}
      if (hit && stilinfo)
	{
	  if (strlen (temp_stil) > 3)
	    {
	      for (j = 0; j < strlen (temp_stil); j++)
		if (temp_stil[j] == STIL_SEP_CHAR)
		  temp_stil[j] = '\n';
	      printf ("%s", temp_stil);
	    }
	  printf ("-------------------------------\n");
	}
      else if (hit && sidinfo)
	printf ("-------------------------------\n");
    }
  fclose (database);
  return 0;
}

/*
 * sid_info(): Gives you all the info available on a sid, i.e. all that is
 *   stored in the database.
 */

void
sid_info (char *line, int stilinfo, FILE * database)
{
  char *titl, *auth, *copy, *path, *laddr, *iaddr, *paddr, *fsize;
  char *nrsongs, *startsong;

  if (!(path = (char *) malloc (256)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(titl = (char *) malloc (33)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(auth = (char *) malloc (33)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(copy = (char *) malloc (33)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(laddr = (char *) malloc (5)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(iaddr = (char *) malloc (5)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(paddr = (char *) malloc (5)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(fsize = (char *) malloc (6)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(nrsongs = (char *) malloc (5)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }
  if (!(startsong = (char *) malloc (5)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }

  titl = strtok (line, SEP);
  auth = strtok (NULL, SEP);
  copy = strtok (NULL, SEP);
  path = strtok (NULL, SEP);
  nrsongs = strtok (NULL, SEP);
  startsong = strtok (NULL, SEP);
  laddr = strtok (NULL, SEP);
  iaddr = strtok (NULL, SEP);
  paddr = strtok (NULL, SEP);
  fsize = strtok (NULL, SEP);

  printf ("Title: \t\t%s\n", titl);
  printf ("Author: \t%s\n", auth);
  printf ("Copyright: \t%s\n", copy);
  printf ("Nr of songs: \t%s\tDefault start song: %s\n",
	  nrsongs, startsong);
  printf ("Load address: \t%s\tInit address: %s\tplay address: %s\n",
	  laddr, iaddr, paddr);
  printf ("File size: \t%s bytes\n", fsize);
}

/*
 * Function: usage
 *   Prints usage information.
 */

void
usage (void)
{
  printf ("Usage: search2list <options>\n");
  printf ("The available options are:\n");

  printf (" -h | --help\t\t\t\tPrint this usage message\n");
  printf (" -e | --extra-information\t\tdisplays all info from sid header.\n");
  printf (" -z | --stil-information\t\tdisplays info from STIL.\n");
  printf (" -d | --database=<file>\t\t\tdatabase file\n");
  printf (" -t | --title=<title regexp>\t\tsong title\n");
  printf (" -a | --author=<author regexp>\t\tauthor\n");
  printf (" -c | --copyright=<copyright regexp>\tcopyright information\n");
  printf (" -n | --number-of-tunes=<number>\tnumber of songs\n");
  printf (" -n<| --number-of-tunes=<<number>\tminimum number of songs\n");
  printf (" -n>| --number-of-tunes=><number>\tmaximum number of songs\n");
  printf (" -s | --start-tune=<number>\t\tdefault start song\n");
  printf (" -s<| --start-tune=<<number>\t\tminimum default start song\n");
  printf (" -s>| --start-tune=><number>\t\tmaximum default start song\n");
  printf (" -l | --load-address=<hex number>\tload address\n");
  printf (" -l<| --load-address=<<hex number>\tminimum load address\n");
  printf (" -l>| --load-address=><hex number>\tmaximum load address\n");
  printf (" -i | --init-address=<hex number>\tinit address\n");
  printf (" -i<| --init-address=<<hex number>\tminimum init address\n");
  printf (" -i>| --init-address=><hex number>\tmaximum init address\n");
  printf (" -p | --play-address=<hex number>\tplay address\n");
  printf (" -p<| --play-address=<<hex number>\tminimum play address\n");
  printf (" -p>| --play-address=><hex number>\tmaximum play address\n");
  printf (" -f | --file-size=<number>\t\tfile size\n");
  printf (" -f<| --file-size=<<number>\t\tminimum file size\n");
  printf (" -f>| --file-size=><number>\t\tmaximum file size\n");

  printf ("Read the texinfo documentation for details.\n");
}

/*
 * Function: str_tolower
 *   Lowercases a string.
 */

void
str_tolower (char *str)
{
  int i;

  for (i = 0; i < strlen (str); ++i)
    if (isupper (str[i]))
      str[i] = tolower (str[i]);
}
