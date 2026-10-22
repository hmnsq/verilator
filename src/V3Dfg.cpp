// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Data flow graph (DFG) representation of logic
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"

#ضمن "V3Ast.h"
#ضمن "V3DfgContext.h"
#ضمن "V3EmitV.h"
#ضمن "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

Dfg_گراف::Dfg_گراف(V3DfgContext& ctx, ثابت string& name)
    : m_ctx{ctx}
    , m_name{name} {}

Dfg_گراف::~Dfg_گراف() {
    for_Each_رأس([&](Dfg_رأس& vtx) { vtx.unlinkDelete(*هذا); });
}

فراغ Dfg_گراف::ادغام_Graphs(std::وکتور<std::unique_ptr<Dfg_گراف>>&& otherps) {
    إذا (otherps.خالی()) أرجع;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ثابت VNUser2_درون_Use user2_درون_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    لكل (Dfg_رأس_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    لكل (ثابت std::unique_ptr<Dfg_گراف>& otherp : otherps) {
        // Process variables
        لكل (Dfg_رأس_Var* ثابت vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            إذا (Dfg_رأس_Var* ثابت altp = vtxp->vscp()->user2u().مقصد<Dfg_رأس_Var*>()) {
                Dfg_رأس* ثابت srcp = vtxp->srcp();
                Dfg_رأس* ثابت defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                إذا (srcp) altp->srcp(srcp);
                إذا (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                استمر;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#اگر_تعریف_شده VL_DEBUG
            vtxp->m_dfgp = هذا;
#پایان_اگر
        }
        m_varVertices.splice(m_varVertices.انتها(), otherp->m_varVertices);
        // Process Ast references
        لكل (Dfg_رأس_Ast* ثابت vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#اگر_تعریف_شده VL_DEBUG
            vtxp->m_dfgp = هذا;
#پایان_اگر
        }
        m_astVertices.splice(m_astVertices.انتها(), otherp->m_astVertices);
        // Process constants
        لكل (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#اگر_تعریف_شده VL_DEBUG
            vtx.m_dfgp = هذا;
#پایان_اگر
        }
        m_constVertices.splice(m_constVertices.انتها(), otherp->m_constVertices);
        // Process operations
        لكل (Dfg_رأس& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#اگر_تعریف_شده VL_DEBUG
            vtx.m_dfgp = هذا;
#پایان_اگر
        }
        m_opVertices.splice(m_opVertices.انتها(), otherp->m_opVertices);
        // Update graph sizes
        m_اندازه += otherp->m_اندازه;
        otherp->m_اندازه = 0;
    }
}

Dfg_رأس_Var* Dfg_گراف::makeNewVar(File_سطر* flp, ثابت std::string& پیشوند,
                                   ثابت Dfg_داده_Type& dtype, AstScope* scopep) {
    AstVarScope* ثابت vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), پیشوند);
    // Create and return the corresponding variable vertex
    إذا (dtype.isArray()) أرجع جديد DfgVarArray{*هذا, vscp};
    أرجع جديد DfgVarPacked{*هذا, vscp};
}

ساكن ثابت std::string مقصد_Dot_Id(ثابت Dfg_رأس& vtx) { أرجع '"' + cvt_مقصد_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ساكن فراغ dump_Dot_رأس(std::ostream& os, ثابت Dfg_رأس& vtx) {
    إذا (ثابت Dfg_رأس_Var* ثابت varVtxp = vtx.cast<Dfg_رأس_Var>()) {
        ثابت AstVarScope* ثابت vscp = varVtxp->vscp();
        os << مقصد_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //آدرس
        os << '\n' << cvt_مقصد_Hex(varVtxp);
        // Original variable, if any
        إذا (ثابت AstVarScope* ثابت tmpForp = varVtxp->tmpForp()) {
            إذا (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ساكن ثابت حرف* ثابت rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[خطأ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        إذا (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } وإلا إذا (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } وإلا {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ثابت حرف* ثابت colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  //خاکستری
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgPrev* ثابت prevVtxp = vtx.cast<DfgPrev>()) {
        ثابت AstVarScope* ثابت vscp = prevVtxp->vscp();
        os << مقصد_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //آدرس
        os << '\n' << cvt_مقصد_Hex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        إذا (prevVtxp->isPacked()) {
            os << ", shape=box";
        } وإلا إذا (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } وإلا {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ثابت حرف* ثابت colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgConst* ثابت constVtxp = vtx.cast<DfgConst>()) {
        ثابت V3_عدد& رقم = constVtxp->رقم();

        os << مقصد_Dot_Id(vtx);
        os << " [label=\"";
        إذا (رقم.عرض() <= 32 && !رقم.isSigned()) {
            os << constVtxp->عرض() << "'d" << رقم.مقصد_UInt() << '\n';
            os << constVtxp->عرض() << "'h" << std::hex << رقم.مقصد_UInt() << std::dec << '\n';
        } وإلا {
            os << رقم.ascii() << '\n';
        }
        os << cvt_مقصد_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgSel* ثابت selVtxp = vtx.cast<DfgSel>()) {
        ثابت uint32_t lsb = selVtxp->lsb();
        ثابت uint32_t msb = lsb + selVtxp->عرض() - 1;
        os << مقصد_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_مقصد_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        إذا (vtx.has_مضرب_Sinks()) {
            os << ", shape=doublecircle";
        } وإلا {
            os << ", shape=circle";
        }
        os << "]\n";
        أرجع;
    }

    إذا (vtx.is<Dfg_رأس_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << مقصد_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_مقصد_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        إذا (vtx.has_مضرب_Sinks()) {
            os << ", shape=doubleoctagon";
        } وإلا {
            os << ", shape=octagon";
        }
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) {
        os << مقصد_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_مقصد_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت حرف* ثابت colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_مقصد_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        أرجع;
    }

    إذا (ثابت Dfg_رأس_Ast* ثابت astVtxp = vtx.cast<Dfg_رأس_Ast>()) {
        os << مقصد_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_درخت(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_مقصد_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت DfgAstRd* ثابت astRdVtxp = astVtxp->cast<DfgAstRd>();
        ثابت حرف* ثابت colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_مقصد_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->درون_Sen_قلم()];
        os << "_L"[astRdVtxp->درون_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        أرجع;
    }

    os << مقصد_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_مقصد_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    إذا (vtx.has_مضرب_Sinks()) {
        os << ", shape=doublecircle";
    } وإلا {
        os << ", shape=circle";
    }
    os << "]\n";
}

فراغ Dfg_گراف::dumpDot(std::ostream& os, ثابت std::string& label,
                       std::function<منطقي(ثابت Dfg_رأس&)> p) ثابت {
    // This generates a graphviz dump, https://www.graphviz.org

    //سرآیند
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    إذا (!p) p = [](ثابت Dfg_رأس&) { أرجع درست; };

    std::unordered_مجموعه<ثابت Dfg_رأس*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
        ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        إذا (!logicp) أرجع;
        إذا (logicp->synth().خالی()) أرجع;
        إذا (!p(vtx)) أرجع;
        os << "subgraph cluster_" << cvt_مقصد_Hex(logicp) << " {\n";
        dump_Dot_رأس(os, *logicp);
        emitted.درج(logicp);
        لكل (Dfg_رأس* ثابت vtxp : logicp->synth()) {
            إذا (!p(*vtxp)) استمر;
            dump_Dot_رأس(os, *vtxp);
            emitted.درج(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
        إذا (emitted.شمار(&vtx)) أرجع;
        إذا (!p(vtx)) أرجع;
        dump_Dot_رأس(os, vtx);
    });
    // Emit all edges
    for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
        إذا (!p(vtx)) أرجع;
        لكل (اندازه_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_رأس* ثابت srcp = vtx.inputp(i);
            إذا (!srcp) استمر;
            إذا (!p(*srcp)) استمر;
            os << مقصد_Dot_Id(*srcp) << " -> " << مقصد_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.خالی() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_گراف::dumpDotString(ثابت std::string& label,
                                    std::function<منطقي(ثابت Dfg_رأس&)> p) ثابت {
    std::stringstream ss;
    dumpDot(ss, label, p);
    أرجع ss.str();
}

فراغ Dfg_گراف::dumpDotFile(ثابت std::string& filename, ثابت std::string& label,
                           std::function<منطقي(ثابت Dfg_رأس&)> p) ثابت {
    ثابت std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    إذا (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

فراغ Dfg_گراف::dumpDotFilePrefixed(ثابت std::string& label,
                                   std::function<منطقي(ثابت Dfg_رأس&)> p) ثابت {
    std::string filename = name();
    إذا (!label.خالی()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

قالب <منطقي T_SinksNotSources>
ساكن std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>>
dfg_گراف_Collect_Cone(ثابت std::وکتور<ثابت Dfg_رأس*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::وکتور<ثابت Dfg_رأس*> صف = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>> resp{
        جديد std::unordered_مجموعه<ثابت Dfg_رأس*>{}};
    // Depth first traversal
    طالما (!صف.خالی()) {
        // Pop next work item
        ثابت Dfg_رأس* ثابت vtxp = صف.پشت();
        صف.برداشت_پشت();
        // Mark vertex as visited, move on if already visited
        إذا (!resp->درج(vtxp).دومی) استمر;
        // Enqueue all siblings of this vertex.
        إذا VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ثابت Dfg_رأس& sink) {
                صف.افزودن_پشت(&sink);
                أرجع خطأ;
            });
        } وإلا {
            vtxp->foreach_منبع([&](ثابت Dfg_رأس& src) {
                صف.افزودن_پشت(&src);
                أرجع خطأ;
            });
        }
    }
    //انجامشده
    أرجع resp;
}

std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>>
Dfg_گراف::منبع_Cone(ثابت std::وکتور<ثابت Dfg_رأس*>& vtxps) ثابت {
    أرجع dfg_گراف_Collect_Cone<خطأ>(vtxps);
}

std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>>
Dfg_گراف::sinkCone(ثابت std::وکتور<ثابت Dfg_رأس*>& vtxps) ثابت {
    أرجع dfg_گراف_Collect_Cone<درست>(vtxps);
}

std::unique_ptr<std::unordered_مجموعه<ثابت Dfg_رأس*>>
Dfg_گراف::neighborhood(ثابت std::وکتور<ثابت Dfg_رأس*>& vtxps, اندازه_t n) ثابت {
    // Neighborhood
    std::وکتور<ثابت Dfg_رأس*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_مجموعه<ثابت Dfg_رأس*> res{vec.begin(), vec.انتها()};
    // Expand neihborhood by 'n' hops
    اندازه_t begin = 0;
    اندازه_t انتها = vec.اندازه();
    لكل (اندازه_t hops = 1; hops <= n; ++hops) {
        لكل (اندازه_t i = begin; i < انتها; ++i) {
            ثابت Dfg_رأس* ثابت vtxp = vec[i];
            vtxp->foreachSink([&](ثابت Dfg_رأس& dst) {
                إذا (res.درج(&dst).دومی) vec.افزودن_پشت(&dst);
                أرجع خطأ;
            });
            vtxp->foreach_منبع([&](ثابت Dfg_رأس& src) {
                إذا (res.درج(&src).دومی) vec.افزودن_پشت(&src);
                أرجع خطأ;
            });
        }
        begin = انتها;
        انتها = vec.اندازه();
    }
    // Move out the results
    أرجع std::make_unique<std::unordered_مجموعه<ثابت Dfg_رأس*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_رأس::Dfg_رأس(Dfg_گراف& dfg, VDfgType type, File_سطر* flp, ثابت Dfg_داده_Type& dt,
                     Dfg_یال* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.جمع_کن_رأس(*هذا);
}

منطقي Dfg_رأس::unsafe() ثابت {
    إذا (is<DfgMux>()) أرجع درست;
    إذا (is<DfgArraySel>()) أرجع !as<DfgArraySel>()->bitp()->is<DfgConst>();
    أرجع خطأ;
}

فراغ Dfg_رأس::typeCheck(ثابت Dfg_گراف& dfg) ثابت {

#تعریف_کن CHECK(cond, پیام_2) \
    UASSERT_OBJ(cond, هذا, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << پیام_2);
    بدل (type()) {
    حالة VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        أرجع;
    }
    حالة VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        أرجع;
    }
    حالة VDfgType::AstRd: {
        ثابت DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        أرجع;
    }
    حالة VDfgType::VarArray:
    حالة VDfgType::VarPacked: {
        ثابت Dfg_رأس_Var& v = *as<Dfg_رأس_Var>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        أرجع;
    }
    حالة VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        أرجع;
    }
    حالة VDfgType::SpliceArray:
    حالة VDfgType::SplicePacked: {
        ثابت Dfg_رأس_Splice& v = *as<Dfg_رأس_Splice>();
        v.foreachDriver([&](ثابت Dfg_رأس& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_داده_Type::select(v.dtype(), lo, src.اندازه()), "driver");
            أرجع خطأ;
        });
        أرجع;
    }
    حالة VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        أرجع;
    }
    حالة VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        أرجع;
    }
    حالة VDfgType::UnitArray: {
        ثابت DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.اندازه() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        أرجع;
    }
    حالة VDfgType::Sel: {
        ثابت DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_داده_Type::select(v.srcp()->dtype(), v.lsb(), v.اندازه()), "sel");
        أرجع;
    }
    حالة VDfgType::تطابق_Masked: {
        ثابت Dfg_تطابق_Masked& v = *as<Dfg_تطابق_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.اندازه() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_رأس_Var>(), "Match should be a variable");
        أرجع;
    }
    حالة VDfgType::Mux: {
        ثابت DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->اندازه() >= v.اندازه(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        أرجع;
    }
    حالة VDfgType::ArraySel: {
        ثابت DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        أرجع;
    }

    حالة VDfgType::جمع_کن_2:
    حالة VDfgType::And:
    حالة VDfgType::Div:
    حالة VDfgType::DivS:
    حالة VDfgType::ModDiv:
    حالة VDfgType::ModDivS:
    حالة VDfgType::Mul:
    حالة VDfgType::MulS:
    حالة VDfgType::Or:
    حالة VDfgType::Sub:
    حالة VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        أرجع;
    }

    حالة VDfgType::Negate:
    حالة VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        أرجع;
    }

    حالة VDfgType::لغزش_L:
    حالة VDfgType::لغزش_R:
    حالة VDfgType::لغزش_RS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        أرجع;
    }

    حالة VDfgType::Concat: {
        ثابت DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.اندازه() == v.rhsp()->اندازه() + v.lhsp()->اندازه(), "Concat result mismatch");
        أرجع;
    }

    حالة VDfgType::Rep: {
        ثابت DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.اندازه() % v.srcp()->اندازه() == 0, "Not a replicate");
        أرجع;
    }

    حالة VDfgType::جریان_L:
    حالة VDfgType::جریان_R: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        أرجع;
    }

    حالة VDfgType::Eq:
    حالة VDfgType::EqCase:
    حالة VDfgType::EqWild:
    حالة VDfgType::Neq:
    حالة VDfgType::NeqCase:
    حالة VDfgType::NeqWild:
    حالة VDfgType::Gt:
    حالة VDfgType::GtS:
    حالة VDfgType::Gte:
    حالة VDfgType::GteS:
    حالة VDfgType::Lt:
    حالة VDfgType::LtS:
    حالة VDfgType::Lte:
    حالة VDfgType::LteS: {
        CHECK(dtype() == Dfg_داده_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        أرجع;
    }

    حالة VDfgType::Extend:
    حالة VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->اندازه() < اندازه(), "Operand should be narrower");
        أرجع;
    }

    حالة VDfgType::LogAnd:
    حالة VDfgType::LogEq:
    حالة VDfgType::LogIf:
    حالة VDfgType::LogOr: {
        CHECK(dtype() == Dfg_داده_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        أرجع;
    }

    حالة VDfgType::LogNot:
    حالة VDfgType::RedAnd:
    حالة VDfgType::RedOr:
    حالة VDfgType::RedXor:
    حالة VDfgType::OneHot:
    حالة VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_داده_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        أرجع;
    }

    حالة VDfgType::Cond: {
        ثابت DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        أرجع;
    }

    حالة VDfgType::شمار_Ones_2: {
        ثابت Dfg_شمار_Ones& v = *as<Dfg_شمار_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.اندازه() == 32U, "Should yield a 32-bit result");
        أرجع;
    }

    حالة VDfgType::Pow:
    حالة VDfgType::PowSS:
    حالة VDfgType::PowSU:
    حالة VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        أرجع;
    }
    }
