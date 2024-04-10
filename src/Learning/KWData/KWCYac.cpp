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

/* First part of user prologue.  */
#line 1 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

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

#line 127 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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

#include "KWCYac.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
	YYSYMBOL_YYEMPTY = -2,
	YYSYMBOL_YYEOF = 0,                        /* "end of file"  */
	YYSYMBOL_YYerror = 1,                      /* error  */
	YYSYMBOL_YYUNDEF = 2,                      /* "invalid token"  */
	YYSYMBOL_BASICIDENTIFIER = 3,              /* BASICIDENTIFIER  */
	YYSYMBOL_EXTENDEDIDENTIFIER = 4,           /* EXTENDEDIDENTIFIER  */
	YYSYMBOL_CONTINUOUSLITTERAL = 5,           /* CONTINUOUSLITTERAL  */
	YYSYMBOL_STRINGLITTERAL = 6,               /* STRINGLITTERAL  */
	YYSYMBOL_LABEL = 7,                        /* LABEL  */
	YYSYMBOL_APPLICATIONID = 8,                /* APPLICATIONID  */
	YYSYMBOL_CLASS = 9,                        /* CLASS  */
	YYSYMBOL_CONTINUOUSTYPE = 10,              /* CONTINUOUSTYPE  */
	YYSYMBOL_SYMBOLTYPE = 11,                  /* SYMBOLTYPE  */
	YYSYMBOL_OBJECTTYPE = 12,                  /* OBJECTTYPE  */
	YYSYMBOL_OBJECTARRAYTYPE = 13,             /* OBJECTARRAYTYPE  */
	YYSYMBOL_ROOT = 14,                        /* ROOT  */
	YYSYMBOL_UNUSED = 15,                      /* UNUSED  */
	YYSYMBOL_DATETYPE = 16,                    /* DATETYPE  */
	YYSYMBOL_TIMETYPE = 17,                    /* TIMETYPE  */
	YYSYMBOL_TIMESTAMPTYPE = 18,               /* TIMESTAMPTYPE  */
	YYSYMBOL_TIMESTAMPTZTYPE = 19,             /* TIMESTAMPTZTYPE  */
	YYSYMBOL_TEXTTYPE = 20,                    /* TEXTTYPE  */
	YYSYMBOL_TEXTLISTTYPE = 21,                /* TEXTLISTTYPE  */
	YYSYMBOL_STRUCTURETYPE = 22,               /* STRUCTURETYPE  */
	YYSYMBOL_23_ = 23,                         /* '}'  */
	YYSYMBOL_24_ = 24,                         /* '{'  */
	YYSYMBOL_25_ = 25,                         /* '('  */
	YYSYMBOL_26_ = 26,                         /* ')'  */
	YYSYMBOL_27_ = 27,                         /* ','  */
	YYSYMBOL_28_ = 28,                         /* '<'  */
	YYSYMBOL_29_ = 29,                         /* '='  */
	YYSYMBOL_30_ = 30,                         /* '>'  */
	YYSYMBOL_31_ = 31,                         /* ']'  */
	YYSYMBOL_32_ = 32,                         /* '['  */
	YYSYMBOL_33_ = 33,                         /* '.'  */
	YYSYMBOL_34_ = 34,                         /* '+'  */
	YYSYMBOL_35_ = 35,                         /* ';'  */
	YYSYMBOL_YYACCEPT = 36,                    /* $accept  */
	YYSYMBOL_IDENTIFIER = 37,                  /* IDENTIFIER  */
	YYSYMBOL_SIMPLEIDENTIFIER = 38,            /* SIMPLEIDENTIFIER  */
	YYSYMBOL_kwclassFile = 39,                 /* kwclassFile  */
	YYSYMBOL_kwclasses = 40,                   /* kwclasses  */
	YYSYMBOL_kwclass = 41,                     /* kwclass  */
	YYSYMBOL_kwclassBegin = 42,                /* kwclassBegin  */
	YYSYMBOL_oaAttributeArrayDeclaration = 43, /* oaAttributeArrayDeclaration  */
	YYSYMBOL_kwclassHeader = 44,               /* kwclassHeader  */
	YYSYMBOL_keyFields = 45,                   /* keyFields  */
	YYSYMBOL_keyFieldList = 46,                /* keyFieldList  */
	YYSYMBOL_metaData = 47,                    /* metaData  */
	YYSYMBOL_kwattributeDeclaration = 48,      /* kwattributeDeclaration  */
	YYSYMBOL_applicationids = 49,              /* applicationids  */
	YYSYMBOL_comments = 50,                    /* comments  */
	YYSYMBOL_rootDeclaration = 51,             /* rootDeclaration  */
	YYSYMBOL_usedDeclaration = 52,             /* usedDeclaration  */
	YYSYMBOL_typeDeclaration = 53,             /* typeDeclaration  */
	YYSYMBOL_refIdentifier = 54,               /* refIdentifier  */
	YYSYMBOL_usedDerivationRule = 55,          /* usedDerivationRule  */
	YYSYMBOL_referenceRule = 56,               /* referenceRule  */
	YYSYMBOL_referenceRuleBody = 57,           /* referenceRuleBody  */
	YYSYMBOL_derivationRule = 58,              /* derivationRule  */
	YYSYMBOL_derivationRuleBody = 59,          /* derivationRuleBody  */
	YYSYMBOL_derivationRuleHeader = 60,        /* derivationRuleHeader  */
	YYSYMBOL_derivationRuleBegin = 61,         /* derivationRuleBegin  */
	YYSYMBOL_derivationRuleOperand = 62,       /* derivationRuleOperand  */
	YYSYMBOL_bigstring = 63,                   /* bigstring  */
	YYSYMBOL_semicolon = 64,                   /* semicolon  */
	YYSYMBOL_openparenthesis = 65,             /* openparenthesis  */
	YYSYMBOL_closeparenthesis = 66             /* closeparenthesis  */
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
typedef yytype_uint8 yy_state_t;

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

