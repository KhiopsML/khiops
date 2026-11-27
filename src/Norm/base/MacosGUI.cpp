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
#include <atomic>
#include <pthread.h>
#include <thread>
#include <unistd.h>

namespace
{
// Indicateur partage entre le thread principal et le thread secondaire
std::atomic<bool> bGUIEventLoopRunning(false);

// Activer l'application et sa fenetre sur la file principale apres la mise a jour AppKit.
void MacosActivateWindow(void* context)
{
	id window = (id)context;

	// Ne pas rouvrir une fenetre fermee depuis la reception de la notification.
	if (((BOOL (*)(id, SEL))objc_msgSend)(window, sel_registerName("isVisible")))
	{
		id nsApp = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"),
							   sel_registerName("sharedApplication"));
		((void (*)(id, SEL, long))objc_msgSend)(nsApp, sel_registerName("setActivationPolicy:"), 0L);

		// Preferer l'API macOS 14+, avec repli pour les versions anterieures.
		if (((BOOL (*)(id, SEL, SEL))objc_msgSend)(nsApp, sel_registerName("respondsToSelector:"),
							   sel_registerName("activate")))
			((void (*)(id, SEL))objc_msgSend)(nsApp, sel_registerName("activate"));
		else
			((void (*)(id, SEL, BOOL))objc_msgSend)(nsApp, sel_registerName("activateIgnoringOtherApps:"),
								(BOOL)1);

		// L'activation de l'application ne garantit pas le focus clavier de sa fenetre.
		((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), (id)nil);
	}

	// Liberer la reference conservee avant la planification de cet appel.
	((void (*)(id, SEL))objc_msgSend)(window, sel_registerName("release"));
}
} // namespace
#endif

boolean MacosIsGUIEventLoopRunning()
{
#ifdef __APPLE__
	return bGUIEventLoopRunning.load();
#else
	return false;
#endif
}

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

		id notificationCenter = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSNotificationCenter"),
									sel_registerName("defaultCenter"));
		__block id activationObserver = nil;
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

			// L'initialisation AWT peut annuler l'activation demandee au lancement.
			// Attendre une fenetre visible plutot que sa prise de focus, qui peut justement ne jamais arriver.
			id notificationName = ((id (*)(id, SEL, const char*))objc_msgSend)(
			    (id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
			    "NSWindowDidUpdateNotification");
			activationObserver = ((id (*)(id, SEL, id, id, id, void (^)(id)))objc_msgSend)(
			    notificationCenter, sel_registerName("addObserverForName:object:queue:usingBlock:"),
			    notificationName, nil, nil, ^(id notification) {
			      id window = ((id (*)(id, SEL))objc_msgSend)(notification, sel_registerName("object"));
			      if (not((BOOL (*)(id, SEL))objc_msgSend)(window, sel_registerName("isVisible")))
				      return;

			      // Activation unique: les rafraichissements suivants ne doivent pas reprendre le focus.
			      ((void (*)(id, SEL, id))objc_msgSend)(
				  notificationCenter, sel_registerName("removeObserver:"), activationObserver);
			      activationObserver = nil;

			      // Conserver la fenetre jusqu'a l'appel differe, qui la liberera.
			      ((id (*)(id, SEL))objc_msgSend)(window, sel_registerName("retain"));

			      // Laisser AppKit terminer la mise a jour en cours avant de demander l'activation.
			      dispatch_async_f(dispatch_get_main_queue(), window, MacosActivateWindow);
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
			bGUIEventLoopRunning.store(false);
		}

		// Retirer l'observateur si la boucle se termine avant le premier affichage.
		if (activationObserver != nil)
			((void (*)(id, SEL, id))objc_msgSend)(notificationCenter, sel_registerName("removeObserver:"),
							      activationObserver);
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
