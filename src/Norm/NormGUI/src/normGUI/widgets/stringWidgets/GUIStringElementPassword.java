// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import java.awt.Component;
import java.awt.GridBagConstraints;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JTable;
import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.PlainDocument;
import normGUI.engine.GUIStringElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique correspondant au style Password. Affichage
 * d'une chaine de caracteres dans une zone de texte de type mot de passe. Les
 * caracteres de saisie sont remplaces par des points a l'affichage. Utilisation
 * de JPasswordField.
 */
public class GUIStringElementPassword extends GUIStringElement
{

        /**
         * Renvoie le composant d'affichage (JPasswordField) contenant la chaine de
         * caracteres
         *
         * @return Le composant d'affichage (JPasswordField)
         */
        protected JComponent buildComponent()
        {
                JPasswordField jpf = new JPasswordField();
                // On limite la saisie a la longueur maximale
                jpf.setDocument(new PlainDocument() {
                        public void insertString(int offset, String str, AttributeSet attSet)
                          throws BadLocationException
                        {
                                boolean valid = true;
                                if (str == null)
                                        return;
                                String old = getText(0, getLength());
                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                if (newStr.length() > getMaxLength())
                                        valid = false;
                                if (valid)
                                        super.insertString(offset, str, attSet);
                        }
                });
                jpf.setEnabled(getEditable());
                jpf.setName(getIdentifier());
                setComponentHelpText(jpf);
                setComponentPreferredSize(jpf, 10);
                return jpf;
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
                                // Important de faire un new String sinon chaine erronee
                                return new String(((JPasswordField)editorComponent).getPassword());
                        }

                        public Component getTableCellEditorComponent(
                          JTable table, Object value, boolean isSelected, int row, int column)
                        {
                                ((JPasswordField)editorComponent).setText(value.toString());
                                return editorComponent;
                        }

                        public Component getTableCellRendererComponent(
                          JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
                        {
                                JPasswordField passwordField = (JPasswordField)buildComponent();

                                // Parametrage des couleurs
                                GUIUnit.initializeTextFieldColors(passwordField, getEditable(), isSelected, hasFocus);

                                passwordField.setText(value.toString());
                                return passwordField;
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

                // Positionnement du champs de saisie
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
                // Important de faire un new String une chaine erronee est envoyee
                return new String(((JPasswordField)component).getPassword());
        }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JPasswordField)component).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JPasswordField)component).selectAll(); }
}
