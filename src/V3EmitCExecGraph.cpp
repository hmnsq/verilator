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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCFunc.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Exec graph emitter

ਵਰਗ Emit_CExec_ਗ੍ਰਾਫ final : ਜਨਤਕ EmitCFunc {
    // METHODS
    ਖਾਲੀ emit_ਗ੍ਰਾਫ(Ast_Exec_ਗ੍ਰਾਫ* nodep) {
        ਅਚਲ std::string name = EmitCUtil::exec_ਗ੍ਰਾਫ_Name(nodep);
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ਅਚਲ AstCFunc*, ਅਚਲ ExecMTask*> mtaskps;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : nodep->depGraphp()->vertices()) {
            ਅਚਲ ExecMTask* ਅਚਲ mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ਅਚਲ AstCFunc*, uint32_t> indices;
        ਲਈ (ਅਚਲ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.ਮਾਪ());
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
        ਲਈ (ਅਚਲ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::ਮੰਜ਼ਿਲ_string(mtaskps.at(callp->funcp())->ਲਾਗਤ()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t n_ਕਿਨਾਰੇ = 0;
        ਲਈ (ਅਚਲ AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskps.at(callp->funcp())->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (!n_ਕਿਨਾਰੇ) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::ਮੰਜ਼ਿਲ_string(indices.at(callp->funcp())) + ", "
                     + std::ਮੰਜ਼ਿਲ_string(indices.at(ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_ਕਿਨਾਰੇ;
            }
        }
        ਜੇ (n_ਕਿਨਾਰੇ) puts("};\n");

        // The graph
        ਅਚਲ std::string ਕਿਨਾਰੇ_Name = n_ਕਿਨਾਰੇ ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::ਮੰਜ਼ਿਲ_string(indices.ਮਾਪ()) + ",\n");
        puts(ਕਿਨਾਰੇ_Name + ", " + std::ਮੰਜ਼ਿਲ_string(n_ਕਿਨਾਰੇ) + ",\n");
        puts(std::ਮੰਜ਼ਿਲ_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ਅਚਲ ਤਾਰਕਿਕ profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ Emit_CExec_ਗ੍ਰਾਫ(AstNetlist* netlistp) {
        std::ਸਦਿਸ਼<Ast_Exec_ਗ੍ਰਾਫ*> execGraphps;
        netlistp->ਸਿਖਰ_Modulep()->foreach(
            [&](Ast_Exec_ਗ੍ਰਾਫ* nodep) { execGraphps.ਧੱਕੋ_ਪਿੱਠ(nodep); });
        ਜੇ (execGraphps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        open_New_Output_ਸਰੋਤ_File(EmitCUtil::ਸਿਖਰ_Class_Name() + "__ExecGraph", ਸੱਚ, ਝੂਠ,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        ਲਈ (Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ nodep : execGraphps) emit_ਗ੍ਰਾਫ(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

ਖਾਲੀ V3EmitC::emitc_Exec_ਗ੍ਰਾਫ() {
    UINFO(2, __FUNCTION__ << ":");
    Emit_CExec_ਗ੍ਰਾਫ{v3Global.rootp()};
}
