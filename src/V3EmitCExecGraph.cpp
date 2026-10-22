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

جماعت Emit_CExec_گراف final : عوامی EmitCFunc {
    // METHODS
    خالی emit_گراف(Ast_Exec_گراف* nodep) {
        ثابت std::string name = EmitCUtil::exec_گراف_Name(nodep);
        ثابت AstUnpackArrayDType* ثابت trigDtypep
            = VN_AS(nodep->triggerp()->varp()->dtypep()->skipRefp(), UnpackArrayDType);

        // MTask of each function, and the index of each function in the topological order
        std::unordered_map<ثابت AstCFunc*, ثابت ExecMTask*> mtaskps;
        برائے (ثابت V3_گراف_راس& vtx : nodep->depGraphp()->vertices()) {
            ثابت ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            mtaskps.emplace(mtaskp->funcp(), mtaskp);
        }
        std::unordered_map<ثابت AstCFunc*, uint32_t> indices;
        برائے (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            indices.emplace(callp->funcp(), indices.سائز());
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
        برائے (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            UASSERT_OBJ(maskp, callp, "MTask call without trigger mask");
            putns(callp, "{&" + funcNameProtect(callp->funcp()) + ", ");
            puts(std::مقصد_string(mtaskps.at(callp->funcp())->لاگت()) + ", ");
            iterateConst(maskp);
            puts(".data()},\n");
            maskp = VN_AS(maskp->nextp(), VarRef);
        }
        puts("};\n");

        // The dependencies between MTasks
        uint32_t n_کنارے = 0;
        برائے (ثابت AstCCall* callp = nodep->callsp(); callp;
             callp = VN_AS(callp->nextp(), CCall)) {
            برائے (ثابت V3_گراف_کنارہ& کنارہ : mtaskps.at(callp->funcp())->برآمد_کنارے()) {
                اگر (!n_کنارے) {
                    puts("\nstatic VL_CONSTINIT_CXX20 const VlExecGraph::Edge " + name
                         + "__edges[] = {\n");
                }
                puts("{" + std::مقصد_string(indices.at(callp->funcp())) + ", "
                     + std::مقصد_string(indices.at(کنارہ.چوٹی()->as<ExecMTask>()->funcp())) + "},\n");
                ++n_کنارے;
            }
        }
        اگر (n_کنارے) puts("};\n");

        // The graph
        ثابت std::string کنارے_Name = n_کنارے ? name + "__edges" : "nullptr";
        putns(nodep, "\nVL_CONSTINIT_CXX20 VlExecGraph " + name + "{\n");
        puts(name + "__vertices, " + std::مقصد_string(indices.سائز()) + ",\n");
        puts(کنارے_Name + ", " + std::مقصد_string(n_کنارے) + ",\n");
        puts(std::مقصد_string(trigDtypep->elementsConst()) + ",\n");
        iterateConst(nodep->triggerCostsp());
        puts(".data(),\n");
        puts("&" + funcNameProtect(nodep->runfuncp()) + ",\n");
        // Profiling measures the parallel execution, so never run serially
        ثابت منطقی profiling = v3Global.opt.profExec() || v3Global.opt.profPgo();
        puts(profiling ? "0" : "VL_THREADS_SERIAL_COST");
        puts("\n};\n");
    }

عوامی:
    صریح Emit_CExec_گراف(AstNetlist* netlistp) {
        std::سمتیہ<Ast_Exec_گراف*> execGraphps;
        netlistp->چوٹی_Modulep()->foreach(
            [&](Ast_Exec_گراف* nodep) { execGraphps.ڈالو_عقب(nodep); });
        اگر (execGraphps.خالی_خانے()) واپس;

        open_New_Output_ماخذ_File(EmitCUtil::چوٹی_Class_Name() + "__ExecGraph", سچ, جھوٹ,
                                "Exec graphs");
        puts("\n");
        puts("#include \"verilated.h\"\n");
        puts("#include \"verilated_threads.h\"\n");
        برائے (Ast_Exec_گراف* ثابت nodep : execGraphps) emit_گراف(nodep);
        closeOutputFile();
    }
};

//######################################################################
// EmitC static functions

خالی V3EmitC::emitc_Exec_گراف() {
    UINFO(2, __FUNCTION__ << ":");
    Emit_CExec_گراف{v3Global.rootp()};
}
