// Copyright (c) 2023-2025 Orange. All rights reserved.
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

// ATTENTION: les regles openparenthesis et closeparenthesis generent
// de nombreux shift/reduce et reduce/reduce conflicts
// D'autre regles, notamment pour le gestion des commentaires, generent
// egalement des conflits

#include "KWClassDomain.h"
#include "KWClass.h"
#include "KWContinuous.h"
#include "KWDerivationRule.h"
#include "KWRelationCreationRule.h"
#include "KWStructureRule.h"
#include "KWMetaData.h"

// Declaration du lexer utilise
void yyerror(char const* fmt);
void yyerrorWithLineCorrection(char const* fmt, int nDeltaLineNumber);
int yylex();

// Fonctions utilitaires pour rappatrier les information du parser vers une regle
boolean ImportParserRuleOperands(const KWDerivationRule* parsedRule, KWDerivationRule* rule);
boolean ImportParserRuleOutputOperands(const KWDerivationRule* parsedRule, KWDerivationRule* rule);
boolean ImportParserOperand(const ALString& sRuleName, int nOperandIndex, KWDerivationRuleOperand* parsedOperand,
			    KWDerivationRuleOperand* operand);

// Work around a bug in the relation between bison and GCC 3.x:
#if defined(__GNUC__) && 3 <= __GNUC__
#define __attribute__(arglist)
#endif

// Domaine de classe courant a utiliser pendant la lecture d'un fichier.
// Ce domaine est positionner par la methode Load de KWClassDomain
static KWClassDomain* kwcdLoadDomain = NULL;

// Classe courante a utiliser pendant la lecture d'un fichier.
// Ce domaine est positionner par la methode Load de KWClassDomain
static KWClass* kwcLoadCurrentClass = NULL;

// Dictionnaire des classes referencees creees a la volee lorsqu'elles sont
// utilisees, mais non crees.
// On rajoute les classes referencees non crees, et on retire les classes crees.
static ObjectDictionary* odReferencedUncreatedClasses = NULL;

// Nombre total d'erreurs de parsing
static int nFileParsingErrorNumber = 0;

#define YY_STATIC

// Debugging YAC
// Ajouter ici les instruction suivantes
//   #define YYDEBUG 1
//   extern char   *yyptok(int i);
// Ajouter l'instruction yydebug = 1 dans le code d'une action du fichier .lex ou .yac

#line 130 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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
	YYSYMBOL_COMMENT = 7,                      /* COMMENT  */
	YYSYMBOL_LABEL = 8,                        /* LABEL  */
	YYSYMBOL_APPLICATIONID = 9,                /* APPLICATIONID  */
	YYSYMBOL_CLASS = 10,                       /* CLASS  */
	YYSYMBOL_CONTINUOUSTYPE = 11,              /* CONTINUOUSTYPE  */
	YYSYMBOL_SYMBOLTYPE = 12,                  /* SYMBOLTYPE  */
	YYSYMBOL_OBJECTTYPE = 13,                  /* OBJECTTYPE  */
	YYSYMBOL_OBJECTARRAYTYPE = 14,             /* OBJECTARRAYTYPE  */
	YYSYMBOL_ROOT = 15,                        /* ROOT  */
	YYSYMBOL_UNUSED = 16,                      /* UNUSED  */
	YYSYMBOL_DATETYPE = 17,                    /* DATETYPE  */
	YYSYMBOL_TIMETYPE = 18,                    /* TIMETYPE  */
	YYSYMBOL_TIMESTAMPTYPE = 19,               /* TIMESTAMPTYPE  */
	YYSYMBOL_TIMESTAMPTZTYPE = 20,             /* TIMESTAMPTZTYPE  */
	YYSYMBOL_TEXTTYPE = 21,                    /* TEXTTYPE  */
	YYSYMBOL_TEXTLISTTYPE = 22,                /* TEXTLISTTYPE  */
	YYSYMBOL_STRUCTURETYPE = 23,               /* STRUCTURETYPE  */
	YYSYMBOL_24_ = 24,                         /* '}'  */
	YYSYMBOL_25_ = 25,                         /* '{'  */
	YYSYMBOL_26_ = 26,                         /* '('  */
	YYSYMBOL_27_ = 27,                         /* ')'  */
	YYSYMBOL_28_ = 28,                         /* ','  */
	YYSYMBOL_29_ = 29,                         /* '<'  */
	YYSYMBOL_30_ = 30,                         /* '='  */
	YYSYMBOL_31_ = 31,                         /* '>'  */
	YYSYMBOL_32_ = 32,                         /* ']'  */
	YYSYMBOL_33_ = 33,                         /* '['  */
	YYSYMBOL_34_ = 34,                         /* ':'  */
	YYSYMBOL_35_ = 35,                         /* '.'  */
	YYSYMBOL_36_ = 36,                         /* '+'  */
	YYSYMBOL_37_ = 37,                         /* ';'  */
	YYSYMBOL_YYACCEPT = 38,                    /* $accept  */
	YYSYMBOL_IDENTIFIER = 39,                  /* IDENTIFIER  */
	YYSYMBOL_SIMPLEIDENTIFIER = 40,            /* SIMPLEIDENTIFIER  */
	YYSYMBOL_kwclassFile = 41,                 /* kwclassFile  */
	YYSYMBOL_kwclasses = 42,                   /* kwclasses  */
	YYSYMBOL_kwclass = 43,                     /* kwclass  */
	YYSYMBOL_kwclassBegin = 44,                /* kwclassBegin  */
	YYSYMBOL_oaAttributeArrayDeclaration = 45, /* oaAttributeArrayDeclaration  */
	YYSYMBOL_kwclassHeader = 46,               /* kwclassHeader  */
	YYSYMBOL_keyFields = 47,                   /* keyFields  */
	YYSYMBOL_fieldList = 48,                   /* fieldList  */
	YYSYMBOL_metaData = 49,                    /* metaData  */
	YYSYMBOL_kwattributeDeclaration = 50,      /* kwattributeDeclaration  */
	YYSYMBOL_applicationids = 51,              /* applicationids  */
	YYSYMBOL_label = 52,                       /* label  */
	YYSYMBOL_comments = 53,                    /* comments  */
	YYSYMBOL_labelOrComments = 54,             /* labelOrComments  */
	YYSYMBOL_rootDeclaration = 55,             /* rootDeclaration  */
	YYSYMBOL_usedDeclaration = 56,             /* usedDeclaration  */
	YYSYMBOL_typeDeclaration = 57,             /* typeDeclaration  */
	YYSYMBOL_refIdentifier = 58,               /* refIdentifier  */
	YYSYMBOL_usedDerivationRule = 59,          /* usedDerivationRule  */
	YYSYMBOL_referenceRule = 60,               /* referenceRule  */
	YYSYMBOL_referenceRuleBody = 61,           /* referenceRuleBody  */
	YYSYMBOL_derivationRule = 62,              /* derivationRule  */
	YYSYMBOL_derivationRuleBody = 63,          /* derivationRuleBody  */
	YYSYMBOL_operandList = 64,                 /* operandList  */
	YYSYMBOL_derivationRuleHeader = 65,        /* derivationRuleHeader  */
	YYSYMBOL_derivationRuleBegin = 66,         /* derivationRuleBegin  */
	YYSYMBOL_derivationRuleOperand = 67,       /* derivationRuleOperand  */
	YYSYMBOL_bigstring = 68,                   /* bigstring  */
	YYSYMBOL_semicolon = 69,                   /* semicolon  */
	YYSYMBOL_openparenthesis = 70,             /* openparenthesis  */
	YYSYMBOL_closeparenthesis = 71             /* closeparenthesis  */
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
#define YYLAST 209

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 34
/* YYNRULES -- Number of rules.  */
#define YYNRULES 101
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 147

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK 278

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                                                               \
	(0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] = {
    0,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 26, 27, 2, 36, 28, 2, 35, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  34, 37, 29, 30,
    31, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  33, 2,
    32, 2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  25,
    2,  24, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 2, 2,  2,  2, 2,  2,  2, 2,  2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2, 2, 2, 2, 2, 2, 1, 2,  3,  4, 5,  6,  7, 8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] = {
    0,    142,  142,  146,  153,  157,  161,  165,  169,  173,  177,  181,  185,  189,  193,  197,  201,
    205,  209,  215,  233,  234,  235,  240,  260,  265,  307,  391,  412,  422,  466,  514,  656,  659,
    666,  676,  690,  693,  713,  735,  753,  774,  923,  926,  941,  944,  951,  954,  971,  974,  984,
    1001, 1004, 1012, 1015, 1023, 1027, 1031, 1035, 1039, 1043, 1047, 1051, 1055, 1059, 1063, 1071, 1074,
    1082, 1085, 1089, 1093, 1098, 1110, 1116, 1135, 1150, 1261, 1265, 1308, 1315, 1326, 1339, 1353, 1364,
    1378, 1389, 1400, 1412, 1424, 1435, 1447, 1454, 1457, 1458, 1462, 1469, 1475, 1476, 1480, 1488, 1494};
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
				      "COMMENT",
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
				      "label",
				      "comments",
				      "labelOrComments",
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

