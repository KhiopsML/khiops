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
 * Definit un element de type reel
 */
public abstract class GUIDoubleElement extends GUIElement
{

        /**
         * Valeur par defaut de l'element
         */
        private double dDefaultValue;

        /**
         * Valeur maximale de l'element
         */
        private double dMaxValue;

        /**
         * Valeur minimale de l'element
         */
        private double dMinValue;

        /**
         * Cree un element de type reel
         */
        public GUIDoubleElement()
        {
                component = buildComponent();
                dDefaultValue = 0;
                dMinValue = Double.MIN_VALUE;
                dMaxValue = Double.MAX_VALUE;
        }

        /**
         * Renvoie le composant d'affichage par defaut (JTextField)
         *
         * @return Le composant d'affichage par defaut
         */
        protected JComponent getDefaultComponent()
        {
                JTextField jtf = new JTextField();
                // On limite la saisie aux reels
                jtf.setDocument(new PlainDocument() {
                        public void insertString(int offset, String str, AttributeSet attSet)
                          throws BadLocationException
                        {
                                boolean valid = true;
                                if (str == null)
                                        return;
                                String old = getText(0, getLength());
                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                try {
                                        double d = Double.parseDouble(newStr);
                                        if (d < dMinValue || d > dMaxValue)
                                                valid = false;
                                } catch (Exception e) {
                                        valid = false;
                                }
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
         * Renvoie la valeur maximale de l'element
         *
         * @return La valeur maximale de l'element
         */
        protected double getMaxValue() { return dMaxValue; }

        /**
         * Renvoie la valeur minimale de l'element
         *
         * @return La valeur minimale de l'element
         */
        protected double getMinValue() { return dMinValue; }

        /**
         * Renvoie la valeur c++ de l'element
         *
         * @param unit L'unite contenant l'element
         * @return Le reel c++ de l'element sous la forme de chaine de caracteres
         */
        public Object getValueIn(GUIUnit unit)
        {
                // Appel de la methode native
                // Remarque: on utilise Float plutot que Double pour limiter le nombre
                // de chiffres apres la virgule a l'affichage
                // C'est le seul endroit on on accede a la methode getDoubleValueAt, mais cela
                // reste une solution "bricolo" pour controler le formatage de l'afficahge des
                // reels
                return new Float(unit.getDoubleValueAt(getIdentifier()));
        }

        /**
         * Rafraichit la valeur graphique de l'element avec la valeur c++ renvoyee par
         * la methode native
         */
        public void graphicRefreshAll() { graphicSetValue(getValueIn(getParentUnit()).toString()); }

        /**
         * Modifie la valeur par defaut de l'element
         *
         * @param dValue La nouvelle valeur par defaut de l'element
         */
        protected void setDefaultValue(double dValue) { dDefaultValue = dValue; }

        /**
         * Modifie la valeur maximale de l'element
         *
         * @param dValue La nouvelle valeur maximale de l'element
         */
        protected void setMaxValue(double dValue) { dMaxValue = dValue; }

        /**
         * Modifie la valeur minimale de l'element
         *
         * @param dValue La nouvelle valeur minimale de l'element
         */
        protected void setMinValue(double dValue) { dMinValue = dValue; }

        /**
         * Modifie la valeur c++ de l'element
         *
         * @param unit  L'unite contenant l'element
         * @param value La nouvelle valeur de la donnee c++ de l'element
         */
        protected void setValueIn(GUIUnit unit, Object value)
        {
                double dValue;
                try {
                        dValue = Double.parseDouble(value.toString());
                } catch (NumberFormatException e) {
                        dValue = dDefaultValue;
                        graphicSetValue(dValue + "");
                }
                unit.setDoubleValueAt(getIdentifier(), dValue);
        }
}

/**
 * Definit l'element graphique par defaut permettant l'affichage de reels. Cette
 * classe utilise un JTextField.
 */
class GUIDoubleElementTextField extends GUIDoubleElement
{

        /**
         * Renvoie le composant d'affichage par defaut
         *
         * @return Le composant d'affichage par defaut
         */
        protected JComponent buildComponent() { return getDefaultComponent(); }

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
        public void graphicSetValue(String sValue) { ((JTextField)component).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextField)component).selectAll(); }
}
