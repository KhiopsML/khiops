// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* First part of user prologue.  */
#line 1 "KWCYac.yac" /* yacc.c:337  */

/* ATTENTION: les regles openparenthesis et closeparenthesis generent             */
/* 3 shift/reduce conflicts et 15 reduce/reduce conflicts                         */
/* La regle kwclassBegin genere 1 reduce/reduce conflict supplementaire           */
/* Attention: modifier ces regles en cas d'evolution du parser                    */
/* Ces regles ne sont utiles que pour le diagnostique des erreurs de parenthesage */
/* ou de rattrapage sur declaration d'attribut erronee                            */
/* Ces regles sont reperables par le mot cle ERRORMGT                             */

#include "KWClassDomain.h"
#include "KWClass.h"
#include "KWContinuous.h"
#include "KWDerivationRule.h"
#include "KWStructureRule.h"
#include "KWMetaData.h"

/* Declaration du lexer utilise */
void yyerror(char const* fmt);
void yyerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber);
int yylex();

/* Work around a bug in the relation between bison and GCC 3.x: */
#if defined(__GNUC__) && 3 <= __GNUC__
#define __attribute__(arglist)
#endif

/* Domaine de classe courant a utiliser pendant la lecture d'un fichier. */
/* Ce domaine est positionner par la methode Load de KWClassDomain       */
static KWClassDomain* kwcdLoadDomain = NULL;

/* Classe courante a utiliser pendant la lecture d'un fichier.     */
/* Ce domaine est positionner par la methode Load de KWClassDomain */
static KWClass* kwcLoadCurrentClass = NULL;

/* Dictionnaire des classes referencees creees a la volee lorsqu'elles sont      */
/* utilisees, mais non crees.                                                    */
/* On rajoute les classes referencees non crees, et on retire les classes crees. */
static ObjectDictionary* odReferencedUncreatedClasses = NULL;

/* Nombre total d'erreurs de parsing */
static int nFileParsingErrorNumber = 0;

#define YY_STATIC

/* Debugging YAC */

/*
#define YYDEBUG 1
extern char   *yyptok(int i);
*/

#line 126 "KWCYac.cpp" /* yacc.c:337  */
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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1
#else
#define YYERROR_VERBOSE 0
#endif

/* Debug traces.  */
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum yytokentype
{
	BASICIDENTIFIER = 258,
	EXTENDEDIDENTIFIER = 259,
	CONTINUOUSLITTERAL = 260,
	STRINGLITTERAL = 261,
	LABEL = 262,
	APPLICATIONID = 263,
	CLASS = 264,
	CONTINUOUSTYPE = 265,
	SYMBOLTYPE = 266,
	OBJECTTYPE = 267,
	OBJECTARRAYTYPE = 268,
	ROOT = 269,
	UNUSED = 270,
	DATETYPE = 271,
	TIMETYPE = 272,
	TIMESTAMPTYPE = 273,
	TEXTTYPE = 274,
	STRUCTURETYPE = 275
};
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 57 "KWCYac.yac" /* yacc.c:352  */

	Continuous cValue;
	ALString* sValue;
	boolean bValue;
	StringVector* svValue;
	KWDerivationRule* kwdrValue;
	KWDerivationRuleOperand* kwdroValue;
	KWClass* kwcValue;
	KWAttribute* kwaValue;
	ObjectArray* oaAttributes;
	KWMetaData* kwmdMetaData;
	int nValue;

#line 201 "KWCYac.cpp" /* yacc.c:352  */
};

typedef union YYSTYPE YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

int yyparse(void);

#ifdef short
#undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif !defined YYSIZE_T
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned
#endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T)-1)

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

#ifndef YY_ATTRIBUTE
#if (defined __GNUC__ && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__))) ||                                 \
    defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#define YY_ATTRIBUTE(Spec) __attribute__(Spec)
#else
#define YY_ATTRIBUTE(Spec) /* empty */
#endif
#endif

#ifndef YY_ATTRIBUTE_PURE
#define YY_ATTRIBUTE_PURE YY_ATTRIBUTE((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if !defined lint || defined __GNUC__
#define YYUSE(E) ((void)(E))
#else
#define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && !defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                                                            \
	_Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")                           \
	    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
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

#if !defined yyoverflow || YYERROR_VERBOSE

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
void free(void*);       /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */

#if (!defined yyoverflow && (!defined __cplusplus || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
	yytype_int16 yyss_alloc;
	YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (sizeof(union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#define YYSTACK_BYTES(N) ((N) * (sizeof(yytype_int16) + sizeof(YYSTYPE)) + YYSTACK_GAP_MAXIMUM)

#define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack_alloc, Stack)                                                                           \
	do                                                                                                             \
	{                                                                                                              \
		YYSIZE_T yynewbytes;                                                                                   \
		YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                                                            \
		Stack = &yyptr->Stack_alloc;                                                                           \
		yynewbytes = yystacksize * sizeof(*Stack) + YYSTACK_GAP_MAXIMUM;                                       \
		yyptr += yynewbytes / sizeof(*yyptr);                                                                  \
	} while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined __GNUC__ && 1 < __GNUC__
#define YYCOPY(Dst, Src, Count) __builtin_memcpy(Dst, Src, (Count) * sizeof(*(Src)))
#else
#define YYCOPY(Dst, Src, Count)                                                                                        \
	do                                                                                                             \
	{                                                                                                              \
		YYSIZE_T yyi;                                                                                          \
		for (yyi = 0; yyi < (Count); yyi++)                                                                    \
			(Dst)[yyi] = (Src)[yyi];                                                                       \
	} while (0)
#endif
#endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 171

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 34
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 31
/* YYNRULES -- Number of rules.  */
#define YYNRULES 90
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 133

#define YYUNDEFTOK 2
#define YYMAXUTOK 275

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX) ((unsigned)(YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] = {
    0,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 23, 24, 2, 32, 25, 2, 31, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 33, 26, 27,
    28, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  30, 2,
    29, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  22,
    2,  21, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 1, 2,  3,  4, 5,  6,  7, 8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] = {
    0,    132,  132,  136,  143,  147,  151,  155,  159,  163,  167,  171,  175,  179,  183,  187,  191,  197,  210,
    211,  214,  217,  227,  235,  278,  357,  376,  387,  432,  481,  573,  580,  588,  594,  604,  617,  637,  656,
    675,  692,  700,  830,  842,  849,  861,  868,  873,  880,  885,  892,  896,  900,  904,  908,  912,  918,  922,
    926,  934,  939,  945,  949,  953,  958,  967,  972,  978,  998,  1014, 1187, 1191, 1198, 1211, 1222, 1236, 1245,
    1254, 1264, 1274, 1285, 1294, 1301, 1302, 1306, 1311, 1317, 1318, 1322, 1327, 1339, 1341};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = {"$end",
				      "error",
				      "$undefined",
				      "BASICIDENTIFIER",
				      "EXTENDEDIDENTIFIER",
				      "CONTINUOUSLITTERAL",
				      "STRINGLITTERAL",
				      "LABEL",
				      "APPLICATIONID",
				      "CLASS",
				      "CONTINUOUSTYPE",
				      "SYMBOLTYPE",
				      "OBJECTTYPE",
				      "OBJECTARRAYTYPE",
				      "ROOT",
				      "UNUSED",
				      "DATETYPE",
				      "TIMETYPE",
				      "TIMESTAMPTYPE",
				      "TEXTTYPE",
				      "STRUCTURETYPE",
				      "'}'",
				      "'{'",
				      "'('",
				      "')'",
				      "','",
				      "'<'",
				      "'='",
				      "'>'",
				      "']'",
				      "'['",
				      "'.'",
				      "'+'",
				      "';'",
				      "$accept",
				      "IDENTIFIER",
				      "SIMPLEIDENTIFIER",
				      "kwclassFile",
				      "kwclasses",
				      "kwclass",
				      "kwclassBegin",
				      "oaAttributeArrayDeclaration",
				      "kwclassHeader",
				      "keyFields",
				      "keyFieldList",
				      "metaData",
				      "kwattributeDeclaration",
				      "applicationids",
				      "comments",
				      "rootDeclaration",
				      "usedDeclaration",
				      "typeDeclaration",
				      "refIdentifier",
				      "usedDerivationRule",
				      "referenceRule",
				      "referenceRuleBody",
				      "derivationRule",
				      "derivationRuleBody",
				      "derivationRuleHeader",
				      "derivationRuleBegin",
				      "derivationRuleOperand",
				      "bigstring",
				      "semicolon",
				      "openparenthesis",
				      "closeparenthesis",
				      YY_NULLPTR};
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] = {0,   256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266,
					 267, 268, 269, 270, 271, 272, 273, 274, 275, 125, 123, 40,
					 41,  44,  60,  61,  62,  93,  91,  46,  43,  59};
#endif

#define YYPACT_NINF -61

#define yypact_value_is_default(Yystate) (!!((Yystate) == (-61)))

#define YYTABLE_NINF -89

#define yytable_value_is_error(Yytable_value) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
    -61, 4,   20,  -61, -61, 82,  -61, -61, 57,  -61, 79,  -61, -61, 9,   -61, -61, 151, 44,  -61, -61, 51,  59,  -61,
    40,  -61, -61, -61, -61, -61, -61, -61, -61, -61, 39,  122, 61,  122, 29,  -61, 122, 122, -61, -61, -61, -61, -61,
    -61, -61, -61, -61, -61, -61, -61, -61, -61, 6,   -61, -61, 72,  122, -61, 76,  72,  122, -61, 44,  122, 122, 5,
    9,   -61, 55,  72,  -61, 9,   -61, 73,  37,  81,  58,  87,  103, 5,   78,  -61, -61, 5,   34,  -61, -61, 74,  -61,
    5,   -61, 9,   -61, -61, 122, -61, 140, -61, 105, -61, -61, -61, -61, -61, 5,   -61, 123, 118, -61, -61, 118, 44,
    -61, 26,  107, -61, -61, 44,  118, 44,  104, -61, -61, 44,  117, 119, 120, -61, -61, -61};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] = {
    42, 0,  20, 1,  41, 0,  19, 18, 0,  44, 17, 26, 47, 84, 44, 23, 0,  22, 43, 45, 0,  81, 21, 48, 49, 50, 55,
    56, 51, 52, 53, 54, 57, 59, 0,  82, 0,  48, 28, 0,  0,  4,  3,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 32, 2,  83, 64, 0,  27, 0,  64, 0,  39, 31, 0,  0,  0,  84, 61, 0,  64, 58, 84, 34, 0,  0,  0,  88, 60,
    90, 70, 69, 75, 80, 0,  74, 77, 66, 76, 39, 0,  65, 84, 39, 44, 0,  29, 0,  63, 85, 71, 62, 89, 68, 72, 0,
    78, 0,  44, 67, 39, 44, 30, 33, 0,  86, 73, 79, 25, 44, 40, 0,  37, 87, 24, 0,  0,  0,  36, 35, 38};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-61, -33, 47,  -61, -61, -61, -61, -61, -61, -61, -61, -46, 48,  -61, -9, -61,
				      -61, -61, -61, -60, -61, -61, 98,  -61, -61, -61, -51, -61, -42, -61, -61};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] = {-1, 87, 56, 1,  5,  7,  8,  37, 9,  64, 76, 77, 15, 2,   10, 20,
					16, 33, 40, 69, 70, 71, 88, 81, 82, 83, 89, 90, 22, 102, 105};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = {
    17,  55,  74,  58,  3,   23,  61, 62,  41,  42,  84,  85,  94,  -44, 43,  44,  45,  46,  47,  48,  49,  50,
    51,  52,  53,  54,  72,  91,  4,  63,  75,  106, 95,  78,  79,  108, 86,  -88, -88, -88, -88, 111, 21,  -88,
    12,  110, 65,  18,  -88, 113, 59, 18,  112, 123, 124, 12,  118, 101, 11,  98,  34,  36,  39,  99,  115, -88,
    121, -48, -48, -48, -48, 38,  12, -48, -48, -48, -48, -48, 13,  14,  92,  101, -44, 6,   93,  60,  18,  114,
    -46, -44, 129, -44, 35,  19,  57, 66,  -44, 96,  97,  67,  73,  120, 68,  107, 122, 100, 109, 41,  42,  127,
    128, 103, 126, 43,  44,  45,  46, 47,  48,  49,  50,  51,  52,  53,  54,  41,  42,  104, 117, 119, 125, 43,
    44,  45,  46,  47,  48,  49,  50, 51,  52,  53,  54,  41,  99,  130, 116, 131, 132, 43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54, 24,  25,  26,  27,  80,  0,   28,  29,  30,  31,  32};

