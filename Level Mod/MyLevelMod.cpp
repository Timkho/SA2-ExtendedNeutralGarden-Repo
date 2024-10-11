/**
 * MyLevelMod.cpp
 * 
 * Description:
 *    The main execution script for My Level Mod, a simple way of importing
 *    levels into Sonic Adventure 2 without having to write code. But for you
 *    creative few, this is where you can add in your own code! Go crazy! Just
 *    try not to break anything :)
 * 
 *    X-Hax discord for code questions: https://discord.gg/gqJCF47
 */


#include "pch.h"
#include "IniReader.h"
#include "LevelImporter.h"
#include "SetupHelpers.h"

DataPointer(Float, flt_1657CEC, 0x1657CEC);
LevelImporter* myLevelMod;

Float OGCamHeight;

//that's for chao water detection
float waterHeight = -1000.2f;
Trampoline* Chao_DetectWater_t = nullptr;

static const void* const RunChaoBehaviourPtr = (void*)0x53D890;
static inline void RunChaoBehaviour(ObjectMaster* obj, void* func, int idk)
{
    __asm
    {
        push[idk]
        push[func]
        mov eax, [obj]
        call RunChaoBehaviourPtr
        add esp, 8
    }
}

float GetWaterHeight(ChaoData1* chaodata1) {
    NJS_VECTOR pos = { chaodata1->entity.Position.x, chaodata1->entity.Position.y - 1.0f, chaodata1->entity.Position.z };
    CharSurfaceInfo surfaceinfo;
    //CharSurfaceInfo surfaceinfo2;

    ListGroundForCollision(pos.x, pos.y, pos.z, 200.0f);
    GetCharacterSurfaceInfo(&pos, &surfaceinfo);

    if (surfaceinfo.TopSurface & SurfaceFlag_NoFog)
    {
        return surfaceinfo.TopSurfaceDist;
    }
    else
    {
        return -10000000.0f;
    }

    //if (surfaceinfo2.TopSurface & SurfaceFlag_NoShadow)
    //{
    //    return surfaceinfo2.TopSurfaceDist;
    //}
}

signed int __cdecl Chao_DetectWater_r(ObjectMaster* obj) {

    ChaoData1* data1 = (ChaoData1*)obj->Data1.Chao;

    //-2 i -12

    boolean WATERDETECTSPOTTED_CHAODROWNGO;

    if (CurrentLevel == LevelIDs_ChaoWorld && CurrentChaoArea != 3)
    {
        return ((decltype(Chao_DetectWater_r)*)Chao_DetectWater_t->Target())(obj);
    }
    else
    {
        ChaoData1* data = obj->Data1.Chao;
        ChaoData2* data2 = (ChaoData2*)obj->EntityData2;

        if (data->entity.Status >= 0)
        {
            if (data->entity.Position.y >= -50)
            {
                data2->WaterHeight = -1;
            }
            else
            {
                data2->WaterHeight = -111;
            }

            //if (data->entity.Position.y + 2.0f >= data2->WaterHeight && data2->WaterHeight <= data->entity.Position.y - 12.0f)
            //WaterHeight              -2 start
            //ChaoData.y
            //WaterHeight-10           -12 finish
            if (data->entity.Position.y >= data2->WaterHeight || data->entity.Position.y < data2->WaterHeight - 20.0f)
            {
                WATERDETECTSPOTTED_CHAODROWNGO = false;
            }
            else
            {
                WATERDETECTSPOTTED_CHAODROWNGO = true;
            }

            if(WATERDETECTSPOTTED_CHAODROWNGO != true)
            {
                data->ChaoBehaviourInfo.CurrentActionInfo.field_0 &= 0xFFFAu;
                return 0;
            }
            else
            {
                if (!(data->ChaoBehaviourInfo.CurrentActionInfo.field_0 & 1))
                {
                    data->ChaoBehaviourInfo.CurrentActionInfo.field_0 = data->ChaoBehaviourInfo.CurrentActionInfo.field_0 | 1;
                    Play3DSound_Pos(0x1020, &obj->Data1.Entity->Position, 0, 0, 0);
                    RunChaoBehaviour(obj, (void*)0x562330, -1);
                }

                if (data2->float4 < 0.0)
                {
                    data2->float4 = data2->float4 * 0.1000000014901161;
                }

                data->ChaoBehaviourInfo.CurrentActionInfo.field_0 |= 4u;

                return 1;
            }
        }
        else
        {
            data->ChaoBehaviourInfo.CurrentActionInfo.field_0 &= 0xFFFEu;
            return 0;
        }
    }
}


void init_WaterHack()
{
    //Chao detects water with position Y instead of collision, this makes them swim in the wrong place;
    //this prevent that pos Y check to work.

    WriteData((float**)0x56168d, &waterHeight);
    Chao_DetectWater_t = new Trampoline(0x561630, 0x561635, Chao_DetectWater_r);
    return;
}

void Chao_OOBLimit_r()
{
	float* playerPosY; // ecx

	if (CurrentChaoArea == 3)
	{
		flt_1657CEC = -750;
		playerPosY = &MainCharObj1[0]->Position.y;
		if (MainCharObj1[0]->Position.y > 450.0)
		{
			PrintDebug("bro hit the +Y");
			*playerPosY = 450.0;
		}
		if (flt_1657CEC > (double)*playerPosY)
		{
			PrintDebug("bro hit the -Y");
			*playerPosY = 0.5;
		}
	}
	else
	{
		flt_1657CEC = OGCamHeight;
	}
}

extern "C" {
	// Runs a single time once the game starts up. Required for My Level Mod.
	__declspec(dllexport) void Init(
			const char* modFolderPath,
			const HelperFunctions& helperFunctions) {
		myLevelMod = new LevelImporter(modFolderPath, helperFunctions);
		myLevelModInit(modFolderPath, myLevelMod);

		myLevelMod->importLevel("objLandTableDark");

		OGCamHeight = flt_1657CEC;
		PrintDebug("This shit works.");
		WriteJump((void*)0x52B200, Chao_OOBLimit_r);

		init_WaterHack();
        //initNewWayPoints();
	}
	
	// Runs for every frame while the game is on. Required for My Level Mod.
    __declspec(dllexport) void __cdecl OnFrame() {
		myLevelMod->onFrame();
	}

	// Runs when the game closes. Required for My Level Mod.
	__declspec(dllexport) void __cdecl OnExit() {
		myLevelMod->free();
	}

	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer };
}

// Required.
// A Function 'Hook,' that automatically runs code whenever a the game has
// loaded a level.
void onLevelLoad();
FunctionHook<void> loadLevelHook(InitCurrentLevelAndScreenCount, onLevelLoad);
void onLevelLoad() {
	myLevelMod->onLevelLoad();
	loadLevelHook.Original();
}



// Have to put this for my job.
// Don't worry! You're free to edit and do whatever you like :)
/*************************************************************************
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *************************************************************************/
