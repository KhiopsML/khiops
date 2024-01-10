// Copyright (c) 2024 Orange. All rights reserved.
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
 * Definit un element de type chaine de caracteres
 *
 * @author Marc Boulle
 */
public abstract class GUIStringElement extends GUIElement
{

        /**
         * Longueur maximale de la chaine
         */
        private int nMaxLength;

        /**
         * Longueur minimale de la chaine
         */
        private int nMinLength;

        /**
         * Valeur par defaut de l'element
         */
        private String sDefaultValue;

        /**
         * Cree un element de type chaine
         */
        public GUIStringElement()
        {
                component = buildComponent();
                nMinLength = 0;
                nMaxLength = Integer.MAX_VALUE;
        }

        /**
         * Renvoie le composant d'affichage par defaut (JTextField)
         *
         * @return Le composant d'affichage par defaut
         */
        protected JComponent getDefaultComponent()
        {
                JTextField textField = new JTextField();
                // On limite la saisie a la longueur maximale
                textField.setDocument(new PlainDocument() {
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
                textField.setEnabled(getEditable());
                textField.setName(getIdentifier());
                setComponentHelpText(textField);
                setComponentPreferredSize(textField, 10);
                return textField;
        }

        /**
         * Renvoie la longueur maximale de la chaine
         *
         * @return La longueur maximale de la chaine
         */
        protected int getMaxLength() { return nMaxLength; }

        /**
         * Renvoie la longueur minimale de la chaine
         *
         * @return La longueur minimale de la chaine
         */
        protected int getMinLength() { return nMinLength; }

        /**
         * Renvoie la valeur c++ de l'element
         *
         * @param unit L'unite contenant l'element
         * @return La chaine de caracteres c++ de l'element
         */
        public Object getValueIn(GUIUnit unit)
        {
                // Appel de la methode native
                return new String(unit.getStringValueAt(getIdentifier()));
        }

        /**
         * Rafraichit la valeur graphique de l'element avec la valeur c++ renvoyee par
         * la methode native
         */
        public void graphicRefreshAll()
        {
                // On nettoie la valeur de ses blancs avant affichage
                graphicSetValue(getParentUnit().getStringValueAt(getIdentifier()).trim());
        }

        /**
         * Modifie la valeur par defaut de l'element
         *
         * @param sValue La nouvelle valeur par defaut de l'element
         */
        protected void setDefaultValue(String sValue) { sDefaultValue = sValue; }

        /**
         * Modifie la longueur maximale de la chaine
         *
         * @param nValue La nouvelle longueur maximale de la chaine
         */
        protected void setMaxLength(int nValue) { nMaxLength = nValue; }

        /**
         * Modifie la longueur minimale de la chaine
         *
         * @param nValue La nouvelle longueur minimale de la chaine
         */
        protected void setMinLength(int nValue) { nMinLength = nValue; }

        /**
         * Modifie la valeur c++ de l'element
         *
         * @param unit  L'unite contenant l'element
         * @param value La nouvelle valeur de la donnee c++ de l'element
         */
        protected void setValueIn(GUIUnit unit, Object value)
        {
                unit.setStringValueAt(getIdentifier(), new String(value.toString()));
        }
}

/**
 * Definit l'element graphique par defaut permettant l'affichage de chaines de
 * caracteres. Cette classe utilise un JTextField.
 *
 * @author Marc Boulle
 */
class GUIStringElementTextField extends GUIStringElement
{

        /**
         * Renvoie le composant d'affichage (JTextField) contenant la chaine de
         * caracteres
         *
         * @return Le composant d'affichage (JTextField)
         */
        public JComponent buildComponent() { return getDefaultComponent(); }

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
        protected String graphicGetValue() { return ((JTextField)component).getText(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JTextField)component).setText(sValue); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextField)component).selectAll(); }
}
