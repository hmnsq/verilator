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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ଅଚଳ V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ଏହି);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ଏହି);
}

ଶୂନ୍ୟ V3GraphVertex::unlinkEdges(V3Graph*) {
    ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

ଶୂନ୍ୟ V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ଏହି);
    // Delete
    delete ଏହି;  // this=nullptr;
}

ଶୂନ୍ୟ V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    ପାଇଁ (ଅଚଳ V3GraphEdge& iedge : inEdges()) {
        ପାଇଁ (ଅଚଳ V3GraphEdge& oedge : outEdges()) {
            ନୂତନ V3GraphEdge{graphp, iedge.fromp(), oedge.ଶୀର୍ଷ(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
ଶୂନ୍ୟ V3GraphVertex::v3errorEnd(ଅଚଳ std::ostringstream& str) ଅଚଳ  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ଯଦି (debug()) nsstr << "\n-vertex: " << ଏହି << '\n';
    ଯଦି (File_ଧାଡ଼ି* ଅଚଳ flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } ଅନ୍ୟଥା {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
ଶୂନ୍ୟ V3GraphVertex::v3errorEndFatal(ଅଚଳ std::ostringstream& str) ଅଚଳ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ପ୍ରତିଜ୍ଞା(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    ଯଦି (vertexp->rank()) os << " r" << vertexp->rank();
    ଯଦି (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    ଯଦି (vertexp->color()) os << " c" << vertexp->color();
    ଫେରନ୍ତୁ os;
}

//######################################################################
//######################################################################
// Edges

ଶୂନ୍ୟ V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight,
                       ତାର୍କିକ cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(ଶୀର୍ଷ, "Null to pointer");
    m_fromp = fromp;
    m_ଶୀର୍ଷ = ଶୀର୍ଷ;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(ଏହି);
    m_ଶୀର୍ଷ->inEdges().linkBack(ଏହି);
}

ଶୂନ୍ୟ V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(ଏହି);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(ଏହି);
}

ଶୂନ୍ୟ V3GraphEdge::relink_ଶୀର୍ଷ(V3GraphVertex* new_ଶୀର୍ଷ) {
    m_ଶୀର୍ଷ->inEdges().unlink(ଏହି);
    m_ଶୀର୍ଷ = new_ଶୀର୍ଷ;
    m_ଶୀର୍ଷ->inEdges().linkBack(ଏହି);
}

ଶୂନ୍ୟ V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(ଏହି);
    // Unlink to side
    m_ଶୀର୍ଷ->inEdges().unlink(ଏହି);
    // Delete
    delete ଏହି;
}

std::string V3GraphEdge::name() ଅଚଳ { ଫେରନ୍ତୁ m_fromp->name() + "->" + m_ଶୀର୍ଷ->name(); }

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3GraphEdge::sortCmp(ଅଚଳ V3GraphEdge* rhsp) ଅଚଳ {
    ଯଦି (!m_weight || !rhsp->m_weight) ଫେରନ୍ତୁ 0;
    ଫେରନ୍ତୁ ଶୀର୍ଷ()->sortCmp(rhsp->ଶୀର୍ଷ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

ଶୂନ୍ୟ V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    ପାଇଁ (V3GraphVertex& vertex : vertices()) {
        ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    ଯେତେବେଳେ (V3GraphVertex* ଅଚଳ vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

ଶୂନ୍ୟ V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    ପାଇଁ (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

ଶୂନ୍ୟ V3Graph::userClearEdges() {
    // Clear user() in all of tree
    ପାଇଁ (V3GraphVertex& vertex : vertices()) {
        ପାଇଁ (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

ଶୂନ୍ୟ V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    ଯଦି (debug())
        ଫେରନ୍ତୁ "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    ଅନ୍ୟଥା
        ଫେରନ୍ତୁ "";
}

ଶୂନ୍ୟ V3Graph::dump(std::ostream& os) ଅଚଳ {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    ପାଇଁ (ଅଚଳ V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        ଯଦି (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

ଶୂନ୍ୟ V3Graph::dumpEdge(std::ostream& os, ଅଚଳ V3GraphVertex& vertex,
                       ଅଚଳ V3GraphEdge& edge) ଅଚଳ {
    ଯଦି (edge.weight() && (edge.fromp() == &vertex || edge.ଶୀର୍ଷ() == &vertex)) {
        os << "\t\t";
        ଯଦି (edge.fromp() == &vertex) os << "-> " << edge.ଶୀର୍ଷ()->name();
        ଯଦି (edge.ଶୀର୍ଷ() == &vertex) os << "<- " << edge.fromp()->name();
        ଯଦି (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
ଶୂନ୍ୟ V3Graph::dumpEdges(std::ostream& os, ଅଚଳ V3GraphVertex& vertex) ଅଚଳ {
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

ଶୂନ୍ୟ V3Graph::dumpDotFilePrefixed(ଅଚଳ string& nameComment, ତାର୍କିକ colorAsSubgraph) ଅଚଳ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
ଶୂନ୍ୟ V3Graph::dumpDotFilePrefixedAlways(ଅଚଳ string& nameComment, ତାର୍କିକ colorAsSubgraph) ଅଚଳ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

ଶୂନ୍ୟ V3Graph::dumpDotFile(ଅଚଳ string& filename, ତାର୍କିକ colorAsSubgraph) ଅଚଳ {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ଅଚଳ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ଯଦି (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, ଅଚଳ V3GraphVertex*> rankSets;
    std::multimap<std::string, ଅଚଳ V3GraphVertex*> subgraphs;
    ପାଇଁ (ଅଚଳ V3GraphVertex& vertex : vertices()) {
        ଅଚଳ string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ଅଚଳ string& dotRank = vertex.dotRank();
        ଯଦି (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ଅଚଳ V3GraphVertex*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> ଅଙ୍କ_Map;

    // Print vertices
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0;
    string subgr;
    ପାଇଁ (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ଅଚଳ string vertexSubgraph = it->first;
        ଅଚଳ V3GraphVertex* vertexp = it->second;
        ଅଙ୍କ_Map[vertexp] = n;
        ଯଦି (subgr != vertexSubgraph) {
            ଯଦି (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            ଯଦି (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        ଯଦି (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        ଯଦି (vertexp->rank()) *logp << " r" << vertexp->rank();
        ଯଦି (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        ଯଦି (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        ଯଦି (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        ଯଦି (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    ଯଦି (subgr != "") *logp << "\t};\n";

    // Print edges
    ପାଇଁ (ଅଚଳ V3GraphVertex& vertex : vertices()) {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vertex.outEdges()) {
            ଯଦି (edge.weight()) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fromVnum = ଅଙ୍କ_Map[edge.fromp()];
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା toVnum = ଅଙ୍କ_Map[edge.ଶୀର୍ଷ()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.ଶୀର୍ଷ()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                ଯଦି (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    ପାଇଁ (ଅଚଳ auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        ଯଦି (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } ଅନ୍ୟଥା {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        ପାଇଁ (auto it{bounds.first}; it != bounds.second; ++it) {
            ଯଦି (it != bounds.first) *logp << ", ";
            *logp << 'n' << ଅଙ୍କ_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

ଶୂନ୍ୟ V3Graph::hashGraphDebug(ଅଚଳ ଅକ୍ଷର* debugName) ଅଚଳ {
    // Disabled when there are no nondeterminism issues in flight.
    ଯଦି (!v3Global.opt.debugNondeterminism()) ଫେରନ୍ତୁ;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ଅଚଳ V3GraphVertex*, uint32_t> vx2Id;
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.ଶୀର୍ଷ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.ମୂଲ୍ୟ()));
}
