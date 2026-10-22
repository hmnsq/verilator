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

DfgGraph::DfgGraph(V3DfgContext& ctx, ثابت string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*هذا); });
}

فراغ DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    ܐܢ (otherps.empty()) ܦܢܐ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    ثابت VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    لكل (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    لكل (ثابت std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        لكل (DfgVertexVar* ثابت vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ܐܢ (DfgVertexVar* ثابت altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* ثابت srcp = vtxp->srcp();
                DfgVertex* ثابت defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ܐܢ (srcp) altp->srcp(srcp);
                ܐܢ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                استمر;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = هذا;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        لكل (DfgVertexAst* ثابت vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = هذا;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        لكل (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = هذا;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        لكل (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = هذا;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(FileLine* flp, ثابت std::string& prefix,
                                   ثابت DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* ثابت vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    ܐܢ (dtype.isArray()) ܦܢܐ جديد DfgVarArray{*هذا, vscp};
    ܦܢܐ جديد DfgVarPacked{*هذا, vscp};
}

ساكن ثابت std::string toDotId(ثابت DfgVertex& vtx) { ܦܢܐ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
ساكن فراغ dumpDotVertex(std::ostream& os, ثابت DfgVertex& vtx) {
    ܐܢ (ثابت DfgVertexVar* ثابت varVtxp = vtx.cast<DfgVertexVar>()) {
        ثابت AstVarScope* ثابت vscp = varVtxp->vscp();
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
        ܐܢ (ثابت AstVarScope* ثابت tmpForp = varVtxp->tmpForp()) {
            ܐܢ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        ساكن ثابت حرف* ثابت rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ܕܓܠܐ][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ܐܢ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ܐܚܪܢܐ ܐܢ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ܐܚܪܢܐ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        ثابت حرف* ثابت colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ܦܢܐ;
    }

    ܐܢ (ثابت DfgPrev* ثابت prevVtxp = vtx.cast<DfgPrev>()) {
        ثابت AstVarScope* ثابت vscp = prevVtxp->vscp();
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
        ܐܢ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ܐܚܪܢܐ ܐܢ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ܐܚܪܢܐ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        ثابت حرف* ثابت colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ܦܢܐ;
    }

    ܐܢ (ثابت DfgConst* ثابت constVtxp = vtx.cast<DfgConst>()) {
        ثابت V3Number& num = constVtxp->num();

        os << toDotId(vtx);
        os << " [label=\"";
        ܐܢ (num.width() <= 32 && !num.isSigned()) {
            os << constVtxp->width() << "'d" << num.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << num.toUInt() << std::dec << '\n';
        } ܐܚܪܢܐ {
            os << num.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ܦܢܐ;
    }

    ܐܢ (ثابت DfgSel* ثابت selVtxp = vtx.cast<DfgSel>()) {
        ثابت uint32_t lsb = selVtxp->lsb();
        ثابت uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ܐܢ (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } ܐܚܪܢܐ {
            os << ", shape=circle";
        }
        os << "]\n";
        ܦܢܐ;
    }

    ܐܢ (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ܐܢ (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } ܐܚܪܢܐ {
            os << ", shape=octagon";
        }
        os << "]\n";
        ܦܢܐ;
    }

    ܐܢ (ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت حرف* ثابت colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        ܦܢܐ;
    }

    ܐܢ (ثابت DfgVertexAst* ثابت astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        ثابت DfgAstRd* ثابت astRdVtxp = astVtxp->cast<DfgAstRd>();
        ثابت حرف* ثابت colorp = astRdVtxp ? "#80ff80"  // Green
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
        ܦܢܐ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ܐܢ (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } ܐܚܪܢܐ {
        os << ", shape=circle";
    }
    os << "]\n";
}

فراغ DfgGraph::dumpDot(std::ostream& os, ثابت std::string& label,
                       std::function<منطقي(ثابت DfgVertex&)> p) ثابت {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ܐܢ (!p) p = [](ثابت DfgVertex&) { ܦܢܐ ܫܪܪܐ; };

    std::unordered_set<ثابت DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](ثابت DfgVertex& vtx) {
        ثابت DfgLogic* ثابت logicp = vtx.cast<DfgLogic>();
        ܐܢ (!logicp) ܦܢܐ;
        ܐܢ (logicp->synth().empty()) ܦܢܐ;
        ܐܢ (!p(vtx)) ܦܢܐ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.insert(logicp);
        لكل (DfgVertex* ثابت vtxp : logicp->synth()) {
            ܐܢ (!p(*vtxp)) استمر;
            dumpDotVertex(os, *vtxp);
            emitted.insert(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](ثابت DfgVertex& vtx) {
        ܐܢ (emitted.count(&vtx)) ܦܢܐ;
        ܐܢ (!p(vtx)) ܦܢܐ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](ثابت DfgVertex& vtx) {
        ܐܢ (!p(vtx)) ܦܢܐ;
        لكل (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* ثابت srcp = vtx.inputp(i);
            ܐܢ (!srcp) استمر;
            ܐܢ (!p(*srcp)) استمر;
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

std::string DfgGraph::dumpDotString(ثابت std::string& label,
                                    std::function<منطقي(ثابت DfgVertex&)> p) ثابت {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ܦܢܐ ss.str();
}

فراغ DfgGraph::dumpDotFile(ثابت std::string& filename, ثابت std::string& label,
                           std::function<منطقي(ثابت DfgVertex&)> p) ثابت {
    ثابت std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ܐܢ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

فراغ DfgGraph::dumpDotFilePrefixed(ثابت std::string& label,
                                   std::function<منطقي(ثابت DfgVertex&)> p) ثابت {
    std::string filename = name();
    ܐܢ (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <منطقي T_SinksNotSources>
ساكن std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
dfgGraphCollectCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<ثابت DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<ثابت DfgVertex*>> resp{
        جديد std::unordered_set<ثابت DfgVertex*>{}};
    // Depth first traversal
    ܟܡܐ (!queue.empty()) {
        // Pop next work item
        ثابت DfgVertex* ثابت vtxp = queue.back();
        queue.pop_back();
        // Mark vertex as visited, move on if already visited
        ܐܢ (!resp->insert(vtxp).second) استمر;
        // Enqueue all siblings of this vertex.
        ܐܢ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](ثابت DfgVertex& sink) {
                queue.push_back(&sink);
                ܦܢܐ ܕܓܠܐ;
            });
        } ܐܚܪܢܐ {
            vtxp->foreachSource([&](ثابت DfgVertex& src) {
                queue.push_back(&src);
                ܦܢܐ ܕܓܠܐ;
            });
        }
    }
    // Done
    ܦܢܐ resp;
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::sourceCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) ثابت {
    ܦܢܐ dfgGraphCollectCone<ܕܓܠܐ>(vtxps);
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::sinkCone(ثابت std::vector<ثابت DfgVertex*>& vtxps) ثابت {
    ܦܢܐ dfgGraphCollectCone<ܫܪܪܐ>(vtxps);
}

std::unique_ptr<std::unordered_set<ثابت DfgVertex*>>
DfgGraph::neighborhood(ثابت std::vector<ثابت DfgVertex*>& vtxps, size_t n) ثابت {
    // Neighborhood
    std::vector<ثابت DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<ثابت DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    لكل (size_t hops = 1; hops <= n; ++hops) {
        لكل (size_t i = begin; i < end; ++i) {
            ثابت DfgVertex* ثابت vtxp = vec[i];
            vtxp->foreachSink([&](ثابت DfgVertex& dst) {
                ܐܢ (res.insert(&dst).second) vec.push_back(&dst);
                ܦܢܐ ܕܓܠܐ;
            });
            vtxp->foreachSource([&](ثابت DfgVertex& src) {
                ܐܢ (res.insert(&src).second) vec.push_back(&src);
                ܦܢܐ ܕܓܠܐ;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    ܦܢܐ std::make_unique<std::unordered_set<ثابت DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, FileLine* flp, ثابت DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*هذا);
}

منطقي DfgVertex::unsafe() ثابت {
    ܐܢ (is<DfgMux>()) ܦܢܐ ܫܪܪܐ;
    ܐܢ (is<DfgArraySel>()) ܦܢܐ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ܦܢܐ ܕܓܠܐ;
}

فراغ DfgVertex::typeCheck(ثابت DfgGraph& dfg) ثابت {

#define CHECK(cond, msg) \
    UASSERT_OBJ(cond, هذا, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << msg);
    بدل (type()) {
    حالة VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ܦܢܐ;
    }
    حالة VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ܦܢܐ;
    }
    حالة VDfgType::AstRd: {
        ثابت DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ܦܢܐ;
    }
    حالة VDfgType::VarArray:
    حالة VDfgType::VarPacked: {
        ثابت DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ܦܢܐ;
    }
    حالة VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ܦܢܐ;
    }
    حالة VDfgType::SpliceArray:
    حالة VDfgType::SplicePacked: {
        ثابت DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](ثابت DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            ܦܢܐ ܕܓܠܐ;
        });
        ܦܢܐ;
    }
    حالة VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ܦܢܐ;
    }
    حالة VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ܦܢܐ;
    }
    حالة VDfgType::UnitArray: {
        ثابت DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ܦܢܐ;
    }
    حالة VDfgType::Sel: {
        ثابت DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        ܦܢܐ;
    }
    حالة VDfgType::MatchMasked: {
        ثابت DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ܦܢܐ;
    }
    حالة VDfgType::Mux: {
        ثابت DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ܦܢܐ;
    }
    حالة VDfgType::ArraySel: {
        ثابت DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ܦܢܐ;
    }

    حالة VDfgType::Add:
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
        ܦܢܐ;
    }

    حالة VDfgType::Negate:
    حالة VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ܦܢܐ;
    }

    حالة VDfgType::ShiftL:
    حالة VDfgType::ShiftR:
    حالة VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ܦܢܐ;
    }

    حالة VDfgType::Concat: {
        ثابت DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        ܦܢܐ;
    }

    حالة VDfgType::Rep: {
        ثابت DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        ܦܢܐ;
    }

    حالة VDfgType::StreamL:
    حالة VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ܦܢܐ;
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
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ܦܢܐ;
    }

    حالة VDfgType::Extend:
    حالة VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        ܦܢܐ;
    }

    حالة VDfgType::LogAnd:
    حالة VDfgType::LogEq:
    حالة VDfgType::LogIf:
    حالة VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ܦܢܐ;
    }

    حالة VDfgType::LogNot:
    حالة VDfgType::RedAnd:
    حالة VDfgType::RedOr:
    حالة VDfgType::RedXor:
    حالة VDfgType::OneHot:
    حالة VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ܦܢܐ;
    }

    حالة VDfgType::Cond: {
        ثابت DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ܦܢܐ;
    }

    حالة VDfgType::CountOnes: {
        ثابت DfgCountOnes& v = *as<DfgCountOnes>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        ܦܢܐ;
    }

    حالة VDfgType::Pow:
    حالة VDfgType::PowSS:
    حالة VDfgType::PowSU:
    حالة VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ܦܢܐ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() ثابت {
    uint32_t result = 0;
    foreachSink([&](ثابت DfgVertex&) {
        ++result;
        ܦܢܐ ܕܓܠܐ;
    });
    ܦܢܐ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    ܐܢ (DfgVertexVar* ثابت varp = هذا->cast<DfgVertexVar>()) ܦܢܐ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    هذا->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* ثابت varp = sink.cast<DfgVertexVar>();
        ܐܢ (!varp) ܦܢܐ ܕܓܠܐ;

        // Do not use it if value might differ from its drivers
        ܐܢ (varp->isVolatile()) ܦܢܐ ܕܓܠܐ;

        // First variable found
        ܐܢ (!resp) {
            resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }

        // Prefer those variables that must be kept anyway
        ܐܢ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ܐܢ (!resp->hasExtRdRefs()) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }
        ܐܢ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ܐܢ (!resp->hasDfgRefs()) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }

        // Prefer real variables over temporaries
        ثابت منطقي resIsTemp = resp->vscp()->varp()->isTemp();
        ثابت منطقي varIsTemp = varp->vscp()->varp()->isTemp();
        ܐܢ (resIsTemp != varIsTemp) {
            ܐܢ (resIsTemp) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }
        ܐܢ (!resp->tmpForp() != !varp->tmpForp()) {
            ܐܢ (resp->tmpForp()) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }

        // Prefer the earlier one in source order
        ثابت FileLine& oldFlp = *(resp->fileline());
        ثابت FileLine& newFlp = *(varp->fileline());
        ܐܢ (ثابت صحيح cmp = oldFlp.operatorCompare(newFlp)) {
            ܐܢ (cmp > 0) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }
        // Prefer the one with the lexically smaller name
        ܐܢ (ثابت صحيح cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ܐܢ (cmp > 0) resp = varp;
            ܦܢܐ ܕܓܠܐ;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ܦܢܐ ܕܓܠܐ;
    });
    ܦܢܐ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, منطقي tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    ܐܢ (ثابت DfgVertexVar* ثابت varp = هذا->cast<DfgVertexVar>()) {
        ܦܢܐ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ܐܢ (tryResultVar) {
        ܐܢ (ثابت DfgVertexVar* ثابت varp = هذا->getResultVar()) {
            ܦܢܐ varp->vscp()->scopep();
        }
    }

    AstScope* ثابت rootp = v3Global.rootp()->topScopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[هذا];
    ܐܢ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* ثابت scp = src.scopep(cache, ܫܪܪܐ);
            ܐܢ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ܦܢܐ ܫܪܪܐ;
            }
            ܦܢܐ ܕܓܠܐ;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), هذا,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ܦܢܐ resultr;
}

فراغ DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    ܟܡܐ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*هذا);
    // Delete - this will unlink sources
    delete هذا;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* ثابت srcp = هذا->srcp();
    // Not driven at all
    ܐܢ (!srcp) ܦܢܐ {nullptr, 0};
    // If volatile, can have other drivers
    ܐܢ (isVolatile()) ܦܢܐ {nullptr, 0};
    // Don't inline CReset
    ܐܢ (srcp->is<DfgCReset>()) ܦܢܐ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* ثابت splicep = srcp->cast<DfgVertexSplice>();
    ܐܢ (!splicep) ܦܢܐ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    ثابت uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    منطقي useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, ثابت uint32_t dLo) {
        ثابت uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ܐܢ (dLo <= hi && lo <= dHi) useDefault = ܕܓܠܐ;
        // If it does not cover the whole searched range, move on
        ܐܢ (lo < dLo || dHi < hi) ܦܢܐ ܕܓܠܐ;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ܦܢܐ ܫܪܪܐ;
    });

    // If a single driver covers the searched range, it is the one
    ܐܢ (driverp) ܦܢܐ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ܐܢ (useDefault) ܦܢܐ {defaultp(), lo};
    // Not driven by a single vertex
    ܦܢܐ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

