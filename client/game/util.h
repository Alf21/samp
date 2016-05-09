#pragma once

#define ENTITY_TYPE_UNKNOWN		0
#define ENTITY_TYPE_PED			1
#define ENTITY_TYPE_VEHICLE		2

void ProcessLineOfSight(VECTOR *vecOrigin, VECTOR *vecLine, VECTOR *colPoint,
		DWORD *pHitEntity, int bCheckBuildings, int bCheckVehicles, int bCheckPeds,
		int bCheckObjects, int bCheckDummies, int bSeeThroughStuff,
		int  bIgnoreSomeObjectsForCamera, int bUnk1);

float GetNormalisation(VECTOR *vec);

void CompressSpeedVector(VECTOR * vec, C_VECTOR2 * c2);
void DecompressSpeedVector(VECTOR * vec, C_VECTOR2 * c2);

void CompressNormalVector(VECTOR * vec, C_VECTOR1 * c1);
void DecompressNormalVector(VECTOR * vec, C_VECTOR1 * c1);

float FloatDifference(float f1, float f2);
float FloatOffset(float f1, float f2);

void __stdcall SetRadarColor(int nIndex,DWORD dwColor);

void __stdcall WorldRemoveEntity(DWORD *dwEnt);
void __stdcall WorldAddEntity(DWORD *dwEnt);

int __stdcall GameGetWeaponModelIDFromWeaponID(int iWeaponID);
void __stdcall GameDisableCheatCodes();
PED_TYPE * __stdcall GamePool_Ped_GetAt(int iID);
int __stdcall GamePool_Ped_GetIndex(PED_TYPE *pActor);
VEHICLE_TYPE * __stdcall GamePool_Vehicle_GetAt(int iID);
ENTITY_TYPE * __stdcall GamePool_Object_GetAt(int iID);
DWORD __stdcall GamePool_Vehicle_GetIndex(VEHICLE_TYPE *pVehicle);
PED_TYPE * __stdcall GamePool_FindPlayerPed();
int Game_PedStatPrim(int model_id);
int Game_PedStat(int model_id);

BYTE * __stdcall GetModelInfo(int nModelIndex);
unsigned short __stdcall GetModelReferenceCount(int nModelIndex);

DWORD __stdcall TranslateColorCodeToRGBA(int iCode);
BOOL __stdcall GameIsEntityOnScreen(DWORD * pdwEnt);
void __stdcall InitPlayerPedPtrRecords();
void __stdcall SetPlayerPedPtrRecord(BYTE bytePlayer, DWORD dwPedPtr);
BYTE __stdcall FindPlayerNumFromPedPtr(DWORD dwPedPtr);
void __stdcall GamePrepareTrain(VEHICLE_TYPE *pVehicle);
float DistanceBetweenHorizontalPoints(float x1, float y1, float x2, float y2);
float DistanceBetweenPoints(float x1, float y1, float z1, float x2, float y2, float z2);

void __stdcall _VectorNormalise(VECTOR *vec);
void __stdcall _CrossProduct(VECTOR *vecOut, VECTOR *vec1, VECTOR *vec2);
void __stdcall _Multiply3x3(VECTOR *vecOut, MATRIX4X4 *matin, VECTOR *vecin);
void __stdcall _Multiply3x3(VECTOR *vecOut, VECTOR *vecin, MATRIX4X4 *matin);
void __stdcall __ml(VECTOR *vecOut, MATRIX4X4 *matin, VECTOR *vecin);


BOOL __stdcall IsValidModel(int iModelID);

void GameResetRadarColors();

float DegToRad(float fDegrees);

bool IsNumeric(char *szString);

void GameForcedExit(int iReasonCode);

void SaveCameraRaster(char *filename);

#define FORCE_EXIT(a) GameForcedExit(a); while(true);

//-----------------------------------------------------------

