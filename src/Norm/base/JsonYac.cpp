// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE JSONSTYPE
/* Substitute the variable and function names.  */
#define yyparse jsonparse
#define yylex jsonlex
#define yyerror jsonerror
#define yydebug jsondebug
#define yynerrs jsonnerrs
#define yylval jsonlval
#define yychar jsonchar

/* First part of user prologue.  */
#line 1 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"

/* Parser de json, inspire de la grammaire  de reference https://www.json.org/json-en.html */

#include "Object.h"
#include "ALString.h"
#include "TextService.h"
#include "JsonObject.h"

/* Declaration du lexer utilise */
void jsonerror(char const* fmt);
void jsonerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber);
int jsonlex();

/* Objet json principal a utiliser pendant la lecture d'un fichier. */
/* Ce domaine est positionner par la methode ReadFile de JsonObject */
static JsonObject* mainReadJsonObject = NULL;

/* Work around a bug in the relation between bison and GCC 3.x: */
#if defined(__GNUC__) && 3 <= __GNUC__
#define __attribute__(arglist)
#endif

/* Nombre total d'erreurs de parsing */
static int nJsonFileParsingErrorNumber = 0;

// Desactivation de warnings pour le Visual C++
#ifdef __MSC__
#pragma warning(disable : 4702) // C4702: unreachable code (mal gere par Visual C++)
#endif                          // __MSC__

#define YY_STATIC

/* Debugging YAC */

/*
#define YYDEBUG 1
extern char   *yyptok(int i);
*/

#line 120 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"

#ifndef YY_CAST
#ifdef __cplusplus
#define YY_CAST(Type, Val) static_cast<Type>(Val)
#define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type>(Val)
#else
#define YY_CAST(Type, Val) ((Type)(Val))
#define YY_REINTERPRET_CAST(Type, Val) ((Type)(Val))
#endif
#endif
#ifndef YY_NULLPTR
#if defined __cplusplus
#if 201103L <= __cplusplus
#define YY_NULLPTR nullptr
#else
#define YY_NULLPTR 0
#endif
#else
#define YY_NULLPTR ((void*)0)
#endif
#endif

#include "JsonYac.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
	YYSYMBOL_YYEMPTY = -2,
	YYSYMBOL_YYEOF = 0,        /* "end of file"  */
	YYSYMBOL_YYerror = 1,      /* error  */
	YYSYMBOL_YYUNDEF = 2,      /* "invalid token"  */
	YYSYMBOL_STRINGVALUE = 3,  /* STRINGVALUE  */
	YYSYMBOL_NUMBERVALUE = 4,  /* NUMBERVALUE  */
	YYSYMBOL_BOOLEANVALUE = 5, /* BOOLEANVALUE  */
	YYSYMBOL_ERROR = 6,        /* ERROR  */
	YYSYMBOL_NULLVALUE = 7,    /* NULLVALUE  */
	YYSYMBOL_8_ = 8,           /* '{'  */
	YYSYMBOL_9_ = 9,           /* '}'  */
	YYSYMBOL_10_ = 10,         /* ','  */
	YYSYMBOL_11_ = 11,         /* ':'  */
	YYSYMBOL_12_ = 12,         /* '['  */
	YYSYMBOL_13_ = 13,         /* ']'  */
	YYSYMBOL_YYACCEPT = 14,    /* $accept  */
	YYSYMBOL_jsonFile = 15,    /* jsonFile  */
	YYSYMBOL_value = 16,       /* value  */
	YYSYMBOL_object = 17,      /* object  */
	YYSYMBOL_members = 18,     /* members  */
	YYSYMBOL_member = 19,      /* member  */
	YYSYMBOL_array = 20,       /* array  */
	YYSYMBOL_values = 21       /* values  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;

#ifdef short
#undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
#include <limits.h> /* INFRINGES ON USER NAME SPACE */
#if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#define YY_STDINT_H
#endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
#undef UINT_LEAST8_MAX
#undef UINT_LEAST16_MAX
#define UINT_LEAST8_MAX 255
#define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
#if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#define YYPTRDIFF_T __PTRDIFF_TYPE__
#define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
#elif defined PTRDIFF_MAX
#ifndef ptrdiff_t
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#endif
#define YYPTRDIFF_T ptrdiff_t
#define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
#else
#define YYPTRDIFF_T long
#define YYPTRDIFF_MAXIMUM LONG_MAX
#endif
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned
#endif
#endif