static const yytype_int8 yycheck[] = {
    9,  34,  62, 36, 0,   14, 39,  40, 3,  4,  5,  6,  72,  7,  9,  10,  11,  12, 13, 14, 15, 16, 17, 18, 19,
    20, 59,  69, 8,  23,  63, 82,  74, 66, 67, 86, 31, 3,   4,  5,  6,   92,  33, 9,  15, 91, 55, 7,  14, 95,
    21, 7,   94, 27, 28,  15, 107, 23, 1,  22, 9,  21, 23,  26, 97, 31,  112, 10, 11, 12, 13, 23, 15, 16, 17,
    18, 19,  20, 21, 22,  25, 23,  0,  1,  29, 37, 7,  96,  9,  7,  123, 9,   33, 14, 33, 23, 14, 24, 25, 27,
    24, 110, 30, 25, 113, 24, 32,  3,  4,  5,  6,  24, 121, 9,  10, 11,  12,  13, 14, 15, 16, 17, 18, 19, 20,
    3,  4,   24, 23, 6,   23, 9,   10, 11, 12, 13, 14, 15,  16, 17, 18,  19,  20, 3,  26, 28, 99, 28, 28, 9,
    10, 11,  12, 13, 14,  15, 16,  17, 18, 19, 20, 10, 11,  12, 13, 67,  -1,  16, 17, 18, 19, 20};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] = {
    0,  37, 47, 0,  8,  38, 1,  39, 40, 42, 48, 1,  15, 21, 22, 46, 50, 48, 7,  14, 49, 33, 62, 48, 10, 11, 12,
    13, 16, 17, 18, 19, 20, 51, 9,  33, 21, 41, 46, 23, 52, 3,  4,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 35, 36, 33, 35, 21, 46, 35, 35, 23, 43, 48, 23, 27, 30, 53, 54, 55, 35, 24, 53, 35, 44, 45, 35, 35, 56,
    57, 58, 59, 5,  6,  31, 35, 56, 60, 61, 62, 25, 29, 53, 62, 24, 25, 22, 26, 24, 23, 63, 24, 24, 64, 60, 25,
    60, 32, 45, 60, 62, 45, 48, 35, 36, 23, 60, 6,  48, 45, 48, 27, 28, 23, 48, 5,  6,  35, 28, 28, 28};

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] = {0,  34, 35, 35, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 37, 38,
				    38, 38, 39, 40, 40, 40, 40, 40, 41, 41, 42, 43, 43, 43, 44, 44, 45, 45, 45,
				    45, 45, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51, 51, 51, 51, 51, 51, 51, 51,
				    51, 52, 52, 53, 53, 53, 53, 53, 54, 55, 55, 56, 57, 57, 58, 59, 59, 60, 60,
				    60, 60, 60, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63, 64, 64};

/* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] = {0, 2,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 0, 3, 2,
				    2, 10, 9, 2, 2, 1, 7, 4, 1, 0, 3, 1, 6, 6, 4, 6, 0, 8, 2, 0, 2, 0, 1,
				    0, 1,  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 2, 1, 3, 3, 0, 2, 2, 3, 2,
				    1, 1,  2, 2, 3, 1, 1, 1, 1, 2, 3, 1, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0};

#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)
#define YYEMPTY (-2)
#define YYEOF 0

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab

#define YYRECOVERING() (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                                                                         \
	do                                                                                                             \
		if (yychar == YYEMPTY)                                                                                 \
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

/* Error token number */
#define YYTERROR 1
#define YYERRCODE 256

/* Enable debugging if requested.  */
#if YYDEBUG

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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
#define YY_LOCATION_PRINT(File, Loc) ((void)0)
#endif

#define YY_SYMBOL_PRINT(Title, Type, Value, Location)                                                                  \
	do                                                                                                             \
	{                                                                                                              \
		if (yydebug)                                                                                           \
		{                                                                                                      \
			YYFPRINTF(stderr, "%s ", Title);                                                               \
			yy_symbol_print(stderr, Type, Value);                                                          \
			YYFPRINTF(stderr, "\n");                                                                       \
		}                                                                                                      \
	} while (0)