#if (!defined yyoverflow && (!defined __cplusplus || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

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
#define YYFINAL 3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 183

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 31
/* YYNRULES -- Number of rules.  */
#define YYNRULES 94
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 137

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK 277

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                                                               \
	(0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] = {
    0,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 25, 26, 2, 34, 27, 2, 33, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  35, 28, 29,
    30, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  32, 2,
    31, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  24,
    2,  23, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 1, 2,  3,  4, 5,  6,  7, 8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] = {
    0,    134,  134,  138,  145,  149,  153,  157,  161,  165,  169,  173,  177,  181,  185,  189,  193,  197,  201,
    207,  220,  221,  224,  227,  237,  245,  286,  365,  384,  395,  438,  485,  577,  584,  592,  598,  608,  621,
    641,  663,  682,  699,  707,  842,  854,  861,  873,  880,  885,  892,  897,  904,  908,  912,  916,  920,  924,
    928,  932,  936,  940,  944,  952,  957,  963,  967,  971,  976,  985,  990,  996,  1016, 1032, 1210, 1214, 1221,
    1234, 1245, 1259, 1268, 1277, 1287, 1297, 1308, 1317, 1324, 1325, 1329, 1334, 1340, 1341, 1345, 1350, 1362, 1364};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST(yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char* yysymbol_name(yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char* const yytname[] = {"\"end of file\"",
				      "error",
				      "\"invalid token\"",
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
				      "TIMESTAMPTZTYPE",
				      "TEXTTYPE",
				      "TEXTLISTTYPE",
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

static const char* yysymbol_name(yysymbol_kind_t yysymbol)
{
	return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-65)

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-93)

#define yytable_value_is_error(Yyn) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
    -65, 4,   39,  -65, -65, 87,  -65, -65, 62,  -65, 86,  -65, -65, 23,  -65, -65, 161, 64,  -65, -65, 80,  55,  -65,
    30,  -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, 72,  128, 63,  128, 42,  -65, 128, 128, -65, -65, -65,
    -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, -65, 6,   -65, -65, 74,  128, -65, 76,  74,  128, -65,
    64,  128, 128, 5,   23,  -65, 3,   74,  -65, 23,  -65, 34,  28,  78,  82,  83,  84,  5,   88,  -65, -65, 5,   37,
    -65, -65, 99,  -65, 5,   -65, 23,  -65, -65, 128, -65, 148, -65, 109, -65, -65, -65, -65, -65, 5,   -65, 129, 124,
    -65, -65, 124, 64,  -65, 38,  111, -65, -65, 64,  124, 64,  108, -65, -65, 64,  123, 125, 126, -65, -65, -65};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] = {
    44, 0,  22, 1,  43, 0,  21, 20, 0,  46, 19, 28, 49, 88, 46, 25, 0,  24, 45, 47, 0,  85, 23, 50, 51, 52, 59, 60,
    53, 54, 55, 56, 57, 58, 61, 63, 0,  86, 0,  50, 30, 0,  0,  4,  3,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 34, 2,  87, 68, 0,  29, 0,  68, 0,  41, 33, 0,  0,  0,  88, 65, 0,  68, 62, 88, 36, 0,  0,  0,  92,
    64, 94, 74, 73, 79, 84, 0,  78, 81, 70, 80, 41, 0,  69, 88, 41, 46, 0,  31, 0,  67, 89, 75, 66, 93, 72, 76, 0,
    82, 0,  46, 71, 41, 46, 32, 35, 0,  90, 77, 83, 27, 46, 42, 0,  39, 91, 26, 0,  0,  0,  38, 37, 40};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-65, -35, 51,  -65, -65, -65, -65, -65, -65, -65, -65, -40, 25,  -65, -9, -65,
				      -65, -65, -65, -64, -65, -65, 104, -65, -65, -65, -57, -65, -29, -65, -65};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] = {0,  91, 60, 1,  5,  7,  8,  39, 9,  68, 80, 81, 15, 2,   10, 20,
					16, 35, 42, 73, 74, 75, 92, 85, 86, 87, 93, 94, 22, 106, 109};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = {
    17,  59,  78,  62,  3,   23,  65,  66,  43,  44,  88,  89,  98,  -46, 45,  46,  47,  48,  49,  50,  51,  52,  53,
    54,  55,  56,  57,  58,  76,  110, 96,  67,  79,  112, 97,  82,  83,  18,  90,  115, -92, -92, -92, -92, 95,  12,
    -92, 4,   40,  99,  69,  -92, 102, 38,  122, 114, 103, 12,  21,  117, 100, 101, 105, 11,  64,  63,  119, 127, 128,
    116, -92, 18,  -50, -50, -50, -50, 125, 12,  -50, -50, -50, -50, -50, -50, -50, 13,  14,  -46, 6,   36,  37,  118,
    133, 18,  -46, -48, -46, 41,  61,  70,  19,  -46, 77,  71,  104, 124, 72,  105, 126, 107, 108, 43,  44,  131, 132,
    111, 130, 45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  43,  44,  113, 121, 123, 129, 45,
    46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  43,  103, 134, 120, 135, 136, 45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  24,  25,  26,  27,  84,  0,   28,  29,  30,  31,  32,  33,  34};

