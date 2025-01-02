// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
import normGUI.engine.GUIFileDirectoryChooser;
import normGUI.engine.GUIList;
import normGUI.engine.GUIStringElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique correspondant au style FileChooser. Affichage
 * d'une chaine de caracteres dans une zone de texte suivie d'un bouton. Le
 * bouton declenche l'ouverture d'une fenetre permettant de selectionner un
 * fichier (JFileChooser). Cette fenetre s'ouvre en pointant sur le repertoire
 * du dernier fichier selectionne. Utilisation de JPanel, JTextField, JButton et
 * JFileChooser.
 *
 * @author Marc Boulle
 */
public class GUIStringElementFileDirectoryChooser extends GUIStringElement implements ActionListener
{

        /**
         * Appelee lors d'un click sur le bouton (lorsque le widget est insere dans une
         * fiche)
         */
        public void actionPerformed(ActionEvent e)
        {
                showFileChooser((JTextField)component.getComponent(0), null, -1, -1);
        }

        /**
         * Ouverture d'un file chooser, et mise a jour de la donnee texte Les donnees de
         * contexte (guiList...) sont a null en cas d'appel depuis une fiche
         */
        public void showFileChooser(JTextField textField, GUIList guiList, int currentRow, int currentColumn)
        {
                // Creation et parametrage d'un FileChooser
                JFileChooser jfc = GUIFileDirectoryChooser.createFileChooser(
                  textField.getText().trim(), getSelectionMode(), getParametersAsArray());

                // Parametrage des libelles
                jfc.setDialogTitle(getLabel());
                jfc.setApproveButtonText("OK");

                // On provoque une perte de focus du champ texte avant l'ouverture du
                // FileChooser
                FocusEvent focusEvent = new FocusEvent(textField, FocusEvent.FOCUS_LOST, false, jfc);
                textField.dispatchEvent(focusEvent);

                // Acces a la fenetre mere
                GUIUnit guiUnit = getParentUnit();
                GUIUnit parentRoot = guiUnit.getParentRoot();

                // On previent la liste qu'un changement a ete effectue, dans le cas d'une
                // liste, pour memoriser l'index courant
                if (guiList != null)
                        guiList.getGUITable().updateIndex(currentRow);

                // On ouvre le JFileChooser, en indiquant qu'une action (systeme) est en cours
                // pour se proteger lors de la gestion des pertes de focus
                parentRoot.setActionRunning(true);

                // Ouverture du FileChooser
                int dialogResult = JFileChooser.CANCEL_OPTION;
                try {
                        dialogResult = jfc.showOpenDialog(parentRoot.frame);
                } catch (Exception e) {
                        dialogResult = JFileChooser.CANCEL_OPTION;
                }

                // Traitement de son retour si validation utilisateur
                if (dialogResult == JFileChooser.APPROVE_OPTION) {
                        // Mise a jour de la valeur
                        textField.setText(jfc.getSelectedFile().getPath());
                        graphicSetValue(jfc.getSelectedFile().getPath());
                        updateElement();

                        // On previent la liste qu'un changement a ete effectue, dans le cas d'une liste
                        if (guiList != null) {
                                guiUnit.graphicFireDataChange();
                                guiUnit.graphicRefreshAll();
                        }
                }
                parentRoot.setActionRunning(false);

                // On redonne le focus au texte
                textField.requestFocus();
                textField.selectAll();
        }

