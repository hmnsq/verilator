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

#ଅନ୍ତର୍ଭାଵ "config_build.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
ସର୍ବସାଧାରଣ:
    ଗଣନା class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ସ୍ଥିର std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    ସ୍ଥିର bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ସ୍ଥିର ଶୂନ୍ୟ enable(bool flag);  // LCOV_EXCL_LINE
    ସ୍ଥିର ଶୂନ୍ୟ reset();
    ସ୍ଥିର ଶୂନ୍ୟ clearModuleCache();
    ସ୍ଥିର AstIfaceRefDType* ifaceRefFromVarDType(Ast_ଗଣ୍ଠି_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ସ୍ଥିର ଶୂନ୍ୟ retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ସ୍ଥିର ଶୂନ୍ୟ retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ସ୍ଥିର ଶୂନ୍ୟ tag(AstRefDType* refp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, ଅଚଳ string& cellPath,
                    ଅଚଳ string& ownerModName);
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା resolveCapturedRefs();
    ସ୍ଥିର ଶୂନ୍ୟ verifyNoDeadRefs(ଅଚଳ std::unordered_set<ଅଚଳ Ast_ଗଣ୍ଠି*>& liveNodes);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର bool enabled() { ଫେରନ୍ତୁ s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ସ୍ଥିର ଅଚଳ VIfaceCaptureTag* captureTag(AstRefDType* refp);
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Module* findOwnerModule(Ast_ଗଣ୍ଠି* nodep);
    // Find a Typedef by name in a module's top-level statements
    ସ୍ଥିର AstTypedef* findTypedefInModule(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* findDTypeInModule(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ସ୍ଥିର AstParamTypeDType* findParamTypeInModule(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ସ୍ଥିର bool retargetRefToModule(AstRefDType* refp, Ast_ଗଣ୍ଠି_Module* targetModp);
    ସ୍ଥିର ଶୂନ୍ୟ addParamType(AstRefDType* refp, ଅଚଳ string& cellPath, Ast_ଗଣ୍ଠି_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ଅଚଳ string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Module* followCellPath(Ast_ଗଣ୍ଠି_Module* startModp, ଅଚଳ string& cellPath);

    ସ୍ଥିର ଶୂନ୍ୟ captureTypedefContext(AstRefDType* refp, ଅଚଳ ଅକ୍ଷର* stageLabel, ପୂର୍ଣ୍ଣସଂଖ୍ୟା dotPos,
                                      ଅଚଳ std::string& dotText, VSymEnt* dotSymp,
                                      Ast_ଗଣ୍ଠି_Module* modp,
                                      ଅଚଳ std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ସ୍ଥିର ଶୂନ୍ୟ dumpEntries(ଅଚଳ string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ସ୍ଥିର ଶୂନ୍ୟ finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
