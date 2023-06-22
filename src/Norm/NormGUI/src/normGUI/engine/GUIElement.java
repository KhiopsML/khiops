// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.Component;
import java.awt.KeyboardFocusManager;
import java.awt.Window;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.event.MouseEvent;
import java.util.EventObject;
import javax.swing.AbstractCellEditor;
import javax.swing.JComponent;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.table.TableCellEditor;
import javax.swing.table.TableCellRenderer;

/**
 * Definit un element d'interface. Un element d'interface est un composant d'une
 * unite (fiche ou liste)
 *
 * @author Marc Boulle
 */
public abstract class GUIElement extends GUIData implements FocusListener
{

        /**
         * Caractere de declenchement d'un refresh d'une donnee suite a modification
         */
        private boolean bTriggerRefresh;

        /**
         * Renvoie le parametre de declenchement d'un refresh suite a toute modification
         * de la donnee
         */
        public boolean getTriggerRefresh() { return bTriggerRefresh; }

        /**
         * Definit un element dans un tableau, son editeur et son rendu
         *
         * @author Marc Boulle
         */
        protected abstract class CellElement
          extends AbstractCellEditor implements TableCellEditor, TableCellRenderer, FocusListener
        {

                /**
                 * Composant utilise pour l'edition, utiliser cette meme instance dans les
                 * methodes getCellEditor() et getTableCellEditorComponent()
                 */
                public JComponent editorComponent;

                /**
                 * Cree l'element du tableau, instancie l'editeur et positionne l'ecouteur de
                 * focus
                 */
                public CellElement()
                {
                        super();
                        editorComponent = buildComponent();
                        setListeners();
                }

                /**
                 * Appelee lorsque le composant charge de l'edition gagne le focus
                 */
                public void focusGained(FocusEvent e) {}

                /**
                 * Appelee lorsque le composant charge de l'edition perd le focus, l'edition est
                 * stoppee, la methode setValueAt() de GUITableModel est appelee pour sauver les
                 * valeurs
                 */
                public void focusLost(FocusEvent e)
                {
                        if (e.getOppositeComponent() != null &&
                            (!e.isTemporary() || !getParentUnit().getActionRunning()))
                                fireEditingStopped();
                }

                /**
                 * Demande a l'editeur s'il peut se mettre en mode edition, un double click
                 * permettra de passer en mode edition
                 */
                public boolean isCellEditable(EventObject anEvent)
                {
                        if (anEvent instanceof MouseEvent) {
                                return ((MouseEvent)anEvent).getClickCount() >= 1;
                        }
                        return true;
                }

                /**
                 * Positionne l'ecouteur de focus, il doit etre positionne le composant
                 * permettant l'edition, a redefinir si editorComponent possede un sous
                 * composant d'edition (widgets EditableComboBox)
                 */
                public void setListeners() { editorComponent.addFocusListener(this); }
        }

        /**
         * Composant graphique associe a l'element
         */
        public JComponent component;

        /**
         * Valeur de l'element, utilise pour la gestion du focus
         */
        protected String value;

        /**
         * Construit la barre de menus
         */
        protected void buildMenuBar() {}

        /**
         * Construit le menu qui regroupe les menus associes aux sous unites
         */
        public void buildSubMenu() {}

        /**
         * Appelee lorsque l'element gagne le focus
         *
         * @param e L'evenement de focus
         */
        public void focusGained(FocusEvent e)
        {
                if (e.getOppositeComponent() != null && (!e.isTemporary() || !getParentUnit().getActionRunning())) {
                        value = graphicGetValue();
                        selectValue();
                }
        }

        /**
         * Appelee lorsque l'element perd le focus
         *
         * @param e L'evenement de focus
         */
        public void focusLost(FocusEvent e)
        {
                // On ne traite que les pertes de focus permanentes, pour eviter
                // les appel lie a la gestion de l'ensemble des fenetre Windows
                // Cela evite des updateElement inutiles, qui de plus perturbent la
                // synchronisation entre Java et C++
                if (e.getOppositeComponent() != null && (!e.isTemporary() || !getParentUnit().getActionRunning())) {
                        // On ne traite pas les pertes de focus d'une fenetre inactive avec action en
                        // cours
                        // (crash sinon, pour cause potentielle de thread d'execution different))
                        if (!getParentUnit().getParentRoot().getActionRunning())
                                updateElement();
                }
        }

