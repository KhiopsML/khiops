Test d'utilisation d'un mod�le de coclustering edit� par Khiops Visualisation

En entree, dans r�pertoire courant:
  . Coclustering.khc
    . fichier de coclustering edit� par Khiops Covizualisation
      . libell�s et annotations de quelques cluster saisies depuis l'outil de visualisation
      . �tat de l'interface
      . visualisable avec Khiops Visualisation
      
En sortiee, dans r�pertoire results
  . SimplifiedCoclustering.khc
    . fichier d'entr�e lue par Khiops Coclustering et r�crit en sortie (ici, il n'y a pas eu de simplification)
    . libell�s et annotations des clusters conserv�s (shortDescription et description)
    . �tat de l'interface perdu
    . visualisable avec Khiops Visualisation, avec toutes les information sur le model et les annotations,
      mais en ayant perdu l'�tat courant
  . SimplifiedCoclustering.json: idem au format json
    