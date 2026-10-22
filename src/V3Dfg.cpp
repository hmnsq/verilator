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
    якщо (otherps.empty()) повернути;

    // NODE STATE
    // AstVarScope::user2p() -> corresponding DfgVertexVar* in 'this' graph
    const VNUser2InUse user2InUse;

    // Set up Ast Variable -> DfgVertexVar map for 'this' graph
    для (DfgVertexVar& vtx : m_varVertices) vtx.vscp()->user2p(&vtx);

    // Merge in each of the other graphs
    для (const std::unique_ptr<DfgGraph>& otherp : otherps) {
        // Process variables
        для (DfgVertexVar* const vtxp : otherp->m_varVertices.unlinkable()) {
            // Variabels that are present in 'this', make them use the DfgVertexVar in 'this'.
            якщо (DfgVertexVar* const altp = vtxp->vscp()->user2u().to<DfgVertexVar*>()) {
                DfgVertex* const srcp = vtxp->srcp();
                DfgVertex* const defaultp = vtxp->defaultp();
                UASSERT_OBJ(!(srcp || defaultp) || (!altp->srcp() && !altp->defaultp()), vtxp,
                            "At most one alias should be driven");
                vtxp->replaceWith(altp);
                якщо (srcp) altp->srcp(srcp);
                якщо (defaultp) altp->defaultp(defaultp);
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
        для (DfgVertexAst* const vtxp : otherp->m_astVertices.unlinkable()) {
            vtxp->m_userGeneration = 0;
#ifdef VL_DEBUG
            vtxp->m_dfgp = this;
#endif
        }
        m_astVertices.splice(m_astVertices.end(), otherp->m_astVertices);
        // Process constants
        для (DfgConst& vtx : otherp->m_constVertices) {
            vtx.m_userGeneration = 0;
#ifdef VL_DEBUG
            vtx.m_dfgp = this;
#endif
        }
        m_constVertices.splice(m_constVertices.end(), otherp->m_constVertices);
        // Process operations
        для (DfgVertex& vtx : otherp->m_opVertices) {
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
    якщо (dtype.isArray()) повернути new DfgVarArray{*this, vscp};
    повернути new DfgVarPacked{*this, vscp};
}

static const std::string toDotId(const DfgVertex& vtx) { повернути '"' + cvtToHex(&vtx) + '"'; }

// Dump one DfgVertex in Graphviz format
static void dumpDotVertex(std::ostream& os, const DfgVertex& vtx) {
    якщо (const DfgVertexVar* const varVtxp = vtx.cast<DfgVertexVar>()) {
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
        якщо (const AstVarScope* const tmpForp = varVtxp->tmpForp()) {
            якщо (tmpForp != vscp) os << "\ntemporary for: " << tmpForp->prettyName();
        }
        // Type and fanout
        os << '\n';
        varVtxp->dtype().astDtypep()->dumpSmall(os);
        os << " / F" << varVtxp->fanout();
        // Reference flags
        os << " / ";
        static const char* const rwmn[2][2] = {{"_", "W"}, {"R", "M"}};
        os << rwmn[varVtxp->hasExtRdRefs()][varVtxp->hasExtWrRefs()];
        os << rwmn[false][varVtxp->hasModWrRefs()];
        os << (varVtxp->hasDfgRefs() ? "D" : "_");
        // End 'label'
        os << '"';
        // Shape
        якщо (varVtxp->is<DfgVarPacked>()) {
            os << ", shape=box";
        } інакше якщо (varVtxp->is<DfgVarArray>()) {
            os << ", shape=box3d";
        } інакше {
            varVtxp->v3fatalSrc("Unhandled DfgVertexVar sub-type");
        }
        // Color
        const char* const colorp = varVtxp->hasExtRefs()     ? "firebrick2"  // Red
                                   : varVtxp->hasModWrRefs() ? "darkorange1"  // Orange
                                   : varVtxp->hasDfgRefs()   ? "gold2"  // Yellow
                                   : varVtxp->tmpForp()      ? "gray95"  // Gray
                                                             : "white";
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        повернути;
    }

    якщо (const DfgPrev* const prevVtxp = vtx.cast<DfgPrev>()) {
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
        якщо (prevVtxp->isPacked()) {
            os << ", shape=box";
        } інакше якщо (prevVtxp->isArray()) {
            os << ", shape=box3d";
        } інакше {
            prevVtxp->v3fatalSrc("Unhandled variable type");
        }
        // Color
        const char* const colorp = "mediumorchid1";  // Purple
        os << ", style=filled";
        os << ", fillcolor=\"" << colorp << "\"";
        // End attributes
        os << "]\n";
        повернути;
    }

    якщо (const DfgConst* const constVtxp = vtx.cast<DfgConst>()) {
        const V3Number& num = constVtxp->num();

        os << toDotId(vtx);
        os << " [label=\"";
        якщо (num.width() <= 32 && !num.isSigned()) {
            os << constVtxp->width() << "'d" << num.toUInt() << '\n';
            os << constVtxp->width() << "'h" << std::hex << num.toUInt() << std::dec << '\n';
        } інакше {
            os << num.ascii() << '\n';
        }
        os << cvtToHex(constVtxp) << '\n';
        os << '"';
        os << ", shape=plain";
        os << "]\n";
        повернути;
    }

    якщо (const DfgSel* const selVtxp = vtx.cast<DfgSel>()) {
        const uint32_t lsb = selVtxp->lsb();
        const uint32_t msb = lsb + selVtxp->width() - 1;
        os << toDotId(vtx);
        os << " [label=\"SEL _[" << msb << ":" << lsb << "]\n";
        os << cvtToHex(selVtxp) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        якщо (vtx.hasMultipleSinks()) {
            os << ", shape=doublecircle";
        } інакше {
            os << ", shape=circle";
        }
        os << "]\n";
        повернути;
    }

    якщо (vtx.is<DfgVertexSplice>() || vtx.is<DfgUnitArray>() || vtx.is<DfgUnresolved>()) {
        os << toDotId(vtx);
        os << " [label=\"" << vtx.typeName() << '\n';
        os << cvtToHex(&vtx) << '\n';
        vtx.dtype().astDtypep()->dumpSmall(os);
        os << " / F" << vtx.fanout() << '"';
        якщо (vtx.hasMultipleSinks()) {
            os << ", shape=doubleoctagon";
        } інакше {
            os << ", shape=octagon";
        }
        os << "]\n";
        повернути;
    }

    якщо (const DfgLogic* const logicp = vtx.cast<DfgLogic>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(logicp->nodep(), ss);
        std::string str = "AstNode: " + cvtToHex(logicp->nodep()) + "\n" + ss.str();
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        const char* const colorp = !logicp->selectedForSynthesis() ? "#d0d0ff"  // Pale Blue
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
        повернути;
    }

    якщо (const DfgVertexAst* const astVtxp = vtx.cast<DfgVertexAst>()) {
        os << toDotId(vtx);
        std::stringstream ss;
        V3EmitV::debugVerilogForTree(astVtxp->exprp(), ss);
        std::string str = "AstNode: " + cvtToHex(astVtxp->exprp()) + "\n" + ss.str() + "\n";
        str = VString::quoteBackslash(str);
        str = VString::quoteAny(str, '"', '\\');
        str = VString::replaceSubstr(str, "\n", "\\l");
        const DfgAstRd* const astRdVtxp = astVtxp->cast<DfgAstRd>();
        const char* const colorp = astRdVtxp ? "#80ff80"  // Green
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
        повернути;
    }

    os << toDotId(vtx);
    os << " [label=\"" << vtx.typeName() << '\n';
    os << cvtToHex(&vtx) << '\n';
    vtx.dtype().astDtypep()->dumpSmall(os);
    os << " / F" << vtx.fanout() << '"';
    якщо (vtx.hasMultipleSinks()) {
        os << ", shape=doublecircle";
    } інакше {
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
    якщо (!p) p = [](const DfgVertex&) { повернути true; };

    std::unordered_set<const DfgVertex*> emitted;
    // Emit all vertices associated with a DfgLogic
    forEachVertex([&](const DfgVertex& vtx) {
        const DfgLogic* const logicp = vtx.cast<DfgLogic>();
        якщо (!logicp) повернути;
        якщо (logicp->synth().empty()) повернути;
        якщо (!p(vtx)) повернути;
        os << "subgraph cluster_" << cvtToHex(logicp) << " {\n";
        dumpDotVertex(os, *logicp);
        emitted.insert(logicp);
        для (DfgVertex* const vtxp : logicp->synth()) {
            якщо (!p(*vtxp)) continue;
            dumpDotVertex(os, *vtxp);
            emitted.insert(vtxp);
        }
        os << "}\n";
    });
    // Emit all remaining vertices
    forEachVertex([&](const DfgVertex& vtx) {
        якщо (emitted.count(&vtx)) повернути;
        якщо (!p(vtx)) повернути;
        dumpDotVertex(os, vtx);
    });
    // Emit all edges
    forEachVertex([&](const DfgVertex& vtx) {
        якщо (!p(vtx)) повернути;
        для (size_t i = 0; i < vtx.nInputs(); ++i) {
            DfgVertex* const srcp = vtx.inputp(i);
            якщо (!srcp) continue;
            якщо (!p(*srcp)) continue;
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
    повернути ss.str();
}

void DfgGraph::dumpDotFile(const std::string& filename, const std::string& label,
                           std::function<bool(const DfgVertex&)> p) const {
    const std::unique_ptr<std::ofstream> os{V3File::new_ofstream(filename)};
    якщо (os->fail()) v3fatal("Can't write file: " << filename);
    dumpDot(*os.get(), label, p);
    os->close();
}

void DfgGraph::dumpDotFilePrefixed(const std::string& label,
                                   std::function<bool(const DfgVertex&)> p) const {
    std::string filename = name();
    якщо (!label.empty()) filename += "-" + label;
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
    поки (!queue.empty()) {
        // Pop next work item
        const DfgVertex* const vtxp = queue.back();
        queue.pop_back();
        // Mark vertex as visited, move on if already visited
        якщо (!resp->insert(vtxp).second) continue;
        // Enqueue all siblings of this vertex.
        якщо VL_CONSTEXPR_CXX17 (T_SinksNotSources) {
            vtxp->foreachSink([&](const DfgVertex& sink) {
                queue.push_back(&sink);
                повернути false;
            });
        } інакше {
            vtxp->foreachSource([&](const DfgVertex& src) {
                queue.push_back(&src);
                повернути false;
            });
        }
    }
    // Done
    повернути resp;
}

std::unique_ptr<std::unordered_set<const DfgVertex*>>
DfgGraph::sourceCone(const std::vector<const DfgVertex*>& vtxps) const {
    повернути dfgGraphCollectCone<false>(vtxps);
}

std::unique_ptr<std::unordered_set<const DfgVertex*>>
DfgGraph::sinkCone(const std::vector<const DfgVertex*>& vtxps) const {
    повернути dfgGraphCollectCone<true>(vtxps);
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
    для (size_t hops = 1; hops <= n; ++hops) {
        для (size_t i = begin; i < end; ++i) {
            const DfgVertex* const vtxp = vec[i];
            vtxp->foreachSink([&](const DfgVertex& dst) {
                якщо (res.insert(&dst).second) vec.push_back(&dst);
                повернути false;
            });
            vtxp->foreachSource([&](const DfgVertex& src) {
                якщо (res.insert(&src).second) vec.push_back(&src);
                повернути false;
            });
        }
        begin = end;
        end = vec.size();
    }
    // Move out the results
    повернути std::make_unique<std::unordered_set<const DfgVertex*>>(std::move(res));
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
    якщо (is<DfgMux>()) повернути true;
    якщо (is<DfgArraySel>()) повернути !as<DfgArraySel>()->bitp()->is<DfgConst>();
    повернути false;
}

void DfgVertex::typeCheck(const DfgGraph& dfg) const {

#define CHECK(cond, msg) \
    UASSERT_OBJ(cond, this, \
                "Dfg type error for vertex " << typeName() << " in " << dfg.name() << ": " \
                                             << msg);
    switch (type()) {
    case VDfgType::Const: {
        CHECK(isPacked(), "Should be Packed type");
        повернути;
    }
    case VDfgType::CReset: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        повернути;
    }
    case VDfgType::AstRd: {
        const DfgAstRd& v = *as<DfgAstRd>();
        CHECK(v.isPacked() || v.isArray(), "Should be Packed or Array type");
        повернути;
    }
    case VDfgType::VarArray:
    case VDfgType::VarPacked: {
        const DfgVertexVar& v = *as<DfgVertexVar>();
        CHECK(!v.defaultp() || v.defaultp()->dtype() == v.dtype(), "'defaultp' should match");
        CHECK(!v.srcp() || v.srcp()->dtype() == v.dtype(), "'srcp' should match");
        повернути;
    }
    case VDfgType::Prev: {
        CHECK(isPacked() || isArray(), "Should be Packed or Array type");
        повернути;
    }
    case VDfgType::SpliceArray:
    case VDfgType::SplicePacked: {
        const DfgVertexSplice& v = *as<DfgVertexSplice>();
        v.foreachDriver([&](const DfgVertex& src, uint32_t lo) {
            CHECK(src.dtype() == DfgDataType::select(v.dtype(), lo, src.size()), "driver");
            повернути false;
        });
        повернути;
    }
    case VDfgType::Logic: {
        CHECK(dtype().isNull(), "Should be Null type");
        повернути;
    }
    case VDfgType::Unresolved: {
        CHECK(!dtype().isNull(), "Should not be Null type");
        повернути;
    }
    case VDfgType::UnitArray: {
        const DfgUnitArray& v = *as<DfgUnitArray>();
        CHECK(v.isArray(), "Should be Array type");
        CHECK(v.size() == 1, "Should be one element");
        CHECK(v.srcp()->dtype() == v.dtype().elemDtype(), "Input should be the element type");
        повернути;
    }
    case VDfgType::Sel: {
        const DfgSel& v = *as<DfgSel>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.dtype() == DfgDataType::select(v.srcp()->dtype(), v.lsb(), v.size()), "sel");
        повернути;
    }
    case VDfgType::MatchMasked: {
        const DfgMatchMasked& v = *as<DfgMatchMasked>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        CHECK(v.lhsp()->isPacked(), "Lhs should be packed");
        CHECK(v.matchp()->isPacked(), "Match should be Packed type");
        CHECK(v.matchp()->is<DfgVertexVar>(), "Match should be a variable");
        повернути;
    }
    case VDfgType::Mux: {
        const DfgMux& v = *as<DfgMux>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.fromp()->isPacked(), "Source operand should be Packed type");
        CHECK(v.fromp()->size() >= v.size(), "Source operand should not be narrower");
        CHECK(v.lsbp()->isPacked(), "Index should be Packed type");
        повернути;
    }
    case VDfgType::ArraySel: {
        const DfgArraySel& v = *as<DfgArraySel>();
        CHECK(v.dtype() == v.fromp()->dtype().elemDtype(), "Element type should match");
        CHECK(v.bitp()->isPacked(), "Index should be Packed type");
        повернути;
    }

    case VDfgType::Add:
    case VDfgType::And:
    case VDfgType::Div:
    case VDfgType::DivS:
    case VDfgType::ModDiv:
    case VDfgType::ModDivS:
    case VDfgType::Mul:
    case VDfgType::MulS:
    case VDfgType::Or:
    case VDfgType::Sub:
    case VDfgType::Xor: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->dtype() == dtype(), "RHS should be same type");
        повернути;
    }

    case VDfgType::Negate:
    case VDfgType::Not: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "Input should be same type");
        повернути;
    }

    case VDfgType::ShiftL:
    case VDfgType::ShiftR:
    case VDfgType::ShiftRS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        повернути;
    }

    case VDfgType::Concat: {
        const DfgConcat& v = *as<DfgConcat>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.lhsp()->isPacked(), "LHS should be Packed type");
        CHECK(v.rhsp()->isPacked(), "RHS should be Packed type");
        CHECK(v.size() == v.rhsp()->size() + v.lhsp()->size(), "Concat result mismatch");
        повернути;
    }

    case VDfgType::Rep: {
        const DfgRep& v = *as<DfgRep>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "'srcp' should be same type");
        CHECK(v.size() % v.srcp()->size() == 0, "Not a replicate");
        повернути;
    }

    case VDfgType::StreamL:
    case VDfgType::StreamR: {
        // TODO: model these without an explicit slice size which is always constant (?)
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "Slice size should be Packed type");
        повернути;
    }

    case VDfgType::Eq:
    case VDfgType::EqCase:
    case VDfgType::EqWild:
    case VDfgType::Neq:
    case VDfgType::NeqCase:
    case VDfgType::NeqWild:
    case VDfgType::Gt:
    case VDfgType::GtS:
    case VDfgType::Gte:
    case VDfgType::GteS:
    case VDfgType::Lt:
    case VDfgType::LtS:
    case VDfgType::Lte:
    case VDfgType::LteS: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->dtype() == inputp(1)->dtype(), "Sides should match");
        повернути;
    }

    case VDfgType::Extend:
    case VDfgType::ExtendS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->isPacked(), "Operand should be same type");
        CHECK(inputp(0)->size() < size(), "Operand should be narrower");
        повернути;
    }

    case VDfgType::LogAnd:
    case VDfgType::LogEq:
    case VDfgType::LogIf:
    case VDfgType::LogOr: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "LHS should be Packed type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        повернути;
    }

    case VDfgType::LogNot:
    case VDfgType::RedAnd:
    case VDfgType::RedOr:
    case VDfgType::RedXor:
    case VDfgType::OneHot:
    case VDfgType::OneHot0: {
        CHECK(dtype() == DfgDataType::packed(1), "Should be 1-bit");
        CHECK(inputp(0)->isPacked(), "Operand should be Packed type");
        повернути;
    }

    case VDfgType::Cond: {
        const DfgCond& v = *as<DfgCond>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.condp()->isPacked(), "Condition should be Packed type");
        CHECK(v.thenp()->dtype() == v.dtype(), "Then should be same type");
        CHECK(v.elsep()->dtype() == v.dtype(), "Else should be same type");
        повернути;
    }

    case VDfgType::CountOnes: {
        const DfgCountOnes& v = *as<DfgCountOnes>();
        CHECK(v.isPacked(), "Should be Packed type");
        CHECK(v.srcp()->isPacked(), "Source should be Packed type");
        CHECK(v.size() == 32U, "Should yield a 32-bit result");
        повернути;
    }

    case VDfgType::Pow:
    case VDfgType::PowSS:
    case VDfgType::PowSU:
    case VDfgType::PowUS: {
        CHECK(isPacked(), "Should be Packed type");
        CHECK(inputp(0)->dtype() == dtype(), "LHS should be same type");
        CHECK(inputp(1)->isPacked(), "RHS should be Packed type");
        повернути;
    }
    }
