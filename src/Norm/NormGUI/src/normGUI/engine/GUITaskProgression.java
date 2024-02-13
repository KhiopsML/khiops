// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;

/**
 * Definit une fenetre d'avertissement
 *
 * @author Marc Boulle
 */
public class GUITaskProgression extends JFrame
{

        /**
         * Indicateur d'une interruption
         */
        private boolean interrupted;

        /**
         * Indicateur d'ouverture
         */
        private boolean isOpened;

        /**
         * Niveau courant
         */
        private int currentLevel;

        /**
         * Nombre de niveaux de gestion de la progression
         */
        private int levelNumber;

        /**
         * Nombre maximum de niveaux de gestion de la progression
         */
        private int maxLevelNumber;

        /**
         * Libelles principals par niveau (JLabel)
         */
        private Vector<JLabel> mainLabels;

        /**
         * Libelles par niveau (JLabel)
         */
        private Vector<JLabel> labels;

        /**
         * Barres de progression par niveau (JProgressBar)
         */
        private Vector<JProgressBar> progressBars;

        /**
         * Bouton d'interruption
         */
        private JButton stop;

        /**
         * Pourcentages de progression par niveau (int)
         */
        private Vector<Integer> progressions;

        /**
         * Cree la fenetre de progression
         */
        public GUITaskProgression()
        {
                super();

                // Variables locales
                JLabel mainLabel;
                JProgressBar progressBar;
                JLabel label;
                Integer progression;

                // Mise en place d'un simple buffering (cf. GUIUnit)
                createBufferStrategy(1);

                // Creation des containers
                mainLabels = new Vector<JLabel>();
                labels = new Vector<JLabel>();
                progressBars = new Vector<JProgressBar>();
                progressions = new Vector<Integer>();

                // Initialisation des tailles des containers
                maxLevelNumber = 20;
                mainLabels.setSize(maxLevelNumber);
                labels.setSize(maxLevelNumber);
                progressBars.setSize(maxLevelNumber);
                progressions.setSize(maxLevelNumber);

                // Initialisation du contenu des containers
                for (int level = 0; level < maxLevelNumber; level++) {
                        // Creation des composants de la fenetre
                        mainLabel = new JLabel();
                        GUIObject.setComponentPreferredSize(mainLabel, 10);
                        progressBar = new JProgressBar();
                        GUIObject.setComponentPreferredSize(progressBar, 10);
                        label = new JLabel();
                        GUIObject.setComponentPreferredSize(label, 10);
                        progression = new Integer(0);

                        // Memorisation dans les containers
                        mainLabels.setElementAt(mainLabel, level);
                        progressBars.setElementAt(progressBar, level);
                        labels.setElementAt(label, level);
                        progressions.setElementAt(progression, level);
                }

                // Initialisation des niveaux
                currentLevel = 0;
                levelNumber = 1;

                // Initialisation du contenu
                initializeContentPane();

                // Interdiction de fermer la fenetre
                setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        }

        /**
         * Indique si la fenetre a ete ouverte au moins une fois
         */
        boolean openedOnce = false;

        /*
         * Parametrage de la position, uniquement a la premiere utilisation
         */
        public void setInitialLocation()
        {
                if (!openedOnce) {
                        Rectangle currentScreenBounds = GUIObject.getCurrentScreenBounds();

                        // Preparation de la fenetre
                        pack();

                        // Positionnnement de la fenetre
                        setSize(currentScreenBounds.width / 3, (int)getSize().getHeight());
                        setLocation(currentScreenBounds.x + currentScreenBounds.width / 30,
                                    currentScreenBounds.y + currentScreenBounds.height / 30);
                }
        }

