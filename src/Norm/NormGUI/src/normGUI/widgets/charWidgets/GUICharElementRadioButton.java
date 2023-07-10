// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.charWidgets;

import java.awt.GridBagConstraints;
import java.awt.GridLayout;
import java.awt.Insets;
import java.util.Enumeration;
import javax.swing.ButtonGroup;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import normGUI.engine.GUICharElement;

/**
 * Represente l'element graphique correspondant au style RadioButton. Affichage
 * de caracteres dans des boutons radio. Un seul choix est possible. Les
 * differents boutons sont deduits a partir de l'attribut parameters, chaque
 * libelle de bouton etant separe par un ; Utilisation de JPanel, JRadioButton.
 *
 * @author Marc Boulle
 */
public class GUICharElementRadioButton extends GUICharElement
{

        private ButtonGroup buttonGroup;

        /**
         * Renvoie le composant d'affichage (JPanel) contenant les caracteres
         *
         * @return Le composant d'affichage (JPanel)
         */
        protected JComponent buildComponent()
        {
                JPanel panel = new JPanel();
                panel.setName(getIdentifier());
                if (getParametersAsArray() != null) {
                        String[] params = getParametersAsArray();
                        panel.setLayout(new GridLayout(params.length, 1));
                        JRadioButton button;
                        buttonGroup = new ButtonGroup();
                        for (int i = 0; i < params.length; i++) {
                                button = new JRadioButton(params[i]);
                                button.setActionCommand(params[i]);
                                button.addFocusListener(this);
                                button.setEnabled(getEditable());
                                button.setMargin(new Insets(2, 0, 2, 2));
                                setComponentHelpText(button);
                                setComponentPreferredSize(button, 10);
                                buttonGroup.add(button);
                                panel.add(button);
                        }
                }
                return panel;
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

                // On met le label a la premiere ligne d'un panel avec un gridlayout comportant
                // le meme
                // nombre de lignes que le nombre de boutons afin de d'aligner le label avec le
                // premier bouton
                JPanel labelPanel = new JPanel(new GridLayout(getParametersAsArray().length, 1));
                labelPanel.add(label);

                // On ajoute ce panel
                constraints.weightx = 0;
                constraints.gridwidth = GridBagConstraints.RELATIVE;
                panel.add(labelPanel, constraints);

                // Positionnement du panel contenant boutons
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
        protected String graphicGetValue() { return buttonGroup.getSelection().getActionCommand(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                Enumeration e = buttonGroup.getElements();
                while (e.hasMoreElements()) {
                        JRadioButton button = (JRadioButton)e.nextElement();
                        if (button.getText().equals(sValue)) {
                                button.setSelected(true);
                                break;
                        }
                }
        }
}
