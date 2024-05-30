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
#line 1 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

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
#include "KWRelationCreationRule.h"
#include "KWStructureRule.h"
#include "KWMetaData.h"

/* Declaration du lexer utilise */
void yyerror(char const* fmt);
void yyerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber);
int yylex();

/* Fonctions utilitaires pour rappatrier les information du parser vers une regle */
boolean ImportParserRuleOperands(const KWDerivationRule* parsedRule, KWDerivationRule* rule);
boolean ImportParserRuleOutputOperands(const KWDRRelationCreationRule* parsedRule, KWDRRelationCreationRule* rule);
boolean ImportParserOperand(const ALString& sRuleName, int nOperandIndex, KWDerivationRuleOperand* parsedOperand,
			    KWDerivationRuleOperand* operand);

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

#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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
	YYSYMBOL_33_ = 33,                         /* ':'  */
	YYSYMBOL_34_ = 34,                         /* '.'  */
	YYSYMBOL_35_ = 35,                         /* '+'  */
	YYSYMBOL_36_ = 36,                         /* ';'  */
	YYSYMBOL_YYACCEPT = 37,                    /* $accept  */
	YYSYMBOL_IDENTIFIER = 38,                  /* IDENTIFIER  */
	YYSYMBOL_SIMPLEIDENTIFIER = 39,            /* SIMPLEIDENTIFIER  */
	YYSYMBOL_kwclassFile = 40,                 /* kwclassFile  */
	YYSYMBOL_kwclasses = 41,                   /* kwclasses  */
	YYSYMBOL_kwclass = 42,                     /* kwclass  */
	YYSYMBOL_kwclassBegin = 43,                /* kwclassBegin  */
	YYSYMBOL_oaAttributeArrayDeclaration = 44, /* oaAttributeArrayDeclaration  */
	YYSYMBOL_kwclassHeader = 45,               /* kwclassHeader  */
	YYSYMBOL_keyFields = 46,                   /* keyFields  */
	YYSYMBOL_fieldList = 47,                   /* fieldList  */
	YYSYMBOL_metaData = 48,                    /* metaData  */
	YYSYMBOL_kwattributeDeclaration = 49,      /* kwattributeDeclaration  */
	YYSYMBOL_applicationids = 50,              /* applicationids  */
	YYSYMBOL_comments = 51,                    /* comments  */
	YYSYMBOL_rootDeclaration = 52,             /* rootDeclaration  */
	YYSYMBOL_usedDeclaration = 53,             /* usedDeclaration  */
	YYSYMBOL_typeDeclaration = 54,             /* typeDeclaration  */
	YYSYMBOL_refIdentifier = 55,               /* refIdentifier  */
	YYSYMBOL_usedDerivationRule = 56,          /* usedDerivationRule  */
	YYSYMBOL_referenceRule = 57,               /* referenceRule  */
	YYSYMBOL_referenceRuleBody = 58,           /* referenceRuleBody  */
	YYSYMBOL_derivationRule = 59,              /* derivationRule  */
	YYSYMBOL_derivationRuleBody = 60,          /* derivationRuleBody  */
	YYSYMBOL_operandList = 61,                 /* operandList  */
	YYSYMBOL_derivationRuleHeader = 62,        /* derivationRuleHeader  */
	YYSYMBOL_derivationRuleBegin = 63,         /* derivationRuleBegin  */
	YYSYMBOL_derivationRuleOperand = 64,       /* derivationRuleOperand  */
	YYSYMBOL_bigstring = 65,                   /* bigstring  */
	YYSYMBOL_semicolon = 66,                   /* semicolon  */
	YYSYMBOL_openparenthesis = 67,             /* openparenthesis  */
	YYSYMBOL_closeparenthesis = 68             /* closeparenthesis  */
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
#define YYLAST 207

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 37
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 32
/* YYNRULES -- Number of rules.  */
#define YYNRULES 97
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 142

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
    2,  2,  2, 2, 2, 2, 2, 2, 2, 25, 26, 2, 35, 27, 2, 34, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  33, 36, 28, 29,
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
    0,    143,  143,  147,  154,  158,  162,  166,  170,  174,  178,  182,  186,  190,  194,  198,  202,
    206,  210,  216,  229,  230,  233,  236,  246,  254,  295,  374,  393,  404,  447,  494,  586,  593,
    601,  607,  617,  630,  650,  672,  691,  708,  716,  851,  863,  870,  882,  889,  894,  901,  906,
    913,  917,  921,  925,  929,  933,  937,  941,  945,  949,  953,  961,  966,  972,  976,  980,  985,
    994,  999,  1005, 1025, 1041, 1141, 1145, 1191, 1198, 1209, 1222, 1235, 1246, 1260, 1269, 1278, 1288,
    1298, 1309, 1318, 1325, 1326, 1330, 1335, 1341, 1342, 1346, 1351, 1363, 1365};
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
				      "':'",
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
				      "fieldList",
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
				      "operandList",
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

#define YYPACT_NINF (-83)

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-96)

