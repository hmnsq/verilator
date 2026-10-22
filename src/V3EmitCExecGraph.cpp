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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCFunc.h"
#ضمن "V3ExecGraph.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

صنف Emit_CExec_ګراف final : عام EmitCFunc {
    // METHODS
    فراغ emit_ګراف(Ast_Exec_ګراف* nodep) {
        ثابت std::string name = EmitCUtil::exec_ګراف_Name(nodep);
        ثابت AstUnpackArrayDType* ثابت trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ثابت AstCFunc*, ثابت ExecMTask*> mtaskps;
        لكل (ثابت V3_ګراف_راس& vtx : nodep->depGraphp()->vertices()) {
            ثابت ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ثابت AstCFunc*, uint32_t> indices;
        لكل (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.اندازه());
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
        لكل (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::منزل_string(mtaskps.at(callp->funcp())->لګښت()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t n_څنډې = 0;
        لكل (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            لكل (ثابت V3_ګراف_څنډه& څنډه : mtaskps.at(callp->funcp())->بهر_څنډې()) {
                إذا (!n_څنډې) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::منزل_string(indices.at(callp->funcp())) + ", "
                     + std::منزل_string(indices.at(څنډه.سر()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_څنډې;
            }
        }
        إذا (n_څنډې) puts("};\n");

        // The graph
        ثابت std::string څنډې_Name = n_څنډې ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::منزل_string(indices.اندازه()) + ",\n");
        puts(څنډې_Name + ", " + std::منزل_string(n_څنډې) + ",\n");
        puts(std::منزل_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ثابت منطقي profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

عام:
    څرګند Emit_CExec_ګراف(AstNetlist* netlistp) {
        std::ویکټور<Ast_Exec_ګراف*> execGraphps;
        netlistp->سر_Modulep()->foreach(
            [&](Ast_Exec_ګراف* nodep) { execGraphps.ټېل_شا(nodep); });
        إذا (execGraphps.تشه()) أرجع;

        open_New_Output_سرچینه_File(EmitCUtil::سر_Class_Name() + "__ExecGraph", ریښتیا, خطأ,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        لكل (Ast_Exec_ګراف* ثابت nodep : execGraphps) emit_ګراف(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

فراغ V3EmitC::emitc_Exec_ګراف() {
    UINFO(2, __FUNCTION__ << ":");
    Emit_CExec_ګراف{v3Global.rootp()};
}
