// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.KeyboardFocusManager;
import java.awt.event.FocusEvent;
import java.util.StringTokenizer;
import java.util.Vector;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.event.PopupMenuEvent;
import javax.swing.event.PopupMenuListener;
import normGUI.engine.GUIElement;
import normGUI.engine.GUIList;
import normGUI.engine.GUIStringElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique correspondant au style EditableComboBox.
 * Affichage de chaines de caracteres dans une zone de liste editable.
 * Utilisation d'une JComboBox.
 */
@SuppressWarnings("unchecked") public class GUIStringElementHelpedComboBox extends GUIStringElement
{

        /**
         * Element source de la comboBox
         */
        private GUIElement sourceElement;

        /**
         * Liste source de la comboBox
         */
        private GUIList sourceList;

        /**
         * Renvoie le composant d'affichage (JComboBox) contenant les chaines de
         * caracteres
         *
         * @return Le composant d'affichage (JComboBox)
         */
        protected JComponent buildComponent()
        {
                findSource();
                JComboBox jcb = new JComboBox();
                jcb.setEditable(true);
                jcb.setName(getIdentifier());
                jcb.setEnabled(getEditable());
                setComponentHelpText(jcb);
                setComponentPreferredSize(jcb, 10);
                jcb.addPopupMenuListener(new PopupMenuListener() {
                        public void popupMenuCanceled(PopupMenuEvent e) {}

                        public void popupMenuWillBecomeInvisible(PopupMenuEvent e)
                        {
                                updateElement();
                                value = graphicGetValue();
                        }

                        public void popupMenuWillBecomeVisible(PopupMenuEvent e)
                        {
                                JComboBox ejcb = (JComboBox)e.getSource();

                                // Acces a la fenetre mere
                                GUIUnit guiUnit = getParentUnit();
                                GUIList guiListRef = new GUIList();

                                // On previent la liste qu'un changement a ete effectue, dans le cas d'une
                                // liste,
                                // pour memoriser l'index courant
                                if (guiUnit.getClass() == guiListRef.getClass()) {
                                        GUIList guiList = (GUIList)guiUnit;
                                        int row = guiList.getGUITable().getSelectedRow();
                                        guiList.getGUITable().updateIndex(row);
                                }

                                // On simule le focus sur la ComboBox pour forcer la mise a jour des donnees
                                // logiques,
                                // permettant le recalcul correct des listes d'aide
                                KeyboardFocusManager focusManager =
                                  KeyboardFocusManager.getCurrentKeyboardFocusManager();
                                Component focusOwner = focusManager.getFocusOwner();
                                if (focusOwner != null) {
                                        FocusEvent focusEvent = new FocusEvent(focusOwner, FocusEvent.FOCUS_GAINED);
                                        ejcb.dispatchEvent(focusEvent);
                                }

                                // Recalcul des valeurs
                                refreshComboBox(ejcb);

                                // On previent la liste qu'un changement a ete effectue, dans le cas d'une liste
                                if (guiUnit.getClass() == guiListRef.getClass()) {
                                        guiUnit.graphicFireDataChange();
                                }
                        }
                });
                return jcb;
        }

        /**
         * Recherche la liste et la colonne source pour la construction de la comboBox a
         * partir des parametres
         */
        private void findSource()
        {

                if (getParameters() == null)
                        return;

                // On recupere le chemin vers la liste
                String path = "";
                path = getParameters().substring(0, getParameters().indexOf(':'));

                // On recupere le chemin vers la colonne
                String column = "";
                column = getParameters().substring(getParameters().indexOf(':') + 1, getParameters().length());

                // On se positionne au niveau de l'unite mere
                GUIUnit guiUnit = getParentUnit().getParentRoot();

                // Tant que le chemin n'est pas parcouru on recherche dans les sous unites
                StringTokenizer st = new StringTokenizer(path, ".");
                while (st.hasMoreTokens()) {
                        guiUnit = (GUIUnit)guiUnit.getFieldAt(st.nextToken());
                        if (guiUnit == null)
                                return;
                }

                // On recupere la liste et la colonne source
                sourceList = (GUIList)guiUnit;
                sourceElement = (GUIElement)sourceList.getFieldAt(column);

                // Si la recherche est infructueuse, les parametres sont errones, on avertit
                // l'utilisateur
                if (sourceList == null || sourceElement == null) {
                        if (debug)
                                displaySystemMessage("Wrong parameters for element " + getLabel());
                }
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
                                JComboBox comboBox = (JComboBox)editorComponent;
                                return ((JTextField)comboBox.getEditor().getEditorComponent()).getText();
                        }

                        public Component getTableCellEditorComponent(
                          JTable table, Object value, boolean isSelected, int row, int column)
                        {
                                ((JComboBox)editorComponent).setSelectedItem(value);
                                return editorComponent;
                        }

                        public Component getTableCellRendererComponent(
                          JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
                        {
                                JComboBox comboBox = (JComboBox)buildComponent();

                                // Parametrage des couleurs
                                GUIUnit.initializeEditableComboBoxColors(comboBox, getEditable(), isSelected, hasFocus);

                                comboBox.setSelectedItem(value.toString());
                                return comboBox;
                        }

                        public void setListeners()
                        {
                                ((JComboBox)editorComponent).getEditor().getEditorComponent().addFocusListener(this);
                        }
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

                // Positionnement de la combobox
                component = buildComponent();
                ((JComboBox)component).getEditor().getEditorComponent().addFocusListener(this);
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(component, constraints);
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue()
        {
                return ((JTextField)((JComboBox)component).getEditor().getEditorComponent()).getText();
        }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                ((JTextField)((JComboBox)component).getEditor().getEditorComponent()).setText(sValue);
        }

        /**
         * Rafraichit les items de la comboBox a partir des valeurs de la liste source
         */
        private void refreshComboBox(JComboBox comboBox)
        {

                if (sourceList == null || sourceElement == null || !(sourceElement instanceof GUIStringElement))
                        return;

                // On execute l'action refresh pour synchroniser les tiers logiques et metiers
                // de maniere a recuperer des donnees coherentes
                // On reste dans le meme thread pour empecher toute autre action utilisateur
                // Il faut que l'implmentation cote C++ soit rapide pour ne pas bloquer
                // l'interface
                try {
                        getParentUnit().executeUserActionAt(GUIUnit.Refresh);
                } catch (Exception ex) {
                }

                // On remplit un tableau avec les items
                Vector<String> items = new Vector<String>();
                String s;
                for (int i = 0; i < sourceList.getItemNumber(); i++) {
                        sourceList.setCurrentItemIndex(i);
                        s = sourceElement.getValueIn(sourceList).toString();
                        items.add(s);
                }

                // On memorise le contenu de l'editeur avant de supprimer les items
                String previousValue = ((JTextField)comboBox.getEditor().getEditorComponent()).getText();

                // On met a jour la comboBox avec les items du tableau
                comboBox.removeAllItems();
                for (int i = 0; i < items.size(); i++)
                        comboBox.addItem(new String(items.get(i).toString()));

                // On restitue la valeur precedente
                ((JTextField)comboBox.getEditor().getEditorComponent()).setText(previousValue);

                // On positionne la valeur selectionnee
                if (items.contains(previousValue))
                        comboBox.setSelectedItem(previousValue);
                else if (items.size() > 0 && previousValue != null && !previousValue.equals(""))
                        comboBox.setSelectedItem(comboBox.getItemAt(0).toString());
        }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue()
        {
                ((JTextField)((JComboBox)component).getEditor().getEditorComponent()).selectAll();
        }
}