#define YYSIZE_MAXIMUM                                                                                                 \
	YY_CAST(YYPTRDIFF_T, (YYPTRDIFF_MAXIMUM < YY_CAST(YYSIZE_T, -1) ? YYPTRDIFF_MAXIMUM : YY_CAST(YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST(YYPTRDIFF_T, sizeof(X))

/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#define YY_(Msgid) dgettext("bison-runtime", Msgid)
#endif
#endif
#ifndef YY_
#define YY_(Msgid) Msgid
#endif
#endif

#ifndef YY_ATTRIBUTE_PURE
#if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_PURE __attribute__((__pure__))
#else
#define YY_ATTRIBUTE_PURE
#endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define YY_ATTRIBUTE_UNUSED
#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if !defined lint || defined __GNUC__
#define YY_USE(E) ((void)(E))
#else
#define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && !defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
#if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                                            \
	_Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")
#else
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                                            \
	_Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")                           \
	    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#endif
#define YY_IGNORE_MAYBE_UNINITIALIZED_END _Pragma("GCC diagnostic pop")
#else
#define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && !defined __ICC && 6 <= __GNUC__
#define YY_IGNORE_USELESS_CAST_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")
#define YY_IGNORE_USELESS_CAST_END _Pragma("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_END
#endif

#define YY_ASSERT(E) ((void)(0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#ifdef YYSTACK_USE_ALLOCA
#if YYSTACK_USE_ALLOCA
#ifdef __GNUC__
#define YYSTACK_ALLOC __builtin_alloca
#elif defined __BUILTIN_VA_ARG_INCR
#include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#elif defined _AIX
#define YYSTACK_ALLOC __alloca
#elif defined _MSC_VER
#include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#define alloca _alloca
#else
#define YYSTACK_ALLOC alloca
#if !defined _ALLOCA_H && !defined EXIT_SUCCESS
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
/* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#endif
#endif
#endif

#ifdef YYSTACK_ALLOC
/* Pacify GCC's 'empty if-body' warning.  */
#define YYSTACK_FREE(Ptr)                                                                                              \
	do                                                                                                             \
	{ /* empty */                                                                                                  \
		;                                                                                                      \
	} while (0)
#ifndef YYSTACK_ALLOC_MAXIMUM
/* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#endif
#else
#define YYSTACK_ALLOC YYMALLOC
#define YYSTACK_FREE YYFREE
#ifndef YYSTACK_ALLOC_MAXIMUM
#define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#endif
#if (defined __cplusplus && !defined EXIT_SUCCESS &&                                                                   \
     !((defined YYMALLOC || defined malloc) && (defined YYFREE || defined free)))
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#ifndef YYMALLOC
#define YYMALLOC malloc
#if !defined malloc && !defined EXIT_SUCCESS
void* malloc(YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#ifndef YYFREE
#define YYFREE free
#if !defined free && !defined EXIT_SUCCESS
void free(void*); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#endif
#endif /* !defined yyoverflow */

#if (!defined yyoverflow && (!defined __cplusplus || (defined JSONSTYPE_IS_TRIVIAL && JSONSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
	yy_state_t yyss_alloc;
	YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (YYSIZEOF(union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#define YYSTACK_BYTES(N) ((N) * (YYSIZEOF(yy_state_t) + YYSIZEOF(YYSTYPE)) + YYSTACK_GAP_MAXIMUM)

#define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack_alloc, Stack)                                                                           \
	do                                                                                                             \
	{                                                                                                              \
		YYPTRDIFF_T yynewbytes;                                                                                \
		YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                                                            \
		Stack = &yyptr->Stack_alloc;                                                                           \
		yynewbytes = yystacksize * YYSIZEOF(*Stack) + YYSTACK_GAP_MAXIMUM;                                     \
		yyptr += yynewbytes / YYSIZEOF(*yyptr);                                                                \
	} while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined __GNUC__ && 1 < __GNUC__
#define YYCOPY(Dst, Src, Count) __builtin_memcpy(Dst, Src, YY_CAST(YYSIZE_T, (Count)) * sizeof(*(Src)))
#else
#define YYCOPY(Dst, Src, Count)                                                                                        \
	do                                                                                                             \
	{                                                                                                              \
		YYPTRDIFF_T yyi;                                                                                       \
		for (yyi = 0; yyi < (Count); yyi++)                                                                    \
			(Dst)[yyi] = (Src)[yyi];                                                                       \
	} while (0)
#endif
#endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 30

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 14
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 8
/* YYNRULES -- Number of rules.  */
#define YYNRULES 20
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 30

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK 262

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                                                               \
	(0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] = {
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 10, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 11, 2, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 12, 2, 13, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 2,  9, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2,  2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,  2, 3,  4, 5, 6, 7};

#if JSONDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] = {0,   76,  76,  95,  103, 104, 105, 117, 130, 143, 151,
				       163, 165, 169, 182, 209, 221, 238, 240, 244, 257};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST(yysymbol_kind_t, yystos[State])

#if JSONDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char* yysymbol_name(yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = {"\"end of file\"",
				      "error",
				      "\"invalid token\"",
				      "STRINGVALUE",
				      "NUMBERVALUE",
				      "BOOLEANVALUE",
				      "ERROR",
				      "NULLVALUE",
				      "'{'",
				      "'}'",
				      "','",
				      "':'",
				      "'['",
				      "']'",
				      "$accept",
				      "jsonFile",
				      "value",
				      "object",
				      "members",
				      "member",
				      "array",
				      "values",
				      YY_NULLPTR};

static const char* yysymbol_name(yysymbol_kind_t yysymbol)
{
	return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-5)

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] = {18, -5, 15, 6,  -5, -4, -5, -5, 13, -5, -5, 8,  -5, 5,  -5,
				     -5, -5, -5, -5, -3, -5, -5, -5, -5, -5, -5, 17, 8,  -5, -5};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] = {0, 3, 0,  0, 2, 0,  15, 11, 0,  13, 1,  0, 12, 0,  6,
				       7, 8, 10, 9, 0, 16, 4,  5,  14, 17, 19, 0, 0,  18, 20};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-5, -5, -2, 28, -5, 16, -5, -5};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] = {0, 3, 20, 21, 8, 9, 22, 26};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] = {14, 15, 16, 17, 18, 2, 10, 11, 5, 19, 24, 14, 15, 16, 17, 18,
				      2,  25, 5,  1,  19, 6, 12, 13, 7, 29, 2,  27, 4,  23, 28};

