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

#अगर_अघोषित VERILATOR_V3LINKDOTIFACECAPTURE_H_
#घोषणा VERILATOR_V3LINKDOTIFACECAPTURE_H_

#समावेश "config_build.h"

#समावेश "V3Ast.h"

#समावेश <deque>
#समावेश <functional>
#समावेश <string>
#समावेश <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
खुला:
    क्रमागत class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    अटल std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    अटल bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    अटल व्योम enable(bool flag);  // LCOV_EXCL_LINE
    अटल व्योम reset();
    अटल व्योम clearModuleCache();
    अटल AstIfaceRefDType* ifaceRefFromVarDType(Ast_गाँठ_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    अटल व्योम retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    अटल व्योम retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    अटल व्योम tag(AstRefDType* refp, स्थिर Ast_गाँठ_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, स्थिर string& cellPath,
                    स्थिर string& ownerModName);
    अटल पूर्णांक resolveCapturedRefs();
    अटल व्योम verifyNoDeadRefs(स्थिर std::unordered_set<स्थिर Ast_गाँठ*>& liveNodes);

खुला:
    अटल bool enabled() { वापस s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    अटल स्थिर VIfaceCaptureTag* captureTag(AstRefDType* refp);
    अटल Ast_गाँठ_Module* findOwnerModule(Ast_गाँठ* nodep);
    // Find a Typedef by name in a module's top-level statements
    अटल AstTypedef* findTypedefInModule(Ast_गाँठ_Module* modp, स्थिर string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    अटल Ast_गाँठ_DType* findDTypeInModule(Ast_गाँठ_Module* modp, स्थिर string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    अटल AstParamTypeDType* findParamTypeInModule(Ast_गाँठ_Module* modp, स्थिर string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    अटल bool retargetRefToModule(AstRefDType* refp, Ast_गाँठ_Module* targetModp);
    अटल व्योम addParamType(AstRefDType* refp, स्थिर string& cellPath, Ast_गाँठ_Module* ownerModp,
                             AstParamTypeDType* paramTypep, स्थिर string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    अटल Ast_गाँठ_Module* followCellPath(Ast_गाँठ_Module* startModp, स्थिर string& cellPath);

    अटल व्योम captureTypedefContext(AstRefDType* refp, स्थिर अक्षर* stageLabel, पूर्णांक dotPos,
                                      स्थिर std::string& dotText, VSymEnt* dotSymp,
                                      Ast_गाँठ_Module* modp,
                                      स्थिर std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    अटल व्योम dumpEntries(स्थिर string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    अटल व्योम finalizeIfaceCapture();
};

#पूर्ण_अगर  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
