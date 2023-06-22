// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.GridBagConstraints;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JPanel;

/**
 * Definit une donnee d'interface. Une donnee d'interface peut etre une unite ou
 * un element. Une donnee possede un caractere editable et une unite mere (nul
 * s'il s'agit de l'unite racine).
 *
 * @author Marc Boulle
 */
public abstract class GUIData extends GUIObject
{

        /**
         * Caractere editable d'une donnee
         */
        private boolean bEditable;

        /**
         * Menu associe a la donnee
         */
        protected JMenuBar menuBar;

        /**
         * Unite mere de la donnee
         */
        private GUIUnit parentUnit;

        /**
         * Menu qui regroupe les menus associes aux sous unites
         */
        protected JMenu subMenu;

        /**
         * Construit la barre de menus
         */
        protected abstract void buildMenuBar();

        /**
         * Construit le menu qui regroupe les menus associes aux sous unites
         */
        protected abstract void buildSubMenu();

        /**
         * Renvoie le caractere editable d'une donnee
         *
         * @return Le caractere editable de la donnee
         */
        public boolean getEditable() { return bEditable; }

        /**
         * Renvoie la barre de menus associee a la donnee
         *
         * @return La barre de menus associee a la donnee
         */
        public JMenuBar getMenuBar()
        {
                if (menuBar == null)
                        buildMenuBar();
                return menuBar;
        }

        /**
         * Renvoie l'unite contenant la donnee
         *
         * @return L'unite mere de la donnee
         */
        public GUIUnit getParentUnit() { return parentUnit; }

        /**
         * Renvoie le menu qui regroupe les menus associes aux sous unites
         *
         * @return Le menu qui regroupe les menus associes aux sous unites
         */
        public JMenu getSubMenu()
        {
                if (subMenu == null)
                        buildSubMenu();
                return subMenu;
        }

        /**
         * Ajoute le composant d'affichage et son libelle dans le panel de l'unite mere
         *
         * @param panel       Panneau de l'unite mere dans lequel sera ajoute le
         *                    composant d'affichage
         * @param constraints Contraintes d'affichage
         */
        public abstract void graphicAddField(JPanel panel, GridBagConstraints constraints);

        /**
         * Rafraichit l'interface avec les donnees c++
         */
        public abstract void graphicRefreshAll();

        /**
         * Previent java d'un changement
         */
        public void graphicFireDataChange() {}

        /**
         * Modifie le caractere editable de la donnee
         *
         * @param bValue Le nouveau caractere editable de la donnee
         */
        public void setEditable(boolean bValue) { bEditable = bValue; }

        /**
         * Modifie l'unite contenant l'element
         *
         * @param parentUnit La nouvelle unite contenant l'element
         */
        public void setParentUnit(GUIUnit parentUnit) { this.parentUnit = parentUnit; }
}