#define yytable_value_is_error(Yyn) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
    -83, 2,   14,  -83, -83, 67,  -83, -83, 110, -83, 66,  -83, -83, -12, -83, -83, 185, 38,  -83, -83, 60,
    46,  -83, 18,  -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, 62,  152, 52,  152, 55,  -83, 152,
    152, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, -83, 9,   -83, -83, 54,
    152, -83, 68,  54,  152, -83, 38,  152, 152, 43,  -12, -83, -8,  54,  -83, -12, -83, 17,  61,  70,  72,
    73,  74,  43,  57,  -83, -83, 43,  6,   -83, -83, 58,  -83, 43,  -83, -12, -83, -83, 152, -83, 172, -83,
    76,  -83, -83, -83, -83, -83, 43,  43,  -83, 86,  75,  -83, -83, 75,  38,  -83, 42,  77,  -83, 71,  -83,
    -83, 38,  75,  38,  132, -83, -83, 43,  38,  78,  80,  82,  -83, -83, -83, -83};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] = {
    44, 0,  22, 1,  43, 0,  21, 20, 0,  46, 19, 28, 49, 91, 46, 25, 0,  24, 45, 47, 0,  88, 23, 50, 51, 52, 59, 60, 53,
    54, 55, 56, 57, 58, 61, 63, 0,  89, 0,  50, 30, 0,  0,  4,  3,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 34, 2,  90, 68, 0,  29, 0,  68, 0,  41, 33, 0,  0,  0,  91, 65, 0,  68, 62, 91, 36, 0,  0,  0,  95, 64, 97, 75,
    73, 82, 87, 0,  81, 84, 70, 83, 41, 0,  69, 91, 41, 46, 0,  31, 0,  67, 92, 78, 66, 96, 72, 79, 0,  0,  85, 0,  46,
    71, 41, 46, 32, 35, 0,  93, 80, 74, 77, 86, 27, 46, 42, 0,  39, 94, 0,  26, 0,  0,  0,  76, 38, 37, 40};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-83, -35, 1,   -83, -83, -83, -83, -83, -83, -83, -83, -78, 3,   -83, -9,  -83,
				      -83, -83, -83, -39, -83, -83, 34,  -83, -83, -83, -83, -82, -83, -60, -83, -83};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] = {0,  91, 60, 1,  5,  7,  8,  39, 9,   68, 80, 81, 15, 2,  10,  20,
					16, 35, 42, 73, 74, 75, 92, 85, 124, 86, 87, 93, 94, 22, 106, 109};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = {
    17,  59,  3,   62,  110, 23,  65,  66,  113, -95, -95, -95, -95, 95,  116, -95, -46, 115, 99,  96,  -95,
    118, 4,   97,  21,  18,  40,  78,  76,  123, 125, 105, 79,  12,  67,  82,  83,  98,  117, 128, -95, 38,
    64,  100, 101, 18,  43,  44,  88,  89,  69,  138, 45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  120, -46, 6,   36,  12,  130, 131, 18,  -46, -48, -46, 90,  63,  70,  19,  -46, 37,  71,
    111, 102, 72,  41,  61,  103, 112, 119, 126, 114, 77,  137, 104, 105, 133, 107, 108, 122, 132, 103, 121,
    84,  127, 0,   139, 129, 140, 11,  141, 0,   0,   0,   0,   0,   0,   134, -50, -50, -50, -50, 0,   12,
    -50, -50, -50, -50, -50, -50, -50, 13,  14,  43,  44,  135, 136, 0,   0,   45,  46,  47,  48,  49,  50,
    51,  52,  53,  54,  55,  56,  57,  58,  43,  44,  0,   0,   0,   0,   45,  46,  47,  48,  49,  50,  51,
    52,  53,  54,  55,  56,  57,  58,  43,  0,   0,   0,   0,   0,   45,  46,  47,  48,  49,  50,  51,  52,
    53,  54,  55,  56,  57,  58,  24,  25,  26,  27,  0,   0,   28,  29,  30,  31,  32,  33,  34};

