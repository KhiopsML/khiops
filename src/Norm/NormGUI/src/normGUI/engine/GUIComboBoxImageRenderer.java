// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Component;
import java.net.URL;
import javax.swing.ImageIcon;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.ListCellRenderer;

/**
 * Definit un rendu personnalise pour les ComboBox contenant des images
 *
 * @author Marc Boulle
 */
public class GUIComboBoxImageRenderer extends JLabel implements ListCellRenderer
{

        /**
         * Tableau de noms des images
         */
        private String[] iconNames;

        /**
         * Cree un rendu personnalise pour les comboBox
         *
         * @param iconNames Le tableau de noms des images
         */
        public GUIComboBoxImageRenderer(String[] iconNames)
        {
                this.iconNames = iconNames;
                GUIObject.setComponentPreferredSize(this, 0);
        }

        /**
         * Renvoie le composant configure pour afficher la valeur specifiee
         *
         * @param list         La liste a dessiner
         * @param value        la valeur retournee par
         *                     list.getModel().getElementAt(index)
         * @param index        L'index de la cellule
         * @param isSelected   Indique si la cellule est selectionnee
         * @param cellHasFocus Indique si la cellule a le focus
         */
        public Component getListCellRendererComponent(JList list,
                                                      Object value,
                                                      int index,
                                                      boolean isSelected,
                                                      boolean cellHasFocus)
        {

                setOpaque(true);

                ImageIcon icon = null;
                String s = "images/" + value.toString() + ".gif";

                // Si on trouve l'image, on l'affiche dans le JLabel
                try {
                        URL url = getClass().getClassLoader().getResource(s);
                        icon = new ImageIcon(url);
                        setIcon(icon);
                        setHorizontalAlignment(JLabel.CENTER);
                        setText("");
                }

                // Si on ne trouve pas l'image, on affiche le libelle dans le JLabel
                catch (NullPointerException e) {
                        setText(value.toString());
                        setHorizontalAlignment(JLabel.LEFT);
                        setIcon(null);
                }
                return this;
        }
}