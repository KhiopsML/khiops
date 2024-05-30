%{
#include <stdlib.h>

// Desactivation de warnings pour le Visual C++
#ifdef __MSC__
#pragma warning(disable : 4505) // C4505: la fonction locale non référencée a été supprimée
#pragma warning(disable : 4996) // C4996: warning for deprecated POSIX names isatty and fileno
#endif                          // __MSC__

/* Redefinition du nombre de token max */
#undef YYLMAX
#define	YYLMAX		100000		/* token and pushback buffer size */

%}

%p 5000
/* pour avoir acces aux numeros de lignes, et moins cher que le -l de la ligne de commande */
%option yylineno 

digit     [0-9]
sign      [\-\+]
exponent  [eE]
separator [\.]
continuous {sign}?({digit}{digit}*{separator}?{digit}*|{digit}*{separator}?{digit}{digit}*)({exponent}?{sign}?{digit}{digit}*)?
letter    [a-zA-Z_\*]
name      {letter}({letter}|{digit})*

/* Attention, la liste des mots cles du langage doit etre reprise dans la methode KWClass::IsStringKeyWord() */
/* si on veut autoriser des noms de variable en collision avec ces mots cles.                                */

%%

\/\/                      {
                          // Les types retournes par le parser sont des unsigned char:
                          // il faut etre compatible sous peine de bugs pour des caracteres interpretes
                          // commes des caracteres speciaux
                          ALString *sValue;
                          int nInput;
						  unsigned char c;

                          // Lecture du commentaire jusqu a la fin de ligne
                          sValue = new ALString ();
						  while ((nInput = yyinput()) != YYEOF)
						  {
						      c = (unsigned char)nInput;
						      if (c == '\n')
						        break;
						      *sValue += c;
						  }
                          sValue->TrimLeft();
                          sValue->TrimRight();

                          // Retour de la valeur et du token
                          yylval.sValue = sValue;
                          return  LABEL;
                          }
                          

\"                       {
                          ALString *sValue;
                          int nInput;
						  unsigned char c=' ';
                          int nNextInput;
						  unsigned char cNext=' ';

                          // Calcul de la valeur de la chaine
                          sValue = new ALString ();
						  while ((nInput = yyinput()) != YYEOF)
						  {
						      c = (unsigned char)nInput;
						      if (c == '\n')
						        break;
							  // Traitement du double-quote
						      if (c == '"')
							  {
							    // On recherche le caractere suivant
								nNextInput = yyinput();
								cNext = (unsigned char)nNextInput;

								// Si pas d'autre double-quote (doublement de double-quote interne), on remet le caractere a analyser avant de declarer la fin du token
								if (cNext != '"')
								{
								  unput(cNext);
								  break;
								}
							  }
						      *sValue += c;
						  }


						  // Test fin de ligne
						  if (c == '\n')
						  {
						      if (sValue->GetLength() <= 100)
						         yyerror("Unfinished string: " + *sValue);
							  else
						         yyerror("Unfinished string: " + sValue->Left(100) + "...");
						  }
						  // Test fin de fichier
						  else if (nInput == YYEOF)
						  {
						      if (sValue->GetLength() <= 100)
						         yyerror("Unfinished string before end of file (special char detected): <" + *sValue + ">");
							  else
						         yyerror("Unfinished string before end of file  (special char detected): <" + sValue->Left(100) + "...>");
						  }
                          
                          // Retour de la valeur et du token
                          yylval.sValue = sValue;
                          return  STRINGLITTERAL;
                          }

\`                        {
                          ALString *sValue;
                          int nInput;
						  unsigned char c=' ';
                          int nNextInput;
						  unsigned char cNext=' ';

						  // Identifiant entre back quotes
                          // Calcul de la valeur de l identifiant
                          sValue = new ALString ();
						  while ((nInput = yyinput()) != YYEOF)
						  {
						      c = (unsigned char)nInput;
						      if (c == '\n')
						        break;
							  // Traitement du back-quote
						      if (c == '`')
							  {
							    // On recherche le caractere suivant
								nNextInput = yyinput();
								cNext = (unsigned char)nNextInput;

								// Si pas d'autre back-quote (doublement de back-quote interne), on remet le caractere a analyser avant de declarer la fin du token
								if (cNext != '`')
								{
								  unput(cNext);
								  break;
								}
							  }
						      *sValue += c;
						  }

						  // Test fin de ligne
						  if (c == '\n')
						  {
						      if (sValue->GetLength() <= 100)
						         yyerror("Unfinished identifier: " + *sValue);
							  else
						         yyerror("Unfinished identifier: " + sValue->Left(100) + "...");
						  }
						  // Test fin de fichier
						  else if (nInput == YYEOF)
						  {
						      if (sValue->GetLength() <= 100)
						         yyerror("Unfinished identifier before end of file (special char detected): <" + *sValue + ">");
							  else
						         yyerror("Unfinished identifier before end of file  (special char detected): <" + sValue->Left(100) + "...>");
						  }

                          // Retour de la valeur et du token
                          yylval.sValue = sValue;
                          return  EXTENDEDIDENTIFIER;
                          }

