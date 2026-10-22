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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LINKDOTIFACECAPTURE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LINKDOTIFACECAPTURE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIface_Capture_ਟੈਗ final {
ਜਨਤਕ:
    ਗਿਣਤੀ class ਕਿਸਮ_2 : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    ਕਿਸਮ_2 m_ਕਿਸਮ;  // What the reference refers to
    string m_cell_ਰਸਤਾ;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_captured_ਵਿੱਚ_Name;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ਸਥਿਰ std::deque<VIface_Capture_ਟੈਗ> s_ਟੈਗ_ਸੂਚੀ;  // Owns every tag; a deque keeps addresses stable
    ਸਥਿਰ bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ਸਥਿਰ ਖਾਲੀ enable(bool flag);  // LCOV_EXCL_LINE
    ਸਥਿਰ ਖਾਲੀ reset();
    ਸਥਿਰ ਖਾਲੀ clearModuleCache();
    ਸਥਿਰ AstIfaceRefDType* ifaceRefFromVarDType(Ast_ਗੰਢ_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ਸਥਿਰ ਖਾਲੀ retarget_Ref_ਮੰਜ਼ਿਲ_Typedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ਸਥਿਰ ਖਾਲੀ retarget_Ref_ਮੰਜ਼ਿਲ_Param_Type(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ਸਥਿਰ ਖਾਲੀ ਟੈਗ(AstRefDType* refp, ਅਚਲ Ast_ਗੰਢ_Module* capturedInp,
                    VIface_Capture_ਟੈਗ::ਕਿਸਮ_2 ਕਿਸਮ, ਅਚਲ string& cell_ਰਸਤਾ,
                    ਅਚਲ string& ownerModName);
    ਸਥਿਰ ਪੂਰਨ resolveCapturedRefs();
    ਸਥਿਰ ਖਾਲੀ verifyNoDeadRefs(ਅਚਲ std::unordered_ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ*>& ਜੀਵਤ_Nodes_2);

ਜਨਤਕ:
    ਸਥਿਰ bool enabled() { ਵਾਪਸ s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ਸਥਿਰ ਅਚਲ VIface_Capture_ਟੈਗ* capture_ਟੈਗ(AstRefDType* refp);
    ਸਥਿਰ Ast_ਗੰਢ_Module* ਲੱਭੋ_Owner_Module(Ast_ਗੰਢ* nodep);
    // Find a Typedef by name in a module's top-level statements
    ਸਥਿਰ AstTypedef* ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ਸਥਿਰ Ast_ਗੰਢ_DType* ਲੱਭੋ_DType_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ਸਥਿਰ AstParamTypeDType* ਲੱਭੋ_Param_Type_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ਸਥਿਰ bool retarget_Ref_ਮੰਜ਼ਿਲ_Module(AstRefDType* refp, Ast_ਗੰਢ_Module* ਨਿਸ਼ਾਨਾ_Modp);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Param_Type(AstRefDType* refp, ਅਚਲ string& cell_ਰਸਤਾ, Ast_ਗੰਢ_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ਅਚਲ string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ਸਥਿਰ Ast_ਗੰਢ_Module* follow_Cell_ਰਸਤਾ(Ast_ਗੰਢ_Module* ਸ਼ੁਰੂ_Modp, ਅਚਲ string& cell_ਰਸਤਾ);

    ਸਥਿਰ ਖਾਲੀ captureTypedefContext(AstRefDType* refp, ਅਚਲ ਅੱਖਰ* stageLabel, ਪੂਰਨ dotPos,
                                      ਅਚਲ std::string& dot_ਪਾਠ, VSymEnt* dotSymp,
                                      Ast_ਗੰਢ_Module* modp,
                                      ਅਚਲ std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ਸਥਿਰ ਖਾਲੀ dumpEntries(ਅਚਲ string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ਸਥਿਰ ਖਾਲੀ finalizeIfaceCapture();
};

#ਜੇ_ਅੰਤ  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
