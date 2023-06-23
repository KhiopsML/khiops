// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Dimension;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.GridBagConstraints;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.Window;
import java.io.File;
import java.net.URL;
import javax.swing.ImageIcon;
import javax.swing.JComponent;
import javax.swing.JTextField;

/**
 * Definit un objet d'interface
 *
 * @author Marc Boulle
 */
public abstract class GUIObject
{

        /**
         * Mode debug
         */
        public static boolean debug = false;

        /**
         * Fenetre globale de gestion de l'aide en ligne
         */
        private static GUIMessage helpWindow = new GUIMessage("Help");

        /**
         * Fenetre globale de gestion des messages
         */
        private static GUIMessage messageWindow = new GUIMessage("Message log");

        /**
         * Fenetre globale de progression
         */
        protected static GUITaskProgression taskProgression = new GUITaskProgression();

        /**
         * Renvoie le gestionnaire de suivi de progression
         */
        public static GUITaskProgression getTaskProgressionManager() { return taskProgression; }

        /**
         * Nettoie la fenetre d'aide
         */
        protected static void clearHelpWindow() { helpWindow.clear(); }

        /**
         * Affiche un message dans la fenetre globale sans retour chariot
         *
         * @param sMessage Le message a afficher
         */
        public static void displayMessageChars(String sMessage)
        {
                if (!debug)
                        messageWindow.displayMessageChars(sMessage);
                // En mode debug, on passe directement par la console pour eviter le probleme
                // des erreurs de la fenetre de message elle-meme (boucle infinie)
                else
                        System.out.println(sMessage);
        }

        /**
         * Affiche un message dans la fenetre globale avec retour chariot
         *
         * @param sMessage Le message a afficher
         */
        public static void displayMessage(String sMessage)
        {
                sMessage += "\n";
                messageWindow.displayMessageChars(sMessage);
        }

        /**
         * Affiche un message systeme dans le shell avec retour chariot
         *
         * @param sMessage Le message a afficher
         */
        public static void displaySystemMessage(String sMessage) { System.out.println(sMessage); }

        /**
         * Modifie le chemin de l'icone a utiliser dans toutes les nouvelles fenetres
         *
         * @param sIconPath Le path de l'icone a utiliser
         */
        public static void setFrameIconPath(String sIconPath)
        {
                sFrameIconPath = sIconPath;

                // Parametrage de l'icone des fenetres
                ImageIcon frameIcon = getImageIcon(getFrameIconPath());
                Image frameIcomImage = null;
                if (frameIcon != null)
                        frameIcomImage = frameIcon.getImage();
                messageWindow.setIconImage(frameIcomImage);
                helpWindow.setIconImage(frameIcomImage);
                taskProgression.setIconImage(frameIcomImage);
        }

        /**
         * Renvoie le chemin de l'icone a utiliser dans toutes les nouvelles fenetres
         *
         * @return Le path de l'icone a utiliser
         */
        public static String getFrameIconPath() { return sFrameIconPath; }

        /**
         * Chemin de l'icone a utiliser dans toutes les nouvelles fenetres
         */
        private static String sFrameIconPath = "khiops.gif";

        /**
         * Renvoie la version du moteur d'interface (compatibilite avec la partie C++)
         *
         * @return La version du moteur d'interface
         */
        private static String getGUIEngineVersion() { return "Swing Version 6.0"; }

        /**
         * Renvoie le chemin d'un fichier de ressource se trouvant soit dans le
         * repertoire courant, soit dans le CLASSPATH, soit dans repertoire de nom "dat"
         * frere d'un repertoire du CLASSPATH
         *
         * @param sRessourceFileName Le nom du fichier de ressource
         * @return Le chemin complet de la ressource, ou chaine vide si non trouve
         */
        protected static String getRessourcePath(String sRessourceFileName)
        {
                String sClassPath;
                String[] paths;
                String sNextPath;
                String sPath;
                File file;
                int nDirectoryIndex;
                int i;

                if (sRessourceFileName.equals(""))
                        return "";

                // Recherche du PATH pour analyse
                sClassPath = System.getProperty("java.library.path");
                paths = sClassPath.split(";");

                // Recherche dans le repertoire courant
                file = new File(sRessourceFileName);
                if (file.canRead())
                        return sRessourceFileName;

                // Recherche du fichier dans tous les chemins du CLASSPATH
                for (i = 0; i < paths.length; i++) {
                        sNextPath = paths[i];

                        // Recherche dans le CLASSPATH
                        sPath = sNextPath + File.separator + sRessourceFileName;
                        file = new File(sPath);
                        if (file.canRead())
                                return sPath;

                        // Recherche dans un repertoire frere de nom "dat"
                        sPath = sNextPath + File.separator + ".." + File.separator + "dat" + File.separator +
                                sRessourceFileName;
                        file = new File(sPath);
                        if (file.canRead())
                                return sPath;

                        // Recherche dans un repertoire fils de nom "dat"
                        sPath = sNextPath + File.separator + sRessourceFileName;
                        file = new File(sPath);
                        if (file.canRead())
                                return sPath;
                }
                return "";
        }