        /**
         * Initialise le contenu graphique
         */
        void initializeContentPane()
        {
                // On vide tout au prealable
                getContentPane().removeAll();

                // Creation des contraintes pour le GridBagLayout
                GridBagConstraints constraints = new GridBagConstraints();
                constraints.anchor = GridBagConstraints.WEST;
                constraints.fill = GridBagConstraints.HORIZONTAL;
                constraints.weightx = 1.0;
                constraints.weighty = 1.0;
                constraints.insets = new Insets(3, 3, 3, 3);
                constraints.gridwidth = GridBagConstraints.REMAINDER;

                // On cree le panel nord dans un scrollpane
                JPanel northPanel = new JPanel(new GridBagLayout());
                JScrollPane scrollPane = new JScrollPane(northPanel);
                getContentPane().setLayout(new BorderLayout());
                getContentPane().add(scrollPane, BorderLayout.CENTER);

                // Creation des sous-panneaux et ajout au panneau principal
                JPanel progressionPanel;
                for (int level = 0; level < levelNumber; level++) {
                        progressionPanel = buildProgressionPanel(level);
                        northPanel.add(progressionPanel, constraints);
                }

                // Ajout du panneau sud
                JPanel southPanel = buildInterruptionPanel();
                getContentPane().add(southPanel, BorderLayout.SOUTH);

                // Preparation de la fenetre
                pack();

                // On recupere les dimensions de l'ecran
                int screenWidth = Toolkit.getDefaultToolkit().getScreenSize().width;
                int screenHeight = Toolkit.getDefaultToolkit().getScreenSize().height;

                // Dimensionnement de la fenetre
                setSize(screenWidth / 3, (int)getSize().getHeight());
                setResizable(false);
        }

        /**
         * Cree un panneau de suivi pour un niveau donne
         */
        JPanel buildProgressionPanel(int level)
        {
                // Le niveau doit etre correcte
                if (level < 0 || level > levelNumber)
                        return null;

                // Recherches des composants de la fenetre
                JLabel mainLabel = (JLabel)mainLabels.get(level);
                JProgressBar progressBar = (JProgressBar)progressBars.get(level);
                JLabel label = (JLabel)labels.get(level);

                // Creation des contraintes pour le GridBagLayout
                GridBagConstraints constraints = new GridBagConstraints();
                constraints.anchor = GridBagConstraints.WEST;
                constraints.fill = GridBagConstraints.HORIZONTAL;
                constraints.weightx = 1.0;
                constraints.weighty = 1.0;
                constraints.insets = new Insets(3, 3, 3, 3);
                constraints.gridwidth = GridBagConstraints.REMAINDER;

                // Creation du panneau
                JPanel panel = new JPanel(new GridBagLayout());

                // Creation et ajout des composants au panel
                panel.setBorder(BorderFactory.createEtchedBorder());
                panel.add(mainLabel, constraints);
                panel.add(progressBar, constraints);
                panel.add(label, constraints);

                return panel;
        }

        /**
         * Cree un panneau contenant le bouton d'interruption du suivi
         */
        JPanel buildInterruptionPanel()
        {
                // Creation des composants de la fenetre
                stop = new JButton("Stop");
                stop.addActionListener(new ActionListener() {
                        public void actionPerformed(ActionEvent e)
                        {
                                interrupted = true;
                                stop.setEnabled(false);
                        }
                });

                // Creation des contraintes pour le GridBagLayout
                GridBagConstraints constraints = new GridBagConstraints();
                constraints.anchor = GridBagConstraints.WEST;
                constraints.fill = GridBagConstraints.HORIZONTAL;
                constraints.weightx = 1.0;
                constraints.weighty = 1.0;
                constraints.insets = new Insets(3, 3, 3, 3);
                constraints.gridwidth = GridBagConstraints.REMAINDER;

                // Creation du panneau
                JPanel panel = new JPanel(new GridBagLayout());

                // Le bouton doit etre centre sans etre redimensionne
                constraints.anchor = GridBagConstraints.CENTER;
                constraints.fill = GridBagConstraints.NONE;
                panel.add(stop, constraints);

                return panel;
        }

