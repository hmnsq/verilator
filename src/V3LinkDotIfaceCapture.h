// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Interface typedef capture helper.
//   Records RefDTypes that reach an interface typedef through a cell path, so
//   V3Param clones can retarget them to the correct interface specialization.
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3LINKDOTIFACECAPTURE_H_
#define VERILATOR_V3LINKDOTIFACECAPTURE_H_

#অন্তর্ভাব "config_build.h"

#অন্তর্ভাব "V3Ast.h"

#অন্তর্ভাব <deque>
#অন্তর্ভাব <functional>
#অন্তর্ভাব <string>
#অন্তর্ভাব <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
সার্বজনিক:
    গণনা class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    স্থির std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    স্থির bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    স্থির শূন্য enable(bool flag);  // LCOV_EXCL_LINE
    স্থির শূন্য reset();
    স্থির শূন্য clearModuleCache();
    স্থির AstIfaceRefDType* ifaceRefFromVarDType(Ast_গ্রন্থি_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    স্থির শূন্য retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    স্থির শূন্য retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    স্থির শূন্য tag(AstRefDType* refp, ধ্রুব Ast_গ্রন্থি_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, ধ্রুব string& cellPath,
                    ধ্রুব string& ownerModName);
    স্থির পূর্ণাঙ্ক resolveCapturedRefs();
    স্থির শূন্য verifyNoDeadRefs(ধ্রুব std::unordered_set<ধ্রুব Ast_গ্রন্থি*>& liveNodes);

সার্বজনিক:
    স্থির bool enabled() { প্রত্যাগম s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    স্থির ধ্রুব VIfaceCaptureTag* captureTag(AstRefDType* refp);
    স্থির Ast_গ্রন্থি_Module* findOwnerModule(Ast_গ্রন্থি* nodep);
    // Find a Typedef by name in a module's top-level statements
    স্থির AstTypedef* findTypedefInModule(Ast_গ্রন্থি_Module* modp, ধ্রুব string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    স্থির Ast_গ্রন্থি_DType* findDTypeInModule(Ast_গ্রন্থি_Module* modp, ধ্রুব string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    স্থির AstParamTypeDType* findParamTypeInModule(Ast_গ্রন্থি_Module* modp, ধ্রুব string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    স্থির bool retargetRefToModule(AstRefDType* refp, Ast_গ্রন্থি_Module* targetModp);
    স্থির শূন্য addParamType(AstRefDType* refp, ধ্রুব string& cellPath, Ast_গ্রন্থি_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ধ্রুব string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    স্থির Ast_গ্রন্থি_Module* followCellPath(Ast_গ্রন্থি_Module* startModp, ধ্রুব string& cellPath);

    স্থির শূন্য captureTypedefContext(AstRefDType* refp, ধ্রুব বর্ণ* stageLabel, পূর্ণাঙ্ক dotPos,
                                      ধ্রুব std::string& dotText, VSymEnt* dotSymp,
                                      Ast_গ্রন্থি_Module* modp,
                                      ধ্রুব std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    স্থির শূন্য dumpEntries(ধ্রুব string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    স্থির শূন্য finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
