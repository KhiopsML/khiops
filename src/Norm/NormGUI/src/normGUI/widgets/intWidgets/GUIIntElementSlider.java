// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.intWidgets;

import java.awt.GridBagConstraints;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import normGUI.engine.GUIIntElement;

/**
 * Represente l'element graphique correspondant au style Slider. Affichage d'un
 * entier au sein d'une regle (slider). Les bornes inferieures et superieures de
 * la regles sont determinees a partir des valeurs min et max de l'element.
 * Utilisation de JSlider.
 */
public class GUIIntElementSlider extends GUIIntElement
{

        /**
         * Renvoie le composant d'affichage (JSlider) contenant l'entier
         *
         * @return Le composant d'affichage (JSlider)
         */
        protected JComponent buildComponent()
        {
                int min = getMinValue();
                int max = getMaxValue();

                // Calcul de l'intervalle entre les ticks, en arrondissant a une puissance de 10
                int major = (max - min) / 4;
                if (major < 1)
                        major = 1;
                int majorUnit = (int)Math.pow(10, (int)(Math.log(major) / Math.log(10)));
                if (majorUnit < 1)
                        majorUnit = 1;
                major = majorUnit * (major / majorUnit);
                int minor = major / 5;
                if (minor < 1)
                        minor = 1;

                JSlider js = new JSlider(min, max);
                js.setName(getIdentifier());
                js.setEnabled(getEditable());
                js.setMajorTickSpacing(major);
                js.setMinorTickSpacing(minor);
                js.setSnapToTicks(true);
                js.setPaintTicks(true);
                js.setPaintLabels(true);
                setComponentHelpText(js);
                return js;
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

                // Positionnement en TOP pour s'aligner avec la barre horizontale du Slider
                label.setVerticalAlignment(JLabel.TOP);

                // Positionnement du slider
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
                Integer nValue = new Integer(((JSlider)component).getValue());
                return nValue.toString();
        }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                try {
                        ((JSlider)component).setValue(Integer.parseInt(sValue));
                } catch (Exception ex) {
                }
        }
}
