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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"

#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ध्रुव V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(एतत्);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(एतत्);
}

शून्य V3GraphVertex::unlinkEdges(V3Graph*) {
    यावत् (V3GraphEdge* ध्रुव ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    यावत् (V3GraphEdge* ध्रुव ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

शून्य V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(एतत्);
    // Delete
    delete एतत्;  // this=nullptr;
}

शून्य V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    प्रति (ध्रुव V3GraphEdge& iedge : inEdges()) {
        प्रति (ध्रुव V3GraphEdge& oedge : outEdges()) {
            नूतन V3GraphEdge{graphp, iedge.fromp(), oedge.शीर्ष(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
शून्य V3GraphVertex::v3errorEnd(ध्रुव std::ostringstream& str) ध्रुव  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    यदि (debug()) nsstr << "\n-vertex: " << एतत् << '\n';
    यदि (File_पंक्ति* ध्रुव flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } अन्यथा {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
शून्य V3GraphVertex::v3errorEndFatal(ध्रुव std::ostringstream& str) ध्रुव
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& चालक<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    यदि (vertexp->rank()) os << " r" << vertexp->rank();
    यदि (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    यदि (vertexp->color()) os << " c" << vertexp->color();
    प्रत्यागम os;
}

//######################################################################
//######################################################################
// Edges

शून्य V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight,
                       तार्किक cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(शीर्ष, "Null to pointer");
    m_fromp = fromp;
    m_शीर्ष = शीर्ष;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(एतत्);
    m_शीर्ष->inEdges().linkBack(एतत्);
}

शून्य V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(एतत्);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(एतत्);
}

शून्य V3GraphEdge::relink_शीर्ष(V3GraphVertex* new_शीर्ष) {
    m_शीर्ष->inEdges().unlink(एतत्);
    m_शीर्ष = new_शीर्ष;
    m_शीर्ष->inEdges().linkBack(एतत्);
}

शून्य V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(एतत्);
    // Unlink to side
    m_शीर्ष->inEdges().unlink(एतत्);
    // Delete
    delete एतत्;
}

std::string V3GraphEdge::name() ध्रुव { प्रत्यागम m_fromp->name() + "->" + m_शीर्ष->name(); }

पूर्णाङ्क V3GraphEdge::sortCmp(ध्रुव V3GraphEdge* rhsp) ध्रुव {
    यदि (!m_weight || !rhsp->m_weight) प्रत्यागम 0;
    प्रत्यागम शीर्ष()->sortCmp(rhsp->शीर्ष());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

शून्य V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    प्रति (V3GraphVertex& vertex : vertices()) {
        यावत् (V3GraphEdge* ध्रुव edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    यावत् (V3GraphVertex* ध्रुव vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

शून्य V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    प्रति (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

शून्य V3Graph::userClearEdges() {
    // Clear user() in all of tree
    प्रति (V3GraphVertex& vertex : vertices()) {
        प्रति (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

शून्य V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    यदि (debug())
        प्रत्यागम "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    अन्यथा
        प्रत्यागम "";
}

शून्य V3Graph::dump(std::ostream& os) ध्रुव {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    प्रति (ध्रुव V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        यदि (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        प्रति (ध्रुव V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        प्रति (ध्रुव V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

शून्य V3Graph::dumpEdge(std::ostream& os, ध्रुव V3GraphVertex& vertex,
                       ध्रुव V3GraphEdge& edge) ध्रुव {
    यदि (edge.weight() && (edge.fromp() == &vertex || edge.शीर्ष() == &vertex)) {
        os << "\t\t";
        यदि (edge.fromp() == &vertex) os << "-> " << edge.शीर्ष()->name();
        यदि (edge.शीर्ष() == &vertex) os << "<- " << edge.fromp()->name();
        यदि (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
शून्य V3Graph::dumpEdges(std::ostream& os, ध्रुव V3GraphVertex& vertex) ध्रुव {
    प्रति (ध्रुव V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    प्रति (ध्रुव V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

शून्य V3Graph::dumpDotFilePrefixed(ध्रुव string& nameComment, तार्किक colorAsSubgraph) ध्रुव {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
शून्य V3Graph::dumpDotFilePrefixedAlways(ध्रुव string& nameComment, तार्किक colorAsSubgraph) ध्रुव {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

शून्य V3Graph::dumpDotFile(ध्रुव string& filename, तार्किक colorAsSubgraph) ध्रुव {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ध्रुव std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    यदि (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, ध्रुव V3GraphVertex*> rankSets;
    std::multimap<std::string, ध्रुव V3GraphVertex*> subgraphs;
    प्रति (ध्रुव V3GraphVertex& vertex : vertices()) {
        ध्रुव string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ध्रुव string& dotRank = vertex.dotRank();
        यदि (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ध्रुव V3GraphVertex*, पूर्णाङ्क> अंक_Map;

    // Print vertices
    पूर्णाङ्क n = 0;
    string subgr;
    प्रति (स्वतः it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ध्रुव string vertexSubgraph = it->first;
        ध्रुव V3GraphVertex* vertexp = it->second;
        अंक_Map[vertexp] = n;
        यदि (subgr != vertexSubgraph) {
            यदि (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            यदि (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        यदि (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        यदि (vertexp->rank()) *logp << " r" << vertexp->rank();
        यदि (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        यदि (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        यदि (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        यदि (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    यदि (subgr != "") *logp << "\t};\n";

    // Print edges
    प्रति (ध्रुव V3GraphVertex& vertex : vertices()) {
        प्रति (ध्रुव V3GraphEdge& edge : vertex.outEdges()) {
            यदि (edge.weight()) {
                ध्रुव पूर्णाङ्क fromVnum = अंक_Map[edge.fromp()];
                ध्रुव पूर्णाङ्क toVnum = अंक_Map[edge.शीर्ष()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.शीर्ष()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                यदि (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    प्रति (ध्रुव स्वतः& dotRank : ranks) {
        *logp << "\t{ rank=";
        यदि (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } अन्यथा {
            *logp << dotRank;
        }
        *logp << "; ";
        स्वतः bounds = rankSets.equal_range(dotRank);
        प्रति (स्वतः it{bounds.first}; it != bounds.second; ++it) {
            यदि (it != bounds.first) *logp << ", ";
            *logp << 'n' << अंक_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->बंद();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

शून्य V3Graph::hashGraphDebug(ध्रुव वर्ण* debugName) ध्रुव {
    // Disabled when there are no nondeterminism issues in flight.
    यदि (!v3Global.opt.debugNondeterminism()) प्रत्यागम;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ध्रुव V3GraphVertex*, uint32_t> vx2Id;
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    प्रति (ध्रुव V3GraphVertex& vtx : vertices()) {
        प्रति (ध्रुव V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.शीर्ष()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.मूल्य()));
}
