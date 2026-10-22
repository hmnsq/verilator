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

#அந்தர்பாவ "config_build.h"

#அந்தர்பாவ "V3Ast.h"

#அந்தர்பாவ <deque>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <string>
#அந்தர்பாவ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
பொது:
    எண்ணிக்கை class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    நிலையான std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    நிலையான bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    நிலையான வெறுமை enable(bool flag);  // LCOV_EXCL_LINE
    நிலையான வெறுமை reset();
    நிலையான வெறுமை clearModuleCache();
    நிலையான AstIfaceRefDType* ifaceRefFromVarDType(Ast_முனை_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    நிலையான வெறுமை retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    நிலையான வெறுமை retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    நிலையான வெறுமை tag(AstRefDType* refp, மாறிலி Ast_முனை_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, மாறிலி string& cellPath,
                    மாறிலி string& ownerModName);
    நிலையான முழு resolveCapturedRefs();
    நிலையான வெறுமை verifyNoDeadRefs(மாறிலி std::unordered_set<மாறிலி Ast_முனை*>& liveNodes);

பொது:
    நிலையான bool enabled() { திரும்பு s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    நிலையான மாறிலி VIfaceCaptureTag* captureTag(AstRefDType* refp);
    நிலையான Ast_முனை_Module* findOwnerModule(Ast_முனை* nodep);
    // Find a Typedef by name in a module's top-level statements
    நிலையான AstTypedef* findTypedefInModule(Ast_முனை_Module* modp, மாறிலி string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    நிலையான Ast_முனை_DType* findDTypeInModule(Ast_முனை_Module* modp, மாறிலி string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    நிலையான AstParamTypeDType* findParamTypeInModule(Ast_முனை_Module* modp, மாறிலி string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    நிலையான bool retargetRefToModule(AstRefDType* refp, Ast_முனை_Module* targetModp);
    நிலையான வெறுமை addParamType(AstRefDType* refp, மாறிலி string& cellPath, Ast_முனை_Module* ownerModp,
                             AstParamTypeDType* paramTypep, மாறிலி string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    நிலையான Ast_முனை_Module* followCellPath(Ast_முனை_Module* startModp, மாறிலி string& cellPath);

    நிலையான வெறுமை captureTypedefContext(AstRefDType* refp, மாறிலி எழுத்து* stageLabel, முழு dotPos,
                                      மாறிலி std::string& dotText, VSymEnt* dotSymp,
                                      Ast_முனை_Module* modp,
                                      மாறிலி std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    நிலையான வெறுமை dumpEntries(மாறிலி string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    நிலையான வெறுமை finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
