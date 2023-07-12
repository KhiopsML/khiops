// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.SwingUtilities;
import javax.swing.text.BadLocationException;
import javax.swing.text.Style;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyleContext;
import javax.swing.text.StyledDocument;

/**
 * Definit une fenetre d'avertissement
 */
public class GUIMessage extends JFrame
{
        /**
         * Cree la fenetre d'avertissement
         *
         * @param sTitle Le titre de la fenetre
         */
        public GUIMessage(String sTitle)
        {
                super(sTitle);

                // Initialisation globale
                GUIManager.initialize();

                // Mise en place d'un simple buffering (cf. GUIUnit)
                createBufferStrategy(1);

                // Initialisation des nombre de messages
                messageNumber = 0;
                displayedMessageNumber = 0;
                currentTextLength = 0;

                // On recupere les dimensions de l'ecran
                int screenWidth = Toolkit.getDefaultToolkit().getScreenSize().width;
                int screenHeight = Toolkit.getDefaultToolkit().getScreenSize().height;

                // Creation de la zone de texte
                pane = new NonWrappingTextPane();
                pane.setText("");
                pane.setAutoscrolls(true);
                pane.setEditable(false);
                pane.setMargin(new Insets(10, 10, 10, 10));

                // Creation et ajout des composants au panel
                JScrollPane scrollPane = new JScrollPane(pane);
                scrollPane.setPreferredSize(new Dimension(screenWidth / 3, screenHeight / 3));
                getContentPane().add(scrollPane, BorderLayout.CENTER);
                JPanel panelButton = new JPanel();
                JButton buttonClear = new JButton("Clear");
                buttonClear.addActionListener(new ActionListener() {
                        public void actionPerformed(ActionEvent e)
                        {
                                clear();
                        }
                });
                JButton buttonOk = new JButton("Close");
                buttonOk.addActionListener(new ActionListener() {
                        public void actionPerformed(ActionEvent e)
                        {
                                dispose();
                        }
                });

                panelButton.add(buttonClear);
                panelButton.add(buttonOk);
                getContentPane().add(panelButton, BorderLayout.SOUTH);
                pack();

                // Definition des styles
                StyledDocument doc = pane.getStyledDocument();
                Style def = StyleContext.getDefaultStyleContext().getStyle(StyleContext.DEFAULT_STYLE);

                // Style par defaut
                Style regular = doc.addStyle("regular", def);

                // Style pour les erreurs fatales
                Style fatalError = doc.addStyle("fatal error", regular);
                StyleConstants.setFontSize(fatalError, StyleConstants.getFontSize(fatalError) * 3 / 2);
                StyleConstants.setForeground(fatalError, Color.RED);
                StyleConstants.setBold(fatalError, true);

                // Style pour les erreurs
                Style error = doc.addStyle("error", regular);
                StyleConstants.setForeground(error, Color.RED);
                StyleConstants.setBold(error, true);

                // Style pour les warnings
                Style warning = doc.addStyle("warning", regular);
                StyleConstants.setBold(warning, true);
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

                        // Positionnnement de la fenetre
                        setSize(currentScreenBounds.width / 3, currentScreenBounds.height / 3);
                        setLocation(currentScreenBounds.x + currentScreenBounds.width / 30,
                                    currentScreenBounds.y + 2 * currentScreenBounds.height / 30 +
                                      currentScreenBounds.height / 4);
                }
        }

        /**
         * Efface le contenu de la fenetre
         */
        public void clear()
        {
                try {
                        // On passe un Runnable pour se securiser vis a vis des thread
                        // Sinon, la modification du Caret en fin de methode genere des exceptions.
                        // cf.
                        // http://forums.codeguru.com/showthread.php?503858-RESOLVED-JTextArea-Null-Pointer-Exception-on-append
                        javax.swing.SwingUtilities.invokeLater(new Runnable() {
                                public void run()
                                {
                                        StyledDocument doc = null;
                                        if (pane != null)
                                                doc = pane.getStyledDocument();
                                        if (doc != null) {
                                                try {
                                                        doc.remove(0, doc.getLength());
                                                        currentTextLength = 0;
                                                } catch (BadLocationException e) {
                                                }
                                        }
                                }
                        });
                } catch (Exception e) {
                }
        }

        /**
         * Affiche un message
         *
         * @param s Le message a afficher
         */
        public void displayMessageChars(final String message)
        {
                // Incrementation du nombre de message
                messageNumber = messageNumber + 1;

                // Affichage du message
                try {
                        // On passe un Runnable pour se securiser vis a vis des thread
                        // Sinon, la modification du Caret en fin de methode genere des exceptions.
                        // cf.
                        // http://forums.codeguru.com/showthread.php?503858-RESOLVED-JTextArea-Null-Pointer-Exception-on-append

                        // Une fois sur 200, on attend la fin du thread pour nettoyer les ressources de
                        // Java
                        // potentiellement fortement sollicitee en cas d'envoi massif de messages
                        // On utilise egalement le invokeAndWait, plus sur, pour les faibles nombre de
                        // messages
                        // en esperant resoudre les (rare) "plantages" sous Linux pour les executions
                        // tres courtes
                        //
                        // Attention: on ne peut utiliser le invokeAndWait quand on est sur le thread
                        // principal de Swing (EventDispatchThread)
                        if (!SwingUtilities.isEventDispatchThread() &&
                            (messageNumber < 100 || messageNumber % 200 == 0)) {
                                javax.swing.SwingUtilities.invokeAndWait(new Runnable() {
                                        public void run()
                                        {
                                                displayMessageCharsInternal(message);
                                        }
                                });
                        }
                        // Le reste du temps, on fait une invocation differee, beaucoup plus rapide
                        else {
                                javax.swing.SwingUtilities.invokeLater(new Runnable() {
                                        public void run()
                                        {
                                                displayMessageCharsInternal(message);
                                        }
                                });
                        }
                } catch (Exception e) {
                }
        }

        /**
         * Affiche un message
         *
         * @param s Le message a afficher
         */
        private void displayMessageCharsInternal(String message)
        {
                displayedMessageNumber = displayedMessageNumber + 1;
                try {
                        StyledDocument doc = null;
                        if (pane != null)
                                doc = pane.getStyledDocument();
                        if (doc != null) {
                                // On limite la taille du message
                                boolean toFront = false;
                                int maxTextLength = 100000;
                                if (currentTextLength + message.length() > maxTextLength) {
                                        String currentText = pane.getText();
                                        int startIndex;
                                        int nextIndex;
                                        int removedLength;
                                        int removedLines;

                                        // Determination de la quantite de texte a supprimer, base
                                        // sur un nombre entier de lignes
                                        removedLength = 0;
                                        removedLines = 0;
                                        for (startIndex = 0; startIndex < maxTextLength / 2; startIndex++) {
                                                if (currentText.charAt(startIndex) == '\n') {
                                                        removedLength = startIndex;
                                                        removedLines++;
                                                }
                                        }
                                        if (removedLength == 0)
                                                removedLength = currentTextLength / 2;

                                        // On supprime le debut dans le panneau, qui memorise les styles utilises
                                        if (removedLength > 0) {
                                                try {
                                                        // Correction "heuristique" sur le nombre de caracteres a
                                                        // supprimer (Les positions ne sont pas les meme dans le doc et
                                                        // dans son texte
                                                        doc.remove(0, removedLength - removedLines);

                                                        // On ajoute ... en tete
                                                        doc.insertString(0, "...", doc.getStyle("regular"));

                                                        // Reactualisation de la taille du texte
                                                        currentTextLength = doc.getLength();
                                                } catch (BadLocationException e) {
                                                }
                                        }
                                }

                                // On affiche le message a la suite
                                try {
                                        // On identifie le type de message pour parametrer le style
                                        if (message.startsWith("fatal error :")) {
                                                doc.insertString(
                                                  doc.getLength(), "fatal error", doc.getStyle("fatal error"));
                                                doc.insertString(
                                                  doc.getLength(), message.substring(12), doc.getStyle("error"));
                                                currentTextLength = currentTextLength + message.length() - 1;
                                                toFront = true;
                                        } else if (message.startsWith("error :")) {
                                                doc.insertString(doc.getLength(), "error", doc.getStyle("error"));
                                                doc.insertString(
                                                  doc.getLength(), message.substring(6), doc.getStyle("regular"));
                                                currentTextLength = currentTextLength + message.length() - 1;
                                                toFront = true;
                                        } else if (message.startsWith("warning :")) {
                                                doc.insertString(doc.getLength(), "warning", doc.getStyle("warning"));
                                                doc.insertString(
                                                  doc.getLength(), message.substring(8), doc.getStyle("regular"));
                                                currentTextLength = currentTextLength + message.length() - 1;
                                        } else {
                                                doc.insertString(doc.getLength(), message, doc.getStyle("regular"));
                                                currentTextLength = currentTextLength + message.length();
                                        }

                                } catch (BadLocationException e) {
                                }

                                // On met le caret en fin, seulement si c'est le dernier message restant a
                                // afficher
                                if (messageNumber == displayedMessageNumber) {
                                        try {
                                                pane.setCaretPosition(doc.getLength());
                                        } catch (Exception e) {
                                        }
                                }

                                // Affichage
                                if (!isShowing()) {
                                        setInitialLocation();
                                        openedOnce = true;
                                        setVisible(true);
                                }

                                // Si necessaire, on repasse la fenetre au premier plan
                                // Inactivee car trop penible lors de batch qui font reapparaitre la
                                // fenetre sans arret
                                // Reactivee carvraiment necessaire en mode interface, quand on ne sait plus
                                // ou est la fenetre de log
                                boolean toFrontActivated = true;
                                if (toFront && toFrontActivated) {
                                        int currentFrameState = getExtendedState();
                                        if ((currentFrameState & Frame.ICONIFIED) == Frame.ICONIFIED)
                                                setExtendedState(currentFrameState & (~Frame.ICONIFIED));
                                        toFront();
                                }
                        }
                } catch (Exception e) {
                }
        }

        /**
         * Definit une variante de JTextPane avec scroll horizontal
         * https://www.java.net/node/650971
         */
        class NonWrappingTextPane extends JTextPane
        {
                public boolean getScrollableTracksViewportWidth()
                {
                        // On retourne false pour avoir une barre de scroll horizontale sans wrapping
                        // On retourne true pour avoir wrapping sans barre de scroll horizontale
                        // On prend ce dernier choix, qui a l'usage s'avere le plus pertinent
                        return true;
                }

                public void setSize(Dimension d)
                {
                        int parentWidth = getParent().getSize().width;
                        int newWidth = d.width;
                        if (newWidth < parentWidth)
                                newWidth = parentWidth;
                        super.setSize(new Dimension(newWidth, d.height));
                }
        }

        /**
         * Zone de texte permettant l'affichage des messages
         */
        private NonWrappingTextPane pane;

        /**
         * Nombre de messages et de message affiches
         */
        private int messageNumber;
        private int currentTextLength;
        private int displayedMessageNumber;
}
