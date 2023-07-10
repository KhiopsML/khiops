// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.extensions;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.util.Vector;
import javax.swing.JButton;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.KeyStroke;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import normGUI.engine.GUIAction;
import normGUI.engine.GUICard;
import normGUI.engine.GUIData;
import normGUI.engine.GUIUnit;

/**
 * Cette classe represente une unite d'interface de type fiche
 *
 * @author Marc Boulle
 */
public class GUICardTabbedPanes extends GUICard implements ChangeListener, FocusListener
{

        /**
         * Onglets regroupant les sous unites
         */
        private JTabbedPane tabbedPanes;

        /**
         * Construit le menu associe a l'unite
         */
        protected void buildMenuBar()
        {

                menuBar = new JMenuBar();

                // Creation du menu des actions de l'unite
                JMenu menu = new JMenu(getLabel());
                menu.setName(getIdentifier());
                if (getShortCut() != ' ')
                        menu.setMnemonic((int)getShortCut());
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                for (int i = 0; i < vectorOfGUIActions.size(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && guiAction.getStyle().equals("") &&
                            !guiAction.getIdentifier().equals(Exit) && !guiAction.getIdentifier().equals(Refresh)) {
                                JMenuItem menuItem = new JMenuItem(guiAction.getLabel());
                                menuItem.setActionCommand(guiAction.getIdentifier());
                                menuItem.setAccelerator(KeyStroke.getKeyStroke(guiAction.getAccelKey()));
                                if (guiAction.getShortCut() != ' ')
                                        menuItem.setMnemonic((int)guiAction.getShortCut());
                                guiAction.setComponentHelpText(menuItem);
                                menuItem.addActionListener(this);
                                menu.add(menuItem);
                        }
                }
                if (menu.getItemCount() > 0)
                        menuBar.add(menu);

                // Ajout de sous-menus pour les composants graphique qui en ont un
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                for (int i = 0; i < vectorOfGUIDatas.size(); i++) {
                        if (vectorOfGUIDatas.get(i) instanceof GUIUnit) {
                                GUIUnit guiUnit = (GUIUnit)vectorOfGUIDatas.get(i);
                                if (guiUnit.getVisible()) {
                                        JMenu jm = guiUnit.getSubMenu();
                                        if (jm != null) {
                                                if (synchronizeMenuWithPane) {
                                                        if (guiUnit.getVisibleDataCount() == 0 &&
                                                            guiUnit.getVisibleActionCount() > 0)
                                                                jm.setEnabled(true);
                                                        else
                                                                jm.setEnabled(false);
                                                }
                                                menuBar.add(jm);
                                        }
                                }
                        }
                }

                if (hasHelpText())
                        menuBar.add(getHelpMenu());
        }

        /**
         * Appelee lors du gain de focus
         */
        public void focusGained(FocusEvent e)
        {
                if (e.getOppositeComponent() != null && (!e.isTemporary() || !getParentUnit().getActionRunning()))
                        refreshMenus();
        }

        /**
         * Appelee lors de la perte de focus
         */
        public void focusLost(FocusEvent e) {}

        /**
         * Methode recursive qui ajoute a l'interface les composants graphiques
         * correspondant aux donnees de la fiche
         *
         * @param panel       Panneau dans lequel seront ajoutes les composants
         *                    graphiques
         * @param constraints Contraintes d'affichage
         */
        public void graphicAddField(JPanel panel, GridBagConstraints constraints)
        {

                // Si la fiche ne contient pas de donnees et n'a pas d'action sous forme de
                // boutons
                if (getVisibleDataCount() == 0 && !hasActionButton()) {
                        panel.setBorder(null);
                        panel = null;
                        return;
                }

                if (getVisibleDataCount() > 0 || hasActionButton()) {

                        boolean displayTabbedPanes = false;
                        tabbedPanes = new JTabbedPane();

                        // S'il n'y a pas assez de place, les onglets sont scrollables
                        // ce qui permet de garder leur sequentialite
                        tabbedPanes.setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);

                        // Parcours de la composition de l'interface pour creation des champs
                        for (int i = 0; i < getDataCount(); i++) {
                                GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                                if (guiData.getVisible()) {

                                        // Si le champ est une sous unite
                                        if (guiData instanceof GUIUnit) {

                                                GUIUnit guiUnit = (GUIUnit)guiData;

                                                // Si l'unite contient des donnees
                                                if (guiUnit.getVisibleDataCount() > 0 || guiUnit.hasActionButton()) {
                                                        // On l'affiche dans un autre panel sans bordure
                                                        GridBagConstraints gbc = new GridBagConstraints();
                                                        gbc.fill = GridBagConstraints.BOTH;
                                                        gbc.insets = new Insets(3, 3, 3, 3);
                                                        JPanel jp = new JPanel(new GridBagLayout());
                                                        jp.setName(guiData.getIdentifier());
                                                        guiUnit.graphicAddField(jp, gbc);
                                                        tabbedPanes.addTab(guiUnit.getLabel(), jp);
                                                        if (guiUnit.getShortCut() != ' ')
                                                                tabbedPanes.setMnemonicAt(tabbedPanes.getTabCount() - 1,
                                                                                          guiUnit.getShortCut());
                                                        displayTabbedPanes = true;
                                                }

                                        }
                                        // Si c'est element terminal
                                        else
                                                guiData.graphicAddField(panel, constraints);
                                }
                        }

                        if (displayTabbedPanes) {
                                constraints.weightx = 1;
                                constraints.weighty = 1;
                                constraints.gridwidth = GridBagConstraints.REMAINDER;
                                panel.add(tabbedPanes, constraints);
                                refreshMenus();
                                tabbedPanes.addChangeListener(this);
                                tabbedPanes.addFocusListener(this);

                                // On agrandit un peut la largeur preferee, trop juste avec la methode par
                                // defaut
                                Dimension preferredDimension = tabbedPanes.getPreferredSize();
                                preferredDimension.setSize(preferredDimension.getWidth() + 25,
                                                           preferredDimension.getHeight());
                                tabbedPanes.setPreferredSize(preferredDimension);
                        }
                }

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
                                if (guiAction.getVisible() && guiAction.getStyle().equals("Button")) {
                                        JButton actionButton = new JButton(guiAction.getLabel());
                                        actionButton.setActionCommand(guiAction.getIdentifier());
                                        if (guiAction.getShortCut() != ' ')
                                                actionButton.setMnemonic((int)guiAction.getShortCut());
                                        guiAction.setComponentHelpText(actionButton);
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
         * Rafraichit les menus
         */
        private void refreshMenus()
        {
                if (synchronizeMenuWithPane) {
                        for (int i = 0; i < tabbedPanes.getTabCount(); i++) {
                                String name = tabbedPanes.getComponentAt(i).getName();
                                GUIUnit unit = (GUIUnit)getFieldAt(name);
                                if (unit.getSubMenu() != null) {
                                        if (i == tabbedPanes.getSelectedIndex())
                                                unit.getSubMenu().setEnabled(true);
                                        else
                                                unit.getSubMenu().setEnabled(false);
                                }
                        }
                }
        }

        /**
         * Appelee lors d'un changement de selection dans les onglets
         *
         * @param e L'evenement de changement de selection dans les onglets
         */
        public void stateChanged(ChangeEvent e) { refreshMenus(); }

        /**
         * Synchronisatin des menus avec l'onglet selectionne
         * Depuis la V10, on ne synchronise plus les menus avec l'onglet selectionne
         * Cela cree des contraintes penible a l'utilisation
         */
        private boolean synchronizeMenuWithPane = false;
}
