// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLSharedObject.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"

//////////////////////////////////////////////////////////////////////////
// Classe PLShared_InputBufferedFile
// Variable partagee qui permet de serialiser tous les attributs des InputBufferedFile
// Les attibuts de position ne sont pas serialises
class PLShared_InputBufferedFile : public PLSharedObject
{
public:
	// Constructeur
	PLShared_InputBufferedFile();
	~PLShared_InputBufferedFile();

	// Acces au bufferedFile
	void SetBufferedFile(InputBufferedFile* bf);
	InputBufferedFile* GetBufferedFile();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

//////////////////////////////////////////////////////////////////////////
// Classe PLShared_OutputBufferedFile
// Variable partagee qui permet de serialiser tous les attributs des OutputBufferedFile
//
class PLShared_OutputBufferedFile : public PLSharedObject
{
public:
	// Constructeur
	PLShared_OutputBufferedFile();
	~PLShared_OutputBufferedFile();

	// Acces au bufferedFile
	void SetBufferedFile(OutputBufferedFile* bf);
	OutputBufferedFile* GetBufferedFile();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLShared_InputBufferedFile::SetBufferedFile(InputBufferedFile* bf)
{
	SetObject(bf);
}

inline InputBufferedFile* PLShared_InputBufferedFile::GetBufferedFile()
{
	// Simple cast
	return cast(InputBufferedFile*, GetObject());
}

inline Object* PLShared_InputBufferedFile::Create() const
{
	return new InputBufferedFile;
}

inline void PLShared_OutputBufferedFile::SetBufferedFile(OutputBufferedFile* bf)
{
	SetObject(bf);
}

inline OutputBufferedFile* PLShared_OutputBufferedFile::GetBufferedFile()
{
	// Simple cast
	return cast(OutputBufferedFile*, GetObject());
}

inline Object* PLShared_OutputBufferedFile::Create() const
{
	return new OutputBufferedFile;
}