static const yytype_int8 yycheck[] = {3, 4,  5, 6, 7,  8, 0, 11, 3, 12, 13, 3,  4, 5,  6, 7,
				      8, 19, 3, 1, 12, 6, 9, 10, 9, 27, 8,  10, 0, 13, 13};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] = {0, 1, 8, 15, 17, 3,  6,  9,  18, 19, 0,  11, 9,  10, 3,
				     4, 5, 6, 7,  12, 16, 17, 20, 19, 13, 16, 21, 10, 13, 16};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] = {0, 14, 15, 15, 16, 16, 16, 16, 16, 16, 16, 17, 17, 18, 18, 18, 19, 20, 20, 21, 21};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = {0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 3, 1, 3, 2, 3, 1, 3};

enum
{
	YYENOMEM = -2
};

#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = JSONEMPTY)

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab
#define YYNOMEM goto yyexhaustedlab

#define YYRECOVERING() (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                                                                         \
	do                                                                                                             \
		if (yychar == JSONEMPTY)                                                                               \
		{                                                                                                      \
			yychar = (Token);                                                                              \
			yylval = (Value);                                                                              \
			YYPOPSTACK(yylen);                                                                             \
			yystate = *yyssp;                                                                              \
			goto yybackup;                                                                                 \
		}                                                                                                      \
		else                                                                                                   \
		{                                                                                                      \
			yyerror(YY_("syntax error: cannot back up"));                                                  \
			YYERROR;                                                                                       \
		}                                                                                                      \
	while (0)

/* Backward compatibility with an undocumented macro.
   Use JSONerror or JSONUNDEF. */
#define YYERRCODE JSONUNDEF

/* Enable debugging if requested.  */
#if JSONDEBUG

#ifndef YYFPRINTF
#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#define YYFPRINTF fprintf
#endif

#define YYDPRINTF(Args)                                                                                                \
	do                                                                                                             \
	{                                                                                                              \
		if (yydebug)                                                                                           \
			YYFPRINTF Args;                                                                                \
	} while (0)

#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                                                                  \
	do                                                                                                             \
	{                                                                                                              \
		if (yydebug)                                                                                           \
		{                                                                                                      \
			YYFPRINTF(stderr, "%s ", Title);                                                               \
			yy_symbol_print(stderr, Kind, Value);                                                          \
			YYFPRINTF(stderr, "\n");                                                                       \
		}                                                                                                      \
	} while (0)