/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE* yyo, int yytype, YYSTYPE const* const yyvaluep)
{
	FILE* yyoutput = yyo;
	YYUSE(yyoutput);
	if (!yyvaluep)
		return;
#ifdef YYPRINT
	if (yytype < YYNTOKENS)
		YYPRINT(yyo, yytoknum[yytype], *yyvaluep);
#endif
	YYUSE(yytype);
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE* yyo, int yytype, YYSTYPE const* const yyvaluep)
{
	YYFPRINTF(yyo, "%s %s (", yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

	yy_symbol_value_print(yyo, yytype, yyvaluep);
	YYFPRINTF(yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void yy_stack_print(yytype_int16* yybottom, yytype_int16* yytop)
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

static void yy_reduce_print(yytype_int16* yyssp, YYSTYPE* yyvsp, int yyrule)
{
	unsigned long yylno = yyrline[yyrule];
	int yynrhs = yyr2[yyrule];
	int yyi;
	YYFPRINTF(stderr, "Reducing stack by rule %d (line %lu):\n", yyrule - 1, yylno);
	/* The symbols being reduced.  */
	for (yyi = 0; yyi < yynrhs; yyi++)
	{
		YYFPRINTF(stderr, "   $%d = ", yyi + 1);
		yy_symbol_print(stderr, yystos[yyssp[yyi + 1 - yynrhs]], &yyvsp[(yyi + 1) - (yynrhs)]);
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
#else /* !YYDEBUG */
#define YYDPRINTF(Args)
#define YY_SYMBOL_PRINT(Title, Type, Value, Location)
#define YY_STACK_PRINT(Bottom, Top)
#define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */

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

#if YYERROR_VERBOSE

#ifndef yystrlen
#if defined __GLIBC__ && defined _STRING_H
#define yystrlen strlen
#else
/* Return the length of YYSTR.  */
static YYSIZE_T yystrlen(const char* yystr)
{
	YYSIZE_T yylen;
	for (yylen = 0; yystr[yylen]; yylen++)
		continue;
	return yylen;
}
#endif
#endif

#ifndef yystpcpy
#if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#define yystpcpy stpcpy
#else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char* yystpcpy(char* yydest, const char* yysrc)
{
	char* yyd = yydest;
	const char* yys = yysrc;

	while ((*yyd++ = *yys++) != '\0')
		continue;

	return yyd - 1;
}
#endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T yytnamerr(char* yyres, const char* yystr)
{
	if (*yystr == '"')
	{
		YYSIZE_T yyn = 0;
		char const* yyp = yystr;

		for (;;)
			switch (*++yyp)
			{
			case '\'':
			case ',':
				goto do_not_strip_quotes;

			case '\\':
				if (*++yyp != '\\')
					goto do_not_strip_quotes;
				else
					goto append;

			append:
			default:
				if (yyres)
					yyres[yyn] = *yyp;
				yyn++;
				break;

			case '"':
				if (yyres)
					yyres[yyn] = '\0';
				return yyn;
			}
	do_not_strip_quotes:;
	}

	if (!yyres)
		return yystrlen(yystr);

	return (YYSIZE_T)(yystpcpy(yyres, yystr) - yyres);
}
#endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int yysyntax_error(YYSIZE_T* yymsg_alloc, char** yymsg, yytype_int16* yyssp, int yytoken)
{
	YYSIZE_T yysize0 = yytnamerr(YY_NULLPTR, yytname[yytoken]);
	YYSIZE_T yysize = yysize0;
	enum
	{
		YYERROR_VERBOSE_ARGS_MAXIMUM = 5
	};
	/* Internationalized format string. */
	const char* yyformat = YY_NULLPTR;
	/* Arguments of yyformat. */
	char const* yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	/* Number of reported tokens (one for the "unexpected", one per
	   "expected"). */
	int yycount = 0;

	/* There are many possibilities here to consider:
	   - If this state is a consistent state with a default action, then
	     the only way this function was invoked is if the default action
	     is an error action.  In that case, don't check for expected
	     tokens because there are none.
	   - The only way there can be no lookahead present (in yychar) is if
	     this state is a consistent state with a default action.  Thus,
	     detecting the absence of a lookahead is sufficient to determine
	     that there is no unexpected or expected token to report.  In that
	     case, just report a simple "syntax error".
	   - Don't assume there isn't a lookahead just because this state is a
	     consistent state with a default action.  There might have been a
	     previous inconsistent state, consistent state with a non-default
	     action, or user semantic action that manipulated yychar.
	   - Of course, the expected token list depends on states to have
	     correct lookahead information, and it depends on the parser not
	     to perform extra reductions after fetching a lookahead from the
	     scanner and before detecting a syntax error.  Thus, state merging
	     (from LALR or IELR) and default reductions corrupt the expected
	     token list.  However, the list is correct for canonical LR with
	     one exception: it will still contain any token that will not be
	     accepted due to an error action in a later state.
	*/
	if (yytoken != YYEMPTY)
	{
		int yyn = yypact[*yyssp];
		yyarg[yycount++] = yytname[yytoken];
		if (!yypact_value_is_default(yyn))
		{
			/* Start YYX at -YYN if negative to avoid negative indexes in
			   YYCHECK.  In other words, skip the first -YYN actions for
			   this state because they are default actions.  */
			int yyxbegin = yyn < 0 ? -yyn : 0;
			/* Stay within bounds of both yycheck and yytname.  */
			int yychecklim = YYLAST - yyn + 1;
			int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
			int yyx;

			for (yyx = yyxbegin; yyx < yyxend; ++yyx)
				if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR &&
				    !yytable_value_is_error(yytable[yyx + yyn]))
				{
					if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
					{
						yycount = 1;
						yysize = yysize0;
						break;
					}
					yyarg[yycount++] = yytname[yyx];
					{
						YYSIZE_T yysize1 = yysize + yytnamerr(YY_NULLPTR, yytname[yyx]);
						if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
							yysize = yysize1;
						else
							return 2;
					}
				}
		}
	}

	switch (yycount)
	{
#define YYCASE_(N, S)                                                                                                  \
	case N:                                                                                                        \
		yyformat = S;                                                                                          \
		break
	default: /* Avoid compiler warnings. */
		YYCASE_(0, YY_("syntax error"));
		YYCASE_(1, YY_("syntax error, unexpected %s"));
		YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
		YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
		YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
		YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
	}

	{
		YYSIZE_T yysize1 = yysize + yystrlen(yyformat);
		if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
			yysize = yysize1;
		else
			return 2;
	}

	if (*yymsg_alloc < yysize)
	{
		*yymsg_alloc = 2 * yysize;
		if (!(yysize <= *yymsg_alloc && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
			*yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
		return 1;
	}

	/* Avoid sprintf, as that infringes on the user's name space.
	   Don't have undefined behavior even if the translation
	   produced a string with the wrong number of "%s"s.  */
	{
		char* yyp = *yymsg;
		int yyi = 0;
		while ((*yyp = *yyformat) != '\0')
			if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
			{
				yyp += yytnamerr(yyp, yyarg[yyi++]);
				yyformat += 2;
			}
			else
			{
				yyp++;
				yyformat++;
			}
	}
	return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void yydestruct(const char* yymsg, int yytype, YYSTYPE* yyvaluep)
{
	YYUSE(yyvaluep);
	if (!yymsg)
		yymsg = "Deleting";
	YY_SYMBOL_PRINT(yymsg, yytype, yyvaluep, yylocationp);

	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	switch (yytype)
	{
	case 3:        /* BASICIDENTIFIER  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1148 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 4:        /* EXTENDEDIDENTIFIER  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1154 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 6:        /* STRINGLITTERAL  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1160 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 7:        /* LABEL  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1166 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 8:        /* APPLICATIONID  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1172 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 35:       /* IDENTIFIER  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1178 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 36:       /* SIMPLEIDENTIFIER  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1184 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 41:       /* oaAttributeArrayDeclaration  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).oaAttributes) != NULL)
			delete ((*yyvaluep).oaAttributes);
		((*yyvaluep).oaAttributes) = NULL;
	}
#line 1190 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 43:       /* keyFields  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1196 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 44:       /* keyFieldList  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1202 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 45:       /* metaData  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwmdMetaData) != NULL)
			delete ((*yyvaluep).kwmdMetaData);
		((*yyvaluep).kwmdMetaData) = NULL;
	}
#line 1208 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 46:       /* kwattributeDeclaration  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwaValue) != NULL)
			delete ((*yyvaluep).kwaValue);
		((*yyvaluep).kwaValue) = NULL;
	}
#line 1214 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 47:       /* applicationids  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1220 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 48:       /* comments  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1226 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 52:       /* refIdentifier  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1232 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 53:       /* usedDerivationRule  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1238 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 54:       /* referenceRule  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1244 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 55:       /* referenceRuleBody  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1250 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 56:       /* derivationRule  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1256 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 57:       /* derivationRuleBody  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1262 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 58:       /* derivationRuleHeader  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1268 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 59:       /* derivationRuleBegin  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1274 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 60:       /* derivationRuleOperand  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).kwdroValue) != NULL)
			delete ((*yyvaluep).kwdroValue);
		((*yyvaluep).kwdroValue) = NULL;
	}
#line 1280 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	case 61:       /* bigstring  */
#line 123 "KWCYac.yac" /* yacc.c:1257  */
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1286 "KWCYac.cpp" /* yacc.c:1257  */
	break;

	default:
		break;
	}
	YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/* The lookahead symbol.  */
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
	int yystate;
	/* Number of tokens to shift before error messages enabled.  */
	int yyerrstatus;

	/* The stacks and their tools:
	   'yyss': related to states.
	   'yyvs': related to semantic values.

	   Refer to the stacks through separate pointers, to allow yyoverflow
	   to reallocate them elsewhere.  */

	/* The state stack.  */
	yytype_int16 yyssa[YYINITDEPTH];
	yytype_int16* yyss;
	yytype_int16* yyssp;

	/* The semantic value stack.  */
	YYSTYPE yyvsa[YYINITDEPTH];
	YYSTYPE* yyvs;
	YYSTYPE* yyvsp;

	YYSIZE_T yystacksize;

	int yyn;
	int yyresult;
	/* Lookahead token as an internal (translated) token number.  */
	int yytoken = 0;
	/* The variables used to return semantic value and location from the
	   action routines.  */
	YYSTYPE yyval;

#if YYERROR_VERBOSE
	/* Buffer for error messages, and its allocated size.  */
	char yymsgbuf[128];
	char* yymsg = yymsgbuf;
	YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N) (yyvsp -= (N), yyssp -= (N))

	/* The number of symbols on the RHS of the reduced rule.
	   Keep to zero when no symbol should be popped.  */
	int yylen = 0;

	yyssp = yyss = yyssa;
	yyvsp = yyvs = yyvsa;
	yystacksize = YYINITDEPTH;

	YYDPRINTF((stderr, "Starting parse\n"));

	yystate = 0;
	yyerrstatus = 0;
	yynerrs = 0;
	yychar = YYEMPTY; /* Cause a token to be read.  */
	goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
	/* In all cases, when you get here, the value and location stacks
	   have just been pushed.  So pushing a state here evens the stacks.  */
	yyssp++;

/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
	*yyssp = (yytype_int16)yystate;

	if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
		goto yyexhaustedlab;
#else
	{
		/* Get the current used size of the three stacks, in elements.  */
		YYSIZE_T yysize = (YYSIZE_T)(yyssp - yyss + 1);

#if defined yyoverflow
		{
			/* Give user a chance to reallocate the stack.  Use copies of
			   these so that the &'s don't force the real ones into
			   memory.  */
			YYSTYPE* yyvs1 = yyvs;
			yytype_int16* yyss1 = yyss;

			/* Each stack pointer address is followed by the size of the
			   data in use in that stack, in bytes.  This used to be a
			   conditional around just the two extra args, but that might
			   be undefined if yyoverflow is a macro.  */
			yyoverflow(YY_("memory exhausted"), &yyss1, yysize * sizeof(*yyssp), &yyvs1,
				   yysize * sizeof(*yyvsp), &yystacksize);
			yyss = yyss1;
			yyvs = yyvs1;
		}
#else /* defined YYSTACK_RELOCATE */
		/* Extend the stack our own way.  */
		if (YYMAXDEPTH <= yystacksize)
			goto yyexhaustedlab;
		yystacksize *= 2;
		if (YYMAXDEPTH < yystacksize)
			yystacksize = YYMAXDEPTH;

		{
			yytype_int16* yyss1 = yyss;
			union yyalloc* yyptr = (union yyalloc*)YYSTACK_ALLOC(YYSTACK_BYTES(yystacksize));
			if (!yyptr)
				goto yyexhaustedlab;
			YYSTACK_RELOCATE(yyss_alloc, yyss);
			YYSTACK_RELOCATE(yyvs_alloc, yyvs);
#undef YYSTACK_RELOCATE
			if (yyss1 != yyssa)
				YYSTACK_FREE(yyss1);
		}
#endif

		yyssp = yyss + yysize - 1;
		yyvsp = yyvs + yysize - 1;

		YYDPRINTF((stderr, "Stack size increased to %lu\n", (unsigned long)yystacksize));

		if (yyss + yystacksize - 1 <= yyssp)
			YYABORT;
	}
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

	YYDPRINTF((stderr, "Entering state %d\n", yystate));

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

	/* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
	if (yychar == YYEMPTY)
	{
		YYDPRINTF((stderr, "Reading a token: "));
		yychar = yylex();
	}

	if (yychar <= YYEOF)
	{
		yychar = yytoken = YYEOF;
		YYDPRINTF((stderr, "Now at end of input.\n"));
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

	/* Discard the shifted token.  */
	yychar = YYEMPTY;

	yystate = yyn;
	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

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
	case 2:
#line 133 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1555 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 3:
#line 137 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1563 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 4:
#line 144 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1571 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 5:
#line 148 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Dictionary");
	}
#line 1579 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 6:
#line 152 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Numerical");
	}
#line 1587 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 7:
#line 156 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Categorical");
	}
#line 1595 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 8:
#line 160 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Entity");
	}
#line 1603 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 9:
#line 164 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Table");
	}
#line 1611 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 10:
#line 168 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Root");
	}
#line 1619 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 11:
#line 172 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Unused");
	}
