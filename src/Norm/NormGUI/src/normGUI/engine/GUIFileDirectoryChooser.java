// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.io.File;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.filechooser.FileNameExtensionFilter;

/**
 * Represente l'element graphique correspondant au style FileChooser. Affichage
 * d'une chaine de caracteres dans une zone de texte suivie d'un bouton. Le
 * bouton declenche l'ouverture d'une fenetre permettant de selectionner un
 * fichier (JFileChooser). Cette fenetre s'ouvre en pointant sur le repertoire
 * du dernier fichier selectionne. REf:
 * https://docs.oracle.com/javase/tutorial/uiswing/components/filechooser.html
 */
public class GUIFileDirectoryChooser extends GUIObject
{
        /**
         * FileChooser global, construit une fois pour toute et reutilise partout Cela
         * ne pose pas de probleme, car l'application ayant un comprtement modal, au
         * plus un FileChooser est utilise a la fois. Et cela permet a cet unique
         * FileChooser de conserver le contexte de selection entre deux appels.
         */
        private static JFileChooser globalFileChooser = new JFileChooser();

        /**
         * Indique si le FileChooser global a ete utilise au moiins une fois
         */
        private static boolean globalFileChooserUsed = false;

        /**
         * Cree un FileChooser en le parametrant correctement
         *
         * @param inputPath      Le chemin en entree pour lancer la recherche
         * @param selectionMode: le type de selection ("FileChooser" ou
         *                       "DirectoryChooser")
         * @param filterParams:  les parametres de filtrage sous la forme [label, ext1,
         *                       ext2,...]
         */

        public static JFileChooser createFileChooser(String inputPath, int selectionMode, String[] filterParams)
        {
                File currentDirectory = globalFileChooser.getCurrentDirectory();

                if (isUnix()) {
                        // Il y a un bug incomprehensible sur Linux :
                        // on n'arrive pas a initialiser le JFileChooser correctement entre 2 appels
                        // et seul le premier appel fonctionne a partir du deuxieme appel, le fichier
                        // selectionne est "" combien meme si l'utilisateur selectionne un fichier.
                        // Le seul moyen de gerer cette erreur est de construire un nouvel objet
                        // a chaque appel (en initialisant le repertoire courant)
                        globalFileChooser = new JFileChooser();
                        globalFileChooser.setCurrentDirectory(currentDirectory);
                }

                // Parametrage par defaut de libelles
                globalFileChooser.setDialogTitle("Open");
                globalFileChooser.setApproveButtonText("Open");
                globalFileChooser.setApproveButtonToolTipText("");

                // Parametrage de la selection
                globalFileChooser.setFileSelectionMode(selectionMode);
                globalFileChooser.resetChoosableFileFilters();

                // Parametrage d'un repertoire par defaut lors de la premiere ouverture
                if (!globalFileChooserUsed) {
                        globalFileChooserUsed = true;

                        // Parametrage du repertoire utilisateur
                        try {
                                globalFileChooser.setCurrentDirectory(new File(System.getProperty("user.dir")));
                        } catch (Exception ex) {
                        }
                }

                // Extraction des parametres de filtrage des extensions de fichier dans le cas
                // des fichiers
                String label;
                String[] extensions = null;
                if (selectionMode == JFileChooser.FILES_ONLY) {
                        if (filterParams != null && filterParams.length > 1) {
                                // Extraction des extensions
                                label = filterParams[0] + " Files (";
                                extensions = new String[filterParams.length - 1];
                                int i;
                                for (i = 1; i < filterParams.length; i++) {
                                        extensions[i - 1] = filterParams[i];
                                        if (i > 1)
                                                label = label + ";";
                                        label = label + "*." + filterParams[i];
                                }
                                label = label + ")";

                                // Parametrage du file chooser, avec le parametrage en priorite sur l'option par
                                // defaut "All files"
                                FileNameExtensionFilter filter = new FileNameExtensionFilter(label, extensions);
                                globalFileChooser.setFileFilter(filter);
                        }
                }

                // On parametre le fichier ou le directory selectionne
                try {
                        if (selectionMode == JFileChooser.FILES_ONLY) {
                                globalFileChooser.setSelectedFile(new File(inputPath));
                        } else if (selectionMode == JFileChooser.DIRECTORIES_ONLY) {
                                globalFileChooser.setCurrentDirectory(new File(inputPath));
                        }

                } catch (Exception ex) {
                }

                // Retourne le FileChooser construit et parametre
                return globalFileChooser;
        }

