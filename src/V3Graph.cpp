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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Graph.h"

#समावेश "V3File.h"
#समावेश "V3Global.h"

#समावेश <map>
#समावेश <memory>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, स्थिर V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(यह);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(यह);
}

व्योम V3GraphVertex::unlinkEdges(V3Graph*) {
    जबतक (V3GraphEdge* स्थिर ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    जबतक (V3GraphEdge* स्थिर ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

व्योम V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(यह);
    // Delete
    मिटाओ यह;  // this=nullptr;
}

व्योम V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    क्रम (स्थिर V3GraphEdge& iedge : inEdges()) {
        क्रम (स्थिर V3GraphEdge& oedge : outEdges()) {
            नया V3GraphEdge{graphp, iedge.fromp(), oedge.शीर्ष(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
व्योम V3GraphVertex::v3errorEnd(स्थिर std::ostringstream& str) स्थिर  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    अगर (debug()) nsstr << "\n-vertex: " << यह << '\n';
    अगर (File_पंक्ति* स्थिर flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } अन्यथा {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
व्योम V3GraphVertex::v3errorEndFatal(स्थिर std::ostringstream& str) स्थिर
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    निश्चित(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    अगर (vertexp->rank()) os << " r" << vertexp->rank();
    अगर (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    अगर (vertexp->color()) os << " c" << vertexp->color();
    वापस os;
}

//######################################################################
//######################################################################
// Edges

व्योम V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
                       bool cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(शीर्ष, "Null to pointer");
    m_fromp = fromp;
    m_शीर्ष = शीर्ष;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(यह);
    m_शीर्ष->inEdges().linkBack(यह);
}

व्योम V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(यह);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(यह);
}

व्योम V3GraphEdge::relink_शीर्ष(V3GraphVertex* new_शीर्ष) {
    m_शीर्ष->inEdges().unlink(यह);
    m_शीर्ष = new_शीर्ष;
    m_शीर्ष->inEdges().linkBack(यह);
}

व्योम V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(यह);
    // Unlink to side
    m_शीर्ष->inEdges().unlink(यह);
    // Delete
    मिटाओ यह;
}

std::string V3GraphEdge::name() स्थिर { वापस m_fromp->name() + "->" + m_शीर्ष->name(); }

पूर्णांक V3GraphEdge::sortCmp(स्थिर V3GraphEdge* rhsp) स्थिर {
    अगर (!m_weight || !rhsp->m_weight) वापस 0;
    वापस शीर्ष()->sortCmp(rhsp->शीर्ष());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { सफा(); }

व्योम V3Graph::सफा() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    क्रम (V3GraphVertex& vertex : vertices()) {
        जबतक (V3GraphEdge* स्थिर edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(मिटाओ edgep, edgep);
        }
    }
    // Delete the old vertices
    जबतक (V3GraphVertex* स्थिर vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(मिटाओ vertexp, vertexp);
    }
}

व्योम V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    क्रम (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

व्योम V3Graph::userClearEdges() {
    // Clear user() in all of tree
    क्रम (V3GraphVertex& vertex : vertices()) {
        क्रम (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

व्योम V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    अगर (debug())
        वापस "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    अन्यथा
        वापस "";
}

व्योम V3Graph::dump(std::ब_प्रवाह& os) स्थिर {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    क्रम (स्थिर V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        अगर (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        क्रम (स्थिर V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        क्रम (स्थिर V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

व्योम V3Graph::dumpEdge(std::ब_प्रवाह& os, स्थिर V3GraphVertex& vertex,
                       स्थिर V3GraphEdge& edge) स्थिर {
    अगर (edge.weight() && (edge.fromp() == &vertex || edge.शीर्ष() == &vertex)) {
        os << "\t\t";
        अगर (edge.fromp() == &vertex) os << "-> " << edge.शीर्ष()->name();
        अगर (edge.शीर्ष() == &vertex) os << "<- " << edge.fromp()->name();
        अगर (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
व्योम V3Graph::dumpEdges(std::ब_प्रवाह& os, स्थिर V3GraphVertex& vertex) स्थिर {
    क्रम (स्थिर V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    क्रम (स्थिर V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

व्योम V3Graph::dumpDotFilePrefixed(स्थिर string& nameComment, bool colorAsSubgraph) स्थिर {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
व्योम V3Graph::dumpDotFilePrefixedAlways(स्थिर string& nameComment, bool colorAsSubgraph) स्थिर {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

व्योम V3Graph::dumpDotFile(स्थिर string& filename, bool colorAsSubgraph) स्थिर {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    स्थिर std::unique_ptr<std::बख_प्रवाह> logp{V3File::new_ofstream(filename)};
    अगर (logp->हार()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, स्थिर V3GraphVertex*> rankSets;
    std::multimap<std::string, स्थिर V3GraphVertex*> subgraphs;
    क्रम (स्थिर V3GraphVertex& vertex : vertices()) {
        स्थिर string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        स्थिर string& dotRank = vertex.dotRank();
        अगर (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<स्थिर V3GraphVertex*, पूर्णांक> अंक_Map;

    // Print vertices
    पूर्णांक n = 0;
    string subgr;
    क्रम (स्वतः it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        स्थिर string vertexSubgraph = it->first;
        स्थिर V3GraphVertex* vertexp = it->second;
        अंक_Map[vertexp] = n;
        अगर (subgr != vertexSubgraph) {
            अगर (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            अगर (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        अगर (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        अगर (vertexp->rank()) *logp << " r" << vertexp->rank();
        अगर (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        अगर (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        अगर (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        अगर (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    अगर (subgr != "") *logp << "\t};\n";

    // Print edges
    क्रम (स्थिर V3GraphVertex& vertex : vertices()) {
        क्रम (स्थिर V3GraphEdge& edge : vertex.outEdges()) {
            अगर (edge.weight()) {
                स्थिर पूर्णांक fromVnum = अंक_Map[edge.fromp()];
                स्थिर पूर्णांक toVnum = अंक_Map[edge.शीर्ष()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.शीर्ष()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                अगर (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    क्रम (स्थिर स्वतः& dotRank : ranks) {
        *logp << "\t{ rank=";
        अगर (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } अन्यथा {
            *logp << dotRank;
        }
        *logp << "; ";
        स्वतः bounds = rankSets.equal_range(dotRank);
        क्रम (स्वतः it{bounds.first}; it != bounds.second; ++it) {
            अगर (it != bounds.first) *logp << ", ";
            *logp << 'n' << अंक_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->बंद();

    अ_बाहर << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

व्योम V3Graph::hashGraphDebug(स्थिर अक्षर* debugName) स्थिर {
    // Disabled when there are no nondeterminism issues in flight.
    अगर (!v3Global.opt.debugNondeterminism()) वापस;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<स्थिर V3GraphVertex*, uint32_t> vx2Id;
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        क्रम (स्थिर V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.शीर्ष()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.मूल्य()));
}
