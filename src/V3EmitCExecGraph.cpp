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
        ध्रुव std::string name = EmitCUtil::execGraphName(nodep);
        ध्रुव AstUnpackArrayDType* ध्रुव trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ध्रुव AstCFunc*, ध्रुव ExecMTask*> mtaskps;
        प्रति (ध्रुव V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            ध्रुव ExecMTask* ध्रुव mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ध्रुव AstCFunc*, uint32_t> indices;
        प्रति (ध्रुव AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.आकार());
        }

        // Declare the MTask functions and their trigger masks, and the dispatch function
        पङ्क्तिं_लिख("\n");
        m_lazyDecls.emit(nodep);
        emitCFuncDecl(nodep->runfuncp(), EmitCParentModule::get(nodep->runfuncp()));

        // The MTasks
        पङ्क्तिं_लिख("\n");
        putns(nodep, "static VL_CONSTINIT_CXX20 const VlExecGraph::Vertex " + name
                         + "__vertices[] = {\n");
        AstVarRef* maskp = nodep->maskps();
        प्रति (ध्रुव AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            पङ्क्तिं_लिख(std::गन्तव्यम्_string(mtaskps.at(callp->funcp())->cost()) + ", ");
            iterateConst(maskp);
            पङ्क्तिं_लिख(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        पङ्क्तिं_लिख("};\n");

        // The dependencies between MTasks
        uint32_t n_धाराः = 0;
        प्रति (ध्रुव AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            प्रति (ध्रुव V3GraphEdge& edge : mtaskps.at(callp->funcp())->बहिर्गमनम्_धाराः()) {
                यदि (!n_धाराः) {
                    पङ्क्तिं_लिख("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                पङ्क्तिं_लिख("{" + std::गन्तव्यम्_string(indices.at(callp->funcp())) + ", "
                     + std::गन्तव्यम्_string(indices.at(edge.शिखरम्()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_धाराः;
            }
        }
        यदि (n_धाराः) पङ्क्तिं_लिख("};\n");

        // The graph
        ध्रुव std::string धाराः_Name = n_धाराः ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        पङ्क्तिं_लिख(name + "__vertices, " + std::गन्तव्यम्_string(indices.आकार()) + ",\n");
        पङ्क्तिं_लिख(धाराः_Name + ", " + std::गन्तव्यम्_string(n_धाराः) + ",\n");
        पङ्क्तिं_लिख(std::गन्तव्यम्_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        पङ्क्तिं_लिख(".data(),\n");
        पङ्क्तिं_लिख("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ध्रुव तार्किक profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        पङ्क्तिं_लिख(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        पङ्क्तिं_लिख("\n};\n");
    }

सार्वजनिक:
    स्पष्ट EmitCExecGraph(AstNetlist* netlistp) {
        std::सदिशम्<AstExecGraph*> execGraphps;
        netlistp->शिखरम्_Modulep()->foreach(
            [&](AstExecGraph* nodep) { execGraphps.प्रक्षिप_पृष्ठभागः(nodep); });
        यदि (execGraphps.रिक्तानि()) प्रत्यागम;

        openNewOutputSourceFile(EmitCUtil::शिखरम्_Class_Name() + "__ExecGraph", सत्य, असत्य,
                                "Exec graphs");
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("#include \"verilated.h\"\n");
        पङ्क्तिं_लिख("#include \"verilated_threads.h\"\n");
        प्रति (AstExecGraph* ध्रुव nodep : execGraphps) emitGraph(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

शून्य V3EmitC::emitcExecGraph() {
    UINFO(2, __FUNCTION__ << ":");
    EmitCExecGraph{v3Global.rootp()};
}
