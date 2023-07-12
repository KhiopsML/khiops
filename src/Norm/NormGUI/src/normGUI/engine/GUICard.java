// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JPanel;

/**
 * Definit une unite d'interface de type fiche
 */
public class GUICard extends GUIUnit
{

        /**
         * Ajoute une sous fiche a la fiche (dans le tableau de donnees)
         *
         * @param sFieldId L'identifiant de la sous fiche a ajouter
         * @param sLabel   Le libelle de la sous fiche a ajouter
         * @param card     La sous fiche a ajouter
         * @param sStyle   Le style de la sous fiche a ajouter
         */
        public void addCardField(String sFieldId, String sLabel, GUICard card, String sStyle)
        {
                // Recherche de la classe de la sous fiche
                Class c = getUserCardClass(sStyle);
                try {
                        // Instanciation de la sous fiche
                        GUICard guiCard = (GUICard)c.newInstance();
                        guiCard = card;
                        guiCard.setIdentifier(sFieldId);
                        guiCard.setLabel(sLabel);
                        guiCard.setStyle(sStyle);
                        guiCard.setParentUnit(this);
                        addField(guiCard);
                } catch (InstantiationException ie) {
                        System.err.println("La classe java " + c + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("La classe java " + c + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Ajoute une sous liste a la fiche (dans le tableau de donnees)
         *
         * @param sFieldId L'identifiant de la sous liste a ajouter
         * @param sLabel   Le libelle de la sous liste a ajouter
         * @param list     La sous liste a ajouter
         * @param sStyle   Le style de la sous liste a ajouter
         */
        public void addListField(String sFieldId, String sLabel, GUIList list, String sStyle)
        {
                // Recherche de la classe de la sous liste
                Class c = getUserListClass(sStyle);
                try {
                        GUIUnit guiUnit = (GUIUnit)c.newInstance();
                        guiUnit = list;
                        guiUnit.setIdentifier(sFieldId);
                        guiUnit.setLabel(sLabel);
                        guiUnit.setStyle(sStyle);
                        guiUnit.setParentUnit(this);
                        addField(guiUnit);
                } catch (InstantiationException ie) {
                        System.err.println("La classe java " + c + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("La classe java " + c + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Renvoie la classe d'une fiche en fonction de son style
         *
         * @param sStyle Le style de la fiche
         * @return La classe de la fiche deduite du style
         */
        private Class getUserCardClass(String sStyle)
        {
                if (sStyle.length() == 0)
                        return GUICard.class;
                else {
                        String className = "normGUI.extensions.GUICard" + sStyle;
                        Class c = null;
                        try {
                                c = Class.forName(className);
                        } catch (ClassNotFoundException e) {
                                c = GUICard.class;
                                if (debug)
                                        displayMessage("GUICard\nAucune fiche ne correspond au style : " + sStyle +
                                                       "\nUtilisation de GUICard");
                        }
                        return c;
                }
        }

        /**
         * Renvoie la classe d'une liste en fonction de son style
         *
         * @param sStyle Le style de la liste
         * @return La classe de la liste deduite du style
         */
        private Class getUserListClass(String sStyle)
        {
                if (sStyle.length() == 0)
                        return GUIList.class;
                else {
                        String className = "normGUI.extensions.GUIList" + sStyle;
                        Class c = null;
                        try {
                                c = Class.forName(className);
                        } catch (ClassNotFoundException e) {
                                c = GUIList.class;
                                if (debug)
                                        displayMessage("GUIList\nAucune liste ne correspond au style : " + sStyle +
                                                       "\nUtilisation de GUIList");
                        }
                        return c;
                }
        }

        /**
         * Ajoute de maniere recursive les composants graphiques a l'interface
         * correspondant aux donnees de la fiche
         *
         * @param panel       Panneau dans lequel seront ajoutes les composants
         *                    graphiques
         * @param constraints Contraintes d'affichage
         */
        public void graphicAddField(JPanel panel, GridBagConstraints constraints)
        {

                // Si la fiche ne contient pas de donnees, on n'affiche pas la bordure
                if (getVisibleDataCount() == 0 && !hasActionButton()) {
                        panel.setBorder(null);
                        panel = null;
                        return;
                }

                if (getVisibleDataCount() > 0 || hasActionButton()) {

                        // Parcours de la composition de l'interface pour creation des champs
                        for (int i = 0; i < getDataCount(); i++) {
                                GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                                if (guiData.getVisible()) {
                                        // Reinitialisation des contraintes
                                        constraints.weightx = 0.0;
                                        constraints.weighty = 0.0;
                                        constraints.gridwidth = GridBagConstraints.RELATIVE;
                                        constraints.gridheight = 1;

                                        // Si le champ est une sous fiche
                                        if (guiData instanceof GUIUnit) {
                                                GUIUnit guiUnit = (GUIUnit)guiData;

                                                // Si l'unite contient des donnees
                                                if (guiUnit.getVisibleDataCount() > 0 || guiUnit.hasActionButton()) {
                                                        // Cas particulier d'une fiche sans bordure, ou on veut
                                                        // minimiser la place utilisee en placant ses element
                                                        // directement dans la fiche en cours
                                                        if ((guiUnit instanceof GUICard) &&
                                                            guiUnit.getParameters().equals("NoBorder")) {
                                                                guiUnit.graphicAddField(panel, constraints);
                                                        }
                                                        // Cas general
                                                        else {
                                                                // On l'affiche dans un autre panel avec une bordure
                                                                GridBagConstraints gbc = new GridBagConstraints();
                                                                gbc.fill = GridBagConstraints.BOTH;
                                                                JPanel jp = new JPanel(new GridBagLayout());
                                                                jp.setName(guiData.getIdentifier());
                                                                gbc.insets = new Insets(3, 3, 3, 3);
                                                                jp.setBorder(
                                                                  BorderFactory.createTitledBorder(guiData.getLabel()));
                                                                guiData.graphicAddField(jp, gbc);
                                                                constraints.weightx = 1.0;
                                                                constraints.weighty = 1.0;
                                                                constraints.gridwidth = GridBagConstraints.REMAINDER;
                                                                panel.add(jp, constraints);
                                                        }
                                                }
                                        }
                                        // Si c'est un element terminal
                                        else
                                                guiData.graphicAddField(panel, constraints);
                                }
                        }
                }

                if (hasActionButton()) {

                        // S'il n'y a aucune action ou bien s'il ne s'agit pas d'une fenetre (dans ce
                        // cas les
                        // boutons apparaissent deja dans le panel sud grace a addToolBar() de GUIUnit)
                        if (getActionCount() == 0 || frame != null)
                                return;

                        // Creation du panel sud
                        JPanel southPanel = new JPanel();

                        // Pour chaque action, on cree un bouton si necessaire
                        for (int i = 0; i < getActionCount(); i++) {
                                GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                                if (guiAction.getVisible() && guiAction.isButton()) {
                                        JButton actionButton = guiAction.createButton();
                                        actionButton.addActionListener(this);

                                        // En cas de Style SmallButton, le bouton est maquette a gauche
                                        if (guiAction.getStyle().equals("SmallButton")) {
                                                GridBagConstraints localConstraints = new GridBagConstraints();
                                                Insets currentInsets = constraints.insets;

                                                // Personnalisation des gridbagconstraints locaux pour afficher le
                                                // bouton a droite avec une marge verticale minimale
                                                localConstraints.gridx = 2;
                                                localConstraints.fill = GridBagConstraints.NONE;
                                                localConstraints.anchor = GridBagConstraints.LINE_END;
                                                localConstraints.insets =
                                                  new Insets(0, currentInsets.left, 0, currentInsets.right);

                                                // Positionnement du bouton
                                                panel.add(actionButton, localConstraints);
                                        }
                                        // Sinon, il se trouve dans le panneau sud
                                        else
                                                southPanel.add(actionButton);
                                }
                        }

                        // Ajout du panel sud au panel de l'unite mere
                        constraints.weightx = 0;
                        constraints.weighty = 0;
                        constraints.gridwidth = GridBagConstraints.REMAINDER;
                        panel.add(southPanel, constraints);
                }
        }

        /**
         * Rafraichit recursivement les valeurs des composants graphiques avec les
         * donnees c++
         */
        public void graphicRefreshAll()
        {

                // Pour chaque donnee de la fiche
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);

                        // Si la donnee est visible
                        if (guiData.getVisible())
                                guiData.graphicRefreshAll();
                }
        }

        /**
         * Previent java d'un changement
         */
        public void graphicFireDataChange()
        {

                // Pour chaque donnee de la fiche
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);

                        // Si la donnee est visible
                        if (guiData.getVisible())
                                guiData.graphicFireDataChange();
                }
        }
}