static const yytype_int8 yycheck[] = {
    9,   36, 66, 38, 0,  14, 41, 42, 3,   4,  5,   6,   76,  7,  9,  10, 11, 12, 13, 14,  15, 16, 17,  18, 19,  20, 21,
    22,  63, 86, 27, 25, 67, 90, 31, 70,  71, 7,   33,  96,  3,  4,  5,  6,  73, 15, 9,   8,  23, 78,  59, 14,  24, 23,
    111, 95, 28, 15, 35, 99, 26, 27, 25,  1,  39,  23,  101, 29, 30, 98, 33, 7,  10, 11,  12, 13, 116, 15, 16,  17, 18,
    19,  20, 21, 22, 23, 24, 0,  1,  9,   35, 100, 127, 7,   7,  9,  9,  25, 35, 25, 14,  14, 26, 29,  26, 114, 32, 25,
    117, 26, 26, 3,  4,  5,  6,  27, 125, 9,  10,  11,  12,  13, 14, 15, 16, 17, 18, 19,  20, 21, 22,  3,  4,   34, 25,
    6,   25, 9,  10, 11, 12, 13, 14, 15,  16, 17,  18,  19,  20, 21, 22, 3,  28, 30, 103, 30, 30, 9,   10, 11,  12, 13,
    14,  15, 16, 17, 18, 19, 20, 21, 22,  10, 11,  12,  13,  71, -1, 16, 17, 18, 19, 20,  21, 22};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] = {
    0,  39, 49, 0,  8,  40, 1,  41, 42, 44, 50, 1,  15, 23, 24, 48, 52, 50, 7,  14, 51, 35, 64, 50, 10, 11, 12, 13,
    16, 17, 18, 19, 20, 21, 22, 53, 9,  35, 23, 43, 48, 25, 54, 3,  4,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 37, 38, 35, 37, 23, 48, 37, 37, 25, 45, 50, 25, 29, 32, 55, 56, 57, 37, 26, 55, 37, 46, 47, 37, 37,
    58, 59, 60, 61, 5,  6,  33, 37, 58, 62, 63, 64, 27, 31, 55, 64, 26, 27, 24, 28, 26, 25, 65, 26, 26, 66, 62, 27,
    62, 34, 47, 62, 64, 47, 50, 37, 38, 25, 62, 6,  50, 47, 50, 29, 30, 25, 50, 5,  6,  37, 30, 30, 30};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] = {0,  36, 37, 37, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38,
				   39, 40, 40, 40, 41, 42, 42, 42, 42, 42, 43, 43, 44, 45, 45, 45, 46, 46, 47,
				   47, 47, 47, 47, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53, 53, 53, 53, 53, 53,
				   53, 53, 53, 53, 53, 54, 54, 55, 55, 55, 55, 55, 56, 57, 57, 58, 59, 59, 60,
				   61, 61, 62, 62, 62, 62, 62, 63, 63, 64, 64, 64, 64, 65, 65, 65, 65, 66, 66};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = {0, 2, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 0, 3,
				   2, 2, 10, 9, 2, 2, 1, 7, 4, 1, 0, 3, 1, 6, 6, 4, 6, 0, 8, 2, 0, 2, 0, 1,
				   0, 1, 0,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 2, 1, 3, 3, 0, 2, 2, 3,
				   2, 1, 1,  2, 2, 3, 1, 1, 1, 1, 2, 3, 1, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0};