static const yytype_int16 yycheck[] = {
    9,   36, 0,   38,  86,  14,  41, 42, 90, 3,  4,  5,  6,  73,  96,  9,   7,  95,  78, 27, 14, 99, 8,  31, 36, 7,
    23,  66, 63,  111, 112, 25,  67, 15, 25, 70, 71, 76, 98, 117, 34,  23,  39, 26,  27, 7,  3,  4,  5,  6,  59, 133,
    9,   10, 11,  12,  13,  14,  15, 16, 17, 18, 19, 20, 21, 22,  101, 0,   1,  9,   15, 29, 30, 7,  7,  9,  9,  34,
    23,  25, 14,  14,  36,  29,  27, 24, 32, 25, 36, 28, 33, 100, 6,   35,  26, 130, 26, 25, 27, 26, 26, 25, 25, 28,
    103, 71, 115, -1,  30,  118, 30, 1,  30, -1, -1, -1, -1, -1,  -1,  128, 10, 11,  12, 13, -1, 15, 16, 17, 18, 19,
    20,  21, 22,  23,  24,  3,   4,  5,  6,  -1, -1, 9,  10, 11,  12,  13,  14, 15,  16, 17, 18, 19, 20, 21, 22, 3,
    4,   -1, -1,  -1,  -1,  9,   10, 11, 12, 13, 14, 15, 16, 17,  18,  19,  20, 21,  22, 3,  -1, -1, -1, -1, -1, 9,
    10,  11, 12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 10,  11,  12,  13, -1,  -1, 16, 17, 18, 19, 20, 21, 22};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] = {
    0,  40, 50, 0,  8,  41, 1,  42, 43, 45, 51, 1,  15, 23, 24, 49, 53, 51, 7,  14, 52, 36, 66, 51, 10, 11, 12, 13, 16,
    17, 18, 19, 20, 21, 22, 54, 9,  36, 23, 44, 49, 25, 55, 3,  4,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 38, 39, 36, 38, 23, 49, 38, 38, 25, 46, 51, 25, 29, 32, 56, 57, 58, 38, 26, 56, 38, 47, 48, 38, 38, 59, 60, 62,
    63, 5,  6,  34, 38, 59, 64, 65, 66, 27, 31, 56, 66, 26, 27, 24, 28, 26, 25, 67, 26, 26, 68, 64, 27, 33, 64, 35, 48,
    64, 66, 48, 51, 38, 39, 25, 64, 61, 64, 6,  51, 48, 51, 29, 30, 25, 27, 51, 5,  6,  38, 64, 30, 30, 30};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] = {0,  37, 38, 38, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 40,
				   41, 41, 41, 42, 43, 43, 43, 43, 43, 44, 44, 45, 46, 46, 46, 47, 47, 48, 48, 48,
				   48, 48, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 54, 54, 54, 54, 54, 54, 54,
				   54, 54, 55, 55, 56, 56, 56, 56, 56, 57, 58, 58, 59, 60, 60, 60, 61, 61, 62, 63,
				   63, 64, 64, 64, 64, 64, 65, 65, 66, 66, 66, 66, 67, 67, 67, 67, 68, 68};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = {0, 2,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 0, 3, 2,
				   2, 10, 9, 2, 2, 1, 7, 4, 1, 0, 3, 1, 6, 6, 4, 6, 0, 8, 2, 0, 2, 0, 1, 0, 1,
				   0, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 2, 1, 3, 3, 0, 2, 2, 3, 2, 1, 3,
				   1, 3,  1, 2, 2, 3, 1, 1, 1, 1, 2, 3, 1, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0};

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
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1046 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_EXTENDEDIDENTIFIER: /* EXTENDEDIDENTIFIER  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1052 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_STRINGLITTERAL: /* STRINGLITTERAL  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1058 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_LABEL: /* LABEL  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1064 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_APPLICATIONID: /* APPLICATIONID  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1070 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_IDENTIFIER: /* IDENTIFIER  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1076 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_SIMPLEIDENTIFIER: /* SIMPLEIDENTIFIER  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1082 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_oaAttributeArrayDeclaration: /* oaAttributeArrayDeclaration  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).oaAttributes) != NULL)
			delete ((*yyvaluep).oaAttributes);
		((*yyvaluep).oaAttributes) = NULL;
	}
#line 1088 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_keyFields: /* keyFields  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1094 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_fieldList: /* fieldList  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1100 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_metaData: /* metaData  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwmdMetaData) != NULL)
			delete ((*yyvaluep).kwmdMetaData);
		((*yyvaluep).kwmdMetaData) = NULL;
	}
#line 1106 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_kwattributeDeclaration: /* kwattributeDeclaration  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwaValue) != NULL)
			delete ((*yyvaluep).kwaValue);
		((*yyvaluep).kwaValue) = NULL;
	}
#line 1112 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_applicationids: /* applicationids  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1118 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_comments: /* comments  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1124 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_refIdentifier: /* refIdentifier  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1130 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_usedDerivationRule: /* usedDerivationRule  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1136 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRule: /* referenceRule  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1142 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRuleBody: /* referenceRuleBody  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1148 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRule: /* derivationRule  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1154 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBody: /* derivationRuleBody  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1160 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_operandList: /* operandList  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).oaOperands) != NULL)
			delete ((*yyvaluep).oaOperands);
		((*yyvaluep).oaOperands) = NULL;
	}
#line 1166 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleHeader: /* derivationRuleHeader  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1172 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBegin: /* derivationRuleBegin  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1178 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleOperand: /* derivationRuleOperand  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdroValue) != NULL)
			delete ((*yyvaluep).kwdroValue);
		((*yyvaluep).kwdroValue) = NULL;
	}
#line 1184 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_bigstring: /* bigstring  */
#line 134 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1190 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
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
#line 144 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1462 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 3: /* IDENTIFIER: EXTENDEDIDENTIFIER  */
#line 148 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1470 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 4: /* SIMPLEIDENTIFIER: BASICIDENTIFIER  */
#line 155 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1478 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 5: /* SIMPLEIDENTIFIER: CLASS  */
#line 159 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Dictionary");
	}
#line 1486 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 6: /* SIMPLEIDENTIFIER: CONTINUOUSTYPE  */
#line 163 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Numerical");
	}
#line 1494 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 7: /* SIMPLEIDENTIFIER: SYMBOLTYPE  */
#line 167 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Categorical");
	}
#line 1502 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 8: /* SIMPLEIDENTIFIER: OBJECTTYPE  */
#line 171 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Entity");
	}
#line 1510 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 9: /* SIMPLEIDENTIFIER: OBJECTARRAYTYPE  */
#line 175 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Table");
	}
#line 1518 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 10: /* SIMPLEIDENTIFIER: ROOT  */
#line 179 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Root");
	}
