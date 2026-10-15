// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Font;
import java.awt.FontFormatException;
import java.awt.GraphicsEnvironment;
import java.awt.Insets;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.ToolTipManager;
import javax.swing.UIManager;

/**
 * Initialisation globale, en particulier en ce qui concerne le UIManager
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
                                        // On privilegie FlatLaf pour moderniser le rendu et conserver des boites
                                        // de dialogue fichiers reactives. En cas d'echec, on conserve le fallback
                                        // historique cross-plateforme (Metal).
                                        try {
                                                if (!initializeFlatLaf()) {
                                                        UIManager.setLookAndFeel(
                                                          UIManager.getCrossPlatformLookAndFeelClassName());
                                                }
                                        } catch (Exception e) {
                                                if (debug)
                                                        displayMessage(
                                                          "Impossible de charger le Look & Feel cross-plateforme");
                                        }

                                        // Choix d'une fonte par defaut
                                        Font defaultFont = loadDefaultFont();

                                        // Pour parametrer la fonte par defaut de toute l'UI.
                                        if (defaultFont != null) {
                                                UIManager.put("defaultFont", defaultFont);
                                                if (!UIManager.getLookAndFeel().getClass().getName().startsWith(
                                                      "com.formdev.flatlaf.")) {
                                                        for (Object key : UIManager.getLookAndFeelDefaults().keySet()
                                                                            .toArray()) {
                                                                if (key instanceof String &&
                                                                    ((String)key).endsWith(".font"))
                                                                        UIManager.put(key, defaultFont);
                                                        }
                                                }
                                        }

                                        // Marge interne des champs texte et des zones de texte
                                        Insets margin = new Insets(8, 10, 8, 10);
                                        UIManager.put("TextArea.margin", margin);
                                        UIManager.put("TextField.margin", margin);

                                        // Libelles des boites de dialogue
                                        Locale locale = new Locale.Builder()
                                                          .setLanguage("en")
                                                          .setScript("Latn")
                                                          .setRegion("US")
                                                          .build();
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
         * Initialise FlatLaf si la librairie est presente dans le classpath.
         */
        private static boolean initializeFlatLaf()
        {
                try {
                        try {
                                Class<?> flatLafClass = Class.forName("com.formdev.flatlaf.FlatLaf");
                                java.lang.reflect.Method registerCustomDefaultsSourceMethod =
                                  flatLafClass.getMethod("registerCustomDefaultsSource", String.class);
                                registerCustomDefaultsSourceMethod.invoke(null, "images");
                        } catch (Throwable ignored) {
                                // Ignorer les erreurs lors de l'enregistrement des sources de valeurs par defaut personnalisees
                        }

                        // Priorite au theme Arc Orange (flatlaf-intellij-themes).
                        // Fallback vers FlatLightLaf si le jar des themes n'est pas disponible.
                        try {
                                Class<?> flatArcOrangeClass =
                                  Class.forName("com.formdev.flatlaf.intellijthemes.FlatArcOrangeIJTheme");
                                Object flatArcOrangeLaf = flatArcOrangeClass.getDeclaredConstructor().newInstance();
                                UIManager.setLookAndFeel((javax.swing.LookAndFeel)flatArcOrangeLaf);
                                return true;
                        } catch (Throwable ignored) {
                                // Ignorer les erreurs lors du chargement du theme Arc Orange
                        }

                        Class<?> flatLightLafClass = Class.forName("com.formdev.flatlaf.FlatLightLaf");
                        Object flatLightLaf = flatLightLafClass.getDeclaredConstructor().newInstance();
                        UIManager.setLookAndFeel((javax.swing.LookAndFeel)flatLightLaf);
                        return true;
                } catch (Throwable e) {
                        if (debug)
                                displayMessage("Impossible de charger FlatLaf, fallback vers Metal");
                        return false;
                }
        }

        /**
         * Charge la fonte Poppins embarquee et retourne une fonte de taille standard.
         * Fallback vers une fonte systeme si Poppins n'est pas disponible.
         */
        private static Font loadDefaultFont()
        {
                try {
                        Font poppinsFont = loadBundledPoppinsFont();
                        if (poppinsFont != null)
                                return poppinsFont;

                        // Fallback: si la fonte est installee systeme, on peut encore l'utiliser.
                        Font systemPoppinsFont = new Font("Poppins", Font.PLAIN, 12);
                        if (!"Dialog".equals(systemPoppinsFont.getFamily()))
                                return systemPoppinsFont;

                        return new Font("MS Sans Serif", Font.PLAIN, 12);
                } catch (Exception e) {
                        if (debug)
                                displayMessage("Impossible de charger la fonte par defaut");
                        return null;
                }
        }

        /**
         * Charge et enregistre la fonte Poppins embarquee dans les ressources du jar.
         */
        private static Font loadBundledPoppinsFont() throws IOException, FontFormatException
        {
                String[] resourcePaths =
                  {"images/Poppins-Regular.ttf"};

                for (String resourcePath : resourcePaths) {
                        try (InputStream fontStream =
                               GUIManager.class.getClassLoader().getResourceAsStream(resourcePath)) {
                                if (fontStream == null)
                                        continue;

                                Font loadedFont = Font.createFont(Font.TRUETYPE_FONT, fontStream);
                                GraphicsEnvironment.getLocalGraphicsEnvironment().registerFont(loadedFont);
                                return loadedFont.deriveFont(Font.PLAIN, 12f);
                        }
                }
                return null;
        }

        /**
         * Indique si le GUIManager a ete initialise
         */
        private static boolean isInitialized = false;
}
