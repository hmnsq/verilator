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
    for_Each_راس([&](Dfg_راس& vtx) { vtx.unlinkDelete(*هذا); });
}

فراغ Dfg_گراف::ملايو_Graphs(std::ويڪٽر<std::unique_ptr<Dfg_گراف>>&& otherps) {
    إذا (otherps.خالي_جايون()) أرجع;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ثابت VNUser2_اندريون_Use user2_اندريون_Use;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    لكل (Dfg_راس_Var& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    لكل (ثابت std::unique_ptr<Dfg_گراف>& otherp : otherps) {
        // Process variables
        لكل (Dfg_راس_Var* ثابت vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            إذا (Dfg_راس_Var* ثابت altp = vtxp->vscp()->user2u().منزل<Dfg_راس_Var*>()) {
                Dfg_راس* ثابت srcp = vtxp->srcp();
                Dfg_راس* ثابت defaultp = vtxp->defaultp();
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
#جيڪڏهن_وصف_ٿيل VL_DEBUG
            vtxp->m_dfgp = هذا;
#جيڪڏهن_پورو
        }
        m_varVertices.splice(m_varVertices.پڇاڙي(), otherp->m_varVertices);
        // Process Ast references
        لكل (Dfg_راس_Ast* ثابت vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#جيڪڏهن_وصف_ٿيل VL_DEBUG
            vtxp->m_dfgp = هذا;
#جيڪڏهن_پورو
        }
        m_astVertices.splice(m_astVertices.پڇاڙي(), otherp->m_astVertices);
        // Process constants
        لكل (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#جيڪڏهن_وصف_ٿيل VL_DEBUG
            vtx.m_dfgp = هذا;
#جيڪڏهن_پورو
        }
        m_constVertices.splice(m_constVertices.پڇاڙي(), otherp->m_constVertices);
        // Process operations
        لكل (Dfg_راس& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#جيڪڏهن_وصف_ٿيل VL_DEBUG
            vtx.m_dfgp = هذا;
#جيڪڏهن_پورو
        }
        m_opVertices.splice(m_opVertices.پڇاڙي(), otherp->m_opVertices);
        // Update graph sizes
        m_سائيز += otherp->m_سائيز;
        otherp->m_سائيز = 0;
    }
}

Dfg_راس_Var* Dfg_گراف::makeNewVar(File_سٽ* flp, ثابت std::string& اڳياڙي,
                                   ثابت Dfg_معلومات_Type& dtype, AstScope* scopep) {
    AstVarScope* ثابت vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), اڳياڙي);
    // Create and return the corresponding variable vertex
    إذا (dtype.isArray()) أرجع جديد DfgVarArray{*هذا, vscp};
    أرجع جديد DfgVarPacked{*هذا, vscp};
}

