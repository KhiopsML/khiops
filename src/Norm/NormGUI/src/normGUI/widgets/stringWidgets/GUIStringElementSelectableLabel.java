// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import java.awt.GridBagConstraints;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextPane;
import normGUI.engine.GUIStringElement;

/**
 * Represente l'element graphique correspondant au style JLabel. Affichage d'une
 * chaine de caracteres dans une zone de texte potentiellement au format html.
 * Utilisation de JTextPane en mode non editable.
 */
public class GUIStringElementSelectableLabel extends GUIStringElement
{

        /**
         * Renvoie le composant d'affichage (JTextPane) contenant la chaine de
         * caracteres
         *
         * @return Le composant d'affichage (JTextPane)
         */
        protected JComponent buildComponent()
        {
                // Creation du widget
                JTextPane jtp = new JTextPane();
                jtp.setName(getIdentifier());
                jtp.setContentType("text/html");
                jtp.setEditable(false);
                jtp.setBackground(null);
                jtp.setBorder(null); // remove the borderlabel
                setComponentHelpText(jtp);
                return jtp;
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
                        constraints.weightx = 0;
                        constraints.gridwidth = GridBagConstraints.RELATIVE;
                        panel.add(label, constraints);
                }
                // Sinon, on permet au composant d'occuper tout l'espace
                else
                        constraints.gridx = 0;

                // Positionnement du champs qui est ici un libelle
                component = buildComponent();
                component.addFocusListener(this);
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(component, constraints);

                // On restitue la contrainte sur la position
                constraints.gridx = -1;
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue() { return ((JTextPane)component).getText(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue) { ((JTextPane)component).setText(sValue.trim()); }

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() { ((JTextPane)component).selectAll(); }
}
