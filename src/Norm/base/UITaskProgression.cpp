// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

/////////////////////////////////////////////////////////////////////////////////////
// Les methodes de cette classe sont redirigee sur la classe java GUITaskProgression
// Les methodes sont d'instances pour etre reimplementables dans une sous-classe
// de TaskProgressionManager, mais elles sont ici redirigees vers des methodes
// de classe de GUITaskProgression

void UITaskProgression::Start()
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode open
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "open", "()V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		env->CallVoidMethod(obj, mid);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

boolean UITaskProgression::IsInterruptionRequested()
{
	boolean bValue = false;

	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jboolean value;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode isInterruptionRequested
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "isInterruptionRequested", "()Z");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		value = env->CallBooleanMethod(obj, mid);
		bValue = value;
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
	return bValue;
}

void UITaskProgression::Stop()
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode close
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "close", "()V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		env->CallVoidMethod(obj, mid);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetLevelNumber(int nValue)
{
	require(nValue >= 0);

	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jint levelNumber;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setLevelNumber
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setLevelNumber", "(I)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		levelNumber = (jint)nValue;
		env->CallVoidMethod(obj, mid, levelNumber);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetCurrentLevel(int nValue)
{
	if (nValue >= 0 and UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jint currentLevel;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setCurrentLevel
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setCurrentLevel", "(I)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		currentLevel = (jint)nValue;
		env->CallVoidMethod(obj, mid, currentLevel);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetTitle(const ALString& sValue)
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jstring title;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setTitle
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setTitle", "(Ljava/lang/String;)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		title = UIObject::ToJstring(env, sValue);
		env->CallVoidMethod(obj, mid, title);
		env->DeleteLocalRef(title);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetMainLabel(const ALString& sValue)
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jstring mainLabel;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setMainLabel
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setMainLabel", "(Ljava/lang/String;)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		mainLabel = UIObject::ToJstring(env, sValue);
		env->CallVoidMethod(obj, mid, mainLabel);
		env->DeleteLocalRef(mainLabel);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetLabel(const ALString& sValue)
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jstring label;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setLabel
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setLabel", "(Ljava/lang/String;)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		label = UIObject::ToJstring(env, sValue);
		env->CallVoidMethod(obj, mid, label);
		env->DeleteLocalRef(label);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

void UITaskProgression::SetProgression(int nValue)
{
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		JNIEnv* env;
		jobject obj;
		jclass cls;
		jmethodID mid;
		jint progression;

		// Recherche de l'environnement d'execution Java
		env = UIObject::GraphicGetJNIEnv();

		// Recherche de la classe GUITaskProgression
		cls = UIObject::GraphicGetClassID("normGUI/engine/GUITaskProgression");

		// Recherche de la methode setProgression
		mid = UIObject::GraphicGetMethodID(cls, "GUITaskProgression", "setProgression", "(I)V");

		// On recherche l'objet global GUITaskProgression
		obj = GetGuiTaskProgressionManager();

		// Appel de la methode
		progression = (jint)nValue;
		env->CallVoidMethod(obj, mid, progression);
		env->DeleteLocalRef(obj);
		env->DeleteLocalRef(cls);
	}
}

boolean UITaskProgression::IsInterruptionResponsive() const
{
	return false;
}

jobject UITaskProgression::GetGuiTaskProgressionManager()
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jobject obj;

	require(UIObject::GetUIMode() == UIObject::Graphic);

	// Recherche de l'environnement d'execution Java
	env = UIObject::GraphicGetJNIEnv();

	// Recherche de la classe GUIObject
	cls = UIObject::GraphicGetClassID("normGUI/engine/GUIObject");

	// Recherche de la methode getTaskProgressionManager
	mid = UIObject::GraphicGetStaticMethodID(cls, "GUIObject", "getTaskProgressionManager",
						 "()LnormGUI/engine/GUITaskProgression;");

	// On recherche l'objet global GUITaskProgression
	obj = env->CallStaticObjectMethod(cls, mid);
	env->DeleteLocalRef(cls);
	return obj;
}

static UITaskProgression globalUITaskProgressionManager;

UITaskProgression* UITaskProgression::GetManager()
{
	return &globalUITaskProgressionManager;
}