"Dictionary"              return CLASS;

"Numerical"               return CONTINUOUSTYPE;

"Categorical"             return SYMBOLTYPE;

"Table"					  return OBJECTARRAYTYPE;

"Root"                    return ROOT;

"Unused"                  return UNUSED;

"Date"					  return DATETYPE;

"Time"                    return TIMETYPE;

"Timestamp"               return TIMESTAMPTYPE;

"TimestampTZ"             return TIMESTAMPTZTYPE;

"Text"                    return TEXTTYPE;

"TextList"                return TEXTLISTTYPE;

"Entity"                  return OBJECTTYPE;

"Structure"               return STRUCTURETYPE;


[<>(){}=;:,+\[\]\.]                return *yytext;

{name}                    {
                          ALString *sValue;
                          sValue = new ALString ( (char*)yytext );
                          yylval.sValue = sValue;
                          return  BASICIDENTIFIER;
                          }

{continuous}              {
                          yylval.cValue=KWContinuous::StringToContinuous((char*)yytext);
                          return(CONTINUOUSLITTERAL);
                          }

"#Missing"                {
                          yylval.cValue=KWContinuous::GetMissingValue();
                          return(CONTINUOUSLITTERAL);
                          }

"#"                      {
                          // Les types retournes par le parser sont des unsigned char:
                          // il faut etre compatible sous peine de bugs pour des caracteres interpretes
                          // commes des caracteres speciaux
                          ALString *sValue;
                          int nInput;
						  unsigned char c;

                          // Lecture du commentaire jusqu a la fin de ligne
                          sValue = new ALString ();
						  while ((nInput = yyinput()) != YYEOF)
						  {
						      c = (unsigned char)nInput;
						      if (c == '\n')
						        break;
						      *sValue += c;
						  }
                          sValue->TrimLeft();
                          sValue->TrimRight();

                          // Retour de la valeur et du token
                          yylval.sValue = sValue;
                          return  APPLICATIONID;
                          }
                          


[ \t\n\f\r\v]+                  ;


.                         {
						  const int nMaxLength = 20;
                          ALString sTmp;
                          ALString sToken;
                          int nInput;
                          char c;
  					      int nCorrectedLineNumber;
                          
                          // Initialisation de la valeur du token
                          c = yytext[0];
						  if (not p_isprint(c))
						  {
  			                sToken += '[';
						    sToken += IntToString((unsigned char)c);
						    sToken += ']';
						  }
						  else
						    sToken += c;
                          
                          // On aspire tous les caracteres non imprimables suivants
						  nCorrectedLineNumber = yylineno;
						  while ((nInput = yyinput()) != YYEOF)
						  {
						      c = (unsigned char)nInput;
						      if (c == '\n' or c == '\r')
							  {
							    nCorrectedLineNumber--;
						        break;
						      }
						      if (not p_isprint(c))
							  {
							    if (sToken.GetLength() <= nMaxLength)
							    {
								  if (sToken.GetLength()+5 > nMaxLength)
								    sToken += "...";
								  else
								  {
						            sToken += '[';
						            sToken += IntToString((unsigned char)c);
						            sToken += ']';
								  }
								}
							  }
						      else
						      {
						        unput(c);
						        break;
						      }
						  }
						  if (nCorrectedLineNumber <= 0)
							nCorrectedLineNumber = 1;
                          Global::AddWarning("Read dictionary file", sTmp + "Line " + IntToString(nCorrectedLineNumber),
                            sTmp + "Unexpected special chars <" + sToken + "> (ignored)");
                          }
