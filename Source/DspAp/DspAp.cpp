// Copyright Epic Games, Inc. All Rights Reserved.

#include "DspAp.h"

#include "MetasoundFrontendModuleRegistrationMacros.h"
#include "Modules/ModuleManager.h"

class FDspApGameModule : public FDefaultGameModuleImpl
{
public:

	virtual void StartupModule()
	{
		METASOUND_REGISTER_ITEMS_IN_MODULE
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FDspApGameModule, DspAp, "DspAp" );
 
