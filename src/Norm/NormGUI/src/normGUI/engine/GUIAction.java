// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Font;
import java.awt.Insets;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.SwingConstants;

/**
 * Definit une action utilisateur
 */
public class GUIAction extends GUIObject
{
        /**
         * Indique si l'action est de type "Button" ou "SmallButton"
         */
        public boolean isButton() { return getStyle().equals("Button") || getStyle().equals("SmallButton"); };

        /**
         * Creation d'un widget bouton selon les specification de l'action
         *
         * @return Le bouton cree
         */
        public JButton createButton()
        {
                JButton actionButton;
                if (getStyle().equals("SmallButton")) {
                        // On utilise un bouton de reference, dont on reduit ensuite la taille de la
                        // police
                        actionButton = new JButton("");
                        Font buttonFont = actionButton.getFont();
                        Font smallButtonFont =
                          new Font(buttonFont.getName(), buttonFont.getStyle(), (int)(0.9 * buttonFont.getSize()));
                        actionButton.setFont(smallButtonFont);
                        actionButton.setText(getLabel());
                        actionButton.setMargin(new Insets(2, 2, 2, 2));
                } else
                        actionButton = new JButton(getLabel());
                applyOptionalIcon(actionButton);
                actionButton.setActionCommand(getIdentifier());
                if (getShortCut() != ' ')
                        actionButton.setMnemonic((int)getShortCut());
                setComponentHelpText(actionButton);
                return actionButton;
        };

        /**
         * Applique une icone Lucide optionnelle au bouton d'action, si specifiee dans les parametres
         */
        private void applyOptionalIcon(JButton actionButton)
        {
                Icon icon = GUIIconLoader.loadSvgIcon(GUIIconLoader.resolveIconPath(getParameters()), null, null);
                if (icon != null) {
                        actionButton.setIcon(icon);
                        actionButton.setIconTextGap(8);
                        actionButton.setHorizontalTextPosition(SwingConstants.RIGHT);
                }
        }

        /**
         * Cle d'acceleration associee a l'action
         */
        private String sAccelKey;

        /**
         * Renvoie la cle d'acceleration
         *
         * @return La cle d'acceleration
         */
        public String getAccelKey() { return sAccelKey; }

        /**
         * Modifie de la cle d'acceleration
         *
         * @param sValue La nouvelle cle d'acceleration
         */
        public void setAccelKey(String sValue) { sAccelKey = sValue; }
}
