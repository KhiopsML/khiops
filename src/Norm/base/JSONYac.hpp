// Copyright (c) 2023-2026 Orange. All rights reserved.
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

#ifndef YY_JSON_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_NORM_BASE_JSONYAC_HPP_INCLUDED
#define YY_JSON_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_NORM_BASE_JSONYAC_HPP_INCLUDED
/* Debug traces.  */
#ifndef JSONDEBUG
#if defined YYDEBUG
#if YYDEBUG
#define JSONDEBUG 1
#else
#define JSONDEBUG 0
#endif
#else /* ! defined YYDEBUG */
#define JSONDEBUG 0
#endif /* ! defined YYDEBUG */
#endif /* ! defined JSONDEBUG */
#if JSONDEBUG
extern int jsondebug;
#endif

/* Token kinds.  */
#ifndef JSONTOKENTYPE
#define JSONTOKENTYPE
enum jsontokentype
{
	JSONEMPTY = -2,
	JSONEOF = 0,        /* "end of file"  */
	JSONerror = 256,    /* error  */
	JSONUNDEF = 257,    /* "invalid token"  */
	STRINGVALUE = 258,  /* STRINGVALUE  */
	NUMBERVALUE = 259,  /* NUMBERVALUE  */
	BOOLEANVALUE = 260, /* BOOLEANVALUE  */
	STRINGERROR = 261,  /* STRINGERROR  */
	ERROR = 262,        /* ERROR  */
	NULLVALUE = 263     /* NULLVALUE  */
};
typedef enum jsontokentype jsontoken_kind_t;
#endif

/* Value type.  */
#if !defined JSONSTYPE && !defined JSONSTYPE_IS_DECLARED
union JSONSTYPE
{
#line 51 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JSONYac.yac"

	ALString* sValue;
	double dValue;
	boolean bValue;
	JSONValue* jsonValue;
	JSONObject* jsonObject;
	JSONArray* jsonArray;
	JSONMember* jsonMember;

#line 90 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JSONYac.hpp"
};
typedef union JSONSTYPE JSONSTYPE;
#define JSONSTYPE_IS_TRIVIAL 1
#define JSONSTYPE_IS_DECLARED 1
#endif

extern JSONSTYPE jsonlval;

int jsonparse(void);

#endif /* !YY_JSON_C_APPLICATIONS_BOULLEMA_DEVGIT_KHIOPS_SRC_NORM_BASE_JSONYAC_HPP_INCLUDED  */
