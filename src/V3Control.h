// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3CONFIG_H_
#నిర్వచించు VERILATOR_V3CONFIG_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Mutex.h"

//######################################################################

class V3Control final {
బహిరంగ:
    నిర్మాణం Fsm_రిజిస్టరు_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_విలువ;
    };

    గణన class Var_Spec_రకం : uint8_t {
        PARAM,  // Select only matching parameters
        పోర్టు,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    స్థిర శూన్య కూడు_Case_నిండిన(స్థిరరాశి string& file, పూర్ణాంకం lineno);
    స్థిర శూన్య కూడు_Case_Parallel(స్థిరరాశి string& file, పూర్ణాంకం lineno);
    స్థిర శూన్య కూడు_Coverage_ఖండం_Off(స్థిరరాశి string& file, పూర్ణాంకం lineno);
    స్థిర శూన్య కూడు_Coverage_ఖండం_Off(స్థిరరాశి string& module, స్థిరరాశి string& blockname);
    స్థిర శూన్య కూడు_Hier_Workers(File_పంక్తి* fl, స్థిరరాశి string& model, పూర్ణాంకం workers);
    స్థిర శూన్య కూడు_Fsm_రిజిస్టరు_Wrapper(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& d,
                                      స్థిరరాశి string& q, స్థిరరాశి string& clock, స్థిరరాశి string& reset,
                                      స్థిరరాశి string& reset_విలువ);
    స్థిర శూన్య కూడు_Ignore(V3_Error_సంకేతం సంకేతం_2, bool on, స్థిరరాశి string& filename, పూర్ణాంకం min, పూర్ణాంకం max);
    స్థిర శూన్య కూడు_Ignore_సరిపోలిక(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& filename, స్థిరరాశి string& contents,
                               స్థిరరాశి string& సరిపోలిక);
    స్థిర శూన్య కూడు_Inline(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& ftask, bool on);
    స్థిర శూన్య కూడు_Module_Pragma(స్థిరరాశి string& module, VPragmaType pragma);
    స్థిర శూన్య కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& hierDpi, uint64_t ఖర్చు);
    స్థిర శూన్య కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& model, స్థిరరాశి string& కీలకం,
                               uint64_t ఖర్చు);
    స్థిర శూన్య కూడు_Scope_Trace_On(bool on, స్థిరరాశి string& scope, పూర్ణాంకం స్థాయిలు);
    స్థిర శూన్య కూడు_Var_Attr(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& ftask,
                           Var_Spec_రకం రకం, స్థిరరాశి string& నమూనా_2, VAttrType type,
                           Ast_Sen_వృక్షం* nodep);

    స్థిర శూన్య applyCase(AstCase* nodep);
    స్థిర శూన్య apply_Coverage_ఖండం(Ast_కణుపు_Module* modulep, AstBegin* nodep);
    స్థిర శూన్య apply_Coverage_ఖండం(Ast_కణుపు_Module* modulep, Ast_Gen_ఖండం* nodep);
    స్థిర శూన్య applyFTask(Ast_కణుపు_Module* modulep, Ast_కణుపు_FTask* ftaskp);
    స్థిర శూన్య applyIgnores(File_పంక్తి* filelinep);
    స్థిర శూన్య applyModule(Ast_కణుపు_Module* modulep);
    స్థిర శూన్య applyVarAttr(స్థిరరాశి Ast_కణుపు_Module* modulep, స్థిరరాశి Ast_కణుపు_FTask* ftaskp,
                             AstVar* varp);

    స్థిర పూర్ణాంకం getHierWorkers(స్థిరరాశి string& model);
    స్థిర File_పంక్తి* get_Hier_Workers_File_పంక్తి(స్థిరరాశి string& model);
    స్థిర స్థిరరాశి Fsm_రిజిస్టరు_Wrapper* get_Fsm_రిజిస్టరు_Wrapper(స్థిరరాశి string& module);
    స్థిర uint64_t get_Profile_దత్తాంశం(స్థిరరాశి string& hierDpi);
    స్థిర uint64_t get_Profile_దత్తాంశం(స్థిరరాశి string& model, స్థిరరాశి string& కీలకం);
    స్థిర File_పంక్తి* get_Profile_దత్తాంశం_File_పంక్తి();
    స్థిర bool getScopeTraceOn(స్థిరరాశి string& scope);

    స్థిర శూన్య contents_నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం);

    స్థిర bool కలిగియుంది_MTask_Profile_దత్తాంశం();
    స్థిర uint64_t get_Current_Hier_ఖండం_ఖర్చు();

    స్థిర bool waive(స్థిరరాశి File_పంక్తి* filelinep, V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& సందేశం);

    స్థిర శూన్య selfTest();
};

#అయితే_ముగింపు  // Guard