#line 1526 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 11: /* SIMPLEIDENTIFIER: UNUSED  */
#line 183 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Unused");
	}
#line 1534 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 12: /* SIMPLEIDENTIFIER: DATETYPE  */
#line 187 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Date");
	}
#line 1542 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 13: /* SIMPLEIDENTIFIER: TIMETYPE  */
#line 191 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Time");
	}
#line 1550 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 14: /* SIMPLEIDENTIFIER: TIMESTAMPTYPE  */
#line 195 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Timestamp");
	}
#line 1558 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 15: /* SIMPLEIDENTIFIER: TIMESTAMPTZTYPE  */
#line 199 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TimestampTZ");
	}
#line 1566 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 16: /* SIMPLEIDENTIFIER: TEXTTYPE  */
#line 203 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Text");
	}
#line 1574 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 17: /* SIMPLEIDENTIFIER: TEXTLISTTYPE  */
#line 207 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TextList");
	}
#line 1582 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 18: /* SIMPLEIDENTIFIER: STRUCTURETYPE  */
#line 211 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Structure");
	}
#line 1590 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 19: /* kwclassFile: applicationids kwclasses comments  */
#line 217 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore l'identification d'application */
		if ((yyvsp[-2].sValue) != NULL)
			delete (yyvsp[-2].sValue);

		/* On ignore les commentaires en fin de fichier */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
	}
#line 1604 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 21: /* kwclasses: kwclasses error  */
#line 231 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Error outside the definition of a dictionary");
		YYABORT;
	}
#line 1611 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 23: /* kwclass: kwclassBegin '}' semicolon  */
#line 237 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* La completion des informations de type (CompleteTypeInfo) est centralisee */
		/* au niveau du domaine en fin de parsing */

		/* Reinitialisation de la classe courante */
		kwcLoadCurrentClass = NULL;
	}
#line 1623 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 24: /* kwclassBegin: kwclassHeader comments  */
#line 247 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les premiers comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));
		(yyval.kwcValue) = (yyvsp[-1].kwcValue);
	}
#line 1635 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 25: /* kwclassBegin: kwclassBegin kwattributeDeclaration  */
#line 255 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-1].kwcValue);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		ALString sMessage;
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
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
#line 1680 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 26: /* kwclassBegin: kwclassBegin '{' comments oaAttributeArrayDeclaration '}' IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 296 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-9].kwcValue);
		KWAttributeBlock* attributeBlock;
		ObjectArray* oaAttributes = (yyvsp[-6].oaAttributes);
		ALString sBlockName;
		KWAttribute* firstAttribute;
		KWAttribute* lastAttribute;
		KWDerivationRule* rule = (yyvsp[-3].kwdrValue);
		ALString sMessage;
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
			if (!kwcClass->CheckNameWithMessage(sBlockName, KWClass::AttributeBlock, sMessage))
			{
				yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
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
#line 1763 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 27: /* kwclassBegin: kwclassBegin '{' comments '}' IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 375 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 1786 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 28: /* kwclassBegin: kwclassBegin error  */
#line 394 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: cette regle qui permet une gestion des erreurs amelioree */
		/* genere un conflit reduce/reduce */
		kwcLoadCurrentClass = NULL;
		YYABORT;
	}
#line 1798 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 29: /* oaAttributeArrayDeclaration: oaAttributeArrayDeclaration kwattributeDeclaration  */
#line 405 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaAttributes = (yyvsp[-1].oaAttributes);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
		ALString sMessage;
		check(oaAttributes);

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
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
#line 1845 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 30: /* oaAttributeArrayDeclaration: kwattributeDeclaration  */
#line 448 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaAttributes;
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
		ALString sMessage;

		/* Creation d'un tableau */
		oaAttributes = new ObjectArray;

		/* Si attribut non valide: on ne fait rien */
		if (attribute == NULL)
			;
		/* Si classe non valide, supression de l'attribut */
		else if (kwcClass == NULL)
			delete attribute;
		/* Sinon, test de validite du nom de l'attribut */
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
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
#line 1894 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 31: /* kwclassHeader: comments rootDeclaration CLASS IDENTIFIER keyFields metaData '{'  */
#line 495 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass;
		KWClass* kwcReferencedClass;
		ALString sMessage;

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
			if (KWClass::CheckNameWithMessage(*((yyvsp[-3].sValue)), KWClass::Class, sMessage))
			{
				kwcClass = new KWClass;
				kwcClass->SetName(*((yyvsp[-3].sValue)));
				kwcdLoadDomain->InsertClass(kwcClass);
			}
			else
				yyerror(sMessage);
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
#line 1987 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 32: /* keyFields: '(' fieldList ')' comments  */
#line 587 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = (yyvsp[-2].svValue);
	}
#line 1998 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 33: /* keyFields: comments  */
#line 594 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* On ignore les comemntaires */
		if ((yyvsp[0].sValue) != NULL)
			delete (yyvsp[0].sValue);
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 2009 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 34: /* keyFields: %empty  */
#line 601 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = NULL; /* pas de champ cle */
	}
#line 2017 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 35: /* fieldList: fieldList ',' IDENTIFIER  */
#line 608 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svFields;

		/* Ajout d'un nouveau de champ */
		svFields = cast(StringVector*, (yyvsp[-2].svValue));
		svFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svFields;
	}
