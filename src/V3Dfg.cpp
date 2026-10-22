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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Dfg.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3DfgContext.h"
#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ধ্রুৱ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*এতত্); });
}

শূন্য DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    যদি (otherps.empty()) ঘূৰাই;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ধ্রুৱ VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    বাবে (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    বাবে (ধ্রুৱ std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        বাবে (DfgVertexVar* ধ্রুৱ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            যদি (DfgVertexVar* ধ্রুৱ altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
                DfgVertex* ধ্রুৱ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                যদি (srcp) altp->srcp(srcp);
                যদি (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                অনুৱর্ত;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এতত্;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        বাবে (DfgVertexAst* ধ্রুৱ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = এতত্;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        বাবে (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এতত্;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        বাবে (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = এতত্;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_সাৰি* flp, ধ্রুৱ std::string& prefix,
                                   ধ্রুৱ DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ধ্রুৱ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    যদি (dtype.isArray()) ঘূৰাই নূতন DfgVarArray{*এতত্, vscp};
    ঘূৰাই নূতন DfgVarPacked{*এতত্, vscp};
}

স্থির ধ্রুৱ std::string toDotId(ধ্রুৱ DfgVertex& vtx) { ঘূৰাই '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
স্থির শূন্য dumpDotVertex(std::ostream& os, ধ্রুৱ DfgVertex& vtx) {
    যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varVtxp = vtx.cast<DfgVertexVar>()) {
        ধ্রুৱ AstVarScope* ধ্রুৱ vscp = varVtxp->vscp();
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
        যদি (ধ্রুৱ AstVarScope* ধ্রুৱ tmpForp = varVtxp->tmpForp()) {
            যদি (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[মিছা][varVtxp->hasModWrRefs()];
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
        ধ্রুৱ ৱর্ণ* ধ্রুৱ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ঘূৰাই;
    }

    যদি (ধ্রুৱ DfgPrev* ধ্রুৱ prevVtxp = vtx.cast<DfgPrev>()) {
        ধ্রুৱ AstVarScope* ধ্রুৱ vscp = prevVtxp->vscp();
        os << toDotId(vtx);
        // Begin attributes
        os << " [";
        // Begin 'label'
        os << "label=\"";
        // Name
        os << vscp->prettyName();
        // Address
        os << '\n' << cvtToHex(prevVtxp);
        // Type and fanout
        os << '\n';
        prevVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << prevVtxp->fanout();
        // End 'label'
        os << '"';
        // Shape
        যদি (prevVtxp->isPacked()) {
            os << ", shape=box";
        } অন্যথা যদি (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } অন্যথা {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ধ্রুৱ ৱর্ণ* ধ্রুৱ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ঘূৰাই;
    }

    যদি (ধ্রুৱ DfgConst* ধ্রুৱ constVtxp = vtx.cast<DfgConst>()) {
        ধ্রুৱ V3_সংখ্যা& অঙ্ক = constVtxp->অঙ্ক();

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
        ঘূৰাই;
    }

    যদি (ধ্রুৱ DfgSel* ধ্রুৱ selVtxp = vtx.cast<DfgSel>()) {
        ধ্রুৱ uint32_t lsb = selVtxp->lsb();
        ধ্রুৱ uint32_t msb = lsb + selVtxp->width() - 1;
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
        ঘূৰাই;
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
        ঘূৰাই;
    }

    যদি (ধ্রুৱ DfgLogic* ধ্রুৱ logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুৱ ৱর্ণ* ধ্রুৱ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        ঘূৰাই;
    }

    যদি (ধ্রুৱ DfgVertexAst* ধ্রুৱ astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_বৃক্ষ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ধ্রুৱ DfgAstRd* ধ্রুৱ astRdVtxp = astVtxp->cast<DfgAstRd>();
        ধ্রুৱ ৱর্ণ* ধ্রুৱ colorp = astRdVtxp ? "#80ff80"  // Green
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
        ঘূৰাই;
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

শূন্য DfgGraph::dumpDot(std::ostream& os, ধ্রুৱ std::string& label,
                       std::function<তার্কিক(ধ্রুৱ DfgVertex&)> p) ধ্রুৱ {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    যদি (!p) p = [](ধ্রুৱ DfgVertex&) { ঘূৰাই সঁচা; };

    std::unordered_set<ধ্রুৱ DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
        ধ্রুৱ DfgLogic* ধ্রুৱ logicp = vtx.cast<DfgLogic>();
        যদি (!logicp) ঘূৰাই;
        যদি (logicp->synth().empty()) ঘূৰাই;
        যদি (!p(vtx)) ঘূৰাই;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ঢোকাও(logicp);
        বাবে (DfgVertex* ধ্রুৱ vtxp : logicp->synth()) {
            যদি (!p(*vtxp)) অনুৱর্ত;
            dumpDotVertex(os, *vtxp);
            emitted.ঢোকাও(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
        যদি (emitted.গুনতি(&vtx)) ঘূৰাই;
        যদি (!p(vtx)) ঘূৰাই;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
        যদি (!p(vtx)) ঘূৰাই;
        বাবে (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ধ্রুৱ srcp = vtx.inputp(i);
            যদি (!srcp) অনুৱর্ত;
            যদি (!p(*srcp)) অনুৱর্ত;
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

std::string DfgGraph::dumpDotString(ধ্রুৱ std::string& label,
                                    std::function<তার্কিক(ধ্রুৱ DfgVertex&)> p) ধ্রুৱ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ঘূৰাই ss.str();
}

শূন্য DfgGraph::dumpDotFile(ধ্রুৱ std::string& filename, ধ্রুৱ std::string& label,
                           std::function<তার্কিক(ধ্রুৱ DfgVertex&)> p) ধ্রুৱ {
    ধ্রুৱ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    যদি (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

শূন্য DfgGraph::dumpDotFilePrefixed(ধ্রুৱ std::string& label,
                                   std::function<তার্কিক(ধ্রুৱ DfgVertex&)> p) ধ্রুৱ {
    std::string filename = name();
    যদি (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <তার্কিক T_SinksNotSources>
স্থির std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>>
dfgGraphCollectCone(ধ্রুৱ std::vector<ধ্রুৱ DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ধ্রুৱ DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>> resp{
        নূতন std::unordered_set<ধ্রুৱ DfgVertex*>{}};
    // Depth first traversal
    যেতিয়ালৈ (!queue.empty()) {
        // Pop next work item
        ধ্রুৱ DfgVertex* ধ্রুৱ vtxp = queue.back();
        queue.বেৰ_কৰো_back();
        // Mark vertex as visited, move on if already visited
        যদি (!resp->ঢোকাও(vtxp).second) অনুৱর্ত;
        // Enqueue all siblings of this vertex.
        যদি VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ধ্রুৱ DfgVertex& sink) {
                queue.ঠেলো_back(&sink);
                ঘূৰাই মিছা;
            });
        } অন্যথা {
            vtxp->foreachSource([&](ধ্রুৱ DfgVertex& src) {
                queue.ঠেলো_back(&src);
                ঘূৰাই মিছা;
            });
        }
    }
    // Done
    ঘূৰাই resp;
}

std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>>
DfgGraph::sourceCone(ধ্রুৱ std::vector<ধ্রুৱ DfgVertex*>& vtxps) ধ্রুৱ {
    ঘূৰাই dfgGraphCollectCone<মিছা>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>>
DfgGraph::sinkCone(ধ্রুৱ std::vector<ধ্রুৱ DfgVertex*>& vtxps) ধ্রুৱ {
    ঘূৰাই dfgGraphCollectCone<সঁচা>(vtxps);
}

std::unique_ptr<std::unordered_set<ধ্রুৱ DfgVertex*>>
DfgGraph::neighborhood(ধ্রুৱ std::vector<ধ্রুৱ DfgVertex*>& vtxps, size_t n) ধ্রুৱ {
    // Neighborhood
    std::vector<ধ্রুৱ DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ধ্রুৱ DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    বাবে (size_t hops = 1; hops <= n; ++hops) {
        বাবে (size_t i = begin; i < end; ++i) {
            ধ্রুৱ DfgVertex* ধ্রুৱ vtxp = vec[i];
            vtxp->foreachSink([&](ধ্রুৱ DfgVertex& dst) {
                যদি (res.ঢোকাও(&dst).second) vec.ঠেলো_back(&dst);
                ঘূৰাই মিছা;
            });
            vtxp->foreachSource([&](ধ্রুৱ DfgVertex& src) {
                যদি (res.ঢোকাও(&src).second) vec.ঠেলো_back(&src);
                ঘূৰাই মিছা;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    ঘূৰাই std::make_unique<std::unordered_set<ধ্রুৱ DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_সাৰি* flp, ধ্রুৱ DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*এতত্);
}

তার্কিক DfgVertex::unsafe() ধ্রুৱ {
    যদি (is<DfgMux>()) ঘূৰাই সঁচা;
    যদি (is<DfgArraySel>()) ঘূৰাই !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ঘূৰাই মিছা;
}

শূন্য DfgVertex::typeCheck(ধ্রুৱ DfgGraph& dfg) ধ্রুৱ {

#define CHECK(cond, বাৰ্তা) \
    UASSERT_OBJ(cond, এতত্, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << বাৰ্তা);
    ৱিকল্প (type()) {
    পক্ষ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::AstRd: {
        ধ্রুৱ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::VarArray:
    পক্ষ VDfgType::VarPacked: {
        ধ্রুৱ DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ঘূৰাই;
    }
    পক্ষ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::SpliceArray:
    পক্ষ VDfgType::SplicePacked: {
        ধ্রুৱ DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ধ্রুৱ DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            ঘূৰাই মিছা;
        });
        ঘূৰাই;
    }
    পক্ষ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::UnitArray: {
        ধ্রুৱ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::Sel: {
        ধ্রুৱ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        ঘূৰাই;
    }
    পক্ষ VDfgType::MatchMasked: {
        ধ্রুৱ DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ঘূৰাই;
    }
    পক্ষ VDfgType::Mux: {
        ধ্রুৱ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ঘূৰাই;
    }
    পক্ষ VDfgType::ArraySel: {
        ধ্রুৱ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Add:
    পক্ষ VDfgType::And:
    পক্ষ VDfgType::Div:
    পক্ষ VDfgType::DivS:
    পক্ষ VDfgType::ModDiv:
    পক্ষ VDfgType::ModDivS:
    পক্ষ VDfgType::Mul:
    পক্ষ VDfgType::MulS:
    পক্ষ VDfgType::Or:
    পক্ষ VDfgType::Sub:
    পক্ষ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Negate:
    পক্ষ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::ShiftL:
    পক্ষ VDfgType::ShiftR:
    পক্ষ VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Concat: {
        ধ্রুৱ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Rep: {
        ধ্রুৱ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        ঘূৰাই;
    }

    পক্ষ VDfgType::StreamL:
    পক্ষ VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Eq:
    পক্ষ VDfgType::EqCase:
    পক্ষ VDfgType::EqWild:
    পক্ষ VDfgType::Neq:
    পক্ষ VDfgType::NeqCase:
    পক্ষ VDfgType::NeqWild:
    পক্ষ VDfgType::Gt:
    পক্ষ VDfgType::GtS:
    পক্ষ VDfgType::Gte:
    পক্ষ VDfgType::GteS:
    পক্ষ VDfgType::Lt:
    পক্ষ VDfgType::LtS:
    পক্ষ VDfgType::Lte:
    পক্ষ VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Extend:
    পক্ষ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        ঘূৰাই;
    }

    পক্ষ VDfgType::LogAnd:
    পক্ষ VDfgType::LogEq:
    পক্ষ VDfgType::LogIf:
    পক্ষ VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::LogNot:
    পক্ষ VDfgType::RedAnd:
    পক্ষ VDfgType::RedOr:
    পক্ষ VDfgType::RedXor:
    পক্ষ VDfgType::OneHot:
    পক্ষ VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Cond: {
        ধ্রুৱ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ঘূৰাই;
    }

    পক্ষ VDfgType::গুনতি_Ones_2: {
        ধ্রুৱ Dfg_গুনতি_Ones& v = *as<Dfg_গুনতি_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        ঘূৰাই;
    }

    পক্ষ VDfgType::Pow:
    পক্ষ VDfgType::PowSS:
    পক্ষ VDfgType::PowSU:
    পক্ষ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ঘূৰাই;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ধ্রুৱ {
    uint32_t result = 0;
    foreachSink([&](ধ্রুৱ DfgVertex&) {
        ++result;
        ঘূৰাই মিছা;
    });
    ঘূৰাই result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    যদি (DfgVertexVar* ধ্রুৱ varp = এতত্->cast<DfgVertexVar>()) ঘূৰাই varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    এতত্->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ধ্রুৱ varp = sink.cast<DfgVertexVar>();
        যদি (!varp) ঘূৰাই মিছা;

        // Do not use it if value might differ from its drivers
        যদি (varp->isVolatile()) ঘূৰাই মিছা;

        // First variable found
        যদি (!resp) {
            resp = varp;
            ঘূৰাই মিছা;
        }

        // Prefer those variables that must be kept anyway
        যদি (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            যদি (!resp->hasExtRdRefs()) resp = varp;
            ঘূৰাই মিছা;
        }
        যদি (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            যদি (!resp->hasDfgRefs()) resp = varp;
            ঘূৰাই মিছা;
        }

        // Prefer real variables over temporaries
        ধ্রুৱ তার্কিক resIsTemp = resp->vscp()->varp()->isTemp();
        ধ্রুৱ তার্কিক varIsTemp = varp->vscp()->varp()->isTemp();
        যদি (resIsTemp != varIsTemp) {
            যদি (resIsTemp) resp = varp;
            ঘূৰাই মিছা;
        }
        যদি (!resp->tmpForp() != !varp->tmpForp()) {
            যদি (resp->tmpForp()) resp = varp;
            ঘূৰাই মিছা;
        }

        // Prefer the earlier one in source order
        ধ্রুৱ File_সাৰি& oldFlp = *(resp->fileline());
        ধ্রুৱ File_সাৰি& newFlp = *(varp->fileline());
        যদি (ধ্রুৱ পূর্ণাঙ্ক cmp = oldFlp.operatorCompare(newFlp)) {
            যদি (cmp > 0) resp = varp;
            ঘূৰাই মিছা;
        }
        // Prefer the one with the lexically smaller name
        যদি (ধ্রুৱ পূর্ণাঙ্ক cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            যদি (cmp > 0) resp = varp;
            ঘূৰাই মিছা;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ঘূৰাই মিছা;
    });
    ঘূৰাই resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, তার্কিক tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = এতত্->cast<DfgVertexVar>()) {
        ঘূৰাই varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    যদি (tryResultVar) {
        যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = এতত্->getResultVar()) {
            ঘূৰাই varp->vscp()->scopep();
        }
    }

    AstScope* ধ্রুৱ rootp = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[এতত্];
    যদি (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ধ্রুৱ scp = src.scopep(cache, সঁচা);
            যদি (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ঘূৰাই সঁচা;
            }
            ঘূৰাই মিছা;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), এতত্,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ঘূৰাই resultr;
}

শূন্য DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    যেতিয়ালৈ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*এতত্);
    // Delete - this will unlink sources
    delete এতত্;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ধ্রুৱ srcp = এতত্->srcp();
    // Not driven at all
    যদি (!srcp) ঘূৰাই {nullptr, 0};
    // If volatile, can have other drivers
    যদি (isVolatile()) ঘূৰাই {nullptr, 0};
    // Don't inline CReset
    যদি (srcp->is<DfgCReset>()) ঘূৰাই {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ধ্রুৱ splicep = srcp->cast<DfgVertexSplice>();
    যদি (!splicep) ঘূৰাই {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ধ্রুৱ uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    তার্কিক useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ধ্রুৱ uint32_t dLo) {
        ধ্রুৱ uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        যদি (dLo <= hi && lo <= dHi) useDefault = মিছা;
        // If it does not cover the whole searched range, move on
        যদি (lo < dLo || dHi < hi) ঘূৰাই মিছা;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ঘূৰাই সঁচা;
    });

    // If a single driver covers the searched range, it is the one
    যদি (driverp) ঘূৰাই {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    যদি (useDefault) ঘূৰাই {defaultp(), lo};
    // Not driven by a single vertex
    ঘূৰাই {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

শ্ৰেণী DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ধ্রুৱ DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ধ্রুৱ DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    স্থির std::string toLetters(size_t মান, তার্কিক lowerCase = মিছা) {
        ধ্রুৱ ৱর্ণ base = lowerCase ? 'a' : 'A';
        std::string s;
        কৰা { s += static_cast<ৱর্ণ>(base + মান % 26); } যেতিয়ালৈ (মান /= 26);
        ঘূৰাই s;
    }

    ধ্রুৱ std::string& internConst(ধ্রুৱ DfgConst& vtx) {
        ধ্রুৱ auto pair = m_internedConsts.emplace(vtx.অঙ্ক().ascii(মিছা), "");
        যদি (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ঘূৰাই pair.first->second;
    }

    ধ্রুৱ std::string& internSelLsb(uint32_t মান) {
        ধ্রুৱ auto pair = m_internedSelLsbs.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        ঘূৰাই pair.first->second;
    }

    ধ্রুৱ std::string& internWordWidth(uint32_t মান) {
        ধ্রুৱ auto pair = m_internedWordWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, সঁচা);
        ঘূৰাই pair.first->second;
    }

    ধ্রুৱ std::string& internWideWidth(uint32_t মান) {
        ধ্রুৱ auto pair = m_internedWideWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ঘূৰাই pair.first->second;
    }

    ধ্রুৱ std::string& internVertex(ধ্রুৱ DfgVertex& vtx) {
        ধ্রুৱ auto pair = m_internedVertices.emplace(&vtx, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        ঘূৰাই pair.first->second;
    }

    শূন্য recordMultiplicityAndDepth(ধ্রুৱ DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& মান = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        মান.first += 1;
        মান.second = std::max(মান.second, depth);
        যদি (!depth) ঘূৰাই;
        vtx.foreachSource([&](ধ্রুৱ DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ঘূৰাই মিছা;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    শূন্য render(ধ্রুৱ DfgVertex& vtx, uint32_t depth, তার্কিক is_মূল_2 = সঁচা) {
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ constp = vtx.cast<DfgConst>()) {
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
        } অন্যথা যদি (!vtx.foreachSource([&](ধ্রুৱ DfgVertex&) { ঘূৰাই সঁচা; })) {
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
            যদি (ধ্রুৱ DfgSel* ধ্রুৱ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                যদি (selp->lsb() == 0) {
                    m_os << '0';
                } অন্যথা {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ধ্রুৱ DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, মিছা);
                ঘূৰাই মিছা;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        যদি (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } অন্যথা {
            ধ্রুৱ uint32_t width = vtx.size();
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

সার্ৱজনিক:
    DfgPatternString(std::ostream& os, ধ্রুৱ DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, মিছা);
        using Pair = std::pair<std::string, ধ্রুৱ DfgVertex*>;
        std::vector<Pair> vertices;
        বাবে (ধ্রুৱ auto& pair : m_multiplicityAndDepth) {
            যদি (pair.second.first == 1) অনুৱর্ত;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ধ্রুৱ Pair& a, ধ্রুৱ Pair& b) {  //
            ঘূৰাই a.first < b.first;
        });
        বাবে (ধ্রুৱ Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ধ্রুৱ {
    std::ostringstream oss;
    DfgPatternString{oss, *এতত্, depth};
    ঘূৰাই oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#অন্তর্ভাৱ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
