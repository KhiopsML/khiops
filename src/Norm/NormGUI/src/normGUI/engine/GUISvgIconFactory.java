// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import com.formdev.flatlaf.extras.FlatSVGIcon;
import java.awt.Color;
import java.util.function.BooleanSupplier;
import javax.swing.Icon;

/**
 * Isole la dependance optionnelle a flatlaf-extras.
 */
final class GUISvgIconFactory
{
        static Icon create(String iconPath, Color highlightColor, BooleanSupplier isHighlighted)
        {
                FlatSVGIcon icon = new FlatSVGIcon(iconPath, 16, 16);
                if (!icon.hasFound())
                        return null;
                if (highlightColor != null && isHighlighted != null)
                        icon.setColorFilter(new FlatSVGIcon.ColorFilter(
                          color -> isHighlighted.getAsBoolean() ? highlightColor : color));
                return icon;
        }
}