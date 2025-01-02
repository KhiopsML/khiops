// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.util.Vector;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JTable;
import javax.swing.KeyStroke;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

/**
 * Definit le tableau associe a la liste
 *
 * @author Marc Boulle
 */
public class GUITable extends JTable
{

        /**
         * Liste des colonnes visibles
         */
        protected Vector columns;

        /**
         * Liste associee au tableau
         */
        protected GUIList guiList;

        /**
         * Menu surgissant associe au tableau
         */
        protected JPopupMenu popMenu;

        /**
         * Cree un tableau
         *
         * @param guiList La liste associee au tableau
         */
        public GUITable(GUIList guiList)
        {
                super(guiList.getGUITableModel());
                this.guiList = guiList;
                this.columns = guiList.getVisibleColumns();

                // Mise en forme du tableau
                setRowHeight(GUIObject.getComponentPreferredHeight());
                setAutoResizeMode(AUTO_RESIZE_LAST_COLUMN);
                getTableHeader().setReorderingAllowed(false);
                setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

                // Utilisation d'une propriete (peu documentee) pour forcer la modification
                // du comportement des cellules vis a vis de la perte de focus
                // Indispensable pour le fonctionnement correct de GUIIntElementSpinner
                putClientProperty("terminateEditOnFocusLost", Boolean.TRUE);

                // La barre d'espace permet de modifier le contenu d'une cellule
                getInputMap().put(KeyStroke.getKeyStroke("SPACE"), "startEditing");

                // Construction du menu surgissant
                buildPopupMenu();

                // Gestion de la souris
                addMouseListener(new MouseAdapter() {
                        public void mouseClicked(MouseEvent e)
                        {
                                // Si on clique a droite et que le composant est enabled
                                if (e.getButton() == 3 && isEnabled()) {
                                        // On recupere la ligne sous le curseur
                                        int row = rowAtPoint(e.getPoint());
                                        // On met a jour la selection graphique
                                        if (row > -1)
                                                setRowSelectionInterval(row, row);
                                        // Si une ligne est selectionnee et si le popup est n'est pas vide
                                        if (getSelectedRow() > -1 && popMenu.getComponentCount() > 0) {
                                                popMenu.show(e.getComponent(), e.getX(), e.getY());
                                        }
                                }
                        }
                });

                // Gestion de la selection
                ListSelectionModel rowSM = getSelectionModel();
                rowSM.addListSelectionListener(new ListSelectionListener() {
                        public void valueChanged(ListSelectionEvent e)
                        {
                                // On ignore les messages superflus
                                if (e.getValueIsAdjusting())
                                        return;
                                ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                                // Si une ligne est selectionnee
                                if (!lsm.isSelectionEmpty()) {
                                        updateIndex(lsm.getMinSelectionIndex());
                                }
                        }
                });

                // Attribution des rendus et des editeurs
                initialize();

                // Initialise la gestion des info-bulle pour les colonnes de l'entete de la
                // table
                initializeHeaderTooltips();
        }

        /**
         * Construit le menu surgissant a partir de la liste d'actions
         */
        public void buildPopupMenu()
        {
                // Si la liste contient des actions, on construit le popmenu
                if (guiList.vectorOfGUIActions.size() > 0) {
                        popMenu = new JPopupMenu();
                        // Pour chaque action
                        for (int i = 0; i < guiList.vectorOfGUIActions.size(); i++) {
                                GUIAction action = (GUIAction)guiList.vectorOfGUIActions.get(i);
                                // Si l'action n'est pas un exit ni un refresh (actions standards des unites)
                                if (action.getVisible() && action.getStyle().equals("") &&
                                    !action.getIdentifier().equals(GUIUnit.Exit) &&
                                    !action.getIdentifier().equals(GUIUnit.Refresh)) {
                                        JMenuItem menuItem = new JMenuItem(action.getLabel());
                                        menuItem.setActionCommand(action.getIdentifier());
                                        menuItem.setAccelerator(KeyStroke.getKeyStroke(action.getAccelKey()));
                                        if (action.getShortCut() != ' ')
                                                menuItem.setMnemonic((int)action.getShortCut());
                                        action.setComponentHelpText(menuItem);
                                        menuItem.addActionListener(new ActionListener() {
                                                public void actionPerformed(ActionEvent e)
                                                {
                                                        // On execute l'action au niveau de l'unite
                                                        guiList.actionPerformed(e);
                                                }
                                        });
                                        popMenu.add(menuItem);
                                }
                        }
                }
        }

