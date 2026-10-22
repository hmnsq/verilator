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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3DfgContext.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, અચલ string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*આ); });
}

શૂન્ય DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    જો (otherps.empty()) પાછા;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    અચલ VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    માટે (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    માટે (અચલ std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        માટે (DfgVertexVar* અચલ vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            જો (DfgVertexVar* અચલ altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* અચલ srcp = vtxp->srcp();
                DfgVertex* અચલ defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                જો (srcp) altp->srcp(srcp);
                જો (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                ચાલુ;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = આ;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        માટે (DfgVertexAst* અચલ vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = આ;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        માટે (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = આ;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        માટે (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = આ;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(File_લીટી* flp, અચલ std::string& prefix,
                                   અચલ DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* અચલ vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    જો (dtype.isArray()) પાછા નવું DfgVarArray{*આ, vscp};
    પાછા નવું DfgVarPacked{*આ, vscp};
}

સ્થિર અચલ std::string toDotId(અચલ DfgVertex& vtx) { પાછા '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
સ્થિર શૂન્ય dumpDotVertex(std::ostream& os, અચલ DfgVertex& vtx) {
    જો (અચલ DfgVertexVar* અચલ varVtxp = vtx.cast<DfgVertexVar>()) {
        અચલ AstVarScope* અચલ vscp = varVtxp->vscp();
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
        જો (અચલ AstVarScope* અચલ tmpForp = varVtxp->tmpForp()) {
            જો (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        સ્થિર અચલ અક્ષર* અચલ rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[ખોટું][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        જો (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } નહીંતર જો (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } નહીંતર {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        અચલ અક્ષર* અચલ colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        પાછા;
    }

    જો (અચલ DfgPrev* અચલ prevVtxp = vtx.cast<DfgPrev>()) {
        અચલ AstVarScope* અચલ vscp = prevVtxp->vscp();
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
        જો (prevVtxp->isPacked()) {
            os << ", shape=box";
        } નહીંતર જો (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } નહીંતર {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        અચલ અક્ષર* અચલ colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        પાછા;
    }

    જો (અચલ DfgConst* અચલ constVtxp = vtx.cast<DfgConst>()) {
        અચલ V3_સંખ્યા& અંક = constVtxp->અંક();

        os << toDotId(vtx);
        os << " [label=\"";
        જો (અંક.width() <= 32 && !અંક.isSigned()) {
            os << constVtxp->width() << "'d" << અંક.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << અંક.toUInt() << std::dec << '\n';
        } નહીંતર {
            os << અંક.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        પાછા;
    }

    જો (અચલ DfgSel* અચલ selVtxp = vtx.cast<DfgSel>()) {
        અચલ uint32_t lsb = selVtxp->lsb();
        અચલ uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        જો (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } નહીંતર {
            os << ", shape=circle";
        }
        os << "]\n";
        પાછા;
    }

    જો (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        જો (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } નહીંતર {
            os << ", shape=octagon";
        }
        os << "]\n";
        પાછા;
    }

    જો (અચલ DfgLogic* અચલ logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_વૃક્ષ(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        અચલ અક્ષર* અચલ colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        પાછા;
    }

    જો (અચલ DfgVertexAst* અચલ astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debug_Verilog_For_વૃક્ષ(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        અચલ DfgAstRd* અચલ astRdVtxp = astVtxp->cast<DfgAstRd>();
        અચલ અક્ષર* અચલ colorp = astRdVtxp ? "#80ff80"  // Green
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
        પાછા;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    જો (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } નહીંતર {
        os << ", shape=circle";
    }
    os << "]\n";
}

શૂન્ય DfgGraph::dumpDot(std::ostream& os, અચલ std::string& label,
                       std::function<તાર્કિક(અચલ DfgVertex&)> p) અચલ {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    જો (!p) p = [](અચલ DfgVertex&) { પાછા સાચું; };

    std::unordered_set<અચલ DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](અચલ DfgVertex& vtx) {
        અચલ DfgLogic* અચલ logicp = vtx.cast<DfgLogic>();
        જો (!logicp) પાછા;
        જો (logicp->synth().empty()) પાછા;
        જો (!p(vtx)) પાછા;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.ઉમેરો(logicp);
        માટે (DfgVertex* અચલ vtxp : logicp->synth()) {
            જો (!p(*vtxp)) ચાલુ;
            dumpDotVertex(os, *vtxp);
            emitted.ઉમેરો(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](અચલ DfgVertex& vtx) {
        જો (emitted.ગણતરી(&vtx)) પાછા;
        જો (!p(vtx)) પાછા;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](અચલ DfgVertex& vtx) {
        જો (!p(vtx)) પાછા;
        માટે (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* અચલ srcp = vtx.inputp(i);
            જો (!srcp) ચાલુ;
            જો (!p(*srcp)) ચાલુ;
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

std::string DfgGraph::dumpDotString(અચલ std::string& label,
                                    std::function<તાર્કિક(અચલ DfgVertex&)> p) અચલ {
    std::stringstream ss;
    dumpDot(ss, label, p);
    પાછા ss.str();
}

શૂન્ય DfgGraph::dumpDotFile(અચલ std::string& filename, અચલ std::string& label,
                           std::function<તાર્કિક(અચલ DfgVertex&)> p) અચલ {
    અચલ std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    જો (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

શૂન્ય DfgGraph::dumpDotFilePrefixed(અચલ std::string& label,
                                   std::function<તાર્કિક(અચલ DfgVertex&)> p) અચલ {
    std::string filename = name();
    જો (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <તાર્કિક T_SinksNotSources>
સ્થિર std::unique_ptr<std::unordered_set<અચલ DfgVertex*>>
dfgGraphCollectCone(અચલ std::vector<અચલ DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<અચલ DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<અચલ DfgVertex*>> resp{
        નવું std::unordered_set<અચલ DfgVertex*>{}};
    // Depth first traversal
    જ્યારે (!queue.empty()) {
        // Pop next work item
        અચલ DfgVertex* અચલ vtxp = queue.back();
        queue.કાઢો_back();
        // Mark vertex as visited, move on if already visited
        જો (!resp->ઉમેરો(vtxp).second) ચાલુ;
        // Enqueue all siblings of this vertex.
        જો VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](અચલ DfgVertex& sink) {
                queue.ધકેલો_back(&sink);
                પાછા ખોટું;
            });
        } નહીંતર {
            vtxp->foreachSource([&](અચલ DfgVertex& src) {
                queue.ધકેલો_back(&src);
                પાછા ખોટું;
            });
        }
    }
    // Done
    પાછા resp;
}

std::unique_ptr<std::unordered_set<અચલ DfgVertex*>>
DfgGraph::sourceCone(અચલ std::vector<અચલ DfgVertex*>& vtxps) અચલ {
    પાછા dfgGraphCollectCone<ખોટું>(vtxps);
}

std::unique_ptr<std::unordered_set<અચલ DfgVertex*>>
DfgGraph::sinkCone(અચલ std::vector<અચલ DfgVertex*>& vtxps) અચલ {
    પાછા dfgGraphCollectCone<સાચું>(vtxps);
}

std::unique_ptr<std::unordered_set<અચલ DfgVertex*>>
DfgGraph::neighborhood(અચલ std::vector<અચલ DfgVertex*>& vtxps, size_t n) અચલ {
    // Neighborhood
    std::vector<અચલ DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<અચલ DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    માટે (size_t hops = 1; hops <= n; ++hops) {
        માટે (size_t i = begin; i < end; ++i) {
            અચલ DfgVertex* અચલ vtxp = vec[i];
            vtxp->foreachSink([&](અચલ DfgVertex& dst) {
                જો (res.ઉમેરો(&dst).second) vec.ધકેલો_back(&dst);
                પાછા ખોટું;
            });
            vtxp->foreachSource([&](અચલ DfgVertex& src) {
                જો (res.ઉમેરો(&src).second) vec.ધકેલો_back(&src);
                પાછા ખોટું;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    પાછા std::make_unique<std::unordered_set<અચલ DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, File_લીટી* flp, અચલ DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*આ);
}

તાર્કિક DfgVertex::unsafe() અચલ {
    જો (is<DfgMux>()) પાછા સાચું;
    જો (is<DfgArraySel>()) પાછા !as<DfgArraySel>()->bitp()->is<DfgConst>();
    પાછા ખોટું;
}

શૂન્ય DfgVertex::typeCheck(અચલ DfgGraph& dfg) અચલ {

#define CHECK(cond, સંદેશ) \
    UASSERT_OBJ(cond, આ, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << સંદેશ);
    પસંદગી (type()) {
    સ્થિતિ VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        પાછા;
    }
    સ્થિતિ VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        પાછા;
    }
    સ્થિતિ VDfgType::AstRd: {
        અચલ DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        પાછા;
    }
    સ્થિતિ VDfgType::VarArray:
    સ્થિતિ VDfgType::VarPacked: {
        અચલ DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        પાછા;
    }
    સ્થિતિ VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        પાછા;
    }
    સ્થિતિ VDfgType::SpliceArray:
    સ્થિતિ VDfgType::SplicePacked: {
        અચલ DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](અચલ DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            પાછા ખોટું;
        });
        પાછા;
    }
    સ્થિતિ VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        પાછા;
    }
    સ્થિતિ VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        પાછા;
    }
    સ્થિતિ VDfgType::UnitArray: {
        અચલ DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        પાછા;
    }
    સ્થિતિ VDfgType::Sel: {
        અચલ DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        પાછા;
    }
    સ્થિતિ VDfgType::MatchMasked: {
        અચલ DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        પાછા;
    }
    સ્થિતિ VDfgType::Mux: {
        અચલ DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        પાછા;
    }
    સ્થિતિ VDfgType::ArraySel: {
        અચલ DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        પાછા;
    }

    સ્થિતિ VDfgType::Add:
    સ્થિતિ VDfgType::And:
    સ્થિતિ VDfgType::Div:
    સ્થિતિ VDfgType::DivS:
    સ્થિતિ VDfgType::ModDiv:
    સ્થિતિ VDfgType::ModDivS:
    સ્થિતિ VDfgType::Mul:
    સ્થિતિ VDfgType::MulS:
    સ્થિતિ VDfgType::Or:
    સ્થિતિ VDfgType::Sub:
    સ્થિતિ VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        પાછા;
    }

    સ્થિતિ VDfgType::Negate:
    સ્થિતિ VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        પાછા;
    }

    સ્થિતિ VDfgType::ShiftL:
    સ્થિતિ VDfgType::ShiftR:
    સ્થિતિ VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        પાછા;
    }

    સ્થિતિ VDfgType::Concat: {
        અચલ DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        પાછા;
    }

    સ્થિતિ VDfgType::Rep: {
        અચલ DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        પાછા;
    }

    સ્થિતિ VDfgType::StreamL:
    સ્થિતિ VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        પાછા;
    }

    સ્થિતિ VDfgType::Eq:
    સ્થિતિ VDfgType::EqCase:
    સ્થિતિ VDfgType::EqWild:
    સ્થિતિ VDfgType::Neq:
    સ્થિતિ VDfgType::NeqCase:
    સ્થિતિ VDfgType::NeqWild:
    સ્થિતિ VDfgType::Gt:
    સ્થિતિ VDfgType::GtS:
    સ્થિતિ VDfgType::Gte:
    સ્થિતિ VDfgType::GteS:
    સ્થિતિ VDfgType::Lt:
    સ્થિતિ VDfgType::LtS:
    સ્થિતિ VDfgType::Lte:
    સ્થિતિ VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        પાછા;
    }

    સ્થિતિ VDfgType::Extend:
    સ્થિતિ VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        પાછા;
    }

    સ્થિતિ VDfgType::LogAnd:
    સ્થિતિ VDfgType::LogEq:
    સ્થિતિ VDfgType::LogIf:
    સ્થિતિ VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        પાછા;
    }

    સ્થિતિ VDfgType::LogNot:
    સ્થિતિ VDfgType::RedAnd:
    સ્થિતિ VDfgType::RedOr:
    સ્થિતિ VDfgType::RedXor:
    સ્થિતિ VDfgType::OneHot:
    સ્થિતિ VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        પાછા;
    }

    સ્થિતિ VDfgType::Cond: {
        અચલ DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        પાછા;
    }

    સ્થિતિ VDfgType::ગણતરી_Ones_2: {
        અચલ Dfg_ગણતરી_Ones& v = *as<Dfg_ગણતરી_Ones>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        પાછા;
    }

    સ્થિતિ VDfgType::Pow:
    સ્થિતિ VDfgType::PowSS:
    સ્થિતિ VDfgType::PowSU:
    સ્થિતિ VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        પાછા;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() અચલ {
    uint32_t result = 0;
    foreachSink([&](અચલ DfgVertex&) {
        ++result;
        પાછા ખોટું;
    });
    પાછા result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    જો (DfgVertexVar* અચલ varp = આ->cast<DfgVertexVar>()) પાછા varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    આ->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* અચલ varp = sink.cast<DfgVertexVar>();
        જો (!varp) પાછા ખોટું;

        // Do not use it if value might differ from its drivers
        જો (varp->isVolatile()) પાછા ખોટું;

        // First variable found
        જો (!resp) {
            resp = varp;
            પાછા ખોટું;
        }

        // Prefer those variables that must be kept anyway
        જો (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            જો (!resp->hasExtRdRefs()) resp = varp;
            પાછા ખોટું;
        }
        જો (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            જો (!resp->hasDfgRefs()) resp = varp;
            પાછા ખોટું;
        }

        // Prefer real variables over temporaries
        અચલ તાર્કિક resIsTemp = resp->vscp()->varp()->isTemp();
        અચલ તાર્કિક varIsTemp = varp->vscp()->varp()->isTemp();
        જો (resIsTemp != varIsTemp) {
            જો (resIsTemp) resp = varp;
            પાછા ખોટું;
        }
        જો (!resp->tmpForp() != !varp->tmpForp()) {
            જો (resp->tmpForp()) resp = varp;
            પાછા ખોટું;
        }

        // Prefer the earlier one in source order
        અચલ File_લીટી& oldFlp = *(resp->fileline());
        અચલ File_લીટી& newFlp = *(varp->fileline());
        જો (અચલ પૂર્ણાંક cmp = oldFlp.operatorCompare(newFlp)) {
            જો (cmp > 0) resp = varp;
            પાછા ખોટું;
        }
        // Prefer the one with the lexically smaller name
        જો (અચલ પૂર્ણાંક cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            જો (cmp > 0) resp = varp;
            પાછા ખોટું;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        પાછા ખોટું;
    });
    પાછા resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, તાર્કિક tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    જો (અચલ DfgVertexVar* અચલ varp = આ->cast<DfgVertexVar>()) {
        પાછા varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    જો (tryResultVar) {
        જો (અચલ DfgVertexVar* અચલ varp = આ->getResultVar()) {
            પાછા varp->vscp()->scopep();
        }
    }

    AstScope* અચલ rootp = v3Global.rootp()->ટોચ_Scopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[આ];
    જો (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* અચલ scp = src.scopep(cache, સાચું);
            જો (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                પાછા સાચું;
            }
            પાછા ખોટું;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), આ,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    પાછા resultr;
}

શૂન્ય DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    જ્યારે (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*આ);
    // Delete - this will unlink sources
    delete આ;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* અચલ srcp = આ->srcp();
    // Not driven at all
    જો (!srcp) પાછા {nullptr, 0};
    // If volatile, can have other drivers
    જો (isVolatile()) પાછા {nullptr, 0};
    // Don't inline CReset
    જો (srcp->is<DfgCReset>()) પાછા {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* અચલ splicep = srcp->cast<DfgVertexSplice>();
    જો (!splicep) પાછા {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    અચલ uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    તાર્કિક useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, અચલ uint32_t dLo) {
        અચલ uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        જો (dLo <= hi && lo <= dHi) useDefault = ખોટું;
        // If it does not cover the whole searched range, move on
        જો (lo < dLo || dHi < hi) પાછા ખોટું;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        પાછા સાચું;
    });

    // If a single driver covers the searched range, it is the one
    જો (driverp) પાછા {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    જો (useDefault) પાછા {defaultp(), lo};
    // Not driven by a single vertex
    પાછા {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

વર્ગ DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<અચલ DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<અચલ DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    સ્થિર std::string toLetters(size_t કિંમત, તાર્કિક lowerCase = ખોટું) {
        અચલ અક્ષર base = lowerCase ? 'a' : 'A';
        std::string s;
        કરો { s += static_cast<અક્ષર>(base + કિંમત % 26); } જ્યારે (કિંમત /= 26);
        પાછા s;
    }

    અચલ std::string& internConst(અચલ DfgConst& vtx) {
        અચલ auto pair = m_internedConsts.emplace(vtx.અંક().ascii(ખોટું), "");
        જો (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        પાછા pair.first->second;
    }

    અચલ std::string& internSelLsb(uint32_t કિંમત) {
        અચલ auto pair = m_internedSelLsbs.emplace(કિંમત, "");
        જો (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        પાછા pair.first->second;
    }

    અચલ std::string& internWordWidth(uint32_t કિંમત) {
        અચલ auto pair = m_internedWordWidths.emplace(કિંમત, "");
        જો (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, સાચું);
        પાછા pair.first->second;
    }

    અચલ std::string& internWideWidth(uint32_t કિંમત) {
        અચલ auto pair = m_internedWideWidths.emplace(કિંમત, "");
        જો (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        પાછા pair.first->second;
    }

    અચલ std::string& internVertex(અચલ DfgVertex& vtx) {
        અચલ auto pair = m_internedVertices.emplace(&vtx, "");
        જો (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        પાછા pair.first->second;
    }

    શૂન્ય recordMultiplicityAndDepth(અચલ DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& કિંમત = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        કિંમત.first += 1;
        કિંમત.second = std::max(કિંમત.second, depth);
        જો (!depth) પાછા;
        vtx.foreachSource([&](અચલ DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            પાછા ખોટું;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    શૂન્ય render(અચલ DfgVertex& vtx, uint32_t depth, તાર્કિક is_મૂળ_2 = સાચું) {
        જો (અચલ DfgConst* અચલ constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            જો (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } નહીંતર જો (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } નહીંતર {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } નહીંતર જો (!is_મૂળ_2 && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } નહીંતર જો (!vtx.foreachSource([&](અચલ DfgVertex&) { પાછા સાચું; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } નહીંતર જો (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } નહીંતર {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            જો (અચલ DfgSel* અચલ selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                જો (selp->lsb() == 0) {
                    m_os << '0';
                } નહીંતર {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](અચલ DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, ખોટું);
                પાછા ખોટું;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        જો (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } નહીંતર {
            અચલ uint32_t width = vtx.size();
            જો (width == 1) {
                m_os << '1';
            } નહીંતર જો (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } નહીંતર {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        જો (vtx.hasMultipleSinks()) m_os << '*';
    }

જાહેર:
    DfgPatternString(std::ostream& os, અચલ DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, ખોટું);
        using Pair = std::pair<std::string, અચલ DfgVertex*>;
        std::vector<Pair> vertices;
        માટે (અચલ auto& pair : m_multiplicityAndDepth) {
            જો (pair.second.first == 1) ચાલુ;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](અચલ Pair& a, અચલ Pair& b) {  //
            પાછા a.first < b.first;
        });
        માટે (અચલ Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) અચલ {
    std::ostringstream oss;
    DfgPatternString{oss, *આ, depth};
    પાછા oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#અન્તર્ભાવ "V3Dfg__gen_visitor_defns.h"  // From ./astgen
