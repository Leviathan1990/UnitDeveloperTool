#include "ScriptVocabulary.h"

namespace ScriptVocabulary
{
    const QStringList& callSyntaxFunctions()
    {
        static const QStringList list = {
            // Every entry below is confirmed by actually appearing as a
            // top-level call in real shipped files - the real game install's
            // Scripts/*.cfg + Scripts/Config/*.cfg, and the campaign content
            // under Campaigns/*/*.oms + *.cfg (2026-08-20 pass). This
            // replaced an earlier version of this list that included several
            // Ghidra-registry-adjacent names that turned out to be wrong or
            // console/keybind-only once checked against real content - see
            // project memory (reference_unitdevelopertool.md) for the full
            // story, including exactly which names were removed and why.

            // Emitters.cfg / EmitterMaterials.cfg
            "Emitter_Create",
            "Emitter_AddGradientPoint",
            "Emitter_SetName",
            "Emitter_SetMaterial",
            // Explosions.cfg / ExplosionsSettings.cfg
            "Effect_Explo_Create",
            "Effect_Explo_AddObject",
            "Effect_Explo_AddThrowObject",
            "Effect_Explo_SetScale",
            "Effect_Explo_SetShake",
            "Effect_Explo_SetSound",
            "Effect_Explo_PolyBlowSpeed",
            "Effect_Explo_DebrisExplosion",
            "Effect_Explo_Lock",
            "Effect_Explo_SetPressureWave",
            // Colors.cfg
            "scene_ambinet_red", "scene_ambinet_green", "scene_ambinet_blue",
            "scene_bgcolor_red", "scene_bgcolor_green", "scene_bgcolor_blue",
            "scene_sun_red", "scene_sun_green", "scene_sun_blue",
            "scene_sun_pitch", "scene_sun_heading",
            "Effect_FogColorR", "Effect_FogColorG", "Effect_FogColorB",
            "Effect_FogHalfColorR", "Effect_FogHalfColorG", "Effect_FogHalfColorB",
            "Effect_LightOfGod_Height", "Effect_LightOfGod_Range",
            "Effect_LightOfGod_Strength", "Effect_LightOfGod_Variation",
            "Effect_LightOfGod_ColorR", "Effect_LightOfGod_ColorG", "Effect_LightOfGod_ColorB",
            // Scripts/Config/Effects.cfg (auto-generated effects settings -
            // same call-syntax grammar, confirmed via the real shipped file)
            "Effect_FogOfWar",
            "Effect_LightOfGod",
            "Effect_Particles",
            "Effect_PolygonBlow",
            "effect_polygonblow_amount", // lowercase in the real file, not a typo here
            // Scripts/Config/Sound.cfg (auto-generated audio settings)
            "InterfaceVolume",
            "se_Bits", "se_Channels",
            "se_EffectSoundsActive", "se_EffectVolume",
            "se_InterfaceSoundsActive",
            "se_MixAhead", "se_MixRate",
            "se_NoSound", "se_OutChannels",
            "se_ReloadOnPlay", "se_UseWaveout",
            "sndopt_EffectsActive", "sndopt_EffectsVolume",
            "sndopt_InterfaceSoundsActive", "sndopt_InterfaceVolume", "sndopt_Quality",
            // Scripts/Config/Music.cfg
            "musicopt_Active", "musicopt_Volume",
            // init.oms / campaign.cfg / AI.oms / orders.cfg mission-scripting
            // family. m_SetConditions/m_Condition/m_ListConditions/
            // m_BuildRestrictionMode/m_NoBuildRestriction corrected
            // 2026-08-20: previously listed as uppercase "M_*" (wrong - and
            // two had the wrong tail too), confirmed lowercase "m_*" both by
            // Ghidra (registration function 004c3120) and by real usage in
            // shipped .oms/.cfg content.
            "m_SetConditions",
            "m_Condition",
            "m_ListConditions",
            "m_BuildRestrictionMode",
            "m_NoBuildRestriction",
            "m_SetTimer",
            "M_Resources",
            "M_SetGameSceneRect",
            "StartMP3",
            "StopMP3",
            "se_killlooping",
            "se_playlooping",
            "bind",
            "exec",
            "input_LastFile",
            "RemoveBonds", // yes, "Bonds" not "Binds" - that's the real spelling in the shipped files
            "AI_AddCommander",
            "ai_commandercolor",
            "AI_InitializeCommanders",
            "AddCustomizedControl",
            "AddMoveOrder",
            "AddPatrolOrder",
            "anim_play",
            "g_SetActiveGui",
            "g_SetCampaignBriefingAnimation",
            "g_SetCampaignMissionText",
            "g_SetCampaignObjectiveText",
            "g_SetDirectActiveGUI",
            "GUI_Disable",
            "SetActiveCampaign",
            "StartCampaignMission",
        };
        return list;
    }

    const QStringList& tabularKeys()
    {
        static const QStringList list = {
            "AttackDelay", "FirstSpyDelay", "FirstAttackDelay", "buildDelay", "InstantAttack",
            "attackDistanceIncrement", "DefendRadius",
            "numberInSpyGroups", "numberInDefendGroups", "numberInAttackGroups",
            "wantNumberOfSpyingGroups", "wantNumberOfDefendingGroups", "wantNumberOfAttackingGroups",
            "NrOfGroupsAttackingTogether",
            "minDistanceForGroup", "minDistanceBetweenBuildings",
            "timeBetweenWarpTargetUpdates", "minDistanceToOwnUnitsForWarpNukeToLaunch",
            "forgettingForCollisionGrid",
            "lowEnergy", "lowResources",
            "cheating",
        };
        return list;
    }
}
