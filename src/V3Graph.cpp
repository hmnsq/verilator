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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, અચલ V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(આ);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(આ);
}

શૂન્ય V3GraphVertex::unlinkEdges(V3Graph*) {
    જ્યારે (V3GraphEdge* અચલ ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    જ્યારે (V3GraphEdge* અચલ ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

શૂન્ય V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(આ);
    // Delete
    delete આ;  // this=nullptr;
}

શૂન્ય V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    માટે (અચલ V3GraphEdge& iedge : inEdges()) {
        માટે (અચલ V3GraphEdge& oedge : outEdges()) {
            નવું V3GraphEdge{graphp, iedge.fromp(), oedge.ટોચ(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
શૂન્ય V3GraphVertex::v3errorEnd(અચલ std::ostringstream& str) અચલ  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    જો (debug()) nsstr << "\n-vertex: " << આ << '\n';
    જો (File_લીટી* અચલ flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } નહીંતર {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
શૂન્ય V3GraphVertex::v3errorEndFatal(અચલ std::ostringstream& str) અચલ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    પ્રતિજ્ઞા(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    જો (vertexp->rank()) os << " r" << vertexp->rank();
    જો (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    જો (vertexp->color()) os << " c" << vertexp->color();
    પાછા os;
}

//######################################################################
//######################################################################
// Edges

શૂન્ય V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight,
                       તાર્કિક cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(ટોચ, "Null to pointer");
    m_fromp = fromp;
    m_ટોચ = ટોચ;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(આ);
    m_ટોચ->inEdges().linkBack(આ);
}

શૂન્ય V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(આ);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(આ);
}

શૂન્ય V3GraphEdge::relink_ટોચ(V3GraphVertex* new_ટોચ) {
    m_ટોચ->inEdges().unlink(આ);
    m_ટોચ = new_ટોચ;
    m_ટોચ->inEdges().linkBack(આ);
}

શૂન્ય V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(આ);
    // Unlink to side
    m_ટોચ->inEdges().unlink(આ);
    // Delete
    delete આ;
}

std::string V3GraphEdge::name() અચલ { પાછા m_fromp->name() + "->" + m_ટોચ->name(); }

પૂર્ણાંક V3GraphEdge::sortCmp(અચલ V3GraphEdge* rhsp) અચલ {
    જો (!m_weight || !rhsp->m_weight) પાછા 0;
    પાછા ટોચ()->sortCmp(rhsp->ટોચ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

શૂન્ય V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    માટે (V3GraphVertex& vertex : vertices()) {
        જ્યારે (V3GraphEdge* અચલ edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    જ્યારે (V3GraphVertex* અચલ vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

શૂન્ય V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    માટે (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

શૂન્ય V3Graph::userClearEdges() {
    // Clear user() in all of tree
    માટે (V3GraphVertex& vertex : vertices()) {
        માટે (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

શૂન્ય V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    જો (debug())
        પાછા "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    નહીંતર
        પાછા "";
}

શૂન્ય V3Graph::dump(std::ostream& os) અચલ {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    માટે (અચલ V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        જો (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        માટે (અચલ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        માટે (અચલ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

શૂન્ય V3Graph::dumpEdge(std::ostream& os, અચલ V3GraphVertex& vertex,
                       અચલ V3GraphEdge& edge) અચલ {
    જો (edge.weight() && (edge.fromp() == &vertex || edge.ટોચ() == &vertex)) {
        os << "\t\t";
        જો (edge.fromp() == &vertex) os << "-> " << edge.ટોચ()->name();
        જો (edge.ટોચ() == &vertex) os << "<- " << edge.fromp()->name();
        જો (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
શૂન્ય V3Graph::dumpEdges(std::ostream& os, અચલ V3GraphVertex& vertex) અચલ {
    માટે (અચલ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    માટે (અચલ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

શૂન્ય V3Graph::dumpDotFilePrefixed(અચલ string& nameComment, તાર્કિક colorAsSubgraph) અચલ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
શૂન્ય V3Graph::dumpDotFilePrefixedAlways(અચલ string& nameComment, તાર્કિક colorAsSubgraph) અચલ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

શૂન્ય V3Graph::dumpDotFile(અચલ string& filename, તાર્કિક colorAsSubgraph) અચલ {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    અચલ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    જો (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, અચલ V3GraphVertex*> rankSets;
    std::multimap<std::string, અચલ V3GraphVertex*> subgraphs;
    માટે (અચલ V3GraphVertex& vertex : vertices()) {
        અચલ string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        અચલ string& dotRank = vertex.dotRank();
        જો (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<અચલ V3GraphVertex*, પૂર્ણાંક> અંક_Map;

    // Print vertices
    પૂર્ણાંક n = 0;
    string subgr;
    માટે (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        અચલ string vertexSubgraph = it->first;
        અચલ V3GraphVertex* vertexp = it->second;
        અંક_Map[vertexp] = n;
        જો (subgr != vertexSubgraph) {
            જો (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            જો (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        જો (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        જો (vertexp->rank()) *logp << " r" << vertexp->rank();
        જો (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        જો (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        જો (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        જો (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    જો (subgr != "") *logp << "\t};\n";

    // Print edges
    માટે (અચલ V3GraphVertex& vertex : vertices()) {
        માટે (અચલ V3GraphEdge& edge : vertex.outEdges()) {
            જો (edge.weight()) {
                અચલ પૂર્ણાંક fromVnum = અંક_Map[edge.fromp()];
                અચલ પૂર્ણાંક toVnum = અંક_Map[edge.ટોચ()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.ટોચ()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                જો (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    માટે (અચલ auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        જો (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } નહીંતર {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        માટે (auto it{bounds.first}; it != bounds.second; ++it) {
            જો (it != bounds.first) *logp << ", ";
            *logp << 'n' << અંક_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

શૂન્ય V3Graph::hashGraphDebug(અચલ અક્ષર* debugName) અચલ {
    // Disabled when there are no nondeterminism issues in flight.
    જો (!v3Global.opt.debugNondeterminism()) પાછા;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<અચલ V3GraphVertex*, uint32_t> vx2Id;
    માટે (અચલ V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    માટે (અચલ V3GraphVertex& vtx : vertices()) {
        માટે (અચલ V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.ટોચ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.કિંમત()));
}
