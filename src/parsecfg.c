/***************************************************************************/
/*                                                                         */
/* parsecfg - a library for parsing a configuration file                   */
/* Copyright (C) 1999 Yuuki NINOMIYA, gm@smn.enjoy.ne.jp                   */
/*    Very small changes by Linus Åkerlund, uxm165t@tninet.se              */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the                           */
/* Free Software Foundation, Inc., 59 Temple Place - Suite 330,            */
/* Boston, MA 02111-1307, USA.                                             */
/*                                                                         */
/***************************************************************************/

/* parsecfg Ver 3.0.3 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "parsecfg.h"


/* proto type declaration of private functions */

void cfgFatalFunc (cfgErrorCode, char *, int, char *);
void (*cfgFatal) (cfgErrorCode, char *, int, char *) = cfgFatalFunc;	/* default error handler */

int parse_simple (char *file, FILE * fp, char *ptr, cfgStruct cfg[], int *line);
void parse_values_between_braces (char *file, FILE * fp, char *parameter, cfgStruct cfg[], int *line, cfgFileType type, int section);
char *parse_word (char *ptr, char **word, cfgKeywordValue word_type);
int store_value (cfgStruct cfg[], char *parameter, char *value, cfgFileType type, int section);
int parse_ini (char *file, FILE * fp, char *ptr, cfgStruct cfg[], int *line, int *section);
int alloc_for_new_section (cfgStruct cfg[], int *section);

char *get_single_line_without_first_spaces (FILE * fp, char **gotstr, int *line);
char *rm_first_spaces (char *ptr);
char *dynamic_fgets (FILE * fp);


/* static variables */

char **parsecfg_section_name = NULL;
int parsecfg_maximum_section;


/*************************************************************/
/*                      PUBLIC FUCNCTIONS                    */
/*************************************************************/

/* --------------------------------------------------
   NAME       cfgSetFatalFunc
   FUNCTION   handle new error handler
   INPUT      f ... pointer to new error handler
   OUTPUT     none
   -------------------------------------------------- */
void 
cfgSetFatalFunc (void (*f) (cfgErrorCode, char *, int, char *))
{
  cfgFatal = f;
}


/* --------------------------------------------------
   NAME       cfgParse
   FUNCTION   parse a configuration file (main function)
   INPUT      file ... name of the configuration file
   cfg .... array of possible variables
   type ... type of the configuration file
   + CFG_SIMPLE .. simple 1:1
   + CFG_INI ..... like Windows INI file
   OUTPUT     the maximum sections
   (if type==CFG_INI then always return 0)
   -------------------------------------------------- */
int 
cfgParse (char *file, cfgStruct cfg[], cfgFileType type)
{
  char *line_buf;
  char *ptr;
  int line = 0;
  FILE *fp;
  int error_code;
  int max_cfg = -1;

  fp = fopen (file, "r");
  if (fp == NULL)
    {
      cfgFatal (CFG_OPEN_FAIL, file, 0, NULL);
    }

  while ((ptr = get_single_line_without_first_spaces (fp, &line_buf, &line)) != NULL)
    {
      switch (type)
	{
	case CFG_SIMPLE:
	  if ((error_code = parse_simple (file, fp, ptr, cfg, &line)) != CFG_NO_ERROR)
	    {
	      cfgFatal (error_code, file, line, line_buf);
	    }
	  break;
	case CFG_INI:
	  if ((error_code = parse_ini (file, fp, ptr, cfg, &line, &max_cfg)) != CFG_NO_ERROR)
	    {
	      cfgFatal (error_code, file, line, line_buf);
	    }
	  break;
	default:
	  cfgFatal (CFG_INTERNAL_ERROR, file, 0, NULL);
	}
      free (line_buf);
    }

  parsecfg_maximum_section = max_cfg + 1;
  return (parsecfg_maximum_section);
}


/* --------------------------------------------------
   NAME       cfgSectionNameToNumber
   FUNCTION   convert section name to number
   INPUT      name ... section name
   OUTPUT     section number (0,1,2,...)
   if no matching, return -1
   -------------------------------------------------- */
int 
cfgSectionNameToNumber (const char *name)
{
  int i;

  for (i = 0; i < parsecfg_maximum_section; i++)
    {
      if (strcasecmp (name, parsecfg_section_name[i]) == 0)
	{
	  return (i);
	}
    }
  return (-1);
}