enum
{
	YYENOMEM = -2
};

#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab
#define YYNOMEM goto yyexhaustedlab

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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

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
#else /* !YYDEBUG */
#define YYDPRINTF(Args) ((void)0)
#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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
	case YYSYMBOL_BASICIDENTIFIER: /* BASICIDENTIFIER  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1030 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_EXTENDEDIDENTIFIER: /* EXTENDEDIDENTIFIER  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1036 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_STRINGLITTERAL: /* STRINGLITTERAL  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1042 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_LABEL: /* LABEL  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1048 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_APPLICATIONID: /* APPLICATIONID  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1054 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_IDENTIFIER: /* IDENTIFIER  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1060 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_SIMPLEIDENTIFIER: /* SIMPLEIDENTIFIER  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1066 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_oaAttributeArrayDeclaration: /* oaAttributeArrayDeclaration  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).oaAttributes) != NULL)
			delete ((*yyvaluep).oaAttributes);
		((*yyvaluep).oaAttributes) = NULL;
	}
#line 1072 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_keyFields: /* keyFields  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1078 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_keyFieldList: /* keyFieldList  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1084 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_metaData: /* metaData  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwmdMetaData) != NULL)
			delete ((*yyvaluep).kwmdMetaData);
		((*yyvaluep).kwmdMetaData) = NULL;
	}
#line 1090 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_kwattributeDeclaration: /* kwattributeDeclaration  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwaValue) != NULL)
			delete ((*yyvaluep).kwaValue);
		((*yyvaluep).kwaValue) = NULL;
	}
#line 1096 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_applicationids: /* applicationids  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1102 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_comments: /* comments  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1108 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_refIdentifier: /* refIdentifier  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1114 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_usedDerivationRule: /* usedDerivationRule  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1120 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRule: /* referenceRule  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1126 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRuleBody: /* referenceRuleBody  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1132 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRule: /* derivationRule  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1138 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBody: /* derivationRuleBody  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1144 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleHeader: /* derivationRuleHeader  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1150 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBegin: /* derivationRuleBegin  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1156 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleOperand: /* derivationRuleOperand  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdroValue) != NULL)
			delete ((*yyvaluep).kwdroValue);
		((*yyvaluep).kwdroValue) = NULL;
	}
#line 1162 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_bigstring: /* bigstring  */
#line 125 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1168 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
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
	if (yychar == YYEMPTY)
	{
		YYDPRINTF((stderr, "Reading a token\n"));
		yychar = yylex();
	}

	if (yychar <= YYEOF)
	{
		yychar = YYEOF;
		yytoken = YYSYMBOL_YYEOF;
		YYDPRINTF((stderr, "Now at end of input.\n"));
	}
	else if (yychar == YYerror)
	{
		/* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
		yychar = YYUNDEF;
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
	yychar = YYEMPTY;
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
	case 2: /* IDENTIFIER: SIMPLEIDENTIFIER  */
#line 135 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1440 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 3: /* IDENTIFIER: EXTENDEDIDENTIFIER  */
#line 139 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1448 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 4: /* SIMPLEIDENTIFIER: BASICIDENTIFIER  */
#line 146 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1456 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 5: /* SIMPLEIDENTIFIER: CLASS  */
#line 150 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Dictionary");
	}
