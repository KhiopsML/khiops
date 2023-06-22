// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Font;
import java.awt.Insets;
import java.util.Locale;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.ToolTipManager;
import javax.swing.UIManager;

/**
 * Initialisation globale, en particulier en ce qui concerne le UIManager
 *
 * @author Marc Boulle
 */
public class GUIManager extends GUIObject
{
        /**
         * Initialise le UIManager
         *
         * @see javax.swing.UIManager
         */
        public static void initialize()
        {
                if (isInitialized)
                        return;

                // Initialisation du manager
                try {
                        // On passe un Runnable pour se securiser vis a vis des thread
                        javax.swing.SwingUtilities.invokeLater(new Runnable() {
                                public void run()
                                {
                                        // Parametrage d'un gestionnaire d'exception par defaut
                                        Thread.setDefaultUncaughtExceptionHandler(new ExceptionHandler());
                                        System.setProperty("sun.awt.exception.handler",
                                                           ExceptionHandler.class.getName());

                                        // Initialisation du UIManager
                                        try {
                                                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
                                        } catch (Exception e) {
                                                if (debug)
                                                        displayMessage(
                                                          "Impossible de charger le Look & Feel du systeme");
                                        }

                                        // Choix d'une font par defaut
                                        Font defaultFont;
                                        try {
                                                defaultFont = new Font("MS Sans Serif", Font.PLAIN, 12);
                                        } catch (Exception e) {
                                                defaultFont = null;
                                                if (debug)
                                                        displayMessage("Impossible de charger la fonte systeme");
                                        }

                                        // Pour parametrer les fontes de tous les composants
                                        // Apparement, la seule solution est de parametrer la fonte de tous les
                                        // compsants
                                        if (defaultFont != null) {
                                                UIManager.put("Button.font", defaultFont);
                                                UIManager.put("CheckBox.font", defaultFont);
                                                UIManager.put("CheckBoxMenuItem.font", defaultFont);
                                                UIManager.put("ColorChooser.font", defaultFont);
                                                UIManager.put("ComboBox.font", defaultFont);
                                                UIManager.put("DesktopIcon.font", defaultFont);
                                                UIManager.put("EditorPane.font", defaultFont);
                                                UIManager.put("FormattedTextField.font", defaultFont);
                                                UIManager.put("Label.font", defaultFont);
                                                UIManager.put("List.font", defaultFont);
                                                UIManager.put("Menu.font", defaultFont);
                                                UIManager.put("MenuBar.font", defaultFont);
                                                UIManager.put("MenuItem.font", defaultFont);
                                                UIManager.put("OptionPane.font", defaultFont);
                                                UIManager.put("Panel.font", defaultFont);
                                                UIManager.put("PasswordField.font", defaultFont);
                                                UIManager.put("PopupMenu.font", defaultFont);
                                                UIManager.put("ProgressBar.font", defaultFont);
                                                UIManager.put("RadioButton.font", defaultFont);
                                                UIManager.put("RadioButtonMenuItem.font", defaultFont);
                                                UIManager.put("ScrollPane.font", defaultFont);
                                                UIManager.put("Slider.font", defaultFont);
                                                UIManager.put("Spinner.font", defaultFont);
                                                UIManager.put("TabbedPane.font", defaultFont);
                                                UIManager.put("Table.font", defaultFont);
                                                UIManager.put("TableHeader.font", defaultFont);
                                                UIManager.put("TextArea.font", defaultFont);
                                                UIManager.put("TextField.font", defaultFont);
                                                UIManager.put("TextPane.font", defaultFont);
                                                UIManager.put("TitledBorder.font", defaultFont);
                                                UIManager.put("ToggleButton.font", defaultFont);
                                                UIManager.put("ToolBar.font", defaultFont);
                                                UIManager.put("ToolTip.font", defaultFont);
                                                UIManager.put("Tree.font", defaultFont);
                                                UIManager.put("Viewport.font", defaultFont);
                                        }

                                        // TextArea
                                        Insets margin = new Insets(3, 3, 3, 3);
                                        UIManager.put("TextArea.margin", margin);

                                        // TextField
                                        UIManager.put("TextField.margin", margin);

                                        // Libelles des boites de dialogue
                                        Locale locale = new Locale("en", "US");
                                        JOptionPane.setDefaultLocale(locale);
                                        JFileChooser.setDefaultLocale(locale);

                                        // Parametrage de la duree des info-bulles: une minute (defaut: 4 s)
                                        ToolTipManager.sharedInstance().setDismissDelay(60000);

                                        // Delai avant apparition de l'infobulle 0.5 secondes (defaut: 0.75 s)
                                        ToolTipManager.sharedInstance().setInitialDelay(500);

                                        // Delai en deca duquel il n'y plus de delai initial si on change de composant:
                                        // 0.15 secondes (defaut: 0.5 s)
                                        ToolTipManager.sharedInstance().setReshowDelay(150);

                                        // Initialisation effectuee
                                        isInitialized = true;
                                }
                        });
                } catch (Exception e) {
                        // Initialisation effectuee: on ne re-essaiera pas
                        isInitialized = true;
                }
        }

        /**
         * Implementation d'un gestion d'exception, pour les exception non capturee
         */
        private static class ExceptionHandler implements Thread.UncaughtExceptionHandler
        {
                public void handle(Throwable thrown)
                {
                        // for EDT exceptions
                        handleException(Thread.currentThread().getName(), thrown);
                }

                public void uncaughtException(Thread thread, Throwable thrown)
                {
                        // for other uncaught exceptions
                        handleException(thread.getName(), thrown);
                }

                protected void handleException(String tname, Throwable thrown)
                {
                        System.out.println("Exception on " + tname);
                        thrown.printStackTrace();
                }
        }

        /**
         * Indique si le GUIManager a ete initialise
         */
        private static boolean isInitialized = false;
}
