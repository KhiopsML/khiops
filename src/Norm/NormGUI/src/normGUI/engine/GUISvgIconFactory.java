// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Color;
import java.util.function.BooleanSupplier;
import java.util.function.Function;
import javax.swing.Icon;

/**
 * Isole la dependance optionnelle a flatlaf-extras.
 */
final class GUISvgIconFactory
{
        private static final int ICON_HEIGHT = 16;

        static Icon create(String iconPath, Color highlightColor, BooleanSupplier isHighlighted)
          throws ReflectiveOperationException
        {
                Class<?> iconClass = Class.forName("com.formdev.flatlaf.extras.FlatSVGIcon");
                Object icon = iconClass.getConstructor(String.class).newInstance(iconPath);
                if (!(Boolean)iconClass.getMethod("hasFound").invoke(icon))
                        return null;
                // Redimensionne l'icone pour qu'elle ait une hauteur de 16 pixels en preservant ses proportions
                int sourceWidth = (Integer)iconClass.getMethod("getIconWidth").invoke(icon);
                int sourceHeight = (Integer)iconClass.getMethod("getIconHeight").invoke(icon);
                if (sourceWidth > 0 && sourceHeight > 0) {
                        int iconWidth = Math.max(1, Math.round((float)ICON_HEIGHT * sourceWidth / sourceHeight));
                        icon = iconClass.getMethod("derive", int.class, int.class).invoke(icon, iconWidth, ICON_HEIGHT);
                }
                if (highlightColor != null && isHighlighted != null) {
                        Class<?> filterClass = Class.forName("com.formdev.flatlaf.extras.FlatSVGIcon$ColorFilter");
                        Function<Color, Color> colorMapper =
                          color -> isHighlighted.getAsBoolean() ? highlightColor : color;
                        Object filter = filterClass.getConstructor(Function.class).newInstance(colorMapper);
                        iconClass.getMethod("setColorFilter", filterClass).invoke(icon, filter);
                }
                return (Icon)icon;
        }
}
