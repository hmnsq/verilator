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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3DfgContext.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, ಧ್ರುವ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*ಏತತ್); });
}

ಶೂನ್ಯ DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    ಯದಿ (otherps.empty()) ಪ್ರತ್ಯಾಗಮ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ಧ್ರುವ VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    ಪ್ರತಿ (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    ಪ್ರತಿ (ಧ್ರುವ std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ಯದಿ (DfgVertexVar* ಧ್ರುವ altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
                DfgVertex* ಧ್ರುವ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ಯದಿ (srcp) altp->srcp(srcp);
                ಯದಿ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ಅನುವರ್ತ;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ಏತತ್;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        ಪ್ರತಿ (DfgVertexAst* ಧ್ರುವ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ಏತತ್;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        ಪ್ರತಿ (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ಏತತ್;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        ಪ್ರತಿ (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ಏತತ್;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_ಸಾಲು* flp, ಧ್ರುವ std::string& prefix,
                                   ಧ್ರುವ DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ಧ್ರುವ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    ಯದಿ (dtype.isArray()) ಪ್ರತ್ಯಾಗಮ ನೂತನ DfgVarArray{*ಏತತ್, vscp};
    ಪ್ರತ್ಯಾಗಮ ನೂತನ DfgVarPacked{*ಏತತ್, vscp};
}

ಸ್ಥಿರ ಧ್ರುವ std::string toDotId(ಧ್ರುವ DfgVertex& vtx) { ಪ್ರತ್ಯಾಗಮ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ಸ್ಥಿರ ಶೂನ್ಯ dumpDotVertex(std::ostream& os, ಧ್ರುವ DfgVertex& vtx) {
    ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varVtxp = vtx.cast<DfgVertexVar>()) {
        ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp = varVtxp->vscp();
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
        ಯದಿ (ಧ್ರುವ AstVarScope* ಧ್ರುವ tmpForp = varVtxp->tmpForp()) {
            ಯದಿ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ಅಸತ್ಯ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ಯದಿ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ಅನ್ಯಥಾ ಯದಿ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ಅನ್ಯಥಾ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (ಧ್ರುವ DfgPrev* ಧ್ರುವ prevVtxp = vtx.cast<DfgPrev>()) {
        ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp = prevVtxp->vscp();
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
        ಯದಿ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ಅನ್ಯಥಾ ಯದಿ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ಅನ್ಯಥಾ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constVtxp = vtx.cast<DfgConst>()) {
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constVtxp->ಅಂಕೆ();

        os << toDotId(vtx);
        os << " [label=\"";
        ಯದಿ (ಅಂಕೆ.width() <= 32 && !ಅಂಕೆ.isSigned()) {
            os << constVtxp->width() << "'d" << ಅಂಕೆ.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << ಅಂಕೆ.toUInt() << std::dec << '\n';
        } ಅನ್ಯಥಾ {
            os << ಅಂಕೆ.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ selVtxp = vtx.cast<DfgSel>()) {
        ಧ್ರುವ uint32_t lsb = selVtxp->lsb();
        ಧ್ರುವ uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ಯದಿ (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } ಅನ್ಯಥಾ {
            os << ", shape=circle";
        }
        os << "]\n";
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ಯದಿ (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } ಅನ್ಯಥಾ {
            os << ", shape=octagon";
        }
        os << "]\n";
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (ಧ್ರುವ DfgLogic* ಧ್ರುವ logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (ಧ್ರುವ DfgVertexAst* ಧ್ರುವ astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ಧ್ರುವ DfgAstRd* ಧ್ರುವ astRdVtxp = astVtxp->cast<DfgAstRd>();
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ colorp = astRdVtxp ? "#80ff80"  // Green
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
        ಪ್ರತ್ಯಾಗಮ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ಯದಿ (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } ಅನ್ಯಥಾ {
        os << ", shape=circle";
    }
    os << "]\n";
}

ಶೂನ್ಯ DfgGraph::dumpDot(std::ostream& os, ಧ್ರುವ std::string& label,
                       std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ DfgVertex&)> p) ಧ್ರುವ {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ಯದಿ (!p) p = [](ಧ್ರುವ DfgVertex&) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; };

    std::unordered_set<ಧ್ರುವ DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
        ಧ್ರುವ DfgLogic* ಧ್ರುವ logicp = vtx.cast<DfgLogic>();
        ಯದಿ (!logicp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (logicp->synth().empty()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!p(vtx)) ಪ್ರತ್ಯಾಗಮ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ಸೇರಿಸು(logicp);
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : logicp->synth()) {
            ಯದಿ (!p(*vtxp)) ಅನುವರ್ತ;
            dumpDotVertex(os, *vtxp);
            emitted.ಸೇರಿಸು(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
        ಯದಿ (emitted.ಎಣಿಕೆ(&vtx)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!p(vtx)) ಪ್ರತ್ಯಾಗಮ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
        ಯದಿ (!p(vtx)) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ಧ್ರುವ srcp = vtx.inputp(i);
            ಯದಿ (!srcp) ಅನುವರ್ತ;
            ಯದಿ (!p(*srcp)) ಅನುವರ್ತ;
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

std::string DfgGraph::dumpDotString(ಧ್ರುವ std::string& label,
                                    std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ DfgVertex&)> p) ಧ್ರುವ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ಪ್ರತ್ಯಾಗಮ ss.str();
}

ಶೂನ್ಯ DfgGraph::dumpDotFile(ಧ್ರುವ std::string& filename, ಧ್ರುವ std::string& label,
                           std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ DfgVertex&)> p) ಧ್ರುವ {
    ಧ್ರುವ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ಯದಿ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

ಶೂನ್ಯ DfgGraph::dumpDotFilePrefixed(ಧ್ರುವ std::string& label,
                                   std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ DfgVertex&)> p) ಧ್ರುವ {
    std::string filename = name();
    ಯದಿ (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <ತಾರ್ಕಿಕ T_SinksNotSources>
ಸ್ಥಿರ std::unique_ptr<std::unordered_set<ಧ್ರುವ DfgVertex*>>
dfgGraphCollectCone(ಧ್ರುವ std::vector<ಧ್ರುವ DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ಧ್ರುವ DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ಧ್ರುವ DfgVertex*>> resp{
        ನೂತನ std::unordered_set<ಧ್ರುವ DfgVertex*>{}};
    // Depth first traversal
    ಯಾವತ್ (!queue.empty()) {
        // Pop next work item
        ಧ್ರುವ DfgVertex* ಧ್ರುವ vtxp = queue.back();
        queue.ತೆಗೆ_back();
        // Mark vertex as visited, move on if already visited
        ಯದಿ (!resp->ಸೇರಿಸು(vtxp).second) ಅನುವರ್ತ;
        // Enqueue all siblings of this vertex.
        ಯದಿ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ಧ್ರುವ DfgVertex& sink) {
                queue.ತಳ್ಳು_back(&sink);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        } ಅನ್ಯಥಾ {
            vtxp->foreachSource([&](ಧ್ರುವ DfgVertex& src) {
                queue.ತಳ್ಳು_back(&src);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        }
    }
    // Done
    ಪ್ರತ್ಯಾಗಮ resp;
}

std::unique_ptr<std::unordered_set<ಧ್ರುವ DfgVertex*>>
DfgGraph::sourceCone(ಧ್ರುವ std::vector<ಧ್ರುವ DfgVertex*>& vtxps) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ dfgGraphCollectCone<ಅಸತ್ಯ>(vtxps);
}

std::unique_ptr<std::unordered_set<ಧ್ರುವ DfgVertex*>>
DfgGraph::sinkCone(ಧ್ರುವ std::vector<ಧ್ರುವ DfgVertex*>& vtxps) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ dfgGraphCollectCone<ಸತ್ಯ>(vtxps);
}

std::unique_ptr<std::unordered_set<ಧ್ರುವ DfgVertex*>>
DfgGraph::neighborhood(ಧ್ರುವ std::vector<ಧ್ರುವ DfgVertex*>& vtxps, size_t n) ಧ್ರುವ {
    // Neighborhood
    std::vector<ಧ್ರುವ DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ಧ್ರುವ DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    ಪ್ರತಿ (size_t hops = 1; hops <= n; ++hops) {
        ಪ್ರತಿ (size_t i = begin; i < end; ++i) {
            ಧ್ರುವ DfgVertex* ಧ್ರುವ vtxp = vec[i];
            vtxp->foreachSink([&](ಧ್ರುವ DfgVertex& dst) {
                ಯದಿ (res.ಸೇರಿಸು(&dst).second) vec.ತಳ್ಳು_back(&dst);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            vtxp->foreachSource([&](ಧ್ರುವ DfgVertex& src) {
                ಯದಿ (res.ಸೇರಿಸು(&src).second) vec.ತಳ್ಳು_back(&src);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    ಪ್ರತ್ಯಾಗಮ std::make_unique<std::unordered_set<ಧ್ರುವ DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*ಏತತ್);
}

ತಾರ್ಕಿಕ DfgVertex::unsafe() ಧ್ರುವ {
    ಯದಿ (is<DfgMux>()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    ಯದಿ (is<DfgArraySel>()) ಪ್ರತ್ಯಾಗಮ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

ಶೂನ್ಯ DfgVertex::typeCheck(ಧ್ರುವ DfgGraph& dfg) ಧ್ರುವ {

#define CHECK(cond, ಸಂದೇಶ) \
    UASSERT_OBJ(cond, ಏತತ್, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << ಸಂದೇಶ);
    ವಿಕಲ್ಪ (type()) {
    ಪಕ್ಷ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::AstRd: {
        ಧ್ರುವ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::VarArray:
    ಪಕ್ಷ VDfgType::VarPacked: {
        ಧ್ರುವ DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::SpliceArray:
    ಪಕ್ಷ VDfgType::SplicePacked: {
        ಧ್ರುವ DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ಧ್ರುವ DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::UnitArray: {
        ಧ್ರುವ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::Sel: {
        ಧ್ರುವ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::MatchMasked: {
        ಧ್ರುವ DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::Mux: {
        ಧ್ರುವ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಪಕ್ಷ VDfgType::ArraySel: {
        ಧ್ರುವ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Add:
    ಪಕ್ಷ VDfgType::And:
    ಪಕ್ಷ VDfgType::Div:
    ಪಕ್ಷ VDfgType::DivS:
    ಪಕ್ಷ VDfgType::ModDiv:
    ಪಕ್ಷ VDfgType::ModDivS:
    ಪಕ್ಷ VDfgType::Mul:
    ಪಕ್ಷ VDfgType::MulS:
    ಪಕ್ಷ VDfgType::Or:
    ಪಕ್ಷ VDfgType::Sub:
    ಪಕ್ಷ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Negate:
    ಪಕ್ಷ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::ShiftL:
    ಪಕ್ಷ VDfgType::ShiftR:
    ಪಕ್ಷ VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Concat: {
        ಧ್ರುವ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Rep: {
        ಧ್ರುವ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::StreamL:
    ಪಕ್ಷ VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Eq:
    ಪಕ್ಷ VDfgType::EqCase:
    ಪಕ್ಷ VDfgType::EqWild:
    ಪಕ್ಷ VDfgType::Neq:
    ಪಕ್ಷ VDfgType::NeqCase:
    ಪಕ್ಷ VDfgType::NeqWild:
    ಪಕ್ಷ VDfgType::Gt:
    ಪಕ್ಷ VDfgType::GtS:
    ಪಕ್ಷ VDfgType::Gte:
    ಪಕ್ಷ VDfgType::GteS:
    ಪಕ್ಷ VDfgType::Lt:
    ಪಕ್ಷ VDfgType::LtS:
    ಪಕ್ಷ VDfgType::Lte:
    ಪಕ್ಷ VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Extend:
    ಪಕ್ಷ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::LogAnd:
    ಪಕ್ಷ VDfgType::LogEq:
    ಪಕ್ಷ VDfgType::LogIf:
    ಪಕ್ಷ VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::LogNot:
    ಪಕ್ಷ VDfgType::RedAnd:
    ಪಕ್ಷ VDfgType::RedOr:
    ಪಕ್ಷ VDfgType::RedXor:
    ಪಕ್ಷ VDfgType::OneHot:
    ಪಕ್ಷ VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Cond: {
        ಧ್ರುವ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::ಎಣಿಕೆ_Ones_2: {
        ಧ್ರುವ Dfg_ಎಣಿಕೆ_Ones& v = *as<Dfg_ಎಣಿಕೆ_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಪಕ್ಷ VDfgType::Pow:
    ಪಕ್ಷ VDfgType::PowSS:
    ಪಕ್ಷ VDfgType::PowSU:
    ಪಕ್ಷ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ಪ್ರತ್ಯಾಗಮ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ಧ್ರುವ {
    uint32_t result = 0;
    foreachSink([&](ಧ್ರುವ DfgVertex&) {
        ++result;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    });
    ಪ್ರತ್ಯಾಗಮ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    ಯದಿ (DfgVertexVar* ಧ್ರುವ varp = ಏತತ್->cast<DfgVertexVar>()) ಪ್ರತ್ಯಾಗಮ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    ಏತತ್->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ಧ್ರುವ varp = sink.cast<DfgVertexVar>();
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Do not use it if value might differ from its drivers
        ಯದಿ (varp->isVolatile()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // First variable found
        ಯದಿ (!resp) {
            resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Prefer those variables that must be kept anyway
        ಯದಿ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ಯದಿ (!resp->hasExtRdRefs()) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ಯದಿ (!resp->hasDfgRefs()) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Prefer real variables over temporaries
        ಧ್ರುವ ತಾರ್ಕಿಕ resIsTemp = resp->vscp()->varp()->isTemp();
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsTemp = varp->vscp()->varp()->isTemp();
        ಯದಿ (resIsTemp != varIsTemp) {
            ಯದಿ (resIsTemp) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (!resp->tmpForp() != !varp->tmpForp()) {
            ಯದಿ (resp->tmpForp()) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Prefer the earlier one in source order
        ಧ್ರುವ File_ಸಾಲು& oldFlp = *(resp->fileline());
        ಧ್ರುವ File_ಸಾಲು& newFlp = *(varp->fileline());
        ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cmp = oldFlp.operatorCompare(newFlp)) {
            ಯದಿ (cmp > 0) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Prefer the one with the lexically smaller name
        ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ಯದಿ (cmp > 0) resp = varp;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    });
    ಪ್ರತ್ಯಾಗಮ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, ತಾರ್ಕಿಕ tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = ಏತತ್->cast<DfgVertexVar>()) {
        ಪ್ರತ್ಯಾಗಮ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ಯದಿ (tryResultVar) {
        ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = ಏತತ್->getResultVar()) {
            ಪ್ರತ್ಯಾಗಮ varp->vscp()->scopep();
        }
    }

    AstScope* ಧ್ರುವ rootp = v3Global.rootp()->ತುದಿ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ಏತತ್];
    ಯದಿ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ಧ್ರುವ scp = src.scopep(cache, ಸತ್ಯ);
            ಯದಿ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ಏತತ್,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ಪ್ರತ್ಯಾಗಮ resultr;
}

ಶೂನ್ಯ DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    ಯಾವತ್ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*ಏತತ್);
    // Delete - this will unlink sources
    delete ಏತತ್;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ಧ್ರುವ srcp = ಏತತ್->srcp();
    // Not driven at all
    ಯದಿ (!srcp) ಪ್ರತ್ಯಾಗಮ {nullptr, 0};
    // If volatile, can have other drivers
    ಯದಿ (isVolatile()) ಪ್ರತ್ಯಾಗಮ {nullptr, 0};
    // Don't inline CReset
    ಯದಿ (srcp->is<DfgCReset>()) ಪ್ರತ್ಯಾಗಮ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ಧ್ರುವ splicep = srcp->cast<DfgVertexSplice>();
    ಯದಿ (!splicep) ಪ್ರತ್ಯಾಗಮ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ಧ್ರುವ uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    ತಾರ್ಕಿಕ useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ಧ್ರುವ uint32_t dLo) {
        ಧ್ರುವ uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ಯದಿ (dLo <= hi && lo <= dHi) useDefault = ಅಸತ್ಯ;
        // If it does not cover the whole searched range, move on
        ಯದಿ (lo < dLo || dHi < hi) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    });

    // If a single driver covers the searched range, it is the one
    ಯದಿ (driverp) ಪ್ರತ್ಯಾಗಮ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ಯದಿ (useDefault) ಪ್ರತ್ಯಾಗಮ {defaultp(), lo};
    // Not driven by a single vertex
    ಪ್ರತ್ಯಾಗಮ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

ವರ್ಗ DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ಧ್ರುವ DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ಧ್ರುವ DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    ಸ್ಥಿರ std::string toLetters(size_t ಮೌಲ್ಯ, ತಾರ್ಕಿಕ lowerCase = ಅಸತ್ಯ) {
        ಧ್ರುವ ವರ್ಣ base = lowerCase ? 'a' : 'A';
        std::string s;
        ಕುರು { s += static_cast<ವರ್ಣ>(base + ಮೌಲ್ಯ % 26); } ಯಾವತ್ (ಮೌಲ್ಯ /= 26);
        ಪ್ರತ್ಯಾಗಮ s;
    }

    ಧ್ರುವ std::string& internConst(ಧ್ರುವ DfgConst& vtx) {
        ಧ್ರುವ auto pair = m_internedConsts.emplace(vtx.ಅಂಕೆ().ascii(ಅಸತ್ಯ), "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internSelLsb(uint32_t ಮೌಲ್ಯ) {
        ಧ್ರುವ auto pair = m_internedSelLsbs.emplace(ಮೌಲ್ಯ, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internWordWidth(uint32_t ಮೌಲ್ಯ) {
        ಧ್ರುವ auto pair = m_internedWordWidths.emplace(ಮೌಲ್ಯ, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internWideWidth(uint32_t ಮೌಲ್ಯ) {
        ಧ್ರುವ auto pair = m_internedWideWidths.emplace(ಮೌಲ್ಯ, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internVertex(ಧ್ರುವ DfgVertex& vtx) {
        ಧ್ರುವ auto pair = m_internedVertices.emplace(&vtx, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಶೂನ್ಯ recordMultiplicityAndDepth(ಧ್ರುವ DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& ಮೌಲ್ಯ = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        ಮೌಲ್ಯ.first += 1;
        ಮೌಲ್ಯ.second = std::max(ಮೌಲ್ಯ.second, depth);
        ಯದಿ (!depth) ಪ್ರತ್ಯಾಗಮ;
        vtx.foreachSource([&](ಧ್ರುವ DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    ಶೂನ್ಯ render(ಧ್ರುವ DfgVertex& vtx, uint32_t depth, ತಾರ್ಕಿಕ is_ಬೇರು_2 = ಸತ್ಯ) {
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ಯದಿ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ಅನ್ಯಥಾ ಯದಿ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ಅನ್ಯಥಾ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ಅನ್ಯಥಾ ಯದಿ (!is_ಬೇರು_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } ಅನ್ಯಥಾ ಯದಿ (!vtx.foreachSource([&](ಧ್ರುವ DfgVertex&) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ಅನ್ಯಥಾ ಯದಿ (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ಅನ್ಯಥಾ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ಯದಿ (selp->lsb() == 0) {
                    m_os << '0';
                } ಅನ್ಯಥಾ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ಧ್ರುವ DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, ಅಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ಯದಿ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ uint32_t width = vtx.size();
            ಯದಿ (width == 1) {
                m_os << '1';
            } ಅನ್ಯಥಾ ಯದಿ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ಅನ್ಯಥಾ {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        ಯದಿ (vtx.hasMultipleSinks()) m_os << '*';
    }

ಸಾರ್ವಜನಿಕ:
    DfgPatternString(std::ostream& os, ಧ್ರುವ DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, ಅಸತ್ಯ);
        using Pair = std::pair<std::string, ಧ್ರುವ DfgVertex*>;
        std::vector<Pair> vertices;
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_multiplicityAndDepth) {
            ಯದಿ (pair.second.first == 1) ಅನುವರ್ತ;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ಧ್ರುವ Pair& a, ಧ್ರುವ Pair& b) {  //
            ಪ್ರತ್ಯಾಗಮ a.first < b.first;
        });
        ಪ್ರತಿ (ಧ್ರುವ Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ಧ್ರುವ {
    std::ostringstream oss;
    DfgPatternString{oss, *ಏತತ್, depth};
    ಪ್ರತ್ಯಾಗಮ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ಅನ್ತರ್ಭಾವ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
