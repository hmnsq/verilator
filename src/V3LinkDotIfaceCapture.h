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

#അന്തര്ഭാവ "config_build.h"

#അന്തര്ഭാവ "V3Ast.h"

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIfaceCaptureTag final {
പൊതു:
    എണ്ണൽ class Kind : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    Kind m_kind;  // What the reference refers to
    string m_cellPath;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_capturedInName;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    സ്ഥിതം std::deque<VIfaceCaptureTag> s_tags;  // Owns every tag; a deque keeps addresses stable
    സ്ഥിതം bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    സ്ഥിതം ശൂന്യം enable(bool flag);  // LCOV_EXCL_LINE
    സ്ഥിതം ശൂന്യം reset();
    സ്ഥിതം ശൂന്യം clearModuleCache();
    സ്ഥിതം AstIfaceRefDType* ifaceRefFromVarDType(Ast_കണ്ണി_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    സ്ഥിതം ശൂന്യം retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    സ്ഥിതം ശൂന്യം retargetRefToParamType(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    സ്ഥിതം ശൂന്യം tag(AstRefDType* refp, സ്ഥിരം Ast_കണ്ണി_Module* capturedInp,
                    VIfaceCaptureTag::Kind kind, സ്ഥിരം string& cellPath,
                    സ്ഥിരം string& ownerModName);
    സ്ഥിതം പൂർണ്ണസംഖ്യ resolveCapturedRefs();
    സ്ഥിതം ശൂന്യം verifyNoDeadRefs(സ്ഥിരം std::unordered_set<സ്ഥിരം Ast_കണ്ണി*>& liveNodes);

പൊതു:
    സ്ഥിതം bool enabled() { തിരികെ s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    സ്ഥിതം സ്ഥിരം VIfaceCaptureTag* captureTag(AstRefDType* refp);
    സ്ഥിതം Ast_കണ്ണി_Module* findOwnerModule(Ast_കണ്ണി* nodep);
    // Find a Typedef by name in a module's top-level statements
    സ്ഥിതം AstTypedef* findTypedefInModule(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    സ്ഥിതം Ast_കണ്ണി_DType* findDTypeInModule(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    സ്ഥിതം AstParamTypeDType* findParamTypeInModule(Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    സ്ഥിതം bool retargetRefToModule(AstRefDType* refp, Ast_കണ്ണി_Module* targetModp);
    സ്ഥിതം ശൂന്യം addParamType(AstRefDType* refp, സ്ഥിരം string& cellPath, Ast_കണ്ണി_Module* ownerModp,
                             AstParamTypeDType* paramTypep, സ്ഥിരം string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    സ്ഥിതം Ast_കണ്ണി_Module* followCellPath(Ast_കണ്ണി_Module* startModp, സ്ഥിരം string& cellPath);

    സ്ഥിതം ശൂന്യം captureTypedefContext(AstRefDType* refp, സ്ഥിരം അക്ഷരം* stageLabel, പൂർണ്ണസംഖ്യ dotPos,
                                      സ്ഥിരം std::string& dotText, VSymEnt* dotSymp,
                                      Ast_കണ്ണി_Module* modp,
                                      സ്ഥിരം std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    സ്ഥിതം ശൂന്യം dumpEntries(സ്ഥിരം string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    സ്ഥിതം ശൂന്യം finalizeIfaceCapture();
};

#endif  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
