// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_LEARNING_KWDATA_KWCYAC_HPP_INCLUDED
#define YY_YY_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_LEARNING_KWDATA_KWCYAC_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum yytokentype
{
	YYEMPTY = -2,
	YYEOF = 0,                /* "end of file"  */
	YYerror = 256,            /* error  */
	YYUNDEF = 257,            /* "invalid token"  */
	BASICIDENTIFIER = 258,    /* BASICIDENTIFIER  */
	EXTENDEDIDENTIFIER = 259, /* EXTENDEDIDENTIFIER  */
	CONTINUOUSLITTERAL = 260, /* CONTINUOUSLITTERAL  */
	STRINGLITTERAL = 261,     /* STRINGLITTERAL  */
	LABEL = 262,              /* LABEL  */
	APPLICATIONID = 263,      /* APPLICATIONID  */
	CLASS = 264,              /* CLASS  */
	CONTINUOUSTYPE = 265,     /* CONTINUOUSTYPE  */
	SYMBOLTYPE = 266,         /* SYMBOLTYPE  */
	OBJECTTYPE = 267,         /* OBJECTTYPE  */
	OBJECTARRAYTYPE = 268,    /* OBJECTARRAYTYPE  */
	ROOT = 269,               /* ROOT  */
	UNUSED = 270,             /* UNUSED  */
	DATETYPE = 271,           /* DATETYPE  */
	TIMETYPE = 272,           /* TIMETYPE  */
	TIMESTAMPTYPE = 273,      /* TIMESTAMPTYPE  */
	TIMESTAMPTZTYPE = 274,    /* TIMESTAMPTZTYPE  */
	TEXTTYPE = 275,           /* TEXTTYPE  */
	TEXTLISTTYPE = 276,       /* TEXTLISTTYPE  */
	STRUCTURETYPE = 277       /* STRUCTURETYPE  */
};
typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 64 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

	Continuous cValue;
	ALString* sValue;
	boolean bValue;
	StringVector* svValue;
	KWDerivationRule* kwdrValue;
	KWDerivationRuleOperand* kwdroValue;
	KWClass* kwcValue;
	KWAttribute* kwaValue;
	ObjectArray* oaAttributes;
	ObjectArray* oaOperands;
	KWMetaData* kwmdMetaData;
	int nValue;

#line 101 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.hpp"
};
typedef union YYSTYPE YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

int yyparse(void);

#endif /* !YY_YY_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_LEARNING_KWDATA_KWCYAC_HPP_INCLUDED  */