#line 1627 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 12:
#line 176 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Date");
	}
#line 1635 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 13:
#line 180 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Time");
	}
#line 1643 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 14:
#line 184 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Timestamp");
	}
#line 1651 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 15:
#line 188 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Text");
	}
#line 1659 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 16:
#line 192 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = new ALString("Structure");
	}
#line 1667 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 17:
#line 198 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ignore l'identification d'application */
		if ((yyvsp[-2].sValue) != NULL)
			delete (yyvsp[-2].sValue);

		/* On ignore les commentaires en fin de fichier */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
	}
#line 1681 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 19:
#line 212 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("Error outside the definition of a dictionary");
		YYABORT;
	}
#line 1688 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 21:
#line 218 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* La completion des informations de type (CompleteTypeInfo) est centralisee */
		/* au niveau du domaine en fin de parsing */

		/* Reinitialisation de la classe courante */
		kwcLoadCurrentClass = NULL;
	}
#line 1700 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 22:
#line 228 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ignore les premiers comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));
		(yyval.kwcValue) = (yyvsp[-1].kwcValue);
	}
#line 1712 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 23:
#line 236 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWClass* kwcClass = (yyvsp[-1].kwcValue);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));
		boolean bOk;

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckName(attribute->GetName(), kwcClass))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ": Incorrect variable name",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les attributs */
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les blocs */
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Si OK, d'insertion */
		else
		{
			bOk = kwcClass->InsertAttribute(attribute);
			assert(bOk);
		}

		(yyval.kwcValue) = kwcClass;
	}
#line 1759 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 24:
#line 279 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWClass* kwcClass = (yyvsp[-9].kwcValue);
		KWAttributeBlock* attributeBlock;
		ObjectArray* oaAttributes = (yyvsp[-6].oaAttributes);
		ALString sBlockName;
		KWAttribute* firstAttribute;
		KWAttribute* lastAttribute;
		KWDerivationRule* rule = (yyvsp[-3].kwdrValue);
		assert(kwcLoadCurrentClass == (yyvsp[-9].kwcValue));
		check(oaAttributes);

		/* On ignore les premiers comemntaires */
		if ((yyvsp[-7].sValue) != NULL)
			delete (yyvsp[-7].sValue);

		/* Nom du bloc */
		if ((yyvsp[-4].sValue) != NULL)
			sBlockName = *((yyvsp[-4].sValue));

		/* Cas d'un bloc avec au moins un attribut valide */
		if (oaAttributes->GetSize() > 0)
		{
			/* Test de validite du nom de l'attribut */
			if (!kwcClass->CheckName(sBlockName, kwcClass))
			{
				yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
							      ": Incorrect sparse variable block name (" + sBlockName +
							      ")",
							  -1);
			}
			/* Test de non existence parmi les attributs */
			else if (kwcClass->LookupAttribute(sBlockName) != NULL)
			{
				yyerrorWithLineCorrection(
				    "Dictionary " + kwcClass->GetName() +
					": Sparse variable block name already used by a variable (" + sBlockName + ")",
				    -1);
			}
			/* Test de non existence parmi les blocs */
			else if (kwcClass->LookupAttributeBlock(sBlockName) != NULL)
			{
				yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
							      ": Sparse variable block name already used by a block (" +
							      sBlockName + ")",
							  -1);
			}
			/* Creation du bloc dans la classe */
			else
			{
				/* Creation du bloc */
				firstAttribute = cast(KWAttribute*, oaAttributes->GetAt(0));
				lastAttribute = cast(KWAttribute*, oaAttributes->GetAt(oaAttributes->GetSize() - 1));
				attributeBlock =
				    kwcClass->CreateAttributeBlock(sBlockName, firstAttribute, lastAttribute);

				/* Parametrage du bloc */
				attributeBlock->SetDerivationRule(rule);
				if ((yyvsp[-1].kwmdMetaData) != NULL)
					attributeBlock->GetMetaData()->CopyFrom((yyvsp[-1].kwmdMetaData));
				if ((yyvsp[0].sValue) != NULL)
					attributeBlock->SetLabel(*((yyvsp[0].sValue)));

				/* On marque la rule a NULL pour indiquer qu'elle est utilisee */
				rule = NULL;
			}
		}

		/* Destruction de l'eventuelle regle si non utilisee */
		if (rule != NULL)
			delete rule;

		/* Tous les attributs du tableau ont deja ete inseres dans la classe */
		// On se contente de detruire le tableau */
		delete oaAttributes;

		/* Nettoyage */
		if ((yyvsp[-4].sValue) != NULL)
			delete (yyvsp[-4].sValue);
		if ((yyvsp[-1].kwmdMetaData) != NULL)
			delete (yyvsp[-1].kwmdMetaData);
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);

		(yyval.kwcValue) = kwcClass;
	}
#line 1842 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 25:
#line 358 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWClass* kwcClass = (yyvsp[-8].kwcValue);

		/* Message d'erreur */
		yyerror("Empty sparse variable block not allowed");

		/* Nettoyage */
		if ((yyvsp[-6].sValue) != NULL)
			delete (yyvsp[-6].sValue);
		delete (yyvsp[-4].sValue);
		if ((yyvsp[-3].kwdrValue) != NULL)
			delete (yyvsp[-3].kwdrValue);
		if ((yyvsp[-1].kwmdMetaData) != NULL)
			delete (yyvsp[-1].kwmdMetaData);
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.kwcValue) = kwcClass;
	}