#line 1464 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 6: /* SIMPLEIDENTIFIER: CONTINUOUSTYPE  */
#line 154 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Numerical");
	}
#line 1472 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 7: /* SIMPLEIDENTIFIER: SYMBOLTYPE  */
#line 158 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Categorical");
	}
#line 1480 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 8: /* SIMPLEIDENTIFIER: OBJECTTYPE  */
#line 162 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Entity");
	}
#line 1488 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 9: /* SIMPLEIDENTIFIER: OBJECTARRAYTYPE  */
#line 166 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Table");
	}
#line 1496 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 10: /* SIMPLEIDENTIFIER: ROOT  */
#line 170 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Root");
	}
#line 1504 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 11: /* SIMPLEIDENTIFIER: UNUSED  */
#line 174 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Unused");
	}
#line 1512 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 12: /* SIMPLEIDENTIFIER: DATETYPE  */
#line 178 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Date");
	}
#line 1520 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 13: /* SIMPLEIDENTIFIER: TIMETYPE  */
#line 182 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Time");
	}
#line 1528 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 14: /* SIMPLEIDENTIFIER: TIMESTAMPTYPE  */
#line 186 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Timestamp");
	}
#line 1536 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 15: /* SIMPLEIDENTIFIER: TIMESTAMPTZTYPE  */
#line 190 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TimestampTZ");
	}
#line 1544 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 16: /* SIMPLEIDENTIFIER: TEXTTYPE  */
#line 194 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Text");
	}
#line 1552 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 17: /* SIMPLEIDENTIFIER: TEXTLISTTYPE  */
#line 198 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TextList");
	}
#line 1560 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 18: /* SIMPLEIDENTIFIER: STRUCTURETYPE  */
#line 202 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Structure");
	}
#line 1568 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 19: /* kwclassFile: applicationids kwclasses comments  */
#line 208 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore l'identification d'application */
		if ((yyvsp[-2].sValue) != NULL)
			delete (yyvsp[-2].sValue);

		/* On ignore les commentaires en fin de fichier */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
	}
#line 1582 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 21: /* kwclasses: kwclasses error  */
#line 222 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Error outside the definition of a dictionary");
		YYABORT;
	}
#line 1589 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 23: /* kwclass: kwclassBegin '}' semicolon  */
#line 228 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* La completion des informations de type (CompleteTypeInfo) est centralisee */
		/* au niveau du domaine en fin de parsing */

		/* Reinitialisation de la classe courante */
		kwcLoadCurrentClass = NULL;
	}
#line 1601 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 24: /* kwclassBegin: kwclassHeader comments  */
#line 238 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les premiers comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));
		(yyval.kwcValue) = (yyvsp[-1].kwcValue);
	}
#line 1613 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 25: /* kwclassBegin: kwclassBegin kwattributeDeclaration  */
#line 246 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-1].kwcValue);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));

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
			kwcClass->InsertAttribute(attribute);
		}

		(yyval.kwcValue) = kwcClass;
	}
