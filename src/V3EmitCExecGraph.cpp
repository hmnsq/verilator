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

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitC.h"
#समावेश "V3EmitCFunc.h"
#समावेश "V3ExecGraph.h"

#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

श्रेणी EmitCExecGraph final : खुला EmitCFunc {
    // METHODS
    व्योम emitGraph(AstExecGraph* nodep) {
        स्थिर std::string name = EmitCUtil::execGraphName(nodep);
        स्थिर AstUnpackArrayDType* स्थिर trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<स्थिर AstCFunc*, स्थिर ExecMTask*> mtaskps;
        क्रम (स्थिर V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            स्थिर ExecMTask* स्थिर mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<स्थिर AstCFunc*, uint32_t> indices;
        क्रम (स्थिर AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.आकार());
        }

        // Declare the MTask functions and their trigger masks, and the dispatch function
        माला_दो("\n");
        m_lazyDecls.emit(nodep);
        emitCFuncDecl(nodep->runfuncp(), EmitCParentModule::लो(nodep->runfuncp()));

        // The MTasks
        माला_दो("\n");
        putns(nodep, "static VL_CONSTINIT_CXX20 const VlExecGraph::Vertex " + name
                         + "__vertices[] = {\n");
        AstVarRef* maskp = nodep->maskps();
        क्रम (स्थिर AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            माला_दो(std::to_string(mtaskps.at(callp->funcp())->cost()) + ", ");
            iterateConst(maskp);
            माला_दो(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        माला_दो("};\n");

        // The dependencies between MTasks
        uint32_t nEdges = 0;
        क्रम (स्थिर AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            क्रम (स्थिर V3GraphEdge& edge : mtaskps.at(callp->funcp())->outEdges()) {
                अगर (!nEdges) {
                    माला_दो("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                माला_दो("{" + std::to_string(indices.at(callp->funcp())) + ", "
                     + std::to_string(indices.at(edge.शीर्ष()->as<ExecMTask>()->funcp())) + "},\n");
                ++nEdges;
            }
        }
        अगर (nEdges) माला_दो("};\n");

        // The graph
        स्थिर std::string edgesName = nEdges ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        माला_दो(name + "__vertices, " + std::to_string(indices.आकार()) + ",\n");
        माला_दो(edgesName + ", " + std::to_string(nEdges) + ",\n");
        माला_दो(std::to_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        माला_दो(".data(),\n");
        माला_दो("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        स्थिर bool profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        माला_दो(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        माला_दो("\n};\n");
    }

खुला:
    explicit EmitCExecGraph(AstNetlist* netlistp) {
        std::vector<AstExecGraph*> execGraphps;
        netlistp->शीर्ष_Modulep()->foreach(
            [&](AstExecGraph* nodep) { execGraphps.धकेलो_back(nodep); });
        अगर (execGraphps.empty()) वापस;

        openNewOutputSourceFile(EmitCUtil::शीर्ष_Class_Name() + "__ExecGraph", true, false,
                                "Exec graphs");
        माला_दो("\n");
        माला_दो("#include \"verilated.h\"\n");
        माला_दो("#include \"verilated_threads.h\"\n");
        क्रम (AstExecGraph* स्थिर nodep : execGraphps) emitGraph(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

व्योम V3EmitC::emitcExecGraph() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCExecGraph{v3Global.rootp()};
}