#line 2031 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 36: /* fieldList: IDENTIFIER  */
#line 618 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svFields;

		/* Creation d'un tableau de champs, avec un premier champ */
		svFields = new StringVector;
		svFields->Add(*(yyvsp[0].sValue));
		delete (yyvsp[0].sValue);
		(yyval.svValue) = svFields;
	}
#line 2045 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 37: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' STRINGLITTERAL '>'  */
#line 631 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2069 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 38: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' CONTINUOUSLITTERAL '>'  */
#line 651 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2095 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 39: /* metaData: metaData '<' SIMPLEIDENTIFIER '>'  */
#line 673 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2118 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 40: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' IDENTIFIER '>'  */
#line 692 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2138 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 41: /* metaData: %empty  */
#line 708 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwmdMetaData) = NULL; /* pas de paires cle valeurs */
	}
#line 2146 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 42: /* kwattributeDeclaration: usedDeclaration typeDeclaration refIdentifier IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 724 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
				if (KWClass::CheckName(*((yyvsp[-5].sValue)), KWClass::Class, NULL))
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
#line 2273 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 43: /* applicationids: applicationids APPLICATIONID  */
#line 852 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2288 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 44: /* applicationids: %empty  */
#line 863 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; /* pas d'identification d'application */
	}
#line 2296 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 45: /* comments: comments LABEL  */
#line 871 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2311 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 46: /* comments: %empty  */
#line 882 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; /* pas de commentaire */
	}
#line 2319 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 47: /* rootDeclaration: ROOT  */
#line 890 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true;
	}
#line 2327 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 48: /* rootDeclaration: %empty  */
#line 894 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false; /* valeur par defaut */
	}
#line 2335 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 49: /* usedDeclaration: UNUSED  */
#line 902 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false;
	}
#line 2343 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 50: /* usedDeclaration: %empty  */
#line 906 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true; /* valeur par defaut */
	}
#line 2351 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 51: /* typeDeclaration: CONTINUOUSTYPE  */
#line 914 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Continuous;
	}
#line 2359 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 52: /* typeDeclaration: SYMBOLTYPE  */
#line 918 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Symbol;
	}
#line 2367 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 53: /* typeDeclaration: DATETYPE  */
#line 922 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Date;
	}
#line 2375 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 54: /* typeDeclaration: TIMETYPE  */
#line 926 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Time;
	}
#line 2383 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 55: /* typeDeclaration: TIMESTAMPTYPE  */
#line 930 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Timestamp;
	}
#line 2391 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 56: /* typeDeclaration: TIMESTAMPTZTYPE  */
#line 934 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TimestampTZ;
	}
#line 2399 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 57: /* typeDeclaration: TEXTTYPE  */
#line 938 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Text;
	}
#line 2407 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 58: /* typeDeclaration: TEXTLISTTYPE  */
#line 942 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TextList;
	}
#line 2415 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 59: /* typeDeclaration: OBJECTTYPE  */
#line 946 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Object;
	}
#line 2423 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 60: /* typeDeclaration: OBJECTARRAYTYPE  */
#line 950 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::ObjectArray;
	}
#line 2431 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 61: /* typeDeclaration: STRUCTURETYPE  */
#line 954 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Structure;
	}
#line 2439 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 62: /* refIdentifier: '(' IDENTIFIER ')'  */
#line 962 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[-1].sValue);
	}
#line 2447 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 63: /* refIdentifier: %empty  */
#line 966 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL;
	}
#line 2455 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 64: /* usedDerivationRule: '=' derivationRule  */
#line 973 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2463 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 65: /* usedDerivationRule: referenceRule  */
#line 977 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2471 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 66: /* usedDerivationRule: '=' derivationRule ')'  */
#line 981 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ')'");
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2480 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 67: /* usedDerivationRule: '(' IDENTIFIER ')'  */
#line 986 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString sTmp;
		yyerror(sTmp + "Invalid syntax (" + *(yyvsp[-1].sValue) + ")");
		if ((yyvsp[-1].sValue) != NULL)
			delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = NULL;
	}
#line 2492 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 68: /* usedDerivationRule: %empty  */
#line 994 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = NULL;
	}
