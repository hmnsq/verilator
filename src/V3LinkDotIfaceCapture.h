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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LINKDOTIFACECAPTURE_H_
#تعریف_کرو VERILATOR_V3LINKDOTIFACECAPTURE_H_

#شامل "config_build.h"

#شامل "V3Ast.h"

#شامل <deque>
#شامل <functional>
#شامل <string>
#شامل <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIface_Capture_لیبل final {
عوامی:
    شمار class قسم_2 : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    قسم_2 m_قسم;  // What the reference refers to
    string m_cell_راستہ;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_captured_اندر_Name;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ساکن std::deque<VIface_Capture_لیبل> s_لیبل_فہرست;  // Owns every tag; a deque keeps addresses stable
    ساکن bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ساکن خالی enable(bool flag);  // LCOV_EXCL_LINE
    ساکن خالی reset();
    ساکن خالی clearModuleCache();
    ساکن AstIfaceRefDType* ifaceRefFromVarDType(Ast_گرہ_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ساکن خالی retarget_Ref_مقصد_Typedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ساکن خالی retarget_Ref_مقصد_Param_Type(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ساکن خالی لیبل(AstRefDType* refp, ثابت Ast_گرہ_Module* capturedInp,
                    VIface_Capture_لیبل::قسم_2 قسم, ثابت string& cell_راستہ,
                    ثابت string& ownerModName);
    ساکن صحیح resolveCapturedRefs();
    ساکن خالی verifyNoDeadRefs(ثابت std::unordered_سیٹ<ثابت Ast_گرہ*>& زندہ_Nodes_2);

عوامی:
    ساکن bool enabled() { واپس s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ساکن ثابت VIface_Capture_لیبل* capture_لیبل(AstRefDType* refp);
    ساکن Ast_گرہ_Module* ڈھونڈو_Owner_Module(Ast_گرہ* nodep);
    // Find a Typedef by name in a module's top-level statements
    ساکن AstTypedef* ڈھونڈو_Typedef_اندر_Module(Ast_گرہ_Module* modp, ثابت string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ساکن Ast_گرہ_DType* ڈھونڈو_DType_اندر_Module(Ast_گرہ_Module* modp, ثابت string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ساکن AstParamTypeDType* ڈھونڈو_Param_Type_اندر_Module(Ast_گرہ_Module* modp, ثابت string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ساکن bool retarget_Ref_مقصد_Module(AstRefDType* refp, Ast_گرہ_Module* ہدف_Modp);
    ساکن خالی جوڑو_Param_Type(AstRefDType* refp, ثابت string& cell_راستہ, Ast_گرہ_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ثابت string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ساکن Ast_گرہ_Module* follow_Cell_راستہ(Ast_گرہ_Module* آغاز_Modp, ثابت string& cell_راستہ);

    ساکن خالی captureTypedefContext(AstRefDType* refp, ثابت حرف* stageLabel, صحیح dotPos,
                                      ثابت std::string& dot_متن, VSymEnt* dotSymp,
                                      Ast_گرہ_Module* modp,
                                      ثابت std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ساکن خالی dumpEntries(ثابت string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ساکن خالی finalizeIfaceCapture();
};

#اگر_ختم  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
