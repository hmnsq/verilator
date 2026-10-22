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

#నిర్వచితం_కాకపోతే VERILATOR_V3LINKDOTIFACECAPTURE_H_
#నిర్వచించు VERILATOR_V3LINKDOTIFACECAPTURE_H_

#అన్తర్భావ "config_build.h"

#అన్తర్భావ "V3Ast.h"

#అన్తర్భావ <deque>
#అన్తర్భావ <functional>
#అన్తర్భావ <string>
#అన్తర్భావ <unordered_set>

class VSymEnt;

// Capture record of an AstRefDType, shared by all clones of that reference
class VIface_Capture_ట్యాగ్ final {
బహిరంగ:
    గణన class రకం_2 : uint8_t {
        TYPEDEF,  // Refers to a typedef in the interface
        PARAM_TYPE  // Refers to a type parameter of the interface
    };
    రకం_2 m_రకం;  // What the reference refers to
    string m_cell_మార్గం;  // Cell path from the owner module (e.g. "cca_io.tlb_io")
    string m_ownerModName;  // Name of the interface that owns the target
    string m_captured_లోపల_Name;  // Original name of the module the reference was captured in
};

class V3LinkDotIfaceCapture final {
    స్థిర std::deque<VIface_Capture_ట్యాగ్> s_ట్యాగులు;  // Owns every tag; a deque keeps addresses stable
    స్థిర bool s_enabled;

    // --- Internal-only methods (not called outside V3LinkDotIfaceCapture.cpp) ---
    స్థిర శూన్య enable(bool flag);  // LCOV_EXCL_LINE
    స్థిర శూన్య reset();
    స్థిర శూన్య clearModuleCache();
    స్థిర AstIfaceRefDType* ifaceRefFromVarDType(Ast_కణుపు_DType* dtypep);
    // Point a reference at a typedef and fix its other links.
    స్థిర శూన్య retarget_Ref_గమ్యం_Typedef(AstRefDType* refp, AstTypedef* typedefp);
    // Same, for a parameter type.
    స్థిర శూన్య retarget_Ref_గమ్యం_Param_Type(AstRefDType* refp, AstParamTypeDType* paramTypep);
    // Tag a reference captured in capturedInp
    స్థిర శూన్య ట్యాగ్(AstRefDType* refp, స్థిరరాశి Ast_కణుపు_Module* capturedInp,
                    VIface_Capture_ట్యాగ్::రకం_2 రకం, స్థిరరాశి string& cell_మార్గం,
                    స్థిరరాశి string& ownerModName);
    స్థిర పూర్ణాంకం resolveCapturedRefs();
    స్థిర శూన్య verifyNoDeadRefs(స్థిరరాశి std::unordered_సమితి<స్థిరరాశి Ast_కణుపు*>& సజీవ_Nodes_2);

బహిరంగ:
    స్థిర bool enabled() { తిరిగి s_enabled; }
    // Tag of a reference, or nullptr if untagged or copied into another module
    స్థిర స్థిరరాశి VIface_Capture_ట్యాగ్* capture_ట్యాగ్(AstRefDType* refp);
    స్థిర Ast_కణుపు_Module* కనుగొను_Owner_Module(Ast_కణుపు* nodep);
    // Find a Typedef by name in a module's top-level statements
    స్థిర AstTypedef* కనుగొను_Typedef_లోపల_Module(Ast_కణుపు_Module* modp, స్థిరరాశి string& name);
    // Find a NodeDType by name and VNType in a module's top-level statements
    స్థిర Ast_కణుపు_DType* కనుగొను_DType_లోపల_Module(Ast_కణుపు_Module* modp, స్థిరరాశి string& name, VNType type);
    // Find a ParamTypeDType by name in a module's top-level statements
    స్థిర AstParamTypeDType* కనుగొను_Param_Type_లోపల_Module(Ast_కణుపు_Module* modp, స్థిరరాశి string& name);
    // Retarget a captured reference at the same-named target in targetModp.
    స్థిర bool retarget_Ref_గమ్యం_Module(AstRefDType* refp, Ast_కణుపు_Module* లక్ష్యం_Modp);
    స్థిర శూన్య కూడు_Param_Type(AstRefDType* refp, స్థిరరాశి string& cell_మార్గం, Ast_కణుపు_Module* ownerModp,
                             AstParamTypeDType* paramTypep, స్థిరరాశి string& paramTypeOwnerModName);

    // Walk a dot-separated cell path (e.g. "cca_io.tlb_io") starting from
    // startModp, returning the module at the end of the path.  Returns
    // nullptr if any component cannot be resolved.
    స్థిర Ast_కణుపు_Module* follow_Cell_మార్గం(Ast_కణుపు_Module* ప్రారంభం_Modp, స్థిరరాశి string& cell_మార్గం);

    స్థిర శూన్య captureTypedefContext(AstRefDType* refp, స్థిరరాశి అక్షరం* stageLabel, పూర్ణాంకం dotPos,
                                      స్థిరరాశి std::string& dot_పాఠ్యం, VSymEnt* dotSymp,
                                      Ast_కణుపు_Module* modp,
                                      స్థిరరాశి std::function<std::string()>& indentFn);

    // Debug: dump all captured references
    స్థిర శూన్య dumpEntries(స్థిరరాశి string& label);

    // Called after V3Param but before V3Dead to fix any remaining cross-interface refs
    // that still point to template nodes (which will be deleted by V3Dead).
    స్థిర శూన్య finalizeIfaceCapture();
};

#అయితే_ముగింపు  // VERILATOR_V3LINKDOTIFACECAPTURE_H_
