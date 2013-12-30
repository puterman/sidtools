/*************************************************************************
 * File: makelist.c
 * A rewrite of the Perl version of makelist.  Recurses through the
 *   directories and extracts info from the sids, saves all info in a
 *   database text file, or prints it to stdout.
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
 *************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include "parsecfg.h"
#include "sidtools.h"
#include "makelist.h"

char write_rc = 1;
char *sids_base_dir;

struct stil_entry *stil_top = NULL;
struct db_entry *db_top = NULL;

/*
 * Function: main
 *   Parses arguments, calls recurs().
 */

int
main (int argc, char **argv)
{
  char **dir;
  int dir_counter = 0;
  char *target = NULL;
  FILE *db_file;
  int number_of_files = 0;
  int number_files_temp = 0;
  int i, stil = 1;
  char *cfg_trash, *siddb, *hvsc_base, *cfg_file;
  int cfg_port;
  
  cfgStruct cfg[] =
  {
    { "SIDDB", CFG_STRING, &target },
    { "HVSC_BASE", CFG_STRING, &hvsc_base },
    { "SIDPLAY", CFG_STRING, &cfg_trash },
    { "SIDPLAY_ARGS", CFG_STRING, &cfg_trash },
    { "PORT", CFG_INT, &cfg_port },
    { NULL, CFG_END, NULL }
  };

  if (!(cfg_file = malloc (sizeof (char) * (strlen (getenv ("HOME")) +
					    strlen (RC_FILE) + 2))))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }

  strcpy (cfg_file, getenv ("HOME"));
  strncat (cfg_file, RC_FILE, 255 - strlen (cfg_file));
  
  if (db_file = fopen (cfg_file, "r"))
    {
      cfgParse (cfg_file, cfg, CFG_SIMPLE);
      fclose (db_file);
    }
  else
    {
      free (cfg_file);
      cfg_file = strdup (CONFIGDIR);

      if (db_file = fopen (cfg_file, "r"))
	{
	  cfgParse (cfg_file, cfg, CFG_SIMPLE);
	  fclose (db_file);
	}
      else
	{
	  fprintf (stderr, "ERROR: No conf file in $HOME%s or %s!\n",
		   RC_FILE, CONFIGDIR);
	  exit (1);
	}
    }
  
  if (argc < 2)
    {
      usage (argv[0]);
      return 1;
    }

  if (!(dir = (char **) malloc (sizeof (char **) * argc - 1)))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }

  for (i = 1; i < argc; i++)
    {
      if (!strncmp (argv[i], "-o", 2))
	{
	  if (argc > i)
	    target = strdup (argv[++i]);
	  else
	    {
	      usage (argv[0]);
	      return 1;
	    }
	}
      else if (!strncmp (argv[i], "-h", 2) || !strncmp (argv[i], "--h", 3))
	{
	  usage (argv[0]);
	  exit (0);
	}
      else if (!strncmp (argv[i], "-n", 2) || !strncmp (argv[i], "--n", 3))
	write_rc = 0;
      else
	dir[dir_counter++] = get_full_path (argv[i], dir[dir_counter]);
    }

  sids_base_dir = get_full_path (dir[0], sids_base_dir);

  if (!(db_file = fopen (target, "w")))
    {
      fprintf (stderr, "%s: Couldn't open %s for writing.\n", argv[0], target);
      return 1;
    }

  for (i = 0; i < dir_counter; i++)
    {
      if ((number_files_temp = recurs (dir[i])) < 0)
	{
	  fprintf (stderr, "%s: can't read directory %s\n", argv[0], dir[i]);
	  return 1;
	}
      else
	number_of_files += number_files_temp;
    }

  fclose (db_file);

  if (stil_parse (sids_base_dir))
    {
      fprintf (stderr, "Couldn't process STIL info...\n");
      stil = 0;
    }

  make_db (target, number_of_files, cfg_file, sids_base_dir);

  return 0;
}

/*
 * Function: recurs
 *   Recurses through a directory tree and extracts info from all sids in
 *   it (sidparse()). 
 */

