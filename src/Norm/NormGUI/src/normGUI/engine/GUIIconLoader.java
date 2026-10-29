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
         * Charge une icone 16x16 avec une couleur de surbrillance optionnelle.
         */
        public static Icon loadSvgIcon(String iconPath, Color highlightColor, BooleanSupplier isHighlighted)
        {
                if (iconPath == null || iconPath.length() == 0 || !svgAvailable)
                        return null;

                try {
                        return GUISvgIconFactory.create(iconPath, highlightColor, isHighlighted);
                } catch (ReflectiveOperationException e) {
                        svgAvailable = false;
                        return null;
                } catch (LinkageError e) {
                        svgAvailable = false;
                        return null;
                }
        }
}