/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE* yyo, yysymbol_kind_t yykind, YYSTYPE const* const yyvaluep)
{
	FILE* yyoutput = yyo;
	YY_USE(yyoutput);
	if (!yyvaluep)
		return;
	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	YY_USE(yykind);
	YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE* yyo, yysymbol_kind_t yykind, YYSTYPE const* const yyvaluep)
{
	YYFPRINTF(yyo, "%s %s (", yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name(yykind));

	yy_symbol_value_print(yyo, yykind, yyvaluep);
	YYFPRINTF(yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void yy_stack_print(yy_state_t* yybottom, yy_state_t* yytop)
{
	YYFPRINTF(stderr, "Stack now");
	for (; yybottom <= yytop; yybottom++)
	{
		int yybot = *yybottom;
		YYFPRINTF(stderr, " %d", yybot);
	}
	YYFPRINTF(stderr, "\n");
}

#define YY_STACK_PRINT(Bottom, Top)                                                                                    \
	do                                                                                                             \
	{                                                                                                              \
		if (yydebug)                                                                                           \
			yy_stack_print((Bottom), (Top));                                                               \
	} while (0)

/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void yy_reduce_print(yy_state_t* yyssp, YYSTYPE* yyvsp, int yyrule)
{
	int yylno = yyrline[yyrule];
	int yynrhs = yyr2[yyrule];
	int yyi;
	YYFPRINTF(stderr, "Reducing stack by rule %d (line %d):\n", yyrule - 1, yylno);
	/* The symbols being reduced.  */
	for (yyi = 0; yyi < yynrhs; yyi++)
	{
		YYFPRINTF(stderr, "   $%d = ", yyi + 1);
		yy_symbol_print(stderr, YY_ACCESSING_SYMBOL(+yyssp[yyi + 1 - yynrhs]), &yyvsp[(yyi + 1) - (yynrhs)]);
		YYFPRINTF(stderr, "\n");
	}
}

#define YY_REDUCE_PRINT(Rule)                                                                                          \
	do                                                                                                             \
	{                                                                                                              \
		if (yydebug)                                                                                           \
			yy_reduce_print(yyssp, yyvsp, Rule);                                                           \
	} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !JSONDEBUG */
#define YYDPRINTF(Args) ((void)0)
#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
#define YY_STACK_PRINT(Bottom, Top)
#define YY_REDUCE_PRINT(Rule)
#endif /* !JSONDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void yydestruct(const char* yymsg, yysymbol_kind_t yykind, YYSTYPE* yyvaluep)
{
	YY_USE(yyvaluep);
	if (!yymsg)
		yymsg = "Deleting";
	YY_SYMBOL_PRINT(yymsg, yykind, yyvaluep, yylocationp);

	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	switch (yykind)
	{
	case YYSYMBOL_STRINGVALUE: /* STRINGVALUE  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 875 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_ERROR: /* ERROR  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 881 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_jsonFile: /* jsonFile  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonObject) != NULL)
			delete ((*yyvaluep).jsonObject);
		((*yyvaluep).jsonObject) = NULL;
	}
#line 887 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_value: /* value  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonValue) != NULL)
			delete ((*yyvaluep).jsonValue);
		((*yyvaluep).jsonValue) = NULL;
	}
#line 893 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_object: /* object  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonObject) != NULL)
			delete ((*yyvaluep).jsonObject);
		((*yyvaluep).jsonObject) = NULL;
	}
#line 899 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_members: /* members  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonObject) != NULL)
			delete ((*yyvaluep).jsonObject);
		((*yyvaluep).jsonObject) = NULL;
	}
#line 905 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_member: /* member  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonMember) != NULL)
			delete ((*yyvaluep).jsonMember);
		((*yyvaluep).jsonMember) = NULL;
	}
#line 911 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_array: /* array  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonArray) != NULL)
			delete ((*yyvaluep).jsonArray);
		((*yyvaluep).jsonArray) = NULL;
	}
#line 917 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case YYSYMBOL_values: /* values  */
#line 69 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		if (((*yyvaluep).jsonArray) != NULL)
			delete ((*yyvaluep).jsonArray);
		((*yyvaluep).jsonArray) = NULL;
	}
#line 923 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	default:
		break;
	}
	YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;

/*----------.
| yyparse.  |
`----------*/

int yyparse(void)
{
	yy_state_fast_t yystate = 0;
	/* Number of tokens to shift before error messages enabled.  */
	int yyerrstatus = 0;

	/* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

	/* Their size.  */
	YYPTRDIFF_T yystacksize = YYINITDEPTH;

	/* The state stack: array, bottom, top.  */
	yy_state_t yyssa[YYINITDEPTH];
	yy_state_t* yyss = yyssa;
	yy_state_t* yyssp = yyss;

	/* The semantic value stack: array, bottom, top.  */
	YYSTYPE yyvsa[YYINITDEPTH];
	YYSTYPE* yyvs = yyvsa;
	YYSTYPE* yyvsp = yyvs;

	int yyn;
	/* The return value of yyparse.  */
	int yyresult;
	/* Lookahead symbol kind.  */
	yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
	/* The variables used to return semantic value and location from the
     action routines.  */
	YYSTYPE yyval;

#define YYPOPSTACK(N) (yyvsp -= (N), yyssp -= (N))

	/* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
	int yylen = 0;

	YYDPRINTF((stderr, "Starting parse\n"));

	yychar = JSONEMPTY; /* Cause a token to be read.  */

	goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
	/* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
	yyssp++;

/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
	YYDPRINTF((stderr, "Entering state %d\n", yystate));
	YY_ASSERT(0 <= yystate && yystate < YYNSTATES);
	YY_IGNORE_USELESS_CAST_BEGIN
	*yyssp = YY_CAST(yy_state_t, yystate);
	YY_IGNORE_USELESS_CAST_END
	YY_STACK_PRINT(yyss, yyssp);

	if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
		YYNOMEM;
#else
	{
		/* Get the current used size of the three stacks, in elements.  */
		YYPTRDIFF_T yysize = yyssp - yyss + 1;

#if defined yyoverflow
		{
			/* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
			yy_state_t* yyss1 = yyss;
			YYSTYPE* yyvs1 = yyvs;

			/* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
			yyoverflow(YY_("memory exhausted"), &yyss1, yysize * YYSIZEOF(*yyssp), &yyvs1,
				   yysize * YYSIZEOF(*yyvsp), &yystacksize);
			yyss = yyss1;
			yyvs = yyvs1;
		}
#else /* defined YYSTACK_RELOCATE */
		/* Extend the stack our own way.  */
		if (YYMAXDEPTH <= yystacksize)
			YYNOMEM;
		yystacksize *= 2;
		if (YYMAXDEPTH < yystacksize)
			yystacksize = YYMAXDEPTH;

		{
			yy_state_t* yyss1 = yyss;
			union yyalloc* yyptr =
			    YY_CAST(union yyalloc*, YYSTACK_ALLOC(YY_CAST(YYSIZE_T, YYSTACK_BYTES(yystacksize))));
			if (!yyptr)
				YYNOMEM;
			YYSTACK_RELOCATE(yyss_alloc, yyss);
			YYSTACK_RELOCATE(yyvs_alloc, yyvs);
#undef YYSTACK_RELOCATE
			if (yyss1 != yyssa)
				YYSTACK_FREE(yyss1);
		}
#endif

		yyssp = yyss + yysize - 1;
		yyvsp = yyvs + yysize - 1;

		YY_IGNORE_USELESS_CAST_BEGIN
		YYDPRINTF((stderr, "Stack size increased to %ld\n", YY_CAST(long, yystacksize)));
		YY_IGNORE_USELESS_CAST_END

		if (yyss + yystacksize - 1 <= yyssp)
			YYABORT;
	}
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

	if (yystate == YYFINAL)
		YYACCEPT;

	goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:
	/* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

	/* First try to decide what to do without reference to lookahead token.  */
	yyn = yypact[yystate];
	if (yypact_value_is_default(yyn))
		goto yydefault;

	/* Not known => get a lookahead token if don't already have one.  */

	/* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
	if (yychar == JSONEMPTY)
	{
		YYDPRINTF((stderr, "Reading a token\n"));
		yychar = yylex();
	}

	if (yychar <= JSONEOF)
	{
		yychar = JSONEOF;
		yytoken = YYSYMBOL_YYEOF;
		YYDPRINTF((stderr, "Now at end of input.\n"));
	}
	else if (yychar == JSONerror)
	{
		/* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
		yychar = JSONUNDEF;
		yytoken = YYSYMBOL_YYerror;
		goto yyerrlab1;
	}
	else
	{
		yytoken = YYTRANSLATE(yychar);
		YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
	}

	/* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
	yyn += yytoken;
	if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
		goto yydefault;
	yyn = yytable[yyn];
	if (yyn <= 0)
	{
		if (yytable_value_is_error(yyn))
			goto yyerrlab;
		yyn = -yyn;
		goto yyreduce;
	}

	/* Count tokens shifted since error; after three, turn off error
     status.  */
	if (yyerrstatus)
		yyerrstatus--;

	/* Shift the lookahead token.  */
	YY_SYMBOL_PRINT("Shifting", yytoken, &yylval, &yylloc);
	yystate = yyn;
	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

	/* Discard the shifted token.  */
	yychar = JSONEMPTY;
	goto yynewstate;

/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
	yyn = yydefact[yystate];
	if (yyn == 0)
		goto yyerrlab;
	goto yyreduce;

/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
	/* yyn is the number of a rule to reduce with.  */
	yylen = yyr2[yyn];

	/* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
	yyval = yyvsp[1 - yylen];

	YY_REDUCE_PRINT(yyn);
	switch (yyn)
	{
	case 2: /* jsonFile: object  */
#line 77 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonObject* jsonObject;
		JsonMember* member;
		int i;

		// On memorise l'objet construit, qui sera detruit par le parser
		jsonObject = (yyvsp[0].jsonObject);
		(yyval.jsonObject) = jsonObject;

		// Transfert du contenu vers l'objet principal
		assert(mainReadJsonObject->GetMemberNumber() == 0);
		for (i = 0; i < jsonObject->GetMemberNumber(); i++)
		{
			member = jsonObject->GetMemberAt(i);
			mainReadJsonObject->AddMember(member);
		}
		jsonObject->RemoveAll();
	}
#line 1210 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 3: /* jsonFile: error  */
#line 96 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonObject) = NULL;
		YYABORT;
	}
#line 1219 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 4: /* value: object  */
#line 103 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonValue) = (yyvsp[0].jsonObject);
	}