/* --------------------------------------------------
   NAME       cfgSectionNumberToName
   FUNCTION   convert section number to name
   INPUT      num ... section number (0,1,2,...)
   OUTPUT     pointer to section name
   if no section, return NULL
   -------------------------------------------------- */
char *
cfgSectionNumberToName (int num)
{
  if (num > parsecfg_maximum_section - 1 || num < 0)
    {
      return (NULL);
    }
  return (parsecfg_section_name[num]);
}


/*************************************************************/
/*                     PRIVATE FUCNCTIONS                    */
/*************************************************************/

/* --------------------------------------------------
   NAME       cfgFatalFunc
   FUNCTION   error handler
   INPUT      error_code ... reason of error
   file ......... the configuration file
   line ......... line number causing error
   str .......... strings at above line
   OUTPUT     never return
   -------------------------------------------------- */
void 
cfgFatalFunc (cfgErrorCode error_code, char *file, int line, char *str)
{
  switch (error_code)
    {
    case CFG_OPEN_FAIL:
      fprintf (stderr, ("Configuration file `%s' is not found.\n"), file);  
      break;
    case CFG_SYNTAX_ERROR:
      fprintf (stderr, ("%s(%d): %s\nsyntax error\n"), file, line, str);
      break;
    case CFG_WRONG_PARAMETER:
      fprintf (stderr, ("%s(%d): %s\nunrecognized parameter\n"), file, line, str);
      break;
    case CFG_INTERNAL_ERROR:
      fprintf (stderr, ("%s(%d): %s\ninternal error\n"), file, line, str);
      break;
    case CFG_INVALID_NUMBER:
      fprintf (stderr, ("%s(%d): %s\ninvalid number\n"), file, line, str);
      break;
    case CFG_OUT_OF_RANGE:
      fprintf (stderr, ("%s(%d): %s\nout of range\n"), file, line, str);
      break;
    case CFG_MEM_ALLOC_FAIL:
      fprintf (stderr, ("%s(%d): %s\nCannot allocate memory.\n"), file, line, str);
      break;
    case CFG_BOOL_ERROR:
      fprintf (stderr, ("%s(%d): %s\nIt must be specified TRUE or FALSE.\n"), file, line, str);
      break;
    case CFG_USED_SECTION:
      fprintf (stderr, ("%s(%d): %s\nThe section name is already used.\n"), file, line, str);
      break;
    default:
      fprintf (stderr, ("%s(%d): %s\nunexplained error\n"), file, line, str);
      break;
    }
  exit (1);
}


/* --------------------------------------------------
   NAME       parse_simple
   FUNCTION   parse as simple 1:1 file
   INPUT      file .. name of the configuration file
   fp .... file pointer to the configuration file
   ptr ... pointer of current parsing
   cfg ... array of possible variables
   line .. pointer to current working line
   OUTPUT     error code (no error is CFG_NO_ERROR)
   -------------------------------------------------- */