        /*
         * Recherche des caracteristiques de l'ecran courant, contena t la fenetre
         * active
         */
        static public Rectangle getCurrentScreenBounds()
        {
                Rectangle currentScreenBounds = null;
                Rectangle screenBounds = null;
                Window activeWindow = null;
                Rectangle activeWindowBounds = null;
                Rectangle commonBounds = null;
                int screenWidth = 0;
                int screenHeight = 0;
                int x = 0;
                int y = 0;

                // Recherche de la fenetre active et de sa position
                try {
                        activeWindow = javax.swing.FocusManager.getCurrentManager().getActiveWindow();
                        activeWindowBounds = activeWindow.getBounds();
                } catch (Exception ex) {
                        activeWindowBounds = null;
                }

                // On recherche l'ecran contenant la fenetre active si elle est disponible
                if (activeWindowBounds != null) {
                        int screen;

                        // Recherche des ecrans
                        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
                        GraphicsDevice[] gd = ge.getScreenDevices();

                        // Recherche de l'ecran contenant la plus grande proportion de la fenetre active
                        double bestCommonSurface = -1;
                        double commonSurface = -1;
                        for (screen = 0; screen < gd.length; screen++) {
                                screenBounds = gd[screen].getDefaultConfiguration().getBounds();

                                // Calcul de la surface commune entre l'ecran et la fenetre
                                commonBounds = screenBounds.intersection(activeWindowBounds);
                                if (commonBounds.getWidth() > 0 && commonBounds.getHeight() > 0)
                                        commonSurface = commonBounds.getWidth() * commonBounds.getHeight();

                                // Test si meilleur ecran trouve
                                if (commonSurface > bestCommonSurface) {
                                        currentScreenBounds = screenBounds;
                                        bestCommonSurface = commonSurface;
                                }
                        }
                }

                // Cas ou on a pas trouve d'ecran contenant le fenetre active
                if (currentScreenBounds == null) {
                        // On recupere les dimension de l'ecran par defaut
                        screenWidth = Toolkit.getDefaultToolkit().getScreenSize().width;
                        screenHeight = Toolkit.getDefaultToolkit().getScreenSize().height;

                        // Positionnnement de la fenetre
                        currentScreenBounds = new Rectangle(0, 0, screenWidth, screenHeight);
                }
                return currentScreenBounds;
        }

        /**
         * Method utilitaire d'affichage du contenu d'un GridBagConstrainte
         */
        public void displayGridBagConstraint(String title, GridBagConstraints constraint)
        {
                displaySystemMessage(title);
                displaySystemMessage("\tgridx:  " + constraint.gridx);
                displaySystemMessage("\tgridy:  " + constraint.gridy);
                displaySystemMessage("\tgridwidth:  " + constraint.gridwidth);
                displaySystemMessage("\tgridheight:  " + constraint.gridheight);
                displaySystemMessage("\tweightx:  " + constraint.weightx);
                displaySystemMessage("\tweighty:  " + constraint.weighty);
                displaySystemMessage("\tanchor:  " + constraint.anchor);
                displaySystemMessage("\tfill:  " + constraint.fill);
                displaySystemMessage("\tinsets:  " + constraint.insets);
                displaySystemMessage("\tipadx:  " + constraint.ipadx);
                displaySystemMessage("\tipady:  " + constraint.ipady);
        }

        /**
         * Affiche la version du moteur d'interface
         */
        public static void main(String[] args)
        {
                System.out.println("Java GUI engine version: " + getGUIEngineVersion());
                System.exit(0);
        }

        /**
         * Visibilite de l'objet d'interface
         */
        private boolean bVisible;

        /**
         * Raccourci clavier
         */
        private char cShortCut;

        /**
         * Text d'aide
         */
        private String sHelpText;

        /**
         * Identifiant
         */
        private String sIdentifier;

        /**
         * Libelle
         */
        private String sLabel;

        /**
         * Parametres
         */
        private String sParameters;

        /**
         * Style
         */
        private String sStyle;

        /**
         * Renvoie le texte d'aide de l'objet d'interface
         *
         * @return Le texte d'aide de l'objet d'interface
         */
        public String getHelpText() { return sHelpText; }

        /**
         * Renvoie l'identifiant de l'objet d'interface
         *
         * @return L'identifiant de l'objet d'interface
         */
        public String getIdentifier() { return sIdentifier; }

        /**
         * Parametrage de la taille preferee d'un composant, en nombre de caracteres
         */
        static public void setComponentPreferredSize(JComponent component, int nCharNumber)
        {
                // Parametrage de la taille preferee
                component.setPreferredSize(
                  new Dimension(getComponentPreferredWidth(nCharNumber), getComponentPreferredHeight()));
        }

