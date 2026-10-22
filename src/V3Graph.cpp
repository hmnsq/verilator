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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ধ্রুব V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(এতত্);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(এতত্);
}

শূন্য V3GraphVertex::unlinkEdges(V3Graph*) {
    যাবত্ (V3GraphEdge* ধ্রুব ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    যাবত্ (V3GraphEdge* ধ্রুব ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

শূন্য V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(এতত্);
    // Delete
    delete এতত্;  // this=nullptr;
}

শূন্য V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    প্রতি (ধ্রুব V3GraphEdge& iedge : inEdges()) {
        প্রতি (ধ্রুব V3GraphEdge& oedge : outEdges()) {
            নূতন V3GraphEdge{graphp, iedge.fromp(), oedge.শীর্ষ(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
শূন্য V3GraphVertex::v3errorEnd(ধ্রুব std::ostringstream& str) ধ্রুব  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    যদি (debug()) nsstr << "\n-vertex: " << এতত্ << '\n';
    যদি (File_সারি* ধ্রুব flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } অন্যথা {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
শূন্য V3GraphVertex::v3errorEndFatal(ধ্রুব std::ostringstream& str) ধ্রুব
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    প্রতিজ্ঞা(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    যদি (vertexp->rank()) os << " r" << vertexp->rank();
    যদি (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    যদি (vertexp->color()) os << " c" << vertexp->color();
    প্রত্যাগম os;
}

//######################################################################
//######################################################################
// Edges

শূন্য V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণাঙ্ক weight,
                       তার্কিক cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(শীর্ষ, "Null to pointer");
    m_fromp = fromp;
    m_শীর্ষ = শীর্ষ;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(এতত্);
    m_শীর্ষ->inEdges().linkBack(এতত্);
}

শূন্য V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(এতত্);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(এতত্);
}

শূন্য V3GraphEdge::relink_শীর্ষ(V3GraphVertex* new_শীর্ষ) {
    m_শীর্ষ->inEdges().unlink(এতত্);
    m_শীর্ষ = new_শীর্ষ;
    m_শীর্ষ->inEdges().linkBack(এতত্);
}

শূন্য V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(এতত্);
    // Unlink to side
    m_শীর্ষ->inEdges().unlink(এতত্);
    // Delete
    delete এতত্;
}

std::string V3GraphEdge::name() ধ্রুব { প্রত্যাগম m_fromp->name() + "->" + m_শীর্ষ->name(); }

পূর্ণাঙ্ক V3GraphEdge::sortCmp(ধ্রুব V3GraphEdge* rhsp) ধ্রুব {
    যদি (!m_weight || !rhsp->m_weight) প্রত্যাগম 0;
    প্রত্যাগম শীর্ষ()->sortCmp(rhsp->শীর্ষ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

শূন্য V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    প্রতি (V3GraphVertex& vertex : vertices()) {
        যাবত্ (V3GraphEdge* ধ্রুব edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    যাবত্ (V3GraphVertex* ধ্রুব vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

শূন্য V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    প্রতি (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

শূন্য V3Graph::userClearEdges() {
    // Clear user() in all of tree
    প্রতি (V3GraphVertex& vertex : vertices()) {
        প্রতি (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

শূন্য V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    যদি (debug())
        প্রত্যাগম "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    অন্যথা
        প্রত্যাগম "";
}

শূন্য V3Graph::dump(std::ostream& os) ধ্রুব {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    প্রতি (ধ্রুব V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        যদি (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        প্রতি (ধ্রুব V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        প্রতি (ধ্রুব V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

শূন্য V3Graph::dumpEdge(std::ostream& os, ধ্রুব V3GraphVertex& vertex,
                       ধ্রুব V3GraphEdge& edge) ধ্রুব {
    যদি (edge.weight() && (edge.fromp() == &vertex || edge.শীর্ষ() == &vertex)) {
        os << "\t\t";
        যদি (edge.fromp() == &vertex) os << "-> " << edge.শীর্ষ()->name();
        যদি (edge.শীর্ষ() == &vertex) os << "<- " << edge.fromp()->name();
        যদি (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
শূন্য V3Graph::dumpEdges(std::ostream& os, ধ্রুব V3GraphVertex& vertex) ধ্রুব {
    প্রতি (ধ্রুব V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    প্রতি (ধ্রুব V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

শূন্য V3Graph::dumpDotFilePrefixed(ধ্রুব string& nameComment, তার্কিক colorAsSubgraph) ধ্রুব {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
শূন্য V3Graph::dumpDotFilePrefixedAlways(ধ্রুব string& nameComment, তার্কিক colorAsSubgraph) ধ্রুব {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

শূন্য V3Graph::dumpDotFile(ধ্রুব string& filename, তার্কিক colorAsSubgraph) ধ্রুব {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ধ্রুব std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    যদি (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, ধ্রুব V3GraphVertex*> rankSets;
    std::multimap<std::string, ধ্রুব V3GraphVertex*> subgraphs;
    প্রতি (ধ্রুব V3GraphVertex& vertex : vertices()) {
        ধ্রুব string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ধ্রুব string& dotRank = vertex.dotRank();
        যদি (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ধ্রুব V3GraphVertex*, পূর্ণাঙ্ক> অঙ্ক_Map;

    // Print vertices
    পূর্ণাঙ্ক n = 0;
    string subgr;
    প্রতি (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ধ্রুব string vertexSubgraph = it->first;
        ধ্রুব V3GraphVertex* vertexp = it->second;
        অঙ্ক_Map[vertexp] = n;
        যদি (subgr != vertexSubgraph) {
            যদি (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            যদি (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        যদি (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        যদি (vertexp->rank()) *logp << " r" << vertexp->rank();
        যদি (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        যদি (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        যদি (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        যদি (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    যদি (subgr != "") *logp << "\t};\n";

    // Print edges
    প্রতি (ধ্রুব V3GraphVertex& vertex : vertices()) {
        প্রতি (ধ্রুব V3GraphEdge& edge : vertex.outEdges()) {
            যদি (edge.weight()) {
                ধ্রুব পূর্ণাঙ্ক fromVnum = অঙ্ক_Map[edge.fromp()];
                ধ্রুব পূর্ণাঙ্ক toVnum = অঙ্ক_Map[edge.শীর্ষ()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.শীর্ষ()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                যদি (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    প্রতি (ধ্রুব auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        যদি (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } অন্যথা {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        প্রতি (auto it{bounds.first}; it != bounds.second; ++it) {
            যদি (it != bounds.first) *logp << ", ";
            *logp << 'n' << অঙ্ক_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

শূন্য V3Graph::hashGraphDebug(ধ্রুব বর্ণ* debugName) ধ্রুব {
    // Disabled when there are no nondeterminism issues in flight.
    যদি (!v3Global.opt.debugNondeterminism()) প্রত্যাগম;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ধ্রুব V3GraphVertex*, uint32_t> vx2Id;
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        প্রতি (ধ্রুব V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.শীর্ষ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.মান()));
}