#define YYPACT_NINF (-87)

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-97)

#define yytable_value_is_error(Yyn) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
    -87, 9,   27,  -87, -87, 181, -87, -87, 77,  -87, 0,   -87, -87, 168, -87, -87, 37,  -87, 24,  -87, 186,
    130, 39,  -87, -87, -87, 170, -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, 51,  -87, -87, -87,
    -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, -87, 72,  -87, 44,  -87, 171, 130, 130, 130,
    -87, -1,  -87, 130, 16,  58,  16,  -87, 130, 72,  16,  130, 130, 52,  24,  -87, 5,   -87, 24,  -87, -9,
    85,  24,  78,  80,  81,  83,  52,  75,  -87, -87, 52,  17,  -87, -87, 68,  -87, 52,  -87, -87, -87, 130,
    -5,  -87, -87, 90,  -87, -87, -87, -87, -87, 52,  52,  -87, 101, 89,  -87, -3,  -87, 151, 6,   -3,  110,
    -87, 107, -87, -87, 131, -87, -87, 29,  -87, -87, -87, 52,  109, -87, -87, 106, 108, 124, -87, -87, -87};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] = {
    42, 0,  20, 1,  43, 0,  22, 21, 0,  24, 19, 28,  25, 53, 47,  52, 0,  54, 92, 46, 0,  0,  93, 23, 46,
    30, 53, 55, 56, 63, 64, 57, 58, 59, 60, 61, 62,  65, 66, 4,   3,  5,  6,  7,  8,  9,  10, 11, 12, 13,
    14, 15, 16, 17, 18, 48, 2,  94, 29, 53, 0,  0,   0,  49, 32,  95, 0,  68, 0,  68, 50, 0,  48, 68, 0,
    0,  0,  92, 70, 0,  67, 92, 35, 0,  36, 92, 0,   96, 69, 100, 79, 77, 86, 91, 0,  85, 88, 74, 87, 36,
    0,  73, 36, 33, 0,  48, 36, 72, 97, 82, 71, 101, 76, 83, 0,   0,  89, 0,  46, 75, 44, 34, 0,  0,  44,
    98, 84, 78, 81, 90, 27, 45, 41, 0,  31, 26, 99,  0,  0,  39,  80, 0,  0,  0,  38, 37, 40};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-87, -21, 34,  -87, -87, -87, -87, -87, -87, -87, -87, -20,
				      -8,  -87, 33,  -7,  -70, -87, -87, -87, -87, -25, -87, -87,
				      84,  -87, -87, -87, -87, -86, -87, -47, -87, -87};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] = {0,  95, 56, 1,  5,  7,  8,  24, 9,  72,  83, 105, 12, 2,  132, 10,  64,
					 16, 20, 38, 62, 77, 78, 79, 96, 89, 127, 90, 91,  97, 98, 23,  109, 112};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] = {
    55,  13,  84,  63,  113, 131, 70,  14,  116, 3,   -51, 25,  26,  70,  119, 15,  58,  59,  103, 104, -96,
    -96, -96, -96, 122, 71,  122, -96, 126, 128, 99,  134, -96, 100, 102, 123, 4,   101, 106, 67,  68,  69,
    74,  108, 81,  73,  75,  21,  85,  76,  82,  140, -96, 86,  87,  39,  40,  92,  93,  138, 139, 22,  41,
    42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  57,  61,  11,  118, 63,  65,  120, 121,
    -46, 80,  124, 94,  -46, -46, -46, -46, 70,  -46, -46, -46, -46, -46, -46, -46, -46, -46, -46, 114, 117,
    107, 108, 129, 110, 115, 111, 130, 39,  40,  141, 142, 125, 143, 122, 41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  39,  40,  137, 136, 144, 14,  145, 41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  39,  146, 133, 135, 0,   88,  0,   41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  14,  0,   14,  14,  0,   0,   -46, 6,   0,   17,  0,   17,  17,  -46,
    0,   0,   -46, 18,  19,  60,  66,  -46, 27,  28,  29,  30,  0,   0,   31,  32,  33,  34,  35,  36,  37};

static const yytype_int16 yycheck[] = {
    21, 8,  72, 8,   90,  8,   7,  7,  94, 0,  10,  19,  19,  7,   100, 15,  24, 24, 27, 28, 3,  4,   5,  6,
    29, 26, 29, 10,  114, 115, 77, 25, 15, 28, 81,  105, 9,   32,  85,  60,  61, 62, 26, 26, 69, 66,  30, 10,
    73, 33, 71, 137, 35,  74,  75, 3,  4,  5,  6,   30,  31,  37,  10,  11,  12, 13, 14, 15, 16, 17,  18, 19,
    20, 21, 22, 23,  37,  26,  1,  99, 8,  37, 102, 104, 7,   27,  106, 35,  11, 12, 13, 14, 7,  16,  17, 18,
    19, 20, 21, 22,  23,  24,  25, 28, 36, 27, 26,  6,   27,  34,  27,  118, 3,  4,  5,  6,  26, 138, 29, 10,
    11, 12, 13, 14,  15,  16,  17, 18, 19, 20, 21,  22,  23,  3,   4,   28,  26, 31, 7,  31, 10, 11,  12, 13,
    14, 15, 16, 17,  18,  19,  20, 21, 22, 23, 3,   31,  122, 124, -1,  75,  -1, 10, 11, 12, 13, 14,  15, 16,
    17, 18, 19, 20,  21,  22,  23, 7,  -1, 7,  7,   -1,  -1,  0,   1,   -1,  16, -1, 16, 16, 7,  -1,  -1, 10,
    24, 25, 24, 24,  15,  11,  12, 13, 14, -1, -1,  17,  18,  19,  20,  21,  22, 23};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] = {
    0,  41, 51, 0,  9,  42, 1,  43, 44, 46, 53, 1,  50, 53, 7,  15, 55, 16, 24, 25, 56, 10, 37, 69, 45,
    50, 53, 11, 12, 13, 14, 17, 18, 19, 20, 21, 22, 23, 57, 3,  4,  10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 39, 40, 37, 50, 53, 24, 26, 58, 8,  54, 37, 24, 39, 39, 39, 7,  26, 47, 39, 26,
    30, 33, 59, 60, 61, 27, 59, 39, 48, 54, 59, 39, 39, 62, 63, 65, 66, 5,  6,  35, 39, 62, 67, 68, 69,
    28, 32, 69, 27, 28, 49, 69, 27, 26, 70, 27, 27, 71, 67, 28, 34, 67, 36, 49, 67, 49, 39, 29, 54, 49,
    26, 67, 64, 67, 6,  53, 8,  52, 40, 25, 52, 26, 28, 30, 31, 67, 5,  6,  39, 31, 31, 31};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] = {0,  38, 39, 39, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 41, 42,
				   42, 42, 43, 44, 44, 44, 44, 44, 45, 45, 46, 47, 47, 48, 48, 49, 49, 49, 49, 49, 50,
				   51, 51, 52, 52, 53, 53, 54, 54, 54, 55, 55, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57,
				   57, 57, 57, 58, 58, 59, 59, 59, 59, 59, 60, 61, 61, 62, 63, 63, 63, 64, 64, 65, 66,
				   66, 67, 67, 67, 67, 67, 68, 68, 69, 69, 69, 69, 70, 70, 70, 70, 71, 71};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = {0,  2,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 2, 2, 4, 1, 2,
				   11, 10, 2, 2, 1, 10, 0, 3, 3, 1, 0, 6, 6, 4, 6, 9, 0, 2, 0, 1, 0, 2, 0, 1, 2, 0,
				   1,  0,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 0, 3, 0, 2, 1, 3, 3, 2, 2, 3, 2, 1,
				   3,  1,  3, 1, 2, 2,  3, 1, 1, 1, 1, 2, 3, 1, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1};

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
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1048 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_EXTENDEDIDENTIFIER: /* EXTENDEDIDENTIFIER  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1054 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_STRINGLITTERAL: /* STRINGLITTERAL  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1060 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_COMMENT: /* COMMENT  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1066 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_LABEL: /* LABEL  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1072 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_APPLICATIONID: /* APPLICATIONID  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1078 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_IDENTIFIER: /* IDENTIFIER  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1084 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_SIMPLEIDENTIFIER: /* SIMPLEIDENTIFIER  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1090 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_oaAttributeArrayDeclaration: /* oaAttributeArrayDeclaration  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).oaAttributes) != NULL)
			delete ((*yyvaluep).oaAttributes);
		((*yyvaluep).oaAttributes) = NULL;
	}
#line 1096 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_keyFields: /* keyFields  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1102 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_fieldList: /* fieldList  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1108 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_metaData: /* metaData  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwmdMetaData) != NULL)
			delete ((*yyvaluep).kwmdMetaData);
		((*yyvaluep).kwmdMetaData) = NULL;
	}
#line 1114 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_kwattributeDeclaration: /* kwattributeDeclaration  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwaValue) != NULL)
			delete ((*yyvaluep).kwaValue);
		((*yyvaluep).kwaValue) = NULL;
	}
#line 1120 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_applicationids: /* applicationids  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1126 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_label: /* label  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1132 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_comments: /* comments  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1138 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_labelOrComments: /* labelOrComments  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).svValue) != NULL)
			delete ((*yyvaluep).svValue);
		((*yyvaluep).svValue) = NULL;
	}
#line 1144 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_refIdentifier: /* refIdentifier  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1150 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_usedDerivationRule: /* usedDerivationRule  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1156 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRule: /* referenceRule  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1162 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_referenceRuleBody: /* referenceRuleBody  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1168 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRule: /* derivationRule  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1174 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBody: /* derivationRuleBody  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1180 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_operandList: /* operandList  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).oaOperands) != NULL)
			delete ((*yyvaluep).oaOperands);
		((*yyvaluep).oaOperands) = NULL;
	}
#line 1186 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleHeader: /* derivationRuleHeader  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1192 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleBegin: /* derivationRuleBegin  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdrValue) != NULL)
			delete ((*yyvaluep).kwdrValue);
		((*yyvaluep).kwdrValue) = NULL;
	}
#line 1198 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_derivationRuleOperand: /* derivationRuleOperand  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).kwdroValue) != NULL)
			delete ((*yyvaluep).kwdroValue);
		((*yyvaluep).kwdroValue) = NULL;
	}
#line 1204 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case YYSYMBOL_bigstring: /* bigstring  */
#line 133 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		if (((*yyvaluep).sValue) != NULL)
			delete ((*yyvaluep).sValue);
		((*yyvaluep).sValue) = NULL;
	}
