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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3DfgContext.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ধ্রুবক string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*এই); });
}

শূন্য DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    যদি (otherps.empty()) ফেরত;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ধ্রুবক VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    জন্য (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    জন্য (ধ্রুবক std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        জন্য (DfgVertexVar* ধ্রুবক vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            যদি (DfgVertexVar* ধ্রুবক altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ধ্রুবক srcp = vtxp->srcp();
                DfgVertex* ধ্রুবক defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                যদি (srcp) altp->srcp(srcp);
                যদি (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                চালু;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এই;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        জন্য (DfgVertexAst* ধ্রুবক vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এই;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        জন্য (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এই;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        জন্য (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এই;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_আকার += otherp->m_আকার;
        otherp->m_আকার = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_সারি* flp, ধ্রুবক std::string& prefix,
                                   ধ্রুবক Dfg_তথ্য_Type& dtype, AstScope* scopep) {
    AstVarScope* ধ্রুবক vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    যদি (dtype.isArray()) ফেরত নতুন DfgVarArray{*এই, vscp};
    ফেরত নতুন DfgVarPacked{*এই, vscp};
}

স্থির ধ্রুবক std::string toDotId(ধ্রুবক DfgVertex& vtx) { ফেরত '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
স্থির শূন্য dumpDotVertex(std::ostream& os, ধ্রুবক DfgVertex& vtx) {
    যদি (ধ্রুবক DfgVertexVar* ধ্রুবক varVtxp = vtx.cast<DfgVertexVar>()) {
        ধ্রুবক AstVarScope* ধ্রুবক vscp = varVtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(varVtxp);
        // Original variable, if any
        যদি (ধ্রুবক AstVarScope* ধ্রুবক tmpForp = varVtxp->tmpForp()) {
            যদি (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        স্থির ধ্রুবক অক্ষর* ধ্রুবক rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[মিথ্যা][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        যদি (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } অন্যথা যদি (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } অন্যথা {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ধ্রুবক অক্ষর* ধ্রুবক colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ফেরত;
    }

    যদি (ধ্রুবক Dfg_পূর্ব* ধ্রুবক পূর্ব_Vtxp = vtx.cast<Dfg_পূর্ব>()) {
        ধ্রুবক AstVarScope* ধ্রুবক vscp = পূর্ব_Vtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(পূর্ব_Vtxp);
        // Type and fanout
        os << '\n';
        পূর্ব_Vtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << পূর্ব_Vtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        যদি (পূর্ব_Vtxp->isPacked()) {
            os << ", shape=box";
        } অন্যথা যদি (পূর্ব_Vtxp->isArray()) {
            os << ", shape=box3d";
        } অন্যথা {
            পূর্ব_Vtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ধ্রুবক অক্ষর* ধ্রুবক colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ফেরত;
    }

    যদি (ধ্রুবক DfgConst* ধ্রুবক constVtxp = vtx.cast<DfgConst>()) {
        ধ্রুবক V3_সংখ্যা& অঙ্ক = constVtxp->অঙ্ক();

        os << toDotId(vtx);
        os << " [label=\"";
        যদি (অঙ্ক.width() <= 32 && !অঙ্ক.isSigned()) {
            os << constVtxp->width() << "'d" << অঙ্ক.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << অঙ্ক.toUInt() << std::dec << '\n';
        } অন্যথা {
            os << অঙ্ক.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ফেরত;
    }

    যদি (ধ্রুবক DfgSel* ধ্রুবক selVtxp = vtx.cast<DfgSel>()) {
        ধ্রুবক uint32_t lsb = selVtxp->lsb();
        ধ্রুবক uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        যদি (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } অন্যথা {
            os << ", shape=circle";
        }
        os << "]\n";
        ফেরত;
    }

    যদি (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        যদি (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } অন্যথা {
            os << ", shape=octagon";
        }
        os << "]\n";
        ফেরত;
    }

    যদি (ধ্রুবক DfgLogic* ধ্রুবক logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুবক অক্ষর* ধ্রুবক colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
                                   : logicp->nonSynthesizable()    ? "#ffd0d0"  // Pale Red
                                   : logicp->reverted()            ? "#faffd0"  // Pale Yellow
                                                                   : "#d0ffd0";  // Pale Green

        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvtToHex(&vtx);
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ফেরত;
    }

    যদি (ধ্রুবক DfgVertexAst* ধ্রুবক astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুবক DfgAstRd* ধ্রুবক astRdVtxp = astVtxp->cast<DfgAstRd>();
        ধ্রুবক অক্ষর* ধ্রুবক colorp = astRdVtxp ? "#80ff80"  // Green
                                             : "#ffff80";  // Yellow
        os << " [label=\"";
        os << str;
        os << '\n' << vtx.typeName() << '\n' << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / ";
        os << "_S"[astRdVtxp->inSenItem()];
        os << "_L"[astRdVtxp->inLoop()];
        os << "\"\n";
        os << ", shape=box, style=\"rounded,filled\", nojustify=true";
        os << ", fillcolor=\"" << colorp << "\"";
        os << "]\n";
        ফেরত;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    যদি (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } অন্যথা {
        os << ", shape=circle";
    }
    os << "]\n";
}

শূন্য DfgGraph::dumpDot(std::ostream& os, ধ্রুবক std::string& label,
                       std::function<যৌক্তিক(ধ্রুবক DfgVertex&)> p) ধ্রুবক {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    যদি (!p) p = [](ধ্রুবক DfgVertex&) { ফেরত সত্য; };

    std::unordered_set<ধ্রুবক DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
        ধ্রুবক DfgLogic* ধ্রুবক logicp = vtx.cast<DfgLogic>();
        যদি (!logicp) ফেরত;
        যদি (logicp->synth().empty()) ফেরত;
        যদি (!p(vtx)) ফেরত;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ঢোকাও(logicp);
        জন্য (DfgVertex* ধ্রুবক vtxp : logicp->synth()) {
            যদি (!p(*vtxp)) চালু;
            dumpDotVertex(os, *vtxp);
            emitted.ঢোকাও(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
        যদি (emitted.গুনতি(&vtx)) ফেরত;
        যদি (!p(vtx)) ফেরত;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
        যদি (!p(vtx)) ফেরত;
        জন্য (আকার_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ধ্রুবক srcp = vtx.inputp(i);
            যদি (!srcp) চালু;
            যদি (!p(*srcp)) চালু;
            os << toDotId(*srcp) << " -> " << toDotId(vtx);
            os << " [headlabel=\"" << vtx.srcName(i) << "\"]";
            os << '\n';
        }
    });

    // Footer
    os << "label=\"" << name() + (label.empty() ? "" : "-" + label) << "\"\n";
    os << "labelloc=t\n";
    os << "labeljust=l\n";
    os << "}\n";
}

std::string DfgGraph::dumpDotString(ধ্রুবক std::string& label,
                                    std::function<যৌক্তিক(ধ্রুবক DfgVertex&)> p) ধ্রুবক {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ফেরত ss.str();
}

শূন্য DfgGraph::dumpDotFile(ধ্রুবক std::string& filename, ধ্রুবক std::string& label,
                           std::function<যৌক্তিক(ধ্রুবক DfgVertex&)> p) ধ্রুবক {
    ধ্রুবক std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    যদি (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

শূন্য DfgGraph::dumpDotFilePrefixed(ধ্রুবক std::string& label,
                                   std::function<যৌক্তিক(ধ্রুবক DfgVertex&)> p) ধ্রুবক {
    std::string filename = name();
    যদি (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <যৌক্তিক T_SinksNotSources>
স্থির std::unique_ptr<std::unordered_set<ধ্রুবক DfgVertex*>>
dfgGraphCollectCone(ধ্রুবক std::vector<ধ্রুবক DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ধ্রুবক DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ধ্রুবক DfgVertex*>> resp{
        নতুন std::unordered_set<ধ্রুবক DfgVertex*>{}};
    // Depth first traversal
    যখন (!queue.empty()) {
        // Pop next work item
        ধ্রুবক DfgVertex* ধ্রুবক vtxp = queue.back();
        queue.বের_করো_back();
        // Mark vertex as visited, move on if already visited
        যদি (!resp->ঢোকাও(vtxp).second) চালু;
        // Enqueue all siblings of this vertex.
        যদি VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ধ্রুবক DfgVertex& sink) {
                queue.ঠেলো_back(&sink);
                ফেরত মিথ্যা;
            });
        } অন্যথা {
            vtxp->foreachSource([&](ধ্রুবক DfgVertex& src) {
                queue.ঠেলো_back(&src);
                ফেরত মিথ্যা;
            });
        }
    }
    // Done
    ফেরত resp;
}

std::unique_ptr<std::unordered_set<ধ্রুবক DfgVertex*>>
DfgGraph::sourceCone(ধ্রুবক std::vector<ধ্রুবক DfgVertex*>& vtxps) ধ্রুবক {
    ফেরত dfgGraphCollectCone<মিথ্যা>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুবক DfgVertex*>>
DfgGraph::sinkCone(ধ্রুবক std::vector<ধ্রুবক DfgVertex*>& vtxps) ধ্রুবক {
    ফেরত dfgGraphCollectCone<সত্য>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুবক DfgVertex*>>
DfgGraph::neighborhood(ধ্রুবক std::vector<ধ্রুবক DfgVertex*>& vtxps, আকার_t n) ধ্রুবক {
    // Neighborhood
    std::vector<ধ্রুবক DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ধ্রুবক DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    আকার_t begin = 0;
    আকার_t end = vec.আকার_2();
    জন্য (আকার_t hops = 1; hops <= n; ++hops) {
        জন্য (আকার_t i = begin; i < end; ++i) {
            ধ্রুবক DfgVertex* ধ্রুবক vtxp = vec[i];
            vtxp->foreachSink([&](ধ্রুবক DfgVertex& dst) {
                যদি (res.ঢোকাও(&dst).second) vec.ঠেলো_back(&dst);
                ফেরত মিথ্যা;
            });
            vtxp->foreachSource([&](ধ্রুবক DfgVertex& src) {
                যদি (res.ঢোকাও(&src).second) vec.ঠেলো_back(&src);
                ফেরত মিথ্যা;
            });
        }
        begin = end;
        end = vec.আকার_2();
    }
    // Move out the results
    ফেরত std::make_unique<std::unordered_set<ধ্রুবক DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*এই);
}

যৌক্তিক DfgVertex::unsafe() ধ্রুবক {
    যদি (is<DfgMux>()) ফেরত সত্য;
    যদি (is<DfgArraySel>()) ফেরত !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ফেরত মিথ্যা;
}

শূন্য DfgVertex::typeCheck(ধ্রুবক DfgGraph& dfg) ধ্রুবক {

#define CHECK(cond, বার্তা) \
    UASSERT_OBJ(cond, এই, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << বার্তা);
    নির্বাচন (type()) {
    ক্ষেত্র VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::AstRd: {
        ধ্রুবক DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::VarArray:
    ক্ষেত্র VDfgType::VarPacked: {
        ধ্রুবক DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ফেরত;
    }
    ক্ষেত্র VDfgType::পূর্ব: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::SpliceArray:
    ক্ষেত্র VDfgType::SplicePacked: {
        ধ্রুবক DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ধ্রুবক DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == Dfg_তথ্য_Type::select(v.dtype(), lo, src.আকার_2()), "driver");
            ফেরত মিথ্যা;
        });
        ফেরত;
    }
    ক্ষেত্র VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::UnitArray: {
        ধ্রুবক DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.আকার_2() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::Sel: {
        ধ্রুবক DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == Dfg_তথ্য_Type::select(v.srcp()->dtype(), v.lsb(), v.আকার_2()), "sel");
        ফেরত;
    }
    ক্ষেত্র VDfgType::MatchMasked: {
        ধ্রুবক DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.আকার_2() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ফেরত;
    }
    ক্ষেত্র VDfgType::Mux: {
        ধ্রুবক DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->আকার_2() >= v.আকার_2(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ফেরত;
    }
    ক্ষেত্র VDfgType::ArraySel: {
        ধ্রুবক DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Add:
    ক্ষেত্র VDfgType::And:
    ক্ষেত্র VDfgType::Div:
    ক্ষেত্র VDfgType::DivS:
    ক্ষেত্র VDfgType::ModDiv:
    ক্ষেত্র VDfgType::ModDivS:
    ক্ষেত্র VDfgType::Mul:
    ক্ষেত্র VDfgType::MulS:
    ক্ষেত্র VDfgType::Or:
    ক্ষেত্র VDfgType::Sub:
    ক্ষেত্র VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Negate:
    ক্ষেত্র VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::ShiftL:
    ক্ষেত্র VDfgType::ShiftR:
    ক্ষেত্র VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Concat: {
        ধ্রুবক DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.আকার_2() == v.rhsp()->আকার_2() + v.lhsp()->আকার_2(), "Concat result mismatch");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Rep: {
        ধ্রুবক DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.আকার_2() % v.srcp()->আকার_2() == 0, "Not a replicate");
        ফেরত;
    }

    ক্ষেত্র VDfgType::StreamL:
    ক্ষেত্র VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Eq:
    ক্ষেত্র VDfgType::EqCase:
    ক্ষেত্র VDfgType::EqWild:
    ক্ষেত্র VDfgType::Neq:
    ক্ষেত্র VDfgType::NeqCase:
    ক্ষেত্র VDfgType::NeqWild:
    ক্ষেত্র VDfgType::Gt:
    ক্ষেত্র VDfgType::GtS:
    ক্ষেত্র VDfgType::Gte:
    ক্ষেত্র VDfgType::GteS:
    ক্ষেত্র VDfgType::Lt:
    ক্ষেত্র VDfgType::LtS:
    ক্ষেত্র VDfgType::Lte:
    ক্ষেত্র VDfgType::LteS: {
        CHECK(dtype() == Dfg_তথ্য_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Extend:
    ক্ষেত্র VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->আকার_2() < আকার_2(), "Operand should be narrower");
        ফেরত;
    }

    ক্ষেত্র VDfgType::LogAnd:
    ক্ষেত্র VDfgType::LogEq:
    ক্ষেত্র VDfgType::LogIf:
    ক্ষেত্র VDfgType::LogOr: {
        CHECK(dtype() == Dfg_তথ্য_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::LogNot:
    ক্ষেত্র VDfgType::RedAnd:
    ক্ষেত্র VDfgType::RedOr:
    ক্ষেত্র VDfgType::RedXor:
    ক্ষেত্র VDfgType::OneHot:
    ক্ষেত্র VDfgType::OneHot0: {
        CHECK(dtype() == Dfg_তথ্য_Type::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Cond: {
        ধ্রুবক DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ফেরত;
    }

    ক্ষেত্র VDfgType::গুনতি_Ones_2: {
        ধ্রুবক Dfg_গুনতি_Ones& v = *as<Dfg_গুনতি_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.আকার_2() == 32U, "Should yield a 32-bit result");
        ফেরত;
    }

    ক্ষেত্র VDfgType::Pow:
    ক্ষেত্র VDfgType::PowSS:
    ক্ষেত্র VDfgType::PowSU:
    ক্ষেত্র VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ফেরত;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ধ্রুবক {
    uint32_t ফলাফল = 0;
    foreachSink([&](ধ্রুবক DfgVertex&) {
        ++ফলাফল;
        ফেরত মিথ্যা;
    });
    ফেরত ফলাফল;
}

DfgVertexVar* DfgVertex::get_ফলাফল_Var() {
    // It's easy if the vertex is already a variable ...
    যদি (DfgVertexVar* ধ্রুবক varp = এই->cast<DfgVertexVar>()) ফেরত varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    এই->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ধ্রুবক varp = sink.cast<DfgVertexVar>();
        যদি (!varp) ফেরত মিথ্যা;

        // Do not use it if value might differ from its drivers
        যদি (varp->isVolatile()) ফেরত মিথ্যা;

        // First variable found
        যদি (!resp) {
            resp = varp;
            ফেরত মিথ্যা;
        }

        // Prefer those variables that must be kept anyway
        যদি (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            যদি (!resp->hasExtRdRefs()) resp = varp;
            ফেরত মিথ্যা;
        }
        যদি (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            যদি (!resp->hasDfgRefs()) resp = varp;
            ফেরত মিথ্যা;
        }

        // Prefer real variables over temporaries
        ধ্রুবক যৌক্তিক res_Is_অস্থায়ী = resp->vscp()->varp()->is_অস্থায়ী();
        ধ্রুবক যৌক্তিক var_Is_অস্থায়ী = varp->vscp()->varp()->is_অস্থায়ী();
        যদি (res_Is_অস্থায়ী != var_Is_অস্থায়ী) {
            যদি (res_Is_অস্থায়ী) resp = varp;
            ফেরত মিথ্যা;
        }
        যদি (!resp->tmpForp() != !varp->tmpForp()) {
            যদি (resp->tmpForp()) resp = varp;
            ফেরত মিথ্যা;
        }

        // Prefer the earlier one in source order
        ধ্রুবক File_সারি& oldFlp = *(resp->fileline());
        ধ্রুবক File_সারি& newFlp = *(varp->fileline());
        যদি (ধ্রুবক পূর্ণসংখ্যা cmp = oldFlp.operatorCompare(newFlp)) {
            যদি (cmp > 0) resp = varp;
            ফেরত মিথ্যা;
        }
        // Prefer the one with the lexically smaller name
        যদি (ধ্রুবক পূর্ণসংখ্যা cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            যদি (cmp > 0) resp = varp;
            ফেরত মিথ্যা;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ফেরত মিথ্যা;
    });
    ফেরত resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, যৌক্তিক try_ফলাফল_Var) VL_MT_DISABLED {
    // If this is a variable, we are done
    যদি (ধ্রুবক DfgVertexVar* ধ্রুবক varp = এই->cast<DfgVertexVar>()) {
        ফেরত varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    যদি (try_ফলাফল_Var) {
        যদি (ধ্রুবক DfgVertexVar* ধ্রুবক varp = এই->get_ফলাফল_Var()) {
            ফেরত varp->vscp()->scopep();
        }
    }

    AstScope* ধ্রুবক rootp = v3Global.rootp()->শীর্ষ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[এই];
    যদি (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ধ্রুবক scp = src.scopep(cache, সত্য);
            যদি (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ফেরত সত্য;
            }
            ফেরত মিথ্যা;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), এই,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ফেরত resultr;
}

শূন্য DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    যখন (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*এই);
    // Delete - this will unlink sources
    delete এই;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t আকার_2) {
    DfgVertex* ধ্রুবক srcp = এই->srcp();
    // Not driven at all
    যদি (!srcp) ফেরত {nullptr, 0};
    // If volatile, can have other drivers
    যদি (isVolatile()) ফেরত {nullptr, 0};
    // Don't inline CReset
    যদি (srcp->is<DfgCReset>()) ফেরত {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ধ্রুবক splicep = srcp->cast<DfgVertexSplice>();
    যদি (!splicep) ফেরত {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ধ্রুবক uint32_t hi = lo + আকার_2 - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    যৌক্তিক useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ধ্রুবক uint32_t dLo) {
        ধ্রুবক uint32_t dHi = dLo + src.আকার_2() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        যদি (dLo <= hi && lo <= dHi) useDefault = মিথ্যা;
        // If it does not cover the whole searched range, move on
        যদি (lo < dLo || dHi < hi) ফেরত মিথ্যা;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ফেরত সত্য;
    });

    // If a single driver covers the searched range, it is the one
    যদি (driverp) ফেরত {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    যদি (useDefault) ফেরত {defaultp(), lo};
    // Not driven by a single vertex
    ফেরত {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

শ্রেণী DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ধ্রুবক DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ধ্রুবক DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    স্থির std::string toLetters(আকার_t মান, যৌক্তিক lowerCase = মিথ্যা) {
        ধ্রুবক অক্ষর base = lowerCase ? 'a' : 'A';
        std::string s;
        করো { s += static_cast<অক্ষর>(base + মান % 26); } যখন (মান /= 26);
        ফেরত s;
    }

    ধ্রুবক std::string& internConst(ধ্রুবক DfgConst& vtx) {
        ধ্রুবক auto pair = m_internedConsts.emplace(vtx.অঙ্ক().ascii(মিথ্যা), "");
        যদি (pair.second) pair.first->second += toLetters(m_internedConsts.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internSelLsb(uint32_t মান) {
        ধ্রুবক auto pair = m_internedSelLsbs.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedSelLsbs.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internWordWidth(uint32_t মান) {
        ধ্রুবক auto pair = m_internedWordWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWordWidths.আকার_2() - 1, সত্য);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internWideWidth(uint32_t মান) {
        ধ্রুবক auto pair = m_internedWideWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWideWidths.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internVertex(ধ্রুবক DfgVertex& vtx) {
        ধ্রুবক auto pair = m_internedVertices.emplace(&vtx, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedVertices.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    শূন্য recordMultiplicityAndDepth(ধ্রুবক DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& মান = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        মান.first += 1;
        মান.second = std::max(মান.second, depth);
        যদি (!depth) ফেরত;
        vtx.foreachSource([&](ধ্রুবক DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ফেরত মিথ্যা;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    শূন্য render(ধ্রুবক DfgVertex& vtx, uint32_t depth, যৌক্তিক is_মূল_2 = সত্য) {
        যদি (ধ্রুবক DfgConst* ধ্রুবক constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            যদি (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } অন্যথা যদি (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } অন্যথা {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } অন্যথা যদি (!is_মূল_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } অন্যথা যদি (!vtx.foreachSource([&](ধ্রুবক DfgVertex&) { ফেরত সত্য; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } অন্যথা যদি (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } অন্যথা {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            যদি (ধ্রুবক DfgSel* ধ্রুবক selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                যদি (selp->lsb() == 0) {
                    m_os << '0';
                } অন্যথা {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ধ্রুবক DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, মিথ্যা);
                ফেরত মিথ্যা;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        যদি (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } অন্যথা {
            ধ্রুবক uint32_t width = vtx.আকার_2();
            যদি (width == 1) {
                m_os << '1';
            } অন্যথা যদি (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } অন্যথা {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        যদি (vtx.hasMultipleSinks()) m_os << '*';
    }

সর্বজনীন:
    DfgPatternString(std::ostream& os, ধ্রুবক DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, মিথ্যা);
        using Pair = std::pair<std::string, ধ্রুবক DfgVertex*>;
        std::vector<Pair> vertices;
        জন্য (ধ্রুবক auto& pair : m_multiplicityAndDepth) {
            যদি (pair.second.first == 1) চালু;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ধ্রুবক Pair& a, ধ্রুবক Pair& b) {  //
            ফেরত a.first < b.first;
        });
        জন্য (ধ্রুবক Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ধ্রুবক {
    std::ostringstream oss;
    DfgPatternString{oss, *এই, depth};
    ফেরত oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#অন্তর্ভাব "V3Dfg__gen_visitor_defns.h"  // From ./astgen
