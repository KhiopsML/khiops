// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.KeyboardFocusManager;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.Hashtable;
import java.util.Stack;
import java.util.Vector;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.KeyStroke;
import javax.swing.SwingUtilities;

/**
 * Definit une unite d'interface. Une unite d'interface peut etre une fiche ou
 * une liste
 *
 * @author Marc Boulle
 */
public abstract class GUIUnit extends GUIData implements ActionListener
{

        /**
         * Identifiant de l'action Exit
         */
        public static String Exit = "Exit";

        /**
         * Identifiant de l'action Help
         */
        public static String Help = "Help";

        /**
         * Identifiant de l'action Refresh
         */
        public static String Refresh = "Refresh";

        /**
         * Renvoie la couleur d'un composant selon son etat
         *
         * @return La couleur d'un composant selon son etat
         */
        public static Color getComponentColor(boolean isForeground,
                                              boolean isEditable,
                                              boolean isSelected,
                                              boolean hasFocus)
        {
                Color componentColor;

                // Couleur en avant-plan
                if (isForeground) {
                        componentColor = Color.BLACK;
                }
                // Couleur en arriere-plan
                else {
                        // Composants editables
                        if (isEditable) {
                                // Editables selectionnes
                                if (isSelected)
                                        componentColor = new Color(245, 245, 245);
                                // Editables non selectionnes
                                else
                                        componentColor = new Color(220, 220, 220);
                        } else
                        // Composants non editables
                        {
                                // Non editables selectionnes
                                if (isSelected)
                                        componentColor = new Color(160, 160, 160);
                                // Non editables non selectionnes
                                else
                                        componentColor = new Color(190, 190, 190);
                        }
                }

                // Inversion si focus
                if (hasFocus)
                        componentColor = new Color(255 - componentColor.getRed(),
                                                   255 - componentColor.getGreen(),
                                                   255 - componentColor.getBlue());
                return componentColor;
        }

        /**
         * Parametrage des couleurs d'un composant
         */
        public static void initializeComponentColors(JComponent component,
                                                     boolean isEditable,
                                                     boolean isSelected,
                                                     boolean hasFocus)
        {
                component.setBackground(GUIUnit.getComponentColor(false, isEditable, isSelected, hasFocus));
                component.setForeground(GUIUnit.getComponentColor(true, isEditable, isSelected, hasFocus));
        }

        /**
         * Parametrage des couleurs d'un composant text
         */
        public static void initializeTextFieldColors(JTextField textField,
                                                     boolean isEditable,
                                                     boolean isSelected,
                                                     boolean hasFocus)
        {
                textField.setBackground(GUIUnit.getComponentColor(false, isEditable, isSelected, hasFocus));
                textField.setForeground(GUIUnit.getComponentColor(true, isEditable, isSelected, hasFocus));
                textField.setDisabledTextColor(GUIUnit.getComponentColor(true, isEditable, isSelected, hasFocus));
        }

        /**
         * Parametrage des couleurs d'un composant combobox editable
         */
        public static void initializeEditableComboBoxColors(JComboBox comboBox,
                                                            boolean isEditable,
                                                            boolean isSelected,
                                                            boolean hasFocus)
        {
                JTextField textField = (JTextField)comboBox.getEditor().getEditorComponent();
                GUIUnit.initializeComponentColors(comboBox, isEditable, isSelected, hasFocus);
                GUIUnit.initializeTextFieldColors(textField, isEditable, isSelected, hasFocus);
                textField.setOpaque(true);
        }

        /**
         * Panel contenant les elements graphiques
         */
        private JPanel centerPanel;

        /**
         * Fenetre principale
         */
        public JFrame frame = null;

        /**
         * Position de la derniere frame affichee
         */
        static private int lastVisibleLocationX = -1;
        static private int lastVisibleLocationY = -1;

        /**
         * Handle sur l'objet C++ associe
         */
        private long uiObjectHandle;

        /**
         * Liste des actions de l'unite
         */
        public Vector<GUIAction> vectorOfGUIActions;

        /**
         * Liste des donnees de l'unite
         */
        public Vector<GUIData> vectorOfGUIDatas;

        /**
         * Renvoie l'indicateur qu'une action est en cours d'execution/ Permet d'inhiber
         * certains rafraichissements d'interface pendant l'execution de l'action.
         * L'indicateur est disponible pour toutes les unites d'interface en cours
         *
         * @return L'indicateur d'action
         */
        public boolean getActionRunning() { return bActionRunning; }

        /**
         * Parametrage de l'indicateur d'action en cours
         *
         * param L'indicateur d'action
         */
        public void setActionRunning(boolean value) { bActionRunning = value; }

        /**
         * Booleen indiquant une action en cours d'execution Le keyword volatile
         * garantie que chaque thread accede a la valeur depuis la memorie centrale
         */
        private volatile boolean bActionRunning = false;

        /**
         * Verrou de gestion de la modalite de la fenetre pendant une action
         */
        Object modalLock = null;
        boolean isModal;

        /**
         * Appelee lors d'un click sur un item de menu, creation du thread permettant
         * l'execution d'une action
         *
         * @param e L'evenement de l'action
         */
        public void actionPerformed(ActionEvent e)
        {
                final String sActionCommand;
                // Acces a l'action
                sActionCommand = e.getActionCommand();

                // Java envoie (rarement) un ActionEvent null
                // lorsque l'utilisateur clique sans arret sur les menus
                if (sActionCommand == null)
                        return;
                assert (!sActionCommand.equals(Refresh));

                // Recherche de l'action utilisateur
                final GUIAction guiAction = getActionAt(sActionCommand);
                if (guiAction == null)
                        return;

                addTrace("actionPerformed " + getLabel() + " " + sActionCommand + " (" + guiAction.getParameters() +
                         ") start");

                // Si l'action est Help
                if (sActionCommand.equals(Help)) {
                        showOnLineHelp();
                        frame.setVisible(true);
                }
                // Si l'action est Exit
                else if (guiAction.getIdentifier().equals(Exit) || guiAction.getParameters().equals(Exit)) {
                        // On ferme la fenetre directement
                        bActionRunning = true;
                        closeGUI(guiAction);
                        bActionRunning = false;
                }
                // Autre action
                else {
                        Thread worker;

                        // Enregistrement de l'action
                        writeOutputUnitActionCommand(guiAction.getIdentifier());

                        // On rend la fenetre inactive
                        bActionRunning = true;
                        startModal(guiAction);

                        // Creation du verrou
                        modalLock = new Object();
                        isModal = false;
                        addTrace("modal lock created: " + isModal);

                        // On envoie un evenement qui sera execute dans l'EDT apres le passage en modal
                        // de la fenetre. Cela garantit que l'action C++ ne sera declenchee qu'apres
                        // le traitement de tous les evenements de l'EDT en cours
                        SwingUtilities.invokeLater(new Runnable() {
                                public void run()
                                {
                                        synchronized (modalLock) {
                                                isModal = true;
                                                addTrace("modal lock notified:" + isModal);
                                                modalLock.notify();
                                        }
                                }
                        });

                        // Creation d'un thread pour executer l'action utilisateur
                        worker = new Thread() {
                                public void run()
                                {
                                        ///////////////////////////////////////////////////////
                                        // Mise en attente du comportement modal de la fenetre

                                        addTrace("modal lock thread start: " + isModal);
                                        synchronized (modalLock) {
                                                while (!isModal)
                                                        try {
                                                                modalLock.wait();
                                                        } catch (InterruptedException e) {
                                                        }
                                        }
                                        addTrace("modal lock thread stop: " + isModal);

                                        // On remet le verrou a null
                                        modalLock = null;
                                        isModal = false;

                                        ///////////////////////////////////////////////////////
                                        // Execution de l'action

                                        // On execute l'action cote C++
                                        try {
                                                addTrace("actionPerformed " + getLabel() + " " + sActionCommand + " (" +
                                                         guiAction.getParameters() + ") before execute");
                                                executeUserActionAt(sActionCommand);
                                                addTrace("actionPerformed " + getLabel() + " " + sActionCommand + " (" +
                                                         guiAction.getParameters() + ") after execute");
                                        } catch (Exception ex) {
                                                addTrace("actionPerformed " + getLabel() + " " + sActionCommand + " (" +
                                                         guiAction.getParameters() + ") exception execute");
                                        }

                                        // On rend la fenetre active a nouveau
                                        stopModal(guiAction);

                                        bActionRunning = false;
                                }
                        };

                        // On lance le traitement de l'action
                        worker.setName("Window " + getLabel() + ": " + sActionCommand);
                        worker.start();
                }
                addTrace("actionPerformed " + getLabel() + " " + sActionCommand + " (" + guiAction.getParameters() +
                         ") stop");
        }

