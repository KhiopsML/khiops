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
                applyOptionalLucideIcon(actionButton);
                actionButton.setActionCommand(getIdentifier());
                if (getShortCut() != ' ')
                        actionButton.setMnemonic((int)getShortCut());
                setComponentHelpText(actionButton);
                return actionButton;
        };

        /**
         * Applique une icône Lucide optionnelle au bouton d'action, si spécifiée dans les paramètres
         */
        private void applyOptionalLucideIcon(JButton actionButton)
        {
                String iconPath = resolveIconPathFromParameters();

                if (iconPath == null)
                        return;

                Icon icon = loadLucideSvgIcon(iconPath);
                if (icon != null) {
                        actionButton.setIcon(icon);
                        actionButton.setIconTextGap(8);
                        actionButton.setHorizontalTextPosition(SwingConstants.RIGHT);
                }
        }

        /**
         * Résout le chemin de l'icône à partir des paramètres de l'action
         */
        private String resolveIconPathFromParameters()
        {
                String parameters = getParameters();

                if (parameters == null || parameters.length() == 0)
                        return null;

                String[] tokens = parameters.split("\\n");
                for (String rawToken : tokens) {
                        String token = rawToken.trim();
                        if (!token.startsWith("icon="))
                                continue;

                        String iconValue = token.substring("icon=".length()).trim();
                        if (iconValue.length() == 0)
                                return null;

                        if (iconValue.equals("rocket") || iconValue.equals("sparkles"))
                                return "images/" + iconValue + ".svg";

                        return iconValue;
                }
                return null;
        }

        /**
         * Charge l'icône SVG Lucide à partir du chemin spécifié, ou retourne null si elle ne peut pas être trouvée
         */
        private Icon loadLucideSvgIcon(String iconPath)
        {
                try {
                        Class<?> svgIconClass = Class.forName("com.formdev.flatlaf.extras.FlatSVGIcon");
                        Object svgIcon = svgIconClass.getConstructor(String.class, int.class, int.class)
                                           .newInstance(iconPath, 16, 16);
                        if (!(svgIcon instanceof Icon))
                                return null;

                        try {
                                Object hasFound = svgIconClass.getMethod("hasFound").invoke(svgIcon);
                                if (hasFound instanceof Boolean && !((Boolean)hasFound).booleanValue())
                                        return null;
                        } catch (Throwable ignored) {
                                // hasFound is optional; ignore and use icon if present.
                        }

                        return (Icon)svgIcon;
                } catch (Throwable ignored) {
                        return null;
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