#line 1865 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 26:
#line 377 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* ERRORMGT */
		/* Attention: cette regle qui permet une gestion des erreurs amelioree */
		/* genere un conflit reduce/reduce */
		kwcLoadCurrentClass = NULL;
		YYABORT;
	}
#line 1877 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 27:
#line 388 "KWCYac.yac" /* yacc.c:1652  */
	{
		ObjectArray* oaAttributes = (yyvsp[-1].oaAttributes);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
		boolean bOk;
		check(oaAttributes);

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckName(attribute->GetName(), kwcClass))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ": Incorrect variable name (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les attributs */
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les blocs */
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Si OK, d'insertion */
		else
		{
			bOk = kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
			assert(bOk);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1926 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 28:
#line 433 "KWCYac.yac" /* yacc.c:1652  */
	{
		ObjectArray* oaAttributes;
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
		boolean bOk;

		/* Creation d'un tableau */
		oaAttributes = new ObjectArray;

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckName(attribute->GetName(), kwcClass))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ": Incorrect variable name (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les attributs */
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Test de non existence parmi les blocs */
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		/* Si OK, d'insertion */
		else
		{
			bOk = kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
			assert(bOk);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1977 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 29:
#line 482 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWClass* kwcClass;
		KWClass* kwcReferencedClass;

		/* Test d'existence de la classe */
		kwcClass = kwcdLoadDomain->LookupClass(*((yyvsp[-3].sValue)));

		/* Test d'existence de la classe en tant que classe referencee uniquement */
		kwcReferencedClass = cast(KWClass*, odReferencedUncreatedClasses->Lookup(*((yyvsp[-3].sValue))));
		assert(kwcReferencedClass == NULL or kwcClass == NULL);

		/* Erreur si la classe existe deja */
		if (kwcClass != NULL)
		{
			yyerror("Dictionary " + *((yyvsp[-3].sValue)) + " already exists");
			kwcClass = NULL;
		}
		/* On utilise la classe referencee si elle existe */
		else if (kwcReferencedClass != NULL)
		{
			/* Insertion dans le domaine */
			kwcClass = kwcReferencedClass;
			kwcdLoadDomain->InsertClass(kwcClass);

			/* Supression de la classe referencees */
			odReferencedUncreatedClasses->RemoveKey(kwcReferencedClass->GetName());
			kwcReferencedClass = NULL;
		}
		/* Sinon, on cree la classe et on l'enregistre */
		else
		{
			/* Test de nom de classe */
			if (KWClass::CheckName(*((yyvsp[-3].sValue)), NULL))
			{
				kwcClass = new KWClass;
				kwcClass->SetName(*((yyvsp[-3].sValue)));
				kwcdLoadDomain->InsertClass(kwcClass);
			}
			else
				yyerror("Incorrect dictionary name (" + *((yyvsp[-3].sValue)) + ")");
		}

		/* Initialisation si necessaire de la classe */
		if (kwcClass != NULL)
		{
			/* Class Label */
			if ((yyvsp[-6].sValue) != NULL)
				kwcClass->SetLabel(*((yyvsp[-6].sValue)));

			/* Classe racine */
			kwcClass->SetRoot((yyvsp[-5].bValue));

			/* Attribut key field */
			if ((yyvsp[-2].svValue) != NULL)
			{
				StringVector* svKeyFields;
				int i;

				// Transfert des champs de la cle */
				svKeyFields = cast(StringVector*, (yyvsp[-2].svValue));
				kwcClass->SetKeyAttributeNumber(svKeyFields->GetSize());
				for (i = 0; i < svKeyFields->GetSize(); i++)
					kwcClass->SetKeyAttributeNameAt(i, svKeyFields->GetAt(i));
			}

			/* Meta-donnees de la classe */
			if ((yyvsp[-1].kwmdMetaData) != NULL)
			{
				kwcClass->GetMetaData()->CopyFrom((yyvsp[-1].kwmdMetaData));
			}
		}

		/* Liberation des tokens */
		if ((yyvsp[-6].sValue) != NULL)
			delete (yyvsp[-6].sValue); /* Label */
		delete (yyvsp[-3].sValue);         /* Name */
		if ((yyvsp[-2].svValue) != NULL)   /* Key fields */
		{
			StringVector* svKeyFields;
			svKeyFields = cast(StringVector*, (yyvsp[-2].svValue));
			delete svKeyFields;
		}
		if ((yyvsp[-1].kwmdMetaData) != NULL)
			delete (yyvsp[-1].kwmdMetaData); /* Key value pairs */

		/* Memorisation dz la classe courante */
		kwcLoadCurrentClass = kwcClass;
		(yyval.kwcValue) = kwcClass;
	}
#line 2070 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 30:
#line 574 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = (yyvsp[-2].svValue);
	}
#line 2081 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 31:
#line 581 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 2092 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 32:
#line 588 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 2100 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 33:
#line 595 "KWCYac.yac" /* yacc.c:1652  */
	{
		StringVector* svKeyFields;

		/* Creation d'un nouveau de champ dans le tableau de champs cles */
		svKeyFields = cast(StringVector*, (yyvsp[-2].svValue));
		svKeyFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svKeyFields;
	}
#line 2114 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 34:
#line 605 "KWCYac.yac" /* yacc.c:1652  */
	{
		StringVector* svKeyFields;

		/* Creation d'un tableau de champ cle, et d'un premier champ dans la cle */
		svKeyFields = new StringVector;
		svKeyFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svKeyFields;
	}
#line 2128 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 35:
#line 618 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWMetaData* metaData;

		/* Creation si necessaire d'une ensemble de paires cles valeur */
		if ((yyvsp[-5].kwmdMetaData) == NULL)
			metaData = new KWMetaData;
		else
			metaData = cast(KWMetaData*, (yyvsp[-5].kwmdMetaData));

		/* Erreur si cle deja existante */
		if (metaData->IsKeyPresent(*(yyvsp[-3].sValue)))
			yyerror("Duplicate key in meta-data for key " + *((yyvsp[-3].sValue)));
		/* Insertion d'une paire avec valeur chaine de caracteres sinon */
		else
			metaData->SetStringValueAt(*((yyvsp[-3].sValue)), *((yyvsp[-1].sValue)));
		delete (yyvsp[-3].sValue);
		delete (yyvsp[-1].sValue);
		(yyval.kwmdMetaData) = metaData;
	}
#line 2152 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 36:
#line 638 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWMetaData* metaData;

		/* Creation si necessaire d'une ensemble de paires cles valeur */
		if ((yyvsp[-5].kwmdMetaData) == NULL)
			metaData = new KWMetaData;
		else
			metaData = cast(KWMetaData*, (yyvsp[-5].kwmdMetaData));

		/* Erreur si cle deja existante */
		if (metaData->IsKeyPresent(*(yyvsp[-3].sValue)))
			yyerror("Duplicate key in meta-data for key " + *((yyvsp[-3].sValue)));
		/* Insertion d'une paire avec valeur numerique sinon */
		else
			metaData->SetDoubleValueAt(*((yyvsp[-3].sValue)), (yyvsp[-1].cValue));
		delete (yyvsp[-3].sValue);
		(yyval.kwmdMetaData) = metaData;
	}
#line 2175 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 37:
#line 657 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWMetaData* metaData;

		/* Creation si necessaire d'une ensemble de paires cles valeur */
		if ((yyvsp[-3].kwmdMetaData) == NULL)
			metaData = new KWMetaData;
		else
			metaData = cast(KWMetaData*, (yyvsp[-3].kwmdMetaData));

		/* Erreur si cle deja existante */
		if (metaData->IsKeyPresent(*(yyvsp[-1].sValue)))
			yyerror("Duplicate key in meta-data for key " + *((yyvsp[-1].sValue)));
		/* Insertion d'une paire avec valeur numerique sinon */
		else
			metaData->SetNoValueAt(*((yyvsp[-1].sValue)));
		delete (yyvsp[-1].sValue);
		(yyval.kwmdMetaData) = metaData;
	}
#line 2198 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 38:
#line 676 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWMetaData* metaData;

		/* Creation si necessaire d'une ensemble de paires cles valeur */
		if ((yyvsp[-5].kwmdMetaData) == NULL)
			metaData = new KWMetaData;
		else
			metaData = cast(KWMetaData*, (yyvsp[-5].kwmdMetaData));

		/* Erreur car la valeur n'est pas du bon type */
		yyerror("Value (" + *((yyvsp[-1].sValue)) + ") of meta-data for key " + *((yyvsp[-3].sValue)) +
			" should be a string value between double quotes");
		delete (yyvsp[-3].sValue);
		delete (yyvsp[-1].sValue);
		(yyval.kwmdMetaData) = metaData;
	}
#line 2218 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 39:
#line 692 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwmdMetaData) = NULL; /* pas de paires cle valeurs */
	}
