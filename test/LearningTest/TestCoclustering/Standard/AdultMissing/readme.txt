Bug détecté par Romain Guigourès le 03/11/2011

Contexte:
  - tri-clustering
  - une variable numerique contient des valeurs manquante
  - il y a un intervalle ne contenant que les valeurs manquantes (Missing)
Bug: dans la hiérarchie, toutes les parties terminales (dont Missing) ont un même père
Solution: les clusters intermédiaires impliquant Missing sont préfixés par *
