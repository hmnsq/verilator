// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for exec graphs
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// Emit each AstExecGraph as a global VlExecGraph, holding the function, static cost and
// trigger mask of each MTask, the dependencies between MTasks, and the function that
// dispatches the graph to the thread pool. The code where the graph executes passes the
// VlExecGraph to the run-time library.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCFunc.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

ವರ್ಗ Emit_CExec_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ EmitCFunc {
    // METHODS
    ಶೂನ್ಯ emit_ನಕ್ಷೆ(Ast_Exec_ನಕ್ಷೆ* nodep) {
        ಸ್ಥಿರಾಂಕ std::string name = EmitCUtil::exec_ನಕ್ಷೆ_Name(nodep);
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstCFunc*, ಸ್ಥಿರಾಂಕ ExecMTask*> mtaskps;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : nodep->depGraphp()->vertices()) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstCFunc*, uint32_t> indices;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.ಗಾತ್ರ_2());
        }

        // Declare the MTask functions and their trigger masks, and the dispatch function
        puts("\n");
        m_lazyDecls.emit(nodep);
        emitCFuncDecl(nodep->runfuncp(), EmitCParentModule::get(nodep->runfuncp()));

        // The MTasks
        puts("\n");
        putns(nodep, "static VL_CONSTINIT_CXX20 const VlExecGraph::Vertex " + name
                         + "__vertices[] = {\n");
        AstVarRef* maskp = nodep->maskps();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::ಗಮ್ಯ_string(mtaskps.at(callp->funcp())->ವೆಚ್ಚ()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t n_ಅಂಚುಗಳು = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskps.at(callp->funcp())->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (!n_ಅಂಚುಗಳು) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::ಗಮ್ಯ_string(indices.at(callp->funcp())) + ", "
                     + std::ಗಮ್ಯ_string(indices.at(ಅಂಚು.ತುದಿ()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_ಅಂಚುಗಳು;
            }
        }
        ಆದರೆ (n_ಅಂಚುಗಳು) puts("};\n");

        // The graph
        ಸ್ಥಿರಾಂಕ std::string ಅಂಚುಗಳು_Name = n_ಅಂಚುಗಳು ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::ಗಮ್ಯ_string(indices.ಗಾತ್ರ_2()) + ",\n");
        puts(ಅಂಚುಗಳು_Name + ", " + std::ಗಮ್ಯ_string(n_ಅಂಚುಗಳು) + ",\n");
        puts(std::ಗಮ್ಯ_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Emit_CExec_ನಕ್ಷೆ(AstNetlist* netlistp) {
        std::ಸದಿಶ<Ast_Exec_ನಕ್ಷೆ*> execGraphps;
        netlistp->ತುದಿ_Modulep()->foreach(
            [&](Ast_Exec_ನಕ್ಷೆ* nodep) { execGraphps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep); });
        ಆದರೆ (execGraphps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        open_New_Output_ಮೂಲ_File(EmitCUtil::ತುದಿ_Class_Name() + "__ExecGraph", ನಿಜ, ಸುಳ್ಳು,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        ಗಾಗಿ (Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ nodep : execGraphps) emit_ನಕ್ಷೆ(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

ಶೂನ್ಯ V3EmitC::emitc_Exec_ನಕ್ಷೆ() {
    UINFO(2, __FUNCTION__ << ":");
    Emit_CExec_ನಕ್ಷೆ{v3Global.rootp()};
}