        /**
         * Mise a jour, si changement de valeur
         */
        protected void updateElement()
        {
                // On supprime les blancs de debut et fin du champs
                graphicSetValue(graphicGetValue().trim());

                // Mise a jour si changement de la valeur
                String refValue = getValueIn(getParentUnit()).toString();
                if (!graphicGetValue().equals(refValue)) {
                        // On met a jour la valeur c++ de l'element et on ecrit le scenario
                        recordUpdateElement(getParentUnit(), graphicGetValue());
                }
        }

        /**
         * Mise a jour effective d'une valeur dans une unite d'interface, avec
         * enregistrement de la commande d'update
         */
        protected void recordUpdateElement(GUIUnit guiUnit, String newValue)
        {
                // On recupere la fenetre active, et passage en inactif
                KeyboardFocusManager focusManager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
                Window window = focusManager.getActiveWindow();
                window.setEnabled(false);

                // Modification de la valeur
                setValueIn(guiUnit, newValue);
                guiUnit.writeOutputUnitFieldCommand(getIdentifier(), newValue);

                // On execute une action de Refresh
                if (getTriggerRefresh()) {
                        try {
                                guiUnit.executeUserActionAt("Refresh");
                        } catch (Exception ex) {
                        }

                        // On rafraichit les composants graphiques de l'unite avec les donnees logiques
                        window.setIgnoreRepaint(true);
                        try {
                                guiUnit.getParentRoot().graphicRefreshAll();
                        } catch (Exception ex) {
                        }
                        try {
                                guiUnit.getParentRoot().graphicFireDataChange();
                        } catch (Exception ex) {
                        }
                        window.setIgnoreRepaint(false);
                }

                // On repasse la fenetre en mode actif
                window.setEnabled(true);
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
                         * editorComponent (idem pour getTableCellEditorComponent())
                         *
                         * @return La valeur contenue dans l'editeur
                         */
                        public Object getCellEditorValue()
                        {
                                return ((JTextField)editorComponent).getText();
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
                                ((JTextField)editorComponent).setText(o.toString());
                                return ((JTextField)editorComponent);
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
                                JTextField textField = (JTextField)getDefaultComponent();

                                // Parametrage des couleurs
                                GUIUnit.initializeTextFieldColors(textField, getEditable(), isSelected, hasFocus);

                                textField.setText(o.toString());
                                return textField;
                        }
                };
        }

        /**
         * Renvoie le composant d'affichage de l'element
         *
         * @return Le composant d'affichage de l'element
         */
        protected abstract JComponent buildComponent();

        public JComponent getComponent() { return component; }

        /**
         * Renvoie le composant d'affichage par defaut. Ce composant est utilise par les
         * widgets par defaut
         *
         * @return Le composant d'affichage par defaut
         */
        protected abstract JComponent getDefaultComponent();

        /**
         * Renvoie la valeur c++ de l'element
         *
         * @param unit L'unite contenant l'element
         * @return La valeur c++ de l'element sou la forme de chaine de caracteres
         */
        public abstract Object getValueIn(GUIUnit unit);

        /**
         * Renvoie la valeur graphique de l'element
         *
         * @return La valeur graphique de l'element sous la forme de chaine de
         *         caracteres
         */
        protected abstract String graphicGetValue();

        /**
         * Modifie la valeur graphique de l'element
         *
         * @param sValue La nouvelle valeur graphique
         */
        public abstract void graphicSetValue(String sValue);

        /**
         * Selectionne la valeur associee a l'element
         */
        protected void selectValue() {}

        /**
         * Modifie la valeur de l'element
         *
         * @param value La nouvelle valeur de l'element
         */
        protected void setValue(String value) { this.value = value; }

        /**
         * Modifie la valeur c++ de l'element
         *
         * @param unit  L'unite contenant l'element
         * @param value La nouvelle valeur de l'element
         */
        protected abstract void setValueIn(GUIUnit unit, Object value);

        /**
         * Modifie le parametre de declenchement d'un refresh suite a toute modification
         * de la donnee
         */
        public void setTriggerRefresh(boolean bValue) { bTriggerRefresh = bValue; }
}