        /**
         * Renvoie le composant d'affichage (JPanel) contenant la chaine de caracteres
         *
         * @return Le composant d'affichage (JPanel)
         */
        protected JComponent buildComponent()
        {

                // TextField
                JTextField textField = new JTextField();
                setComponentPreferredSize(textField, 30);
                setComponentHelpText(textField);
                textField.setEnabled(getEditable());

                // Bouton
                JButton button = new JButton("...");
                setComponentPreferredSize(button, 2);
                if (getSelectionMode() == JFileChooser.DIRECTORIES_ONLY)
                        button.setToolTipText("Click to choose directory");
                else if (getSelectionMode() == JFileChooser.FILES_ONLY)
                        button.setToolTipText("Click to choose file");
                else
                        button.setToolTipText("Click to choose file or directory");
                button.setEnabled(getEditable());
                button.setFocusable(false);

                // Panel
                JPanel panel = new JPanel();
                panel.setLayout(new BorderLayout());
                panel.add(textField, BorderLayout.CENTER);
                panel.add(button, BorderLayout.EAST);
                setComponentPreferredSize(panel, 32);
                panel.setName(getIdentifier());

                // Lorsque le panneau gagne le focus, il faut donner le focus au textField
                panel.addFocusListener(new FocusAdapter() {
                        public void focusGained(FocusEvent e)
                        {
                                if (e.getOppositeComponent() != null &&
                                    (!e.isTemporary() || !getParentUnit().getActionRunning())) {
                                        // On donne le focus au texte et on le selectionne
                                        JPanel panel = (JPanel)e.getSource();
                                        JTextField textField = ((JTextField)panel.getComponent(0));
                                        textField.requestFocus();
                                        textField.selectAll();
                                }
                        }
                });
                return panel;
        }

        /**
         * Renvoie l'element insere dans un tableau, cet element communiquera avec la
         * JTable
         *
         * @return L'element insere dans le tableau
         */
        public CellElement getCellElement()
        {
                return new CellElement() {
                        public Object getCellEditorValue()
                        {
                                return ((JTextField)editorComponent.getComponent(0)).getText();
                        }

                        public Component getTableCellEditorComponent(
                          JTable table, Object value, boolean isSelected, int row, int column)
                        {
                                final JTextField textField = (JTextField)editorComponent.getComponent(0);
                                // Memorisation de la position courante
                                currentRow = row;
                                currentColumn = column;

                                // Parametrage du champ de saisie
                                textField.setName(getIdentifier());
                                textField.setText(value.toString());
                                return editorComponent;
                        }

                        public Component getTableCellRendererComponent(
                          JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
                        {
                                JPanel panel = (JPanel)buildComponent();
                                JTextField textField = (JTextField)panel.getComponent(0);
                                JButton button = (JButton)panel.getComponent(1);

                                // Parametrage des couleurs
                                GUIUnit.initializeTextFieldColors(textField, getEditable(), isSelected, hasFocus);
                                GUIUnit.initializeComponentColors(button, getEditable(), isSelected, hasFocus);

                                // Memorisation de la valeur
                                textField.setText(value.toString());
                                return panel;
                        }

                        public void setListeners()
                        {
                                ((JButton)editorComponent.getComponent(1)).addActionListener(new ActionListener() {
                                        public void actionPerformed(ActionEvent e)
                                        {
                                                JTextField textField = (JTextField)editorComponent.getComponent(0);
                                                GUIUnit guiUnit = getParentUnit();
                                                GUIList guiList = (GUIList)guiUnit;
                                                showFileChooser(textField, guiList, currentRow, currentColumn);
                                        }
                                });
                        }

                        /**
                         * Position courante
                         */
                        private int currentRow;
                        private int currentColumn;
                };
        }

        /**
         * Ajoute le composant d'affichage et son libelle dans le panel de l'unite mere
         *
         * @param panel       Panneau de l'unite mere dans lequel sera ajoute le
         *                    composant d'affichage
         * @param constraints Contraintes d'affichage
         */
        public void graphicAddField(JPanel panel, GridBagConstraints constraints)
        {

                // Positionnement du libelle
                JLabel label = new JLabel(getLabel());
                setComponentHelpText(label);
                constraints.weightx = 0;
                constraints.gridwidth = GridBagConstraints.RELATIVE;
                panel.add(label, constraints);

                // Positionnement du panel
                component = buildComponent();
                component.getComponent(0).addFocusListener(this);
                ((JButton)component.getComponent(1)).addActionListener(this);
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(component, constraints);
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue() { return ((JTextField)component.getComponent(0)).getText(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JTextField)component.getComponent(0)).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextField)component.getComponent(0)).selectAll(); }

        /**
         * Indique le mode de selection du composant
         */
        protected int getSelectionMode() { return JFileChooser.FILES_AND_DIRECTORIES; }
}
