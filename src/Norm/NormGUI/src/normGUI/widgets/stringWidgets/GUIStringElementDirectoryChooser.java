// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.stringWidgets;

import javax.swing.JFileChooser;

/**
 * Represente l'element graphique correspondant au style FileChooser, pour la
 * selection des repertoires seulement.
 */
public class GUIStringElementDirectoryChooser extends GUIStringElementFileDirectoryChooser
{
        /**
         * Indique le mode de selection du composant
         */
        protected int getSelectionMode() { return JFileChooser.DIRECTORIES_ONLY; }
}