int
recurs (char *dir)
{
  struct dirent *dir_entry;
  DIR *directory;
  struct stat *file_stat;
  char current_file[256];
  int number_of_files = 0;

  if (!(file_stat = (struct stat *) malloc (sizeof (struct stat))))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }

  if ((directory = opendir (dir)) == NULL)
    return -1;
  while ((dir_entry = readdir (directory)) != NULL)
    {
      if (dir_entry->d_name[0] != '.')
	{
	  strcpy (current_file, dir);

	  /* FIXME: does this work everywhere? */
	  if (current_file[strlen (current_file) - 1] != '/')
	    strncat (current_file, "/", 255 - strlen (current_file));
	  strncat (current_file, dir_entry->d_name,
		   255 - strlen (current_file));
	  stat (current_file, file_stat);
	  if (S_ISDIR (file_stat->st_mode))
	    {
	      printf ("Moving into %s\n", current_file);
	      number_of_files += recurs (current_file);
	    }
	  else if (S_ISREG (file_stat->st_mode) &&
		   (strcmp (SID_SUFFIX,
			    current_file + strlen (current_file) - 4) == 0))
	    {
	      ++number_of_files;
	      if (sidparse (current_file, file_stat->st_size))
		--number_of_files;
	    }
	}
    }
  free (file_stat);
  closedir (directory);
  return number_of_files;
}

/*
 * function: sidparse
 *   Extracts info about author, title and copyright from a sid, and adds
 *   the path to it.  Now also extracts further information, like the
 *   number of songs, default song, load address, init address, play
 *   address and also the file size.
 *   For into on the sid header format, see
 *   http://www.geocities.com/SiliconValley/Lakes/5147/sidplay/info.html
 *   For info on the sidtools database format, se the texinfo docs.
 */

int
sidparse (char *current_file, int size)
{
  int i, c;
  unsigned char str[144];
  FILE *f;
  int data_offset;
  int nr_of_songs, default_song, load_addr, init_addr, play_addr;
  struct db_entry *entry;

  if (!(entry = (struct db_entry *) malloc (sizeof (struct db_entry))))
    {
      fprintf (stderr, "ERROR: Couldn't allocate memory\n");
      exit (1);
    }

  if ((f = fopen (current_file, "rb")) == NULL)
    {
      fprintf (stderr, "WARNING: Couldn't open file %s for reading.\n",
	       current_file);
      return 1;
    }

  for (i = 0; i < 128; i++)
    if ((c = fgetc (f)) == EOF)
      {
	fprintf (stderr, "WARNING: File %s is weird, shorter than 128 bytes.\n",
		 current_file);
	return 1;
      }
    else
      str[i] = c;
  fclose (f);

  if (strncmp (str, "PSID", 4))
    {
      fprintf (stderr, "WARNING: File %s does not have a valid PSID header.\n",
	       current_file);
      return 1;
    }

  data_offset = (str[6] * 256) + str[7];
  entry->nr_songs = (str[14] * 256) + str[15];
  entry->start_song = (str[16] * 256) + str[17];
  entry->load_addr = (str[9] * 256) + str[8];
  entry->init_addr = (str[10] * 256) + str[11];
  entry->play_addr = (str[12] * 256) + str[13];

  if (entry->load_addr == 0)
    entry->load_addr = (str[data_offset + 1] * 256) + str[data_offset];

  if (entry->init_addr == 0)
    entry->init_addr = entry->load_addr;

  if (entry->play_addr == 0)
    entry->play_addr = entry->load_addr;

  if (!(str + 22) || !(str + 54) || !(str + 86))
    {
      fprintf (stderr, "WARNING: File %s seems to be corrupt\n",
	       current_file);
      return 1;
    }

  entry->title = strdup (str + 22);
  entry->author = strdup (str + 54);
  entry->copy = strdup (str + 86);
  entry->file = strdup (current_file);
  entry->size = size;

  /* Push the new entry on top of the stack */
  entry->next = db_top;
  db_top = entry;

  return 0;
}

/*
 * stil_parse: makes a line for each STIL entry and saves to a temorary file.
 */