#line 1225 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 5: /* value: array  */
#line 104 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonValue) = (yyvsp[0].jsonArray);
	}
#line 1231 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 6: /* value: STRINGVALUE  */
#line 106 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonString* jsonString;

		// Creation d'une valeur chaine de caractere
		jsonString = new JsonString;
		(yyval.jsonValue) = jsonString;

		// Alimentation
		jsonString->SetString(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
	}
#line 1247 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 7: /* value: NUMBERVALUE  */
#line 118 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonNumber* jsonNumber;
		double dValue;

		// Creation d'une valeur chaine de caractere
		jsonNumber = new JsonNumber;
		(yyval.jsonValue) = jsonNumber;

		// Alimentation
		dValue = (yyvsp[0].dValue);
		jsonNumber->SetNumber(dValue);
	}
#line 1264 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 8: /* value: BOOLEANVALUE  */
#line 131 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonBoolean* jsonBoolean;
		boolean bValue;

		// Creation d'une valeur chaine de caractere
		jsonBoolean = new JsonBoolean;
		(yyval.jsonValue) = jsonBoolean;

		// Alimentation
		bValue = (yyvsp[0].bValue);
		jsonBoolean->SetBoolean(bValue);
	}
#line 1281 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 9: /* value: NULLVALUE  */
#line 144 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonNull* jsonNull;

		// Creation d'une valeur chaine de caractere
		jsonNull = new JsonNull;
		(yyval.jsonValue) = jsonNull;
	}
