// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.Rectangle;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Vector;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

/**
 * Definit une unite d'interface de type liste
 */
public class GUIList extends GUIUnit
{

        /**
         * Tableau associe a la liste
         */
        protected GUITable guiTable;

        /**
         * Modele du tableau
         */
        protected GUITableModel guiTableModel;

        /**
         * Ajoute une ligne a la liste c++
         */
        protected native void addItem();

        /**
         * Renvoie l'index de la liste c++
         *
         * @return L'index de la liste c++
         */
        public native int getCurrentItemIndex();

        /**
         * Renvoie l'indice de fraicheur de la liste c++
         *
         * @return L'indice de fraicheur de la liste c++
         */
        public native int getFreshness();

        /**
         * Renvoie le tableau associe a la liste
         *
         * @return Le tableau associe a la liste
         */
        public GUITable getGUITable() { return this.guiTable; }

        /**
         * Renvoie le modele du tableau
         *
         * @return Le modele du tableau
         */
        public GUITableModel getGUITableModel() { return guiTableModel; }

        /**
         * Renvoie le nombre de lignes de la liste c++
         *
         * @return Le nombre de ligne de la liste c++
         */
        public native int getItemNumber();

        /**
         * Renvoie les noms des colonnes sous forme de tableau
         *
         * @return Le tableau de chaine regoupant les noms des colonnes
         */
        public String[] getListHeader()
        {
                GUIData guiData;
                String[] columnNames = new String[getVisibleColumns().size()];
                int j = 0;
                for (int i = 0; i < vectorOfGUIDatas.size(); i++) {
                        guiData = (GUIData)vectorOfGUIDatas.elementAt(i);
                        if (guiData.getVisible()) {
                                columnNames[j] = guiData.getLabel();
                                j++;
                        }
                }
                return columnNames;
        }

        /**
         * Renvoie la ligne selectionnee dans la liste c++
         *
         * @return La ligne selectionnee dans la liste c++
         */
        public native int getSelectedItemIndex();

        /**
         * Renvoie la liste des colonnes a afficher (donnees visibles)
         *
         * @return La liste des colonnes a afficher
         */
        public Vector<GUIData> getVisibleColumns()
        {
                Vector<GUIData> columns = new Vector<GUIData>();
                GUIData guiData;
                if (vectorOfGUIDatas == null)
                        return new Vector<GUIData>();
                else {
                        for (int i = 0; i < vectorOfGUIDatas.size(); i++) {
                                guiData = (GUIData)vectorOfGUIDatas.get(i);
                                if (guiData.getVisible())
                                        columns.add(vectorOfGUIDatas.get(i));
                        }
                        return columns;
                }
        }

