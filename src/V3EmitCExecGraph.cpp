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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCFunc.h"
#అన్తర్భావ "V3ExecGraph.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

తరగతి Emit_CExec_లేఖాచిత్రం final : బహిరంగ EmitCFunc {
    // METHODS
    శూన్య emit_లేఖాచిత్రం(Ast_Exec_లేఖాచిత్రం* nodep) {
        స్థిరరాశి std::string name = EmitCUtil::exec_లేఖాచిత్రం_Name(nodep);
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<స్థిరరాశి AstCFunc*, స్థిరరాశి ExecMTask*> mtaskps;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : nodep->depGraphp()->vertices()) {
            స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<స్థిరరాశి AstCFunc*, uint32_t> indices;
        కొరకు (స్థిరరాశి AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.కొలత());
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
        కొరకు (స్థిరరాశి AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::గమ్యం_string(mtaskps.at(callp->funcp())->ఖర్చు()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t n_అంచులు = 0;
        కొరకు (స్థిరరాశి AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mtaskps.at(callp->funcp())->బహిర్గతం_అంచులు()) {
                ఒకవేళ (!n_అంచులు) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::గమ్యం_string(indices.at(callp->funcp())) + ", "
                     + std::గమ్యం_string(indices.at(అంచు.శిఖరం()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_అంచులు;
            }
        }
        ఒకవేళ (n_అంచులు) puts("};\n");

        // The graph
        స్థిరరాశి std::string అంచులు_Name = n_అంచులు ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::గమ్యం_string(indices.కొలత()) + ",\n");
        puts(అంచులు_Name + ", " + std::గమ్యం_string(n_అంచులు) + ",\n");
        puts(std::గమ్యం_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        స్థిరరాశి తార్కిక profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

బహిరంగ:
    స్పష్టమైన Emit_CExec_లేఖాచిత్రం(AstNetlist* netlistp) {
        std::సదిశ<Ast_Exec_లేఖాచిత్రం*> execGraphps;
        netlistp->శిఖరం_Modulep()->foreach(
            [&](Ast_Exec_లేఖాచిత్రం* nodep) { execGraphps.నెట్టు_వెనుక(nodep); });
        ఒకవేళ (execGraphps.ఖాళీలు()) తిరిగి;

        open_New_Output_ఆధారం_File(EmitCUtil::శిఖరం_Class_Name() + "__ExecGraph", నిజం, అబద్ధం,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        కొరకు (Ast_Exec_లేఖాచిత్రం* స్థిరరాశి nodep : execGraphps) emit_లేఖాచిత్రం(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

శూన్య V3EmitC::emitc_Exec_లేఖాచిత్రం() {
    UINFO(2, __FUNCTION__ << ":");
    Emit_CExec_లేఖాచిత్రం{v3Global.rootp()};
}