#line 2500 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 69: /* referenceRule: referenceRuleBody ']'  */
#line 1000 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2508 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 70: /* referenceRuleBody: '[' derivationRuleOperand  */
#line 1006 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2532 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 71: /* referenceRuleBody: referenceRuleBody ',' derivationRuleOperand  */
#line 1026 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
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
#line 2550 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 72: /* derivationRule: derivationRuleBody closeparenthesis  */
#line 1042 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		boolean bOk = true;
		KWDerivationRule* ruleBody = (yyvsp[-1].kwdrValue);
		KWDerivationRule* rule;
		KWDRRelationCreationRule* relationCreationRuleBody;
		KWDRRelationCreationRule* relationCreationRule;
		ALString sTmp;

		/* Recherche de la regle de reference */
		/* On ensuite recuperer au maximum les informations de la regle clonee */
		/* et rapatrier les informations issues du parsing concernant les operandes */
		check(ruleBody);
		rule = KWDerivationRule::CloneDerivationRule(ruleBody->GetName());

		/* Erreur si regle inexistante */
		if (rule == NULL)
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
			bOk = false;
		}
		/* Erreur si regle predefinie de Reference */
		else if (rule->GetName() == KWDerivationRule::GetReferenceRuleName())
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
			bOk = false;
		}

		/* Import des operandes de la regle */
		if (bOk)
			bOk = ImportParserRuleOperands(ruleBody, rule);

		/* Gestion des operandes en sortie dans le cas ou le parser a stocke des operandes en sortie */
		if (bOk and KWType::IsRelation(ruleBody->GetType()) and not ruleBody->GetReference())
		{
			/* Erreur si la regle en cours n'est pas une regle de creation d'instance */
			if (not KWType::IsRelation(rule->GetType()) or rule->GetReference())
			{
				yyerror(sTmp + "Derivation rule " + rule->GetName() +
					" does not accept output operands");
				bOk = false;
			}
			/* Sinon, transfert des operandes en sortie */
			else
			{
				/* On est passe prealablement dans le parser par une regle de creation de relation */
				/* pour stocker les operandes en sortie, que l'on va ici exploiter */

				/* Cast des regles pour gerer les operandes en sortie */
				relationCreationRuleBody = cast(KWDRRelationCreationRule*, ruleBody);
				relationCreationRule = cast(KWDRRelationCreationRule*, rule);

				/* Import des operandes en sortie de la regle */
				bOk = ImportParserRuleOutputOperands(relationCreationRuleBody, relationCreationRule);
			}
		}

		/* Verification de la definition de la regle */
		if (bOk and not rule->CheckDefinition())
		{
			yyerror(sTmp + "Derivation rule " + rule->GetName() + " incorrectly specified");
			bOk = false;
		}

		/* Test si erreur dans le transfert des operandes */
		if (not bOk)
		{
			if (rule != NULL)
			{
				delete rule;
				rule = NULL;
			}
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

		/* Finalisation */
		delete ruleBody;
		(yyval.kwdrValue) = rule;
	}
#line 2651 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 73: /* derivationRuleBody: derivationRuleBegin  */
#line 1142 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2659 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 74: /* derivationRuleBody: derivationRuleBegin ':' operandList  */
#line 1146 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* ruleBody = (yyvsp[-2].kwdrValue);
		KWDRRelationCreationRule* ruleRelationCreationBody;
		ObjectArray oaOperands;
		ObjectArray* oaOutputOperands;
		int nOperand;
		KWDerivationRuleOperand* operand;

		/* On passe par une regle de creation de relation pour stocker les operandes en sortie */
		ruleRelationCreationBody = new KWDRRelationCreationRule;

		// On transfer les operande initiaux vers un tableau
		for (nOperand = 0; nOperand < ruleBody->GetOperandNumber(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, ruleBody->GetOperandAt(nOperand));
			oaOperands.Add(operand);
		}
		ruleBody->RemoveAllOperands();

		/* On copie la regle initiale, maintenant nettoyee de ses operandes */
		ruleBody->SetType(KWType::ObjectArray);
		ruleRelationCreationBody->CopyFrom(ruleBody);
		delete ruleBody;

		/* On recupere les operandes initiaux */
		for (nOperand = 0; nOperand < oaOperands.GetSize(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(nOperand));
			ruleRelationCreationBody->AddOperand(operand);
		}

		/* On recupere la liste des operandes en sortie */
		oaOutputOperands = cast(ObjectArray*, (yyvsp[0].oaOperands));

		/* Parametrage des operandes en sortie */
		assert(ruleRelationCreationBody->GetOutputOperandNumber() == 0);
		for (nOperand = 0; nOperand < oaOutputOperands->GetSize(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOutputOperands->GetAt(nOperand));
			ruleRelationCreationBody->AddOutputOperand(operand);
		}
		delete oaOutputOperands;

		(yyval.kwdrValue) = ruleRelationCreationBody;
	}
#line 2709 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 75: /* derivationRuleBody: derivationRuleHeader  */
#line 1192 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2717 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 76: /* operandList: operandList ',' derivationRuleOperand  */
#line 1199 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaOperandList = (yyvsp[-2].oaOperands);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);
		check(oaOperandList);
		check(operand);

		/* Ajout d'un operande */
		oaOperandList->Add(operand);
		(yyval.oaOperands) = oaOperandList;
	}
#line 2732 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 77: /* operandList: derivationRuleOperand  */
#line 1210 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaOperandList;
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);
		check(operand);

		/* Creation d'un tableau doperandes, avec un premier operande */
		oaOperandList = new ObjectArray;
		oaOperandList->Add(operand);
		(yyval.oaOperands) = oaOperandList;
	}
#line 2747 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 78: /* derivationRuleHeader: IDENTIFIER openparenthesis  */
#line 1223 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule;

		/* Construction d'une regle pour accueillir les specification */
		rule = new KWDerivationRule;
		rule->SetName(*((yyvsp[-1].sValue)));
		delete (yyvsp[-1].sValue);
		(yyval.kwdrValue) = rule;
	}
#line 2761 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 79: /* derivationRuleBegin: derivationRuleHeader derivationRuleOperand  */
#line 1236 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-1].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() == 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2776 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 80: /* derivationRuleBegin: derivationRuleBegin ',' derivationRuleOperand  */
#line 1247 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() > 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2791 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 81: /* derivationRuleOperand: IDENTIFIER  */
#line 1261 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetDataItemName(*((yyvsp[0].sValue)));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2804 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 82: /* derivationRuleOperand: CONTINUOUSLITTERAL  */
#line 1270 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Continuous);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetContinuousConstant((yyvsp[0].cValue));
		(yyval.kwdroValue) = operand;
	}
