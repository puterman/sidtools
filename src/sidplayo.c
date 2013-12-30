/**********************************************************************
 * File: sidplayo.c
 *
 * A frontend to sidplay.  Plays a song and displays info.  Exits as
 *   soon as it's started sidplay.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <parsecfg.h>
#include <assert.h>
#include "sidtools.h"
#include "sidplayo.h"

/*
 * Function: main
 *   Does most of the stuff.  Parses args, launches sidplay etc.
 */

int
main (int argc, char *argv[])
{
  FILE *tmp, *db_file;
  char info[256];
  char *command = NULL;
  int i, cmd_len = 0, cfg_trash_int;
  char *temp2, *temp, *sidplay, *sidplay_args, *cfg_trash;
  char *cfg_file;

  cfgStruct cfg[] =
  {
    { "SIDDB", CFG_STRING, &cfg_trash },
    { "HVSC_BASE", CFG_STRING, &cfg_trash },
    { "SIDPLAY", CFG_STRING, &sidplay },
    { "SIDPLAY_ARGS", CFG_STRING, &sidplay_args },
    { "PORT", CFG_INT, &cfg_trash_int },
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

  if (!sidplay)
    {
      fprintf (stderr, "ERROR: SIDPLAY not set in rc file!\n");
      exit (1);
    }

  kill_sidplayo (sidplay);
  if (argc < 2)
    return 0;
      
  if (!strcmp (argv[1], "-h")
      || !strncmp (argv[1], "--help", strlen (argv[1])))
    {
      usage (argv[0]);
      return 0;
    }
  else if (strcmp (argv[argc - 1] + strlen (argv[argc - 1]) - 4, SID_SUFFIX) == 0)
    {
      cmd_len = strlen (sidplay);
      cmd_len += strlen (sidplay_args);

      for (i = 0; i < argc; i++)
	cmd_len += strlen (argv[i]) + 4;

      if (!(command = (char *)malloc (sizeof (char) * cmd_len)))
	{
	  fprintf (stderr, "ERROR: Out of memory!\n");
	  exit (1);
	}
      
      strcpy (command, sidplay);
      strcat (command, " ");
      strcat (command, sidplay_args);
      for (i = 1; i < (argc - 1); i++)
	{
	  strcat (command, " ");
	  strcat (command, argv[i]);
	}
      strcat (command, " ");
      strcat (command, " \"");
      temp = argv[argc - 1];
      while (temp2 = strchr (temp, '\"'))
	{
	  if ((strlen (command) + (temp2 - temp)) > 511)
	    {
	      fprintf (stderr, "ERROR: Too long input, exiting.\n");
	      exit (1);
	    }
	  strcat (command, temp);
	  strcat (command, "\\\"");
	  temp = temp2 + 1;
	}
      strcat (command, temp);
      strcat (command, "\" &");
    }
  else
    {
      cmd_len = strlen (sidplay);
      cmd_len += strlen (sidplay_args);
      cmd_len += 8;
      
      for (i = 0; i < argc; i++)
	cmd_len += strlen (argv[i]) + 4;

      if (!(command = (char *)malloc (sizeof (char) * cmd_len)))
	{
	  fprintf (stderr, "ERROR: Out of memory!\n");
	  exit (1);
	}

      strcpy (command, "zcat \"");
      temp = argv[argc - 1];
      while (temp2 = strchr (temp, '\"'))
	{
	  if ((strlen (command) + (temp2 - temp)) > 511)
	    {
	      fprintf (stderr, "ERROR: Too long input, exiting.\n");
	      exit (1);
	    }
	  strcat (command, temp);
	  strcat (command, "\\\"");
	  temp = temp2 + 1;
	}
      strcat (command, temp);
      strcat (command, "\" | ");
      strcat (command, sidplay);
      strcat (command, " ");
      strcat (command, sidplay_args);
      strcat (command, " -");
      for (i = 1; i < (argc - 1); i++)
	{
	  strcat (command, argv[i]);
	  strcat (command, " ");
	}
      strcat (command, " &");
    }

  if (!strstr (command, SID_SUFFIX))
    {
      usage (argv[0]);
      return 1;
    }
  
  assert (cmd_len > strlen (command));

  tmp = popen (command, "r");

  if (!tmp)
    {
      fprintf(stderr, "Couldn't run sidplay!\n");
      exit(1);
    }
  
  while (!feof (tmp))
    {
      fgets (info, 255, tmp);
      if (!info)
	break;
      if (strstr (info, "^C") != 0)
	break;
      else if ((strstr (info, "-------") == 0) &&
	       (strcmp (info, "\n")))
	printf ("%s", info);
      else
	continue;  
    }
  pclose (tmp);
  printf ("\n");
  return 0;
}

/*
 * Function: kill_sidplayo
 *   Kills any old sidplayo processes.
 */

void
kill_sidplayo (char *sidplay)
{
  FILE *temp;
  char *info;
  char *kill_command;
  char process_id[256];
  int i, j, kill_cmd_len = 0, info_len;

  assert(sidplay != NULL);
  assert(strlen(sidplay) > 0);

  temp = popen (PS_CMD, "r"); 
  if (!temp)
    {
      fprintf(stderr, "Couldn't run ps\n");
      exit(1);
    }
  
  info_len = strlen (sidplay) + 256;
  
  if (!(info = (char *)malloc (sizeof (char) * (info_len + 1))))
    {
      fprintf (stderr, "ERROR: Out of memory!\n");
      exit (1);
    }
  
  while (!feof (temp))
    {
      fgets (info, info_len, temp);
      if ((strrchr (sidplay, '/') &&
	  (strstr (info, (strrchr (sidplay, '/'))+1) != 0) &&
	  (!strstr (info, "sidplayo"))) ||
	  ((strstr (info, sidplay) != 0) &&
	   !strstr (info, "sidplayo")))
	{
	  for (i = 0; isspace (info[i]) || isdigit (info[i]) && i < info_len;
	       i++)
	    process_id[i] = info[i];
	  process_id[i] = '\0';

	  kill_cmd_len = strlen (sidplay) + strlen (process_id) + 10;
	  
	  if (!(kill_command = (char *)malloc (sizeof (char) * kill_cmd_len)))
	    {
	      fprintf (stderr, "ERROR: Out of memory!\n");
	      exit (1);
	    }
	  	  
	  strcpy (kill_command, "kill ");
	  strncat (kill_command, process_id, info_len - strlen (kill_command));
	  system (kill_command);

	  assert (kill_cmd_len > strlen (kill_command));
	  
	  free (kill_command);
	  break;
	}
    }
  free (info);
  pclose (temp);
}

void
usage (char *binary)
{
  printf ("USAGE: %s [-h] [args to sidplay] [sid-tune]\n", binary);
  printf ("If no arguments are given sidplayo stops the tune currently playing.\n");
}
