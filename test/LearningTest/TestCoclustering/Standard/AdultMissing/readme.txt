Bug d�tect� par Romain Guigour�s le 03/11/2011

Contexte:
  - tri-clustering
  - une variable numerique contient des valeurs manquante
  - il y a un intervalle ne contenant que les valeurs manquantes (Missing)
Bug: dans la hi�rarchie, toutes les parties terminales (dont Missing) ont un m�me p�re
Solution: les clusters interm�diaires impliquant Missing sont pr�fix�s par *
