// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.GridBagConstraints;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.PlainDocument;

/**
 * Definit un element de type caractere
 *
 * @author Marc Boulle
 */
public abstract class GUICharElement extends GUIElement
{

        /**
         * Valeur par defaut de l'element
         */
        private char cDefaultValue;

        /**
         * Cree un element de type caractere
         */
        public GUICharElement()
        {
                component = buildComponent();
                cDefaultValue = ' ';
        }

        /**
         * Renvoie le composant d'affichage par defaut (JTextField)
         *
         * @return Le composant d'affichage par defaut
         */
        protected JComponent getDefaultComponent()
        {
                JTextField jtf = new JTextField();
                // On limite la saisie a la longueur maximale
                jtf.setDocument(new PlainDocument() {
                        public void insertString(int offset, String str, AttributeSet attSet)
                          throws BadLocationException
                        {
                                boolean valid = true;
                                if (str == null)
                                        return;
                                String old = getText(0, getLength());
                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                if (newStr.length() > 1)
                                        valid = false;
                                if (valid)
                                        super.insertString(offset, str, attSet);
                        }
                });
                jtf.setEnabled(getEditable());
                jtf.setName(getIdentifier());
                setComponentHelpText(jtf);
                setComponentPreferredSize(jtf, 10);
                return jtf;
        }

        /**
         * Renvoie la valeur c++ de l'element
         *
         * @param unit L'unite contenant l'element
         * @return Le caractere c++ de l'element sous la forme de chaine de caracteres
         */
        public Object getValueIn(GUIUnit unit)
        {
                // Appel de la methode native
                return new Character(unit.getCharValueAt(getIdentifier()));
        }

        /**
         * Rafraichit la valeur graphique de l'element avec la valeur c++ renvoyee par
         * la methode native
         */
        public void graphicRefreshAll()
        {
                graphicSetValue(new Character(getParentUnit().getCharValueAt(getIdentifier())).toString());
        }

        /**
         * Modifie la valeur par defaut de l'element
         *
         * @param cValue La nouvelle valeur par defaut de l'element
         */
        protected void setDefaultValue(char cValue) { cDefaultValue = cValue; }

        /**
         * Modifie la valeur c++ de l'element
         *
         * @param unit  L'unite contenant l'element
         * @param value La nouvelle valeur de la donnee c++ de l'element
         */
        protected void setValueIn(GUIUnit unit, Object value)
        {
                char cValue;
                if (value.toString().length() > 0)
                        cValue = value.toString().charAt(0);
                else {
                        cValue = cDefaultValue;
                        graphicSetValue(cValue + "");
                }
                unit.setCharValueAt(getIdentifier(), cValue);
        }
}

/**
 * Definit l'element graphique par defaut permettant l'affichage de caracteres.
 * Cette classe utilise un JTextField.
 *
 * @author Marc Boulle
 */
class GUICharElementTextField extends GUICharElement
{

        /**
         * Renvoie le composant d'affichage par defaut
         *
         * @return Le composant d'affichage par defaut
         */
        protected JComponent buildComponent() { return getDefaultComponent(); }

        /**
         * Ajoute le composant d'affichage et son libelle dans le panel de l'unite
         * mere
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
        protected String graphicGetValue() { return ((JTextField)component).getText(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JTextField)component).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextField)component).selectAll(); }
}