#line 1210 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
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
#line 143 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1482 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 3: /* IDENTIFIER: EXTENDEDIDENTIFIER  */
#line 147 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1490 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 4: /* SIMPLEIDENTIFIER: BASICIDENTIFIER  */
#line 154 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 1498 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 5: /* SIMPLEIDENTIFIER: CLASS  */
#line 158 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Dictionary");
	}
#line 1506 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 6: /* SIMPLEIDENTIFIER: CONTINUOUSTYPE  */
#line 162 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Numerical");
	}
#line 1514 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 7: /* SIMPLEIDENTIFIER: SYMBOLTYPE  */
#line 166 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Categorical");
	}
#line 1522 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 8: /* SIMPLEIDENTIFIER: OBJECTTYPE  */
#line 170 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Entity");
	}
#line 1530 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 9: /* SIMPLEIDENTIFIER: OBJECTARRAYTYPE  */
#line 174 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Table");
	}
#line 1538 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 10: /* SIMPLEIDENTIFIER: ROOT  */
#line 178 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Root");
	}
#line 1546 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 11: /* SIMPLEIDENTIFIER: UNUSED  */
#line 182 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Unused");
	}
#line 1554 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 12: /* SIMPLEIDENTIFIER: DATETYPE  */
#line 186 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Date");
	}
#line 1562 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 13: /* SIMPLEIDENTIFIER: TIMETYPE  */
#line 190 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Time");
	}
#line 1570 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 14: /* SIMPLEIDENTIFIER: TIMESTAMPTYPE  */
#line 194 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Timestamp");
	}
#line 1578 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 15: /* SIMPLEIDENTIFIER: TIMESTAMPTZTYPE  */
#line 198 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TimestampTZ");
	}
#line 1586 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 16: /* SIMPLEIDENTIFIER: TEXTTYPE  */
#line 202 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Text");
	}
#line 1594 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 17: /* SIMPLEIDENTIFIER: TEXTLISTTYPE  */
#line 206 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("TextList");
	}
#line 1602 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 18: /* SIMPLEIDENTIFIER: STRUCTURETYPE  */
#line 210 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = new ALString("Structure");
	}
#line 1610 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 19: /* kwclassFile: applicationids kwclasses comments  */
#line 216 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sId = (yyvsp[-2].sValue);
		StringVector* svComments = (yyvsp[0].svValue);

		// On ignore l'identification d'application
		if (sId != NULL)
			delete sId;

		// On interdit les commentaires en fin de fichier
		if (svComments != NULL)
		{
			delete svComments;
			yyerrorWithLineCorrection("Comments at the end of the file are not allowed", -1);
		}
	}
#line 1630 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 22: /* kwclasses: kwclasses error  */
#line 236 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Error outside the definition of a dictionary");
		YYABORT;
	}
#line 1637 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 23: /* kwclass: kwclassBegin comments '}' semicolon  */
#line 241 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-3].kwcValue);
		StringVector* svComments = (yyvsp[-2].svValue);

		// La completion des informations de type (CompleteTypeInfo) est centralisee
		// au niveau du domaine en fin de parsing

		// Commentaires internes
		if (svComments != NULL)
		{
			kwcClass->SetInternalComments(svComments);
			delete svComments;
		}

		// Reinitialisation de la classe courante
		kwcLoadCurrentClass = NULL;
	}
#line 1659 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 24: /* kwclassBegin: kwclassHeader  */
#line 261 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		assert(kwcLoadCurrentClass == (yyvsp[0].kwcValue));
		(yyval.kwcValue) = (yyvsp[0].kwcValue);
	}
#line 1668 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 25: /* kwclassBegin: kwclassBegin kwattributeDeclaration  */
#line 266 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-1].kwcValue);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		ALString sMessage;
		assert(kwcLoadCurrentClass == (yyvsp[-1].kwcValue));

		// Si attribut non valide: on ne fait rien
		if (attribute == NULL)
			;
		// Si classe non valide, supression de l'attribut
		else if (kwcClass == NULL)
			delete attribute;
		// Sinon, test de validite du nom de l'attribut
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
			delete attribute;
		}
		// Test de non existence parmi les attributs
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Test de non existence parmi les blocs
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Si OK, d'insertion
		else
		{
			kwcClass->InsertAttribute(attribute);
		}

		(yyval.kwcValue) = kwcClass;
	}
#line 1714 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 26: /* kwclassBegin: kwclassBegin comments '{' oaAttributeArrayDeclaration comments '}' IDENTIFIER usedDerivationRule semicolon metaData label  */
#line 308 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-10].kwcValue);
		StringVector* svComments = (yyvsp[-9].svValue);
		ObjectArray* oaAttributes = (yyvsp[-7].oaAttributes);
		StringVector* svInternalComments = (yyvsp[-6].svValue);
		ALString* sBlockName = (yyvsp[-4].sValue);
		KWDerivationRule* rule = (yyvsp[-3].kwdrValue);
		KWMetaData* metaData = (yyvsp[-1].kwmdMetaData);
		ALString* sLabel = (yyvsp[0].sValue);
		KWAttributeBlock* attributeBlock;
		KWAttribute* firstAttribute;
		KWAttribute* lastAttribute;
		ALString sMessage;
		assert(kwcLoadCurrentClass == (yyvsp[-10].kwcValue));
		check(oaAttributes);

		// Cas d'un bloc avec au moins un attribut valide
		if (oaAttributes->GetSize() > 0)
		{
			// Test de validite du nom de l'attribut
			if (!kwcClass->CheckNameWithMessage(*sBlockName, KWClass::AttributeBlock, sMessage))
			{
				yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
			}
			// Test de non existence parmi les attributs
			else if (kwcClass->LookupAttribute(*sBlockName) != NULL)
			{
				yyerrorWithLineCorrection(
				    "Dictionary " + kwcClass->GetName() +
					": Sparse variable block name already used by a variable (" + *sBlockName + ")",
				    -1);
			}
			// Test de non existence parmi les blocs
			else if (kwcClass->LookupAttributeBlock(*sBlockName) != NULL)
			{
				yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
							      ": Sparse variable block name already used by a block (" +
							      *sBlockName + ")",
							  -1);
			}
			// Creation du bloc dans la classe
			else
			{
				// Creation du bloc
				firstAttribute = cast(KWAttribute*, oaAttributes->GetAt(0));
				lastAttribute = cast(KWAttribute*, oaAttributes->GetAt(oaAttributes->GetSize() - 1));
				attributeBlock =
				    kwcClass->CreateAttributeBlock(*sBlockName, firstAttribute, lastAttribute);

				// Parametrage du bloc
				attributeBlock->SetDerivationRule(rule);
				if (metaData != NULL)
					attributeBlock->GetMetaData()->CopyFrom(metaData);
				if (sLabel != NULL)
					attributeBlock->SetLabel(*(sLabel));
				if (svComments != NULL)
					attributeBlock->SetComments(svComments);
				if (svInternalComments != NULL)
					attributeBlock->SetInternalComments(svInternalComments);

				// On marque la rule a NULL pour indiquer qu'elle est utilisee
				rule = NULL;
			}
		}

		// Destruction de l'eventuelle regle si non utilisee
		if (rule != NULL)
			delete rule;

		// Tous les attributs du tableau ont deja ete inseres dans la classe
		// On se contente de detruire le tableau
		delete oaAttributes;

		// Nettoyage
		if (svComments != NULL)
			delete svComments;
		if (svInternalComments != NULL)
			delete svInternalComments;
		if (sBlockName != NULL)
			delete sBlockName;
		if (metaData != NULL)
			delete metaData;
		if (sLabel != NULL)
			delete sLabel;

		(yyval.kwcValue) = kwcClass;
	}