#line 2817 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 83: /* derivationRuleOperand: bigstring  */
#line 1279 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Symbol);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetSymbolConstant(Symbol(*((yyvsp[0].sValue))));
		delete (yyvsp[0].sValue);
		(yyval.kwdroValue) = operand;
	}
#line 2831 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 84: /* derivationRuleOperand: derivationRule  */
#line 1289 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule((yyvsp[0].kwdrValue));
		if (operand->GetDerivationRule() != NULL)
			operand->SetType(operand->GetDerivationRule()->GetType());
		(yyval.kwdroValue) = operand;
	}
#line 2845 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 85: /* derivationRuleOperand: '.' derivationRuleOperand  */
#line 1299 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand;
		operand = (yyvsp[0].kwdroValue);
		operand->SetScopeLevel(operand->GetScopeLevel() + 1);
		(yyval.kwdroValue) = operand;
	}
#line 2856 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 86: /* bigstring: bigstring '+' STRINGLITTERAL  */
#line 1310 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* Concatenation des deux chaines */
		(yyval.sValue) = new ALString(*(yyvsp[-2].sValue) + *(yyvsp[0].sValue));

		/* Destruction des ancienne chaines */
		delete (yyvsp[-2].sValue);
		delete (yyvsp[0].sValue);
	}
#line 2869 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 87: /* bigstring: STRINGLITTERAL  */
#line 1319 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 2877 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 89: /* semicolon: ';' ';'  */
#line 1327 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous ';'");
	}
#line 2885 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 90: /* semicolon: ';' ';' ';'  */
#line 1331 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ';'");
	}
#line 2893 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 91: /* semicolon: %empty  */
#line 1335 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Missing ';'");
	}
#line 2901 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 93: /* openparenthesis: '(' '('  */
#line 1343 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous '('");
	}
#line 2909 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 94: /* openparenthesis: '(' '(' '('  */
#line 1347 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many '('");
	}
#line 2917 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 95: /* openparenthesis: %empty  */
#line 1351 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing '('");
	}
#line 2930 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 97: /* closeparenthesis: %empty  */
#line 1365 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		/* ERRORMGT */
		/* Attention: supprimer cette instruction en cas d'evolution du parser */
		/* Cette instruction est la pour aider au diagnostique des erreurs */
		/* de parenthesage: elle est utile dans ce cas, mais genere (avec  */
		/* sa consoeur 3 shift/reduce conflicts et 12 reduce conflicts     */
		yyerror("Missing ')'");
	}
#line 2943 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

#line 2947 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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

#line 1376 "D:/Users/miib6422/Documents/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

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

