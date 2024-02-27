// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KDDataPreparationAttributeCreationTask.h"

////////////////////////////////////////////////////////////////////////////////
// Classe generique portant sur la vue d'un classifieur
class KDDataPreparationAttributeCreationTaskView : public UIObjectView
{
public:
	// Constructeur
	// (doit donner un nom au predictor)
	KDDataPreparationAttributeCreationTaskView();
	~KDDataPreparationAttributeCreationTaskView();

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KDDataPreparationAttributeCreationTaskView* KWSpecificPredictorView::Create() const
	//      {
	//          return new KWSpecificPredictorView;
	//      }
	virtual KDDataPreparationAttributeCreationTaskView* Create() const;

	// Nom de la tache de creation d'attribut editee
	const ALString& GetName() const;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes
	// (en castant l'objet en parametre)

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par le predicteur specifique
	void EventRefresh(Object* object) override;

	// Acces au predicteur
	void SetObject(Object* object) override;
	KDDataPreparationAttributeCreationTask* GetAttributeCreationTask();

	// Libelles
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////////
	// Administration des vues sur les tache de creation d'attributs

	// Memorisation global d'une vue sur une tache de creation d'attributs (defaut: NULL)
	// Supprimer l'eventuelle tache precedente
	// Memoire: la tache appartient a l'appele, et devra etre detruit en appelant la methode avec NULL en parametre
	static void SetGlobalCreationTaskView(KDDataPreparationAttributeCreationTaskView* attributeCreationTaskView);

	// Recherche de la tache courante de creation d'attributs (defaut: NULL)
	// Cette tache pourra etre modifiee pour la parametrer
	static KDDataPreparationAttributeCreationTaskView* GetGlobalCreationTaskView();

	////////////////////////////////////////////////////////
	///// Implemenattion
protected:
	// Nom de la tache de creation d'attribut
	ALString sName;

	// Vue globale sur une tache de creation d'attribut
	static KDDataPreparationAttributeCreationTaskView* globalAttributeCreationTaskView;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDPBivariateCrossProductsCreationTaskView
class KDDPBivariateCrossProductsCreationTaskView : public KDDataPreparationAttributeCreationTaskView
{
public:
	// Constructeur
	KDDPBivariateCrossProductsCreationTaskView();
	~KDDPBivariateCrossProductsCreationTaskView();

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par le predicteur specifique
	void EventRefresh(Object* object) override;
};
