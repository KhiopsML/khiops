// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "MacosGUI.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#endif

void MacosOpenGUIUnit(JNIEnv* env, jobject guiObject, jmethodID mid)
{
#ifdef __APPLE__
	// Sur macOS, le thread principal doit traiter les evenements AppKit via
	// [NSApp run]. On cree jniThread pour appeler Java open() pendant que
	// le thread principal tourne dans la boucle AppKit.
	// openGUI est appele directement depuis jniThread (hors AWT EDT) pour
	// que l'EDT reste libre lors des callbacks AppKit natifs.
	if (pthread_main_np())
	{
		// Installer UITaskProgression maintenant que AppKit va demarrer
		if (TaskProgression::GetManager() == NULL and not TaskProgression::IsStarted())
			TaskProgression::SetManager(UITaskProgression::GetManager());

		JavaVM* jvm = NULL;
		env->GetJavaVM(&jvm);
		assert(jvm != NULL);
		jobject globalGuiObject = env->NewGlobalRef(guiObject);

		id nsApp = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"),
							   sel_registerName("sharedApplication"));
		if (nsApp)
		{
			// NSApplicationActivationPolicyRegular = 0: icone Dock, focus normal
			((void (*)(id, SEL, long))objc_msgSend)(nsApp, sel_registerName("setActivationPolicy:"), 0L);
		}

		std::thread jniThread(
		    [jvm, globalGuiObject, mid, nsApp]()
		    {
			    JNIEnv* threadEnv = NULL;
			    jvm->AttachCurrentThread((void**)&threadEnv, NULL);
			    threadEnv->CallVoidMethod(globalGuiObject, mid);
			    threadEnv->DeleteGlobalRef(globalGuiObject);
			    jvm->DetachCurrentThread();
			    if (nsApp)
				    dispatch_async(dispatch_get_main_queue(), ^{
				      ((void (*)(id, SEL, id))objc_msgSend)(nsApp, sel_registerName("stop:"), (id)nil);
				    });
		    });

		if (nsApp)
		{
			// Completer la sequence de lancement pour que l'activation soit reconnue par macOS
			((void (*)(id, SEL))objc_msgSend)(nsApp, sel_registerName("finishLaunching"));
			((void (*)(id, SEL, BOOL))objc_msgSend)(nsApp, sel_registerName("activateIgnoringOtherApps:"),
								(BOOL)1);
			// L'init AWT reinitialise l'activation pendant la creation des peers natifs; re-activer apres
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 300 * NSEC_PER_MSEC), dispatch_get_main_queue(),
				       ^{
					 ((void (*)(id, SEL, BOOL))objc_msgSend)(
					     nsApp, sel_registerName("activateIgnoringOtherApps:"), (BOOL)1);
				       });
			((void (*)(id, SEL))objc_msgSend)(nsApp, sel_registerName("run"));
		}
		jniThread.join();
	}
	else
	{
		// Thread worker deja attache a la JVM: appel direct
		env->CallVoidMethod(guiObject, mid);
	}
#endif
}

void MacosSetJavaStartedOnFirstThread()
{
#ifdef __APPLE__
	char sEnvVarName[64];
	snprintf(sEnvVarName, sizeof(sEnvVarName), "JAVA_STARTED_ON_FIRST_THREAD_%d", (int)getpid());
	setenv(sEnvVarName, "1", 1);
#endif
}