#line 1658 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 26: /* kwclassBegin: kwclassBegin '{' comments oaAttributeArrayDeclaration '}' IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 287 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 1741 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 27: /* kwclassBegin: kwclassBegin '{' comments '}' IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 366 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 1764 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 28: /* kwclassBegin: kwclassBegin error  */
#line 385 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: cette regle qui permet une gestion des erreurs amelioree */
		/* genere un conflit reduce/reduce */
		kwcLoadCurrentClass = NULL;
		YYABORT;
	}
#line 1776 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 29: /* oaAttributeArrayDeclaration: oaAttributeArrayDeclaration kwattributeDeclaration  */
#line 396 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaAttributes = (yyvsp[-1].oaAttributes);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
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
			kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1823 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 30: /* oaAttributeArrayDeclaration: kwattributeDeclaration  */
#line 439 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaAttributes;
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;

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
			kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1872 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 31: /* kwclassHeader: comments rootDeclaration CLASS IDENTIFIER keyFields metaData '{'  */
#line 486 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 1965 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 32: /* keyFields: '(' keyFieldList ')' comments  */
#line 578 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = (yyvsp[-2].svValue);
	}
#line 1976 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 33: /* keyFields: comments  */
#line 585 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 1987 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 34: /* keyFields: %empty  */
#line 592 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 1995 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 35: /* keyFieldList: keyFieldList ',' IDENTIFIER  */
#line 599 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svKeyFields;

		/* Creation d'un nouveau de champ dans le tableau de champs cles */
		svKeyFields = cast(StringVector*, (yyvsp[-2].svValue));
		svKeyFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svKeyFields;
	}
#line 2009 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 36: /* keyFieldList: IDENTIFIER  */
#line 609 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svKeyFields;

		/* Creation d'un tableau de champ cle, et d'un premier champ dans la cle */
		svKeyFields = new StringVector;
		svKeyFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svKeyFields;
	}
#line 2023 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 37: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' STRINGLITTERAL '>'  */
#line 622 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2047 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 38: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' CONTINUOUSLITTERAL '>'  */
#line 642 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
		/* Erreur si valeur Missing */
		else if ((yyvsp[-1].cValue) == KWContinuous::GetMissingValue())
			yyerror("Missing value not allowed in meta-data for key " + *((yyvsp[-3].sValue)));
		/* Insertion d'une paire avec valeur numerique sinon */
		else
			metaData->SetDoubleValueAt(*((yyvsp[-3].sValue)), (yyvsp[-1].cValue));
		delete (yyvsp[-3].sValue);
		(yyval.kwmdMetaData) = metaData;
	}
#line 2073 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 39: /* metaData: metaData '<' SIMPLEIDENTIFIER '>'  */
#line 664 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2096 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 40: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' IDENTIFIER '>'  */
#line 683 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2116 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 41: /* metaData: %empty  */
#line 699 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwmdMetaData) = NULL; /* pas de paires cle valeurs */
	}
#line 2124 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 42: /* kwattributeDeclaration: usedDeclaration typeDeclaration refIdentifier IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 715 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
			// Erreur dans le cas du type Structure
			else if (attribute->GetType() == KWType::Structure and
				 attribute->GetStructureName() != rule->GetStructureName())
				yyerrorWithLineCorrection(
				    "Type of variable " + attribute->GetName() + " (" +
					KWType::ToString(attribute->GetType()) + "(" + attribute->GetStructureName() +
					")) inconsistent with that returned by derivation rule " +
					attribute->GetDerivationRule()->GetName() + " (" +
					KWType::ToString(attribute->GetDerivationRule()->GetType()) + "(" +
					rule->GetStructureName() + "))",
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
#line 2251 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 43: /* applicationids: applicationids APPLICATIONID  */
#line 843 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2266 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 44: /* applicationids: %empty  */
#line 854 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; /* pas d'identification d'application */
	}
#line 2274 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 45: /* comments: comments LABEL  */
#line 862 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2289 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 46: /* comments: %empty  */
#line 873 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; /* pas de commentaire */
	}
