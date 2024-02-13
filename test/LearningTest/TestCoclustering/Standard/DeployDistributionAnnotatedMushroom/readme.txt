Test d'utilisation d'un modéle de coclustering edité par Khiops Visualisation

En entree, dans répertoire courant:
  . Coclustering.khc
    . fichier de coclustering edité par Khiops Covizualisation
      . libellés et annotations de quelques cluster saisies depuis l'outil de visualisation
      . état de l'interface
      . visualisable avec Khiops Visualisation
      
En sortiee, dans répertoire results
  . SimplifiedCoclustering.khc
    . fichier d'entrée lue par Khiops Coclustering et récrit en sortie (ici, il n'y a pas eu de simplification)
    . libellés et annotations des clusters conservés (shortDescription et description)
    . état de l'interface perdu
    . visualisable avec Khiops Visualisation, avec toutes les information sur le model et les annotations,
      mais en ayant perdu l'état courant
  . SimplifiedCoclustering.json: idem au format json
    