        /**
         * Test si on est une fiche avec un seul champ texte de type file chooser
         *
         * @param guiUnit L'unite d'interface a tester Ce type de fiche permet d'une
         *                part un parametrage complet d'un FileChooser, d'autre part
         *                assure une compatibilite ascendante par rapport aux anciennes
         *                fiches utilises auparavant ayant un seul champ String de style
         *                FileChooser, pour les versions de Khiops jusqu'a V9
         */
        static public boolean isFileChooserCard(GUIUnit guiUnit)
        {
                boolean ok = true;
                GUIData field = null;

                ok = ok && guiUnit.getStyle().equals("FileChooser");
                ok = ok && guiUnit instanceof GUICard;
                ok = ok && guiUnit.vectorOfGUIDatas.size() == 1;
                ok = ok && guiUnit.getActionCount() == 3;
                ok = ok && guiUnit.getVisibleActionCount() == 3;
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(0)).getIdentifier().equals("Exit");
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(1)).getIdentifier().equals("Refresh");
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(2)).getIdentifier().equals("OK");
                if (ok) {
                        field = (GUIData)(guiUnit.vectorOfGUIDatas.elementAt(0));
                }
                ok = ok && field instanceof GUIStringElement;
                ok = ok && (field.getStyle().equals("FileChooser") || field.getStyle().equals("DirectoryChooser"));
                return ok;
        }

        /**
         * Gestion du choix d'un fichier avec un FileChooser parametrer par un GUIUnit
         *
         * @param guiFileChooserCard L'unite d'interface parametrant le FileChooser
         * @param parentFrame        Frame parent du FileChooser
         */
        static public void startFileChooser(GUIUnit guiFileChooserCard, JFrame parentFrame)
        {
                File selectedFile = null;

                // Acces au champ et a sa valeur
                GUIElement fileNameField = (GUIElement)guiFileChooserCard.vectorOfGUIDatas.elementAt(0);
                GUIAction actionOK = (GUIAction)guiFileChooserCard.vectorOfGUIActions.elementAt(2);
                String inputFileName = fileNameField.getValueIn(guiFileChooserCard).toString();

                // Parametrage de la selection
                int selectionMode = JFileChooser.FILES_AND_DIRECTORIES;
                if (fileNameField.getStyle().equals("FileChooser"))
                        selectionMode = JFileChooser.FILES_ONLY;
                else if (fileNameField.getStyle().equals("DirectoryChooser"))
                        selectionMode = JFileChooser.DIRECTORIES_ONLY;

                // Creation et parametrage d'un FileChooser
                JFileChooser jfc =
                  createFileChooser(inputFileName, selectionMode, fileNameField.getParametersAsArray());

                // Parametrage du titre
                if (!guiFileChooserCard.getLabel().equals(""))
                        jfc.setDialogTitle(guiFileChooserCard.getLabel());

                // Parametrage du bouton "ouvrir"
                jfc.setApproveButtonText(actionOK.getLabel());
                if (!actionOK.getHelpText().equals(""))
                        jfc.setApproveButtonToolTipText(actionOK.getHelpText());

                // Ouverture du FileChooser
                int dialogResult = JFileChooser.CANCEL_OPTION;
                try {
                        dialogResult = jfc.showOpenDialog(parentFrame);
                } catch (Exception e) {
                        dialogResult = JFileChooser.CANCEL_OPTION;
                }

                // Traitement de son retour si validation utilisateur
                if (dialogResult == JFileChooser.APPROVE_OPTION) {
                        selectedFile = jfc.getSelectedFile();

                        String outputFileName = selectedFile.getPath();
                        if (!outputFileName.equals(inputFileName)) {
                                // Modification de la valeur
                                fileNameField.setValueIn(guiFileChooserCard, outputFileName);

                                // Enregistrement dans le scenario
                                guiFileChooserCard.writeOutputUnitFieldCommand(fileNameField.getIdentifier(),
                                                                               outputFileName);
                        }
                        // Enregistrement de la commande de sortie dans le scenartio
                        guiFileChooserCard.writeOutputUnitActionCommand("OK");
                }
                // Cas d'un cancel utilisateur
                else {
                        // Mise a vide de la valeur, sans l'enregistrer
                        fileNameField.setValueIn(guiFileChooserCard, "");

                        // Enregistrement de la commande de sortie dans le scenartio
                        guiFileChooserCard.writeOutputUnitActionCommand("Exit");
                }
        }

        /**
         * Variable memorisant l'OS
         */
        private static String OS = null;

        /**
         * Memorisation de l'OS
         */
        private static String getOsName()
        {
                if (OS == null) {
                        OS = System.getProperty("os.name");
                }
                return OS;
        }

        public static boolean isWindows() { return getOsName().startsWith("Windows"); }

        public static boolean isMac() { return (getOsName().indexOf("mac") >= 0); }

        public static boolean isUnix()
        {
                return (getOsName().indexOf("nix") >= 0 || getOsName().indexOf("nux") >= 0 ||
                        getOsName().indexOf("aix") > 0);
        }
}
