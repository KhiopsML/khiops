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

void MacosRunGUI(MacosGUIThreadFunction function, void* context)
{
#ifdef __APPLE__
	// Sur macOS, le thread principal doit traiter les evenements AppKit via [NSApp run]
	// pendant que la fonction est executee sur un thread secondaire.
	if (pthread_main_np())
	{
		// Installer UITaskProgression maintenant que AppKit va demarrer
		if (TaskProgression::GetManager() == NULL and not TaskProgression::IsStarted())
			TaskProgression::SetManager(UITaskProgression::GetManager());

		// Memoriser l'application au premier plan avant que Khiops ne prenne le focus.
		id workspace = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSWorkspace"),
							       sel_registerName("sharedWorkspace"));
		id previousApplication =
		    ((id (*)(id, SEL))objc_msgSend)(workspace, sel_registerName("frontmostApplication"));
		id notificationCenter = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSNotificationCenter"),
									sel_registerName("defaultCenter"));
		__block id activationObserver = nil;
		__block id resignationObserver = nil;
		id nsApp = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"),
							   sel_registerName("sharedApplication"));
		if (nsApp)
		{
			// Forcer l'apparence Aqua pour un theme Light
			id appearanceName = ((id (*)(id, SEL, const char*))objc_msgSend)(
			    (id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
			    "NSAppearanceNameAqua");
			id aquaAppearance = ((id (*)(id, SEL, id))objc_msgSend)(
			    (id)objc_getClass("NSAppearance"), sel_registerName("appearanceNamed:"), appearanceName);
			if (aquaAppearance)
				((void (*)(id, SEL, id))objc_msgSend)(nsApp, sel_registerName("setAppearance:"),
								      aquaAppearance);

			// NSApplicationActivationPolicyRegular = 0: icone Dock, focus normal
			((void (*)(id, SEL, long))objc_msgSend)(nsApp, sel_registerName("setActivationPolicy:"), 0L);

			// Menu principal avec un menu application: AWT y insere les menus Java des fenetres
			if (((id (*)(id, SEL))objc_msgSend)(nsApp, sel_registerName("mainMenu")) == nil)
			{
				id mainMenu = ((id (*)(id, SEL))objc_msgSend)(
				    ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenu"),
								    sel_registerName("alloc")),
				    sel_registerName("init"));
				id appMenu = ((id (*)(id, SEL))objc_msgSend)(
				    ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenu"),
								    sel_registerName("alloc")),
				    sel_registerName("init"));
				id appMenuItem = ((id (*)(id, SEL))objc_msgSend)(
				    ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenuItem"),
								    sel_registerName("alloc")),
				    sel_registerName("init"));
				((void (*)(id, SEL, id))objc_msgSend)(appMenuItem, sel_registerName("setSubmenu:"),
								      appMenu);
				((void (*)(id, SEL, id))objc_msgSend)(mainMenu, sel_registerName("addItem:"),
								      appMenuItem);
				((void (*)(id, SEL, id))objc_msgSend)(nsApp, sel_registerName("setMainMenu:"),
								      mainMenu);
			}

			// Au premier focus, un changement reel d'application debloque les menus macOS avec AWT embarque.
			id notificationName = ((id (*)(id, SEL, const char*))objc_msgSend)(
			    (id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
			    "NSWindowDidBecomeKeyNotification");
			activationObserver = ((id (*)(id, SEL, id, id, id, void (^)(id)))objc_msgSend)(
			    notificationCenter, sel_registerName("addObserverForName:object:queue:usingBlock:"),
			    notificationName, nil, nil, ^(id notification) {
			      ((void (*)(id, SEL, id))objc_msgSend)(
				  notificationCenter, sel_registerName("removeObserver:"), activationObserver);
			      activationObserver = nil;
			      if (previousApplication == nil or
				  ((int (*)(id, SEL))objc_msgSend)(previousApplication,
								   sel_registerName("processIdentifier")) == getpid())
				      return;
			      dispatch_async(dispatch_get_main_queue(), ^{
				// Attendre la desactivation effective avant de rendre le focus a Khiops, sans delai fixe.
				id resignationName = ((id (*)(id, SEL, const char*))objc_msgSend)(
				    (id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
				    "NSApplicationDidResignActiveNotification");
				resignationObserver = ((id (*)(id, SEL, id, id, id, void (^)(id)))objc_msgSend)(
				    notificationCenter, sel_registerName("addObserverForName:object:queue:usingBlock:"),
				    resignationName, nsApp, nil, ^(id resignationNotification) {
				      ((void (*)(id, SEL, id))objc_msgSend)(
					  notificationCenter, sel_registerName("removeObserver:"), resignationObserver);
				      resignationObserver = nil;
				      dispatch_async(dispatch_get_main_queue(), ^{
					((void (*)(id, SEL, BOOL))objc_msgSend)(
					    nsApp, sel_registerName("activateIgnoringOtherApps:"), (BOOL)1);
				      });
				    });

				// NSApplicationActivateIgnoringOtherApps = 2 pour provoquer le changement de focus.
				BOOL bActivated = ((BOOL (*)(id, SEL, unsigned long))objc_msgSend)(
				    previousApplication, sel_registerName("activateWithOptions:"), 2UL);
				if (not bActivated and resignationObserver != nil)
				{
					((void (*)(id, SEL, id))objc_msgSend)(notificationCenter,
									      sel_registerName("removeObserver:"),
									      resignationObserver);
					resignationObserver = nil;
				}
			      });
			    });
		}

		// La fonction est executee sur le thread secondaire pour laisser le thread principal
		// traiter les evenements AppKit; a sa fin, on demande a NSApp de sortir de sa boucle.
		std::thread jniThread(
		    [function, context, nsApp]()
		    {
			    function(context);
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
			((void (*)(id, SEL))objc_msgSend)(nsApp, sel_registerName("run"));
		}

		// Retirer les observateurs restants si la boucle se termine avant les notifications attendues.
		if (activationObserver != nil)
			((void (*)(id, SEL, id))objc_msgSend)(notificationCenter, sel_registerName("removeObserver:"),
							      activationObserver);
		if (resignationObserver != nil)
			((void (*)(id, SEL, id))objc_msgSend)(notificationCenter, sel_registerName("removeObserver:"),
							      resignationObserver);
		jniThread.join();
	}
	else
	{
		function(context);
	}
#endif
}

namespace
{
struct MacosGUIUnitContext
{
	JavaVM* jvm;
	jobject guiObject;
	jmethodID method;
};

void MacosCallGUIUnit(void* context)
{
	MacosGUIUnitContext* guiUnitContext = (MacosGUIUnitContext*)context;
	JNIEnv* env = NULL;
	guiUnitContext->jvm->AttachCurrentThread((void**)&env, NULL);
	env->CallVoidMethod(guiUnitContext->guiObject, guiUnitContext->method);
	env->DeleteGlobalRef(guiUnitContext->guiObject);
	guiUnitContext->jvm->DetachCurrentThread();
}
} // namespace

void MacosOpenGUIUnit(JNIEnv* env, jobject guiObject, jmethodID mid)
{
#ifdef __APPLE__
	if (pthread_main_np())
	{
		JavaVM* jvm = NULL;
		env->GetJavaVM(&jvm);
		assert(jvm != NULL);
		MacosGUIUnitContext context;
		context.jvm = jvm;
		context.guiObject = env->NewGlobalRef(guiObject);
		context.method = mid;
		MacosRunGUI(MacosCallGUIUnit, &context);
	}
	else
		env->CallVoidMethod(guiObject, mid);
#else
	env->CallVoidMethod(guiObject, mid);
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