        /**
         * Ajoute une action a la liste des actions
         *
         * @param sActionId L'identifiant de l'action
         * @param sLabel    Le libelle de l'action
         */
        public void addAction(String sActionId, String sLabel)
        {
                GUIAction guiAction = new GUIAction();
                guiAction.setIdentifier(sActionId);
                guiAction.setLabel(sLabel);
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                vectorOfGUIActions.add(guiAction);
        }

        /**
         * Ajoute un element de type booleen : recherche la classe, instancie l'element,
         * et ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param bDefaultValue La valeur par defaut de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addBooleanField(String sFieldId, String sLabel, boolean bDefaultValue, String sStyle)
        {
                String s = null;
                Class c = null;
                // On deduit le nom de la classe a partir du style
                if (sStyle.length() != 0 && !sStyle.equals("TextField")) {
                        s = "normGUI.widgets.booleanWidgets.GUIBooleanElement" + sStyle;
                        // Recherche de la classe du widget
                        try {
                                c = Class.forName(s);
                        } catch (ClassNotFoundException cnfe) {
                                if (debug)
                                        displayMessage("Champ : " + sFieldId + "\nAucun booleanWidget ne correspond au"
                                                       + " style : " + sStyle + "\nUtilisation du widget par defaut");
                                c = GUIBooleanElementTextField.class;
                        }
                } else
                        c = GUIBooleanElementTextField.class;
                try {
                        // Instanciation du widget
                        GUIBooleanElement guiBooleanElement = (GUIBooleanElement)c.newInstance();
                        // Caracterisation du widget
                        guiBooleanElement.setIdentifier(sFieldId);
                        guiBooleanElement.setLabel(sLabel);
                        guiBooleanElement.setDefaultValue(bDefaultValue);
                        guiBooleanElement.setValue(new Boolean(bDefaultValue).toString());
                        guiBooleanElement.setStyle(sStyle);
                        guiBooleanElement.setParentUnit(this);
                        // Ajout du widget dans le vecteur de donnees de l'unite
                        addField(guiBooleanElement);
                } catch (InstantiationException ie) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Ajoute un element de type caractere : recherche la classe, instancie
         * l'element, et ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param cDefaultValue La valeur par defaut de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addCharField(String sFieldId, String sLabel, char cDefaultValue, String sStyle)
        {
                String s = null;
                Class c = null;
                // On deduit le nom de la classe a partir du style
                if (sStyle.length() != 0 && !sStyle.equals("TextField")) {
                        s = "normGUI.widgets.charWidgets.GUICharElement" + sStyle;
                        // Recherche de la classe du widget
                        try {
                                c = Class.forName(s);
                        } catch (ClassNotFoundException cnfe) {
                                if (debug)
                                        displayMessage("Champ : " + sFieldId +
                                                       "\nAucun charWidget ne correspond au style : " + sStyle +
                                                       "\nUtilisation du widget par defaut");
                                c = GUICharElementTextField.class;
                        }
                } else
                        c = GUICharElementTextField.class;
                try {
                        // Instanciation du widget
                        GUICharElement guiCharElement = (GUICharElement)c.newInstance();
                        // Caracterisation du widget
                        guiCharElement.setIdentifier(sFieldId);
                        guiCharElement.setLabel(sLabel);
                        guiCharElement.setDefaultValue(cDefaultValue);
                        guiCharElement.setValue(new Character(cDefaultValue).toString());
                        guiCharElement.setStyle(sStyle);
                        guiCharElement.setParentUnit(this);
                        // Ajout du widget dans le vecteur de donnees de l'unite
                        addField(guiCharElement);
                } catch (InstantiationException ie) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Ajoute un element de type reel : recherche la classe, instancie l'element, et
         * ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param dDefaultValue La valeur par defaut de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addDoubleField(String sFieldId, String sLabel, double dDefaultValue, String sStyle)
        {
                addRangedDoubleField(sFieldId, sLabel, dDefaultValue, Double.MIN_VALUE, Double.MAX_VALUE, sStyle);
        }

        /**
         * Ajoute une donnee a la liste des donnees de l'unite
         *
         * @param guiData La donnee a ajouter
         */
        protected void addField(GUIData guiData)
        {
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                vectorOfGUIDatas.add(guiData);
        }

        /**
         * Ajoute un element de type entier : recherche la classe, instancie l'element,
         * et ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param nDefaultValue La valeur par defaut de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addIntField(String sFieldId, String sLabel, int nDefaultValue, String sStyle)
        {
                addRangedIntField(sFieldId, sLabel, nDefaultValue, Integer.MIN_VALUE, Integer.MAX_VALUE, sStyle);
        }

        /**
         * Ajoute la barre de menu a la fenetre
         */
        protected void addMenuBar()
        {
                if (getMenuBar() == null)
                        return;
                else
                        frame.setJMenuBar(menuBar);
        }