int
stil_parse (char *sids_base_dir)
{
  char *stil_path;
  char *out_file;
  FILE *stil;
  char line[4097];
  char temp[83];
  char finish = 0, end_of_line = 0, i = 0;
  struct stil_entry *new;

  if (!(stil_path = (char *) malloc ((strlen (sids_base_dir) + 20) *
				     sizeof (char))))
    {
      fprintf (stderr, "Error: Couldn't allocate memory!\n");
      exit (1);
    }
  strcpy (stil_path, sids_base_dir);
  strcat (stil_path, "/DOCUMENTS/STIL.txt");

  if (!(stil = fopen (stil_path, "r")))
    {
      fprintf (stderr, "Error: Couldn't open %s for reading!\n", stil_path);
      return 1;
    }

  while (!feof (stil))
    {
      end_of_line = 0;
      if (!(fgets (temp, 82, stil)))
	finish = 1;
      else if (temp[0] == '#')
	continue;
      else if (strlen (temp) < 3)
	end_of_line = 1;
      for (i = 0; i < strlen (temp); i++)
	{
	  if (temp[i] == 0x0d)
	    temp[i] = ' ';
	  else if (temp[i] == 0x0a)
	    temp[i] = STIL_SEP_CHAR;
	}
      if (end_of_line)
	{
	  if (!(new = (struct stil_entry *) malloc (sizeof (struct stil_entry))))
	    {
	      fprintf (stderr, "ERROR: Couldn't allocate memory\n");
	      exit (1);
	    }

	  if (line[0] == '/' && strlen (line) > 3)
	    {
	      new->entry = strdup (line);

	      /* Push the new entry on the stack */
	      new->next = stil_top;
	      stil_top = new;
	    }

	  line[0] = 0;
	  end_of_line = 0;
	  if (finish)
	    break;
	}
      else
	strncat (line, temp, 4096 - strlen (line));
    }

  fclose (stil);

  return 0;
}

/* 
 * Function: make_db
 *   Adds the number of sids to the top of the database file.
 */

