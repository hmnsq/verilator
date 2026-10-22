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

#که_نه_تعریف_شوی VERILATOR_V3LINKDOTIFACECAPTURE_H_
#تعریف_کړه VERILATOR_V3LINKDOTIFACECAPTURE_H_

#ضمن "config_build.h"

#ضمن "V3Ast.h"

#ضمن <deque>
#ضمن <functional>
#ضمن <string>
#ضمن <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIface_Capture_نښان final {
عام:
    تعداد class ډول_2 : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    ډول_2 m_ډول;  // What the reference refers to
    string m_cell_لار;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_captured_دننه_Name;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    ساكن std::deque<VIface_Capture_نښان> s_نښانونه;  // Owns every tag; a deque keeps addresses stable
    ساكن bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    ساكن فراغ enable(bool flag);  // LCOV_EXCL_LINE
    ساكن فراغ reset();
    ساكن فراغ clearModuleCache();
    ساكن AstIfaceRefDType* ifaceRefFromVarDType(Ast_غوټه_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    ساكن فراغ retarget_Ref_منزل_Typedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    ساكن فراغ retarget_Ref_منزل_Param_Type(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    ساكن فراغ نښان(AstRefDType* refp, ثابت Ast_غوټه_Module* capturedInp,
                    VIface_Capture_نښان::ډول_2 ډول, ثابت string& cell_لار,
                    ثابت string& ownerModName);
    ساكن صحيح resolveCapturedRefs();
    ساكن فراغ verifyNoDeadRefs(ثابت std::unordered_سېټ<ثابت Ast_غوټه*>& ژوندی_Nodes_2);

عام:
    ساكن bool enabled() { أرجع s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    ساكن ثابت VIface_Capture_نښان* capture_نښان(AstRefDType* refp);
    ساكن Ast_غوټه_Module* پیدا_کړه_Owner_Module(Ast_غوټه* nodep);
    // Find a Typedef by name in a module's top-level statements
    ساكن AstTypedef* پیدا_کړه_Typedef_دننه_Module(Ast_غوټه_Module* modp, ثابت string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    ساكن Ast_غوټه_DType* پیدا_کړه_DType_دننه_Module(Ast_غوټه_Module* modp, ثابت string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    ساكن AstParamTypeDType* پیدا_کړه_Param_Type_دننه_Module(Ast_غوټه_Module* modp, ثابت string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    ساكن bool retarget_Ref_منزل_Module(AstRefDType* refp, Ast_غوټه_Module* موخه_Modp);
    ساكن فراغ جمع_کړه_Param_Type(AstRefDType* refp, ثابت string& cell_لار, Ast_غوټه_Module* ownerModp,
                             AstParamTypeDType* paramTypep, ثابت string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    ساكن Ast_غوټه_Module* follow_Cell_لار(Ast_غوټه_Module* پیل_Modp, ثابت string& cell_لار);

    ساكن فراغ captureTypedefContext(AstRefDType* refp, ثابت حرف* stageLabel, صحيح dotPos,
                                      ثابت std::string& dot_متن, VSymEnt* dotSymp,
                                      Ast_غوټه_Module* modp,
                                      ثابت std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    ساكن فراغ dumpEntries(ثابت string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    ساكن فراغ finalizeIfaceCapture();
};

#د_که_پای  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
