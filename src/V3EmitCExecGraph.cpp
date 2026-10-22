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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCFunc.h"
#अन्तर्भाव "V3ExecGraph.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

वर्ग EmitCExecGraph final : सार्वजनिक EmitCFunc {
    // METHODS
    शून्य emitGraph(AstExecGraph* nodep) {
        स्थिरांक std::string name = EmitCUtil::execGraphName(nodep);
        स्थिरांक AstUnpackArrayDType* स्थिरांक trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<स्थिरांक AstCFunc*, स्थिरांक ExecMTask*> mtaskps;
        साठी (स्थिरांक V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            स्थिरांक ExecMTask* स्थिरांक mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<स्थिरांक AstCFunc*, uint32_t> indices;
        साठी (स्थिरांक AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.size());
        }

        // Declare the MTask functions and their trigger masks, and the dispatch function
        माला_दो("\n");
        m_lazyDecls.emit(nodep);
        emitCFuncDecl(nodep->runfuncp(), EmitCParentModule::get(nodep->runfuncp()));

        // The MTasks
        माला_दो("\n");
        putns(nodep, "static VL_CONSTINIT_CXX20 const VlExecGraph::Vertex " + name
                         + "__vertices[] = {\n");
        AstVarRef* maskp = nodep->maskps();
        साठी (स्थिरांक AstCCall* callp = nodep->callsp(); callp;
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
        साठी (स्थिरांक AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            साठी (स्थिरांक V3GraphEdge& edge : mtaskps.at(callp->funcp())->outEdges()) {
                जर (!nEdges) {
                    माला_दो("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                माला_दो("{" + std::to_string(indices.at(callp->funcp())) + ", "
                     + std::to_string(indices.at(edge.शीर्ष()->as<ExecMTask>()->funcp())) + "},\n");
                ++nEdges;
            }
        }
        जर (nEdges) माला_दो("};\n");

        // The graph
        स्थिरांक std::string edgesName = nEdges ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        माला_दो(name + "__vertices, " + std::to_string(indices.size()) + ",\n");
        माला_दो(edgesName + ", " + std::to_string(nEdges) + ",\n");
        माला_दो(std::to_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        माला_दो(".data(),\n");
        माला_दो("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        स्थिरांक तार्किक profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        माला_दो(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        माला_दो("\n};\n");
    }

सार्वजनिक:
    explicit EmitCExecGraph(AstNetlist* netlistp) {
        std::vector<AstExecGraph*> execGraphps;
        netlistp->शीर्ष_Modulep()->foreach(
            [&](AstExecGraph* nodep) { execGraphps.ढकला_back(nodep); });
        जर (execGraphps.empty()) परत;

        openNewOutputSourceFile(EmitCUtil::शीर्ष_Class_Name() + "__ExecGraph", सत्य, असत्य,
                                "Exec graphs");
        माला_दो("\n");
        माला_दो("#include \"verilated.h\"\n");
        माला_दो("#include \"verilated_threads.h\"\n");
        साठी (AstExecGraph* स्थिरांक nodep : execGraphps) emitGraph(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

शून्य V3EmitC::emitcExecGraph() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCExecGraph{v3Global.rootp()};
}