#line 1802 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 27: /* kwclassBegin: kwclassBegin comments '{' comments '}' IDENTIFIER usedDerivationRule semicolon metaData comments  */
#line 392 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWClass* kwcClass = (yyvsp[-9].kwcValue);

		// Message d'erreur
		yyerror("Empty sparse variable block not allowed");

		// Nettoyage
		if ((yyvsp[-8].svValue) != NULL)
			delete (yyvsp[-8].svValue);
		if ((yyvsp[-6].svValue) != NULL)
			delete (yyvsp[-6].svValue);
		delete (yyvsp[-4].sValue);
		if ((yyvsp[-3].kwdrValue) != NULL)
			delete (yyvsp[-3].kwdrValue);
		if ((yyvsp[-1].kwmdMetaData) != NULL)
			delete (yyvsp[-1].kwmdMetaData);
		if ((yyvsp[0].svValue) != NULL)
			delete (yyvsp[0].svValue);
		(yyval.kwcValue) = kwcClass;
	}
#line 1827 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 28: /* kwclassBegin: kwclassBegin error  */
#line 413 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		// Attention: cette regle qui permet une gestion des erreurs amelioree
		// genere un conflit reduce/reduce
		kwcLoadCurrentClass = NULL;
		YYABORT;
	}
#line 1838 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 29: /* oaAttributeArrayDeclaration: oaAttributeArrayDeclaration kwattributeDeclaration  */
#line 423 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaAttributes = (yyvsp[-1].oaAttributes);
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		KWClass* kwcClass = kwcLoadCurrentClass;
		ALString sMessage;
		check(oaAttributes);

		// Si attribut non valide: on ne fait rien
		if (attribute == NULL)
			;
		// Si classe non valide, supression de l'attribut
		else if (kwcClass == NULL)
			delete attribute;
		// Sinon, test de validite du nom de l'attribut
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
			delete attribute;
		}
		// Test de non existence parmi les attributs
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Test de non existence parmi les blocs
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Si OK, d'insertion
		else
		{
			kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1886 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 30: /* oaAttributeArrayDeclaration: kwattributeDeclaration  */
#line 467 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWAttribute* attribute = (yyvsp[0].kwaValue);
		ObjectArray* oaAttributes;
		KWClass* kwcClass = kwcLoadCurrentClass;
		ALString sMessage;

		// Creation d'un tableau
		oaAttributes = new ObjectArray;

		// Si attribut non valide: on ne fait rien
		if (attribute == NULL)
			;
		// Si classe non valide, supression de l'attribut
		else if (kwcClass == NULL)
			delete attribute;
		// Sinon, test de validite du nom de l'attribut
		else if (!kwcClass->CheckNameWithMessage(attribute->GetName(), KWClass::Attribute, sMessage))
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() + ", " + sMessage, -1);
			delete attribute;
		}
		// Test de non existence parmi les attributs
		else if (kwcClass->LookupAttribute(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used (" + attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Test de non existence parmi les blocs
		else if (kwcClass->LookupAttributeBlock(attribute->GetName()) != NULL)
		{
			yyerrorWithLineCorrection("Dictionary " + kwcClass->GetName() +
						      ": Variable name already used by a block (" +
						      attribute->GetName() + ")",
						  -1);
			delete attribute;
		}
		// Si OK, d'insertion
		else
		{
			kwcClass->InsertAttribute(attribute);
			oaAttributes->Add(attribute);
		}

		(yyval.oaAttributes) = oaAttributes;
	}
#line 1936 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 31: /* kwclassHeader: comments rootDeclaration CLASS IDENTIFIER labelOrComments keyFields labelOrComments metaData labelOrComments '{'  */
#line 519 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svComments1 = (yyvsp[-9].svValue);
		boolean bRoot = (yyvsp[-8].bValue);
		ALString* sIdentifier = (yyvsp[-6].sValue);
		StringVector* svComments2 = (yyvsp[-5].svValue);
		StringVector* svKeyFields = (yyvsp[-4].svValue);
		StringVector* svComments3 = (yyvsp[-3].svValue);
		KWMetaData* metaData = (yyvsp[-2].kwmdMetaData);
		StringVector* svComments4 = (yyvsp[-1].svValue);
		KWClass* kwcClass;
		KWClass* kwcReferencedClass;
		StringVector svAllComments;
		StringVector svClassComments;
		int nCommentStartIndex;
		int i;
		ALString sMessage;

		// Test d'existence de la classe
		kwcClass = kwcdLoadDomain->LookupClass(*sIdentifier);

		// Test d'existence de la classe en tant que classe referencee uniquement
		kwcReferencedClass = cast(KWClass*, odReferencedUncreatedClasses->Lookup(*sIdentifier));
		assert(kwcReferencedClass == NULL or kwcClass == NULL);

		// Erreur si la classe existe deja
		if (kwcClass != NULL)
		{
			yyerror("Dictionary " + *sIdentifier + " already exists");
			kwcClass = NULL;
		}
		// On utilise la classe referencee si elle existe
		else if (kwcReferencedClass != NULL)
		{
			// Insertion dans le domaine
			kwcClass = kwcReferencedClass;
			kwcdLoadDomain->InsertClass(kwcClass);

			// Supression de la classe referencees
			odReferencedUncreatedClasses->RemoveKey(kwcReferencedClass->GetName());
			kwcReferencedClass = NULL;
		}
		// Sinon, on cree la classe et on l'enregistre
		else
		{
			// Test de nom de classe
			if (KWClass::CheckNameWithMessage(*sIdentifier, KWClass::Class, sMessage))
			{
				kwcClass = new KWClass;
				kwcClass->SetName(*sIdentifier);
				kwcdLoadDomain->InsertClass(kwcClass);
			}
			else
				yyerror(sMessage);
		}

		// Initialisation si necessaire de la classe
		if (kwcClass != NULL)
		{
			// Commentaire de la classe en cours
			svClassComments.CopyFrom(kwcClass->GetComments());

			// Classe racine
			kwcClass->SetRoot(bRoot);

			// Attribut key field
			if (svKeyFields != NULL)
			{
				// Transfert des champs de la cle
				svKeyFields = cast(StringVector*, svKeyFields);
				kwcClass->SetKeyAttributeNumber(svKeyFields->GetSize());
				for (i = 0; i < svKeyFields->GetSize(); i++)
					kwcClass->SetKeyAttributeNameAt(i, svKeyFields->GetAt(i));
			}

			// Meta-donnees de la classe
			if (metaData != NULL)
				kwcClass->GetMetaData()->CopyFrom(metaData);

			// On recupere les commentaires existants
			svAllComments.CopyFrom(kwcClass->GetComments());

			// On recupere tous les commentaires entre le debut et la fin de la declaration de la classe
			if (svComments1 != NULL)
			{
				for (i = 0; i < svComments1->GetSize(); i++)
					svAllComments.Add(svComments1->GetAt(i));
			}
			if (svComments2 != NULL)
			{
				for (i = 0; i < svComments2->GetSize(); i++)
					svAllComments.Add(svComments2->GetAt(i));
			}
			if (svComments3 != NULL)
			{
				for (i = 0; i < svComments3->GetSize(); i++)
					svAllComments.Add(svComments3->GetAt(i));
			}
			if (svComments4 != NULL)
			{
				for (i = 0; i < svComments4->GetSize(); i++)
					svAllComments.Add(svComments4->GetAt(i));
			}

			// Libelle de la classe: le premier des commentaires, sauf s'il existe deja
			nCommentStartIndex = 0;
			if (kwcClass->GetLabel() == "" and svAllComments.GetSize() > 0)
			{
				kwcClass->SetLabel(svAllComments.GetAt(0));
				nCommentStartIndex = 1;
			}

			// Mise a jour des commentaires de la classe, en excluant potentiellement le premier commentaire reserve au libelle
			if (svAllComments.GetSize() > 0)
			{
				for (i = nCommentStartIndex; i < svAllComments.GetSize(); i++)
					svClassComments.Add(svAllComments.GetAt(i));
				kwcClass->SetComments(&svClassComments);
			}
		}

		// Liberation des tokens
		delete sIdentifier;
		if (svKeyFields != NULL)
			delete svKeyFields;
		if (metaData != NULL)
			delete metaData;
		if (svComments1 != NULL)
			delete svComments1;
		if (svComments2 != NULL)
			delete svComments2;
		if (svComments3 != NULL)
			delete svComments3;
		if (svComments4 != NULL)
			delete svComments4;

		// Memorisation de la classe courante
		kwcLoadCurrentClass = kwcClass;
		(yyval.kwcValue) = kwcClass;
	}
#line 2074 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 32: /* keyFields: %empty  */
#line 656 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = NULL; // pas de champ cle
	}
#line 2082 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 33: /* keyFields: '(' fieldList ')'  */
#line 660 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = (yyvsp[-1].svValue);
	}
#line 2090 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 34: /* fieldList: fieldList ',' IDENTIFIER  */
#line 667 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svFields = (yyvsp[-2].svValue);
		ALString* sIdentifier = (yyvsp[0].sValue);

		// Ajout d'un nouveau de champ
		svFields->Add(*sIdentifier);
		delete sIdentifier;
		(yyval.svValue) = svFields;
	}
