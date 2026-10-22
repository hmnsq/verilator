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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgContext.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ଅଚଳ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*ଏହି); });
}

ଶୂନ୍ୟ DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    ଯଦି (otherps.empty()) ଫେରନ୍ତୁ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ଅଚଳ VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    ପାଇଁ (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    ପାଇଁ (ଅଚଳ std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        ପାଇଁ (DfgVertexVar* ଅଚଳ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ଯଦି (DfgVertexVar* ଅଚଳ altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ଅଚଳ srcp = vtxp->srcp();
                DfgVertex* ଅଚଳ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ଯଦି (srcp) altp->srcp(srcp);
                ଯଦି (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ଜାରି;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ଏହି;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        ପାଇଁ (DfgVertexAst* ଅଚଳ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ଏହି;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        ପାଇଁ (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ଏହି;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        ପାଇଁ (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ଏହି;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_ଧାଡ଼ି* flp, ଅଚଳ std::string& prefix,
                                   ଅଚଳ DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ଅଚଳ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    ଯଦି (dtype.isArray()) ଫେରନ୍ତୁ ନୂତନ DfgVarArray{*ଏହି, vscp};
    ଫେରନ୍ତୁ ନୂତନ DfgVarPacked{*ଏହି, vscp};
}

ସ୍ଥିର ଅଚଳ std::string toDotId(ଅଚଳ DfgVertex& vtx) { ଫେରନ୍ତୁ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ସ୍ଥିର ଶୂନ୍ୟ dumpDotVertex(std::ostream& os, ଅଚଳ DfgVertex& vtx) {
    ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varVtxp = vtx.cast<DfgVertexVar>()) {
        ଅଚଳ AstVarScope* ଅଚଳ vscp = varVtxp->vscp();
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
        ଯଦି (ଅଚଳ AstVarScope* ଅଚଳ tmpForp = varVtxp->tmpForp()) {
            ଯଦି (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ମିଥ୍ୟା][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ଯଦି (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ଅନ୍ୟଥା ଯଦି (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ଅନ୍ୟଥା {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ଫେରନ୍ତୁ;
    }

    ଯଦି (ଅଚଳ DfgPrev* ଅଚଳ prevVtxp = vtx.cast<DfgPrev>()) {
        ଅଚଳ AstVarScope* ଅଚଳ vscp = prevVtxp->vscp();
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
        ଯଦି (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ଅନ୍ୟଥା ଯଦି (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ଅନ୍ୟଥା {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ଫେରନ୍ତୁ;
    }

    ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constVtxp = vtx.cast<DfgConst>()) {
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = constVtxp->ଅଙ୍କ();

        os << toDotId(vtx);
        os << " [label=\"";
        ଯଦି (ଅଙ୍କ.width() <= 32 && !ଅଙ୍କ.isSigned()) {
            os << constVtxp->width() << "'d" << ଅଙ୍କ.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << ଅଙ୍କ.toUInt() << std::dec << '\n';
        } ଅନ୍ୟଥା {
            os << ଅଙ୍କ.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ଫେରନ୍ତୁ;
    }

    ଯଦି (ଅଚଳ DfgSel* ଅଚଳ selVtxp = vtx.cast<DfgSel>()) {
        ଅଚଳ uint32_t lsb = selVtxp->lsb();
        ଅଚଳ uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ଯଦି (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } ଅନ୍ୟଥା {
            os << ", shape=circle";
        }
        os << "]\n";
        ଫେରନ୍ତୁ;
    }

    ଯଦି (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ଯଦି (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } ଅନ୍ୟଥା {
            os << ", shape=octagon";
        }
        os << "]\n";
        ଫେରନ୍ତୁ;
    }

    ଯଦି (ଅଚଳ DfgLogic* ଅଚଳ logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        ଫେରନ୍ତୁ;
    }

    ଯଦି (ଅଚଳ DfgVertexAst* ଅଚଳ astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ଅଚଳ DfgAstRd* ଅଚଳ astRdVtxp = astVtxp->cast<DfgAstRd>();
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ colorp = astRdVtxp ? "#80ff80"  // Green
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
        ଫେରନ୍ତୁ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ଯଦି (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } ଅନ୍ୟଥା {
        os << ", shape=circle";
    }
    os << "]\n";
}

ଶୂନ୍ୟ DfgGraph::dumpDot(std::ostream& os, ଅଚଳ std::string& label,
                       std::function<ତାର୍କିକ(ଅଚଳ DfgVertex&)> p) ଅଚଳ {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ଯଦି (!p) p = [](ଅଚଳ DfgVertex&) { ଫେରନ୍ତୁ ସତ୍ୟ; };

    std::unordered_set<ଅଚଳ DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
        ଅଚଳ DfgLogic* ଅଚଳ logicp = vtx.cast<DfgLogic>();
        ଯଦି (!logicp) ଫେରନ୍ତୁ;
        ଯଦି (logicp->synth().empty()) ଫେରନ୍ତୁ;
        ଯଦି (!p(vtx)) ଫେରନ୍ତୁ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ଭର୍ତ୍ତି(logicp);
        ପାଇଁ (DfgVertex* ଅଚଳ vtxp : logicp->synth()) {
            ଯଦି (!p(*vtxp)) ଜାରି;
            dumpDotVertex(os, *vtxp);
            emitted.ଭର୍ତ୍ତି(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
        ଯଦି (emitted.ଗଣନା_2(&vtx)) ଫେରନ୍ତୁ;
        ଯଦି (!p(vtx)) ଫେରନ୍ତୁ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
        ଯଦି (!p(vtx)) ଫେରନ୍ତୁ;
        ପାଇଁ (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ଅଚଳ srcp = vtx.inputp(i);
            ଯଦି (!srcp) ଜାରି;
            ଯଦି (!p(*srcp)) ଜାରି;
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

std::string DfgGraph::dumpDotString(ଅଚଳ std::string& label,
                                    std::function<ତାର୍କିକ(ଅଚଳ DfgVertex&)> p) ଅଚଳ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ଫେରନ୍ତୁ ss.str();
}

ଶୂନ୍ୟ DfgGraph::dumpDotFile(ଅଚଳ std::string& filename, ଅଚଳ std::string& label,
                           std::function<ତାର୍କିକ(ଅଚଳ DfgVertex&)> p) ଅଚଳ {
    ଅଚଳ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ଯଦି (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

ଶୂନ୍ୟ DfgGraph::dumpDotFilePrefixed(ଅଚଳ std::string& label,
                                   std::function<ତାର୍କିକ(ଅଚଳ DfgVertex&)> p) ଅଚଳ {
    std::string filename = name();
    ଯଦି (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <ତାର୍କିକ T_SinksNotSources>
ସ୍ଥିର std::unique_ptr<std::unordered_set<ଅଚଳ DfgVertex*>>
dfgGraphCollectCone(ଅଚଳ std::vector<ଅଚଳ DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ଅଚଳ DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ଅଚଳ DfgVertex*>> resp{
        ନୂତନ std::unordered_set<ଅଚଳ DfgVertex*>{}};
    // Depth first traversal
    ଯେତେବେଳେ (!queue.empty()) {
        // Pop next work item
        ଅଚଳ DfgVertex* ଅଚଳ vtxp = queue.back();
        queue.କାଢ଼_back();
        // Mark vertex as visited, move on if already visited
        ଯଦି (!resp->ଭର୍ତ୍ତି(vtxp).second) ଜାରି;
        // Enqueue all siblings of this vertex.
        ଯଦି VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ଅଚଳ DfgVertex& sink) {
                queue.ଠେଲ_back(&sink);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        } ଅନ୍ୟଥା {
            vtxp->foreachSource([&](ଅଚଳ DfgVertex& src) {
                queue.ଠେଲ_back(&src);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        }
    }
    // Done
    ଫେରନ୍ତୁ resp;
}

std::unique_ptr<std::unordered_set<ଅଚଳ DfgVertex*>>
DfgGraph::sourceCone(ଅଚଳ std::vector<ଅଚଳ DfgVertex*>& vtxps) ଅଚଳ {
    ଫେରନ୍ତୁ dfgGraphCollectCone<ମିଥ୍ୟା>(vtxps);
}

std::unique_ptr<std::unordered_set<ଅଚଳ DfgVertex*>>
DfgGraph::sinkCone(ଅଚଳ std::vector<ଅଚଳ DfgVertex*>& vtxps) ଅଚଳ {
    ଫେରନ୍ତୁ dfgGraphCollectCone<ସତ୍ୟ>(vtxps);
}

std::unique_ptr<std::unordered_set<ଅଚଳ DfgVertex*>>
DfgGraph::neighborhood(ଅଚଳ std::vector<ଅଚଳ DfgVertex*>& vtxps, size_t n) ଅଚଳ {
    // Neighborhood
    std::vector<ଅଚଳ DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ଅଚଳ DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    ପାଇଁ (size_t hops = 1; hops <= n; ++hops) {
        ପାଇଁ (size_t i = begin; i < end; ++i) {
            ଅଚଳ DfgVertex* ଅଚଳ vtxp = vec[i];
            vtxp->foreachSink([&](ଅଚଳ DfgVertex& dst) {
                ଯଦି (res.ଭର୍ତ୍ତି(&dst).second) vec.ଠେଲ_back(&dst);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            vtxp->foreachSource([&](ଅଚଳ DfgVertex& src) {
                ଯଦି (res.ଭର୍ତ୍ତି(&src).second) vec.ଠେଲ_back(&src);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    ଫେରନ୍ତୁ std::make_unique<std::unordered_set<ଅଚଳ DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*ଏହି);
}

ତାର୍କିକ DfgVertex::unsafe() ଅଚଳ {
    ଯଦି (is<DfgMux>()) ଫେରନ୍ତୁ ସତ୍ୟ;
    ଯଦି (is<DfgArraySel>()) ଫେରନ୍ତୁ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

ଶୂନ୍ୟ DfgVertex::typeCheck(ଅଚଳ DfgGraph& dfg) ଅଚଳ {

#define CHECK(cond, ସନ୍ଦେଶ) \
    UASSERT_OBJ(cond, ଏହି, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << ସନ୍ଦେଶ);
    ବାଛ (type()) {
    ସ୍ଥିତି VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::AstRd: {
        ଅଚଳ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::VarArray:
    ସ୍ଥିତି VDfgType::VarPacked: {
        ଅଚଳ DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::SpliceArray:
    ସ୍ଥିତି VDfgType::SplicePacked: {
        ଅଚଳ DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ଅଚଳ DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::UnitArray: {
        ଅଚଳ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::Sel: {
        ଅଚଳ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::MatchMasked: {
        ଅଚଳ DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::Mux: {
        ଅଚଳ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ଫେରନ୍ତୁ;
    }
    ସ୍ଥିତି VDfgType::ArraySel: {
        ଅଚଳ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Add:
    ସ୍ଥିତି VDfgType::And:
    ସ୍ଥିତି VDfgType::Div:
    ସ୍ଥିତି VDfgType::DivS:
    ସ୍ଥିତି VDfgType::ModDiv:
    ସ୍ଥିତି VDfgType::ModDivS:
    ସ୍ଥିତି VDfgType::Mul:
    ସ୍ଥିତି VDfgType::MulS:
    ସ୍ଥିତି VDfgType::Or:
    ସ୍ଥିତି VDfgType::Sub:
    ସ୍ଥିତି VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Negate:
    ସ୍ଥିତି VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::ShiftL:
    ସ୍ଥିତି VDfgType::ShiftR:
    ସ୍ଥିତି VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Concat: {
        ଅଚଳ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Rep: {
        ଅଚଳ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::StreamL:
    ସ୍ଥିତି VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Eq:
    ସ୍ଥିତି VDfgType::EqCase:
    ସ୍ଥିତି VDfgType::EqWild:
    ସ୍ଥିତି VDfgType::Neq:
    ସ୍ଥିତି VDfgType::NeqCase:
    ସ୍ଥିତି VDfgType::NeqWild:
    ସ୍ଥିତି VDfgType::Gt:
    ସ୍ଥିତି VDfgType::GtS:
    ସ୍ଥିତି VDfgType::Gte:
    ସ୍ଥିତି VDfgType::GteS:
    ସ୍ଥିତି VDfgType::Lt:
    ସ୍ଥିତି VDfgType::LtS:
    ସ୍ଥିତି VDfgType::Lte:
    ସ୍ଥିତି VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Extend:
    ସ୍ଥିତି VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::LogAnd:
    ସ୍ଥିତି VDfgType::LogEq:
    ସ୍ଥିତି VDfgType::LogIf:
    ସ୍ଥିତି VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::LogNot:
    ସ୍ଥିତି VDfgType::RedAnd:
    ସ୍ଥିତି VDfgType::RedOr:
    ସ୍ଥିତି VDfgType::RedXor:
    ସ୍ଥିତି VDfgType::OneHot:
    ସ୍ଥିତି VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Cond: {
        ଅଚଳ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::ଗଣନା_Ones_2: {
        ଅଚଳ Dfg_ଗଣନା_Ones& v = *as<Dfg_ଗଣନା_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିତି VDfgType::Pow:
    ସ୍ଥିତି VDfgType::PowSS:
    ସ୍ଥିତି VDfgType::PowSU:
    ସ୍ଥିତି VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ଫେରନ୍ତୁ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ଅଚଳ {
    uint32_t result = 0;
    foreachSink([&](ଅଚଳ DfgVertex&) {
        ++result;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    });
    ଫେରନ୍ତୁ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    ଯଦି (DfgVertexVar* ଅଚଳ varp = ଏହି->cast<DfgVertexVar>()) ଫେରନ୍ତୁ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    ଏହି->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ଅଚଳ varp = sink.cast<DfgVertexVar>();
        ଯଦି (!varp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Do not use it if value might differ from its drivers
        ଯଦି (varp->isVolatile()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // First variable found
        ଯଦି (!resp) {
            resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Prefer those variables that must be kept anyway
        ଯଦି (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ଯଦି (!resp->hasExtRdRefs()) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ଯଦି (!resp->hasDfgRefs()) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Prefer real variables over temporaries
        ଅଚଳ ତାର୍କିକ resIsTemp = resp->vscp()->varp()->isTemp();
        ଅଚଳ ତାର୍କିକ varIsTemp = varp->vscp()->varp()->isTemp();
        ଯଦି (resIsTemp != varIsTemp) {
            ଯଦି (resIsTemp) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (!resp->tmpForp() != !varp->tmpForp()) {
            ଯଦି (resp->tmpForp()) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Prefer the earlier one in source order
        ଅଚଳ File_ଧାଡ଼ି& oldFlp = *(resp->fileline());
        ଅଚଳ File_ଧାଡ଼ି& newFlp = *(varp->fileline());
        ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cmp = oldFlp.operatorCompare(newFlp)) {
            ଯଦି (cmp > 0) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Prefer the one with the lexically smaller name
        ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ଯଦି (cmp > 0) resp = varp;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    });
    ଫେରନ୍ତୁ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, ତାର୍କିକ tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = ଏହି->cast<DfgVertexVar>()) {
        ଫେରନ୍ତୁ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ଯଦି (tryResultVar) {
        ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = ଏହି->getResultVar()) {
            ଫେରନ୍ତୁ varp->vscp()->scopep();
        }
    }

    AstScope* ଅଚଳ rootp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ଏହି];
    ଯଦି (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ଅଚଳ scp = src.scopep(cache, ସତ୍ୟ);
            ଯଦି (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ଏହି,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ଫେରନ୍ତୁ resultr;
}

ଶୂନ୍ୟ DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    ଯେତେବେଳେ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*ଏହି);
    // Delete - this will unlink sources
    delete ଏହି;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ଅଚଳ srcp = ଏହି->srcp();
    // Not driven at all
    ଯଦି (!srcp) ଫେରନ୍ତୁ {nullptr, 0};
    // If volatile, can have other drivers
    ଯଦି (isVolatile()) ଫେରନ୍ତୁ {nullptr, 0};
    // Don't inline CReset
    ଯଦି (srcp->is<DfgCReset>()) ଫେରନ୍ତୁ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ଅଚଳ splicep = srcp->cast<DfgVertexSplice>();
    ଯଦି (!splicep) ଫେରନ୍ତୁ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ଅଚଳ uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    ତାର୍କିକ useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ଅଚଳ uint32_t dLo) {
        ଅଚଳ uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ଯଦି (dLo <= hi && lo <= dHi) useDefault = ମିଥ୍ୟା;
        // If it does not cover the whole searched range, move on
        ଯଦି (lo < dLo || dHi < hi) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    });

    // If a single driver covers the searched range, it is the one
    ଯଦି (driverp) ଫେରନ୍ତୁ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ଯଦି (useDefault) ଫେରନ୍ତୁ {defaultp(), lo};
    // Not driven by a single vertex
    ଫେରନ୍ତୁ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

ଶ୍ରେଣୀ DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ଅଚଳ DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ଅଚଳ DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    ସ୍ଥିର std::string toLetters(size_t ମୂଲ୍ୟ, ତାର୍କିକ lowerCase = ମିଥ୍ୟା) {
        ଅଚଳ ଅକ୍ଷର base = lowerCase ? 'a' : 'A';
        std::string s;
        କର { s += static_cast<ଅକ୍ଷର>(base + ମୂଲ୍ୟ % 26); } ଯେତେବେଳେ (ମୂଲ୍ୟ /= 26);
        ଫେରନ୍ତୁ s;
    }

    ଅଚଳ std::string& internConst(ଅଚଳ DfgConst& vtx) {
        ଅଚଳ auto pair = m_internedConsts.emplace(vtx.ଅଙ୍କ().ascii(ମିଥ୍ୟା), "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internSelLsb(uint32_t ମୂଲ୍ୟ) {
        ଅଚଳ auto pair = m_internedSelLsbs.emplace(ମୂଲ୍ୟ, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internWordWidth(uint32_t ମୂଲ୍ୟ) {
        ଅଚଳ auto pair = m_internedWordWidths.emplace(ମୂଲ୍ୟ, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, ସତ୍ୟ);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internWideWidth(uint32_t ମୂଲ୍ୟ) {
        ଅଚଳ auto pair = m_internedWideWidths.emplace(ମୂଲ୍ୟ, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internVertex(ଅଚଳ DfgVertex& vtx) {
        ଅଚଳ auto pair = m_internedVertices.emplace(&vtx, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଶୂନ୍ୟ recordMultiplicityAndDepth(ଅଚଳ DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& ମୂଲ୍ୟ = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        ମୂଲ୍ୟ.first += 1;
        ମୂଲ୍ୟ.second = std::max(ମୂଲ୍ୟ.second, depth);
        ଯଦି (!depth) ଫେରନ୍ତୁ;
        vtx.foreachSource([&](ଅଚଳ DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    ଶୂନ୍ୟ render(ଅଚଳ DfgVertex& vtx, uint32_t depth, ତାର୍କିକ is_ମୂଳ_2 = ସତ୍ୟ) {
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ଯଦି (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ଅନ୍ୟଥା ଯଦି (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ଅନ୍ୟଥା {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ଅନ୍ୟଥା ଯଦି (!is_ମୂଳ_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } ଅନ୍ୟଥା ଯଦି (!vtx.foreachSource([&](ଅଚଳ DfgVertex&) { ଫେରନ୍ତୁ ସତ୍ୟ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ଅନ୍ୟଥା ଯଦି (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ଅନ୍ୟଥା {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ଯଦି (ଅଚଳ DfgSel* ଅଚଳ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ଯଦି (selp->lsb() == 0) {
                    m_os << '0';
                } ଅନ୍ୟଥା {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ଅଚଳ DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, ମିଥ୍ୟା);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ଯଦି (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ଅନ୍ୟଥା {
            ଅଚଳ uint32_t width = vtx.size();
            ଯଦି (width == 1) {
                m_os << '1';
            } ଅନ୍ୟଥା ଯଦି (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ଅନ୍ୟଥା {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        ଯଦି (vtx.hasMultipleSinks()) m_os << '*';
    }

ସର୍ବସାଧାରଣ:
    DfgPatternString(std::ostream& os, ଅଚଳ DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, ମିଥ୍ୟା);
        using Pair = std::pair<std::string, ଅଚଳ DfgVertex*>;
        std::vector<Pair> vertices;
        ପାଇଁ (ଅଚଳ auto& pair : m_multiplicityAndDepth) {
            ଯଦି (pair.second.first == 1) ଜାରି;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ଅଚଳ Pair& a, ଅଚଳ Pair& b) {  //
            ଫେରନ୍ତୁ a.first < b.first;
        });
        ପାଇଁ (ଅଚଳ Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ଅଚଳ {
    std::ostringstream oss;
    DfgPatternString{oss, *ଏହି, depth};
    ଫେରନ୍ତୁ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ଅନ୍ତର୍ଭାଵ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
