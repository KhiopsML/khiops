%option prefix="json"

/* Does not need yywrap() */
%option noyywrap

%{
#include <stdlib.h>

// Desactivation de warnings pour le Visual C++
#ifdef __MSC__
#pragma warning(disable : 4505) // C4505: la fonction locale non référencée a été supprimée
#pragma warning(disable : 4996) // C4996: warning for deprecated POSIX names isatty and fileno
#endif                          // __MSC__
%}

/* Pour avoir acces aux numeros de lignes, et moins cher que le -l de la ligne de commande */
%option yylineno 

DIGIT           [0-9]
DIGIT1          [1-9]
INTNUM          {DIGIT1}{DIGIT}*
FRACT           "."{DIGIT}+
FLOAT           ({INTNUM}|0){FRACT}?
EXP             [eE][+-]?{DIGIT}+
NUMBER          -?{FLOAT}{EXP}?

UNICODE         \\u[A-Fa-f0-9]{4}
ESCAPECHAR      \\["\\/bfnrt]
CHAR            [^"\\]|{ESCAPECHAR}|{UNICODE}
STRING          \"{CHAR}*\"

WHITESPACE      [ \t\r\n]


%%

\{              {return '{';}
\}              {return '}';}
\[              {return '[';}
\]              {return ']';}
,               {return ',';}
:               {return ':';}
true            {jsonlval.bValue = true; return BOOLEANVALUE;}
false           {jsonlval.bValue = false; return BOOLEANVALUE;}
null            {return NULLVALUE;}

 

{STRING}        {
                    boolean bOk;
                    ALString *sValue;

                    // Conversion sans les double quote de debut et fin
                    sValue = new ALString;
                    yytext[yyleng-1] = '\0';
                    bOk = TextService::JsonToCString((char*)&yytext[1], *sValue);
                    jsonlval.sValue = sValue; 
                    
                    // On retourne la chaine convertie en cas de succes
                    if (bOk)
                        return STRINGVALUE;
                    // Sinon, on retourne la chaine convertie au mieux, mais en tant que chaine erronnee
                    else
                        return STRINGERROR;
                }

{NUMBER}        {
                    char* endptr;
                    double dValue;

                    // Conversion, en projetant sur les valeurs extremes en cas d'oberflow
    		        dValue = strtod((char*)yytext, &endptr);
                    if (dValue == -HUGE_VAL)
                        dValue = -DBL_MAX;
                    else if (dValue == HUGE_VAL)
                        dValue = DBL_MAX;
                    assert(not p_isnan(dValue));
                    jsonlval.dValue = dValue; 
                    return NUMBERVALUE;
                }

{WHITESPACE}    {
                  // On ignore les caracteres d'espacement
                }

.               {
                    ALString *sValue;

                    sValue = new ALString(yytext);
                    jsonlval.sValue = sValue; 
                    return ERROR;
                }

%%