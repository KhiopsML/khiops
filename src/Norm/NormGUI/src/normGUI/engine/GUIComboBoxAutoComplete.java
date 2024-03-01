// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

package normGUI.engine;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import javax.swing.ComboBoxModel;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JComboBox;
import javax.swing.JTextField;
import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.PlainDocument;

/**
 * Definit une comboBox avec auto completion
 */
public class GUIComboBoxAutoComplete extends JComboBox implements JComboBox.KeySelectionManager
{

        /**
         * Indicateur de temps
         */
        private long lap;

        /**
         * Chaine de caractere a chercher dans la comboBox
         */
        private String searchFor;

        /**
         * Cree une comboBox contenant des reels appartenant a un intervalle
         *
         * @param items Le tableau de reels
         * @param min   La valeur reelle minimale
         * @param max   La valeur reelle maximale
         */
        public GUIComboBoxAutoComplete(Double[] items, final double min, final double max)
        {
                super(new DefaultComboBoxModel(items));
                setEditable(true);
                lap = new java.util.Date().getTime();
                setKeySelectionManager(this);
                JTextField tf;
                if (getEditor() != null) {
                        tf = (JTextField)getEditor().getEditorComponent();
                        if (tf != null) {
                                tf.setDocument(new PlainDocument() {
                                        public void insertString(int offset, String str, AttributeSet a)
                                          throws BadLocationException
                                        {
                                                if (str == null)
                                                        return;
                                                String old = getText(0, getLength());
                                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                                try {
                                                        double d = Double.parseDouble(newStr);
                                                        if (d < min || d > max)
                                                                return;
                                                } catch (Exception e) {
                                                        return;
                                                }
                                                super.insertString(offset, str, a);
                                                if (!isPopupVisible() && str.length() != 0)
                                                        fireActionEvent();
                                        }
                                });
                                addActionListener(new ActionListener() {
                                        public void actionPerformed(ActionEvent evt)
                                        {
                                                JTextField tf = (JTextField)getEditor().getEditorComponent();
                                                String text = tf.getText();
                                                if (text.length() > 0) {
                                                        ComboBoxModel aModel = getModel();
                                                        String current;
                                                        for (int i = 0; i < aModel.getSize(); i++) {
                                                                current = aModel.getElementAt(i).toString();
                                                                if (current.toLowerCase().startsWith(
                                                                      text.toLowerCase())) {
                                                                        tf.setText(current);
                                                                        tf.setSelectionStart(text.length());
                                                                        tf.setSelectionEnd(current.length());
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                });
                        }
                }
        }

        /**
         * Cree une comboBox contenant des chaines de caracteres a longueur limitee
         *
         * @param items     Le tableau de chaines de caracteres
         * @param maxLength La longueur maximale des chaines de caracteres
         */
        public GUIComboBoxAutoComplete(String[] items, final int maxLength)
        {
                super(new DefaultComboBoxModel(items));
                setEditable(true);
                lap = new java.util.Date().getTime();
                setKeySelectionManager(this);
                JTextField tf;
                if (getEditor() != null) {
                        tf = (JTextField)getEditor().getEditorComponent();
                        if (tf != null) {
                                tf.setDocument(new PlainDocument() {
                                        public void insertString(int offset, String str, AttributeSet a)
                                          throws BadLocationException
                                        {
                                                if (str == null)
                                                        return;
                                                String old = getText(0, getLength());
                                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                                if (newStr.length() > maxLength)
                                                        return;
                                                super.insertString(offset, str, a);
                                                if (!isPopupVisible() && str.length() != 0)
                                                        fireActionEvent();
                                        }
                                });
                                addActionListener(new ActionListener() {
                                        public void actionPerformed(ActionEvent evt)
                                        {
                                                JTextField tf = (JTextField)getEditor().getEditorComponent();
                                                String text = tf.getText();
                                                if (text.length() > 0) {
                                                        ComboBoxModel aModel = getModel();
                                                        String current;
                                                        for (int i = 0; i < aModel.getSize(); i++) {
                                                                current = aModel.getElementAt(i).toString();
                                                                if (current.toLowerCase().startsWith(
                                                                      text.toLowerCase())) {
                                                                        tf.setText(current);
                                                                        tf.setSelectionStart(text.length());
                                                                        tf.setSelectionEnd(current.length());
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                });
                        }
                }
        }

        /**
         * Cree une comboBox contenant des entiers appartenant a un intervalle
         *
         * @param items Le tableau d'entiers sous formes chaines de caracteres
         * @param min   La valeur entiere minimale
         * @param max   La valeur entiere maximale
         */
        public GUIComboBoxAutoComplete(String[] items, final int min, final int max)
        {
                super(new DefaultComboBoxModel(items));
                setEditable(true);
                lap = new java.util.Date().getTime();
                setKeySelectionManager(this);
                JTextField tf;
                if (getEditor() != null) {
                        tf = (JTextField)getEditor().getEditorComponent();
                        if (tf != null) {
                                tf.setDocument(new PlainDocument() {
                                        public void insertString(int offset, String str, AttributeSet a)
                                          throws BadLocationException
                                        {
                                                if (str == null)
                                                        return;
                                                String old = getText(0, getLength());
                                                String newStr = old.substring(0, offset) + str + old.substring(offset);
                                                try {
                                                        int i = Integer.parseInt(newStr);
                                                        if (i < min || i > max)
                                                                return;
                                                } catch (Exception e) {
                                                        return;
                                                }
                                                super.insertString(offset, str, a);
                                                if (!isPopupVisible() && str.length() != 0)
                                                        fireActionEvent();
                                        }
                                });
                                addActionListener(new ActionListener() {
                                        public void actionPerformed(ActionEvent evt)
                                        {
                                                JTextField tf = (JTextField)getEditor().getEditorComponent();
                                                String text = tf.getText();
                                                if (text.length() > 0) {
                                                        ComboBoxModel aModel = getModel();
                                                        String current;
                                                        for (int i = 0; i < aModel.getSize(); i++) {
                                                                current = aModel.getElementAt(i).toString();
                                                                if (current.toLowerCase().startsWith(
                                                                      text.toLowerCase())) {
                                                                        tf.setText(current);
                                                                        tf.setSelectionStart(text.length());
                                                                        tf.setSelectionEnd(current.length());
                                                                        break;
                                                                }
                                                        }
                                                }
                                        }
                                });
                        }
                }
        }

        /**
         * Renvoie la ligne qui devra etre selectionnee
         *
         * @param aKey   La cle
         * @param aModel Le modele de la comboBox
         */
        public int selectionForKey(char aKey, ComboBoxModel aModel)
        {
                long now = new java.util.Date().getTime();
                if (searchFor != null && aKey == KeyEvent.VK_BACK_SPACE && searchFor.length() > 0)
                        searchFor = searchFor.substring(0, searchFor.length() - 1);
                else {
                        if (lap + 1000 < now)
                                searchFor = "" + aKey;
                        else
                                searchFor = searchFor + aKey;
                }
                lap = now;
                String current;
                for (int i = 0; i < aModel.getSize(); i++) {
                        current = aModel.getElementAt(i).toString().toLowerCase();
                        if (current.toLowerCase().startsWith(searchFor.toLowerCase()))
                                return i;
                }
                return -1;
        }
}
