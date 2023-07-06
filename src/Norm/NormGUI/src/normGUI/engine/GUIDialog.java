// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Dimension;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import javax.swing.ImageIcon;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JOptionPane;

/**
 * Represente l'element graphique correspondant au style Dialog. Deux style de
 * dialog sont prise en compte: . question: libelle + deux boutons Yes et No ->
 * renvoie un booleen . input: champ de saisie texte + deux boutons Ok et Cancel
 * -> renvoie la valeur saisie, vide si cancel
 * https://docs.oracle.com/javase/tutorial/uiswing/components/dialog.html
 *
 * @author Marc Boulle
 */
public class GUIDialog extends GUIObject
{
        /**
         * Test si on est une fiche avec un seul champ texte de type dialog
         *
         * @param guiUnit L'unite d'interface a tester Ce type de fiche permet d'une
         *                part un parametrage complet d'une Dialogr, d'autre part assure
         *                une compatibilite ascendante par rapport aux anciennes fiches
         *                utilises auparavant de type UIConfirmationCard avec deux
         *                bouton d'acceptation et d'annulation pour les versions de
         *                Khiops jusqu'a V9
         */
        static public boolean isDialogCard(GUIUnit guiUnit)
        {
                boolean ok = true;
                GUIData fieldQuestion = null;
                GUIData fieldQuestionType = null;

                ok = ok && guiUnit.getStyle().equals("QuestionDialog");
                ok = ok && guiUnit instanceof GUICard;
                ok = ok && guiUnit.vectorOfGUIDatas.size() == 2;
                ok = ok && guiUnit.getActionCount() == 3;
                ok = ok && guiUnit.getVisibleActionCount() == 3;
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(0)).getIdentifier().equals("Exit");
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(1)).getIdentifier().equals("Refresh");
                ok = ok && ((GUIAction)guiUnit.vectorOfGUIActions.elementAt(2)).getIdentifier().equals("OK");
                if (ok) {
                        fieldQuestion = (GUIData)(guiUnit.vectorOfGUIDatas.elementAt(0));
                        fieldQuestionType = (GUIData)(guiUnit.vectorOfGUIDatas.elementAt(1));
                }
                ok = ok && fieldQuestion instanceof GUIStringElement;
                ok = ok && fieldQuestion.getIdentifier().equals("Question");
                ok = ok && fieldQuestion.getStyle().equals("FormattedLabel");
                ok = ok && fieldQuestionType instanceof GUIStringElement;
                ok = ok && fieldQuestionType.getIdentifier().equals("QuestionType");
                ok = ok && fieldQuestionType.getStyle().equals("FormattedLabel");
                ok = ok && !fieldQuestionType.getVisible();
                return ok;
        }

        /**
         * Gestion d'une boite de dialogue parametree par un GUIUnit
         *
         * @param guiDialogCard        L'unite d'interface parametrant la boite de
         *                             dialogue
         * @param parentFrame          Frame parent de la boite de dialogue
         * @param lastVisibleLocationX Position X a utiliser (si different de -1) dans
         *                             le cas d'un parentFram nul
         * @param lastVisibleLocationY Position X a utiliser (si different de -1) dans
         *                             le cas d'un parentFram nul
         */
        static public void startDialog(final GUIUnit guiDialogCard,
                                       JFrame parentFrame,
                                       int lastVisibleLocationX,
                                       int lastVisibleLocationY)
        {
                // Acces au champ et a sa valeur
                final GUIElement questionField = (GUIElement)guiDialogCard.vectorOfGUIDatas.elementAt(0);
                final GUIElement questionTypeField = (GUIElement)guiDialogCard.vectorOfGUIDatas.elementAt(1);
                GUIAction actionOK = (GUIAction)guiDialogCard.vectorOfGUIActions.elementAt(2);
                String question = questionField.getValueIn(guiDialogCard).toString();
                String questionType = questionTypeField.getValueIn(guiDialogCard).toString();
                boolean isQuestion = (guiDialogCard.getStyle().equals("QuestionDialog"));
                assert (isQuestion);

                // Recherche des parametres
                int questionTypeParam = JOptionPane.PLAIN_MESSAGE;
                if (questionType.equals("Information"))
                        questionTypeParam = JOptionPane.INFORMATION_MESSAGE;
                if (questionType.equals("Question"))
                        questionTypeParam = JOptionPane.QUESTION_MESSAGE;
                else if (questionType.equals("Warning"))
                        questionTypeParam = JOptionPane.WARNING_MESSAGE;
                else if (questionType.equals("Error"))
                        questionTypeParam = JOptionPane.ERROR_MESSAGE;
                int answerTypeParam = JOptionPane.YES_NO_OPTION;

                // Creation et parametrage d'une boite de dialogue
                final JOptionPane optionPane;
                optionPane = new JOptionPane(question, questionTypeParam, answerTypeParam);
                final JDialog dialog = new JDialog(parentFrame, guiDialogCard.getLabel(), true);
                dialog.setContentPane(optionPane);

                // Pour interdire de sortie autrement que par les boutons: on ne le fait pas par
                // homogeneite par rapport au reste du comportement de l'interface
                boolean defaultCloseForbidden = false;
                if (defaultCloseForbidden)
                        dialog.setDefaultCloseOperation(JDialog.DO_NOTHING_ON_CLOSE);

                // On autorise la sortie autrement que par les boutons, mais on l'interprete
                // comme un "No"
                dialog.addWindowListener(new WindowAdapter() {
                        public void windowClosing(WindowEvent we)
                        {
                                // Mise a vide de la valeur a vide, pour signaler un "No", sans l'enregistrer
                                questionField.setValueIn(guiDialogCard, "");

                                // Enregistrement de la commande de sortie dans le scenartio
                                guiDialogCard.writeOutputUnitActionCommand("Exit");
                        }
                });

                // Pour quitter la boite de dialogue quand on clique sur un des boutons
                optionPane.addPropertyChangeListener(new PropertyChangeListener() {
                        public void propertyChange(PropertyChangeEvent e)
                        {
                                String prop = e.getPropertyName();

                                if (dialog.isVisible() && (e.getSource() == optionPane) &&
                                    (JOptionPane.VALUE_PROPERTY.equals(prop))) {
                                        dialog.setVisible(false);
                                }
                        }
                });

                // Ajustement de la taille de la fenetre pour tenir compte de la longueur du titre
                dialog.pack();
                int minDialogWidth = getMinFramedWidth(guiDialogCard.getLabel());
                Dimension currentPreferredSize = dialog.getPreferredSize();
                if (currentPreferredSize.getWidth() <= minDialogWidth) {
                        dialog.setPreferredSize(new Dimension(minDialogWidth, (int)currentPreferredSize.getHeight()));
                        dialog.pack();
                }

                // Positionnement de la boite de dialogue par rapport a la fenetre parente
                dialog.setLocationRelativeTo(parentFrame);

                // Parametrage specifique dans le cas d'une fenetre parentre nul
                if (parentFrame == null) {
                        // Positionnement si possible en fonction d'un position passee en parametre
                        if (lastVisibleLocationX >= 0 && lastVisibleLocationY >= 0)
                                dialog.setLocation(lastVisibleLocationX - dialog.getWidth() / 2,
                                                   lastVisibleLocationY - dialog.getHeight() / 2);

                        // Parametrage de l'icone
                        ImageIcon frameIcon;
                        if (!getFrameIconPath().equals("")) {
                                try {
                                        frameIcon = getImageIcon(getFrameIconPath());
                                } catch (Exception ex) {
                                        frameIcon = null;
                                }
                                if (frameIcon != null)
                                        dialog.setIconImage(frameIcon.getImage());
                        }
                }

                // Ouverture de la boite de dialogue
                int dialogResult = JOptionPane.YES_OPTION;
                try {
                        dialog.setVisible(true);
                } catch (Exception e) {
                        dialogResult = JOptionPane.YES_OPTION;
                }

                // Traitement de son retour si validation utilisateur
                dialogResult = ((Integer)optionPane.getValue()).intValue();
                // Choix du Yes
                if (dialogResult == JOptionPane.YES_OPTION) {
                        // Enregistrement de la commande de sortie dans le scenartio
                        guiDialogCard.writeOutputUnitActionCommand("OK");
                }
                // Choix du No, ou sortie de la boite (esc)
                else {
                        // Mise a vide de la valeur a vide, pour signaler un "No", sans l'enregistrer
                        questionField.setValueIn(guiDialogCard, "");

                        // Enregistrement de la commande de sortie dans le scenartio
                        guiDialogCard.writeOutputUnitActionCommand("Exit");
                }
        }
}