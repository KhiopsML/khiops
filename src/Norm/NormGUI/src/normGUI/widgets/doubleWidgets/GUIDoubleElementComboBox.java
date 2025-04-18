// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.doubleWidgets;

import java.awt.Component;
import java.awt.GridBagConstraints;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTable;
import normGUI.engine.GUIDoubleElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique correspondant au style ComboBox. Affichage de
 * reels dans une zone de liste. Utilisation d'une JComboBox.
 */
public class GUIDoubleElementComboBox extends GUIDoubleElement
{

        /**
         * Renvoie le composant d'affichage (JComboBox) contenant les reels
         *
         * @return Le composant d'affichage (JComboBox)
         */
        protected JComponent buildComponent()
        {
                JComboBox jcb;
                if (getItemsArrayOfDouble() != null)
                        jcb = new JComboBox<Double>(getItemsArrayOfDouble());
                else
                        jcb = new JComboBox();
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
                                return ((JComboBox)editorComponent).getSelectedItem();
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
                                GUIUnit.initializeComponentColors(comboBox, getEditable(), isSelected, hasFocus);

                                comboBox.setSelectedItem(value.toString());
                                return comboBox;
                        }
                };
        }

        /**
         * Renvoie un tableau de reels a partir des parametres
         *
         * @return Le tableau de reel construit a partir des parametres
         */
        private Double[] getItemsArrayOfDouble()
        {
                if (getParametersAsArray() != null) {
                        String[] sParams = getParametersAsArray();
                        Double[] dParams = new Double[sParams.length];
                        for (int i = 0; i < sParams.length; i++)
                                dParams[i] = Double.valueOf(sParams[i]);
                        return dParams;
                } else
                        return null;
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
                if (((JComboBox)component).getSelectedIndex() != -1)
                        return ((JComboBox)component).getSelectedItem().toString();
                else
                        return "";
        }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JComboBox)component).setSelectedItem(Double.valueOf(sValue)); }
}
