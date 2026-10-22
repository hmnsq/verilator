// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph optimizations
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

#define VL_MT_DISABLED_CODE_UNIT 1

#include "config_build.h"
#include "verilatedos.h"

#include "V3Graph.h"

#include "V3File.h"
#include "V3Global.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, const V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(this);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(this);
}

void V3GraphVertex::unlinkEdges(V3Graph*) {
    동안 (V3GraphEdge* const ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    동안 (V3GraphEdge* const ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

void V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(this);
    // Delete
    delete this;  // this=nullptr;
}

void V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    위해 (const V3GraphEdge& iedge : inEdges()) {
        위해 (const V3GraphEdge& oedge : outEdges()) {
            new V3GraphEdge{graphp, iedge.fromp(), oedge.top(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
void V3GraphVertex::v3errorEnd(const std::ostringstream& str) const  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    만약 (debug()) nsstr << "\n-vertex: " << this << '\n';
    만약 (FileLine* const flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } 아니면 {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
void V3GraphVertex::v3errorEndFatal(const std::ostringstream& str) const
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    assert(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    만약 (vertexp->rank()) os << " r" << vertexp->rank();
    만약 (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    만약 (vertexp->color()) os << " c" << vertexp->color();
    반환 os;
}

//######################################################################
//######################################################################
// Edges

void V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* top, 정수 weight,
                       bool cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(top, "Null to pointer");
    m_fromp = fromp;
    m_top = top;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(this);
    m_top->inEdges().linkBack(this);
}

void V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(this);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(this);
}

void V3GraphEdge::relinkTop(V3GraphVertex* newTop) {
    m_top->inEdges().unlink(this);
    m_top = newTop;
    m_top->inEdges().linkBack(this);
}

void V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(this);
    // Unlink to side
    m_top->inEdges().unlink(this);
    // Delete
    delete this;
}

std::string V3GraphEdge::name() const { 반환 m_fromp->name() + "->" + m_top->name(); }

정수 V3GraphEdge::sortCmp(const V3GraphEdge* rhsp) const {
    만약 (!m_weight || !rhsp->m_weight) 반환 0;
    반환 top()->sortCmp(rhsp->top());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

void V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    위해 (V3GraphVertex& vertex : vertices()) {
        동안 (V3GraphEdge* const edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    동안 (V3GraphVertex* const vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

void V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    위해 (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

void V3Graph::userClearEdges() {
    // Clear user() in all of tree
    위해 (V3GraphVertex& vertex : vertices()) {
        위해 (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

void V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    만약 (debug())
        반환 "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    아니면
        반환 "";
}

void V3Graph::dump(std::ostream& os) const {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    위해 (const V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        만약 (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        위해 (const V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        위해 (const V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

void V3Graph::dumpEdge(std::ostream& os, const V3GraphVertex& vertex,
                       const V3GraphEdge& edge) const {
    만약 (edge.weight() && (edge.fromp() == &vertex || edge.top() == &vertex)) {
        os << "\t\t";
        만약 (edge.fromp() == &vertex) os << "-> " << edge.top()->name();
        만약 (edge.top() == &vertex) os << "<- " << edge.fromp()->name();
        만약 (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
void V3Graph::dumpEdges(std::ostream& os, const V3GraphVertex& vertex) const {
    위해 (const V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    위해 (const V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

void V3Graph::dumpDotFilePrefixed(const string& nameComment, bool colorAsSubgraph) const {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
void V3Graph::dumpDotFilePrefixedAlways(const string& nameComment, bool colorAsSubgraph) const {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

void V3Graph::dumpDotFile(const string& filename, bool colorAsSubgraph) const {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    const std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    만약 (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, const V3GraphVertex*> rankSets;
    std::multimap<std::string, const V3GraphVertex*> subgraphs;
    위해 (const V3GraphVertex& vertex : vertices()) {
        const string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        const string& dotRank = vertex.dotRank();
        만약 (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<const V3GraphVertex*, 정수> numMap;

    // Print vertices
    정수 n = 0;
    string subgr;
    위해 (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        const string vertexSubgraph = it->first;
        const V3GraphVertex* vertexp = it->second;
        numMap[vertexp] = n;
        만약 (subgr != vertexSubgraph) {
            만약 (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            만약 (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        만약 (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        만약 (vertexp->rank()) *logp << " r" << vertexp->rank();
        만약 (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        만약 (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        만약 (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        만약 (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    만약 (subgr != "") *logp << "\t};\n";

    // Print edges
    위해 (const V3GraphVertex& vertex : vertices()) {
        위해 (const V3GraphEdge& edge : vertex.outEdges()) {
            만약 (edge.weight()) {
                const 정수 fromVnum = numMap[edge.fromp()];
                const 정수 toVnum = numMap[edge.top()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.top()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                만약 (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    위해 (const auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        만약 (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } 아니면 {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        위해 (auto it{bounds.first}; it != bounds.second; ++it) {
            만약 (it != bounds.first) *logp << ", ";
            *logp << 'n' << numMap[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

void V3Graph::hashGraphDebug(const char* debugName) const {
    // Disabled when there are no nondeterminism issues in flight.
    만약 (!v3Global.opt.debugNondeterminism()) 반환;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<const V3GraphVertex*, uint32_t> vx2Id;
    위해 (const V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    위해 (const V3GraphVertex& vtx : vertices()) {
        위해 (const V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.top()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.value()));
}
