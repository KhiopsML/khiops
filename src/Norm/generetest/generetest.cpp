// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Sample.h"
#include "SampleView.h"

void SampleTest()
{
	Sample sample;
	SampleView sampleView;

	sampleView.SetObject(&sample);
	sampleView.Open();
	cout << sample << endl;
}

int main(int argc, char** argv)
{
	// MemSetAllocIndexExit(759);

	UIObject::SetUIMode(UIObject::Graphic);
	UIObject::ParseMainParameters(argc, argv);

	SampleTest();
	return 0;
}