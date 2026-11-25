#ifndef SCRIPTVOCABULARY_H
#define SCRIPTVOCABULARY_H

#include <QStringList>

// Ground-truthed vocabulary for the two real Outforce text-script syntaxes,
// used as the single source of truth for BOTH ScriptHighlighter (keyword
// coloring) and ScriptEditorWindow's suggestion box (autocomplete) - see
// project memory (reference_unitdevelopertool.md) for how this was derived.
//
// callSyntaxFunctions() was re-verified directly against Outforce_w2k.exe
// via Ghidra AND against real shipped content (2026-08-20, two passes):
//
// Pass 1 (Ghidra only): the engine's script/console interpreter registers
// every valid command through one of a handful of central registration
// functions, so decompiling those gives exact names/case for a whole
// family at once. This caught a real bug (the .oms condition/build-
// restriction commands were listed as uppercase "M_*", corrected to the
// real lowercase "m_*" - matching a fix already made once before in the
// sibling OutforceMapEditor project, see [[project-overview]]'s build-
// restriction fix) but ALSO produced several wrong inclusions: Ghidra
// registration alone can't tell a command genuinely authored into a
// shipped file apart from one that's only ever a live console/keybind
// target - both are registered exactly the same way.
//
// Pass 2 (real files): cross-checked every candidate against the actual
// game install (Scripts/*.cfg, Scripts/Config/*.cfg) and shipped campaign
// content (Campaigns/*/*.oms, *.cfg, AI.oms, orders.cfg) - only names that
// genuinely appear as a top-level call in one of these are listed below.
// This reversed several Pass-1 guesses: e.g. 13 Emitter_* "navigation"
// commands (Emitter_NextEmitter, Emitter_AddRedValue, etc.) turned out to
// appear ONLY as bind() keybind targets in Commands_EmitterEdit.cfg, never
// as authored content - exactly the "must be issued via the game's built-
// in console" pattern - so they were removed. Conversely, se_killlooping/
// se_playlooping and a whole family of se_*/sndopt_*/musicopt_* audio
// settings (real, just living in Scripts/Config/Sound.cfg and Music.cfg,
// a genre not checked in Pass 1) were added, while se_NextMp3/
// se_Mp3PlayList/se_PlayMp3 (Pass-1 guesses) turned out to appear nowhere
// at all, not even as keybind targets, and were removed.
//
// A comprehensive real-usage scan additionally found ~194 top-level
// command names total across the full game install - most (Video/Network/
// Interface/server/skirmish settings, debug-console camera commands, etc.)
// are deliberately NOT included here since they belong to file genres this
// tool doesn't otherwise touch (client settings, not mission/effects
// content) - seemed inconsistent to expand scope that far as a side effect
// of one vocabulary correction. See project memory for the full list.
namespace ScriptVocabulary
{
    // FunctionName(args...); style - Emitters.cfg, EmitterMaterials.cfg,
    // Explosions.cfg, ExplosionsSettings.cfg, Colors.cfg, and init.oms
    // (same call-syntax family, confirmed via InitOmsReader.cpp and, as of
    // 2026-08-20, directly via Ghidra - see the note above).
    const QStringList& callSyntaxFunctions();

    // Name<TAB>Value<TAB>Variance style, InitParametersN.ai only - the
    // fixed/known key set from ai/AIStructs.h's getKnownInitParamNames().
    // BuildFitnessN.ai/MaxNumberOfUnitsN.ai keys are open-ended unit
    // template names, not a fixed vocabulary, so intentionally not listed
    // here - no fabricated suggestions for those files.
    const QStringList& tabularKeys();
}

#endif // SCRIPTVOCABULARY_H
