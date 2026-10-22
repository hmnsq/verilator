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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3DfgContext.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, സ്ഥിരം string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*ഇത്); });
}

ശൂന്യം DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    എങ്കിൽ (otherps.empty()) തിരികെ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    സ്ഥിരം VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    വേണ്ടി (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    വേണ്ടി (സ്ഥിരം std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        വേണ്ടി (DfgVertexVar* സ്ഥിരം vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            എങ്കിൽ (DfgVertexVar* സ്ഥിരം altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* സ്ഥിരം srcp = vtxp->srcp();
                DfgVertex* സ്ഥിരം defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                എങ്കിൽ (srcp) altp->srcp(srcp);
                എങ്കിൽ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                തുടരുക;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ഇത്;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        വേണ്ടി (DfgVertexAst* സ്ഥിരം vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = ഇത്;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        വേണ്ടി (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ഇത്;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        വേണ്ടി (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = ഇത്;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_വരി* flp, സ്ഥിരം std::string& prefix,
                                   സ്ഥിരം DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* സ്ഥിരം vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    എങ്കിൽ (dtype.isArray()) തിരികെ പുതിയ DfgVarArray{*ഇത്, vscp};
    തിരികെ പുതിയ DfgVarPacked{*ഇത്, vscp};
}

സ്ഥിതം സ്ഥിരം std::string toDotId(സ്ഥിരം DfgVertex& vtx) { തിരികെ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
സ്ഥിതം ശൂന്യം dumpDotVertex(std::ostream& os, സ്ഥിരം DfgVertex& vtx) {
    എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varVtxp = vtx.cast<DfgVertexVar>()) {
        സ്ഥിരം AstVarScope* സ്ഥിരം vscp = varVtxp->vscp();
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
        എങ്കിൽ (സ്ഥിരം AstVarScope* സ്ഥിരം tmpForp = varVtxp->tmpForp()) {
            എങ്കിൽ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[അസത്യം][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        എങ്കിൽ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } അല്ലെങ്കിൽ എങ്കിൽ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } അല്ലെങ്കിൽ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        സ്ഥിരം അക്ഷരം* സ്ഥിരം colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        തിരികെ;
    }

    എങ്കിൽ (സ്ഥിരം DfgPrev* സ്ഥിരം prevVtxp = vtx.cast<DfgPrev>()) {
        സ്ഥിരം AstVarScope* സ്ഥിരം vscp = prevVtxp->vscp();
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
        എങ്കിൽ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } അല്ലെങ്കിൽ എങ്കിൽ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } അല്ലെങ്കിൽ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        സ്ഥിരം അക്ഷരം* സ്ഥിരം colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        തിരികെ;
    }

    എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constVtxp = vtx.cast<DfgConst>()) {
        സ്ഥിരം V3_സംഖ്യ& അക്കം = constVtxp->അക്കം();

        os << toDotId(vtx);
        os << " [label=\"";
        എങ്കിൽ (അക്കം.width() <= 32 && !അക്കം.isSigned()) {
            os << constVtxp->width() << "'d" << അക്കം.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << അക്കം.toUInt() << std::dec << '\n';
        } അല്ലെങ്കിൽ {
            os << അക്കം.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        തിരികെ;
    }

    എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം selVtxp = vtx.cast<DfgSel>()) {
        സ്ഥിരം uint32_t lsb = selVtxp->lsb();
        സ്ഥിരം uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        എങ്കിൽ (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } അല്ലെങ്കിൽ {
            os << ", shape=circle";
        }
        os << "]\n";
        തിരികെ;
    }

    എങ്കിൽ (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        എങ്കിൽ (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } അല്ലെങ്കിൽ {
            os << ", shape=octagon";
        }
        os << "]\n";
        തിരികെ;
    }

    എങ്കിൽ (സ്ഥിരം DfgLogic* സ്ഥിരം logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_വൃക്ഷം(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        സ്ഥിരം അക്ഷരം* സ്ഥിരം colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        തിരികെ;
    }

    എങ്കിൽ (സ്ഥിരം DfgVertexAst* സ്ഥിരം astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_വൃക്ഷം(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        സ്ഥിരം DfgAstRd* സ്ഥിരം astRdVtxp = astVtxp->cast<DfgAstRd>();
        സ്ഥിരം അക്ഷരം* സ്ഥിരം colorp = astRdVtxp ? "#80ff80"  // Green
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
        തിരികെ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    എങ്കിൽ (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } അല്ലെങ്കിൽ {
        os << ", shape=circle";
    }
    os << "]\n";
}

ശൂന്യം DfgGraph::dumpDot(std::ostream& os, സ്ഥിരം std::string& label,
                       std::function<തർക്കം(സ്ഥിരം DfgVertex&)> p) സ്ഥിരം {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    എങ്കിൽ (!p) p = [](സ്ഥിരം DfgVertex&) { തിരികെ സത്യം; };

    std::unordered_set<സ്ഥിരം DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
        സ്ഥിരം DfgLogic* സ്ഥിരം logicp = vtx.cast<DfgLogic>();
        എങ്കിൽ (!logicp) തിരികെ;
        എങ്കിൽ (logicp->synth().empty()) തിരികെ;
        എങ്കിൽ (!p(vtx)) തിരികെ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.തിരുകുക(logicp);
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : logicp->synth()) {
            എങ്കിൽ (!p(*vtxp)) തുടരുക;
            dumpDotVertex(os, *vtxp);
            emitted.തിരുകുക(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
        എങ്കിൽ (emitted.എണ്ണം(&vtx)) തിരികെ;
        എങ്കിൽ (!p(vtx)) തിരികെ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
        എങ്കിൽ (!p(vtx)) തിരികെ;
        വേണ്ടി (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* സ്ഥിരം srcp = vtx.inputp(i);
            എങ്കിൽ (!srcp) തുടരുക;
            എങ്കിൽ (!p(*srcp)) തുടരുക;
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

std::string DfgGraph::dumpDotString(സ്ഥിരം std::string& label,
                                    std::function<തർക്കം(സ്ഥിരം DfgVertex&)> p) സ്ഥിരം {
    std::stringstream ss;
    dumpDot(ss, label, p);
    തിരികെ ss.str();
}

ശൂന്യം DfgGraph::dumpDotFile(സ്ഥിരം std::string& filename, സ്ഥിരം std::string& label,
                           std::function<തർക്കം(സ്ഥിരം DfgVertex&)> p) സ്ഥിരം {
    സ്ഥിരം std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    എങ്കിൽ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

ശൂന്യം DfgGraph::dumpDotFilePrefixed(സ്ഥിരം std::string& label,
                                   std::function<തർക്കം(സ്ഥിരം DfgVertex&)> p) സ്ഥിരം {
    std::string filename = name();
    എങ്കിൽ (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <തർക്കം T_SinksNotSources>
സ്ഥിതം std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>>
dfgGraphCollectCone(സ്ഥിരം std::vector<സ്ഥിരം DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<സ്ഥിരം DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>> resp{
        പുതിയ std::unordered_set<സ്ഥിരം DfgVertex*>{}};
    // Depth first traversal
    ആകുമ്പോൾ (!queue.empty()) {
        // Pop next work item
        സ്ഥിരം DfgVertex* സ്ഥിരം vtxp = queue.back();
        queue.എടുക്കുക_back();
        // Mark vertex as visited, move on if already visited
        എങ്കിൽ (!resp->തിരുകുക(vtxp).second) തുടരുക;
        // Enqueue all siblings of this vertex.
        എങ്കിൽ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](സ്ഥിരം DfgVertex& sink) {
                queue.തള്ളുക_back(&sink);
                തിരികെ അസത്യം;
            });
        } അല്ലെങ്കിൽ {
            vtxp->foreachSource([&](സ്ഥിരം DfgVertex& src) {
                queue.തള്ളുക_back(&src);
                തിരികെ അസത്യം;
            });
        }
    }
    // Done
    തിരികെ resp;
}

std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>>
DfgGraph::sourceCone(സ്ഥിരം std::vector<സ്ഥിരം DfgVertex*>& vtxps) സ്ഥിരം {
    തിരികെ dfgGraphCollectCone<അസത്യം>(vtxps);
}

std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>>
DfgGraph::sinkCone(സ്ഥിരം std::vector<സ്ഥിരം DfgVertex*>& vtxps) സ്ഥിരം {
    തിരികെ dfgGraphCollectCone<സത്യം>(vtxps);
}

std::unique_ptr<std::unordered_set<സ്ഥിരം DfgVertex*>>
DfgGraph::neighborhood(സ്ഥിരം std::vector<സ്ഥിരം DfgVertex*>& vtxps, size_t n) സ്ഥിരം {
    // Neighborhood
    std::vector<സ്ഥിരം DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<സ്ഥിരം DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    വേണ്ടി (size_t hops = 1; hops <= n; ++hops) {
        വേണ്ടി (size_t i = begin; i < end; ++i) {
            സ്ഥിരം DfgVertex* സ്ഥിരം vtxp = vec[i];
            vtxp->foreachSink([&](സ്ഥിരം DfgVertex& dst) {
                എങ്കിൽ (res.തിരുകുക(&dst).second) vec.തള്ളുക_back(&dst);
                തിരികെ അസത്യം;
            });
            vtxp->foreachSource([&](സ്ഥിരം DfgVertex& src) {
                എങ്കിൽ (res.തിരുകുക(&src).second) vec.തള്ളുക_back(&src);
                തിരികെ അസത്യം;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    തിരികെ std::make_unique<std::unordered_set<സ്ഥിരം DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_വരി* flp, സ്ഥിരം DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*ഇത്);
}

തർക്കം DfgVertex::unsafe() സ്ഥിരം {
    എങ്കിൽ (is<DfgMux>()) തിരികെ സത്യം;
    എങ്കിൽ (is<DfgArraySel>()) തിരികെ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    തിരികെ അസത്യം;
}

ശൂന്യം DfgVertex::typeCheck(സ്ഥിരം DfgGraph& dfg) സ്ഥിരം {

#define CHECK(cond, സന്ദേശം) \
    UASSERT_OBJ(cond, ഇത്, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << സന്ദേശം);
    തിരഞ്ഞെടുപ്പ് (type()) {
    അവസ്ഥ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::AstRd: {
        സ്ഥിരം DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::VarArray:
    അവസ്ഥ VDfgType::VarPacked: {
        സ്ഥിരം DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::SpliceArray:
    അവസ്ഥ VDfgType::SplicePacked: {
        സ്ഥിരം DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](സ്ഥിരം DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            തിരികെ അസത്യം;
        });
        തിരികെ;
    }
    അവസ്ഥ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::UnitArray: {
        സ്ഥിരം DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::Sel: {
        സ്ഥിരം DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::MatchMasked: {
        സ്ഥിരം DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::Mux: {
        സ്ഥിരം DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        തിരികെ;
    }
    അവസ്ഥ VDfgType::ArraySel: {
        സ്ഥിരം DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Add:
    അവസ്ഥ VDfgType::And:
    അവസ്ഥ VDfgType::Div:
    അവസ്ഥ VDfgType::DivS:
    അവസ്ഥ VDfgType::ModDiv:
    അവസ്ഥ VDfgType::ModDivS:
    അവസ്ഥ VDfgType::Mul:
    അവസ്ഥ VDfgType::MulS:
    അവസ്ഥ VDfgType::Or:
    അവസ്ഥ VDfgType::Sub:
    അവസ്ഥ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Negate:
    അവസ്ഥ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::ShiftL:
    അവസ്ഥ VDfgType::ShiftR:
    അവസ്ഥ VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Concat: {
        സ്ഥിരം DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Rep: {
        സ്ഥിരം DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::StreamL:
    അവസ്ഥ VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Eq:
    അവസ്ഥ VDfgType::EqCase:
    അവസ്ഥ VDfgType::EqWild:
    അവസ്ഥ VDfgType::Neq:
    അവസ്ഥ VDfgType::NeqCase:
    അവസ്ഥ VDfgType::NeqWild:
    അവസ്ഥ VDfgType::Gt:
    അവസ്ഥ VDfgType::GtS:
    അവസ്ഥ VDfgType::Gte:
    അവസ്ഥ VDfgType::GteS:
    അവസ്ഥ VDfgType::Lt:
    അവസ്ഥ VDfgType::LtS:
    അവസ്ഥ VDfgType::Lte:
    അവസ്ഥ VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Extend:
    അവസ്ഥ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::LogAnd:
    അവസ്ഥ VDfgType::LogEq:
    അവസ്ഥ VDfgType::LogIf:
    അവസ്ഥ VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::LogNot:
    അവസ്ഥ VDfgType::RedAnd:
    അവസ്ഥ VDfgType::RedOr:
    അവസ്ഥ VDfgType::RedXor:
    അവസ്ഥ VDfgType::OneHot:
    അവസ്ഥ VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Cond: {
        സ്ഥിരം DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::എണ്ണം_Ones_2: {
        സ്ഥിരം Dfg_എണ്ണം_Ones& v = *as<Dfg_എണ്ണം_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        തിരികെ;
    }

    അവസ്ഥ VDfgType::Pow:
    അവസ്ഥ VDfgType::PowSS:
    അവസ്ഥ VDfgType::PowSU:
    അവസ്ഥ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        തിരികെ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() സ്ഥിരം {
    uint32_t result = 0;
    foreachSink([&](സ്ഥിരം DfgVertex&) {
        ++result;
        തിരികെ അസത്യം;
    });
    തിരികെ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    എങ്കിൽ (DfgVertexVar* സ്ഥിരം varp = ഇത്->cast<DfgVertexVar>()) തിരികെ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    ഇത്->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* സ്ഥിരം varp = sink.cast<DfgVertexVar>();
        എങ്കിൽ (!varp) തിരികെ അസത്യം;

        // Do not use it if value might differ from its drivers
        എങ്കിൽ (varp->isVolatile()) തിരികെ അസത്യം;

        // First variable found
        എങ്കിൽ (!resp) {
            resp = varp;
            തിരികെ അസത്യം;
        }

        // Prefer those variables that must be kept anyway
        എങ്കിൽ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            എങ്കിൽ (!resp->hasExtRdRefs()) resp = varp;
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            എങ്കിൽ (!resp->hasDfgRefs()) resp = varp;
            തിരികെ അസത്യം;
        }

        // Prefer real variables over temporaries
        സ്ഥിരം തർക്കം resIsTemp = resp->vscp()->varp()->isTemp();
        സ്ഥിരം തർക്കം varIsTemp = varp->vscp()->varp()->isTemp();
        എങ്കിൽ (resIsTemp != varIsTemp) {
            എങ്കിൽ (resIsTemp) resp = varp;
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (!resp->tmpForp() != !varp->tmpForp()) {
            എങ്കിൽ (resp->tmpForp()) resp = varp;
            തിരികെ അസത്യം;
        }

        // Prefer the earlier one in source order
        സ്ഥിരം File_വരി& oldFlp = *(resp->fileline());
        സ്ഥിരം File_വരി& newFlp = *(varp->fileline());
        എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ cmp = oldFlp.operatorCompare(newFlp)) {
            എങ്കിൽ (cmp > 0) resp = varp;
            തിരികെ അസത്യം;
        }
        // Prefer the one with the lexically smaller name
        എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            എങ്കിൽ (cmp > 0) resp = varp;
            തിരികെ അസത്യം;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        തിരികെ അസത്യം;
    });
    തിരികെ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, തർക്കം tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = ഇത്->cast<DfgVertexVar>()) {
        തിരികെ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    എങ്കിൽ (tryResultVar) {
        എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = ഇത്->getResultVar()) {
            തിരികെ varp->vscp()->scopep();
        }
    }

    AstScope* സ്ഥിരം rootp = v3Global.rootp()->മുകൾ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[ഇത്];
    എങ്കിൽ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* സ്ഥിരം scp = src.scopep(cache, സത്യം);
            എങ്കിൽ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                തിരികെ സത്യം;
            }
            തിരികെ അസത്യം;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), ഇത്,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    തിരികെ resultr;
}

ശൂന്യം DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    ആകുമ്പോൾ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*ഇത്);
    // Delete - this will unlink sources
    delete ഇത്;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* സ്ഥിരം srcp = ഇത്->srcp();
    // Not driven at all
    എങ്കിൽ (!srcp) തിരികെ {nullptr, 0};
    // If volatile, can have other drivers
    എങ്കിൽ (isVolatile()) തിരികെ {nullptr, 0};
    // Don't inline CReset
    എങ്കിൽ (srcp->is<DfgCReset>()) തിരികെ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* സ്ഥിരം splicep = srcp->cast<DfgVertexSplice>();
    എങ്കിൽ (!splicep) തിരികെ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    സ്ഥിരം uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    തർക്കം useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, സ്ഥിരം uint32_t dLo) {
        സ്ഥിരം uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        എങ്കിൽ (dLo <= hi && lo <= dHi) useDefault = അസത്യം;
        // If it does not cover the whole searched range, move on
        എങ്കിൽ (lo < dLo || dHi < hi) തിരികെ അസത്യം;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        തിരികെ സത്യം;
    });

    // If a single driver covers the searched range, it is the one
    എങ്കിൽ (driverp) തിരികെ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    എങ്കിൽ (useDefault) തിരികെ {defaultp(), lo};
    // Not driven by a single vertex
    തിരികെ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

വർഗ്ഗം DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<സ്ഥിരം DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<സ്ഥിരം DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    സ്ഥിതം std::string toLetters(size_t മൂല്യം, തർക്കം lowerCase = അസത്യം) {
        സ്ഥിരം അക്ഷരം base = lowerCase ? 'a' : 'A';
        std::string s;
        ചെയ്യുക { s += static_cast<അക്ഷരം>(base + മൂല്യം % 26); } ആകുമ്പോൾ (മൂല്യം /= 26);
        തിരികെ s;
    }

    സ്ഥിരം std::string& internConst(സ്ഥിരം DfgConst& vtx) {
        സ്ഥിരം auto pair = m_internedConsts.emplace(vtx.അക്കം().ascii(അസത്യം), "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internSelLsb(uint32_t മൂല്യം) {
        സ്ഥിരം auto pair = m_internedSelLsbs.emplace(മൂല്യം, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internWordWidth(uint32_t മൂല്യം) {
        സ്ഥിരം auto pair = m_internedWordWidths.emplace(മൂല്യം, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, സത്യം);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internWideWidth(uint32_t മൂല്യം) {
        സ്ഥിരം auto pair = m_internedWideWidths.emplace(മൂല്യം, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internVertex(സ്ഥിരം DfgVertex& vtx) {
        സ്ഥിരം auto pair = m_internedVertices.emplace(&vtx, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        തിരികെ pair.first->second;
    }

    ശൂന്യം recordMultiplicityAndDepth(സ്ഥിരം DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& മൂല്യം = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        മൂല്യം.first += 1;
        മൂല്യം.second = std::max(മൂല്യം.second, depth);
        എങ്കിൽ (!depth) തിരികെ;
        vtx.foreachSource([&](സ്ഥിരം DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            തിരികെ അസത്യം;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    ശൂന്യം render(സ്ഥിരം DfgVertex& vtx, uint32_t depth, തർക്കം is_വേര്_2 = സത്യം) {
        എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            എങ്കിൽ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } അല്ലെങ്കിൽ എങ്കിൽ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } അല്ലെങ്കിൽ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (!is_വേര്_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } അല്ലെങ്കിൽ എങ്കിൽ (!vtx.foreachSource([&](സ്ഥിരം DfgVertex&) { തിരികെ സത്യം; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } അല്ലെങ്കിൽ എങ്കിൽ (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } അല്ലെങ്കിൽ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                എങ്കിൽ (selp->lsb() == 0) {
                    m_os << '0';
                } അല്ലെങ്കിൽ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](സ്ഥിരം DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, അസത്യം);
                തിരികെ അസത്യം;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        എങ്കിൽ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } അല്ലെങ്കിൽ {
            സ്ഥിരം uint32_t width = vtx.size();
            എങ്കിൽ (width == 1) {
                m_os << '1';
            } അല്ലെങ്കിൽ എങ്കിൽ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } അല്ലെങ്കിൽ {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        എങ്കിൽ (vtx.hasMultipleSinks()) m_os << '*';
    }

പൊതു:
    DfgPatternString(std::ostream& os, സ്ഥിരം DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, അസത്യം);
        using Pair = std::pair<std::string, സ്ഥിരം DfgVertex*>;
        std::vector<Pair> vertices;
        വേണ്ടി (സ്ഥിരം auto& pair : m_multiplicityAndDepth) {
            എങ്കിൽ (pair.second.first == 1) തുടരുക;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](സ്ഥിരം Pair& a, സ്ഥിരം Pair& b) {  //
            തിരികെ a.first < b.first;
        });
        വേണ്ടി (സ്ഥിരം Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) സ്ഥിരം {
    std::ostringstream oss;
    DfgPatternString{oss, *ഇത്, depth};
    തിരികെ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#അന്തര്ഭാവ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
