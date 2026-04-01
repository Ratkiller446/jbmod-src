//========= Copyright Valve Corporation, All rights reserved. ============//

#include "daslang_jbmod_module.h"
#include "cbase.h"
#include "tier0/memdbgon.h"

#include <daScript/daScript.h>
#include <daScript/module/built_in.h>
#include <daScript/module/math.h>
#include <daScript/module/strings.h>
#include <daScript/module/rtti.h>

namespace DaslangJBMod
{
    int32_t AddInts(int32_t a, int32_t b) { return a + b; }
    float GetServerTime() { return 0.0f; }
    int GetPlayerCount() { return 0; }
}

void RegisterDaslangJBModModule(das::ModuleLibrary& lib)
{
    using namespace das;
    
    lib->addExtern<DaslangJBMod::AddInts>("add_ints", SideEffects::none, "Adds two integers")
        ->args("a", "b");
    
    lib->addExtern<DaslangJBMod::GetServerTime>("server_time", SideEffects::none, "Get current server time")
        ->args();
    
    lib->addExtern<DaslangJBMod::GetPlayerCount>("player_count", SideEffects::none, "Get number of players")
        ->args();
}