        /**
         * Largeur d'un composant en nombre de caracteres
         */
        static public int getComponentPreferredWidth(int nCharNumber)
        {
                int charWidth;

                // Recherche de la dimension preferee d'un textfield
                // Solution bidulique pour avoir la taille moyenne par caractere, en prenant la taille moyenne entre 'n'
                // et 'm'
                JTextField textFieldEmpty = new JTextField("");
                JTextField textFieldMN = new JTextField("mn");
                charWidth =
                  (int)(textFieldMN.getPreferredSize().getWidth() - textFieldEmpty.getPreferredSize().getWidth()) / 2;
                return nCharNumber * charWidth + 2;
        }

        /**
         * Hauteur par defaut des composants
         */
        static public int getComponentPreferredHeight()
        {
                Dimension dim = null;

                // Recherche de la dimension preferee d'un textfield
                JTextField textField = new JTextField();
                dim = textField.getPreferredSize();
                return (int)(dim.getHeight());
        }

        /**
         * Renvoie l'ImageIcon stockee dans le jar
         *
         * @param path Le chemin vers l'image
         * @return L'ImageIcon stockee dans le jar
         */
        static public ImageIcon getImageIcon(String path)
        {
                try {
                        URL url = messageWindow.getClass().getClassLoader().getResource(path);
                        return new ImageIcon(url);
                } catch (NullPointerException e) {
                        return null;
                }
        }

        /**
         * Renvoie le libelle de l'objet d'interface
         *
         * @return Le libelle de l'objet d'interface
         */
        public String getLabel() { return sLabel; }

        /**
         * Renvoie les parametres de l'objet d'interface
         *
         * @return Les parametres de l'objet d'interface
         */
        public String getParameters() { return sParameters; }

        /**
         * Renvoie les parametres sous la forme d'un tableau de chaines de caracteres
         *
         * @return Le tableau de chaines contenant les parametres
         */
        public String[] getParametersAsArray()
        {
                if (sParameters != null) {
                        // On ajoute un blanc en fin des parametres pour forcer la methode split a cree
                        // un dernier token
                        String[] params = (sParameters + " ").split("\n");
                        // On supprime le blan du dernier token
                        String lastToken = params[params.length - 1];
                        params[params.length - 1] = lastToken.substring(0, lastToken.length() - 1);
                        return params;
                } else
                        return null;
        }

        /**
         * Renvoie le raccourci clavier de l'objet d'interface
         *
         * @return Le raccourci clavier de l'objet d'interface
         */
        public char getShortCut() { return cShortCut; }

        /**
         * Renvoie le style de l'objet d'interface
         *
         * @return Le style de l'objet d'interface
         */
        public String getStyle() { return sStyle; }

        /**
         * Renvoie la visibilite de l'objet d'interface
         *
         * @return La visibilite de l'objet d'interface
         */
        public boolean getVisible() { return bVisible; }

        /**
         * Modifie le texte d'aide
         *
         * @param sValue Le nouveau texte d'aide
         */
        public void setHelpText(String sValue) { sHelpText = sValue; }

        /**
         * Parametrage de l'info-bulle d'un composant java avec le texte d'aide
         *
         * @param sValue Le nouveau texte d'aide
         */
        public void setComponentHelpText(JComponent component)
        {
                try {
                        if (!getHelpText().equals(""))
                                component.setToolTipText(getHelpText());
                } catch (Exception e) {
                }
        }

        /**
         * Modifie l'identifiant
         *
         * @param sValue Le nouvel identifiant
         */
        public void setIdentifier(String sValue) { sIdentifier = sValue; }

        /**
         * Modifie le libelle
         *
         * @param sValue Le nouveau libelle
         */
        public void setLabel(String sValue) { sLabel = sValue; }

        /**
         * Modifie les parametres
         *
         * @param sValue Les nouveaux parametres
         */
        public void setParameters(String sValue) { sParameters = sValue; }

        /**
         * Modifie le raccourci clavier
         *
         * @param cValue Le nouveau raccourci clavier
         */
        public void setShortCut(char cValue) { cShortCut = cValue; }

        /**
         * Modifie le style
         *
         * @param sValue Le nouveau style
         */
        public void setStyle(String sValue) { sStyle = sValue; }

        /**
         * Modifie la visibilite
         *
         * @param bValue La nouvelle visibilite
         */
        public void setVisible(boolean bValue) { bVisible = bValue; }

        /**
         * Ajoute un texte d'aide dans la fenetre d'aide
         *
         * @param sText Le texte d'aide a ajouter
         */
        public void writeHelpText(String sText) { helpWindow.displayMessageChars(sText + "\n"); }

        /**
         * Ecrit l'aide en ligne d'un composant d'interface
         *
         * @param sIndent Le texte d'aide
         */
        public void writeOnLineHelp(String sIndent)
        {
                if (!getLabel().equals("")) {
                        if (sIndent.equals(""))
                                writeHelpText(sIndent + getLabel());
                        else
                                writeHelpText(sIndent + ". " + getLabel());
                        if (!getHelpText().equals("")) {
                                writeHelpText(getHelpText());
                                writeHelpText("");
                        }
                }
        }
}