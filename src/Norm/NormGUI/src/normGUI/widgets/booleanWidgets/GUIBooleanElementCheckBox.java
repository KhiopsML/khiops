// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.booleanWidgets;

import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTable;
import normGUI.engine.GUIBooleanElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique de style CheckBox permettant l'affichage de
 * booleens dans une case a cocher. Utilisation d'une JCheckBox.
 *
 * @author Marc Boulle
 */
public class GUIBooleanElementCheckBox extends GUIBooleanElement
{

        /**
         * Renvoie le composant d'affichage (JCheckBox) contenant les booleens
         *
         * @return Le composant d'affichage (JCheckBox)
         */
        protected JComponent buildComponent()
        {
                JCheckBox jcb = new JCheckBox(" ");
                jcb.setBorder(null);
                jcb.setMargin(new Insets(0, 0, 0, 0));
                jcb.setName(getIdentifier());
                jcb.setEnabled(getEditable());
                setComponentHelpText(jcb);
                setComponentPreferredSize(jcb, 10);
                return jcb;
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
                                if (((JCheckBox)editorComponent).isSelected())
                                        return "true";
                                else
                                        return "false";
                        }

                        public Component getTableCellEditorComponent(
                          JTable table, Object value, boolean isSelected, int row, int column)
                        {
                                GUIUnit.initializeComponentColors(editorComponent, getEditable(), isSelected, false);
                                ((JCheckBox)editorComponent).setHorizontalAlignment(JCheckBox.CENTER);
                                if (value.toString().equals("true"))
                                        ((JCheckBox)editorComponent).setSelected(true);
                                else
                                        ((JCheckBox)editorComponent).setSelected(false);
                                return editorComponent;
                        }

                        public Component getTableCellRendererComponent(
                          JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
                        {
                                JCheckBox checkBoxRenderer = (JCheckBox)buildComponent();
                                checkBoxRenderer.setHorizontalAlignment(JCheckBox.CENTER);

                                // Parametrage des couleurs
                                GUIUnit.initializeComponentColors(
                                  checkBoxRenderer, getEditable(), isSelected, hasFocus);

                                if (value.toString() == "true")
                                        checkBoxRenderer.setSelected(true);
                                return checkBoxRenderer;
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

                // Positionnement de la checkbox
                component = buildComponent();
                component.addFocusListener(this);
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
                if (((JCheckBox)component).isSelected())
                        return "true";
                else
                        return "false";
        }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                if (sValue.trim() == "true")
                        ((JCheckBox)component).setSelected(true);
                else
                        ((JCheckBox)component).setSelected(false);
        }
}