// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Color;
import java.util.function.BooleanSupplier;
import javax.swing.Icon;

/**
 * Charge les icones SVG optionnelles sans dependance directe a FlatLaf.
 */
public class GUIIconLoader
{
        private static boolean svgAvailable = true;

        /**
         * Resout le chemin de l'icone SVG optionnelle a partir des parametres.
         */
        public static String resolveIconPath(String parameters)
        {
                if (parameters == null || parameters.length() == 0)
                        return null;

                for (String rawToken : parameters.split("\\n")) {
                        String token = rawToken.trim();
                        if (!token.startsWith("icon="))
                                continue;

                        String iconValue = token.substring("icon=".length()).trim();
                        if (iconValue.length() == 0)
                                return null;
                        if (iconValue.indexOf('/') >= 0)
                                return iconValue;
                        return "images/" + iconValue + ".svg";
                }
                return null;
        }

        /**
         * Charge une icone 16x16 avec une couleur de surbrillance optionnelle.
         */
        public static Icon loadSvgIcon(String iconPath, Color highlightColor, BooleanSupplier isHighlighted)
        {
                if (iconPath == null || !svgAvailable)
                        return null;

                try {
                        return GUISvgIconFactory.create(iconPath, highlightColor, isHighlighted);
                } catch (LinkageError e) {
                        svgAvailable = false;
                        return null;
                }
        }
}