#line 2297 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 47: /* rootDeclaration: ROOT  */
#line 881 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true;
	}
#line 2305 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 48: /* rootDeclaration: %empty  */
#line 885 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false; /* valeur par defaut */
	}
#line 2313 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 49: /* usedDeclaration: UNUSED  */
#line 893 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false;
	}
#line 2321 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 50: /* usedDeclaration: %empty  */
#line 897 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true; /* valeur par defaut */
	}
#line 2329 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 51: /* typeDeclaration: CONTINUOUSTYPE  */
#line 905 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Continuous;
	}
#line 2337 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 52: /* typeDeclaration: SYMBOLTYPE  */
#line 909 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Symbol;
	}
#line 2345 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 53: /* typeDeclaration: DATETYPE  */
#line 913 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Date;
	}
#line 2353 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 54: /* typeDeclaration: TIMETYPE  */
#line 917 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Time;
	}
#line 2361 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 55: /* typeDeclaration: TIMESTAMPTYPE  */
#line 921 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Timestamp;
	}
#line 2369 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 56: /* typeDeclaration: TIMESTAMPTZTYPE  */
#line 925 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TimestampTZ;
	}
#line 2377 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 57: /* typeDeclaration: TEXTTYPE  */
#line 929 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Text;
	}
#line 2385 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 58: /* typeDeclaration: TEXTLISTTYPE  */
#line 933 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TextList;
	}
#line 2393 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 59: /* typeDeclaration: OBJECTTYPE  */
#line 937 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Object;
	}
#line 2401 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 60: /* typeDeclaration: OBJECTARRAYTYPE  */
#line 941 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::ObjectArray;
	}
#line 2409 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 61: /* typeDeclaration: STRUCTURETYPE  */
#line 945 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Structure;
	}
#line 2417 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 62: /* refIdentifier: '(' IDENTIFIER ')'  */
#line 953 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[-1].sValue);
	}
#line 2425 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 63: /* refIdentifier: %empty  */
#line 957 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL;
	}
#line 2433 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 64: /* usedDerivationRule: '=' derivationRule  */
#line 964 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2441 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 65: /* usedDerivationRule: referenceRule  */
#line 968 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2449 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 66: /* usedDerivationRule: '=' derivationRule ')'  */
#line 972 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ')'");
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2458 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 67: /* usedDerivationRule: '(' IDENTIFIER ')'  */
#line 977 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString sTmp;
		yyerror(sTmp + "Invalid syntax (" + *(yyvsp[-1].sValue) + ")");
		if ((yyvsp[-1].sValue) != NULL)
			delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = NULL;
	}
#line 2470 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 68: /* usedDerivationRule: %empty  */
#line 985 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = NULL;
	}
#line 2478 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 69: /* referenceRule: referenceRuleBody ']'  */
#line 991 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2486 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 70: /* referenceRuleBody: '[' derivationRuleOperand  */
#line 997 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2510 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 71: /* referenceRuleBody: referenceRuleBody ',' derivationRuleOperand  */
#line 1017 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2528 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 72: /* derivationRule: derivationRuleBody closeparenthesis  */
#line 1033 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
				/* Test de compatibilite avec la regle enregistree, sauf si regle avec operande de type indetermine */
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

						/* Dereferencement de la regle de derivation depuis l'operande de travail */
						ruleBodyOperand->SetDerivationRule(NULL);
					}
				}
			}

			/* Verification de la definition de la regle */
			if (bRuleOk and not rule->CheckDefinition())
			{
				bRuleOk = false;
				yyerror(sTmp + "Derivation rule " + rule->GetName() + " incorrectly specified");
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

					/* Acces a la regle de structure, transformation au format structure et nettoyage memoire */
					/* Cette optimisation memoire des regles structure est critique dans le cas de dictionnaires */
					/* de tres grande taille. Sinon, des millions d'operandes de regles sont potentiellement crees, */
					/* puis lors de la compilation des dictionnaire, l'essentiel de la memoire liberee laisse des trous */
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
#line 2707 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 73: /* derivationRuleBody: derivationRuleBegin  */
#line 1211 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2715 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 74: /* derivationRuleBody: derivationRuleHeader  */
#line 1215 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2723 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 75: /* derivationRuleHeader: IDENTIFIER openparenthesis  */
#line 1222 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule;

		/* Construction d'une regle pour accueillir les specification */
		rule = new KWDerivationRule;
		rule->SetName(*((yyvsp[-1].sValue)));
		delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = rule;
	}