        /**
         * Renvoie le menu surgissant
         *
         * @return Le menu surgissant
         */
        public JPopupMenu getPopMenu() { return popMenu; }

        /**
         * Renvoie la chaines a afficher dans les infobulles
         *
         * @param e L'evenement de souris
         * @return La chaine a afficher dans les infobulles
         */
        public String getToolTipText(MouseEvent e)
        {
                int col = columnAtPoint(e.getPoint());
                int realCol = convertColumnIndexToModel(col);
                GUIObject guiObject = (GUIObject)columns.elementAt(realCol);
                return guiObject.getHelpText();
        }

        /**
         * Initialise le tableau avec les rendus et les editeurs
         */
        public void initialize()
        {

                // Pour chaque colonne
                int nColumnWidth;
                for (int i = 0; i < getColumnCount(); i++) {

                        // On recupere l'element associe
                        GUIElement guiElement = (GUIElement)columns.get(i);

                        // On affecte le rendu a la colonne
                        getColumnModel().getColumn(i).setCellRenderer(guiElement.getCellElement());

                        // On prend en compte la largeur preferee des composants, avec un max sauf pour
                        // la derniere colonne
                        nColumnWidth = (int)guiElement.buildComponent().getPreferredSize().getWidth();
                        getColumnModel().getColumn(i).setPreferredWidth(nColumnWidth);
                        getColumnModel().getColumn(i).setMinWidth(nColumnWidth / 2);
                        if (i < getColumnCount() - 1)
                                getColumnModel().getColumn(i).setMaxWidth(nColumnWidth * 10);

                        // On affecte l'editeur a la colonne
                        getColumnModel().getColumn(i).setCellEditor(guiElement.getCellElement());
                }
        }

        /**
         * Modifie l'index de la ligne selectionnee dans la liste c++, et ecrit cette
         * modification dans le scenario
         *
         * @param index Le nouvel index de la ligne selectionnee
         */
        public void updateIndex(int index)
        {

                // Si la selection physique differe de la selection logique
                if (index != guiList.getSelectedItemIndex() && 0 < getRowCount() && index < getRowCount()) {

                        // On met a jour l'index selectionne cote c++
                        guiList.setSelectedItemIndex(index);

                        // On ecrit le nouvel index dans le scenario
                        guiList.writeOutputUnitListIndexCommand("" + index);
                }
        }

        /**
         * Initialise la gestion des info-bulle pour les colonnes de l'entete de la
         * table
         */
        public void initializeHeaderTooltips()
        {
                ColumnHeaderToolTips tips = new ColumnHeaderToolTips();
                getTableHeader().addMouseMotionListener(tips);
        }

        /**
         * Classe permettant d'afficher les info-bulles sur les colonne d'entete de la
         * table
         *
         * @param sStyle Le style de la liste
         * @return La classe de la liste deduite du style
         */
        class ColumnHeaderToolTips extends MouseMotionAdapter
        {
                int vCurrentColIndex = -1;

                public void mouseMoved(MouseEvent evt)
                {
                        int vColIndex = getColumnModel().getColumnIndexAtX(evt.getX());
                        if (vColIndex >= 0 && vColIndex != vCurrentColIndex) {
                                GUIObject guiObject = (GUIObject)columns.elementAt(vColIndex);
                                guiObject.setComponentHelpText(getTableHeader());
                                vCurrentColIndex = vColIndex;
                        }
                }
        }
}