#line 2104 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 35: /* fieldList: IDENTIFIER  */
#line 677 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sIdentifier = (yyvsp[0].sValue);
		StringVector* svFields;

		// Creation d'un tableau de champs, avec un premier champ
		svFields = new StringVector;
		svFields->Add(*sIdentifier);
		delete sIdentifier;
		(yyval.svValue) = svFields;
	}
#line 2119 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 36: /* metaData: %empty  */
#line 690 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwmdMetaData) = NULL; // pas de paires cle valeurs
	}
#line 2127 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 37: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' STRINGLITTERAL '>'  */
#line 694 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWMetaData* metaData = (yyvsp[-5].kwmdMetaData);
		ALString* sKey = (yyvsp[-3].sValue);
		ALString* sValue = (yyvsp[-1].sValue);

		// Creation si necessaire d'une ensemble de paires cles valeur
		if (metaData == NULL)
			metaData = new KWMetaData;

		// Erreur si cle deja existante
		if (metaData->IsKeyPresent(*sKey))
			yyerror("Duplicate key in meta-data for key " + *(sKey));
		// Insertion d'une paire avec valeur chaine de caracteres sinon
		else
			metaData->SetStringValueAt(*(sKey), *(sValue));
		delete sKey;
		delete sValue;
		(yyval.kwmdMetaData) = metaData;
	}
#line 2151 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 38: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' CONTINUOUSLITTERAL '>'  */
#line 714 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWMetaData* metaData = (yyvsp[-5].kwmdMetaData);
		ALString* sKey = (yyvsp[-3].sValue);
		Continuous cValue = (yyvsp[-1].cValue);

		// Creation si necessaire d'une ensemble de paires cles valeur
		if (metaData == NULL)
			metaData = new KWMetaData;

		// Erreur si cle deja existante
		if (metaData->IsKeyPresent(*sKey))
			yyerror("Duplicate key in meta-data for key " + *(sKey));
		// Erreur si valeur Missing
		else if (cValue == KWContinuous::GetMissingValue())
			yyerror("Missing value not allowed in meta-data for key " + *(sKey));
		// Insertion d'une paire avec valeur numerique sinon
		else
			metaData->SetDoubleValueAt(*(sKey), cValue);
		delete sKey;
		(yyval.kwmdMetaData) = metaData;
	}
#line 2177 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 39: /* metaData: metaData '<' SIMPLEIDENTIFIER '>'  */
#line 736 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWMetaData* metaData = (yyvsp[-3].kwmdMetaData);
		ALString* sKey = (yyvsp[-1].sValue);

		// Creation si necessaire d'une ensemble de paires cles valeur
		if (metaData == NULL)
			metaData = new KWMetaData;

		// Erreur si cle deja existante
		if (metaData->IsKeyPresent(*sKey))
			yyerror("Duplicate key in meta-data for key " + *(sKey));
		// Insertion d'une paire avec valeur numerique sinon
		else
			metaData->SetNoValueAt(*(sKey));
		delete sKey;
		(yyval.kwmdMetaData) = metaData;
	}
#line 2199 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 40: /* metaData: metaData '<' SIMPLEIDENTIFIER '=' IDENTIFIER '>'  */
#line 754 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWMetaData* metaData = (yyvsp[-5].kwmdMetaData);
		ALString* sKey = (yyvsp[-3].sValue);
		ALString* sValue = (yyvsp[-1].sValue);

		// Creation si necessaire d'une ensemble de paires cles valeur
		if (metaData == NULL)
			metaData = new KWMetaData;

		// Erreur car la valeur n'est pas du bon type
		yyerror("Value (" + *(sValue) + ") of meta-data for key " + *(sKey) +
			" should be a string value between double quotes");
		delete sKey;
		delete sValue;
		(yyval.kwmdMetaData) = metaData;
	}
#line 2219 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 41: /* kwattributeDeclaration: comments usedDeclaration typeDeclaration refIdentifier IDENTIFIER usedDerivationRule semicolon metaData label  */
#line 783 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svComments = (yyvsp[-8].svValue);
		boolean bUsed = (yyvsp[-7].bValue);
		int nType = (yyvsp[-6].nValue);
		ALString* sRefIdentifier = (yyvsp[-5].sValue);
		ALString* sIdentifier = (yyvsp[-4].sValue);
		KWDerivationRule* rule = (yyvsp[-3].kwdrValue);
		KWMetaData* metaData = (yyvsp[-1].kwmdMetaData);
		ALString* slabel = (yyvsp[0].sValue);
		KWAttribute* attribute;

		// Creation  et initialisation d'un attribut
		attribute = new KWAttribute;
		attribute->SetUsed(bUsed);
		attribute->SetType(nType);

		// Test de coherence entre le type et le complement de type dans le cas d'un type relation
		if (KWType::IsRelation(attribute->GetType()))
		{
			if (sRefIdentifier == NULL)
				yyerrorWithLineCorrection("Variable " + *sIdentifier + " of type " +
							      KWType::ToString(nType) + ": missing " +
							      KWType::ToString(nType) + " dictionary",
							  -1);
		}
		// Test de coherence entre le type et le complement de type dans le cas d'un type Structure
		else if (attribute->GetType() == KWType::Structure)
		{
			if (sRefIdentifier == NULL)
				yyerrorWithLineCorrection("Variable " + *sIdentifier + " of type " +
							      KWType::ToString(nType) + ": missing " +
							      KWType::ToString(nType) + " dictionary",
							  -1);
		}
		// Test d'absence de complement de type dans les autres cas
		else
		{
			if (sRefIdentifier != NULL)
				yyerrorWithLineCorrection("Variable " + *sIdentifier + " of type " +
							      KWType::ToString(nType) + ": erroneous (" +
							      *(sRefIdentifier) + ") type complement",
							  -1);
		}

		// Classe referencee
		if (KWType::IsRelation(attribute->GetType()))
		{
			KWClass* kwcReferencedClass = NULL;

			// Test d'existence de la classe
			if (sRefIdentifier != NULL)
				kwcReferencedClass = kwcdLoadDomain->LookupClass(*sRefIdentifier);

			// Sinon, test d'existence de la classe en tant que classe referencee uniquement
			if (kwcReferencedClass == NULL and sRefIdentifier != NULL)
				kwcReferencedClass =
				    cast(KWClass*, odReferencedUncreatedClasses->Lookup(*sRefIdentifier));

			// Si la classe n'existe pas, on essaie de la creer
			if (kwcReferencedClass == NULL and sRefIdentifier != NULL)
			{
				// Test de nom de classe
				if (KWClass::CheckName(*sRefIdentifier, KWClass::Class, NULL))
				{
					kwcReferencedClass = new KWClass;
					kwcReferencedClass->SetName(*sRefIdentifier);

					// Memorisation dans le dictionnaire des classe referencees
					odReferencedUncreatedClasses->SetAt(kwcReferencedClass->GetName(),
									    kwcReferencedClass);
				}
				else
					yyerrorWithLineCorrection(
					    "Incorrect referenced dictionary name (" + *sRefIdentifier + ")", -1);
			}

			// On memorise la classe referencee
			attribute->SetClass(kwcReferencedClass);
		}
		// Structure referencee
		else if (attribute->GetType() == KWType::Structure)
		{
			if (sRefIdentifier != NULL)
				attribute->SetStructureName(*sRefIdentifier);
		}
		if (sRefIdentifier != NULL)
			delete sRefIdentifier;

		// Nom de l'attribut
		attribute->SetName(*sIdentifier);
		delete sIdentifier;
		attribute->SetDerivationRule(rule);

		// Completion eventuelle de la regle par les infos de type de l'attribut
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

		// Meta-donnees de l'attribut
		if (metaData != NULL)
		{
			attribute->GetMetaData()->CopyFrom(metaData);
			delete metaData;
		}

		// Libelle
		if (slabel != NULL)
		{
			attribute->SetLabel(*slabel);
			delete slabel;
		}

		// Commentaires
		if (svComments != NULL)
		{
			attribute->SetComments(svComments);
			delete svComments;
		}

		(yyval.kwaValue) = attribute;
	}
#line 2359 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 42: /* applicationids: %empty  */
#line 923 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; // pas d'identification d'application
	}
#line 2367 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 43: /* applicationids: applicationids APPLICATIONID  */
#line 927 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		// On ne garde que la premiere ligne de chaque identification d'application
		if ((yyvsp[-1].sValue) == NULL)
			(yyval.sValue) = (yyvsp[0].sValue);
		else
		{
			delete (yyvsp[0].sValue);
			(yyval.sValue) = (yyvsp[-1].sValue);
		}
	}
#line 2382 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 44: /* label: %empty  */
#line 941 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL; // pas de libelle
	}
#line 2390 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 45: /* label: LABEL  */
#line 945 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 2398 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 46: /* comments: %empty  */
#line 951 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = NULL; // pas de commentaire
	}
#line 2406 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 47: /* comments: comments COMMENT  */
#line 955 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svComments = (yyvsp[-1].svValue);
		ALString* sComment = (yyvsp[0].sValue);

		// Creation du vecteur de commentaires si neccesaire
		if (svComments == NULL)
			svComments = new StringVector;

		// Ajout du commentaire
		svComments->Add(*sComment);
		delete sComment;
		(yyval.svValue) = svComments;
	}