void
make_db (char *file_name, int number_of_files, char *rc_file,
	 char *sids_base_dir)
{
  FILE *in;
  FILE *out;
  FILE *stil_f;
  char *new_name, *new_rc, *new_rc_temp;
  char *full_file_name;
  char db_line[513];
  char *stil_name;
  struct db_entry *db_last;
  struct stil_entry *stil_temp = stil_top, *stil_last = stil_top;
  int base_dir_len;
  char hit = 0;
  int i, read_ok;

  printf ("Merging STIL and sid info.  This takes about 15 seconds\n");
  printf ("on my P200 with 32 MB RAM when creating a database with the\n");
  printf ("whole HVSC and some more sids.  YMMV.\n");

  base_dir_len = strlen (sids_base_dir);

  full_file_name = get_full_path (file_name, full_file_name);

  if (!(new_name = (char *) malloc (strlen (file_name) + 6)))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }

  strcpy (new_name, file_name);
  strcat (new_name, "-temp");

  if ((out = fopen (new_name, "w")) == NULL)
    {
      fprintf (stderr, "ERROR: Can't open temporary file for writing.\n");
      exit (1);
    }

  fprintf (out, "%d\n", number_of_files);

  while (db_top)
    {
      fprintf (out, "%s%s%s%s%s%s%s%s%d%s%d%s%04x%s%04x%s%04x%s%d%s\n",
	       db_top->title, SEP, db_top->author, SEP, db_top->copy, SEP,
	       db_top->file, SEP, db_top->nr_songs, SEP, db_top->start_song, SEP,
	       db_top->load_addr, SEP, db_top->init_addr, SEP, db_top->play_addr,
	       SEP, db_top->size, SEP);

      stil_temp = stil_top;
      hit = 0;

      while (stil_temp)
	{
	  if (!strncmp (db_top->file + base_dir_len, stil_temp->entry,
		     strchr (stil_temp->entry, '.') - stil_temp->entry + 1))
	    {
	      fprintf (out, "%s\n", stil_temp->entry);

	      /* Remove this STIL entry from the list */
	      if (stil_temp == stil_top)
		{
		  stil_last = stil_temp->next;
		  free (stil_temp->entry);
		  free (stil_temp);
		  stil_top = stil_last;
		}
	      else
		{
		  stil_last->next = stil_temp->next;
		  free (stil_temp->entry);
		  free (stil_temp);
		}
	      hit = 1;
	      break;
	    }
	  stil_last = stil_temp;
	  stil_temp = stil_temp->next;
	}
      if (!hit)
	fprintf (out, "\n");

      db_last = db_top;
      db_top = db_top->next;

      free (db_last->author);
      free (db_last->title);
      free (db_last->copy);
      free (db_last->file);
      free (db_last);
    }

  fclose (out);
  unlink (file_name);
  if (rename (new_name, file_name) == -1)
    {
      fprintf (stderr, "ERROR: Couldn't rename file.\n");
      exit (1);
    }

  /* FIXME */

  new_rc = strdup (getenv ("HOME"));
  
  if (!(new_rc = realloc (new_rc, sizeof (char) * strlen (new_rc) +
			    strlen (RC_FILE))))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }
  
  strncat (new_rc, RC_FILE, 255 - strlen (new_rc));

  if (!(new_rc_temp = malloc ((strlen (rc_file) + 6) * sizeof (char))))
    {
      fprintf (stderr, "ERROR: Can't allocate memory!\n");
      exit (1);
    }

  strcpy (new_rc_temp, new_rc);
  strcat (new_rc_temp, "-temp");

  if (write_rc)
    {
      if ((in = fopen (rc_file, "r")) == NULL)
	read_ok = 0;
      else
	read_ok = 1;

      if ((out = fopen (new_rc_temp, "w")) == NULL)
	{
	  fprintf (stderr, "ERROR: Couldn't open %s for writing.\n",
		   new_rc_temp);
	  exit (1);
	}

      if (read_ok)
	{      
	  while (!feof (in))
	    {
	      if (!(fgets (db_line, 512, in)))
		break;
	      if (!strstr (db_line, "SIDDB"))
		fprintf (out, "%s", db_line);
	      else if (strstr (db_line, "HVSC_BASE"))
		fprintf (out, "HVSC_BASE = %s\n", sids_base_dir);
	      else if (strstr (db_line, "SIDDB"))
		fprintf (out, "SIDDB = %s\n", full_file_name);
	    }
	  fclose (in);
	}
      else
	{
	  fprintf (out, "HVSC_BASE = %s\n", sids_base_dir);
	  fprintf (out, "SIDDB = %s\n", full_file_name);
	}      

      fclose (out);
      
      if (rename (new_rc_temp, new_rc) == -1)
	fprintf (stderr, "rename failed\n");
    }
  
}

  
/*
 * usage: prints usage information.
 */

void
usage (char *filename)
{
  printf ("USAGE: %s <dirs> [ -o <db_file> ]\n", filename);
  printf ("       %s -h|--help prints this usage message.\n", filename);
  printf ("          -n|--no-rc means the rc file will not be written\n");
}

/*
 * get_full_path: return the absolute path to filename.  full_path is
 *   a char pointer without allocated memory.
 */

char *
get_full_path (char *filename, char *full_path)
{
  int path_up_count = 0;

  while (!strncmp (filename, "..", 2))
    {
      if (filename[2] == '/')
	filename = filename + 3;
      else if (!strcmp ("..", filename))
	strcpy (filename, "");
      else
	break;
      path_up_count++;
    }

  if (filename[0] == '/')
    {
      if (filename[strlen (filename) - 1] == '/')
	filename[strlen (filename) - 1] = '\0';
      return filename;
    }
  else if (!strcmp (filename, "."))
    strcpy (filename, "");
  else if (filename[0] == '.' && filename[1] == '/')
    filename = filename + 2;

  if (!(full_path = (char *) malloc ((516 + strlen (filename))* sizeof (char))))
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (1);
    }

  if (!(full_path = getcwd (full_path, 510 - strlen (filename))))
    {
      fprintf (stderr, "Couldn't get current working directory...\n");
      exit (1);
    }

  if (path_up_count)
    {
      for (; path_up_count > 0; path_up_count--)
	full_path[strrchr (full_path, '/') - full_path] = '\0';
    }

  strcat (full_path, "/");
  strcat (full_path, filename);

  if (full_path[strlen (full_path) - 1] == '/')
    full_path[strlen (full_path) - 1] = '\0';

  return full_path;
}