#line 1293 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 10: /* value: ERROR  */
#line 152 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		// Message d'erreur
		jsonerror("Invalid json value");

		// Nettoyage et sortie
		delete (yyvsp[0].sValue);
		(yyval.jsonValue) = NULL;
		YYABORT;
	}
#line 1307 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 11: /* object: '{' '}'  */
#line 164 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonObject) = new JsonObject;
	}
#line 1313 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 12: /* object: '{' members '}'  */
#line 166 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonObject) = (yyvsp[-1].jsonObject);
	}
#line 1319 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 13: /* members: member  */
#line 170 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonObject* jsonObject;
		JsonMember* member;

		// Creation d'un objet json
		jsonObject = new JsonObject;
		(yyval.jsonObject) = jsonObject;

		// Alimentation avec un premier membre d'objet
		member = cast(JsonMember*, (yyvsp[0].jsonMember));
		jsonObject->AddMember(member);
	}
#line 1336 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 14: /* members: members ',' member  */
#line 183 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonObject* jsonObject;
		JsonMember* member;
		ALString sJsonKey;

		// On recupere l'objet json en cours
		jsonObject = cast(JsonObject*, (yyvsp[-2].jsonObject));
		(yyval.jsonObject) = jsonObject;

		// Ajout d'un nouveau membre d'objet, s'il n'en existe pas deja un du meme nom
		member = cast(JsonMember*, (yyvsp[0].jsonMember));
		if (jsonObject->LookupMember(member->GetKey()) == NULL)
			jsonObject->AddMember(member);
		// Erreur sinon
		else
		{
			TextService::CToJsonString(member->GetKey(), sJsonKey);
			jsonerror("Duplicate key \"" + sJsonKey + "\" in json object");

			// Nettoyage et sortie
			delete member;
			delete jsonObject;
			(yyval.jsonObject) = NULL;
			YYABORT;
		}
	}
