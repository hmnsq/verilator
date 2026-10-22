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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ಧ್ರುವ V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ಏತತ್);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ಏತತ್);
}

ಶೂನ್ಯ V3GraphVertex::unlinkEdges(V3Graph*) {
    ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

ಶೂನ್ಯ V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ಏತತ್);
    // Delete
    delete ಏತತ್;  // this=nullptr;
}

ಶೂನ್ಯ V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& iedge : inEdges()) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& oedge : outEdges()) {
            ನೂತನ V3GraphEdge{graphp, iedge.fromp(), oedge.ತುದಿ(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
ಶೂನ್ಯ V3GraphVertex::v3errorEnd(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ಯದಿ (debug()) nsstr << "\n-vertex: " << ಏತತ್ << '\n';
    ಯದಿ (File_ಸಾಲು* ಧ್ರುವ flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } ಅನ್ಯಥಾ {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
ಶೂನ್ಯ V3GraphVertex::v3errorEndFatal(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ಪ್ರತಿಜ್ಞಾ(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    ಯದಿ (vertexp->rank()) os << " r" << vertexp->rank();
    ಯದಿ (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    ಯದಿ (vertexp->color()) os << " c" << vertexp->color();
    ಪ್ರತ್ಯಾಗಮ os;
}

//######################################################################
//######################################################################
// Edges

ಶೂನ್ಯ V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight,
                       ತಾರ್ಕಿಕ cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(ತುದಿ, "Null to pointer");
    m_fromp = fromp;
    m_ತುದಿ = ತುದಿ;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(ಏತತ್);
    m_ತುದಿ->inEdges().linkBack(ಏತತ್);
}

ಶೂನ್ಯ V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(ಏತತ್);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(ಏತತ್);
}

ಶೂನ್ಯ V3GraphEdge::relink_ತುದಿ(V3GraphVertex* new_ತುದಿ) {
    m_ತುದಿ->inEdges().unlink(ಏತತ್);
    m_ತುದಿ = new_ತುದಿ;
    m_ತುದಿ->inEdges().linkBack(ಏತತ್);
}

ಶೂನ್ಯ V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(ಏತತ್);
    // Unlink to side
    m_ತುದಿ->inEdges().unlink(ಏತತ್);
    // Delete
    delete ಏತತ್;
}

std::string V3GraphEdge::name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fromp->name() + "->" + m_ತುದಿ->name(); }

ಪೂರ್ಣಾಙ್ಕ V3GraphEdge::sortCmp(ಧ್ರುವ V3GraphEdge* rhsp) ಧ್ರುವ {
    ಯದಿ (!m_weight || !rhsp->m_weight) ಪ್ರತ್ಯಾಗಮ 0;
    ಪ್ರತ್ಯಾಗಮ ತುದಿ()->sortCmp(rhsp->ತುದಿ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

ಶೂನ್ಯ V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    ಪ್ರತಿ (V3GraphVertex& vertex : vertices()) {
        ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    ಯಾವತ್ (V3GraphVertex* ಧ್ರುವ vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

ಶೂನ್ಯ V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    ಪ್ರತಿ (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

ಶೂನ್ಯ V3Graph::userClearEdges() {
    // Clear user() in all of tree
    ಪ್ರತಿ (V3GraphVertex& vertex : vertices()) {
        ಪ್ರತಿ (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

ಶೂನ್ಯ V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    ಯದಿ (debug())
        ಪ್ರತ್ಯಾಗಮ "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    ಅನ್ಯಥಾ
        ಪ್ರತ್ಯಾಗಮ "";
}

ಶೂನ್ಯ V3Graph::dump(std::ostream& os) ಧ್ರುವ {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        ಯದಿ (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

ಶೂನ್ಯ V3Graph::dumpEdge(std::ostream& os, ಧ್ರುವ V3GraphVertex& vertex,
                       ಧ್ರುವ V3GraphEdge& edge) ಧ್ರುವ {
    ಯದಿ (edge.weight() && (edge.fromp() == &vertex || edge.ತುದಿ() == &vertex)) {
        os << "\t\t";
        ಯದಿ (edge.fromp() == &vertex) os << "-> " << edge.ತುದಿ()->name();
        ಯದಿ (edge.ತುದಿ() == &vertex) os << "<- " << edge.fromp()->name();
        ಯದಿ (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
ಶೂನ್ಯ V3Graph::dumpEdges(std::ostream& os, ಧ್ರುವ V3GraphVertex& vertex) ಧ್ರುವ {
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

ಶೂನ್ಯ V3Graph::dumpDotFilePrefixed(ಧ್ರುವ string& nameComment, ತಾರ್ಕಿಕ colorAsSubgraph) ಧ್ರುವ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
ಶೂನ್ಯ V3Graph::dumpDotFilePrefixedAlways(ಧ್ರುವ string& nameComment, ತಾರ್ಕಿಕ colorAsSubgraph) ಧ್ರುವ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

ಶೂನ್ಯ V3Graph::dumpDotFile(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ colorAsSubgraph) ಧ್ರುವ {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ಧ್ರುವ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ಯದಿ (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, ಧ್ರುವ V3GraphVertex*> rankSets;
    std::multimap<std::string, ಧ್ರುವ V3GraphVertex*> subgraphs;
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vertex : vertices()) {
        ಧ್ರುವ string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ಧ್ರುವ string& dotRank = vertex.dotRank();
        ಯದಿ (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ಧ್ರುವ V3GraphVertex*, ಪೂರ್ಣಾಙ್ಕ> ಅಂಕೆ_Map;

    // Print vertices
    ಪೂರ್ಣಾಙ್ಕ n = 0;
    string subgr;
    ಪ್ರತಿ (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ಧ್ರುವ string vertexSubgraph = it->first;
        ಧ್ರುವ V3GraphVertex* vertexp = it->second;
        ಅಂಕೆ_Map[vertexp] = n;
        ಯದಿ (subgr != vertexSubgraph) {
            ಯದಿ (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            ಯದಿ (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        ಯದಿ (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        ಯದಿ (vertexp->rank()) *logp << " r" << vertexp->rank();
        ಯದಿ (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        ಯದಿ (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        ಯದಿ (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        ಯದಿ (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    ಯದಿ (subgr != "") *logp << "\t};\n";

    // Print edges
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vertex : vertices()) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vertex.outEdges()) {
            ಯದಿ (edge.weight()) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fromVnum = ಅಂಕೆ_Map[edge.fromp()];
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ toVnum = ಅಂಕೆ_Map[edge.ತುದಿ()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.ತುದಿ()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                ಯದಿ (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    ಪ್ರತಿ (ಧ್ರುವ auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        ಯದಿ (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } ಅನ್ಯಥಾ {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        ಪ್ರತಿ (auto it{bounds.first}; it != bounds.second; ++it) {
            ಯದಿ (it != bounds.first) *logp << ", ";
            *logp << 'n' << ಅಂಕೆ_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

ಶೂನ್ಯ V3Graph::hashGraphDebug(ಧ್ರುವ ವರ್ಣ* debugName) ಧ್ರುವ {
    // Disabled when there are no nondeterminism issues in flight.
    ಯದಿ (!v3Global.opt.debugNondeterminism()) ಪ್ರತ್ಯಾಗಮ;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ಧ್ರುವ V3GraphVertex*, uint32_t> vx2Id;
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.ತುದಿ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.ಮೌಲ್ಯ()));
}