        /**
         * Ajoute un element de type reel : recherche la classe, instancie l'element, et
         * ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param dDefaultValue La valeur par defaut de l'element a ajouter
         * @param dMin          La valeur minimale de l'element a ajouter
         * @param dMax          La valeur maximale de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addRangedDoubleField(String sFieldId,
                                         String sLabel,
                                         double dDefaultValue,
                                         double dMin,
                                         double dMax,
                                         String sStyle)
        {
                String s = null;
                Class c = null;
                // On deduit le nom de la classe a partir du style
                if (sStyle.length() != 0 && !sStyle.equals("TextField")) {
                        s = "normGUI.widgets.doubleWidgets.GUIDoubleElement" + sStyle;
                        // Recherche de la classe du widget
                        try {
                                c = Class.forName(s);
                        } catch (ClassNotFoundException cnfe) {
                                if (debug)
                                        displayMessage("Champ : " + sFieldId +
                                                       "\nAucun doubleWidget ne correspond au style : " + sStyle +
                                                       "\nUtilisation du widget par defaut");
                                c = GUIDoubleElementTextField.class;
                        }
                } else
                        c = GUIDoubleElementTextField.class;
                try {
                        // Instanciation du widget
                        GUIDoubleElement guiDoubleElement = (GUIDoubleElement)c.newInstance();
                        // Caracterisation du widget
                        guiDoubleElement.setIdentifier(sFieldId);
                        guiDoubleElement.setLabel(sLabel);
                        guiDoubleElement.setDefaultValue(dDefaultValue);
                        guiDoubleElement.setValue(new Double(dDefaultValue).toString());
                        guiDoubleElement.setStyle(sStyle);
                        guiDoubleElement.setMinValue(dMin);
                        guiDoubleElement.setMaxValue(dMax);
                        guiDoubleElement.setParentUnit(this);
                        // Ajout du widget dans le vecteur de donnees de l'unite
                        addField(guiDoubleElement);
                } catch (InstantiationException ie) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Ajoute un element de type entier : recherche la classe, instancie l'element,
         * et ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param nDefaultValue La valeur par defaut de l'element a ajouter
         * @param nMin          La valeur minimale de l'element a ajouter
         * @param nMax          La valeur maximale de l'element a ajouter
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addRangedIntField(String sFieldId,
                                      String sLabel,
                                      int nDefaultValue,
                                      int nMin,
                                      int nMax,
                                      String sStyle)
        {
                String s = null;
                Class c = null;
                // On deduit le nom de la classe a partir du style
                if (sStyle.length() != 0 && !sStyle.equals("TextField")) {
                        s = "normGUI.widgets.intWidgets.GUIIntElement" + sStyle;
                        // Recherche de la classe du widget
                        try {
                                c = Class.forName(s);
                        } catch (ClassNotFoundException cnfe) {
                                if (debug)
                                        displayMessage("Champ : " + sFieldId +
                                                       "\nAucun intWidget ne correspond au style : " + sStyle +
                                                       "\nUtilisation du widget par defaut");
                                c = GUIIntElementTextField.class;
                        }
                } else
                        c = GUIIntElementTextField.class;
                try {
                        // Instanciation du widget
                        GUIIntElement guiIntElement = (GUIIntElement)c.newInstance();
                        // Caracterisation du widget
                        guiIntElement.setIdentifier(sFieldId);
                        guiIntElement.setLabel(sLabel);
                        guiIntElement.setDefaultValue(nDefaultValue);
                        guiIntElement.setValue(new Integer(nDefaultValue).toString());
                        guiIntElement.setStyle(sStyle);
                        guiIntElement.setMinValue(nMin);
                        guiIntElement.setMaxValue(nMax);
                        guiIntElement.setParentUnit(this);
                        // Ajout du widget dans le vecteur de donnees de l'unite
                        addField(guiIntElement);
                } catch (InstantiationException ie) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + iae);
                }
        }

        /**
         * Ajoute un element de type chaine de caracteres : recherche la classe,
         * instancie l'element, et ajoute l'element dans le tableau de donnees
         *
         * @param sFieldId      L'identifiant de l'element a ajouter
         * @param sLabel        Le libelle de l'element a ajouter
         * @param sDefaultValue La valeur par defaut de l'element a ajouter
         * @param nMinLength    La longueur minimale de la chaine
         * @param nMaxLength    La longueur maximale de la chaine
         * @param sStyle        Le style de l'element a ajouter
         */
        public void addRangedStringField(String sFieldId,
                                         String sLabel,
                                         String sDefaultValue,
                                         int nMinLength,
                                         int nMaxLength,
                                         String sStyle)
        {
                String s = null;
                Class c = null;
                // On deduit le nom de la classe a partir du style
                if (sStyle.length() != 0 && !sStyle.equals("TextField")) {
                        s = "normGUI.widgets.stringWidgets.GUIStringElement" + sStyle;
                        // Recherche de la classe du widget
                        try {
                                c = Class.forName(s);
                        } catch (ClassNotFoundException cnfe) {
                                if (debug)
                                        displayMessage("Champ : " + sFieldId +
                                                       "\nAucun stringWidget ne correspond au style : " + sStyle +
                                                       "\nUtilisation du widget par defaut");
                                c = GUIStringElementTextField.class;
                        }
                } else
                        c = GUIStringElementTextField.class;
                try {
                        // Instanciation du widget
                        GUIStringElement guiStringElement = (GUIStringElement)c.newInstance();
                        // Caracterisation du widget
                        guiStringElement.setIdentifier(sFieldId);
                        guiStringElement.setLabel(sLabel);
                        guiStringElement.setDefaultValue(sDefaultValue);
                        guiStringElement.setValue(sDefaultValue);
                        guiStringElement.setStyle(sStyle);
                        guiStringElement.setMinLength(nMinLength);
                        guiStringElement.setMaxLength(nMaxLength);
                        guiStringElement.setParentUnit(this);
                        // Ajout du widget dans le vecteur de donnees de l'unite
                        addField(guiStringElement);
                } catch (InstantiationException ie) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + ie);
                } catch (IllegalAccessException iae) {
                        System.err.println("la classe " + s + " ne peut etre instanciee " + iae);
                }
        }

        protected void addToolBar()
        {

                if (getActionCount() == 0)
                        return;

                // Creation des panneaux
                JPanel southPanel = new JPanel();

                // Creation des boutons action
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && guiAction.isButton() && !guiAction.getIdentifier().equals(Exit) &&
                            !guiAction.getIdentifier().equals(Refresh)) {
                                JButton actionButton = guiAction.createButton();
                                actionButton.addActionListener(this);
                                southPanel.add(actionButton);
                        }
                }

                // Recherche du libelle de l'action exit
                String sLabelOK;
                GUIAction exitAction = getActionAt(Exit);
                if (exitAction == null)
                        return;
                else
                        sLabelOK = exitAction.getLabel();

                // Ajout du bouton s'il est visible
                if (exitAction.getVisible()) {
                        JButton buttonOK = new JButton(sLabelOK);
                        buttonOK.setActionCommand(Exit);
                        if (exitAction.getShortCut() != ' ')
                                buttonOK.setMnemonic((int)exitAction.getShortCut());
                        exitAction.setComponentHelpText(buttonOK);
                        buttonOK.addActionListener(this);
                        southPanel.add(buttonOK);
                }

                // Ajout du panneau a la fenetre
                frame.getContentPane().add(southPanel, BorderLayout.SOUTH);
        }

        /**
         * Construit le menu associe a l'unite
         */
        protected void buildMenuBar()
        {

                menuBar = new JMenuBar();

                // Creation du menu des actions de l'unite
                JMenu menu = new JMenu(getLabel());
                menu.setName(getIdentifier());
                if (getShortCut() != ' ')
                        menu.setMnemonic((int)getShortCut());
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && guiAction.getStyle().equals("") &&
                            !guiAction.getIdentifier().equals(Exit) && !guiAction.getIdentifier().equals(Refresh)) {
                                JMenuItem menuItem = new JMenuItem(guiAction.getLabel());
                                menuItem.setActionCommand(guiAction.getIdentifier());
                                menuItem.setAccelerator(KeyStroke.getKeyStroke(guiAction.getAccelKey()));
                                if (guiAction.getShortCut() != ' ')
                                        menuItem.setMnemonic((int)guiAction.getShortCut());
                                guiAction.setComponentHelpText(menuItem);
                                menuItem.addActionListener(this);
                                menu.add(menuItem);
                        }
                }

                // On met toujours au moins un menu dans la menubar (sinon, bug avec ALT)
                menuBar.add(menu);
                if (menu.getItemCount() == 0)
                        menu.setVisible(false);

                // Ajout de sous-menus pour les composants graphique qui en ont un
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                for (int i = 0; i < vectorOfGUIDatas.size(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getVisible()) {
                                JMenu jm = guiData.getSubMenu();
                                if (jm != null)
                                        menuBar.add(jm);
                        }
                }

                // Ajout du texte d'aide
                if (hasHelpText())
                        menuBar.add(getHelpMenu());

                // Barre invisible si necessaire
                if (menuBar.getMenuCount() == 0)
                        menuBar.setVisible(false);
        }

        public void buildSubMenu()
        {
                // Memorisation d'un eventuelle action de type "Exit"
                JMenuItem exitMenuItem = null;

                // Creation du menu des actions de l'unite
                subMenu = new JMenu(getLabel());
                subMenu.setName(getIdentifier());
                if (getShortCut() != ' ')
                        subMenu.setMnemonic((int)getShortCut());
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                for (int i = 0; i < vectorOfGUIActions.size(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && guiAction.getStyle().equals("") &&
                            !guiAction.getIdentifier().equals(Exit) && !guiAction.getIdentifier().equals(Refresh)) {
                                JMenuItem menuItem = new JMenuItem(guiAction.getLabel());
                                menuItem.setActionCommand(guiAction.getIdentifier());
                                menuItem.setAccelerator(KeyStroke.getKeyStroke(guiAction.getAccelKey()));
                                if (guiAction.getShortCut() != ' ')
                                        menuItem.setMnemonic((int)guiAction.getShortCut());
                                guiAction.setComponentHelpText(menuItem);
                                menuItem.addActionListener(this);
                                // Memorisation s'il s'agit de "Exit"
                                if (guiAction.getParameters().equals(Exit))
                                        exitMenuItem = menuItem;
                                // Ajout au sous-menu sinon
                                subMenu.add(menuItem);
                        }
                }

                // Ajout de sous-menus pour les composants graphiques qui en ont un
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                for (int i = 0; i < vectorOfGUIDatas.size(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getVisible()) {
                                JMenu jm = guiData.getSubMenu();
                                if (jm != null) {
                                        // Ajout d'un separateur
                                        if (subMenu.getItemCount() > 0)
                                                subMenu.addSeparator();
                                        subMenu.add(jm);
                                }
                        }
                }

                // Ajout de l'eventuelle action "Exit" en toute fin de menu
                if (exitMenuItem != null) {
                        // Ajout d'un separateur
                        if (subMenu.getItemCount() > 0)
                                subMenu.addSeparator();
                        subMenu.add(exitMenuItem);
                }

                // Pas de menu si vide
                if (subMenu.getItemCount() == 0)
                        subMenu = null;
        }

        /**
         * Lance l'execution de l'action cote c++
         *
         * @param sActionId L'identifiant de l'action a executer
         */
        public native void executeUserActionAt(String sActionId);

        /**
         * Renvoie une action de l'unite en fonction de son identifiant
         *
         * @param sIdentifier L'identifiant de l'action
         * @return L'action recherchee
         */
        protected GUIAction getActionAt(String sIdentifier)
        {
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getIdentifier().equals(sIdentifier))
                                return guiAction;
                }
                return null;
        }

        /**
         * Renvoie le nombre d'actions
         */
        public int getActionCount()
        {
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                return vectorOfGUIActions.size();
        }

        /**
         * Renvoie le nombre d'actions visibles recursivement
         */
        public int getVisibleActionCount()
        {
                int visibleActionCount = 0;
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible())
                                visibleActionCount++;
                }
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getVisible()) {
                                // Si le champ est une sous fiche, on ne le considere comme visible
                                // que s'il a lui-meme des action visibles
                                if (guiData instanceof GUIUnit)
                                        visibleActionCount += ((GUIUnit)guiData).getVisibleActionCount();
                        }
                }
                return visibleActionCount;
        }

        /**
         * Renvoie le booleen c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @return La valeur de l'element
         */
        public native boolean getBooleanValueAt(String sFieldId);

        /**
         * Renvoie le caractere c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @return La valeur de l'element
         */
        public native char getCharValueAt(String sFieldId);

        /**
         * Renvoie le nombre de donnees
         */
        public int getDataCount()
        {
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                return vectorOfGUIDatas.size();
        }

        /**
         * Renvoie le nombre de donnees visibles recursivement
         */
        public int getVisibleDataCount()
        {
                int visibleDataCount = 0;
                if (vectorOfGUIDatas == null)
                        vectorOfGUIDatas = new Vector<GUIData>();
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getVisible()) {
                                // Si le champ est une sous fiche, on ne le considere comme visible
                                // que s'il a lui-meme des champs visibles
                                if (guiData instanceof GUIUnit)
                                        visibleDataCount += ((GUIUnit)guiData).getVisibleDataCount();
                                // Cas des champs elementaires
                                else
                                        visibleDataCount++;
                        }
                }
                return visibleDataCount;
        }

        /**
         * Renvoie le reel c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @return La valeur de l'element
         */
        public native double getDoubleValueAt(String sFieldId);

        /**
         * Renvoie une donnee de l'unite en fonction de son identifiant
         *
         * @param sIdentifier L'identifiant de la donnee
         * @return La donnee recherchee
         */
        public GUIData getFieldAt(String sIdentifier)
        {
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getIdentifier().equals(sIdentifier))
                                return guiData;
                }
                return null;
        }

        /**
         * Renvoie le premier element focusable du tableau de donnee
         *
         * @return Le premier element focusable du tableau de donnee
         */
        private GUIElement getFirstFocusableElement()
        {
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = ((GUIData)vectorOfGUIDatas.get(i));
                        if (guiData instanceof GUIUnit)
                                return ((GUIUnit)guiData).getFirstFocusableElement();
                        else if ((((GUIElement)guiData).getComponent()).isEnabled())
                                return (GUIElement)guiData;
                }
                return null;
        }

        /**
         * Renvoie le menu d'aide
         *
         * @return Le menu d'aide de l'unite
         */
        public JMenu getHelpMenu()
        {
                JMenu helpMenu = new JMenu("?");
                helpMenu.setName("help");
                JMenuItem helpItem = new JMenuItem("Help");
                helpItem.setActionCommand(Help);
                helpItem.addActionListener(this);
                helpMenu.add(helpItem);
                return helpMenu;
        }

        /**
         * Renvoie l'entier c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @return La valeur c++ de l'element
         */
        public native int getIntValueAt(String sFieldId);

        /**
         * Renvoie l'unite racine de l'interface
         *
         * @return L'unite racine de l'interface
         */
        public GUIUnit getParentRoot()
        {
                GUIUnit parent = this;
                while (true) {
                        if (parent.getParentUnit() != null)
                                parent = parent.getParentUnit();
                        else
                                break;
                }
                return parent;
        }

        /**
         * Renvoie la chaine de caracteres c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @return La valeur c++ de l'element
         */
        public native String getStringValueAt(String sFieldId);

        /**
         * Renvoie le handle de l'objet C++ associe
         *
         * @return Le handle de l'objet c++ associe
         */
        private long getUIObjectHandle() { return uiObjectHandle; }

        /**
         * Indique si l'unite possede au moins une action a representer sous la forme de
         * bouton
         *
         * @return Un booleen indiquant si l'unite possede au moins une action a
         *         representer sous la forme de bouton
         */
        public boolean hasActionButton()
        {
                // Pas de recherche recursive
                boolean recursiveSearch = false;

                // Recherche dans la fenetre en cours
                if (vectorOfGUIActions == null)
                        vectorOfGUIActions = new Vector<GUIAction>();
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && guiAction.isButton())
                                return true;
                }

                // Recherche dans les sous-fenetres
                if (recursiveSearch) {
                        for (int i = 0; i < getDataCount(); i++) {
                                GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                                if (guiData.getVisible()) {
                                        // Si le champ est une sous fiche, on ne le considere comme visible
                                        // que s'il a lui-meme des action visibles
                                        if (guiData instanceof GUIUnit && ((GUIUnit)guiData).hasActionButton())
                                                return true;
                                }
                        }
                }
                return false;
        }

        /**
         * Methode permettant de conditionner la presence du menu aide au sein de la
         * barre d'outil
         *
         * @return Booleen indiquant si l'unite possede des textes d'aide
         */
        public boolean hasHelpText()
        {

                // Recherche des textes d'aide pour les actions
                boolean hasActionHelpText = false;
                for (int i = 0; i < getActionCount(); i++) {
                        GUIAction guiAction = (GUIAction)vectorOfGUIActions.get(i);
                        if (guiAction.getVisible() && !guiAction.getIdentifier().equals(Exit) &&
                            !guiAction.getIdentifier().equals(Refresh)) {
                                if (guiAction.getHelpText().length() > 0)
                                        hasActionHelpText = true;
                        }
                }

                // Recherche des textes d'aide pour les donnees
                boolean hasDataUnitHelpText = false;
                boolean hasDataElementHelpText = false;
                for (int i = 0; i < getDataCount(); i++) {
                        GUIData guiData = (GUIData)vectorOfGUIDatas.get(i);
                        if (guiData.getVisible()) {
                                // S'il s'agit d'une unite
                                if (guiData instanceof GUIUnit)
                                        hasDataUnitHelpText = ((GUIUnit)guiData).hasHelpText();
                                // S'il s'agit d'un element
                                else {
                                        if (guiData.getHelpText().length() > 0)
                                                hasDataElementHelpText = true;
                                }
                        }
                }

                // On ne considere que l'aide sur les unites, l'aide sur les elements terminaux
                // etant fournie par les info-bulles
                return (getHelpText().length() > 0 || hasDataUnitHelpText);
        }

        /**
         * Pile de tous les objets GUIUNit ouverts
         */
        private static Stack<GUIUnit> openedGUIUnitsStack = new Stack<GUIUnit>();

        /**
         * Verrou de fermeture gestion des fenetres
         */
        Object closeLock = null;

        /**
         * Initialise et ouvre l'unite Gestion de la synchronisation entre Java et C++,
         * en attendant la fermeture de la fenetre via le verrou de gestion des fenetres
         */
        public void open()
        {
                GUIUnit parentOpenedGUIUnit;
                JFrame parentOpenedFrame;

                assert (frame == null);

                // Initialisation globale
                GUIManager.initialize();

                // Recherche de la GUIUNit ouverte precedente
                parentOpenedGUIUnit = null;
                if (!openedGUIUnitsStack.empty())
                        parentOpenedGUIUnit = openedGUIUnitsStack.peek();

                // Recherche de la frame precedente
                parentOpenedFrame = null;
                if (parentOpenedGUIUnit != null)
                        parentOpenedFrame = parentOpenedGUIUnit.frame;

                // Gestion directe par un FileChooser des fiches elementaires de choix
                // d'un seul champ de type fichier
                boolean directFileChooser = true;
                if (directFileChooser && GUIFileDirectoryChooser.isFileChooserCard(this)) {
                        // Lancement du FileChooser
                        addTrace("Open FileChooser start");
                        try {
                                GUIFileDirectoryChooser.startFileChooser(this, parentOpenedFrame);
                        } catch (Exception e) {
                        }
                        addTrace("Open FileChooser stop");
                        return;
                }

                // Gestion directe par une boite de dialogue des fiches elementaires de style
                // "QuestionDialog"
                boolean directDialog = true;
                if (directDialog && GUIDialog.isDialogCard(this)) {
                        // Mancement de la boite de dialogue
                        addTrace("Open Dialog start");
                        try {
                                GUIDialog.startDialog(
                                  this, parentOpenedFrame, lastVisibleLocationX, lastVisibleLocationY);
                        } catch (Exception e) {
                        }
                        addTrace("Open Dialog stop");
                        return;
                }

                // Ouverture dans le cas standard
                addTrace("Open start");

                // Creation du verrou
                closeLock = new Object();

                // Creation et ouverture de la fenetre
                openGUI(parentOpenedFrame);
                addTrace("openGUI finished: frame " + frame.getTitle());

                // Empilement dans la liste des frames ouvertes
                assert (frame != null);
                openedGUIUnitsStack.push(this);

                // Creation d'un thread de synchronisation, permettant d'attendre la fermeture
                // de la fenetre
                Thread closeThread = new Thread() {
                        public void run()
                        {
                                synchronized (closeLock) {
                                        addTrace("close lock thread start: " + frame.getTitle() + " " +
                                                 frame.isVisible());
                                        while (frame.isVisible())
                                                try {
                                                        closeLock.wait();
                                                } catch (InterruptedException e) {
                                                }
                                        addTrace("close lock thread stop: " + frame.getTitle() + " " +
                                                 frame.isVisible());
                                }
                        }
                };

                // Lancement du thread de synchronisation de la fermeture
                try {
                        closeThread.start();
                } catch (Exception ex) {
                }

                // Attente de la fin du thread
                try {
                        closeThread.join();
                } catch (Exception ex) {
                }

                // On remet le verrou a null
                closeLock = null;

                // On depile de la liste des frames ouverte
                GUIUnit popedGUIUnit;
                popedGUIUnit = openedGUIUnitsStack.pop();
                assert (popedGUIUnit == this);
                assert (frame != null);
                frame = null;
                addTrace("Open stop");
        }

        /**
         * Creation de la fenetre principale et ouverture Enregistrement d'une action de
         * fermeture pour gerer le verrou de synchronisation permettant d'attendre la
         * fermeture de la fenetre au programme appelant la methode Open
         *
         * @param parentFrame Frame parent du FileChooser
         */
        private void openGUI(JFrame parentFrame)
        {
                // L'ordre des instructions suivantes est important
                addTrace("openGUI " + getLabel() + " start");

                // On cree la fenetre et on la desactive pendant la construction
                frame = new JFrame(getLabel());

                // On lui associe un nom unique lie a la librairie
                frame.setName(frameUniqueName);

                // Mise en place d'un simple buffering
                // Cf. bug java en dual monitor et carte graphique
                // http://bugs.java.com/view_bug.do?bug_id=6933331
                // http://stackoverflow.com/questions/6436944/java-illegalstateexception-buffers-have-not-been-created
                frame.createBufferStrategy(1);

                // Desactivation temporaire de la fenetre, le temps de son initialisation
                frame.setEnabled(false);

                // Parametrage de l'icone
                ImageIcon frameIcon;
                if (!getFrameIconPath().equals("")) {
                        try {
                                frameIcon = getImageIcon(getFrameIconPath());
                        } catch (Exception ex) {
                                frameIcon = null;
                        }
                        if (frameIcon != null)
                                frame.setIconImage(frameIcon.getImage());
                }

                // On cree le gridbaglayout et les contraintes
                GridBagLayout gridbag = new GridBagLayout();
                GridBagConstraints constraints = new GridBagConstraints();
                constraints.fill = GridBagConstraints.BOTH;
                constraints.insets = new Insets(3, 3, 3, 3);

                // On cree le panel central dans un scrollpane qu'on ajoute a la fanetre
                centerPanel = new JPanel(gridbag);
                JScrollPane scrollPane = new JScrollPane(centerPanel);
                frame.getContentPane().setLayout(new BorderLayout());
                frame.getContentPane().add(scrollPane, BorderLayout.CENTER);

                // Si la fiche ne contient pas de donnees, on n'affiche pas la bordure
                if (getDataCount() == 0)
                        scrollPane.setBorder(null);

                // Ajout de la barre de menu
                addMenuBar();

                // Ajout des boutons action
                addToolBar();

                // Creation de la composition
                graphicAddField(centerPanel, constraints);

                // Initialisation des valeurs
                graphicRefreshAll();

                // Ajustement de la taille de la fenetre pour tenir compte de la longueur du titre
                frame.pack();
                int minFrameWidth = getMinFramedWidth(getLabel());
                Dimension currentPreferredSize = frame.getPreferredSize();
                if (currentPreferredSize.getWidth() <= minFrameWidth) {
                        frame.setPreferredSize(new Dimension(minFrameWidth, (int)currentPreferredSize.getHeight()));
                        frame.pack();
                }

                // Creation et enregistrement d'une gestion personnaliser de la fermeture
                // de la fenetre pour gerer la synchronisation avec le programme
                // ayant ouvert la fenetre
                WindowAdapter windowClosingListener;
                windowClosingListener = new WindowAdapter() {
                        public void windowClosing(WindowEvent arg0)
                        {
                                // On recherche la frame ayant emis l'evenement pour conditionner
                                // la fermeture selon le status de la frame a fermer
                                JFrame closingFrame;
                                try {
                                        closingFrame = ((JFrame)arg0.getWindow());
                                } catch (Exception ex) {
                                        closingFrame = null;
                                }
                                addTrace("windowClosing " + closingFrame.getTitle() + ":  " +
                                         closingFrame.getDefaultCloseOperation());
                                if (closingFrame != null && closingFrame.getName().equals(frameUniqueName) &&
                                    closingFrame.getDefaultCloseOperation() != JFrame.DO_NOTHING_ON_CLOSE)
                                        closeGUI(null);
                        }
                };
                frame.addWindowListener(windowClosingListener);

                // Position de la fenetre par rapport a la fenetre parente si elle est est cree
                // par GUIUnit, au centre de l'ecran sinon
                frame.setLocationRelativeTo(parentFrame);

                // Dans le cas d'un frame null, on regarde si l'on dispose d'une derniere
                // posituion d'affichage
                if (parentFrame == null) {
                        if (lastVisibleLocationX >= 0 && lastVisibleLocationY >= 0)
                                frame.setLocation(lastVisibleLocationX - frame.getWidth() / 2,
                                                  lastVisibleLocationY - frame.getHeight() / 2);
                }

                // On active la fenetre, on la centre et on la rend visible
                frame.setEnabled(true);
                frame.setVisible(true);

                // On attribut le focus au premier element
                GUIElement firstElement = getFirstFocusableElement();
                if (firstElement != null)
                        firstElement.getComponent().requestFocus();
                addTrace("openGUI " + getLabel() + " stop");
        }

        /**
         * Fermeture de la fenetre principal Gestion de la perte de focus et mise a jour
         * des valeur cote C++ par celles de l'interface
         *
         * @param guiAction: action declenchant la fermeture, d'identifiant ou de
         *                   parametre Exit, ou null s'il s'agit d'une fermeture par le
         *                   menu systeme
         */
        private void closeGUI(GUIAction guiAction)
        {
                KeyboardFocusManager focusManager;
                Component focusOwner;
                final GUIUnit parentRoot;
                final String exitAction;

                assert (guiAction == null || guiAction.getIdentifier() == Exit || guiAction.getParameters() == Exit);
                addTrace("closeGUI " + getLabel() + " start");
                if (guiAction == null)
                        addTrace("closeGUI system Exit");
                else
                        addTrace("closeGUI user Exit(" + guiAction.getIdentifier() + ", " + guiAction.getParameters() +
                                 ")");

                // Recherche de la fenetre a fermer, qui peut etre une fenetre englobante
                parentRoot = getParentRoot();

                // On la rend inactive toute de suite
                parentRoot.frame.setEnabled(true);

                // On simule une perte de focus pour forcer le rafraichissement des donnees
                // C++
                // si par exemple on passe directement par le menu systeme de la fenetre
                focusManager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
                focusOwner = focusManager.getFocusOwner();
                try {
                        // On memorise le composant possedant le focus et on simule une perte de focus
                        if (focusOwner != null) {
                                FocusEvent focusEvent =
                                  new FocusEvent(focusOwner, FocusEvent.FOCUS_LOST, false, parentRoot.frame);
                                focusOwner.dispatchEvent(focusEvent);
                        }
                } catch (Exception ex) {
                }

                // Recherche de l'action de sortie a executer
                if (guiAction != null && guiAction.getParameters().equals(Exit))
                        exitAction = guiAction.getIdentifier();
                else
                        exitAction = Exit;

                // Enregistrement de l'action
                addTrace("exitPerformed " + exitAction);
                writeOutputUnitActionCommand(exitAction);

                // On envoie un evenement qui sera execute dans l'EDT apres la fin
                // du traitement des evenements en cours
                SwingUtilities.invokeLater(new Runnable() {
                        public void run()
                        {
                                // On execute l'action de sortie pour forcer la mise a jour des valeurs C++ par
                                // celle de l'interface
                                addTrace("windowClosing before execute " + exitAction);
                                try {
                                        executeUserActionAt(exitAction);
                                } catch (Exception ex) {
                                }
                                addTrace("windowClosing after execute " + exitAction);

                                // On ferme la fenetre en gerant le verrou de synchronisation
                                synchronized (parentRoot.closeLock) {
                                        addTrace("windowClosing notify");
                                        // Memorisation de la position affichee
                                        lastVisibleLocationX =
                                          parentRoot.frame.getX() + parentRoot.frame.getWidth() / 2;
                                        lastVisibleLocationY =
                                          parentRoot.frame.getY() + parentRoot.frame.getHeight() / 2;
                                        // Fermeture de la fenetre
                                        parentRoot.frame.dispose();
                                        parentRoot.closeLock.notify();
                                }
                                addTrace("closeGUI " + getLabel() + " stop");
                        }
                });
        }

        /**
         * Acces a la frame courante ayant le focus, en renvoyant null si ce n'est pas
         * une frame cree par GUIUNit
         */
        public static JFrame getCurrentNormFrame()
        {
                Window parentWindow = null;
                JFrame parentFrame = null;

                // On recupere la fenetre active, qui n'est pas necessairement une frame gere
                // par NormGUI
                try {
                        KeyboardFocusManager focusManager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
                        parentWindow = focusManager.getActiveWindow();
                        parentFrame = (JFrame)parentWindow;

                        // Test si le nom de la frame correspond a celui des frames crees par GUIUnit
                        if (parentFrame != null &&
                            (parentFrame.getTitle() == null || !parentFrame.getName().equals(frameUniqueName)))
                                parentFrame = null;

                        // Test additionnel que le titre est non vide
                        if (parentFrame != null &&
                            (parentFrame.getTitle() == null || parentFrame.getTitle().equals("")))
                                parentFrame = null;
                } catch (Exception e) {
                        parentFrame = null;
                }
                if (parentFrame != null)
                        GUIUnit.addTrace("Current Norm frame: " + parentFrame.getName() + "(" + parentFrame.getTitle() +
                                         ")");
                return parentFrame;
        }

        /**
         * Nom des frames crees par GUIUnit
         */
        private static String frameUniqueName = "NormGUI copyright Orange 2021";

        /**
         * Parametrage de l'etat enable d'un Frame et de son contenu, de facon a
         * permettre l'acces au menu systeme (iconiser, maximiser...) L'etat de la Frame
         * est propage recursivement a tout son contenu, sauf a la frame elle-meme (son
         * parametrage simple se propage aux menus systemes) et aux exceptions
         */
        void setDescendantsEnabled(Component c, boolean value, Hashtable<Component, Component> exceptionComponents)
        {
                if (c == null)
                        return;
                if (!(c instanceof JFrame)) {
                        if (!exceptionComponents.containsKey(c)) {
                                c.setEnabled(value);
                        }
                }
                // On propage aux composants des containers, sauf ceux des tables, qui sont
                // geres de facon dynamique
                if (c instanceof Container && !(c instanceof GUITable)) {
                        Component[] kids = ((Container)c).getComponents();
                        for (int i = 0; i < kids.length; i++)
                                setDescendantsEnabled(kids[i], value, exceptionComponents);
                }
        }

        /**
         * Recherche de tous les descendants disable Permet de memoriser les composants
         * initialement disable, pour restituer leur etat initial
         */
        void searchAllDisabledComponents(Component c, Hashtable<Component, Component> disabledComponents)
        {
                if (c == null)
                        return;
                if (!(c instanceof JFrame)) {
                        if (!c.isEnabled())
                                disabledComponents.put(c, c);
                }
                // On propage aux composants des containers, sauf ceux des tables, qui sont
                // geres de facon dynamique
                if (c instanceof Container && !(c instanceof GUITable)) {
                        Component[] kids = ((Container)c).getComponents();
                        for (int i = 0; i < kids.length; i++)
                                searchAllDisabledComponents(kids[i], disabledComponents);
                }
        }

        /**
         * Arret de l'edition dans les sous-tables Permet de forcer la perte de focus
         */
        void stopCellEditionInTables(Component c)
        {
                if (c == null)
                        return;
                if (c instanceof GUITable) {
                        GUITable table = (GUITable)c;
                        if (table.isEditing())
                                table.getCellEditor().stopCellEditing();
                }
                // On propage aux composants des containers, sauf ceux des tables, qui sont
                // geres de facon dynamique
                if (c instanceof Container && !(c instanceof GUITable)) {
                        Component[] kids = ((Container)c).getComponents();
                        for (int i = 0; i < kids.length; i++)
                                stopCellEditionInTables(kids[i]);
                }
        }

        /**
         * Gestion du contexte de la fenetre pour traiter l'excution d'un action cote
         * C++ en mettant la fenetre en disabled le temps de l'excution de l'action,
         * puis en restaurant son contexte. attention, on doit passer par la
         * currentFrame pour obtenir la frame englobante dans le cas de sous-fiches
         * d'une fenetre globale
         */
        private JFrame currentFrame;
        private int currentFrameState;
        private int currentDefaultCloseOperation;
        private Hashtable<Component, Component> currentDisabledComponents;

        /**
         * Passage en mode modal de la fenetre, en memorisant son contexte
         *
         * @param guiAction: action excecutee apres cette methode
         */
        private void startModal(GUIAction guiAction)
        {
                KeyboardFocusManager focusManager;
                Component focusOwner;

                assert (!guiAction.getIdentifier().equals(Exit));
                addTrace("startModal " + guiAction.getIdentifier() + " (" + guiAction.getParameters() + ") start");

                // Recherche de la frame courante, qui peut etre celle d'une fenetre englobante
                currentFrame = getParentRoot().frame;

                // On recupere la fenetre active
                focusManager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
                focusOwner = focusManager.getFocusOwner();

                // Memorisation des etats de la fenetre au lancement de l'action
                currentFrameState = currentFrame.getExtendedState();
                currentDefaultCloseOperation = currentFrame.getDefaultCloseOperation();

                // On interdit de clore la fenetre quand une action est en cours
                currentFrame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

                // Arret de l'edition dans les sous-tables
                stopCellEditionInTables(currentFrame);

                // On simule une perte de focus
                if (focusOwner != null) {
                        FocusEvent focusEvent = new FocusEvent(focusOwner, FocusEvent.FOCUS_LOST, false, currentFrame);
                        focusOwner.dispatchEvent(focusEvent);
                }

                // On cherche les composants initialement disabled pour les remettre dans le
                // meme etat
                currentDisabledComponents = new Hashtable<Component, Component>();
                searchAllDisabledComponents(currentFrame, currentDisabledComponents);

                // On desactive cette fenetre pour un assurer un comportement modal
                setDescendantsEnabled(currentFrame, false, currentDisabledComponents);
                addTrace("startModal " + guiAction.getIdentifier() + " (" + guiAction.getParameters() + ") stop");
        }

        /**
         * Fin du mode modal de la fenetre, en restituant son contexte
         *
         * @param guiAction: action executee avant cette methode
         */
        private void stopModal(final GUIAction guiAction)
        {
                addTrace("stopModal " + guiAction.getIdentifier() + " (" + guiAction.getParameters() + ") start");
                currentFrame.setIgnoreRepaint(true);

                // On restitue le caractere actif des composants pour qu'il se repeignent
                // correctement selon lors caractere editable, en laissant la fenetre non active
                setDescendantsEnabled(currentFrame, true, currentDisabledComponents);
                currentFrame.setEnabled(false);

                // On rafraichit les composants graphiques de l'unite avec les donnees logiques
                try {
                        getParentRoot().graphicRefreshAll();
                } catch (Exception ex) {
                }
                try {
                        getParentRoot().graphicFireDataChange();
                } catch (Exception ex) {
                }

                // On retabli le comportement initial des demandes de fermeture de la fenetre
                currentFrame.setDefaultCloseOperation(currentDefaultCloseOperation);

                // On restitue l'etat de la fenetre au moment de l'action
                currentFrame.setExtendedState(currentFrameState);

                // On rend la fenetre active et visible
                currentFrame.setEnabled(true);

                // On redemande le focus
                currentFrame.requestFocus();

                // On lance la repeinture de la fenetre dans un thread s'executant apres le
                // traitement des methodes precedentes
                currentFrame.setIgnoreRepaint(false);
                currentFrame.repaint();
                addTrace("stopModal " + guiAction.getIdentifier() + " (" + guiAction.getParameters() + ") stop");
        }

        /**
         * Modifie le booleen c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @param bValue   La nouvelle valeur c++ de l'element
         */
        public native void setBooleanValueAt(String sFieldId, boolean bValue);

        /**
         * Modifie le caractere c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @param cValue   La nouvelle valeur c++ de l'element
         */
        public native void setCharValueAt(String sFieldId, char cValue);

        /**
         * Modifie le reel c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @param dValue   La nouvelle valeur c++ de l'element
         */
        public native void setDoubleValueAt(String sFieldId, double dValue);

        /**
         * Modifie l'entier c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @param nValue   La nouvelle valeur c++ de l'element
         */
        public native void setIntValueAt(String sFieldId, int nValue);

        /**
         * Modifie la chaine de caracteres c++ d'un element
         *
         * @param sFieldId L'identifiant de l'element
         * @param sValue   La nouvelle valeur c++ de l'element
         */
        public native void setStringValueAt(String sFieldId, String sValue);

        /**
         * Initialise le handle de l'objet C++ associe
         *
         * @param handle Le handle de l'objet c++ associe
         */
        private void setUIObjectHandle(long handle) { uiObjectHandle = handle; }

        /**
         * Affiche l'aide en ligne
         */
        private void showOnLineHelp()
        {
                clearHelpWindow();
                writeOnLineHelp("");
        }

        /**
         * Ecrit l'aide en ligne
         *
         * @param sIndent Le texte d'aide en ligne
         */
        public void writeOnLineHelp(String sIndent)
        {
                int nAction;
                int nActionNumber;
                GUIAction action;
                int nField;
                GUIData field;
                int nFieldNumber;

                // On ne montre que l'aide sur les unites, l'aide sur les actions ou les
                // elements
                // etant traitee par les info-bulles
                boolean showActionHelpText = false;
                boolean showDataUnitHelpText = true;
                boolean showDataElementHelpText = false;

                // Ecriture de l'aide generale
                super.writeOnLineHelp(sIndent);

                // Aide sur les actions
                if (showActionHelpText) {
                        // Comptage des actions visibles
                        nActionNumber = 0;
                        if (vectorOfGUIActions == null)
                                vectorOfGUIActions = new Vector<GUIAction>();
                        for (nAction = 0; nAction < vectorOfGUIActions.size(); nAction++) {
                                action = (GUIAction)vectorOfGUIActions.elementAt(nAction);
                                if (action.getVisible() == true) {
                                        // Si l'action n'est pas une action standard d'une unite
                                        if (!action.getIdentifier().equals(Exit) &&
                                            !action.getIdentifier().equals(Refresh))
                                                nActionNumber++;
                                }
                        }

                        // Affichage de l'aide des actions
                        if (nActionNumber > 0) {
                                if (sIndent.equals(""))
                                        writeHelpText("");
                                writeHelpText(sIndent + "  Actions");
                                for (nAction = 0; nAction < vectorOfGUIActions.size(); nAction++) {
                                        action = (GUIAction)vectorOfGUIActions.elementAt(nAction);
                                        if (action.getVisible() == true) {
                                                if (!(action.getIdentifier().equals(Exit)) &&
                                                    !(action.getIdentifier().equals(Refresh))) {
                                                        action.writeOnLineHelp(sIndent + "  ");
                                                }
                                        }
                                }
                        }
                }

                // Aide sur les champs
                if (showDataElementHelpText && showDataUnitHelpText) {
                        // Comptage des champs visibles
                        if (vectorOfGUIDatas == null)
                                vectorOfGUIDatas = new Vector<GUIData>();
                        nFieldNumber = 0;
                        for (nField = 0; nField < vectorOfGUIDatas.size(); nField++) {
                                field = (GUIData)vectorOfGUIDatas.elementAt(nField);
                                if (field.getVisible() == true)
                                        nFieldNumber++;
                        }

                        // Affichage de l'aide des champs
                        if (nFieldNumber > 0) {
                                if (sIndent.equals(""))
                                        writeHelpText("");
                                writeHelpText(sIndent + "  Fields");
                                for (nField = 0; nField < vectorOfGUIDatas.size(); nField++) {
                                        field = (GUIData)vectorOfGUIDatas.elementAt(nField);
                                        if (field.getVisible() == true)
                                                field.writeOnLineHelp(sIndent + "  ");
                                }
                        }
                }

                // Aide sur les champs unites seulements
                if (!showDataElementHelpText && showDataUnitHelpText) {
                        for (nField = 0; nField < vectorOfGUIDatas.size(); nField++) {
                                field = (GUIData)vectorOfGUIDatas.elementAt(nField);
                                if (field.getVisible() && field instanceof GUIUnit) {
                                        if (sIndent.equals(""))
                                                writeHelpText("");
                                        field.writeOnLineHelp(sIndent + "  ");
                                }
                        }
                }
        }

        /**
         * Ecrit une action dans le fichier des scenarios
         *
         * @param sActionId L'identifiant de l'action
         */
        protected native void writeOutputUnitActionCommand(String sActionId);

        /**
         * Ecrit un changement de valeur dans le fichier des scenarios
         *
         * @param sFieldId L'identifiant de l'element
         * @param sValue   La valeur de l'element
         */
        public native void writeOutputUnitFieldCommand(String sFieldId, String sValue);

        /**
         * Ecrit un changement de selection au sein d'une liste Methode rattachee a
         * GUIUnit car la gestion des scenarios est centralisee au niveau des unites
         *
         * @param sValue La nouvelle valeur de l'index selectionne
         */
        public native void writeOutputUnitListIndexCommand(String sValue);

        /**
         * Affiche un message systeme dans le shell avec retour chariot
         *
         * @param sMessage Le message a afficher
         */
        public static void addTrace(String sMessage)
        {
                if (trace)
                        displaySystemMessage("trace: " + sMessage);
        }

        /**
         * Indicateur de trace
         */
        private static boolean trace = false;

        //////////////////////////////////////////////////////////////////////////
        // Principes de la synchronisation entre C++ et java
        //
        // a) open():
        // - ouverture depuis lancement par C++ d'un fenetre java
        // - toutes les specifications de la fenetres sont prealablement transferee de
        // C++ vers java avant appel par C++ de cette methode java
        // - la methode:
        // . creer un verrou closeLock
        // . ouvre la fenetre par appel a OpenGUI()
        // .. la fenetre enregistre un windowClosingListener appelant closeGUI()
        // .. la methode closeGUI est synchronise avec le verrou closeLock, qu'elle
        // notifie des que la frame n'est plus visible
        // . attention, la fenetre se lance dans son propre thread
        // (EDT: Event Dispatching Thread) et le programme C++ appelant
        // risque de continuer et de se terminer
        // . cree un thread de mise en attente de femeture de la fenetre
        // .. il fait une boucle d'attente sur la fermeture de la fenetre (non visible),
        // synchronisee avec le verrou closeLock, en faisant des wait() sur le verrou
        // .. il est lance par start(), puis on attend sa fin par join()
        // .. le programme C++ appelant est en attente de la fermeture de la fenetre
        // . cas specifique de gestion des UIFileChoosedrCard
        //
        // b) openGUI():
        // - ouverture de la fenetre, avec enregistrement du windowClosingListener
        //
        // c) closeGUI():
        // - fermeture de la fenetre, avec gestion du verrou closeLock
        //
        // d) actionPerformed():
        // - traitement des actions de type GUIAction
        // - si action d'identifiant "Exit" ou de parametre "Exit":
        // appel direct de guiClose()
        // - sinon
        // . appel de startModal() pour rendre la fenetre inactive
        // . creation et lancement d'un thread de traitement de l'action
        // .. appel de la methode C++ correspondant a l'action par executeUserActionAt()
        // .. si action de parametre "Exit": appel de closeGUI()
        // .. sinon, appel de stopModal() pour rendre la fenetre active
        //
        // e) startModal()
        // - memorisation du contexte:
        // . currentFrame, currentFrameState, currentDefaultCloseOperation,
        // - on provoque une perte de focus pour forcer la mise a jour des donnees C++
        // par les donnees de l'interface java avant execution de l'action
        // - desactivation de la fenetre et de ses composant (par des setEnabled(false))
        //
        // f) stopModal()
        // - recherche du contexte, et restitution de ce contexte
        // - activation de la fenetre et de ses composant (par des setEnabled(true))
        // - envoie d'un message de repeinture, qui sera traite dans l'EDT
        // apres la fin de l'action en cours
}