#line 2424 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 48: /* labelOrComments: %empty  */
#line 971 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.svValue) = NULL; // pas de commentaire
	}
#line 2432 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 49: /* labelOrComments: LABEL  */
#line 975 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sComment = (yyvsp[0].sValue);
		StringVector* svComments;

		svComments = new StringVector;
		svComments->Add(*sComment);
		delete sComment;
		(yyval.svValue) = svComments;
	}
#line 2446 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 50: /* labelOrComments: labelOrComments COMMENT  */
#line 985 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		StringVector* svComments = (yyvsp[-1].svValue);
		ALString* sComment = (yyvsp[0].sValue);

		// Creation du vecteur de commentaires si neccesaire
		if (svComments == NULL)
			svComments = new StringVector;

		// Ajout du commentaire
		svComments->Add(*sComment);
		delete sComment;
		(yyval.svValue) = svComments;
	}
#line 2464 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 51: /* rootDeclaration: %empty  */
#line 1001 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false; // valeur par defaut
	}
#line 2472 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 52: /* rootDeclaration: ROOT  */
#line 1005 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true;
	}
#line 2480 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 53: /* usedDeclaration: %empty  */
#line 1012 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = true; // valeur par defaut
	}
#line 2488 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 54: /* usedDeclaration: UNUSED  */
#line 1016 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.bValue) = false;
	}
#line 2496 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 55: /* typeDeclaration: CONTINUOUSTYPE  */
#line 1024 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Continuous;
	}
#line 2504 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 56: /* typeDeclaration: SYMBOLTYPE  */
#line 1028 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Symbol;
	}
#line 2512 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 57: /* typeDeclaration: DATETYPE  */
#line 1032 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Date;
	}
#line 2520 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 58: /* typeDeclaration: TIMETYPE  */
#line 1036 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Time;
	}
#line 2528 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 59: /* typeDeclaration: TIMESTAMPTYPE  */
#line 1040 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Timestamp;
	}
#line 2536 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 60: /* typeDeclaration: TIMESTAMPTZTYPE  */
#line 1044 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TimestampTZ;
	}
#line 2544 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 61: /* typeDeclaration: TEXTTYPE  */
#line 1048 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Text;
	}
#line 2552 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 62: /* typeDeclaration: TEXTLISTTYPE  */
#line 1052 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::TextList;
	}
#line 2560 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 63: /* typeDeclaration: OBJECTTYPE  */
#line 1056 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Object;
	}
#line 2568 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 64: /* typeDeclaration: OBJECTARRAYTYPE  */
#line 1060 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::ObjectArray;
	}
#line 2576 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 65: /* typeDeclaration: STRUCTURETYPE  */
#line 1064 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.nValue) = KWType::Structure;
	}
#line 2584 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 66: /* refIdentifier: %empty  */
#line 1071 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = NULL;
	}
#line 2592 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 67: /* refIdentifier: '(' IDENTIFIER ')'  */
#line 1075 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[-1].sValue);
	}
#line 2600 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 68: /* usedDerivationRule: %empty  */
#line 1082 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = NULL;
	}
#line 2608 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 69: /* usedDerivationRule: '=' derivationRule  */
#line 1086 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2616 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 70: /* usedDerivationRule: referenceRule  */
#line 1090 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2624 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 71: /* usedDerivationRule: '=' derivationRule ')'  */
#line 1094 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ')'");
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2633 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 72: /* usedDerivationRule: '(' IDENTIFIER ')'  */
#line 1099 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sIdentifier = (yyvsp[-1].sValue);
		ALString sTmp;

		yyerror(sTmp + "Invalid syntax (" + *sIdentifier + ")");
		if (sIdentifier != NULL)
			delete sIdentifier;
		(yyval.kwdrValue) = NULL;
	}
#line 2647 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 73: /* referenceRule: referenceRuleBody ']'  */
#line 1111 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[-1].kwdrValue);
	}
#line 2655 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 74: /* referenceRuleBody: '[' derivationRuleOperand  */
#line 1117 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);
		KWDerivationRule* rule;

		// Construction d'une regle pour accueillir les specifications
		rule = KWDerivationRule::CloneDerivationRule(KWDerivationRule::GetReferenceRuleName());

		// Destruction des operandes
		rule->DeleteAllOperands();

		// Ajout d'un premier operande: le premier champ de la cle de reference
		if (operand->GetType() == KWType::Unknown)
			operand->SetType(KWType::Symbol);
		rule->AddOperand(operand);

		// On retourner la regle
		(yyval.kwdrValue) = rule;
	}
#line 2678 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 75: /* referenceRuleBody: referenceRuleBody ',' derivationRuleOperand  */
#line 1136 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		// Ajout d'un autre operande: un autre champ de la cle de reference
		if (operand->GetType() == KWType::Unknown)
			operand->SetType(KWType::Symbol);
		rule->AddOperand(operand);

		// On retourner la regle
		(yyval.kwdrValue) = rule;
	}
#line 2695 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 76: /* derivationRule: derivationRuleBody closeparenthesis  */
#line 1151 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* ruleBody = (yyvsp[-1].kwdrValue);
		boolean bOk = true;
		KWDerivationRule* rule;
		ALString sTmp;

		// Recherche de la regle de reference
		// On ensuite recuperer au maximum les informations de la regle clonee
		// et rapatrier les informations issues du parsing concernant les operandes
		check(ruleBody);
		rule = KWDerivationRule::CloneDerivationRule(ruleBody->GetName());

		// Erreur si regle inexistante
		if (rule == NULL)
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
			bOk = false;
		}
		// Erreur si regle predefinie de Reference
		else if (rule->GetName() == KWDerivationRule::GetReferenceRuleName())
		{
			yyerror("Unknown derivation rule '" + ruleBody->GetName() + "'");
			bOk = false;
		}

		// Import des operandes de la regle
		if (bOk)
			bOk = ImportParserRuleOperands(ruleBody, rule);

		// Gestion des operandes en sortie dans le cas ou le parser a stocke des operandes en sortie
		if (bOk and KWType::IsRelation(ruleBody->GetType()) and not ruleBody->GetReference())
		{
			// Erreur si la regle en cours n'est pas une regle de creation d'instance
			if (not KWType::IsRelation(rule->GetType()) or rule->GetReference())
			{
				yyerror(sTmp + "Derivation rule " + rule->GetName() +
					" does not accept output operands");
				bOk = false;
			}
			// Sinon, transfert des operandes en sortie
			else
			{
				// Import des operandes en sortie de la regle
				// On est passe prealablement dans le parser par une regle de creation de relation
				// pour stocker les operandes en sortie, que l'on va ici exploiter
				bOk = ImportParserRuleOutputOperands(ruleBody, rule);
			}
		}
		// Gestion des operandes en sortie dans le cas ou le parser n'a stocke des operandes en sortie
		else if (bOk and KWType::IsRelation(rule->GetType()) and not rule->GetReference())
		{
			// Test du nombre d'operandes en sortie
			if ((rule->GetVariableOutputOperandNumber() and rule->GetOutputOperandNumber() > 1) or
			    (not rule->GetVariableOutputOperandNumber() and rule->GetOutputOperandNumber() > 0))
			{
				yyerror(sTmp + "Missing output operands for rule " + rule->GetName() +
					" that requires them (at least " +
					IntToString(rule->GetOutputOperandNumber() -
						    (rule->GetVariableOutputOperandNumber() ? 1 : 0)) +
					")");
				bOk = false;
			}
			// Supression des eventuels operandes en sortie inutiles
			else if (rule->GetOutputOperandNumber() > 0)
				cast(KWDRRelationCreationRule*, rule)->DeleteAllOutputOperands();
		}

		// Verification de la definition de la regle
		if (bOk and not rule->CheckDefinition())
		{
			yyerror(sTmp + "Derivation rule " + rule->GetName() + " incorrectly specified");
			bOk = false;
		}

		// Test si erreur dans le transfert des operandes
		if (not bOk)
		{
			if (rule != NULL)
			{
				delete rule;
				rule = NULL;
			}
		}
		// Sinon, on tente de compresser la regle
		else
		{
			if (rule->IsStructureRule())
			{
				KWDRStructureRule* structureRule;

				// Acces a la regle de structure, transformation au format structure et nettoyage memoire
				// Cette optimisation memoire des regles structure est critique dans le cas de dictionnaires
				// de tres grande taille. Sinon, des millions d'operandes de regles sont potentiellement crees,
				// puis lors de la compilation des dictionnaires, l'essentiel de la memoire liberee laisse des trous
				// dans les segments de la heap, qui ne peuvent etre rendus au systeme
				assert(rule->CheckDefinition());
				structureRule = cast(KWDRStructureRule*, rule);
				if (structureRule->CheckConstantOperands(false))
				{
					structureRule->BuildStructureFromBase(rule);
					structureRule->CleanCompiledBaseInterface();
				}
			}
		}

		// Finalisation
		delete ruleBody;
		(yyval.kwdrValue) = rule;
	}