ساكن ثابت std::string منزل_Dot_Id(ثابت Dfg_راس& vtx) { أرجع '"' + cvt_منزل_Hex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ساكن فراغ dump_Dot_راس(std::ostream& os, ثابت Dfg_راس& vtx) {
    إذا (ثابت Dfg_راس_Var* ثابت varVtxp = vtx.cast<Dfg_راس_Var>()) {
        ثابت AstVarScope* ثابت vscp = varVtxp->vscp();
        os << منزل_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //پتو
        os << '\n' << cvt_منزل_Hex(varVtxp);
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
                                   : varVtxp->tmpForp()      ? "gray95"  //ناسي
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgPrev* ثابت prevVtxp = vtx.cast<DfgPrev>()) {
        ثابت AstVarScope* ثابت vscp = prevVtxp->vscp();
        os << منزل_Dot_Id(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        //پتو
        os << '\n' << cvt_منزل_Hex(prevVtxp);
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
        ثابت V3_عدد& انگ = constVtxp->انگ();

        os << منزل_Dot_Id(vtx);
        os << " [label=\"";
        إذا (انگ.ويڪر() <= 32 && !انگ.isSigned()) {
            os << constVtxp->ويڪر() << "'d" << انگ.منزل_UInt() << '\n';
            os << constVtxp->ويڪر() << "'h" << std::hex << انگ.منزل_UInt() << std::dec << '\n';
        } وإلا {
            os << انگ.ascii() << '\n';
        }
        os << cvt_منزل_Hex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgSel* ثابت selVtxp = vtx.cast<DfgSel>()) {
        ثابت uint32_t lsb = selVtxp->lsb();
        ثابت uint32_t msb = lsb + selVtxp->ويڪر() - 1;
        os << منزل_Dot_Id(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvt_منزل_Hex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        إذا (vtx.has_گڻو_Sinks()) {
            os << ", shape=doublecircle";
        } وإلا {
            os << ", shape=circle";
        }
        os << "]\n";
        أرجع;
    }

    إذا (vtx.is<Dfg_راس_Splice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << منزل_Dot_Id(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvt_منزل_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        إذا (vtx.has_گڻو_Sinks()) {
            os << ", shape=doubleoctagon";
        } وإلا {
            os << ", shape=octagon";
        }
        os << "]\n";
        أرجع;
    }

    إذا (ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) {
        os << منزل_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_وڻ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvt_منزل_Hex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت حرف* ثابت colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_منزل_Hex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        أرجع;
    }

    إذا (ثابت Dfg_راس_Ast* ثابت astVtxp = vtx.cast<Dfg_راس_Ast>()) {
        os << منزل_Dot_Id(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_وڻ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvt_منزل_Hex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت DfgAstRd* ثابت astRdVtxp = astVtxp->cast<DfgAstRd>();
        ثابت حرف* ثابت colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvt_منزل_Hex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->اندريون_Sen_شي()];
        os << "_L"[astRdVtxp->اندريون_Loop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        أرجع;
    }

    os << منزل_Dot_Id(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvt_منزل_Hex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    إذا (vtx.has_گڻو_Sinks()) {
        os << ", shape=doublecircle";
    } وإلا {
        os << ", shape=circle";
    }
    os << "]\n";
}

فراغ Dfg_گراف::dumpDot(std::ostream& os, ثابت std::string& label,
                       std::function<منطقي(ثابت Dfg_راس&)> p) ثابت {
    // This generates a graphviz dump, https://www.graphviz.org

    //سرو
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    إذا (!p) p = [](ثابت Dfg_راس&) { أرجع سچ; };

    std::unordered_سيٽ<ثابت Dfg_راس*> emitted;
    // Emit all vertices associated with a DfgLogic
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        إذا (!logicp) أرجع;
        إذا (logicp->synth().خالي_جايون()) أرجع;
        إذا (!p(vtx)) أرجع;
        os << "subgraph cluster_" << cvt_منزل_Hex(logicp) << " {\n";
        dump_Dot_راس(os, *logicp);
        emitted.وجهو(logicp);
        لكل (Dfg_راس* ثابت vtxp : logicp->synth()) {
            إذا (!p(*vtxp)) استمر;
            dump_Dot_راس(os, *vtxp);
            emitted.وجهو(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        إذا (emitted.ڳڻپ(&vtx)) أرجع;
        إذا (!p(vtx)) أرجع;
        dump_Dot_راس(os, vtx);
    });
    // Emit all edges
    for_Each_راس([&](ثابت Dfg_راس& vtx) {
        إذا (!p(vtx)) أرجع;
        لكل (سائيز_t i = 0; i < vtx.nInputs(); ++i) {
            Dfg_راس* ثابت srcp = vtx.inputp(i);
            إذا (!srcp) استمر;
            إذا (!p(*srcp)) استمر;
            os << منزل_Dot_Id(*srcp) << " -> " << منزل_Dot_Id(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.خالي_جايون() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string Dfg_گراف::dumpDotString(ثابت std::string& label,
                                    std::function<منطقي(ثابت Dfg_راس&)> p) ثابت {
    std::stringstream ss;
    dumpDot(ss, label, p);
    أرجع ss.str();
}

فراغ Dfg_گراف::dumpDotFile(ثابت std::string& filename, ثابت std::string& label,
                           std::function<منطقي(ثابت Dfg_راس&)> p) ثابت {
    ثابت std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    إذا (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

فراغ Dfg_گراف::dumpDotFilePrefixed(ثابت std::string& label,
                                   std::function<منطقي(ثابت Dfg_راس&)> p) ثابت {
    std::string filename = name();
    إذا (!label.خالي_جايون()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

سانچو <منطقي T_SinksNotSources>
ساكن std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>>
dfg_گراف_Collect_Cone(ثابت std::ويڪٽر<ثابت Dfg_راس*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::ويڪٽر<ثابت Dfg_راس*> قطار = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>> resp{
        جديد std::unordered_سيٽ<ثابت Dfg_راس*>{}};
    // Depth first traversal
    طالما (!قطار.خالي_جايون()) {
        // Pop next work item
        ثابت Dfg_راس* ثابت vtxp = قطار.پٺ();
        قطار.ڪڍو_پٺ();
        // Mark vertex as visited, move on if already visited
        إذا (!resp->وجهو(vtxp).ٻيو) استمر;
        // Enqueue all siblings of this vertex.
        إذا VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ثابت Dfg_راس& sink) {
                قطار.ڌڪو_پٺ(&sink);
                أرجع خطأ;
            });
        } وإلا {
            vtxp->foreach_ذريعو([&](ثابت Dfg_راس& src) {
                قطار.ڌڪو_پٺ(&src);
                أرجع خطأ;
            });
        }
    }
    //پورو
    أرجع resp;
}

std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>>
Dfg_گراف::ذريعو_Cone(ثابت std::ويڪٽر<ثابت Dfg_راس*>& vtxps) ثابت {
    أرجع dfg_گراف_Collect_Cone<خطأ>(vtxps);
}

std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>>
Dfg_گراف::sinkCone(ثابت std::ويڪٽر<ثابت Dfg_راس*>& vtxps) ثابت {
    أرجع dfg_گراف_Collect_Cone<سچ>(vtxps);
}

std::unique_ptr<std::unordered_سيٽ<ثابت Dfg_راس*>>
Dfg_گراف::neighborhood(ثابت std::ويڪٽر<ثابت Dfg_راس*>& vtxps, سائيز_t n) ثابت {
    // Neighborhood
    std::ويڪٽر<ثابت Dfg_راس*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_سيٽ<ثابت Dfg_راس*> res{vec.begin(), vec.پڇاڙي()};
    // Expand neihborhood by 'n' hops
    سائيز_t begin = 0;
    سائيز_t پڇاڙي = vec.سائيز();
    لكل (سائيز_t hops = 1; hops <= n; ++hops) {
        لكل (سائيز_t i = begin; i < پڇاڙي; ++i) {
            ثابت Dfg_راس* ثابت vtxp = vec[i];
            vtxp->foreachSink([&](ثابت Dfg_راس& dst) {
                إذا (res.وجهو(&dst).ٻيو) vec.ڌڪو_پٺ(&dst);
                أرجع خطأ;
            });
            vtxp->foreach_ذريعو([&](ثابت Dfg_راس& src) {
                إذا (res.وجهو(&src).ٻيو) vec.ڌڪو_پٺ(&src);
                أرجع خطأ;
            });
        }
        begin = پڇاڙي;
        پڇاڙي = vec.سائيز();
    }
    // Move out the results
    أرجع std::make_unique<std::unordered_سيٽ<ثابت Dfg_راس*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

Dfg_راس::Dfg_راس(Dfg_گراف& dfg, VDfgType type, File_سٽ* flp, ثابت Dfg_معلومات_Type& dt,
                     Dfg_ڪنارو* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.جوڙيو_راس(*هذا);
}

منطقي Dfg_راس::unsafe() ثابت {
    إذا (is<DfgMux>()) أرجع سچ;
    إذا (is<DfgArraySel>()) أرجع !as<DfgArraySel>()->bitp()->is<DfgConst>();
    أرجع خطأ;
}

فراغ Dfg_راس::typeCheck(ثابت Dfg_گراف& dfg) ثابت {

#وصف_ڪريو CHECK(cond, نياپو) \
    UASSERT_OBJ(cond, هذا, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << نياپو);
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
        ثابت Dfg_راس_Var& v = *as<Dfg_راس_Var>();
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
        ثابت Dfg_راس_Splice& v = *as<Dfg_راس_Splice>();
        v.foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_معلومات_Type::select(v.dtype(), lo, src.سائيز()), "driver");
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
        CHECK(v.سائيز() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        أرجع;
    }
    حالة VDfgType::Sel: {
        ثابت DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_معلومات_Type::select(v.srcp()->dtype(), v.lsb(), v.سائيز()), "sel");
        أرجع;
    }
    حالة VDfgType::ميلاپ_Masked: {
        ثابت Dfg_ميلاپ_Masked& v = *as<Dfg_ميلاپ_Masked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.سائيز() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<Dfg_راس_Var>(), "Match should be a variable");
        أرجع;
    }
    حالة VDfgType::Mux: {
        ثابت DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->سائيز() >= v.سائيز(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        أرجع;
    }
    حالة VDfgType::ArraySel: {
        ثابت DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        أرجع;
    }

    حالة VDfgType::جوڙيو_2:
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

    حالة VDfgType::سرڪايو_L:
    حالة VDfgType::سرڪايو_R:
    حالة VDfgType::سرڪايو_RS: {
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
        CHECK(v.سائيز() == v.rhsp()->سائيز() + v.lhsp()->سائيز(), "Concat result mismatch");
        أرجع;
    }

    حالة VDfgType::Rep: {
        ثابت DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.سائيز() % v.srcp()->سائيز() == 0, "Not a replicate");
        أرجع;
    }

    حالة VDfgType::وهڪرو_L:
    حالة VDfgType::وهڪرو_R: {
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
        CHECK(dtype() == Dfg_معلومات_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        أرجع;
    }

    حالة VDfgType::Extend:
    حالة VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->سائيز() < سائيز(), "Operand should be narrower");
        أرجع;
    }

    حالة VDfgType::LogAnd:
    حالة VDfgType::LogEq:
    حالة VDfgType::LogIf:
    حالة VDfgType::LogOr: {
        CHECK(dtype() == Dfg_معلومات_Type::packed(1), "Should be 1-bit");
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
        CHECK(dtype() == Dfg_معلومات_Type::packed(1), "Should be 1-bit");
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

    حالة VDfgType::ڳڻپ_Ones_2: {
        ثابت Dfg_ڳڻپ_Ones& v = *as<Dfg_ڳڻپ_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.سائيز() == 32U, "Should yield a 32-bit result");
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
#وصف_رد CHECK
}

uint32_t Dfg_راس::fanout() ثابت {
    uint32_t نتيجو = 0;
    foreachSink([&](ثابت Dfg_راس&) {
        ++نتيجو;
        أرجع خطأ;
    });
    أرجع نتيجو;
}

Dfg_راس_Var* Dfg_راس::get_نتيجو_Var() {
    // It's easy if the vertex is already a variable ...
    إذا (Dfg_راس_Var* ثابت varp = هذا->cast<Dfg_راس_Var>()) أرجع varp;

    // Inspect existing variables written by this vertex, and choose one
    Dfg_راس_Var* resp = خالي_اشارو;
    // cppcheck-has-bug-suppress constParameter
    هذا->foreachSink([&resp](Dfg_راس& sink) {
        Dfg_راس_Var* ثابت varp = sink.cast<Dfg_راس_Var>();
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
        ثابت منطقي res_Is_عارضي = resp->vscp()->varp()->is_عارضي();
        ثابت منطقي var_Is_عارضي = varp->vscp()->varp()->is_عارضي();
        إذا (res_Is_عارضي != var_Is_عارضي) {
            إذا (res_Is_عارضي) resp = varp;
            أرجع خطأ;
        }
        إذا (!resp->tmpForp() != !varp->tmpForp()) {
            إذا (resp->tmpForp()) resp = varp;
            أرجع خطأ;
        }

        // Prefer the earlier one in source order
        ثابت File_سٽ& oldFlp = *(resp->fileline());
        ثابت File_سٽ& newFlp = *(varp->fileline());
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

AstScope* Dfg_راس::scopep(ScopeCache& cache, منطقي try_نتيجو_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    إذا (ثابت Dfg_راس_Var* ثابت varp = هذا->cast<Dfg_راس_Var>()) {
        أرجع varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    إذا (try_نتيجو_Var) {
        إذا (ثابت Dfg_راس_Var* ثابت varp = هذا->get_نتيجو_Var()) {
            أرجع varp->vscp()->scopep();
        }
    }

    AstScope* ثابت rootp = v3Global.rootp()->چوٽي_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[هذا];
    إذا (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = خالي_اشارو;
        foreach_ذريعو([&](Dfg_راس& src) {
            AstScope* ثابت scp = src.scopep(cache, سچ);
            إذا (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                أرجع سچ;
            }
            أرجع خطأ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), هذا,
                "DfgVertex::scopep called on graph with circular operations");

    //پورو
    أرجع resultr;
}

فراغ Dfg_راس::unlinkDelete(Dfg_گراف& dfg) {
    // Unlink sink edges
    طالما (!m_sinks.خالي_جايون()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.remove_راس(*هذا);
    // Delete - this will unlink sources
    ميٽيو هذا;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<Dfg_راس*, uint32_t> Dfg_راس_Var::driver_جو_Range(uint32_t lo, uint32_t سائيز) {
    Dfg_راس* ثابت srcp = هذا->srcp();
    // Not driven at all
    إذا (!srcp) أرجع {خالي_اشارو, 0};
    // If volatile, can have other drivers
    إذا (isVolatile()) أرجع {خالي_اشارو, 0};
    // Don't inline CReset
    إذا (srcp->is<DfgCReset>()) أرجع {خالي_اشارو, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    Dfg_راس_Splice* ثابت splicep = srcp->cast<Dfg_راس_Splice>();
    إذا (!splicep) أرجع {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ثابت uint32_t hi = lo + سائيز - 1;
    Dfg_راس* driverp = خالي_اشارو;
    uint32_t driverLo = 0;
    منطقي useDefault = defaultp();
    splicep->foreachDriver([&](Dfg_راس& src, ثابت uint32_t dLo) {
        ثابت uint32_t dHi = dLo + src.سائيز() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        إذا (dLo <= hi && lo <= dHi) useDefault = خطأ;
        // If it does not cover the whole searched range, move on
        إذا (lo < dLo || dHi < hi) أرجع خطأ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        أرجع سچ;
    });

    // If a single driver covers the searched range, it is the one
    إذا (driverp) أرجع {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    إذا (useDefault) أرجع {defaultp(), lo};
    // Not driven by a single vertex
    أرجع {خالي_اشارو, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

صنف Dfg_نمونو_String final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_interned_لفظ_Widths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ثابت Dfg_راس*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ثابت Dfg_راس*, std::pair<uint32_t, uint32_t>> m_multiplicity_And_اونهائي;

    ساكن std::string منزل_Letters(سائيز_t قدر, منطقي هيٺاهون_Case = خطأ) {
        ثابت حرف بنياد = هيٺاهون_Case ? 'a' : 'A';
        std::string s;
        افعل { s += static_cast<حرف>(بنياد + قدر % 26); } طالما (قدر /= 26);
        أرجع s;
    }

    ثابت std::string& internConst(ثابت DfgConst& vtx) {
        ثابت خودڪار pair = m_internedConsts.emplace(vtx.انگ().ascii(خطأ), "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedConsts.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& internSelLsb(uint32_t قدر) {
        ثابت خودڪار pair = m_internedSelLsbs.emplace(قدر, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedSelLsbs.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& intern_لفظ_ويڪر(uint32_t قدر) {
        ثابت خودڪار pair = m_interned_لفظ_Widths.emplace(قدر, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_interned_لفظ_Widths.سائيز() - 1, سچ);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& intern_Wide_ويڪر(uint32_t قدر) {
        ثابت خودڪار pair = m_internedWideWidths.emplace(قدر, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedWideWidths.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& intern_راس(ثابت Dfg_راس& vtx) {
        ثابت خودڪار pair = m_internedVertices.emplace(&vtx, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedVertices.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    فراغ رڪارڊ_Multiplicity_And_اونهائي(ثابت Dfg_راس& vtx, uint32_t اونهائي) {
        std::pair<uint32_t, uint32_t>& قدر = m_multiplicity_And_اونهائي
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, اونهائي))
                                                   .پهريون->ٻيو;
        قدر.پهريون += 1;
        قدر.ٻيو = std::max(قدر.ٻيو, اونهائي);
        إذا (!اونهائي) أرجع;
        vtx.foreach_ذريعو([&](ثابت Dfg_راس& src) {
            رڪارڊ_Multiplicity_And_اونهائي(src, اونهائي - 1);
            أرجع خطأ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    فراغ render(ثابت Dfg_راس& vtx, uint32_t اونهائي, منطقي is_پاڙ_2 = سچ) {
        إذا (ثابت DfgConst* ثابت constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            إذا (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وإلا إذا (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } وإلا {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وإلا إذا (!is_پاڙ_2 && m_multiplicity_And_اونهائي.at(&vtx).پهريون > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << intern_راس(vtx);
        } وإلا إذا (!vtx.foreach_ذريعو([&](ثابت Dfg_راس&) { أرجع سچ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } وإلا إذا (اونهائي == 0) {
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
            vtx.foreach_ذريعو([&](ثابت Dfg_راس& src) {
                m_os << ' ';
                render(src, اونهائي - 1, خطأ);
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
            ثابت uint32_t ويڪر = vtx.سائيز();
            إذا (ويڪر == 1) {
                m_os << '1';
            } وإلا إذا (ويڪر <= VL_QUADSIZE) {
                m_os << intern_لفظ_ويڪر(ويڪر);
            } وإلا {
                m_os << intern_Wide_ويڪر(ويڪر);
            }
        }

        // Mark it if it has multiple sinks
        إذا (vtx.has_گڻو_Sinks()) m_os << '*';
    }

عام:
    Dfg_نمونو_String(std::ostream& os, ثابت Dfg_راس& vtx, uint32_t اونهائي)
        : m_os{os} {
        رڪارڊ_Multiplicity_And_اونهائي(vtx, اونهائي);
        render(vtx, اونهائي, خطأ);
        استعمال Pair = std::pair<std::string, ثابت Dfg_راس*>;
        std::ويڪٽر<Pair> vertices;
        لكل (ثابت خودڪار& pair : m_multiplicity_And_اونهائي) {
            إذا (pair.ٻيو.پهريون == 1) استمر;
            vertices.emplace_پٺ(intern_راس(*pair.پهريون), pair.پهريون);
        }
        std::ترتيب_ڏيو(vertices.begin(), vertices.پڇاڙي(), [](ثابت Pair& a, ثابت Pair& b) {  //
            أرجع a.پهريون < b.پهريون;
        });
        لكل (ثابت Pair& pair : vertices) {
            m_os << " | " << pair.پهريون << " is ";
            render(*pair.ٻيو, m_multiplicity_And_اونهائي.at(pair.ٻيو).ٻيو);
        }
    }
};

std::string Dfg_راس::نمونو_String(uint32_t اونهائي) ثابت {
    std::ostringstream oss;
    Dfg_نمونو_String{oss, *هذا, اونهائي};
    أرجع oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ضمن "V3Dfg__gen_visitor_defns.h"  // From ./astgen
