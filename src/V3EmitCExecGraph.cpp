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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCFunc.h"
#شامل "V3ExecGraph.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

رده EmitCExecGraph final : عمومی EmitCFunc {
    // METHODS
    تهی emitGraph(AstExecGraph* nodep) {
        ثابت std::string name = EmitCUtil::execGraphName(nodep);
        ثابت AstUnpackArrayDType* ثابت trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ثابت AstCFunc*, ثابت ExecMTask*> mtaskps;
        برای (ثابت V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            ثابت ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ثابت AstCFunc*, uint32_t> indices;
        برای (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.size());
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
        برای (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::to_string(mtaskps.at(callp->funcp())->cost()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t nEdges = 0;
        برای (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            برای (ثابت V3GraphEdge& edge : mtaskps.at(callp->funcp())->outEdges()) {
                اگر (!nEdges) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::to_string(indices.at(callp->funcp())) + ", "
                     + std::to_string(indices.at(edge.بالاترین()->as<ExecMTask>()->funcp())) + "},\n");
                ++nEdges;
            }
        }
        اگر (nEdges) puts("};\n");

        // The graph
        ثابت std::string edgesName = nEdges ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::to_string(indices.size()) + ",\n");
        puts(edgesName + ", " + std::to_string(nEdges) + ",\n");
        puts(std::to_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ثابت منطقی profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

عمومی:
    explicit EmitCExecGraph(AstNetlist* netlistp) {
        std::vector<AstExecGraph*> execGraphps;
        netlistp->بالاترین_Modulep()->foreach(
            [&](AstExecGraph* nodep) { execGraphps.افزودن_back(nodep); });
        اگر (execGraphps.empty()) بازگشت;

        openNewOutputSourceFile(EmitCUtil::بالاترین_Class_Name() + "__ExecGraph", درست, نادرست,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        برای (AstExecGraph* ثابت nodep : execGraphps) emitGraph(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

تهی V3EmitC::emitcExecGraph() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCExecGraph{v3Global.rootp()};
}
