﻿#pragma once
#include "internal/jip_nvse.h"





DEFINE_CMD_ALT_COND_PLUGIN(GetNumActorsInRangeFromRef,, "Returns the amount of actors that are a certain distance nearby to the calling reference.", 1, kParams_OneFloat_OneInt);
//Code ripped from both JIP (GetActorsByProcessingLevel) and SUP.
UINT32 __fastcall GetNumActorsInRangeFromRefCALL(TESObjectREFR* thisObj, float range, UInt32 flags)
{
	if (range <= 0) return 0;
	if (!thisObj) return 0;
	
	UInt32 numActors = 0;
	bool const noDeadActors = flags & 0x1;
	//bool const something = flags & 0x2;
	
	MobileObject** objArray = g_processManager->objects.data, ** arrEnd = objArray;
	objArray += g_processManager->beginOffsets[0];  //Only objects in High process.
	arrEnd += g_processManager->endOffsets[0];
	Actor* actor;

	if (thisObj->IsActor())  //To avoid redundant (actor != thisObj) checks.
	{
		for (; objArray != arrEnd; objArray++)
		{
			actor = (Actor*)*objArray;
			//Console_Print("Current actor >>>%08x", actor->refID);

			if (actor && actor->IsActor() && actor != thisObj)
			{
				if (noDeadActors && actor->GetDead())
					continue;

				if (GetDistance3D(thisObj, actor) <= range)
					numActors++;
			}
		}
	}
	else if (flags)  //To avoid redundant flag checks.
	{
		for (; objArray != arrEnd; objArray++)
		{
			actor = (Actor*)*objArray;
			//Console_Print("Current actor >>>%08x", actor->refID);

			if (actor && actor->IsActor() && actor != thisObj)
			{
				if (noDeadActors && actor->GetDead())
					continue;

				if (GetDistance3D(thisObj, actor) <= range)
					numActors++;
			}
		}
	}
	else
	{
		for (; objArray != arrEnd; objArray++)
		{
			actor = (Actor*)*objArray;
			if (actor && actor->IsActor())
			{
				if (GetDistance3D(thisObj, actor) <= range)
				{
					numActors++;
				}
			}
		}
	}

	// Player is not included in the looped array, so we need to check for it outside the loop.
	if (thisObj != g_thePlayer)
	{
		actor = (Actor*)g_thePlayer;
		if (noDeadActors)
		{
			if (actor->GetDead())
				return numActors;
		}
		if (GetDistance3D(thisObj, actor) <= range)
		{
			numActors++;
		}
	}

	//Console_Print("Result: %d", numActors);
	return numActors; 
}

bool Cmd_GetNumActorsInRangeFromRef_Eval(COMMAND_ARGS_EVAL)
{
	*result = GetNumActorsInRangeFromRefCALL(thisObj, *(float*)&arg1, (UInt32)arg2);
	return true;
}
bool Cmd_GetNumActorsInRangeFromRef_Execute(COMMAND_ARGS)
{
	float range;
	UINT32 flags;
	if (ExtractArgs(EXTRACT_ARGS, &range, &flags))
		*result = GetNumActorsInRangeFromRefCALL(thisObj, range, flags);
	else
		*result = 0;
	return true;
}


DEFINE_CMD_ALT_COND_PLUGIN(GetNumCombatActorsFromActor, , "Returns the amount of actors that are allies/targets to the calling actor, with optional filters.", 1, kParams_OneFloat_OneInt);
//Code ripped off of JIP's GetCombatActors.
UINT32 __fastcall GetNumCombatActorsFromActorCALL(TESObjectREFR* thisObj, float range, UInt32 flags)
{
	if (!thisObj) return 0;
	if (!thisObj->IsActor()) return 0;
	if (!flags) return 0;
	//Even if the calling actor is dead, they could still have combat targets, so we don't filter that out.
	
	UINT32 numActors = 0;
	bool const getAllies = flags & 0x1;
	bool const getTargets = flags & 0x2;
	
	Actor* actor;
	if (range <= 0)  //ignore distance.
	{
		if (thisObj == g_thePlayer)
		{
			CombatActors* cmbActors = g_thePlayer->combatActors;
			if (!cmbActors) return numActors;
			if (getAllies)
			{
				numActors += cmbActors->allies.size;   //thisObj is its own combat ally, for whatever reason...
				numActors--;                           //So we decrement it by one to get rid of that.
			}
			if (getTargets)
			{
				numActors += cmbActors->targets.size;
			}
		}
		else
		{
			actor = (Actor*)thisObj;
			if (getAllies && actor->combatAllies)
			{
				numActors += actor->combatAllies->size;
			}
			if (getTargets && actor->combatTargets)
			{
				numActors += actor->combatTargets->size;
			}
		}
	}
	else  //---Account for distance.
	{
		UINT32 count;
		
		if (thisObj == g_thePlayer)
		{
			CombatActors* cmbActors = g_thePlayer->combatActors;
			if (!cmbActors) return numActors;
			if (getAllies)
			{
				CombatAlly* allies = cmbActors->allies.data;
				for (count = cmbActors->allies.size; count; count--, allies++)
				{
					actor = allies->ally;
					if (actor && (actor != thisObj))
					{
						if (GetDistance3D(thisObj, actor) <= range)
						{
							numActors++;
						}
					}
				}
			}
			if (getTargets)
			{
				CombatTarget* targets = cmbActors->targets.data;
				for (count = cmbActors->targets.size; count; count--, targets++)
				{
					actor = targets->target;
					if (actor)
					{
						if (GetDistance3D(thisObj, actor) <= range)
						{
							numActors++;
						}
					}
				}
			}
		}
		else
		{
			actor = (Actor*)thisObj;
			Actor** actorsArr = NULL;
			if (getAllies && actor->combatAllies)
			{
				actorsArr = actor->combatAllies->data;
				if (actorsArr)
				{
					count = actor->combatAllies->size;
					for (; count; count--, actorsArr++)   //can I merge these two loops, to be easier to debug?
					{
						actor = *actorsArr;
						if (actor && (actor != thisObj))  //thisObj is its own combat ally, for whatever reason...
						{
							if (GetDistance3D(thisObj, actor) <= range)
							{
								numActors++;
							}
						}
					}
				}
			}
			if (getTargets && actor->combatTargets)  
			{
				actorsArr = actor->combatTargets->data;
				if (actorsArr)
				{
					count = actor->combatTargets->size;
					for (; count; count--, actorsArr++)   //can I merge these two loops, to be easier to debug?
					{
						actor = *actorsArr;
						if (actor)  //thisObj cannot be its own target, no need to check against that.
						{
							if (GetDistance3D(thisObj, actor) <= range)
							{
								numActors++;
							}
						}
					}
				}
			}

		}
	}
	return numActors;
}

bool Cmd_GetNumCombatActorsFromActor_Eval(COMMAND_ARGS_EVAL)
{
	*result = GetNumCombatActorsFromActorCALL(thisObj, *(float*)&arg1, (UInt32)arg2);
	return true;
}
bool Cmd_GetNumCombatActorsFromActor_Execute(COMMAND_ARGS)
{
	float range;
	UINT32 flags;
	if (ExtractArgs(EXTRACT_ARGS, &range, &flags))
		*result = GetNumCombatActorsFromActorCALL(thisObj, range, flags);
	else
		*result = 0;
	return true;
}



#if IFYOULIKEBROKENSHIT



#endif