        /**
         * Ouvre la fenetre
         */
        public void open()
        {
                // Initialisation du contenu
                initializeContentPane();

                // Initialisation dans un etat interruptible
                isOpened = true;
                interrupted = false;
                stop.setEnabled(true);

                // Parametrage de l'affichage des barres de progression
                JProgressBar progressBar;
                Integer progression;
                for (int level = 0; level < levelNumber; level++) {
                        progressBar = (JProgressBar)progressBars.get(level);
                        progression = (Integer)progressions.get(level);
                        progressBar.setBorderPainted(progression.intValue() != 0);
                }

                // Affichage
                setInitialLocation();
                openedOnce = true;
                setVisible(true);
        }

        /**
         * Ferme le fenetre
         */
        public void close()
        {
                // Fermeture de la fenetre
                isOpened = false;
                interrupted = false;
                dispose();
        }

        /**
         * Lance l'ouverture dans un thread
         */
        public void run()
        {
                setInitialLocation();
                openedOnce = true;
                setVisible(true);
        }

        /**
         * Renvoie le nombre de niveaux
         *
         * @return Le nombre de niveaux
         */
        public int getLevelNumber() { return levelNumber; }

        /**
         * Modifie le nombre de niveaux
         *
         * @param number Le nouveau nombre de niveaux
         */
        public void setLevelNumber(int number)
        {
                if (isOpened())
                        return;
                else if (number < 1)
                        levelNumber = 1;
                else if (number > maxLevelNumber)
                        levelNumber = maxLevelNumber;
                else
                        levelNumber = number;
        }

        /**
         * Renvoie le niveau courant
         *
         * @return Le niveau courant
         */
        public int getCurrentLevel() { return currentLevel; }

        /**
         * Modifie le niveau courant
         *
         * @param level Le nouveau niveau courant
         */
        public void setCurrentLevel(int level)
        {
                if (level < 0 || level > levelNumber)
                        return;
                currentLevel = level;
        }

        /**
         * Renvoie le libelle
         *
         * @return Le libelle
         */
        public String getLabel()
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return new String();
                JLabel label = (JLabel)labels.get(currentLevel);
                return label.getText();
        }

        /**
         * Renvoie le libelle principal
         *
         * @return Le libelle principal
         */
        public String getMainLabel()
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return new String();
                JLabel mainLabel = (JLabel)mainLabels.get(currentLevel);
                return mainLabel.getText();
        }

        /**
         * Renvoie le poucentage de progression
         *
         * @return Le pourcentage de progression
         */
        public int getProgression()
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return 0;
                Integer progression = (Integer)progressions.get(currentLevel);
                return progression.intValue();
        }

        /**
         * Indique si la fenetre est ouverte
         *
         * @return Booleen indiquant si la fenetre est ouverte
         */
        public boolean isOpened() { return isOpened; }

        /**
         * Modifie le libelle
         *
         * @param sLabel Le nouveau libelle
         */
        public void setLabel(String sLabel)
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return;
                JLabel label = (JLabel)labels.get(currentLevel);
                label.setText(sLabel);
        }

        /**
         * Modifie le libelle principal
         *
         * @param sMainLabel Le nouveau libelle principal
         */
        public void setMainLabel(String sMainLabel)
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return;
                JLabel mainLabel = (JLabel)mainLabels.get(currentLevel);
                mainLabel.setText(sMainLabel);
        }

        /**
         * Modifie le pourcentage de progression
         *
         * @param progression Le nouveau pourcentage de progression
         */
        public void setProgression(int progressionValue)
        {
                if (currentLevel < 0 || currentLevel > levelNumber)
                        return;
                Integer progression = new Integer(progressionValue);
                progressions.setElementAt(progression, currentLevel);
                JProgressBar progressBar = (JProgressBar)progressBars.get(currentLevel);
                progressBar.setValue(progressionValue);
                progressBar.setBorderPainted(progressionValue != 0);
        }

        /**
         * Indique si le traitement doit etre interrompu
         *
         * @return Booleen indiquant si le traitement doit etre interrompu
         */
        public boolean isInterruptionRequested() { return interrupted; }
}