#undef CHECK
}

uint32_t DfgVertex::fanout() const {
    uint32_t result = 0;
    foreachSink([&](const DfgVertex&) {
        ++result;
        повернути false;
    });
    повернути result;
}

DfgVertexVar* DfgVertex::getResultVar() {
    // It's easy if the vertex is already a variable ...
    якщо (DfgVertexVar* const varp = this->cast<DfgVertexVar>()) повернути varp;

    // Inspect existing variables written by this vertex, and choose one
    DfgVertexVar* resp = nullptr;
    // cppcheck-has-bug-suppress constParameter
    this->foreachSink([&resp](DfgVertex& sink) {
        DfgVertexVar* const varp = sink.cast<DfgVertexVar>();
        якщо (!varp) повернути false;

        // Do not use it if value might differ from its drivers
        якщо (varp->isVolatile()) повернути false;

        // First variable found
        якщо (!resp) {
            resp = varp;
            повернути false;
        }

        // Prefer those variables that must be kept anyway
        якщо (resp->hasExtRdRefs() != varp->hasExtRdRefs()) {
            якщо (!resp->hasExtRdRefs()) resp = varp;
            повернути false;
        }
        якщо (resp->hasDfgRefs() != varp->hasDfgRefs()) {
            якщо (!resp->hasDfgRefs()) resp = varp;
            повернути false;
        }

        // Prefer real variables over temporaries
        const bool resIsTemp = resp->vscp()->varp()->isTemp();
        const bool varIsTemp = varp->vscp()->varp()->isTemp();
        якщо (resIsTemp != varIsTemp) {
            якщо (resIsTemp) resp = varp;
            повернути false;
        }
        якщо (!resp->tmpForp() != !varp->tmpForp()) {
            якщо (resp->tmpForp()) resp = varp;
            повернути false;
        }

        // Prefer the earlier one in source order
        const FileLine& oldFlp = *(resp->fileline());
        const FileLine& newFlp = *(varp->fileline());
        якщо (const ціле cmp = oldFlp.operatorCompare(newFlp)) {
            якщо (cmp > 0) resp = varp;
            повернути false;
        }
        // Prefer the one with the lexically smaller name
        якщо (const ціле cmp = resp->vscp()->name().compare(varp->vscp()->name())) {
            якщо (cmp > 0) resp = varp;
            повернути false;
        }
        // 'resp' and 'varp' are all the same, keep using the existing 'resp'
        повернути false;
    });
    повернути resp;
}

