// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.booleanWidgets;

import java.awt.GridBagConstraints;
import java.awt.GridLayout;
import java.awt.Insets;
import javax.swing.ButtonGroup;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import normGUI.engine.GUIBooleanElement;

/**
 * Represente l'element graphique correspondant au style RadioButton. Affichage
 * de booleens dans des boutons radio. Un seul choix est possible. Les
 * differents boutons sont deduits a partir de l'attribut parameters, chaque
 * libelle de bouton etant separe par un ; Utilisation de JPanel, JRadioButton.
 */
public class GUIBooleanElementRadioButton extends GUIBooleanElement
{

        private ButtonGroup bg;

        /**
         * Renvoie le composant d'affichage (JPanel) contenant les booleens
         *
         * @return Le composant d'affichage (JPanel)
         */
        protected JComponent buildComponent()
        {

                // Button true
                JRadioButton buttonTrue = new JRadioButton("true");
                buttonTrue.setActionCommand("true");
                buttonTrue.setMargin(new Insets(2, 0, 2, 2));
                buttonTrue.setEnabled(getEditable());
                setComponentHelpText(buttonTrue);
                setComponentPreferredSize(buttonTrue, 10);
                buttonTrue.addFocusListener(this);

                // Button false
                JRadioButton buttonFalse = new JRadioButton("false");
                buttonFalse.setActionCommand("false");
                buttonFalse.setMargin(new Insets(2, 0, 2, 2));
                buttonFalse.setEnabled(getEditable());
                setComponentHelpText(buttonFalse);
                setComponentPreferredSize(buttonFalse, 10);
                buttonFalse.addFocusListener(this);

                // Button group
                bg = new ButtonGroup();
                bg.add(buttonTrue);
                bg.add(buttonFalse);

                // Panel
                JPanel jp = new JPanel(new GridLayout(2, 1));
                jp.setName(getIdentifier());
                jp.add(buttonTrue);
                jp.add(buttonFalse);
                return jp;
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

                // Creation du libelle
                JLabel label = new JLabel(getLabel());
                setComponentHelpText(label);

                // On met le label a la premiere ligne d'un panel a 2 lignes pour l'aligner avec
                // le true
                JPanel labelPanel = new JPanel(new GridLayout(2, 1));
                labelPanel.add(label);

                // On ajoute ce panel au panel general
                constraints.weightx = 0;
                constraints.gridwidth = GridBagConstraints.RELATIVE;
                panel.add(labelPanel, constraints);

                // Positionnement du panel contenant les boutons
                component = buildComponent();
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(component, constraints);
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue() { return bg.getSelection().getActionCommand(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                if (sValue.trim() == "true")
                        ((JRadioButton)component.getComponent(0)).setSelected(true);
                else
                        ((JRadioButton)component.getComponent(1)).setSelected(true);
        }
}