#لغو_تعریف CHECK
}

uint32_t Dfg_رأس::fanout() ثابت {
    uint32_t نتیجه = 0;
    foreachSink([&](ثابت Dfg_رأس&) {
        ++نتیجه;
        أرجع خطأ;
    });
    أرجع نتیجه;
}

Dfg_رأس_Var* Dfg_رأس::get_نتیجه_Var() {
    // It's easy if the vertex is already a variable ...
    إذا (Dfg_رأس_Var* ثابت varp = هذا->cast<Dfg_رأس_Var>()) أرجع varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_رأس_Var* resp = اشاره_تهی;
    // cppcheck-has-bug-suppress constParameter
    هذا->foreachSink([&resp](Dfg_رأس& sink) {
        Dfg_رأس_Var* ثابت varp = sink.cast<Dfg_رأس_Var>();
        إذا (!varp) أرجع خطأ;

        // Do not use it if value might differ from its drivers
        إذا (varp->isVolatile()) أرجع خطأ;

        // First variable found
        إذا (!resp) {
            resp = varp;
            أرجع خطأ;
        }

        // Prefer those variables that must be kept anyway
        إذا (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            إذا (!resp->hasExtRdRefs()) resp = varp;
            أرجع خطأ;
        }
        إذا (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            إذا (!resp->hasDfgRefs()) resp = varp;
            أرجع خطأ;
        }

        // Prefer real variables over temporaries
        ثابت منطقي res_Is_موقت = resp->vscp()->varp()->is_موقت();
        ثابت منطقي var_Is_موقت = varp->vscp()->varp()->is_موقت();
        إذا (res_Is_موقت != var_Is_موقت) {
            إذا (res_Is_موقت) resp = varp;
            أرجع خطأ;
        }
        إذا (!resp->tmpForp() != !varp->tmpForp()) {
            إذا (resp->tmpForp()) resp = varp;
            أرجع خطأ;
        }

        // Prefer the earlier one in source order
        ثابت File_سطر& oldFlp = *(resp->fileline());
        ثابت File_سطر& newFlp = *(varp->fileline());
        إذا (ثابت صحيح cmp = oldFlp.operatorCompare(newFlp)) {
            إذا (cmp > 0) resp = varp;
            أرجع خطأ;
        }
        // Prefer the one with the lexically smaller name
        إذا (ثابت صحيح cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            إذا (cmp > 0) resp = varp;
            أرجع خطأ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        أرجع خطأ;
    });
    أرجع resp;
}

AstScope* Dfg_رأس::scopep(ScopeCache& cache, منطقي try_نتیجه_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    إذا (ثابت Dfg_رأس_Var* ثابت varp = هذا->cast<Dfg_رأس_Var>()) {
        أرجع varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    إذا (try_نتیجه_Var) {
        إذا (ثابت Dfg_رأس_Var* ثابت varp = هذا->get_نتیجه_Var()) {
            أرجع varp->vscp()->scopep();
        }
    }

    AstScope* ثابت rootp = v3Global.rootp()->بالاترین_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[هذا];
    إذا (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = اشاره_تهی;
        foreach_منبع([&](Dfg_رأس& src) {
            AstScope* ثابت scp = src.scopep(cache, درست);
            إذا (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                أرجع درست;
            }
            أرجع خطأ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), هذا,
                "DfgVertex::scopep called on graph with circular operations");

    //انجامشده
    أرجع resultr;
}

فراغ Dfg_رأس::unlinkDelete(Dfg_گراف& dfg) {
    // Unlink sink edges
    طالما (!m_sinks.خالی()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_رأس(*هذا);
    // Delete - this will unlink sources
    حذف هذا;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_رأس*, uint32_t> Dfg_رأس_Var::driver_از_Range(uint32_t lo, uint32_t اندازه) {
    Dfg_رأس* ثابت srcp = هذا->srcp();
    // Not driven at all
    إذا (!srcp) أرجع {اشاره_تهی, 0};
    // If volatile, can have other drivers
    إذا (isVolatile()) أرجع {اشاره_تهی, 0};
    // Don't inline CReset
    إذا (srcp->is<DfgCReset>()) أرجع {اشاره_تهی, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_رأس_Splice* ثابت splicep = srcp->cast<Dfg_رأس_Splice>();
    إذا (!splicep) أرجع {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ثابت uint32_t hi = lo + اندازه - 1;
    Dfg_رأس* driverp = اشاره_تهی;
    uint32_t driverLo = 0;
    منطقي useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_رأس& src, ثابت uint32_t dLo) {
        ثابت uint32_t dHi = dLo + src.اندازه() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        إذا (dLo <= hi && lo <= dHi) useDefault = خطأ;
        // If it does not cover the whole searched range, move on
        إذا (lo < dLo || dHi < hi) أرجع خطأ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        أرجع درست;
    });

    // If a single driver covers the searched range, it is the one
    إذا (driverp) أرجع {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    إذا (useDefault) أرجع {defaultp(), lo};
    // Not driven by a single vertex
    أرجع {اشاره_تهی, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

صنف Dfg_الگو_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_کلمه_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ثابت Dfg_رأس*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ثابت Dfg_رأس*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_عمق;

    ساكن std::string مقصد_Letters(اندازه_t مقدار, منطقي پایینی_Case = خطأ) {
        ثابت حرف پایه = پایینی_Case ? 'a' : 'A';
        std::string s;
        افعل { s += static_cast<حرف>(پایه + مقدار % 26); } طالما (مقدار /= 26);
        أرجع s;
    }

    ثابت std::string& internConst(ثابت DfgConst& vtx) {
        ثابت خودکار pair = m_internedConsts.emplace(vtx.رقم().ascii(خطأ), "");
        إذا (pair.دومی) pair.اولی->دومی += مقصد_Letters(m_internedConsts.اندازه() - 1);
        أرجع pair.اولی->دومی;
    }

    ثابت std::string& internSelLsb(uint32_t مقدار) {
        ثابت خودکار pair = m_internedSelLsbs.emplace(مقدار, "");
        إذا (pair.دومی) pair.اولی->دومی += مقصد_Letters(m_internedSelLsbs.اندازه() - 1);
        أرجع pair.اولی->دومی;
    }

    ثابت std::string& intern_کلمه_عرض(uint32_t مقدار) {
        ثابت خودکار pair = m_interned_کلمه_Widths.emplace(مقدار, "");
        إذا (pair.دومی) pair.اولی->دومی += مقصد_Letters(m_interned_کلمه_Widths.اندازه() - 1, درست);
        أرجع pair.اولی->دومی;
    }

    ثابت std::string& intern_Wide_عرض(uint32_t مقدار) {
        ثابت خودکار pair = m_internedWideWidths.emplace(مقدار, "");
        إذا (pair.دومی) pair.اولی->دومی += مقصد_Letters(m_internedWideWidths.اندازه() - 1);
        أرجع pair.اولی->دومی;
    }

    ثابت std::string& intern_رأس(ثابت Dfg_رأس& vtx) {
        ثابت خودکار pair = m_internedVertices.emplace(&vtx, "");
        إذا (pair.دومی) pair.اولی->دومی += مقصد_Letters(m_internedVertices.اندازه() - 1);
        أرجع pair.اولی->دومی;
    }

    فراغ رکورد_Multiplicity_And_عمق(ثابت Dfg_رأس& vtx, uint32_t عمق) {
        std::pair<uint32_t, uint32_t>& مقدار = m_multiplicity_And_عمق
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, عمق))
                                                   .اولی->دومی;
        مقدار.اولی += 1;
        مقدار.دومی = std::max(مقدار.دومی, عمق);
        إذا (!عمق) أرجع;
        vtx.foreach_منبع([&](ثابت Dfg_رأس& src) {
            رکورد_Multiplicity_And_عمق(src, عمق - 1);
            أرجع خطأ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    فراغ render(ثابت Dfg_رأس& vtx, uint32_t عمق, منطقي is_ریشه_2 = درست) {
        إذا (ثابت DfgConst* ثابت constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            إذا (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وإلا إذا (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } وإلا {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وإلا إذا (!is_ریشه_2 && m_multiplicity_And_عمق.at(&vtx).اولی > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_رأس(vtx);
        } وإلا إذا (!vtx.foreach_منبع([&](ثابت Dfg_رأس&) { أرجع درست; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } وإلا إذا (عمق == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } وإلا {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            إذا (ثابت DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                إذا (selp->lsb() == 0) {
                    m_os << '0';
                } وإلا {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreach_منبع([&](ثابت Dfg_رأس& src) {
                m_os << ' ';
                render(src, عمق - 1, خطأ);
                أرجع خطأ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        إذا (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } وإلا {
            ثابت uint32_t عرض = vtx.اندازه();
            إذا (عرض == 1) {
                m_os << '1';
            } وإلا إذا (عرض <= VL_QUADSIZE) {
                m_os << intern_کلمه_عرض(عرض);
            } وإلا {
                m_os << intern_Wide_عرض(عرض);
            }
        }

        // Mark it if it has multiple sinks
        إذا (vtx.has_مضرب_Sinks()) m_os << '*';
    }

عام:
    Dfg_الگو_String(std::ostream& os, ثابت Dfg_رأس& vtx, uint32_t عمق)
        : m_os{os} {
        رکورد_Multiplicity_And_عمق(vtx, عمق);
        render(vtx, عمق, خطأ);
        استفاده Pair = std::pair<std::string, ثابت Dfg_رأس*>;
        std::وکتور<Pair> vertices;
        لكل (ثابت خودکار& pair : m_multiplicity_And_عمق) {
            إذا (pair.دومی.اولی == 1) استمر;
            vertices.emplace_پشت(intern_رأس(*pair.اولی), pair.اولی);
        }
        std::مرتب_سازی(vertices.begin(), vertices.انتها(), [](ثابت Pair& a, ثابت Pair& b) {  //
            أرجع a.اولی < b.اولی;
        });
        لكل (ثابت Pair& pair : vertices) {
            m_os << " | " << pair.اولی << " is ";
            render(*pair.دومی, m_multiplicity_And_عمق.at(pair.دومی).دومی);
        }
    }
};

std::string Dfg_رأس::الگو_String(uint32_t عمق) ثابت {
    std::ostringstream oss;
    Dfg_الگو_String{oss, *هذا, عمق};
    أرجع oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ضمن "V3Dfg__gen_visitor_defns.h"  // From ./astgen
