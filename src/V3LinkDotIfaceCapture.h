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

#ಅನ್ತರ್ಭಾವ "config_build.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ಸ್ಥಿರ std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    ಸ್ಥಿರ bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ಸ್ಥಿರ ಶೂನ್ಯ enable(bool flag);  // LCOV_EXCL_LINE
    ಸ್ಥಿರ ಶೂನ್ಯ reset();
    ಸ್ಥಿರ ಶೂನ್ಯ clearModuleCache();
    ಸ್ಥಿರ AstIfaceRefDType* ifaceRefFromVarDType(Ast_ಗಂಟು_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ಸ್ಥಿರ ಶೂನ್ಯ retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ಸ್ಥಿರ ಶೂನ್ಯ retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ಸ್ಥಿರ ಶೂನ್ಯ tag(AstRefDType* refp, ಧ್ರುವ Ast_ಗಂಟು_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, ಧ್ರುವ string& cellPath,
                    ಧ್ರುವ string& ownerModName);
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ resolveCapturedRefs();
    ಸ್ಥಿರ ಶೂನ್ಯ verifyNoDeadRefs(ಧ್ರುವ std::unordered_set<ಧ್ರುವ Ast_ಗಂಟು*>& liveNodes);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ bool enabled() { ಪ್ರತ್ಯಾಗಮ s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ಸ್ಥಿರ ಧ್ರುವ VIfaceCaptureTag* captureTag(AstRefDType* refp);
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* findOwnerModule(Ast_ಗಂಟು* nodep);
    // Find a Typedef by name in a module's top-level statements
    ಸ್ಥಿರ AstTypedef* findTypedefInModule(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* findDTypeInModule(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ಸ್ಥಿರ AstParamTypeDType* findParamTypeInModule(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ಸ್ಥಿರ bool retargetRefToModule(AstRefDType* refp, Ast_ಗಂಟು_Module* targetModp);
    ಸ್ಥಿರ ಶೂನ್ಯ addParamType(AstRefDType* refp, ಧ್ರುವ string& cellPath, Ast_ಗಂಟು_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ಧ್ರುವ string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* followCellPath(Ast_ಗಂಟು_Module* startModp, ಧ್ರುವ string& cellPath);

    ಸ್ಥಿರ ಶೂನ್ಯ captureTypedefContext(AstRefDType* refp, ಧ್ರುವ ವರ್ಣ* stageLabel, ಪೂರ್ಣಾಙ್ಕ dotPos,
                                      ಧ್ರುವ std::string& dotText, VSymEnt* dotSymp,
                                      Ast_ಗಂಟು_Module* modp,
                                      ಧ್ರುವ std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ಸ್ಥಿರ ಶೂನ್ಯ dumpEntries(ಧ್ರುವ string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ಸ್ಥಿರ ಶೂನ್ಯ finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