#line 1367 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 15: /* members: ERROR  */
#line 210 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		// Message d'erreur
		jsonerror("Invalid json key");

		// Nettoyage et sortie
		delete (yyvsp[0].sValue);
		(yyval.jsonObject) = NULL;
		YYABORT;
	}
#line 1381 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 16: /* member: STRINGVALUE ':' value  */
#line 222 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonMember* member;
		JsonValue* jsonValue;

		// Creation d'un membre d'objet
		member = new JsonMember;
		(yyval.jsonMember) = member;

		// Alimentation
		jsonValue = cast(JsonValue*, (yyvsp[0].jsonValue));
		member->SetKey(*(yyvsp[-2].sValue));
		member->SetValue(jsonValue);
		delete (yyvsp[-2].sValue);
	}
#line 1400 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 17: /* array: '[' ']'  */
#line 239 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonArray) = new JsonArray;
	}
#line 1406 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 18: /* array: '[' values ']'  */
#line 241 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		(yyval.jsonArray) = (yyvsp[-1].jsonArray);
	}
#line 1412 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 19: /* values: value  */
#line 245 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonArray* jsonArray;
		JsonValue* jsonValue;

		// Creation d'un tableau de valeur
		jsonArray = new JsonArray;
		(yyval.jsonArray) = jsonArray;

		// Alimentation avec un premier membre d'objet
		jsonValue = cast(JsonValue*, (yyvsp[0].jsonValue));
		jsonArray->AddValue(jsonValue);
	}
#line 1429 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

	case 20: /* values: values ',' value  */
#line 258 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"
	{
		JsonArray* jsonArray;
		JsonValue* jsonValue;

		// On recupere le tableau en cours
		jsonArray = cast(JsonArray*, (yyvsp[-2].jsonArray));
		(yyval.jsonArray) = jsonArray;

		// Ajout d'un nouveau membre d'objet
		jsonValue = cast(JsonValue*, (yyvsp[0].jsonValue));
		jsonArray->AddValue(jsonValue);
	}
#line 1446 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"
	break;

#line 1450 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.cpp"

	default:
		break;
	}
	/* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
	YY_SYMBOL_PRINT("-> $$ =", YY_CAST(yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

	YYPOPSTACK(yylen);
	yylen = 0;

	*++yyvsp = yyval;

	/* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
	{
		const int yylhs = yyr1[yyn] - YYNTOKENS;
		const int yyi = yypgoto[yylhs] + *yyssp;
		yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp ? yytable[yyi] : yydefgoto[yylhs]);
	}

	goto yynewstate;

/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
	/* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
	yytoken = yychar == JSONEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE(yychar);
	/* If not already recovering from an error, report this error.  */
	if (!yyerrstatus)
	{
		++yynerrs;
		yyerror(YY_("syntax error"));
	}

	if (yyerrstatus == 3)
	{
		/* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

		if (yychar <= JSONEOF)
		{
			/* Return failure if at end of input.  */
			if (yychar == JSONEOF)
				YYABORT;
		}
		else
		{
			yydestruct("Error: discarding", yytoken, &yylval);
			yychar = JSONEMPTY;
		}
	}

	/* Else will try to reuse lookahead token after shifting the error
     token.  */
	goto yyerrlab1;

/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
	/* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
	if (0)
		YYERROR;
	++yynerrs;

	/* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
	YYPOPSTACK(yylen);
	yylen = 0;
	YY_STACK_PRINT(yyss, yyssp);
	yystate = *yyssp;
	goto yyerrlab1;

/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
	yyerrstatus = 3; /* Each real token shifted decrements this.  */

	/* Pop stack until we find a state that shifts the error token.  */
	for (;;)
	{
		yyn = yypact[yystate];
		if (!yypact_value_is_default(yyn))
		{
			yyn += YYSYMBOL_YYerror;
			if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
			{
				yyn = yytable[yyn];
				if (0 < yyn)
					break;
			}
		}

		/* Pop the current state because it cannot handle the error token.  */
		if (yyssp == yyss)
			YYABORT;

		yydestruct("Error: popping", YY_ACCESSING_SYMBOL(yystate), yyvsp);
		YYPOPSTACK(1);
		yystate = *yyssp;
		YY_STACK_PRINT(yyss, yyssp);
	}

	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

	/* Shift the error token.  */
	YY_SYMBOL_PRINT("Shifting", YY_ACCESSING_SYMBOL(yyn), yyvsp, yylsp);

	yystate = yyn;
	goto yynewstate;

/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
	yyresult = 0;
	goto yyreturnlab;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
	yyresult = 1;
	goto yyreturnlab;

/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
	yyerror(YY_("memory exhausted"));
	yyresult = 2;
	goto yyreturnlab;