int 
parse_simple (char *file, FILE * fp, char *ptr, cfgStruct cfg[], int *line)
{
  char *parameter;
  char *value;
  int error_code;

  if ((ptr = parse_word (ptr, &parameter, CFG_PARAMETER)) == NULL)
    {
      return (CFG_SYNTAX_ERROR);
    }
  if (*ptr == '{')
    {
      ptr = rm_first_spaces (ptr + 1);
      if (*ptr != '\0' && *ptr != '#')
	{
	  return (CFG_SYNTAX_ERROR);
	}
      parse_values_between_braces (file, fp, parameter, cfg, line, CFG_SIMPLE, 0);
    }
  else
    {
      if ((ptr = parse_word (ptr, &value, CFG_VALUE)) == NULL)
	{
	  return (CFG_SYNTAX_ERROR);
	}
      if ((error_code = store_value (cfg, parameter, value, CFG_SIMPLE, 0)) != CFG_NO_ERROR)
	{
	  return (error_code);
	}
      free (parameter);
      free (value);
    }
  return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   NAME       parse_word
   FUNCTION   parse a word
   INPUT      ptr ... pointer of current parsing
   word .. pointer of pointer for parsed word
   (dynamic allocating)
   word_type ... what word type parse as
   + CFG_PARAMETER ... parameter
   + CFG_VALUE ....... value 
   + CFG_SECTION ..... section
   OUTPUT     new current pointer (if syntax error, return NULL)
   -------------------------------------------------- */
char *
parse_word (char *ptr, char **word, cfgKeywordValue word_type)
{
  int len = 0;
  cfgQuote quote_flag;

  switch (*ptr)
    {
    case '\"':
      quote_flag = CFG_DOUBLE_QUOTE;
      ptr++;
      break;
    case '\'':
      quote_flag = CFG_SINGLE_QUOTE;
      ptr++;
      break;
    default:
      quote_flag = CFG_NO_QUOTE;
      break;
    }

  for (;;)
    {
      if (quote_flag == CFG_NO_QUOTE)
	{
	  if (*(ptr + len) == ' ' || *(ptr + len) == '\t' || *(ptr + len) == '\0' || *(ptr + len) == '#' ||
	      (*(ptr + len) == '=' && word_type == CFG_PARAMETER) ||
	      (*(ptr + len) == ']' && word_type == CFG_SECTION))
	    {
	      break;
	    }
	}
      else if (quote_flag == CFG_DOUBLE_QUOTE)
	{
	  if (*(ptr + len) == '\"')
	    {
	      break;
	    }
	}
      else if (quote_flag == CFG_SINGLE_QUOTE)
	{
	  if (*(ptr + len) == '\'')
	    {
	      break;
	    }
	}
      if (*(ptr + len) == '\0')
	{
	  return (NULL);
	}
      len++;
    }
  if ((*word = malloc (len + 1)) == NULL)
    {
      cfgFatalFunc (CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
    }
  strncpy (*word, ptr, len);
  *(*word + len) = '\0';

  ptr += (len + (quote_flag == CFG_NO_QUOTE ? 0 : 1));

  ptr = rm_first_spaces (ptr);

  switch (word_type)
    {
    case CFG_PARAMETER:
      if (*ptr != '=')
	{
	  return (NULL);
	}
      ptr++;
      ptr = rm_first_spaces (ptr);
      break;
    case CFG_VALUE:
      if (*ptr != '\0' && *ptr != '#')
	{
	  return (NULL);
	}
      break;
    case CFG_SECTION:
      if (*ptr != ']')
	{
	  return (NULL);
	}
      break;
    default:
      return (NULL);
    }
  return (ptr);
}


/* --------------------------------------------------
   NAME       store_value
   FUNCTION   store the value according to cfg
   INPUT      cfg ......... array of possible variables
   parameter ... parameter
   value ....... value
   type ........ type of the configuration file
   section ..... section number
   OUTPUT     error code
   -------------------------------------------------- */
int 
store_value (cfgStruct cfg[], char *parameter, char *value, cfgFileType type, int section)
{
  int num;
  long tmp;
  unsigned long utmp;
  char *endptr;
  char *strptr;
  cfgList *listptr;


  for (num = 0; cfg[num].type != CFG_END; num++)
    {
      if (strcasecmp (parameter, cfg[num].parameterName) == 0)
	{
	  errno = 0;
	  switch (cfg[num].type)
	    {
	    case CFG_BOOL:
	      if (strcasecmp (value, "TRUE") == 0 ||
		  strcasecmp (value, "YES") == 0 ||
		  strcasecmp (value, "T") == 0 ||
		  strcasecmp (value, "Y") == 0 ||
		  strcasecmp (value, "1") == 0)
		{
		  if (type == CFG_INI)
		    {
		      *(*(int **) (cfg[num].value) + section) = 1;
		    }
		  else
		    {
		      *(int *) (cfg[num].value) = 1;
		    }
		  return (CFG_NO_ERROR);
		}
	      else if (strcasecmp (value, "FALSE") == 0 ||
		       strcasecmp (value, "NO") == 0 ||
		       strcasecmp (value, "F") == 0 ||
		       strcasecmp (value, "N") == 0 ||
		       strcasecmp (value, "0") == 0)
		{
		  if (type == CFG_INI)
		    {
		      *(*(int **) (cfg[num].value) + section) = 0;
		    }
		  else
		    {
		      *(int *) (cfg[num].value) = 0;
		    }
		  return (CFG_NO_ERROR);
		}
	      return (CFG_BOOL_ERROR);

	    case CFG_STRING:
	      if ((strptr = malloc (strlen (value) + 1)) == NULL)
		{
		  return (CFG_MEM_ALLOC_FAIL);
		}
	      strcpy (strptr, value);
	      if (type == CFG_INI)
		{
		  *(*(char ***) (cfg[num].value) + section) = strptr;
		}
	      else
		{
		  *(char **) (cfg[num].value) = strptr;
		}
	      return (CFG_NO_ERROR);

	    case CFG_INT:
	      tmp = strtol (value, &endptr, 10);
	      if (*endptr)
		{
		  return (CFG_INVALID_NUMBER);
		}
	      if (errno == ERANGE || tmp > INT_MAX || tmp < INT_MIN)
		{
		  return (CFG_OUT_OF_RANGE);
		}
	      if (type == CFG_INI)
		{
		  *(*(int **) (cfg[num].value) + section) = tmp;
		}
	      else
		{
		  *(int *) (cfg[num].value) = tmp;
		}
	      return (CFG_NO_ERROR);

	    case CFG_UINT:
	      utmp = strtoul (value, &endptr, 10);
	      if (*endptr)
		{
		  return (CFG_INVALID_NUMBER);
		}
	      if (errno == ERANGE || tmp > UINT_MAX)
		{
		  return (CFG_OUT_OF_RANGE);
		}
	      if (type == CFG_INI)
		{
		  *(*(unsigned int **) (cfg[num].value) + section) = utmp;
		}
	      else
		{
		  *(unsigned int *) (cfg[num].value) = utmp;
		}
	      return (CFG_NO_ERROR);

	    case CFG_LONG:
	      tmp = strtol (value, &endptr, 10);
	      if (*endptr)
		{
		  return (CFG_INVALID_NUMBER);
		}
	      if (errno == ERANGE)
		{
		  return (CFG_OUT_OF_RANGE);
		}
	      if (type == CFG_INI)
		{
		  *(*(long **) (cfg[num].value) + section) = tmp;
		}
	      else
		{
		  *(long *) (cfg[num].value) = tmp;
		}
	      return (CFG_NO_ERROR);

	    case CFG_ULONG:
	      utmp = strtoul (value, &endptr, 10);
	      if (*endptr)
		{
		  return (CFG_INVALID_NUMBER);
		}
	      if (errno == ERANGE)
		{
		  return (CFG_OUT_OF_RANGE);
		}
	      if (type == CFG_INI)
		{
		  *(*(unsigned long **) (cfg[num].value) + section) = utmp;
		}
	      else
		{
		  *(unsigned long *) (cfg[num].value) = utmp;
		}
	      return (CFG_NO_ERROR);

	    case CFG_STRING_LIST:
	      if (type == CFG_INI)
		{
		  listptr = *(*(cfgList ***) (cfg[num].value) + section);
		}
	      else
		{
		  listptr = *(cfgList **) (cfg[num].value);
		}
	      if (listptr != NULL)
		{
		  while (listptr->next != NULL)
		    {
		      listptr = listptr->next;
		    }
		  if ((listptr = listptr->next = malloc (sizeof (cfgList))) == NULL)
		    {
		      return (CFG_MEM_ALLOC_FAIL);
		    }
		}
	      else
		{
		  if ((listptr = malloc (sizeof (cfgList))) == NULL)
		    {
		      return (CFG_MEM_ALLOC_FAIL);
		    }
		  if (type == CFG_INI)
		    {
		      *(*(cfgList ***) (cfg[num].value) + section) = listptr;
		    }
		  else
		    {
		      *(cfgList **) (cfg[num].value) = listptr;
		    }
		}
	      if ((strptr = malloc (strlen (value) + 1)) == NULL)
		{
		  return (CFG_MEM_ALLOC_FAIL);
		}
	      strcpy (strptr, value);
	      listptr->str = strptr;
	      listptr->next = NULL;
	      return (CFG_NO_ERROR);

	    default:
	      return (CFG_INTERNAL_ERROR);
	    }
	}
    }
  return (CFG_WRONG_PARAMETER);
}


/* --------------------------------------------------
   NAME       parse_values_between_braces
   FUNCTION   parse values between braces
   INPUT      file ....... name of the configuraion file
   fp ......... file pointer to the configuration file
   parameter .. parameter
   cfg ........ array of possible variables
   line ....... pointer to current working line
   OUTPUT     none
   -------------------------------------------------- */
void 
parse_values_between_braces (char *file, FILE * fp, char *parameter, cfgStruct cfg[], int *line, cfgFileType type, int section)
{
  char *line_buf;
  char *value;
  char *ptr;
  int error_code;

  while ((ptr = get_single_line_without_first_spaces (fp, &line_buf, line)) != NULL)
    {
      if (*ptr == '}')
	{
	  ptr = rm_first_spaces (ptr + 1);
	  if (*ptr != '\0' && *ptr != '#')
	    {
	      cfgFatal (CFG_SYNTAX_ERROR, file, *line, line_buf);
	    }
	  free (line_buf);
	  return;
	}
      if (parse_word (ptr, &value, CFG_VALUE) == NULL)
	{
	  cfgFatal (CFG_SYNTAX_ERROR, file, *line, line_buf);
	}
      if ((error_code = store_value (cfg, parameter, value, type, section)) != CFG_NO_ERROR)
	{
	  cfgFatal (error_code, file, *line, line_buf);
	}
      free (line_buf);
      free (value);
    }
}


/* --------------------------------------------------
   NAME       parse_ini
   FUNCTION   parse the configuration file as like Windows INI file
   INPUT      file .. name of the configuration file
   fp .... file pointer to the configuration file
   ptr ... pointer of current parsing
   cfg ... array of possible variables
   line .. pointer to current working line
   section ... pointer to current section number
   OUTPUT     error code (no error is CFG_NO_ERROR)
   -------------------------------------------------- */
int 
parse_ini (char *file, FILE * fp, char *ptr, cfgStruct cfg[], int *line, int *section)
{
  char *parameter;
  char *value;
  int error_code;
  int i;

  if (*ptr == '[')
    {
      if ((error_code = alloc_for_new_section (cfg, section)) != CFG_NO_ERROR)
	{
	  return (error_code);
	}
      ptr = rm_first_spaces (ptr + 1);

      parsecfg_section_name = realloc (parsecfg_section_name, sizeof (char *) * (*section + 1));

      if ((ptr = parse_word (ptr, &parsecfg_section_name[*section], CFG_SECTION)) == NULL)
	{
	  return (CFG_SYNTAX_ERROR);
	}
      for (i = 0; i < *section; i++)
	{
	  if (strcasecmp (parsecfg_section_name[*section], parsecfg_section_name[i]) == 0)
	    {
	      return (CFG_USED_SECTION);
	    }
	}
      ptr = rm_first_spaces (ptr + 1);
      if (*ptr != '\0' && *ptr != '#')
	{
	  return (CFG_SYNTAX_ERROR);
	}
      return (CFG_NO_ERROR);
    }

  if ((ptr = parse_word (ptr, &parameter, CFG_PARAMETER)) == NULL)
    {
      return (CFG_SYNTAX_ERROR);
    }
  if (*ptr == '{')
    {
      ptr = rm_first_spaces (ptr + 1);
      if (*ptr != '\0' && *ptr != '#')
	{
	  return (CFG_SYNTAX_ERROR);
	}
      parse_values_between_braces (file, fp, parameter, cfg, line, CFG_INI, *section);
    }
  else
    {
      if ((ptr = parse_word (ptr, &value, CFG_VALUE)) == NULL)
	{
	  return (CFG_SYNTAX_ERROR);
	}
      if ((error_code = store_value (cfg, parameter, value, CFG_INI, *section)) != CFG_NO_ERROR)
	{
	  return (error_code);
	}
      free (parameter);
      free (value);
    }
  return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   NAME       alloc_for_new_section
   FUNCTION   allocate memory for new section
   INPUT      cfg ....... array of possible variables
   section ... pointer to current section number
   OUTPUT     error code
   -------------------------------------------------- */
int 
alloc_for_new_section (cfgStruct cfg[], int *section)
{
  int num;
  void *ptr;

  (*section)++;
  for (num = 0; cfg[num].type != CFG_END; num++)
    {
      switch (cfg[num].type)
	{
	case CFG_BOOL:
	case CFG_INT:
	case CFG_UINT:
	  if (*section == 0)
	    {
	      *(int **) (cfg[num].value) = NULL;
	    }
	  if ((ptr = realloc (*(int **) (cfg[num].value), sizeof (int) * (*section + 1))) == NULL)
	    {
	      return (CFG_MEM_ALLOC_FAIL);
	    }
	  *(int **) (cfg[num].value) = ptr;
	  if (cfg[num].type == CFG_BOOL)
	    {
	      *(*((int **) (cfg[num].value)) + *section) = -1;
	    }
	  else
	    {
	      *(*((int **) (cfg[num].value)) + *section) = 0;
	    }
	  break;

	case CFG_LONG:
	case CFG_ULONG:
	  if (*section == 0)
	    {
	      *(long **) (cfg[num].value) = NULL;
	    }
	  if ((ptr = realloc (*(long **) (cfg[num].value), sizeof (long) * (*section + 1))) == NULL)
	    {
	      return (CFG_MEM_ALLOC_FAIL);
	    }
	  *(long **) (cfg[num].value) = ptr;
	  *(*((long **) (cfg[num].value)) + *section) = 0;
	  break;

	case CFG_STRING:
	  if (*section == 0)
	    {
	      *(char ***) (cfg[num].value) = NULL;
	    }
	  if ((ptr = realloc (*(char ***) (cfg[num].value), sizeof (char *) * (*section + 1))) == NULL)
	    {
	      return (CFG_MEM_ALLOC_FAIL);
	    }
	  *(char ***) (cfg[num].value) = ptr;
	  *(*(char ***) (cfg[num].value) + *section) = NULL;
	  break;

	case CFG_STRING_LIST:
	  if (*section == 0)
	    {
	      *(cfgList ***) (cfg[num].value) = NULL;
	    }
	  if ((ptr = realloc (*(cfgList ***) (cfg[num].value), sizeof (cfgList *) * (*section + 1))) == NULL)
	    {
	      return (CFG_MEM_ALLOC_FAIL);
	    }
	  *(cfgList ***) (cfg[num].value) = ptr;
	  *(*(cfgList ***) (cfg[num].value) + *section) = NULL;
	  break;

	default:
	  return (CFG_INTERNAL_ERROR);
	}
    }
  return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   NAME       rm_first_spaces
   FUNCTION   remove lead-off spaces and tabs in the string
   INPUT      ptr ... pointer to string
   OUTPUT     new poniter after removing
   -------------------------------------------------- */
char *
rm_first_spaces (char *ptr)
{
  while (*ptr == ' ' || *ptr == '\t')
    {
      ptr++;
    }
  return (ptr);
}


/* --------------------------------------------------
   NAME       get_single_line_without_first_spaces
   FUNCTION   get a single line without lead-off spaces
   and tabs from file
   INPUT      fp ....... file pointer
   gotptr ... pointer stored got string
   line ..... pointer to line number
   OUTPUT     new poniter after removing
   -------------------------------------------------- */
char *
get_single_line_without_first_spaces (FILE * fp, char **gotstr, int *line)
{
  char *ptr;

  for (;;)
    {
      if ((*gotstr = dynamic_fgets (fp)) == NULL)
	{
	  return (NULL);
	}
      (*line)++;
      ptr = rm_first_spaces (*gotstr);
      if (*ptr != '#' && *ptr != '\0')
	{
	  return (ptr);
	}
      free (*gotstr);
    }
}


/* --------------------------------------------------
   NAME       dynamic_fgets
   FUNCTION   fgets with dynamic allocated memory
   INPUT      fp ... file pointer
   OUTPUT     pointer to got strings
   -------------------------------------------------- */
char *
dynamic_fgets (FILE * fp)
{
  char *ptr;
  char temp[128];
  int i;

  ptr = malloc (1);
  if (ptr == NULL)
    {
      cfgFatalFunc (CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
    }
  *ptr = '\0';
  for (i = 0;; i++)
    {
      if (fgets (temp, 128, fp) == NULL)
	{
	  free (ptr);
	  return (NULL);
	}
      ptr = realloc (ptr, 127 * (i + 1) + 1);
      if (ptr == NULL)
	{
	  cfgFatalFunc (CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
	}
      strcat (ptr, temp);
      if (strchr (temp, '\n') != NULL)
	{
	  *strchr (ptr, '\n') = '\0';
	  return (ptr);
	}
    }
}