        /**
         * Ajoute la liste dans le panel de l'unite mere
         *
         * @param panel       Panneau de l'unite mere dans lequel sera ajoute le
         *                    composant d'affichage
         * @param constraints Contraintes d'affichage
         */
        public void graphicAddField(JPanel panel, GridBagConstraints constraints)
        {
                guiTableModel = new GUITableModel(this);
                guiTable = new GUITable(this);

                // Positionnement de la table
                JScrollPane jsp = new JScrollPane(guiTable);

                // Gestion du click droit sur le scrollpane contenant le tableau
                jsp.getViewport().addMouseListener(new MouseAdapter() {
                        public void mouseClicked(MouseEvent e)
                        {
                                // Si on clique a droite et que le composant est enabled
                                if (e.getButton() == 3 && guiTable.isEnabled())
                                        guiTable.getPopMenu().show(e.getComponent(), e.getX(), e.getY());
                        }
                });

                // On recupere les dimensions de l'ecran
                Rectangle screenBounds = getCurrentScreenBounds();

                // On fixe la hauteur a 5 lignes par defaut
                int defaultRowNumber = 5;
                int rowNumber = defaultRowNumber;

                // Nombre max de lignes, en tenant compte de la taille de l'ecran, avec une marge heuristique
                // pour tenir compte de la barre de tache, de la barre de titre, de menu, d'un libelle, de boutons...
                int screenMaxUsableHeight = screenBounds.height * 2 / 3;
                int maxRowNumber = screenMaxUsableHeight / getComponentPreferredHeight();
                if (maxRowNumber < defaultRowNumber)
                        maxRowNumber = defaultRowNumber;

                // Recherche du (LineNumber, LastColumnExtraWidth) dans les parametres
                int tableWidth = guiTable.getPreferredSize().width;
                if (!getParameters().equals("") && getParametersAsArray().length == 2) {
                        String value;

                        // Recherche du nombre de lignes
                        value = getParametersAsArray()[0];
                        if (!value.equals("")) {
                                try {
                                        rowNumber = Integer.parseInt(value);
                                } catch (Exception ex) {
                                        rowNumber = defaultRowNumber;
                                }
                                if (rowNumber < 1)
                                        rowNumber = defaultRowNumber;
                                if (rowNumber > maxRowNumber)
                                        rowNumber = maxRowNumber;
                        }

                        // Recherche de la taille supplementaires a ajouter
                        value = getParametersAsArray()[1];
                        int lastColumnExtraWidth = 0;
                        if (!value.equals("")) {
                                try {
                                        lastColumnExtraWidth = Integer.parseInt(value);
                                } catch (Exception ex) {
                                        lastColumnExtraWidth = 0;
                                }
                                if (lastColumnExtraWidth < 0 || lastColumnExtraWidth > 50)
                                        lastColumnExtraWidth = 0;
                                tableWidth = tableWidth + getComponentPreferredWidth(lastColumnExtraWidth);
                        }
                }

                // On limite l'affichage a 15 colonnes (de 10 caracteres)
                int tableHeight = rowNumber * getComponentPreferredHeight();
                if (tableWidth > 15 * getComponentPreferredWidth(10))
                        tableWidth = 15 * getComponentPreferredWidth(10);
                if (tableWidth > screenBounds.width)
                        tableWidth = screenBounds.width;
                if (tableHeight > screenMaxUsableHeight)
                        tableHeight = screenMaxUsableHeight;
                guiTable.setPreferredScrollableViewportSize(new Dimension(tableWidth, tableHeight));

                // Ajout du composant
                constraints.weightx = 1.0;
                constraints.weighty = 1.0;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(jsp, constraints);

                if (hasActionButton()) {

                        // S'il n'y a aucune action ou bien s'il ne s'agit pas d'une fenetre (dans ce
                        // cas les
                        // boutons apparaissent deja dans le panel sud grace a addToolBar() de GUIUnit)
                        if (getActionCount() == 0 || frame != null)
                                return;

                        // Creation du panel sud
                        JPanel southPanel = new JPanel();

                        // Pour chaque action, on cree un bouton si necessaire
                        for (int i = 0; i < getActionCount(); i++) {
                                GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                                if (guiAction.getVisible() && guiAction.isButton()) {
                                        JButton actionButton = guiAction.createButton();
                                        actionButton.addActionListener(this);
                                        southPanel.add(actionButton);
                                }
                        }

                        // Ajout du panel sud au panel de l'unite mere
                        constraints.weightx = 0;
                        constraints.weighty = 0;
                        constraints.gridwidth = GridBagConstraints.REMAINDER;
                        panel.add(southPanel, constraints);
                }
        }

        /**
         * Rafraichit la liste apres l'execution d'une action
         */
        public void graphicRefreshAll()
        {
                // On met a jour le contenu de la table
                guiTableModel.refreshAll();
        }

        /**
         * Previent java d'un changement
         */
        public void graphicFireDataChange()
        {
                guiTableModel.fireTableDataChanged();

                // On met a jour la selection graphique
                int row = getSelectedItemIndex();
                if (-1 < row && row < guiTable.getRowCount())
                        guiTable.setRowSelectionInterval(row, row);
                else if (0 < guiTable.getRowCount())
                        guiTable.removeRowSelectionInterval(0, guiTable.getRowCount() - 1);
        }

        /**
         * Insere une ligne dans la liste c++
         *
         * @param nIndex L'index de la ligne a inserer
         */
        protected native void insertItemAt(int nIndex);

        /**
         * Supprime toutes les lignes de la liste c++
         */
        private native void removeAllItems();

        /**
         * Supprime une ligne dans la liste c++
         *
         * @param nIndex L'index de la ligne a supprimer
         */
        protected native void removeItemAt(int nIndex);

        /**
         * Modifie de l'index de la ligne courante c++
         *
         * @param nIndex Le nouvel index de la ligne courante
         */
        public native void setCurrentItemIndex(int nIndex);

        /**
         * Modifie l'index de la ligne selectionnee c++
         *
         * @param nIndex Le nouvel index de la ligne selectionnee
         */
        public native void setSelectedItemIndex(int nIndex);
}
