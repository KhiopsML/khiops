// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import java.awt.GridBagConstraints;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.PlainDocument;
import normGUI.engine.GUIStringElement;

/**
 * Represente l'element graphique correspondant au style TextArea. Affichage
 * d'une chaine de caracteres dans une zone de texte multiligne. La zone de
 * texte multiligne est disposee dans un JScrollPane. Utilisation de JTextArea,
 * JScrollPane.
 */
public class GUIStringElementTextArea extends GUIStringElement
{

        /**
         * Renvoie le composant d'affichage (JTextArea) contenant la chaine de
         * caracteres
         *
         * @return Le composant d'affichage (JTextArea)
         */
        protected JComponent buildComponent()
        {
                int rows = 5;
                int columns = 30;
                String[] sizeParameters;

                // On regarde s'il y a un parametrage specifique de la taille
                sizeParameters = getParametersAsArray();
                if (sizeParameters != null && sizeParameters.length == 2) {
                        try {
                                int value1 = Integer.parseInt(sizeParameters[0]);
                                int value2 = Integer.parseInt(sizeParameters[1]);
                                if (0 <= value1 && value1 <= 100 && 0 <= value2 && value2 <= 500) {
                                        rows = value1;
                                        columns = value2;
                                }
                        } catch (Exception ex) {
                                rows = 5;
                                columns = 30;
                        }
                }

                // Creation du widget
                JTextArea jta = new JTextArea(rows, columns);

                // On limite la saisie a la longueur maximale
                jta.setDocument(new PlainDocument() {
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
                jta.setEnabled(getEditable());
                jta.setName(getIdentifier());
                setComponentHelpText(jta);
                return jta;
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

                // Positionnement du libelle, si est non vide
                if (!getLabel().equals("")) {
                        JLabel label = new JLabel(getLabel());
                        setComponentHelpText(label);
                        constraints.weightx = 1;
                        constraints.gridwidth = GridBagConstraints.REMAINDER;
                        panel.add(label, constraints);
                }
                // Sinon, on permet au composant d'occuper tout l'espace
                else
                        constraints.gridx = 0;

                // Positionnement du champs de saisie
                component = buildComponent();
                component.addFocusListener(this);
                JScrollPane scrollPane = new JScrollPane(component);
                scrollPane.getViewport().setPreferredSize(component.getPreferredSize());
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(scrollPane, constraints);

                // On restitue la contrainte sur la position
                constraints.gridx = -1;
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue() { return ((JTextArea)component).getText(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JTextArea)component).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextArea)component).selectAll(); }
}