#line 2807 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 77: /* derivationRuleBody: derivationRuleBegin  */
#line 1262 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2815 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 78: /* derivationRuleBody: derivationRuleBegin ':' operandList  */
#line 1266 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* ruleBody = (yyvsp[-2].kwdrValue);
		ObjectArray* oaOutputOperands = (yyvsp[0].oaOperands);
		KWDRRelationCreationRule* ruleRelationCreationBody;
		ObjectArray oaOperands;
		int nOperand;
		KWDerivationRuleOperand* operand;

		// On passe par une regle de creation de relation pour stocker les operandes en sortie
		ruleRelationCreationBody = new KWDRRelationCreationRule;

		// On transfer les operande initiaux vers un tableau
		for (nOperand = 0; nOperand < ruleBody->GetOperandNumber(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, ruleBody->GetOperandAt(nOperand));
			oaOperands.Add(operand);
		}
		ruleBody->RemoveAllOperands();

		// On copie la regle initiale, maintenant nettoyee de ses operandes
		ruleBody->SetType(KWType::ObjectArray);
		ruleRelationCreationBody->CopyFrom(ruleBody);
		delete ruleBody;

		// On recupere les operandes initiaux
		for (nOperand = 0; nOperand < oaOperands.GetSize(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(nOperand));
			ruleRelationCreationBody->AddOperand(operand);
		}

		// Parametrage des operandes en sortie
		assert(ruleRelationCreationBody->GetOutputOperandNumber() == 0);
		for (nOperand = 0; nOperand < oaOutputOperands->GetSize(); nOperand++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOutputOperands->GetAt(nOperand));
			ruleRelationCreationBody->AddOutputOperand(operand);
		}
		delete oaOutputOperands;

		(yyval.kwdrValue) = ruleRelationCreationBody;
	}
#line 2862 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 79: /* derivationRuleBody: derivationRuleHeader  */
#line 1309 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.kwdrValue) = (yyvsp[0].kwdrValue);
	}
#line 2870 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 80: /* operandList: operandList ',' derivationRuleOperand  */
#line 1316 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ObjectArray* oaOperandList = (yyvsp[-2].oaOperands);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);
		check(oaOperandList);
		check(operand);

		// Ajout d'un operande
		oaOperandList->Add(operand);
		(yyval.oaOperands) = oaOperandList;
	}
#line 2885 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 81: /* operandList: derivationRuleOperand  */
#line 1327 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);
		ObjectArray* oaOperandList;
		check(operand);

		// Creation d'un tableau doperandes, avec un premier operande
		oaOperandList = new ObjectArray;
		oaOperandList->Add(operand);
		(yyval.oaOperands) = oaOperandList;
	}
#line 2900 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 82: /* derivationRuleHeader: IDENTIFIER openparenthesis  */
#line 1340 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sIdentifier = (yyvsp[-1].sValue);
		KWDerivationRule* rule;

		// Construction d'une regle pour accueillir les specification
		rule = new KWDerivationRule;
		rule->SetName(*sIdentifier);
		delete sIdentifier;
		(yyval.kwdrValue) = rule;
	}
#line 2915 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 83: /* derivationRuleBegin: derivationRuleHeader derivationRuleOperand  */
#line 1354 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-1].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() == 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2930 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 84: /* derivationRuleBegin: derivationRuleBegin ',' derivationRuleOperand  */
#line 1365 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[-2].kwdrValue);
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		check(rule);
		assert(rule->GetOperandNumber() > 0);
		check(operand);
		rule->AddOperand(operand);
		(yyval.kwdrValue) = rule;
	}
#line 2945 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 85: /* derivationRuleOperand: IDENTIFIER  */
#line 1379 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sIdentifier = (yyvsp[0].sValue);
		KWDerivationRuleOperand* operand;

		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetDataItemName(*sIdentifier);
		delete sIdentifier;
		(yyval.kwdroValue) = operand;
	}
#line 2960 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 86: /* derivationRuleOperand: CONTINUOUSLITTERAL  */
#line 1390 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		Continuous cValue = (yyvsp[0].cValue);
		KWDerivationRuleOperand* operand;

		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Continuous);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetContinuousConstant(cValue);
		(yyval.kwdroValue) = operand;
	}
#line 2975 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 87: /* derivationRuleOperand: bigstring  */
#line 1401 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sValue = (yyvsp[0].sValue);
		KWDerivationRuleOperand* operand;

		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Symbol);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetSymbolConstant(Symbol(*sValue));
		delete sValue;
		(yyval.kwdroValue) = operand;
	}
#line 2991 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 88: /* derivationRuleOperand: derivationRule  */
#line 1413 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRule* rule = (yyvsp[0].kwdrValue);
		KWDerivationRuleOperand* operand;

		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(rule);
		if (operand->GetDerivationRule() != NULL)
			operand->SetType(operand->GetDerivationRule()->GetType());
		(yyval.kwdroValue) = operand;
	}
#line 3007 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 89: /* derivationRuleOperand: '.' derivationRuleOperand  */
#line 1425 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		KWDerivationRuleOperand* operand = (yyvsp[0].kwdroValue);

		operand->SetScopeLevel(operand->GetScopeLevel() + 1);
		(yyval.kwdroValue) = operand;
	}
#line 3018 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 90: /* bigstring: bigstring '+' STRINGLITTERAL  */
#line 1436 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		ALString* sValue1 = (yyvsp[-2].sValue);
		ALString* sValue2 = (yyvsp[0].sValue);

		// Concatenation des deux chaines
		(yyval.sValue) = new ALString(*sValue1 + *sValue2);

		// Destruction des ancienne chaines
		delete sValue1;
		delete sValue2;
	}
#line 3034 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 91: /* bigstring: STRINGLITTERAL  */
#line 1448 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		(yyval.sValue) = (yyvsp[0].sValue);
	}
#line 3042 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 92: /* semicolon: %empty  */
#line 1454 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Missing ';'");
	}
#line 3050 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 94: /* semicolon: ';' ';'  */
#line 1459 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous ';'");
	}
#line 3058 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 95: /* semicolon: ';' ';' ';'  */
#line 1463 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many ';'");
	}
#line 3066 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 96: /* openparenthesis: %empty  */
#line 1469 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		// Cette instruction est la pour aider au diagnostic des erreurs
		// de parenthesage: elle est utile dans ce cas, mais genere (avec
		// sa consoeur de nombreux shift/reduce et reduce conflicts
		yyerror("Missing '('");
	}
#line 3077 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 98: /* openparenthesis: '(' '('  */
#line 1477 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("There is one superfluous '('");
	}
#line 3085 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 99: /* openparenthesis: '(' '(' '('  */
#line 1481 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		yyerror("Too many '('");
	}
#line 3093 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

	case 100: /* closeparenthesis: %empty  */
#line 1488 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"
	{
		// Cette instruction est la pour aider au diagnostic des erreurs
		// de parenthesage: elle est utile dans ce cas, mais genere (avec
		// sa consoeur de nombreux shift/reduce et reduce conflicts
		yyerror("Missing ')'");
	}
#line 3104 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"
	break;

#line 3108 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.cpp"

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

#line 1498 "C:/Applications/boullema/DevGit/khiops/src/Learning/KWData/KWCYac.yac"

#include "KWCLex.inc"

// default yywrap that tells yylex to return 0
int yywrap()
{
	return 1;
}

// default yyerror for YACC and LEX
void yyerror(char const* fmt)
{
	yyerrorWithLineCorrection(fmt, 0);
}