AstScope* DfgVertex::scopep(ScopeCache& cache, bool tryResultVar) VL_MT_DISABLED {
    // If this is a variable, we are done
    якщо (const DfgVertexVar* const varp = this->cast<DfgVertexVar>()) {
        повернути varp->vscp()->scopep();
    }

    // Try the result var first if instructed (usully only in the recursive case)
    якщо (tryResultVar) {
        якщо (const DfgVertexVar* const varp = this->getResultVar()) {
            повернути varp->vscp()->scopep();
        }
    }

    AstScope* const rootp = v3Global.rootp()->topScopep()->scopep();

    // Note: the recursive invocation can cause a re-hash but that will not invalidate references
    AstScope*& resultr = cache[this];
    якщо (!resultr) {
        // Mark to prevent infinite recursion on circular graphs - should never be called on such
        resultr = reinterpret_cast<AstScope*>(1);
        // Find scope based on sources, falling back on the root scope. Never use a package
        // scope, including the constant pool, so new variables are not created in packages.
        AstScope* foundp = nullptr;
        foreachSource([&](DfgVertex& src) {
            AstScope* const scp = src.scopep(cache, true);
            якщо (scp != rootp && !VN_IS(scp->modp(), Package)) {
                foundp = scp;
                повернути true;
            }
            повернути false;
        });
        resultr = foundp ? foundp : rootp;
    }

    // Die on a graph circular through operation vertices
    UASSERT_OBJ(resultr != reinterpret_cast<AstScope*>(1), this,
                "DfgVertex::scopep called on graph with circular operations");

    // Done
    повернути resultr;
}

