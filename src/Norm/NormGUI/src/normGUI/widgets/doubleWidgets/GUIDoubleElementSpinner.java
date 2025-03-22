// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.widgets.doubleWidgets;

import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.event.FocusEvent;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import normGUI.engine.GUIDoubleElement;
import normGUI.engine.GUIUnit;

/**
 * Represente l'element graphique correspondant au style Slider. Affichage d'un
 * entier au sein d'une regle (slider). Les bornes inferieures et superieures de
 * la regles sont determinees a partir des valeurs min et max de l'element.
 * Utilisation de JSlider.
 */
public class GUIDoubleElementSpinner extends GUIDoubleElement
{

        /**
         * Renvoie le composant d'affichage (JSpinner) contenant l'entier
         *
         * @return Le composant d'affichage (JSpinner)
         */
        protected JComponent buildComponent()
        {
                // Creation d'un model d'edition des nombres
                double initValue = getMinValue();
                double min = getMinValue();
                double max = getMaxValue();
                int digitNumber = 0;
                String formatString;
                int i;

                // Parametrage de l'intervalle separant deux valeurs
                if (getParametersAsArray() != null) {
                        String[] params = getParametersAsArray();
                        if (params.length == 1) {
                                try {
                                        digitNumber = Integer.parseInt(params[0]);
                                } catch (Exception ex) {
                                        digitNumber = 0;
                                }
                        }
                }

                // Creation du modele de nombre
                SpinnerNumberModel model = new SpinnerNumberModel(initValue, min, max, Math.pow(10, -digitNumber));

                // Creation du composant
                JSpinner js = new JSpinner(model);
                js.setName(getIdentifier());
                js.setEnabled(getEditable());
                setComponentHelpText(js);
                setComponentPreferredSize(js, 10);

                // Personnalisation de la locale de l'editeur
                if (digitNumber == 0)
                        formatString = "0";
                else {
                        formatString = "0.";
                        for (i = 0; i < digitNumber; i++)
                                formatString += "0";
                }
                JSpinner.NumberEditor editor = new JSpinner.NumberEditor(js, formatString);
                DecimalFormat format = editor.getFormat();
                Locale myLocale = new Locale.Builder().setLanguage("en").setScript("Latn").setRegion("US").build();
                format.setDecimalFormatSymbols(new DecimalFormatSymbols(myLocale));
                js.setEditor(editor);

                // Ajout de focusListener sur le Spinner
                // pour etre sur de capturer la perte de focus en toutes circonstances, celle-ci
                // etant critique
                // pour le declenchement de la mise a jour de la valeur vers le C++
                js.addFocusListener(this);

                return js;
        }

        /**
         * Appelee lorsque le composant charge de l'edition perd le focus, l'edition est
         * stoppee, la methode setValueAt() de GUITableModel est appelee pour sauver les
         * valeurs
         */
        public void focusLost(FocusEvent e)
        {
                if (e.getOppositeComponent() != null && (!e.isTemporary() || !getParentUnit().getActionRunning())) {
                        // Dans le Spinner Java standard, la saisie n'est validee qu'avec le Enter
                        // Ici, on force sa validation des la perte de focus
                        // La gestion des bornes est assuree par le SpinnerNumberModel
                        JSpinner js = (JSpinner)component;

                        // Validation de la valeur
                        try {
                                js.commitEdit();
                        } catch (Exception ex) {
                        }

                        // Appel de l'implementation mere
                        super.focusLost(e);
                }
        }

        /**
         * Renvoie l'element insere dans un tableau, cet element communiquera avec la
         * JTable
         *
         * @return L'element insere dans le tableau
         */
        public CellElement getCellElement()
        {
                return new CellElement() {
                        /**
                         * Renvoie la valeur contenue dans l'editeur Attention : utilisation de l'objet
                         * editorComponent, en prenant sa partie texte (idem pour
                         * getTableCellEditorComponent())
                         *
                         * @return La valeur contenue dans l'editeur
                         */
                        public Object getCellEditorValue()
                        {
                                JSpinner js = (JSpinner)editorComponent;

                                // Validation de la valeur
                                try {
                                        js.commitEdit();
                                } catch (Exception ex) {
                                }
                                return js.getValue();
                        }

                        /**
                         * Renvoie le composant graphique d'edition du tableau, sa valeur doit etre
                         * initialisee avec l'objet o Attention : utilisation de l'objet editorComponent
                         * (idem pour getCellEditorValue())
                         *
                         * @param table      La table qui contient l'editeur
                         * @param o          La valeur a editer
                         * @param isSelected Vrai si la cellule est surlignee
                         * @param row        Ligne de la cellule a editer
                         * @param column     Colonne de la cellule a editer
                         * @return Le composant graphique d'edition
                         */
                        public Component getTableCellEditorComponent(
                          JTable table, Object o, boolean isSelected, int row, int column)
                        {
                                ((JSpinner)editorComponent).setValue(o);
                                return ((JSpinner)editorComponent);
                        }

                        /**
                         * Renvoie le rendu du composant graphique du tableau, sa valeur doit etre
                         * initialisee avec l'objet o Attention : Creation d'un nouvel objet composant
                         * graphique obligatoire
                         *
                         * @param table      La table qui contient la cellule
                         * @param o          La valeur de la cellule
                         * @param isSelected Vrai si la cellule est selectionnee
                         * @param row        Ligne de la cellule
                         * @param column     Colonne de la cellule
                         * @return Le composant graphique
                         */
                        public Component getTableCellRendererComponent(
                          JTable table, Object o, boolean isSelected, boolean hasFocus, int row, int column)
                        {
                                JSpinner spinner = (JSpinner)buildComponent();
                                JTextField textField = ((JSpinner.NumberEditor)spinner.getEditor()).getTextField();

                                // Parametrage des couleurs
                                GUIUnit.initializeTextFieldColors(textField, getEditable(), isSelected, hasFocus);

                                spinner.setValue(o);
                                return spinner;
                        }
                };
        }

        /**
         * Ajoute le composant d'affichage et son libelle dans le panel de l'unite mere
         *
         * @param panel       Panneau de l'unite mere dans lequel sera ajoute le
         *                    composant d'affichage
         * @param constraints Contraintes d'affichage
         */
        public void graphicAddField(JPanel panel, GridBagConstraints constraints)
        {

                // Positionnement du libelle
                JLabel label = new JLabel(getLabel());
                setComponentHelpText(label);
                constraints.weightx = 0;
                constraints.gridwidth = GridBagConstraints.RELATIVE;
                panel.add(label, constraints);

                // Positionnement du spinner
                component = buildComponent();
                ((JSpinner.NumberEditor)((JSpinner)component).getEditor()).getTextField().addFocusListener(this);
                constraints.weightx = 1;
                constraints.gridwidth = GridBagConstraints.REMAINDER;
                panel.add(component, constraints);
        }

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element
         */
        protected String graphicGetValue() { return ((JSpinner)component).getValue().toString(); }

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique de l'element
         */
        public void graphicSetValue(String sValue)
        {
                try {
                        ((JSpinner)component).setValue(Double.parseDouble(sValue));
                } catch (Exception ex) {
                }
        }
}
