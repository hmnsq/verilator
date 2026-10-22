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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LINKDOTIFACECAPTURE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LINKDOTIFACECAPTURE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIface_Capture_ಟ್ಯಾಗ್ final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ class ಬಗೆ_2 : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    ಬಗೆ_2 m_ಬಗೆ;  // What the reference refers to
    string m_cell_ಮಾರ್ಗ;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_captured_ಒಳ_Name;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ಸ್ಥಿರ std::deque<VIface_Capture_ಟ್ಯಾಗ್> s_ಟ್ಯಾಗುಗಳು;  // Owns every tag; a deque keeps addresses stable
    ಸ್ಥಿರ bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ಸ್ಥಿರ ಶೂನ್ಯ enable(bool flag);  // LCOV_EXCL_LINE
    ಸ್ಥಿರ ಶೂನ್ಯ reset();
    ಸ್ಥಿರ ಶೂನ್ಯ clearModuleCache();
    ಸ್ಥಿರ AstIfaceRefDType* ifaceRefFromVarDType(Ast_ಗಂಟು_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ಸ್ಥಿರ ಶೂನ್ಯ retarget_Ref_ಗಮ್ಯ_Typedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ಸ್ಥಿರ ಶೂನ್ಯ retarget_Ref_ಗಮ್ಯ_Param_Type(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ಸ್ಥಿರ ಶೂನ್ಯ ಟ್ಯಾಗ್(AstRefDType* refp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* capturedInp,
                    VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2 ಬಗೆ, ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ,
                    ಸ್ಥಿರಾಂಕ string& ownerModName);
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ resolveCapturedRefs();
    ಸ್ಥಿರ ಶೂನ್ಯ verifyNoDeadRefs(ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*>& ಜೀವಂತ_Nodes_2);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ bool enabled() { ಹಿಂತಿರುಗಿ s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* capture_ಟ್ಯಾಗ್(AstRefDType* refp);
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Owner_Module(Ast_ಗಂಟು* nodep);
    // Find a Typedef by name in a module's top-level statements
    ಸ್ಥಿರ AstTypedef* ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_DType_ಒಳ_Module(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ಸ್ಥಿರ AstParamTypeDType* ಕಂಡುಹಿಡಿ_Param_Type_ಒಳ_Module(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ಸ್ಥಿರ bool retarget_Ref_ಗಮ್ಯ_Module(AstRefDType* refp, Ast_ಗಂಟು_Module* ಗುರಿ_Modp);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Param_Type(AstRefDType* refp, ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ, Ast_ಗಂಟು_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ಸ್ಥಿರಾಂಕ string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ಸ್ಥಿರ Ast_ಗಂಟು_Module* follow_Cell_ಮಾರ್ಗ(Ast_ಗಂಟು_Module* ಆರಂಭ_Modp, ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ);

    ಸ್ಥಿರ ಶೂನ್ಯ captureTypedefContext(AstRefDType* refp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* stageLabel, ಪೂರ್ಣಾಂಕ dotPos,
                                      ಸ್ಥಿರಾಂಕ std::string& dot_ಪಠ್ಯ, VSymEnt* dotSymp,
                                      Ast_ಗಂಟು_Module* modp,
                                      ಸ್ಥಿರಾಂಕ std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ಸ್ಥಿರ ಶೂನ್ಯ dumpEntries(ಸ್ಥಿರಾಂಕ string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ಸ್ಥಿರ ಶೂನ್ಯ finalizeIfaceCapture();
};

#ಆದರೆ_ಅಂತ್ಯ  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