#line 2737 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 76: /* derivationRuleBegin: derivationRuleHeader derivationRuleOperand  */
#line 1235 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-1].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() == 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2752 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 77: /* derivationRuleBegin: derivationRuleBegin ',' derivationRuleOperand  */
#line 1246 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() > 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2767 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 78: /* derivationRuleOperand: IDENTIFIER  */
#line 1260 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetDataItemName(*((yyvsp[0].sValue)));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2780 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 79: /* derivationRuleOperand: CONTINUOUSLITTERAL  */
#line 1269 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Continuous);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetContinuousConstant((yyvsp[0].cValue));
		(yyval.kwdroValue) = operand;
	}
#line 2793 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 80: /* derivationRuleOperand: bigstring  */
#line 1278 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Symbol);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetSymbolConstant(Symbol(*((yyvsp[0].sValue))));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2807 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 81: /* derivationRuleOperand: derivationRule  */
#line 1288 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule((yyvsp[0].kwdrValue));
		if (operand->GetDerivationRule() != NULL)
			operand->SetType(operand->GetDerivationRule()->GetType());
		(yyval.kwdroValue) = operand;
	}
#line 2821 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 82: /* derivationRuleOperand: '.' derivationRuleOperand  */
#line 1298 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = (yyvsp[0].kwdroValue);
		operand->SetScopeLevel(operand->GetScopeLevel() + 1);
		(yyval.kwdroValue) = operand;
	}
#line 2832 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 83: /* bigstring: bigstring '+' STRINGLITTERAL  */
#line 1309 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* Concatenation des deux chaines */
		(yyval.sValue) = new ALString(*(yyvsp[-2].sValue) + *(yyvsp[0].sValue));

		/* Destruction des ancienne chaines */
		delete (yyvsp[-2].sValue);
		delete (yyvsp[0].sValue);
	}
#line 2845 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 84: /* bigstring: STRINGLITTERAL  */
#line 1318 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 2853 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 86: /* semicolon: ';' ';'  */
#line 1326 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous ';'");
	}
#line 2861 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 87: /* semicolon: ';' ';' ';'  */
#line 1330 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ';'");
	}
#line 2869 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 88: /* semicolon: %empty  */
#line 1334 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Missing ';'");
	}
#line 2877 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 90: /* openparenthesis: '(' '('  */
#line 1342 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous '('");
	}
#line 2885 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 91: /* openparenthesis: '(' '(' '('  */
#line 1346 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many '('");
	}
#line 2893 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 92: /* openparenthesis: %empty  */
#line 1350 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing '('");
	}
#line 2906 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 94: /* closeparenthesis: %empty  */
#line 1364 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing ')'");
	}
#line 2919 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

#line 2923 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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
	yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE(yychar);
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
		yydestruct("Cleanup: popping", YY_ACCESSING_SYMBOL(+*yyssp), yyvsp);
		YYPOPSTACK(1);
	}
#ifndef yyoverflow
	if (yyss != yyssa)
		YYSTACK_FREE(yyss);
#endif

	return yyresult;
}

#line 1375 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

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
	snprintf(sErrorLine, sizeof(sErrorLine), "Line %d", nLineNumber);
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
				AddError("Dictionary " + kwcClass->GetName() + " used, but not declared");
			}
		}

		/* Desactivation du nombre max d'erreurs a afficher */
		Global::DesactivateErrorFlowControl();

		/* Destruction des classes crees si au moins une erreur de parsing detectee */
		/* ou au moins une classe referencee non cree                               */
		if (nFileParsingErrorNumber > 0 or odReferencedUncreatedClasses->GetCount() > 0)
		{
			/* En cas d'erreur, ajout d'une ligne blanche pour separer des autres logs */
			AddError("Errors detected during parsing " + sFileName + ": read operation cancelled");
			AddSimpleMessage("");
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