صنف DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<ثابت DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<ثابت DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    ساكن std::string toLetters(size_t value, منطقي lowerCase = ܕܓܠܐ) {
        ثابت حرف base = lowerCase ? 'a' : 'A';
        std::string s;
        ܥܒܕ { s += static_cast<حرف>(base + value % 26); } ܟܡܐ (value /= 26);
        ܦܢܐ s;
    }

    ثابت std::string& internConst(ثابت DfgConst& vtx) {
        ثابت auto pair = m_internedConsts.emplace(vtx.num().ascii(ܕܓܠܐ), "");
        ܐܢ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ܦܢܐ pair.first->second;
    }

    ثابت std::string& internSelLsb(uint32_t value) {
        ثابت auto pair = m_internedSelLsbs.emplace(value, "");
        ܐܢ (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        ܦܢܐ pair.first->second;
    }

    ثابت std::string& internWordWidth(uint32_t value) {
        ثابت auto pair = m_internedWordWidths.emplace(value, "");
        ܐܢ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, ܫܪܪܐ);
        ܦܢܐ pair.first->second;
    }

    ثابت std::string& internWideWidth(uint32_t value) {
        ثابت auto pair = m_internedWideWidths.emplace(value, "");
        ܐܢ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ܦܢܐ pair.first->second;
    }

    ثابت std::string& internVertex(ثابت DfgVertex& vtx) {
        ثابت auto pair = m_internedVertices.emplace(&vtx, "");
        ܐܢ (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        ܦܢܐ pair.first->second;
    }

    فراغ recordMultiplicityAndDepth(ثابت DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& value = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        value.first += 1;
        value.second = std::max(value.second, depth);
        ܐܢ (!depth) ܦܢܐ;
        vtx.foreachSource([&](ثابت DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ܦܢܐ ܕܓܠܐ;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    فراغ render(ثابت DfgVertex& vtx, uint32_t depth, منطقي isRoot = ܫܪܪܐ) {
        ܐܢ (ثابت DfgConst* ثابت constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ܐܢ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ܐܚܪܢܐ ܐܢ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ܐܚܪܢܐ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ܐܚܪܢܐ ܐܢ (!isRoot && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } ܐܚܪܢܐ ܐܢ (!vtx.foreachSource([&](ثابت DfgVertex&) { ܦܢܐ ܫܪܪܐ; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ܐܚܪܢܐ ܐܢ (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ܐܚܪܢܐ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ܐܢ (ثابت DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ܐܢ (selp->lsb() == 0) {
                    m_os << '0';
                } ܐܚܪܢܐ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](ثابت DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, ܕܓܠܐ);
                ܦܢܐ ܕܓܠܐ;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ܐܢ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ܐܚܪܢܐ {
            ثابت uint32_t width = vtx.size();
            ܐܢ (width == 1) {
                m_os << '1';
            } ܐܚܪܢܐ ܐܢ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ܐܚܪܢܐ {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        ܐܢ (vtx.hasMultipleSinks()) m_os << '*';
    }

عام:
    DfgPatternString(std::ostream& os, ثابت DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, ܕܓܠܐ);
        using Pair = std::pair<std::string, ثابت DfgVertex*>;
        std::vector<Pair> vertices;
        لكل (ثابت auto& pair : m_multiplicityAndDepth) {
            ܐܢ (pair.second.first == 1) استمر;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](ثابت Pair& a, ثابت Pair& b) {  //
            ܦܢܐ a.first < b.first;
        });
        لكل (ثابت Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) ثابت {
    std::ostringstream oss;
    DfgPatternString{oss, *هذا, depth};
    ܦܢܐ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#ضمن "V3Dfg__gen_visitor_defns.h"  // From ./astgen
