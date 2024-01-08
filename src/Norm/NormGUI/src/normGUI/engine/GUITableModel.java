// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.util.Vector;
import javax.swing.table.AbstractTableModel;

/**
 * Definit le modele de tableau. Cette classe communique avec le tableau
 * (GUITable) et avec la partie c++ (UIList)
 */
public class GUITableModel extends AbstractTableModel
{

        /**
         * Liste associee
         */
        private GUIList guiList;

        /**
         * Cree un modele de tableau
         *
         * @param guiList La liste associee
         */
        public GUITableModel(GUIList guiList) { this.guiList = guiList; }

        /**
         * Renvoie la classe d'une colonne du tableau
         *
         * @param c L'index de la colonne
         * @return La classe de la colonne
         */
        public Class getColumnClass(int c) { return getValueAt(0, c).getClass(); }

        /**
         * Renvoie le nombre de colonnes du tableau
         *
         * @return Le nombre de colonnes du tableau
         */
        public int getColumnCount() { return guiList.getListHeader().length; }

        /**
         * Renvoie le nom d'une colonne de tableau
         *
         * @param col L'index de la colonne
         * @return Le nom de la colonne
         */
        public String getColumnName(int col) { return guiList.getListHeader()[col]; }

        /**
         * Renvoie le nombre de lignes du tableau
         *
         * @return Le nombre de lignes du tableau
         */
        public int getRowCount()
        {
                // Renvoie une valeur bufferisee
                return nBufferRowNumber;
        }

        /**
         * Renvoie un objet dans une cellule du tableau
         *
         * @param row La ligne du tableau
         * @param col La colonne du tabeau
         * @return L'objet du tableau
         */
        public Object getValueAt(int row, int col)
        {
                // Renvoie une valeur bufferisee
                return getBufferValueAt(row, col);
        }

        /**
         * Rafraichissement du contenu du tableau
         */
        public void refreshAll()
        {
                int currentRow;
                int row;
                int col;
                GUIElement guiElement;
                Vector columns;

                // Methode declenche par graphicRefreshAll, seul acces au composant C++ pour
                // minimiser les risques lies au multi-thread
                // Toutes les autres methodes passent par le contenu bufferise du tableau
                nBufferRowNumber = guiList.getItemNumber();

                // Boucle de rafraichissement des valeurs
                currentRow = -1;
                if (guiList.getItemNumber() > 0)
                        currentRow = guiList.getCurrentItemIndex();
                columns = guiList.getVisibleColumns();
                for (row = 0; row < nBufferRowNumber; row++) {
                        // On se positionne sur la ligne courante c++
                        // Attention : on utilise la ligne courante afin de ne pas perturber la
                        // selection
                        guiList.setCurrentItemIndex(row);

                        // Boucle sur les colonnes
                        for (col = 0; col < getColumnCount(); col++) {
                                guiElement = (GUIElement)columns.get(col);

                                // On recupere la valeur c++ de l'element
                                Object value = guiElement.getValueIn(guiList);

                                // On la memorise dans le buffer de valeurs
                                setBufferValueAt(value, row, col);
                        }
                }
                guiList.setCurrentItemIndex(currentRow);
        }

        /**
         * Renvoie le caractere editable d'une cellule du tableau
         *
         * @param row La ligne du tableau
         * @param col La colonne du tableau
         * @return Le caractere editable de la cellule
         */
        public boolean isCellEditable(int row, int col)
        {
                GUIData guiData = (GUIData)guiList.getVisibleColumns().elementAt(col);
                return guiData.getEditable();
        }

        /**
         * Modifie les valeurs c++ avec les valeurs du tableau
         *
         * @param value La nouvelle valeur
         * @param row   La ligne du tableau
         * @param col   La colonne du tableau
         */
        public void setValueAt(Object value, int row, int col)
        {
                // On se base sur la derniere valeur bufferisee du RowCount pour eviter de
                // solliciter getRowCount
                // (qui pose toujours des probleme de synchronisation)
                if (0 <= row && row < nBufferRowNumber) {
                        // On recupere l'element correspondant a la colonne
                        Vector columns = guiList.getVisibleColumns();
                        GUIElement guiElement = (GUIElement)columns.get(col);

                        // On se positionne sur la ligne courante c++
                        // Attention : on utilise la ligne courante afin de ne pas perturber la
                        // selection
                        guiList.setCurrentItemIndex(row);

                        // Si la nouvelle valeur est differente de la valeur c++
                        if (!guiElement.getValueIn(guiList).toString().equals(value.toString())) {
                                // On enregistrement l'index selectionne dans la table prealablement
                                guiList.getGUITable().updateIndex(row);

                                // On met a jour la valeur c++ de l'element et on ecrit le scenario
                                guiElement.recordUpdateElement(guiList, value.toString());

                                // On recherche a nouveau la valeur c++, qui a pu etre actualisee
                                // par le recordUpdateElement (dans le cas TriggerRefresh)
                                guiList.setCurrentItemIndex(row);
                                value = guiElement.getValueIn(guiList);
                        }

                        // On la memorise bufferisee
                        setBufferValueAt(value, row, col);
                }
        }

        /**
         * Renvoie un objet bufferise dans une cellule du tableau
         *
         * @param row La ligne du tableau
         * @param col La colonne du tabeau
         * @return L'objet du tableau
         */
        private Object getBufferValueAt(int row, int col)
        {
                // Index de la valeur
                int valueIndex = row * getColumnCount() + col;

                // Retour par defaut si valeur inexistante
                if (vectorOfBufferValues == null || valueIndex >= vectorOfBufferValues.capacity() ||
                    vectorOfBufferValues.elementAt(valueIndex) == null)
                        return new String();
                // Retour de la valeur
                else
                        return vectorOfBufferValues.elementAt(valueIndex);
        }

        /**
         * Modifie les valeurs bufferisees du tableau
         *
         * @param value La nouvelle valeur
         * @param row   La ligne du tableau
         * @param col   La colonne du tableau
         */
        private void setBufferValueAt(Object value, int row, int col)
        {
                // Index de la valeur
                int valueIndex = row * getColumnCount() + col;

                // Creation et retaillage si necessaire
                if (vectorOfBufferValues == null) {
                        vectorOfBufferValues = new Vector<Object>();
                        vectorOfBufferValues.setSize(100);
                }
                if (vectorOfBufferValues.size() <= valueIndex)
                        vectorOfBufferValues.setSize(2 * (valueIndex + 1));

                // Memorisation de la valeur
                vectorOfBufferValues.setElementAt(value, valueIndex);
        }

        /**
         * Liste des valeurs de la table, permettant de bufferiser les valeurs provenant
         * du C++ Ces valeur sont collectees en fonction des besoins d'affichage, et
         * utilisees pour // les reaffichage pour ne pas rentrer en conflit avec les
         * acces C++
         */
        private Vector<Object> vectorOfBufferValues;
        private int nBufferRowNumber;
}