#line 2226 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 40:
#line 708 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWAttribute* attribute;
		KWDerivationRule* rule;

		/* Creation  et initialisation d'un attribut */
		attribute = new KWAttribute;
		attribute->SetUsed((yyvsp[-7].bValue));
		attribute->SetType((yyvsp[-6].nValue));

		/* Test de coherence entre le type et le complement de type dans le cas d'un type relation */
		if (KWType::IsRelation(attribute->GetType()))
		{
			if ((yyvsp[-5].sValue) == NULL)
				yyerrorWithLineCorrection("Variable " + *((yyvsp[-4].sValue)) + " of type " +
							      KWType::ToString((yyvsp[-6].nValue)) + ": missing " +
							      KWType::ToString((yyvsp[-6].nValue)) + " dictionary",
							  -1);
		}
		/* Test de coherence entre le type et le complement de type dans le cas d'un type Structure */
		else if (attribute->GetType() == KWType::Structure)
		{
			if ((yyvsp[-5].sValue) == NULL)
				yyerrorWithLineCorrection("Variable " + *((yyvsp[-4].sValue)) + " of type " +
							      KWType::ToString((yyvsp[-6].nValue)) + ": missing " +
							      KWType::ToString((yyvsp[-6].nValue)) + " dictionary",
							  -1);
		}
		/* Test d'absence de complement de type dans les autres cas */
		else
		{
			if ((yyvsp[-5].sValue) != NULL)
				yyerrorWithLineCorrection("Variable " + *((yyvsp[-4].sValue)) + " of type " +
							      KWType::ToString((yyvsp[-6].nValue)) + ": erroneous (" +
							      *((yyvsp[-5].sValue)) + ") type complement",
							  -1);
		}

		/* Classe referencee */
		if (KWType::IsRelation(attribute->GetType()))
		{
			KWClass* kwcReferencedClass = NULL;

			/* Test d'existence de la classe */
			if ((yyvsp[-5].sValue) != NULL)
				kwcReferencedClass = kwcdLoadDomain->LookupClass(*((yyvsp[-5].sValue)));

			/* Sinon, test d'existence de la classe en tant que classe referencee uniquement */
			if (kwcReferencedClass == NULL and (yyvsp[-5].sValue) != NULL)
				kwcReferencedClass =
				    cast(KWClass*, odReferencedUncreatedClasses->Lookup(*((yyvsp[-5].sValue))));

			/* Si la classe n'existe pas, on essaie de la creer */
			if (kwcReferencedClass == NULL and (yyvsp[-5].sValue) != NULL)
			{
				/* Test de nom de classe */
				if (KWClass::CheckName(*((yyvsp[-5].sValue)), NULL))
				{
					kwcReferencedClass = new KWClass;
					kwcReferencedClass->SetName(*((yyvsp[-5].sValue)));

					/* Memorisation dans le dictionnaire des classe referencees */
					odReferencedUncreatedClasses->SetAt(kwcReferencedClass->GetName(),
									    kwcReferencedClass);
				}
				else
					yyerrorWithLineCorrection(
					    "Incorrect referenced dictionary name (" + *((yyvsp[-5].sValue)) + ")", -1);
			}

			/* On memorise la classe referencee */
			attribute->SetClass(kwcReferencedClass);
		}
		/* Structure referencee */
		else if (attribute->GetType() == KWType::Structure)
		{
			if ((yyvsp[-5].sValue) != NULL)
				attribute->SetStructureName(*((yyvsp[-5].sValue)));
		}
		if ((yyvsp[-5].sValue) != NULL)
			delete (yyvsp[-5].sValue);

		/* Nom de l'attribut */
		attribute->SetName(*((yyvsp[-4].sValue)));
		delete (yyvsp[-4].sValue); /* liberation de la valeur de IDENTIFIER */
		rule = (yyvsp[-3].kwdrValue);
		attribute->SetDerivationRule(rule);

		/* Completion eventuelle de la regle par les infos de type de l'attribut */
		if (rule != NULL)
		{
			// Completion specifique dans le cas de la regle de gestion des references
			if (rule->GetName() == KWDerivationRule::GetReferenceRuleName() and
			    rule->GetOperandNumber() > 0)
			{
				if (KWType::IsRelation(rule->GetType()) and rule->GetObjectClassName() == "" and
				    attribute->GetClass() != NULL)
					rule->SetObjectClassName(attribute->GetClass()->GetName());
			}

			// Completion standard
			if (KWType::IsRelation(rule->GetType()) and rule->GetObjectClassName() == "" and
			    attribute->GetClass() != NULL)
				rule->SetObjectClassName(attribute->GetClass()->GetName());

			// Erreur si type renvoye par la regle different du type de l'attribut
			if (attribute->GetType() != rule->GetType())
				yyerrorWithLineCorrection(
				    "Type of variable " + attribute->GetName() + " (" +
					KWType::ToString(attribute->GetType()) +
					") inconsistent with that returned by derivation rule " +
					attribute->GetDerivationRule()->GetName() + " (" +
					KWType::ToString(attribute->GetDerivationRule()->GetType()) + ")",
				    -1);
		}

		/* Meta-donnees de l'attribut */
		if ((yyvsp[-1].kwmdMetaData) != NULL)
		{
			attribute->GetMetaData()->CopyFrom((yyvsp[-1].kwmdMetaData));
			delete (yyvsp[-1].kwmdMetaData);
		}

		/* Commentaires */
		if ((yyvsp[0].sValue) != NULL)
		{
			attribute->SetLabel(*((yyvsp[0].sValue)));
			delete (yyvsp[0].sValue);
		}

		(yyval.kwaValue) = attribute;
	}
#line 2348 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 41:
#line 831 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ne garde que la premiere ligne de chaque identification d'application */
		if ((yyvsp[-1].sValue) == NULL)
			(yyval.sValue) = (yyvsp[0].sValue);
		else
		{
			delete (yyvsp[0].sValue);
			(yyval.sValue) = (yyvsp[-1].sValue);
		}
	}
#line 2363 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 42:
#line 842 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = NULL; /* pas d'identification d'application */
	}
#line 2371 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 43:
#line 850 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* On ne garde que la premiere ligne de chaque commentaire */
		if ((yyvsp[-1].sValue) == NULL)
			(yyval.sValue) = (yyvsp[0].sValue);
		else
		{
			delete (yyvsp[0].sValue);
			(yyval.sValue) = (yyvsp[-1].sValue);
		}
	}
#line 2386 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 44:
#line 861 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = NULL; /* pas de commentaire */
	}
#line 2394 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 45:
#line 869 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.bValue) = true;
	}
#line 2402 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 46:
#line 873 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.bValue) = false; /* valeur par defaut */
	}
#line 2410 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 47:
#line 881 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.bValue) = false;
	}
#line 2418 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 48:
#line 885 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.bValue) = true; /* valeur par defaut */
	}
#line 2426 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 49:
#line 893 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Continuous;
	}
#line 2434 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 50:
#line 897 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Symbol;
	}
#line 2442 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 51:
#line 901 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Date;
	}
#line 2450 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 52:
#line 905 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Time;
	}
#line 2458 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 53:
#line 909 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Timestamp;
	}
#line 2466 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 54:
#line 913 "KWCYac.yac" /* yacc.c:1652  */
	{
		if (not GetLearningTextVariableMode())
			yyerror("Type Text is unknown");
		(yyval.nValue) = KWType::Text;
	}
#line 2476 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 55:
#line 919 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Object;
	}
#line 2484 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 56:
#line 923 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::ObjectArray;
	}
#line 2492 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 57:
#line 927 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.nValue) = KWType::Structure;
	}
#line 2500 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 58:
#line 935 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = (yyvsp[-1].sValue);
	}
#line 2508 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 59:
#line 939 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = NULL;
	}
#line 2516 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 60:
#line 946 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2524 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 61:
#line 950 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2532 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 62:
#line 954 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("Too many ')'");
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2541 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 63:
#line 959 "KWCYac.yac" /* yacc.c:1652  */
	{
		ALString sTmp;
		yyerror(sTmp + "Invalid syntax (" + *(yyvsp[-1].sValue) + ")");
		if ((yyvsp[-1].sValue) != NULL)
			delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = NULL;
	}
#line 2553 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 64:
#line 967 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = NULL;
	}
#line 2561 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 65:
#line 973 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2569 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 66:
#line 979 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* rule;
		KWDerivationRuleOperand* operand;

		/* Construction d'une regle pour accueillir les specifications */
		rule = KWDerivationRule::CloneDerivationRule(KWDerivationRule::GetReferenceRuleName());

		/* Destruction des operandes */
		rule->DeleteAllOperands();

		/* Ajout d'un premier operande: le premier champ de la cle de reference */
		operand = (yyvsp[0].kwdroValue);
		if (operand->GetType() == KWType::Unknown)
			operand->SetType(KWType::Symbol);
		rule->AddOperand(operand);

		/* On retourner la regle */
		(yyval.kwdrValue) = rule;
	}
#line 2593 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 67:
#line 999 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand;

		/* Ajout d'un autre operande: un autre champ de la cle de reference */
		operand = (yyvsp[0].kwdroValue);
		if (operand->GetType() == KWType::Unknown)
			operand->SetType(KWType::Symbol);
		rule->AddOperand(operand);

		/* On retourner la regle */
		(yyval.kwdrValue) = rule;
	}
