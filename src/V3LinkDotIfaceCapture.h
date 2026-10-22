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

#अन्तर्भाव "config_build.h"

#अन्तर्भाव "V3Ast.h"

#अन्तर्भाव <deque>
#अन्तर्भाव <functional>
#अन्तर्भाव <string>
#अन्तर्भाव <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
सार्वजनिक:
    गणना class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    स्थिर std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    स्थिर bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    स्थिर शून्य enable(bool flag);  // LCOV_EXCL_LINE
    स्थिर शून्य reset();
    स्थिर शून्य clearModuleCache();
    स्थिर AstIfaceRefDType* ifaceRefFromVarDType(Ast_गाठ_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    स्थिर शून्य retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    स्थिर शून्य retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    स्थिर शून्य tag(AstRefDType* refp, ध्रुव Ast_गाठ_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, ध्रुव string& cellPath,
                    ध्रुव string& ownerModName);
    स्थिर पूर्णाङ्क resolveCapturedRefs();
    स्थिर शून्य verifyNoDeadRefs(ध्रुव std::unordered_set<ध्रुव Ast_गाठ*>& liveNodes);

सार्वजनिक:
    स्थिर bool enabled() { प्रत्यागम s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    स्थिर ध्रुव VIfaceCaptureTag* captureTag(AstRefDType* refp);
    स्थिर Ast_गाठ_Module* findOwnerModule(Ast_गाठ* nodep);
    // Find a Typedef by name in a module's top-level statements
    स्थिर AstTypedef* findTypedefInModule(Ast_गाठ_Module* modp, ध्रुव string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    स्थिर Ast_गाठ_DType* findDTypeInModule(Ast_गाठ_Module* modp, ध्रुव string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    स्थिर AstParamTypeDType* findParamTypeInModule(Ast_गाठ_Module* modp, ध्रुव string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    स्थिर bool retargetRefToModule(AstRefDType* refp, Ast_गाठ_Module* targetModp);
    स्थिर शून्य addParamType(AstRefDType* refp, ध्रुव string& cellPath, Ast_गाठ_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ध्रुव string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    स्थिर Ast_गाठ_Module* followCellPath(Ast_गाठ_Module* startModp, ध्रुव string& cellPath);

    स्थिर शून्य captureTypedefContext(AstRefDType* refp, ध्रुव वर्ण* stageLabel, पूर्णाङ्क dotPos,
                                      ध्रुव std::string& dotText, VSymEnt* dotSymp,
                                      Ast_गाठ_Module* modp,
                                      ध्रुव std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    स्थिर शून्य dumpEntries(ध्रुव string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    स्थिर शून्य finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