void DfgVertex::unlinkDelete(DfgGraph& dfg) {
    // Unlink sink edges
    поки (!m_sinks.empty()) m_sinks.frontp()->unlinkSrcp();
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
    якщо (!srcp) повернути {nullptr, 0};
    // If volatile, can have other drivers
    якщо (isVolatile()) повернути {nullptr, 0};
    // Don't inline CReset
    якщо (srcp->is<DfgCReset>()) повернути {nullptr, 0};

    // If not driven via a splice, then it is driven whole, at the same offsets
    DfgVertexSplice* const splicep = srcp->cast<DfgVertexSplice>();
    якщо (!splicep) повернути {srcp, lo};

    // Find the driver that covers the whole searched range, if there is a single one
    const uint32_t hi = lo + size - 1;
    DfgVertex* driverp = nullptr;
    uint32_t driverLo = 0;
    bool useDefault = defaultp();
    splicep->foreachDriver([&](DfgVertex& src, const uint32_t dLo) {
        const uint32_t dHi = dLo + src.size() - 1;
        // Note whether it overlaps the searched range, so the default cannot be used
        якщо (dLo <= hi && lo <= dHi) useDefault = false;
        // If it does not cover the whole searched range, move on
        якщо (lo < dLo || dHi < hi) повернути false;
        // Save the driver that covers the whole searched range
        driverp = &src;
        driverLo = dLo;
        повернути true;
    });

    // If a single driver covers the searched range, it is the one
    якщо (driverp) повернути {driverp, lo - driverLo};
    // Otherwise the default driver is responsible for it, if nothing else overlaps it
    якщо (useDefault) повернути {defaultp(), lo};
    // Not driven by a single vertex
    повернути {nullptr, 0};
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
        const char base = lowerCase ? 'a' : 'A';
        std::string s;
        do { s += static_cast<char>(base + value % 26); } поки (value /= 26);
        повернути s;
    }

    const std::string& internConst(const DfgConst& vtx) {
        const auto pair = m_internedConsts.emplace(vtx.num().ascii(false), "");
        якщо (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        повернути pair.first->second;
    }

    const std::string& internSelLsb(uint32_t value) {
        const auto pair = m_internedSelLsbs.emplace(value, "");
        якщо (pair.second) pair.first->second += toLetters(m_internedSelLsbs.size() - 1);
        повернути pair.first->second;
    }

    const std::string& internWordWidth(uint32_t value) {
        const auto pair = m_internedWordWidths.emplace(value, "");
        якщо (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, true);
        повернути pair.first->second;
    }

    const std::string& internWideWidth(uint32_t value) {
        const auto pair = m_internedWideWidths.emplace(value, "");
        якщо (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        повернути pair.first->second;
    }

    const std::string& internVertex(const DfgVertex& vtx) {
        const auto pair = m_internedVertices.emplace(&vtx, "");
        якщо (pair.second) pair.first->second += toLetters(m_internedVertices.size() - 1);
        повернути pair.first->second;
    }

    void recordMultiplicityAndDepth(const DfgVertex& vtx, uint32_t depth) {
        std::pair<uint32_t, uint32_t>& value = m_multiplicityAndDepth
                                                   .emplace(std::piecewise_construct,  //
                                                            std::forward_as_tuple(&vtx),  //
                                                            std::forward_as_tuple(0, depth))
                                                   .first->second;
        value.first += 1;
        value.second = std::max(value.second, depth);
        якщо (!depth) повернути;
        vtx.foreachSource([&](const DfgVertex& src) {
            recordMultiplicityAndDepth(src, depth - 1);
            повернути false;
        });
    }

    // Render the vertx into ss, and return true if the recursion reached the given depth,
    // meaning an S-expression with that nesting level has been rendered.
    void render(const DfgVertex& vtx, uint32_t depth, bool isRoot = true) {
        якщо (const DfgConst* const constp = vtx.cast<DfgConst>()) {
            // Base case 1: constant
            якщо (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } інакше якщо (constp->isOnes()) {
                m_os << "(CONST ONES)";
            } інакше {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } інакше якщо (!isRoot && m_multiplicityAndDepth.at(&vtx).first > 1) {
            // Base case 2: vertex appearing multiple times
            m_os << internVertex(vtx);
        } інакше якщо (!vtx.foreachSource([&](const DfgVertex&) { повернути true; })) {
            // Base case 3: vertex with no inputs (input variable)
            m_os << '(' << vtx.typeName() << ')';
        } інакше якщо (depth == 0) {
            // Base case 4: deep vertex (apperaing only once)
            m_os << "_";
        } інакше {
            // Recursively print an S-expression for the vertex
            m_os << '(';
            // Name
            m_os << vtx.typeName();
            // Specials
            якщо (const DfgSel* const selp = vtx.cast<DfgSel>()) {
                m_os << '@';
                якщо (selp->lsb() == 0) {
                    m_os << '0';
                } інакше {
                    m_os << internSelLsb(selp->lsb());
                }
            }
            // Operands
            vtx.foreachSource([&](const DfgVertex& src) {
                m_os << ' ';
                render(src, depth - 1, false);
                повернути false;
            });
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        якщо (!vtx.dtype().isPacked()) {
            vtx.dtype().astDtypep()->dumpSmall(m_os);
        } інакше {
            const uint32_t width = vtx.size();
            якщо (width == 1) {
                m_os << '1';
            } інакше якщо (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } інакше {
                m_os << internWideWidth(width);
            }
        }

        // Mark it if it has multiple sinks
        якщо (vtx.hasMultipleSinks()) m_os << '*';
    }

public:
    DfgPatternString(std::ostream& os, const DfgVertex& vtx, uint32_t depth)
        : m_os{os} {
        recordMultiplicityAndDepth(vtx, depth);
        render(vtx, depth, false);
        using Pair = std::pair<std::string, const DfgVertex*>;
        std::vector<Pair> vertices;
        для (const auto& pair : m_multiplicityAndDepth) {
            якщо (pair.second.first == 1) continue;
            vertices.emplace_back(internVertex(*pair.first), pair.first);
        }
        std::sort(vertices.begin(), vertices.end(), [](const Pair& a, const Pair& b) {  //
            повернути a.first < b.first;
        });
        для (const Pair& pair : vertices) {
            m_os << " | " << pair.first << " is ";
            render(*pair.second, m_multiplicityAndDepth.at(pair.second).second);
        }
    }
};

std::string DfgVertex::patternString(uint32_t depth) const {
    std::ostringstream oss;
    DfgPatternString{oss, *this, depth};
    повернути oss.str();
}

//------------------------------------------------------------------------------
// DfgVisitor

#include "V3Dfg__gen_visitor_defns.h"  // From ./astgen
