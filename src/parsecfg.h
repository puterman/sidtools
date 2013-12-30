/***************************************************************************/
/*                                                                         */
/* parsecfg - a library for parsing a configuration file                   */
/* Copyright (C) 1999 Yuuki NINOMIYA, gm@smn.enjoy.ne.jp                   */
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

#ifndef PARSECFG_H_INCLUDED
#define PARSECFG_H_INCLUDED


/* error code */
typedef enum{
	CFG_NO_ERROR,
	CFG_OPEN_FAIL,
	CFG_SYNTAX_ERROR,
	CFG_WRONG_PARAMETER,
	CFG_INTERNAL_ERROR,
	CFG_INVALID_NUMBER,
	CFG_OUT_OF_RANGE,
	CFG_MEM_ALLOC_FAIL,
	CFG_BOOL_ERROR,
	CFG_USED_SECTION,
} cfgErrorCode;

/* type of the configuration file */
typedef enum{
	CFG_SIMPLE,
	CFG_INI,
} cfgFileType;

/* constants for recognized value types */
typedef enum{
	CFG_END,
	CFG_BOOL,
	CFG_STRING,
	CFG_INT,
	CFG_UINT,
	CFG_LONG,
	CFG_ULONG,
	CFG_STRING_LIST,
} cfgValueType;

typedef enum{
        CFG_PARAMETER,
        CFG_VALUE,
	CFG_SECTION,
} cfgKeywordValue;

typedef enum{
	CFG_NO_QUOTE,
	CFG_SINGLE_QUOTE,
	CFG_DOUBLE_QUOTE,
} cfgQuote;


typedef struct{
	char *parameterName;
	cfgValueType type;
	void *value;
} cfgStruct;

typedef struct cfglist *cfgListPtr;
typedef struct cfglist{
	char *str;
	cfgListPtr next;
} cfgList;


/* proto type declaration of public functions */
void cfgSetFatalFunc(void (*f)(cfgErrorCode,char *,int,char *));
int cfgParse(char *file,cfgStruct cfg[],cfgFileType type);
int cfgSectionNameToNumber(const char *name);
char *cfgSectionNumberToName(int num);


#endif /* PARSECFG_H_INCLUDED */