#line 2611 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 68:
#line 1015 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* ruleBody = (yyvsp[-1].kwdrValue);
		KWDerivationRule* rule;
		KWDerivationRuleOperand* ruleBodyOperand;
		KWDerivationRuleOperand* ruleOperand;
		int i;
		boolean bRuleOk;
		ALString sTmp;

		/* Recherche de la regle de reference */
		check(ruleBody);
		rule = KWDerivationRule::CloneDerivationRule(ruleBody->GetName());

		/* Erreur si regle inexistante */
		if (rule == NULL)
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
		}
		/* Erreur si regle predefinie de Reference */
		else if (rule->GetName() == KWDerivationRule::GetReferenceRuleName())
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
			delete rule;
			rule = NULL;
		}
		/* Sinon, test du nombre d'arguments */
		else if ((rule->GetVariableOperandNumber() and
			  ruleBody->GetOperandNumber() < rule->GetOperandNumber() - 1) or
			 (not rule->GetVariableOperandNumber() and
			  ruleBody->GetOperandNumber() != rule->GetOperandNumber()))

		{
			yyerror(sTmp + "Number of operands (" + IntToString(ruleBody->GetOperandNumber()) +
				") inconsistent with that of rule " + rule->GetName() + " (" +
				IntToString(rule->GetOperandNumber()) + ")");
			delete rule;
			rule = NULL;
		}
		/* Verification et transfert des operandes */
		else
		{
			/* Dans le cas d'un nombre variable d'operandes, on commence par rajouter */
			/* eventuellement des operandes en fin de regle pour preparer l'instanciation */
			if (ruleBody->GetOperandNumber() > rule->GetOperandNumber())
			{
				assert(rule->GetVariableOperandNumber());
				while (rule->GetOperandNumber() < ruleBody->GetOperandNumber())
					rule->AddOperand(rule->GetOperandAt(rule->GetOperandNumber() - 1)->Clone());
			}

			/* Dans le cas d'un nombre variable d'operandes, on supprime eventuellement */
			/* le dernier operande, qui n'est pas obligatoire */
			if (ruleBody->GetOperandNumber() < rule->GetOperandNumber())
			{
				assert(rule->GetVariableOperandNumber());
				assert(ruleBody->GetOperandNumber() == rule->GetOperandNumber() - 1);
				rule->DeleteAllVariableOperands();
			}
			assert(ruleBody->GetOperandNumber() == rule->GetOperandNumber());

			/* Transfert des operandes */
			bRuleOk = true;
			for (i = 0; i < rule->GetOperandNumber(); i++)
			{
				/* acces aux operandes */
				ruleOperand = rule->GetOperandAt(i);
				ruleBodyOperand = ruleBody->GetOperandAt(i);

				/* Transfert d'informations de la regle de reference vers la regle a verifier */
				if (ruleBodyOperand->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
				{
					ruleBodyOperand->SetType(ruleOperand->GetType());
					if (KWType::IsRelation(ruleOperand->GetType()))
						ruleBodyOperand->SetObjectClassName(ruleOperand->GetObjectClassName());
				}
				if (ruleOperand->GetType() == KWType::Structure)
				{
					ruleBodyOperand->SetType(KWType::Structure);
					ruleBodyOperand->SetStructureName(ruleOperand->GetStructureName());
				}

				/* Test si operande candidate valide */
				if (not ruleBodyOperand->CheckDefinition())
				{
					bRuleOk = false;
					yyerror(sTmp + "Incorrect operand " + IntToString(1 + i) + " for rule " +
						rule->GetName());
				}
				/* Test de compatibilite avec la regle enregistree, sauf si regle avec operande de type
				 * indetermine */
				else if (ruleOperand->GetType() != KWType::Unknown and
					 not ruleBodyOperand->CheckFamily(ruleOperand))
				{
					bRuleOk = false;
					yyerror(sTmp + "Operand " + IntToString(1 + i) +
						" inconsistent with that of rule " + rule->GetName());
					break;
				}
				/* Transfert de l'origine de l'operande */
				else
				{
					/* Transfert du niveau de scope */
					ruleOperand->SetScopeLevel(ruleBodyOperand->GetScopeLevel());

					/* Transfert d'une valeur constante */
					ruleOperand->SetOrigin(ruleBodyOperand->GetOrigin());
					if (ruleOperand->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
					{
						ruleOperand->SetType(ruleBodyOperand->GetType());
						ruleOperand->SetStringConstant(ruleBodyOperand->GetStringConstant());
					}
					/* Transfert d'un attribut */
					else if (ruleOperand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
						ruleOperand->SetDataItemName(ruleBodyOperand->GetDataItemName());
					else
					/* Transfert d'une regle */
					{
						// Transfert de la regle */
						if (ruleOperand->GetDerivationRule() != NULL)
						{
							assert(ruleBodyOperand->GetDerivationRule() != NULL);
							delete ruleOperand->GetDerivationRule();
						}
						ruleOperand->SetDerivationRule(ruleBodyOperand->GetDerivationRule());

						/* Transfert des infos portees par la regle de derivation */
						if (ruleOperand->GetDerivationRule() != NULL)
						{
							ruleOperand->SetType(
							    ruleOperand->GetDerivationRule()->GetType());
							if (KWType::IsRelation(ruleOperand->GetType()))
								ruleOperand->SetObjectClassName(
								    ruleOperand->GetDerivationRule()
									->GetObjectClassName());
							if (ruleOperand->GetType() == KWType::Structure)
								ruleOperand->SetStructureName(
								    ruleOperand->GetDerivationRule()
									->GetStructureName());
						}

						/* Dereferencement de la regle de derivation depuis l'operande de
						 * travail */
						ruleBodyOperand->SetDerivationRule(NULL);
					}
				}
			}

			/* Test si erreur dans le transfert des operandes */
			if (not bRuleOk)
			{
				delete rule;
				rule = NULL;
			}
			/* Sinon, on tente de compresser la regle */
			else
			{
				if (rule->IsStructureRule())
				{
					KWDRStructureRule* structureRule;

					/* Acces a la regle de structure, transformation au format structure et
					 * nettoyage memoire */
					/* Cette optimisation memoire des regles structure est critique dans le cas de
					 * dictionnaires */
					/* de tres grande taille. Sinon, des millions d'operandes de regles sont
					 * potentiellement crees, */
					/* puis lors de la compilation des dictionnaire, l'essentiel de la memoire
					 * liberee laisse des trous */
					/* dans les segments de la heap, qui ne peuvent etre rendus au systeme */
					assert(rule->CheckDefinition());
					structureRule = cast(KWDRStructureRule*, rule);
					structureRule->BuildStructureFromBase(rule);
					structureRule->CleanCompiledBaseInterface();
				}
			}
		}
		delete ruleBody;

		(yyval.kwdrValue) = rule;
	}
#line 2785 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 69:
#line 1188 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2793 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 70:
#line 1192 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2801 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 71:
#line 1199 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* rule;

		/* Construction d'une regle pour accueillir les specification */
		rule = new KWDerivationRule;
		rule->SetName(*((yyvsp[-1].sValue)));
		delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = rule;
	}
#line 2815 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 72:
#line 1212 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* rule = (yyvsp[-1].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() == 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2830 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 73:
#line 1223 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() > 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2845 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 74:
#line 1237 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetDataItemName(*((yyvsp[0].sValue)));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2858 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 75:
#line 1246 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Continuous);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetContinuousConstant((yyvsp[0].cValue));
		(yyval.kwdroValue) = operand;
	}
#line 2871 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 76:
#line 1255 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Symbol);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetSymbolConstant(Symbol(*((yyvsp[0].sValue))));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2885 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 77:
#line 1265 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule((yyvsp[0].kwdrValue));
		if (operand->GetDerivationRule() != NULL)
			operand->SetType(operand->GetDerivationRule()->GetType());
		(yyval.kwdroValue) = operand;
	}
#line 2899 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 78:
#line 1275 "KWCYac.yac" /* yacc.c:1652  */
	{
		KWDerivationRuleOperand* operand;
		operand = (yyvsp[0].kwdroValue);
		operand->SetScopeLevel(operand->GetScopeLevel() + 1);
		(yyval.kwdroValue) = operand;
	}
#line 2910 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 79:
#line 1286 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* Concatenation des deux chaines */
		(yyval.sValue) = new ALString(*(yyvsp[-2].sValue) + *(yyvsp[0].sValue));

		/* Destruction des ancienne chaines */
		delete (yyvsp[-2].sValue);
		delete (yyvsp[0].sValue);
	}
#line 2923 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 80:
#line 1295 "KWCYac.yac" /* yacc.c:1652  */
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 2931 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 82:
#line 1303 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("There is one superfluous ';'");
	}
#line 2939 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 83:
#line 1307 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("Too many ';'");
	}
#line 2947 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 84:
#line 1311 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("Missing ';'");
	}
#line 2955 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 86:
#line 1319 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("There is one superfluous '('");
	}
#line 2963 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 87:
#line 1323 "KWCYac.yac" /* yacc.c:1652  */
	{
		yyerror("Too many '('");
	}
#line 2971 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 88:
#line 1327 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing '('");
	}
#line 2984 "KWCYac.cpp" /* yacc.c:1652  */
	break;

	case 90:
#line 1341 "KWCYac.yac" /* yacc.c:1652  */
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing ')'");
	}
