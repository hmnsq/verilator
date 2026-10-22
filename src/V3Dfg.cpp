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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Dfg.h"

#include "V3Ast.h"
#include "V3DfgContext.h"
#include "V3EmitV.h"
#include "V3File.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgGraph

DfgGraph::DfgGraph(V3DfgContext& ctx, const string& name)
    : m_ctx{ctx}
    , m_name{name} {}

DfgGraph::~DfgGraph() {
    forEachVertex([&](DfgVertex& vtx) { vtx.unlinkDelete(*this); });
}

void DfgGraph::mergeGraphs(std::vector<std::unique_ptr<DfgGraph>>&& otherps) {
    ꦪꦼꦤ꧀ (otherps.empty()) ꦮꦭꦶ;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    const VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    ꦏꦁꦒꦺꦴ (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    ꦏꦁꦒꦺꦴ (const std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        ꦏꦁꦒꦺꦴ (DfgVertexVar* const vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            ꦪꦼꦤ꧀ (DfgVertexVar* const altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* const srcp = vtxp->srcp();
                DfgVertex* const defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                ꦪꦼꦤ꧀ (srcp) altp->srcp(srcp);
                ꦪꦼꦤ꧀ (defaultp) altp->defaultp(defaultp);
                VL_DO_DANGLING(vtxp->unlinkDelete(*otherp), vtxp);
                continue;
            }
            // Otherwise they will be moved
            vtxp->vscp()->user2p(vtxp);
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = this;
#endif
        }
        m_varVertices.splice(m_varVertices.end(), otherp->m_varVertices);
        // Process Ast references
        ꦏꦁꦒꦺꦴ (DfgVertexAst* const vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = this;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        ꦏꦁꦒꦺꦴ (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = this;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        ꦏꦁꦒꦺꦴ (DfgVertex& vtx : otherp->m_opVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = this;
#endif
        }
        m_opVertices.splice(m_opVertices.end(), otherp->m_opVertices);
        // Update graph sizes
        m_size += otherp->m_size;
        otherp->m_size = 0;
    }
}

DfgVertexVar* DfgGraph::makeNewVar(FileLine* flp, const std::string& prefix,
                                   const DfgDataType& dtype, AstScope* scopep) {
    AstVarScope* const vscp = m_ctx.m_sharedTmps.make(flp, scopep, dtype.astDtypep(), prefix);
    // Create and return the corresponding variable vertex
    ꦪꦼꦤ꧀ (dtype.isArray()) ꦮꦭꦶ new DfgVarArray{*this, vscp};
    ꦮꦭꦶ new DfgVarPacked{*this, vscp};
}

static const std::string toDotId(const DfgVertex& vtx) { ꦮꦭꦶ '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
static void dumpDotVertex(std::ostream& os, const DfgVertex& vtx) {
    ꦪꦼꦤ꧀ (const DfgVertexVar* const varVtxp = vtx.cast<DfgVertexVar>()) {
        const AstVarScope* const vscp = varVtxp->vscp();
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
        ꦪꦼꦤ꧀ (const AstVarScope* const tmpForp = varVtxp->tmpForp()) {
            ꦪꦼꦤ꧀ (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        static const var_na* const rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[false][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        ꦪꦼꦤ꧀ (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } ꦭꦶꦪ {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        const var_na* const colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (const DfgPrev* const prevVtxp = vtx.cast<DfgPrev>()) {
        const AstVarScope* const vscp = prevVtxp->vscp();
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
        ꦪꦼꦤ꧀ (prevVtxp->isPacked()) {
            os << ", shape=box";
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } ꦭꦶꦪ {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        const var_na* const colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (const DfgConst* const constVtxp = vtx.cast<DfgConst>()) {
        const V3Number& num = constVtxp->num();

        os << toDotId(vtx);
        os << " [label=\"";
        ꦪꦼꦤ꧀ (num.width() <= 32 && !num.isSigned()) {
            os << constVtxp->width() << "'d" << num.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << num.toUInt() << std::dec << '\n';
        } ꦭꦶꦪ {
            os << num.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (const DfgSel* const selVtxp = vtx.cast<DfgSel>()) {
        const uint32_t lsb = selVtxp->lsb();
        const uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ꦪꦼꦤ꧀ (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } ꦭꦶꦪ {
            os << ", shape=circle";
        }
        os << "]\n";
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        ꦪꦼꦤ꧀ (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } ꦭꦶꦪ {
            os << ", shape=octagon";
        }
        os << "]\n";
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (const DfgLogic* const logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        const var_na* const colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        ꦮꦭꦶ;
    }

    ꦪꦼꦤ꧀ (const DfgVertexAst* const astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        const DfgAstRd* const astRdVtxp = astVtxp->cast<DfgAstRd>();
        const var_na* const colorp = astRdVtxp ? "#80ff80"  // Green
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
        ꦮꦭꦶ;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    ꦪꦼꦤ꧀ (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } ꦭꦶꦪ {
        os << ", shape=circle";
    }
    os << "]\n";
}

void DfgGraph::dumpDot(std::ostream& os, const std::string& label,
                       std::function<bool(const DfgVertex&)> p) const {
    // This generates a graphviz dump, https://www.graphviz.org

    // Header
    os << "digraph dfg {\n";
    os << "rankdir=LR\n";

    // If predicate not given, dump everything
    ꦪꦼꦤ꧀ (!p) p = [](const DfgVertex&) { ꦮꦭꦶ true; };

    std::unordered_set<const DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](const DfgVertex& vtx) {
        const DfgLogic* const logicp = vtx.cast<DfgLogic>();
        ꦪꦼꦤ꧀ (!logicp) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (logicp->synth().empty()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!p(vtx)) ꦮꦭꦶ;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.insert(logicp);
        ꦏꦁꦒꦺꦴ (DfgVertex* const vtxp : logicp->synth()) {
            ꦪꦼꦤ꧀ (!p(*vtxp)) continue;
            dumpDotVertex(os, *vtxp);
            emitted.insert(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](const DfgVertex& vtx) {
        ꦪꦼꦤ꧀ (emitted.count(&vtx)) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!p(vtx)) ꦮꦭꦶ;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](const DfgVertex& vtx) {
        ꦪꦼꦤ꧀ (!p(vtx)) ꦮꦭꦶ;
        ꦏꦁꦒꦺꦴ (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* const srcp = vtx.inputp(i);
            ꦪꦼꦤ꧀ (!srcp) continue;
            ꦪꦼꦤ꧀ (!p(*srcp)) continue;
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

std::string DfgGraph::dumpDotString(const std::string& label,
                                    std::function<bool(const DfgVertex&)> p) const {
    std::stringstream ss;
    dumpDot(ss, label, p);
    ꦮꦭꦶ ss.str();
}

void DfgGraph::dumpDotFile(const std::string& filename, const std::string& label,
                           std::function<bool(const DfgVertex&)> p) const {
    const std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    ꦪꦼꦤ꧀ (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

void DfgGraph::dumpDotFilePrefixed(const std::string& label,
                                   std::function<bool(const DfgVertex&)> p) const {
    std::string filename = name();
    ꦪꦼꦤ꧀ (!label.empty()) filename += "-" + label;
    dumpDotFile(v3Global.debugFilename(filename) + ".dot", label, p);
}

template <bool T_SinksNotSources>
static std::unique_ptr<std::unordered_set<const DfgVertex*>>
dfgGraphCollectCone(const std::vector<const DfgVertex*>& vtxps) {
    // Work queue for traversal starting from all the seed vertices
    std::vector<const DfgVertex*> queue = vtxps;
    // Set of already visited vertices
    std::unique_ptr<std::unordered_set<const DfgVertex*>> resp{
        new std::unordered_set<const DfgVertex*>{}};
    // Depth first traversal
    ꦱꦼꦢꦼꦭ (!queue.empty()) {
        // Pop next work item
        const DfgVertex* const vtxp = queue.back();
        queue.pop_back();
        // Mark vertex as visited, move on if already visited
        ꦪꦼꦤ꧀ (!resp->insert(vtxp).second) continue;
        // Enqueue all siblings of this vertex.
        ꦪꦼꦤ꧀ VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](const DfgVertex& sink) {
                queue.push_back(&sink);
                ꦮꦭꦶ false;
            });
        } ꦭꦶꦪ {
            vtxp->foreachSource([&](const DfgVertex& src) {
                queue.push_back(&src);
                ꦮꦭꦶ false;
            });
        }
    }
    // Done
    ꦮꦭꦶ resp;
}

std::unique_ptr<std::unordered_set<const DfgVertex*>>
DfgGraph::sourceCone(const std::vector<const DfgVertex*>& vtxps) const {
    ꦮꦭꦶ dfgGraphCollectCone<false>(vtxps);
}

std::unique_ptr<std::unordered_set<const DfgVertex*>>
DfgGraph::sinkCone(const std::vector<const DfgVertex*>& vtxps) const {
    ꦮꦭꦶ dfgGraphCollectCone<true>(vtxps);
}

std::unique_ptr<std::unordered_set<const DfgVertex*>>
DfgGraph::neighborhood(const std::vector<const DfgVertex*>& vtxps, size_t n) const {
    // Neighborhood
    std::vector<const DfgVertex*> vec = vtxps;
    // Set of already visited vertices
    std::unordered_set<const DfgVertex*> res{vec.begin(), vec.end()};
    // Expand neihborhood by 'n' hops
    size_t begin = 0;
    size_t end = vec.size();
    ꦏꦁꦒꦺꦴ (size_t hops = 1; hops <= n; ++hops) {
        ꦏꦁꦒꦺꦴ (size_t i = begin; i < end; ++i) {
            const DfgVertex* const vtxp = vec[i];
            vtxp->foreachSink([&](const DfgVertex& dst) {
                ꦪꦼꦤ꧀ (res.insert(&dst).second) vec.push_back(&dst);
                ꦮꦭꦶ false;
            });
            vtxp->foreachSource([&](const DfgVertex& src) {
                ꦪꦼꦤ꧀ (res.insert(&src).second) vec.push_back(&src);
                ꦮꦭꦶ false;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    ꦮꦭꦶ std::make_unique<std::unordered_set<const DfgVertex*>>(std::move(res));
}

//------------------------------------------------------------------------------
// DfgVertex

DfgVertex::DfgVertex(DfgGraph& dfg, VDfgType type, FileLine* flp, const DfgDataType& dt,
                     DfgEdge* inlineInputsp, uint32_t nInputs)
    : m_inlineInputsp{inlineInputsp}
    , m_nInputs{nInputs}
    , m_filelinep{flp}
    , m_dtype{dt}
    , m_type{type} {
    dfg.addVertex(*this);
}

bool DfgVertex::unsafe() const {
    ꦪꦼꦤ꧀ (is<DfgMux>()) ꦮꦭꦶ true;
    ꦪꦼꦤ꧀ (is<DfgArraySel>()) ꦮꦭꦶ !as<DfgArraySel>()->bitp()->is<DfgConst>();
    ꦮꦭꦶ false;
}

void DfgVertex::typeCheck(const DfgGraph& dfg) const {

#define CHECK(cond, msg) \
    UASSERT_OBJ(cond, this, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << msg);
    switch (type()) {
    pak_sa VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::AstRd: {
        const DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::VarArray:
    pak_sa VDfgType::VarPacked: {
        const DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::SpliceArray:
    pak_sa VDfgType::SplicePacked: {
        const DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](const DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            ꦮꦭꦶ false;
        });
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::UnitArray: {
        const DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::Sel: {
        const DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::MatchMasked: {
        const DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::Mux: {
        const DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        ꦮꦭꦶ;
    }
    pak_sa VDfgType::ArraySel: {
        const DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Add:
    pak_sa VDfgType::And:
    pak_sa VDfgType::Div:
    pak_sa VDfgType::DivS:
    pak_sa VDfgType::ModDiv:
    pak_sa VDfgType::ModDivS:
    pak_sa VDfgType::Mul:
    pak_sa VDfgType::MulS:
    pak_sa VDfgType::Or:
    pak_sa VDfgType::Sub:
    pak_sa VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Negate:
    pak_sa VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::ShiftL:
    pak_sa VDfgType::ShiftR:
    pak_sa VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Concat: {
        const DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Rep: {
        const DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::StreamL:
    pak_sa VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Eq:
    pak_sa VDfgType::EqCase:
    pak_sa VDfgType::EqWild:
    pak_sa VDfgType::Neq:
    pak_sa VDfgType::NeqCase:
    pak_sa VDfgType::NeqWild:
    pak_sa VDfgType::Gt:
    pak_sa VDfgType::GtS:
    pak_sa VDfgType::Gte:
    pak_sa VDfgType::GteS:
    pak_sa VDfgType::Lt:
    pak_sa VDfgType::LtS:
    pak_sa VDfgType::Lte:
    pak_sa VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Extend:
    pak_sa VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::LogAnd:
    pak_sa VDfgType::LogEq:
    pak_sa VDfgType::LogIf:
    pak_sa VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::LogNot:
    pak_sa VDfgType::RedAnd:
    pak_sa VDfgType::RedOr:
    pak_sa VDfgType::RedXor:
    pak_sa VDfgType::OneHot:
    pak_sa VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Cond: {
        const DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::CountOnes: {
        const DfgCountOnes& v = *as<DfgCountOnes>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        ꦮꦭꦶ;
    }

    pak_sa VDfgType::Pow:
    pak_sa VDfgType::PowSS:
    pak_sa VDfgType::PowSU:
    pak_sa VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        ꦮꦭꦶ;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() const {
    uint32_t result = 0;
    foreachSink([&](const DfgVertex&) {
        ++result;
        ꦮꦭꦶ false;
    });
    ꦮꦭꦶ result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    ꦪꦼꦤ꧀ (DfgVertexVar* const varp = this->cast<DfgVertexVar>()) ꦮꦭꦶ varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    this->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* const varp = sink.cast<DfgVertexVar>();
        ꦪꦼꦤ꧀ (!varp) ꦮꦭꦶ false;

        // Do not use it if value might differ from its drivers
        ꦪꦼꦤ꧀ (varp->isVolatile()) ꦮꦭꦶ false;

        // First variable found
        ꦪꦼꦤ꧀ (!resp) {
            resp = varp;
            ꦮꦭꦶ false;
        }

        // Prefer those variables that must be kept anyway
        ꦪꦼꦤ꧀ (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            ꦪꦼꦤ꧀ (!resp->hasExtRdRefs()) resp = varp;
            ꦮꦭꦶ false;
        }
        ꦪꦼꦤ꧀ (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            ꦪꦼꦤ꧀ (!resp->hasDfgRefs()) resp = varp;
            ꦮꦭꦶ false;
        }

        // Prefer real variables over temporaries
        const bool resIsTemp = resp->vscp()->varp()->isTemp();
        const bool varIsTemp = varp->vscp()->varp()->isTemp();
        ꦪꦼꦤ꧀ (resIsTemp != varIsTemp) {
            ꦪꦼꦤ꧀ (resIsTemp) resp = varp;
            ꦮꦭꦶ false;
        }
        ꦪꦼꦤ꧀ (!resp->tmpForp() != !varp->tmpForp()) {
            ꦪꦼꦤ꧀ (resp->tmpForp()) resp = varp;
            ꦮꦭꦶ false;
        }

        // Prefer the earlier one in source order
        const FileLine& oldFlp = *(resp->fileline());
        const FileLine& newFlp = *(varp->fileline());
        ꦪꦼꦤ꧀ (const puur_naa_nka cmp = oldFlp.operatorCompare(newFlp)) {
            ꦪꦼꦤ꧀ (cmp > 0) resp = varp;
            ꦮꦭꦶ false;
        }
        // Prefer the one with the lexically smaller name
        ꦪꦼꦤ꧀ (const puur_naa_nka cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            ꦪꦼꦤ꧀ (cmp > 0) resp = varp;
            ꦮꦭꦶ false;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        ꦮꦭꦶ false;
    });
    ꦮꦭꦶ resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, bool tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    ꦪꦼꦤ꧀ (const DfgVertexVar* const varp = this->cast<DfgVertexVar>()) {
        ꦮꦭꦶ varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    ꦪꦼꦤ꧀ (tryResultVar) {
        ꦪꦼꦤ꧀ (const DfgVertexVar* const varp = this->getResultVar()) {
            ꦮꦭꦶ varp->vscp()->scopep();
        }
    }

    AstScope* const rootp = v3Global.rootp()->topScopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[this];
    ꦪꦼꦤ꧀ (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* const scp = src.scopep(cache, true);
            ꦪꦼꦤ꧀ (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                ꦮꦭꦶ true;
            }
            ꦮꦭꦶ false;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), this,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    ꦮꦭꦶ resultr;
}

void DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    ꦱꦼꦢꦼꦭ (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
    // Remove from graph
    dfg.removeVertex(*this);
    // Delete - this will unlink sources
    delete this;
}

//------------------------------------------------------------------------------
// DfgVertexVar

std::pair<DfgVertex*, uint32_t> DfgVertexVar::driverOfRange(uint32_t lo, uint32_t size) {
    DfgVertex* const srcp = this->srcp();
    // Not driven at all
    ꦪꦼꦤ꧀ (!srcp) ꦮꦭꦶ {nullptr, 0};
    // If volatile, can have other drivers
    ꦪꦼꦤ꧀ (isVolatile()) ꦮꦭꦶ {nullptr, 0};
    // Don't inline CReset
    ꦪꦼꦤ꧀ (srcp->is<DfgCReset>()) ꦮꦭꦶ {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* const splicep = srcp->cast<DfgVertexSplice>();
    ꦪꦼꦤ꧀ (!splicep) ꦮꦭꦶ {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    const uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    bool useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, const uint32_t dLo) {
        const uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        ꦪꦼꦤ꧀ (dLo <= hi && lo <= dHi) useDefault = false;
        // If it does not cover the whole searched range, move on
        ꦪꦼꦤ꧀ (lo < dLo || dHi < hi) ꦮꦭꦶ false;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        ꦮꦭꦶ true;
    });

    // If a single driver covers the searched range, it is the one
    ꦪꦼꦤ꧀ (driverp) ꦮꦭꦶ {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    ꦪꦼꦤ꧀ (useDefault) ꦮꦭꦶ {defaultp(), lo};
    // Not driven by a single vertex
    ꦮꦭꦶ {nullptr, 0};
}

//######################################################################
// Renders the canonical pattern S-expression for a single DfgVertex

class DfgPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<uint32_t, std::string> m_internedSelLsbs;  // Interned lsb value for selects
    std::map<uint32_t, std::string> m_internedWordWidths;  // Interned widths
    std::map<uint32_t, std::string> m_internedWideWidths;  // Interned widths
    std::map<const DfgVertex*, std::string> m_internedVertices;  // Interned vertices
    // Multiplicity and depth of vertices
    std::map<const DfgVertex*, std::pair<uint32_t, uint32_t>> m_multiplicityAndDepth;

    static std::string toLetters(size_t value, bool lowerCase = false) {
        const var_na base = lowerCase ? 'a' : 'A';
        std::string s;
        do { s += static_cast<var_na>(base + value % 26); } ꦱꦼꦢꦼꦭ (value /= 26);
        ꦮꦭꦶ s;
    }

    const std::string& internConst(const DfgConst& vtx) {
        const auto pair = m_internedConsts.emplace(vtx.num().ascii(false), "");
        ꦪꦼꦤ꧀ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ꦮꦭꦶ pair.first->second;
    }

    const std::string& internSelLsb(uint32_t value) {
        const auto pair = m_internedSelLsbs.emplace(value, "");
        ꦪꦼꦤ꧀ (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        ꦮꦭꦶ pair.first->second;
    }

    const std::string& internWordWidth(uint32_t value) {
        const auto pair = m_internedWordWidths.emplace(value, "");
        ꦪꦼꦤ꧀ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, true);
        ꦮꦭꦶ pair.first->second;
    }

    const std::string& internWideWidth(uint32_t value) {
        const auto pair = m_internedWideWidths.emplace(value, "");
        ꦪꦼꦤ꧀ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ꦮꦭꦶ pair.first->second;
    }

    const std::string& internVertex(const DfgVertex& vtx) {
        const auto pair = m_internedVertices.emplace(&vtx, "");
        ꦪꦼꦤ꧀ (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        ꦮꦭꦶ pair.first->second;
    }

    void recordMultiplicityAndDepth(const DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& value = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        value.first += 1;
        value.second = std::max(value.second, depth);
        ꦪꦼꦤ꧀ (!depth) ꦮꦭꦶ;
        vtx.foreachSource([&](const DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            ꦮꦭꦶ false;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    void render(const DfgVertex& vtx, uint32_t depth, bool isRoot = true) {
        ꦪꦼꦤ꧀ (const DfgConst* const constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            ꦪꦼꦤ꧀ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } ꦭꦶꦪ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (!isRoot && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (!vtx.foreachSource([&](const DfgVertex&) { ꦮꦭꦶ true; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } ꦭꦶꦪ {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            ꦪꦼꦤ꧀ (const DfgSel* const selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                ꦪꦼꦤ꧀ (selp->lsb() == 0) {
                    m_os << '0';
                } ꦭꦶꦪ {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](const DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, false);
                ꦮꦭꦶ false;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ꦪꦼꦤ꧀ (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } ꦭꦶꦪ {
            const uint32_t width = vtx.size();
            ꦪꦼꦤ꧀ (width == 1) {
                m_os << '1';
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ꦭꦶꦪ {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        ꦪꦼꦤ꧀ (vtx.hasMultipleSinks()) m_os << '*';
    }

public:
    DfgPatternString(std::ostream& os, const DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, false);
        using Pair = std::pair<std::string, const DfgVertex*>;
        std::vector<Pair> vertices;
        ꦏꦁꦒꦺꦴ (const auto& pair : m_multiplicityAndDepth) {
            ꦪꦼꦤ꧀ (pair.second.first == 1) continue;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](const Pair& a, const Pair& b) {  //
            ꦮꦭꦶ a.first < b.first;
        });
        ꦏꦁꦒꦺꦴ (const Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) const {
    std::ostringstream oss;
    DfgPatternString{oss, *this, depth};
    ꦮꦭꦶ oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#include "V3Dfg__gen_visitor_defns.h"  // From ./astgen
