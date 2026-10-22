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

#شامل "config_build.h"

#شامل "V3Ast.h"

#شامل <deque>
#شامل <functional>
#شامل <string>
#شامل <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
عمومی:
    شمارش class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ایستا std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    ایستا bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ایستا تهی enable(bool flag);  // LCOV_EXCL_LINE
    ایستا تهی reset();
    ایستا تهی clearModuleCache();
    ایستا AstIfaceRefDType* ifaceRefFromVarDType(Ast_گره_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ایستا تهی retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ایستا تهی retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ایستا تهی tag(AstRefDType* refp, ثابت Ast_گره_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, ثابت string& cellPath,
                    ثابت string& ownerModName);
    ایستا صحیح resolveCapturedRefs();
    ایستا تهی verifyNoDeadRefs(ثابت std::unordered_set<ثابت Ast_گره*>& liveNodes);

عمومی:
    ایستا bool enabled() { بازگشت s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ایستا ثابت VIfaceCaptureTag* captureTag(AstRefDType* refp);
    ایستا Ast_گره_Module* findOwnerModule(Ast_گره* nodep);
    // Find a Typedef by name in a module's top-level statements
    ایستا AstTypedef* findTypedefInModule(Ast_گره_Module* modp, ثابت string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ایستا Ast_گره_DType* findDTypeInModule(Ast_گره_Module* modp, ثابت string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ایستا AstParamTypeDType* findParamTypeInModule(Ast_گره_Module* modp, ثابت string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ایستا bool retargetRefToModule(AstRefDType* refp, Ast_گره_Module* targetModp);
    ایستا تهی addParamType(AstRefDType* refp, ثابت string& cellPath, Ast_گره_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ثابت string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ایستا Ast_گره_Module* followCellPath(Ast_گره_Module* startModp, ثابت string& cellPath);

    ایستا تهی captureTypedefContext(AstRefDType* refp, ثابت نویسه* stageLabel, صحیح dotPos,
                                      ثابت std::string& dotText, VSymEnt* dotSymp,
                                      Ast_گره_Module* modp,
                                      ثابت std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ایستا تهی dumpEntries(ثابت string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ایستا تهی finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