boolean ImportParserRuleOperands(const KWDerivationRule* parsedRule, KWDerivationRule* rule)
{
	boolean bOk = true;
	KWDerivationRuleOperand* parsedOperand;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(parsedRule != NULL);
	require(rule != NULL);

	/* Test du nombre d'operandes */
	if ((rule->GetVariableOperandNumber() and parsedRule->GetOperandNumber() < rule->GetOperandNumber() - 1) or
	    (not rule->GetVariableOperandNumber() and parsedRule->GetOperandNumber() != rule->GetOperandNumber()))

	{
		yyerror(sTmp + "Number of operands (" + IntToString(parsedRule->GetOperandNumber()) +
			") inconsistent with that of rule " + rule->GetName() + " (" +
			IntToString(rule->GetOperandNumber()) + ")");
		bOk = false;
	}
	/* Verification et transfert des operandes */
	else
	{
		/* Dans le cas d'un nombre variable d'operandes, on commence par rajouter */
		/* eventuellement des operandes en fin de regle pour preparer l'instanciation */
		if (parsedRule->GetOperandNumber() > rule->GetOperandNumber())
		{
			assert(rule->GetVariableOperandNumber());
			while (rule->GetOperandNumber() < parsedRule->GetOperandNumber())
				rule->AddOperand(rule->GetOperandAt(rule->GetOperandNumber() - 1)->Clone());
		}

		/* Dans le cas d'un nombre variable d'operandes, on supprime eventuellement */
		/* le dernier operande, qui n'est pas obligatoire */
		if (parsedRule->GetOperandNumber() < rule->GetOperandNumber())
		{
			assert(rule->GetVariableOperandNumber());
			assert(parsedRule->GetOperandNumber() == rule->GetOperandNumber() - 1);
			rule->DeleteAllVariableOperands();
		}
		assert(parsedRule->GetOperandNumber() == rule->GetOperandNumber());

		/* Transfert des operandes */
		for (i = 0; i < rule->GetOperandNumber(); i++)
		{
			/* acces aux operandes */
			operand = rule->GetOperandAt(i);
			parsedOperand = parsedRule->GetOperandAt(i);

			/* Import de l'operande */
			bOk = ImportParserOperand(rule->GetName(), i, parsedOperand, operand);
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean ImportParserRuleOutputOperands(const KWDRRelationCreationRule* parsedRule, KWDRRelationCreationRule* rule)
{
	boolean bOk = true;
	KWDerivationRuleOperand* parsedOperand;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(parsedRule != NULL);
	require(rule != NULL);

	/* Test du nombre d'operandes en sortie */
	if ((rule->GetVariableOutputOperandNumber() and
	     parsedRule->GetOutputOperandNumber() < rule->GetOutputOperandNumber() - 1) or
	    (not rule->GetVariableOutputOperandNumber() and
	     parsedRule->GetOutputOperandNumber() != rule->GetOutputOperandNumber()))

	{
		yyerror(sTmp + "Number of output operands (" + IntToString(parsedRule->GetOutputOperandNumber()) +
			") inconsistent with that of rule " + rule->GetName() + " (" +
			IntToString(rule->GetOutputOperandNumber()) + ")");
		bOk = false;
	}
	/* Verification et transfert des operandes en sortie */
	else
	{
		/* Dans le cas d'un nombre variable d'operandes en sortie, on commence par rajouter */
		/* eventuellement des operandes en fin de regle pour preparer l'instanciation */
		if (parsedRule->GetOutputOperandNumber() > rule->GetOutputOperandNumber())
		{
			assert(rule->GetVariableOutputOperandNumber());
			while (rule->GetOutputOperandNumber() < parsedRule->GetOutputOperandNumber())
				rule->AddOutputOperand(rule->GetOperandAt(rule->GetOutputOperandNumber() - 1)->Clone());
		}

		/* Dans le cas d'un nombre variable d'operandes en sortie, on supprime eventuellement */
		/* le dernier operande, qui n'est pas obligatoire */
		if (parsedRule->GetOutputOperandNumber() < rule->GetOutputOperandNumber())
		{
			assert(rule->GetVariableOutputOperandNumber());
			assert(parsedRule->GetOutputOperandNumber() == rule->GetOutputOperandNumber() - 1);
			rule->DeleteAllVariableOutputOperands();
		}
		assert(parsedRule->GetOutputOperandNumber() == rule->GetOutputOperandNumber());

		/* Transfert des operandes en sortie */
		for (i = 0; i < rule->GetOutputOperandNumber(); i++)
		{
			/* acces aux operandes en sortie */
			operand = rule->GetOutputOperandAt(i);
			parsedOperand = parsedRule->GetOutputOperandAt(i);

			/* Import de l'operande */
			bOk = ImportParserOperand(rule->GetName(), i, parsedOperand, operand);
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean ImportParserOperand(const ALString& sRuleName, int nOperandIndex, KWDerivationRuleOperand* parsedOperand,
			    KWDerivationRuleOperand* operand)
{
	boolean bOk = true;
	ALString sTmp;

	require(parsedOperand != NULL);
	require(operand != NULL);

	/* Transfert d'informations de la regle de reference vers la regle a verifier */
	if (parsedOperand->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
	{
		parsedOperand->SetType(operand->GetType());
		if (KWType::IsRelation(operand->GetType()))
			parsedOperand->SetObjectClassName(operand->GetObjectClassName());
	}
	if (operand->GetType() == KWType::Structure)
	{
		parsedOperand->SetType(KWType::Structure);
		parsedOperand->SetStructureName(operand->GetStructureName());
	}

	/* Test si operande candidate valide */
	if (not parsedOperand->CheckDefinition())
	{
		bOk = false;
		yyerror(sTmp + "Incorrect operand " + IntToString(1 + nOperandIndex) + " for rule " + sRuleName);
	}
	/* Test de compatibilite avec la regle enregistree, sauf si regle avec operande de type indetermine */
	else if (operand->GetType() != KWType::Unknown and not parsedOperand->CheckFamily(operand))
	{
		bOk = false;
		yyerror(sTmp + "Operand " + IntToString(1 + nOperandIndex) + " inconsistent with that of rule " +
			sRuleName);
	}
	/* Transfert de l'origine de l'operande */
	else
	{
		/* Transfert du niveau de scope */
		operand->SetScopeLevel(parsedOperand->GetScopeLevel());

		/* Transfert d'une valeur constante */
		operand->SetOrigin(parsedOperand->GetOrigin());
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
		{
			operand->SetType(parsedOperand->GetType());
			operand->SetStringConstant(parsedOperand->GetStringConstant());
		}
		/* Transfert d'un attribut */
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
			operand->SetDataItemName(parsedOperand->GetDataItemName());
		else
		/* Transfert d'une regle */
		{
			// Transfert de la regle */
			if (operand->GetDerivationRule() != NULL)
			{
				assert(parsedOperand->GetDerivationRule() != NULL);
				delete operand->GetDerivationRule();
			}
			operand->SetDerivationRule(parsedOperand->GetDerivationRule());

			/* Transfert des infos portees par la regle de derivation */
			if (operand->GetDerivationRule() != NULL)
			{
				operand->SetType(operand->GetDerivationRule()->GetType());
				if (KWType::IsRelation(operand->GetType()))
					operand->SetObjectClassName(operand->GetDerivationRule()->GetObjectClassName());
				if (operand->GetType() == KWType::Structure)
					operand->SetStructureName(operand->GetDerivationRule()->GetStructureName());
			}

			/* Dereferencement de la regle de derivation depuis l'operande de travail */
			parsedOperand->SetDerivationRule(NULL);
		}
	}
	return bOk;
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
