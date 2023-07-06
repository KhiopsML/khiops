// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef ISHOWPARAMETERVIEW_H
#define ISHOWPARAMETERVIEW_H

#include "UserInterface.h"
#include "KIInterpretationSpec.h"

#include "KILeverVariablesSpecView.h"

/////////////////////////////////////////////////////////////////////
/// Classe KIHowParameterView : Vue sur le PARAMETRAGE specifique du
/// RENFORCEMENT d'un interpreteur de scores
class KIHowParameterView : public UIObjectView
{
public:
	/// Constructeur
	KIHowParameterView();
	~KIHowParameterView();

	/// Constructeur generique
	KIHowParameterView* Create() const;

	// La methode stocke l'objet passe en parametre, puis appelle EventRefresh
	virtual void SetObject(Object*);

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes

	/// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object);

	/// Mise a jour des valeurs de l'interface par le classifier specifique
	void EventRefresh(Object* object);

	KILeverVariablesSpecView* GetLeverVariablesSpecView();

protected:
	KILeverVariablesSpecView* leverVariablesSpecView;
};

inline KILeverVariablesSpecView* KIHowParameterView::GetLeverVariablesSpecView()
{
	return leverVariablesSpecView;
}

#endif // ISHOWVIEWPARAMETER_H