/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
	if (yychar != JSONEMPTY)
	{
		/* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
		yytoken = YYTRANSLATE(yychar);
		yydestruct("Cleanup: discarding lookahead", yytoken, &yylval);
	}
	/* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
	YYPOPSTACK(yylen);
	YY_STACK_PRINT(yyss, yyssp);
	while (yyssp != yyss)
	{
		yydestruct("Cleanup: popping", YY_ACCESSING_SYMBOL(+*yyssp), yyvsp);
		YYPOPSTACK(1);
	}
#ifndef yyoverflow
	if (yyss != yyssa)
		YYSTACK_FREE(yyss);
#endif

	return yyresult;
}

#line 272 "C:/Applications/boullema/DevGit/khiops/src/Norm/base/JsonYac.yac"

#include "JsonLex.inc"

/* default yyerror for YACC and LEX */
void jsonerror(char const* fmt)
{
	jsonerrorWithLineCorrection(fmt, 0);
}

/* Variante avec une correction du numero de ligne */
void jsonerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber)
{
	char sErrorLine[20];
	ALString sLabel;
	int nLineNumber;

	nJsonFileParsingErrorNumber++;
	nLineNumber = yylineno + nDeltaLineNumber;
	if (nLineNumber <= 0)
		nLineNumber = 1;
	snprintf(sErrorLine, sizeof(sErrorLine), "Line %d", nLineNumber);
	sLabel = fmt;
	Global::AddError("Read json file", sErrorLine, sLabel);
}

int jsonparse();

/* Implementation de la methode de lecture de fichier de KWClassDomain */
boolean JsonObject::ReadFile(const ALString& sFileName)
{
	boolean bOk = true;
	FILE* fFile;
	ALString sLocalFileName;

	/* Affichage de stats memoire si log memoire actif */
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile Begin");

	/* Initialisation de l'objet json principal a utiliser pour la lecture */
	assert(mainReadJsonObject == NULL);
	mainReadJsonObject = this;
	mainReadJsonObject->DeleteAll();

	/* Erreur si pas de nom de fichier */
	fFile = NULL;
	if (sFileName == "")
	{
		AddError("Missing file name");
		bOk = false;
	}
	/* Sinon, ouverture du fichier */
	else
	{
		// Copie depuis un fichier distant si necessaire
		bOk = PLRemoteFileService::BuildInputWorkingFile(sFileName, sLocalFileName);
		if (bOk)
			bOk = FileService::OpenInputBinaryFile(sLocalFileName, fFile);
	}

	/* On continue si fichier ouvert correctement */
	if (bOk)
	{
		assert(fFile != NULL);

		/* Activation du nombre max d'erreurs a afficher */
		nJsonFileParsingErrorNumber = 0;
		Global::ActivateErrorFlowControl();

		/* Positionnement du fichier a parser par la variable yyin de LEX */
		jsonlineno = 1;
		jsonrestart(fFile);

		/* Parsing */
		jsonparse();

		/* Cleaning lexer */
		jsonlex_destroy();

		/* Fermeture du fichier */
		FileService::CloseInputBinaryFile(sLocalFileName, fFile);

		/* Si fichier distant, on supprime la copie locale */
		PLRemoteFileService::CleanInputWorkingFile(sFileName, sLocalFileName);

		/* Desactivation du nombre max d'erreurs a afficher */
		Global::DesactivateErrorFlowControl();

		/* Nettoyage si erreurs */
		if (nJsonFileParsingErrorNumber > 0)
		{
			/* Destruction du contenu de l'objet construit */
			mainReadJsonObject->DeleteAll();

			/* En cas d'erreur, ajout d'une ligne blanche pour separer des autres logs */
			AddError("Errors detected during parsing " + sFileName + ": read operation cancelled");
			bOk = false;
		}
		nJsonFileParsingErrorNumber = 0;
	}

	/* Nettoyage */
	mainReadJsonObject = NULL;

	/* Affichage de stats memoire si log memoire actif */
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile End");

	return bOk;
}

void JsonObject::SetLineno(int nValue)
{
	jsonlineno = nValue;
}

int JsonObject::GetLineno()
{
	return jsonlineno;
}

void JsonObject::Restart(FILE* inputFile)
{
	jsonrestart(inputFile);
}

int JsonObject::Lex(JSONSTYPE* jsonValue)
{
	int nToken;

	require(jsonValue != NULL);

	// Lecture du token, puis de sa valeur
	nToken = jsonlex();
	*jsonValue = jsonlval;
	return nToken;
}

int JsonObject::LexDestroy()
{
	return jsonlex_destroy();
}