// Variante avec une correction du numero de ligne
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

	// Test du nombre d'operandes
	if ((rule->GetVariableOperandNumber() and parsedRule->GetOperandNumber() < rule->GetOperandNumber() - 1) or
	    (not rule->GetVariableOperandNumber() and parsedRule->GetOperandNumber() != rule->GetOperandNumber()))

	{
		yyerror(sTmp + "Number of operands (" + IntToString(parsedRule->GetOperandNumber()) +
			") inconsistent with that of rule " + rule->GetName() + " (" +
			IntToString(rule->GetOperandNumber()) + ")");
		bOk = false;
	}
	// Verification et transfert des operandes
	else
	{
		// Dans le cas d'un nombre variable d'operandes, on commence par rajouter
		// eventuellement des operandes en fin de regle pour preparer l'instanciation
		if (parsedRule->GetOperandNumber() > rule->GetOperandNumber())
		{
			assert(rule->GetVariableOperandNumber());
			while (rule->GetOperandNumber() < parsedRule->GetOperandNumber())
				rule->AddOperand(rule->GetOperandAt(rule->GetOperandNumber() - 1)->Clone());
		}

		// Dans le cas d'un nombre variable d'operandes, on supprime eventuellement
		// le dernier operande, qui n'est pas obligatoire
		if (parsedRule->GetOperandNumber() < rule->GetOperandNumber())
		{
			assert(rule->GetVariableOperandNumber());
			assert(parsedRule->GetOperandNumber() == rule->GetOperandNumber() - 1);
			rule->DeleteAllVariableOperands();
		}
		assert(parsedRule->GetOperandNumber() == rule->GetOperandNumber());

		// Transfert des operandes
		for (i = 0; i < rule->GetOperandNumber(); i++)
		{
			// acces aux operandes
			operand = rule->GetOperandAt(i);
			parsedOperand = parsedRule->GetOperandAt(i);

			// Import de l'operande
			bOk = ImportParserOperand(rule->GetName(), i, parsedOperand, operand);
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean ImportParserRuleOutputOperands(const KWDerivationRule* parsedRule, KWDerivationRule* rule)
{
	boolean bOk = true;
	KWDRRelationCreationRule* creationRule;
	KWDerivationRuleOperand* parsedOperand;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(parsedRule != NULL);
	require(rule != NULL);
	require(not parsedRule->GetReference());
	require(not rule->GetReference());

	// Test du nombre d'operandes en sortie
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
	// Verification et transfert des operandes en sortie
	else
	{
		// Cast de la regle a alimenter en regle de creation de relation pour acceder a ses methodes dediees
		creationRule = cast(KWDRRelationCreationRule*, rule);

		// Dans le cas d'un nombre variable d'operandes en sortie, on commence par rajouter
		// eventuellement des operandes en fin de regle pour preparer l'instanciation
		if (parsedRule->GetOutputOperandNumber() > rule->GetOutputOperandNumber())
		{
			assert(rule->GetVariableOutputOperandNumber());
			while (rule->GetOutputOperandNumber() < parsedRule->GetOutputOperandNumber())
				creationRule->AddOutputOperand(
				    rule->GetOutputOperandAt(rule->GetOutputOperandNumber() - 1)->Clone());
		}

		// Dans le cas d'un nombre variable d'operandes en sortie, on supprime eventuellement
		// le dernier operande, qui n'est pas obligatoire
		if (parsedRule->GetOutputOperandNumber() < rule->GetOutputOperandNumber())
		{
			assert(rule->GetVariableOutputOperandNumber());
			assert(parsedRule->GetOutputOperandNumber() == rule->GetOutputOperandNumber() - 1);
			creationRule->DeleteAllVariableOutputOperands();
		}
		assert(parsedRule->GetOutputOperandNumber() == rule->GetOutputOperandNumber());

		// Transfert des operandes en sortie
		for (i = 0; i < rule->GetOutputOperandNumber(); i++)
		{
			// acces aux operandes en sortie
			operand = rule->GetOutputOperandAt(i);
			parsedOperand = parsedRule->GetOutputOperandAt(i);

			// Import de l'operande
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

	// Transfert d'informations de la regle de reference vers la regle a verifier
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

	// Test si operande candidate valide
	if (not parsedOperand->CheckDefinition())
	{
		bOk = false;
		yyerror(sTmp + "Incorrect operand " + IntToString(1 + nOperandIndex) + " for rule " + sRuleName);
	}
	// Test de compatibilite avec la regle enregistree, sauf si regle avec operande de type indetermine
	else if (operand->GetType() != KWType::Unknown and not parsedOperand->CheckFamily(operand))
	{
		bOk = false;
		yyerror(sTmp + "Operand " + IntToString(1 + nOperandIndex) + " inconsistent with that of rule " +
			sRuleName);
	}
	// Transfert de l'origine de l'operande
	else
	{
		// Transfert du niveau de scope
		operand->SetScopeLevel(parsedOperand->GetScopeLevel());

		// Transfert d'une valeur constante
		operand->SetOrigin(parsedOperand->GetOrigin());
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
		{
			operand->SetType(parsedOperand->GetType());
			operand->SetStringConstant(parsedOperand->GetStringConstant());
		}
		// Transfert d'un attribut
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
			operand->SetDataItemName(parsedOperand->GetDataItemName());
		else
		// Transfert d'une regle
		{
			// Transfert de la regle
			if (operand->GetDerivationRule() != NULL)
			{
				assert(parsedOperand->GetDerivationRule() != NULL);
				delete operand->GetDerivationRule();
			}
			operand->SetDerivationRule(parsedOperand->GetDerivationRule());

			// Transfert des infos portees par la regle de derivation
			if (operand->GetDerivationRule() != NULL)
			{
				operand->SetType(operand->GetDerivationRule()->GetType());
				if (KWType::IsRelation(operand->GetType()))
					operand->SetObjectClassName(operand->GetDerivationRule()->GetObjectClassName());
				if (operand->GetType() == KWType::Structure)
					operand->SetStructureName(operand->GetDerivationRule()->GetStructureName());
			}

			// Dereferencement de la regle de derivation depuis l'operande de travail
			parsedOperand->SetDerivationRule(NULL);
		}
	}
	return bOk;
}

int yyparse();

// Implementation de la methode de lecture de fichier de KWClassDomain
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

	// Affichage de stats memoire si log memoire actif
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile Begin");

	// Initialisation du domaine de classe a utiliser pour le Load
	assert(kwcdLoadDomain == NULL);
	kwcdLoadDomain = this;

	// Initialisation de la classe courante a utiliser pour le Load
	assert(kwcLoadCurrentClass == NULL);
	kwcLoadCurrentClass = NULL;

	// Creation du dictionnaire des classes referencees non crees
	assert(odReferencedUncreatedClasses == NULL);
	odReferencedUncreatedClasses = new ObjectDictionary;

	// Erreur si pas de nom de fichier
	fFile = NULL;
	if (sFileName == "")
	{
		AddError("Missing file name");
		bOk = false;
	}
	// Sinon, ouverture du fichier
	else
	{
		// Copie depuis HDFS si necessaire
		bOk = PLRemoteFileService::BuildInputWorkingFile(sFileName, sLocalFileName);
		if (bOk)
			bOk = FileService::OpenInputBinaryFile(sLocalFileName, fFile);
	}

	// On continue si fichier ouvert correctement
	if (bOk)
	{
		assert(fFile != NULL);

		// Memorisation de toutes les classes initiales
		kwcdLoadDomain->ExportClassDictionary(&odInitialClasses);

		// Activation du nombre max d'erreurs a afficher
		nFileParsingErrorNumber = 0;
		Global::ActivateErrorFlowControl();

		// Positionnement du fichier a parser par la variable yyin de LEX
		yylineno = 1;
		yyrestart(fFile);

		// Parsing
		yyparse();

		// Cleaning lexer
		yylex_destroy();

		// Fermeture du fichier
		FileService::CloseInputBinaryFile(sLocalFileName, fFile);

		// Si HDFS on supprime la copie locale
		PLRemoteFileService::CleanInputWorkingFile(sFileName, sLocalFileName);

		// Completion des informations de type au niveau du domaine
		if (nFileParsingErrorNumber == 0)
			kwcdLoadDomain->CompleteTypeInfo();

		// Lecture des informations privees depuis les meta donnees
		if (nFileParsingErrorNumber == 0)
		{
			for (i = 0; i < kwcdLoadDomain->GetClassNumber(); i++)
			{
				kwcClass = kwcdLoadDomain->GetClassAt(i);
				kwcClass->ReadPrivateMetaData();
			}
		}

		// Messages d'erreur pour les classes referencees non crees
		if (nFileParsingErrorNumber > 0 or odReferencedUncreatedClasses->GetCount() > 0)
		{
			odReferencedUncreatedClasses->ExportObjectArray(&oaReferencedUncreatedClasses);
			for (i = 0; i < oaReferencedUncreatedClasses.GetSize(); i++)
			{
				kwcClass = cast(KWClass*, oaReferencedUncreatedClasses.GetAt(i));
				AddError("Dictionary " + kwcClass->GetName() + " used, but not declared");
			}
		}

		// Desactivation du nombre max d'erreurs a afficher
		Global::DesactivateErrorFlowControl();

		// Destruction des classes crees si au moins une erreur de parsing detectee
		// ou au moins une classe referencee non cree
		if (nFileParsingErrorNumber > 0 or odReferencedUncreatedClasses->GetCount() > 0)
		{
			// En cas d'erreur, ajout d'une ligne blanche pour separer des autres logs
			AddError("Errors detected during parsing " + sFileName + ": read operation cancelled");
			AddSimpleMessage("");
			bOk = false;

			// Recherche des nouvelles classes crees
			for (i = 0; i < kwcdLoadDomain->GetClassNumber(); i++)
			{
				kwcClass = kwcdLoadDomain->GetClassAt(i);
				if (odInitialClasses.Lookup(kwcClass->GetName()) == NULL)
					oaNewClasses.Add(kwcClass);
			}

			// Destruction des classes nouvellement crees
			for (i = 0; i < oaNewClasses.GetSize(); i++)
			{
				kwcClass = cast(KWClass*, oaNewClasses.GetAt(i));
				kwcdLoadDomain->DeleteClass(kwcClass->GetName());
			}

			// Destruction des classes referencees non crees
			odReferencedUncreatedClasses->DeleteAll();
		}
		nFileParsingErrorNumber = 0;
	}

	// Nettoyage
	kwcdLoadDomain = NULL;
	kwcLoadCurrentClass = NULL;
	delete odReferencedUncreatedClasses;
	odReferencedUncreatedClasses = NULL;

	// Affichage de stats memoire si log memoire actif
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " ReadFile End");

	return bOk;
}