#line 2997 "KWCYac.cpp" /* yacc.c:1652  */
	break;

#line 3001 "KWCYac.cpp" /* yacc.c:1652  */
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
	YY_SYMBOL_PRINT("-> $$ =", yyr1[yyn], &yyval, &yyloc);

	YYPOPSTACK(yylen);
	yylen = 0;
	YY_STACK_PRINT(yyss, yyssp);

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
	yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE(yychar);

	/* If not already recovering from an error, report this error.  */
	if (!yyerrstatus)
	{
		++yynerrs;
#if !YYERROR_VERBOSE
		yyerror(YY_("syntax error"));
#else
#define YYSYNTAX_ERROR yysyntax_error(&yymsg_alloc, &yymsg, yyssp, yytoken)
		{
			char const* yymsgp = YY_("syntax error");
			int yysyntax_error_status;
			yysyntax_error_status = YYSYNTAX_ERROR;
			if (yysyntax_error_status == 0)
				yymsgp = yymsg;
			else if (yysyntax_error_status == 1)
			{
				if (yymsg != yymsgbuf)
					YYSTACK_FREE(yymsg);
				yymsg = (char*)YYSTACK_ALLOC(yymsg_alloc);
				if (!yymsg)
				{
					yymsg = yymsgbuf;
					yymsg_alloc = sizeof yymsgbuf;
					yysyntax_error_status = 2;
				}
				else
				{
					yysyntax_error_status = YYSYNTAX_ERROR;
					yymsgp = yymsg;
				}
			}
			yyerror(yymsgp);
			if (yysyntax_error_status == 2)
				goto yyexhaustedlab;
		}
#undef YYSYNTAX_ERROR
#endif
	}

	if (yyerrstatus == 3)
	{
		/* If just tried and failed to reuse lookahead token after an
		   error, discard it.  */

		if (yychar <= YYEOF)
		{
			/* Return failure if at end of input.  */
			if (yychar == YYEOF)
				YYABORT;
		}
		else
		{
			yydestruct("Error: discarding", yytoken, &yylval);
			yychar = YYEMPTY;
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

	for (;;)
	{
		yyn = yypact[yystate];
		if (!yypact_value_is_default(yyn))
		{
			yyn += YYTERROR;
			if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
			{
				yyn = yytable[yyn];
				if (0 < yyn)
					break;
			}
		}

		/* Pop the current state because it cannot handle the error token.  */
		if (yyssp == yyss)
			YYABORT;

		yydestruct("Error: popping", yystos[yystate], yyvsp);
		YYPOPSTACK(1);
		yystate = *yyssp;
		YY_STACK_PRINT(yyss, yyssp);
	}

	YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	*++yyvsp = yylval;
	YY_IGNORE_MAYBE_UNINITIALIZED_END

	/* Shift the error token.  */
	YY_SYMBOL_PRINT("Shifting", yystos[yyn], yyvsp, yylsp);

	yystate = yyn;
	goto yynewstate;

/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
	yyresult = 0;
	goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
	yyresult = 1;
	goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
	yyerror(YY_("memory exhausted"));
	yyresult = 2;
	/* Fall through.  */
#endif

/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
	if (yychar != YYEMPTY)
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
		yydestruct("Cleanup: popping", yystos[*yyssp], yyvsp);
		YYPOPSTACK(1);
	}
#ifndef yyoverflow
	if (yyss != yyssa)
		YYSTACK_FREE(yyss);
#endif
#if YYERROR_VERBOSE
	if (yymsg != yymsgbuf)
		YYSTACK_FREE(yymsg);
#endif
	return yyresult;
}
#line 1352 "KWCYac.yac" /* yacc.c:1918  */

#include "KWCLex.inc"

/* default yywrap that tells yylex to return 0 */
int yywrap()
{
	return 1;
}

/* default yyerror for YACC and LEX */
void yyerror(char const* fmt)
{
	yyerrorWithLineCorrection(fmt, 0);
}

/* Variante avec une correction du numero de ligne */
void yyerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber)
{
	char sErrorLine[20];
	ALString sLabel;
	int nLineNumber;

	nFileParsingErrorNumber++;
	nLineNumber = yylineno + nDeltaLineNumber;
	if (nLineNumber <= 0)
		nLineNumber = 1;
	sprintf(sErrorLine, "Line %d", nLineNumber);
	sLabel = fmt;
	Global::AddError("Read dictionary file", sErrorLine, sLabel);
}

int yyparse();

/* Implementation de la methode de lecture de fichier de KWClassDomain */
boolean KWClassDomain::ReadFile(const ALString& sFileName)
{
	boolean bOk = true;
	FILE* fFile;
	ObjectDictionary odInitialClasses;
	ObjectArray oaNewClasses;
	ObjectArray oaReferencedUncreatedClasses;
	int i;
	KWClass* kwcClass;
	ALString sLocalFileName;

	/* Affichage de stats memoire si log memoire actif */
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile Begin");

	/* Initialisation du domaine de classe a utiliser pour le Load */
	assert(kwcdLoadDomain == NULL);
	kwcdLoadDomain = this;

	/* Initialisation de la classe courante a utiliser pour le Load */
	assert(kwcLoadCurrentClass == NULL);
	kwcLoadCurrentClass = NULL;

	/* Creation du dictionnaire des classes referencees non crees */
	assert(odReferencedUncreatedClasses == NULL);
	odReferencedUncreatedClasses = new ObjectDictionary;

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
		// Copie depuis HDFS si necessaire
		bOk = PLRemoteFileService::BuildInputWorkingFile(sFileName, sLocalFileName);
		if (bOk)
			bOk = FileService::OpenInputBinaryFile(sLocalFileName, fFile);
	}

	/* On continue si fichier ouvert correctement */
	if (bOk)
	{
		assert(fFile != NULL);

		/* Memorisation de toutes les classes initiales */
		kwcdLoadDomain->ExportClassDictionary(&odInitialClasses);

		/* Activation du nombre max d'erreurs a afficher */
		nFileParsingErrorNumber = 0;
		Global::ActivateErrorFlowControl();

		/* Positionnement du fichier a parser par la variable yyin de LEX */
		yylineno = 1;
		yyrestart(fFile);

		/* Parsing */
		yyparse();

		/* Cleaning lexer */
		yylex_destroy();

		/* Fermeture du fichier */
		FileService::CloseInputBinaryFile(sLocalFileName, fFile);

		/* Si HDFS on supprime la copie locale */
		PLRemoteFileService::CleanInputWorkingFile(sFileName, sLocalFileName);

		/* Completion des informations de type au niveau du domaine */
		if (nFileParsingErrorNumber == 0)
			kwcdLoadDomain->CompleteTypeInfo();

		/* Lecture des informations sur les attributs utilises mais non charges en memoire */
		if (nFileParsingErrorNumber == 0)
		{
			for (i = 0; i < kwcdLoadDomain->GetClassNumber(); i++)
			{
				kwcClass = kwcdLoadDomain->GetClassAt(i);
				kwcClass->ReadNotLoadedMetaData();
			}
		}

		/* Messages d'erreur pour les classes referencees non crees */
		if (nFileParsingErrorNumber > 0 or odReferencedUncreatedClasses->GetCount() > 0)
		{
			odReferencedUncreatedClasses->ExportObjectArray(&oaReferencedUncreatedClasses);
			for (i = 0; i < oaReferencedUncreatedClasses.GetSize(); i++)
			{
				kwcClass = cast(KWClass*, oaReferencedUncreatedClasses.GetAt(i));
				AddError("Error detected during parsing " + sFileName + ": dictionary " +
					 kwcClass->GetName() + " used, but not declared");
			}
		}

		/* Desactivation du nombre max d'erreurs a afficher */
		Global::DesactivateErrorFlowControl();

		/* Destruction des classes crees si au moins une erreur de parsing detectee */
		/* ou au moins une classe referencee non cree                               */
		if (nFileParsingErrorNumber > 0 or odReferencedUncreatedClasses->GetCount() > 0)
		{
			AddError("Error detected during parsing " + sFileName + ": read operation cancelled");
			bOk = false;

			/* Recherche des nouvelles classes crees */
			for (i = 0; i < kwcdLoadDomain->GetClassNumber(); i++)
			{
				kwcClass = kwcdLoadDomain->GetClassAt(i);
				if (odInitialClasses.Lookup(kwcClass->GetName()) == NULL)
					oaNewClasses.Add(kwcClass);
			}

			/* Destruction des classes nouvellement crees */
			for (i = 0; i < oaNewClasses.GetSize(); i++)
			{
				kwcClass = cast(KWClass*, oaNewClasses.GetAt(i));
				kwcdLoadDomain->DeleteClass(kwcClass->GetName());
			}

			/* Destruction des classes referencees non crees */
			odReferencedUncreatedClasses->DeleteAll();
		}
		nFileParsingErrorNumber = 0;
	}

	/* Nettoyage */
	kwcdLoadDomain = NULL;
	kwcLoadCurrentClass = NULL;
	delete odReferencedUncreatedClasses;
	odReferencedUncreatedClasses = NULL;

	/* Affichage de stats memoire si log memoire actif */
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile End");

	return bOk;
}
