// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

typedef void (*MacosGUIThreadFunction)(void*);

// Execute la fonction hors du thread principal macOS;
// lorsque l'appel vient deja d'un thread secondaire, la boucle AppKit reste active pendant son execution.
void MacosRunGUI(MacosGUIThreadFunction function, void* context);

// Appel de la methode Java open() en gerant la boucle AppKit sur le thread principal macOS
void MacosOpenGUIUnit(JNIEnv* env, jobject guiObject, jmethodID mid);

// Positionne JAVA_STARTED_ON_FIRST_THREAD_<pid> pour que l'AWT utilise le thread courant comme EDT
void MacosSetJavaStartedOnFirstThread();
