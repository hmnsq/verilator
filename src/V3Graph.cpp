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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Graph.h"

#شامل "V3File.h"
#شامل "V3Global.h"

#شامل <map>
#شامل <memory>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ثابت V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(این);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(این);
}

تهی V3GraphVertex::unlinkEdges(V3Graph*) {
    وقتی (V3GraphEdge* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    وقتی (V3GraphEdge* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

تهی V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(این);
    // Delete
    delete این;  // this=nullptr;
}

تهی V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    برای (ثابت V3GraphEdge& iedge : inEdges()) {
        برای (ثابت V3GraphEdge& oedge : outEdges()) {
            جدید V3GraphEdge{graphp, iedge.fromp(), oedge.بالاترین(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
تهی V3GraphVertex::v3errorEnd(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    اگر (debug()) nsstr << "\n-vertex: " << این << '\n';
    اگر (File_سطر* ثابت flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } وگرنه {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
تهی V3GraphVertex::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    اگر (vertexp->rank()) os << " r" << vertexp->rank();
    اگر (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    اگر (vertexp->color()) os << " c" << vertexp->color();
    بازگشت os;
}

//######################################################################
//######################################################################
// Edges

تهی V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight,
                       منطقی cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(بالاترین, "Null to pointer");
    m_fromp = fromp;
    m_بالاترین = بالاترین;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(این);
    m_بالاترین->inEdges().linkBack(این);
}

تهی V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(این);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(این);
}

تهی V3GraphEdge::relink_بالاترین(V3GraphVertex* new_بالاترین) {
    m_بالاترین->inEdges().unlink(این);
    m_بالاترین = new_بالاترین;
    m_بالاترین->inEdges().linkBack(این);
}

تهی V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(این);
    // Unlink to side
    m_بالاترین->inEdges().unlink(این);
    // Delete
    delete این;
}

std::string V3GraphEdge::name() ثابت { بازگشت m_fromp->name() + "->" + m_بالاترین->name(); }

صحیح V3GraphEdge::sortCmp(ثابت V3GraphEdge* rhsp) ثابت {
    اگر (!m_weight || !rhsp->m_weight) بازگشت 0;
    بازگشت بالاترین()->sortCmp(rhsp->بالاترین());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

تهی V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    برای (V3GraphVertex& vertex : vertices()) {
        وقتی (V3GraphEdge* ثابت edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    وقتی (V3GraphVertex* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

تهی V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    برای (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

تهی V3Graph::userClearEdges() {
    // Clear user() in all of tree
    برای (V3GraphVertex& vertex : vertices()) {
        برای (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

تهی V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    اگر (debug())
        بازگشت "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    وگرنه
        بازگشت "";
}

تهی V3Graph::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    برای (ثابت V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        اگر (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        برای (ثابت V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        برای (ثابت V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

تهی V3Graph::dumpEdge(std::ostream& os, ثابت V3GraphVertex& vertex,
                       ثابت V3GraphEdge& edge) ثابت {
    اگر (edge.weight() && (edge.fromp() == &vertex || edge.بالاترین() == &vertex)) {
        os << "\t\t";
        اگر (edge.fromp() == &vertex) os << "-> " << edge.بالاترین()->name();
        اگر (edge.بالاترین() == &vertex) os << "<- " << edge.fromp()->name();
        اگر (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
تهی V3Graph::dumpEdges(std::ostream& os, ثابت V3GraphVertex& vertex) ثابت {
    برای (ثابت V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    برای (ثابت V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

تهی V3Graph::dumpDotFilePrefixed(ثابت string& nameComment, منطقی colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
تهی V3Graph::dumpDotFilePrefixedAlways(ثابت string& nameComment, منطقی colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

تهی V3Graph::dumpDotFile(ثابت string& filename, منطقی colorAsSubgraph) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    اگر (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, ثابت V3GraphVertex*> rankSets;
    std::multimap<std::string, ثابت V3GraphVertex*> subgraphs;
    برای (ثابت V3GraphVertex& vertex : vertices()) {
        ثابت string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ثابت string& dotRank = vertex.dotRank();
        اگر (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3GraphVertex*, صحیح> رقم_Map;

    // Print vertices
    صحیح n = 0;
    string subgr;
    برای (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string vertexSubgraph = it->first;
        ثابت V3GraphVertex* vertexp = it->second;
        رقم_Map[vertexp] = n;
        اگر (subgr != vertexSubgraph) {
            اگر (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            اگر (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        اگر (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        اگر (vertexp->rank()) *logp << " r" << vertexp->rank();
        اگر (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        اگر (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        اگر (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        اگر (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    اگر (subgr != "") *logp << "\t};\n";

    // Print edges
    برای (ثابت V3GraphVertex& vertex : vertices()) {
        برای (ثابت V3GraphEdge& edge : vertex.outEdges()) {
            اگر (edge.weight()) {
                ثابت صحیح fromVnum = رقم_Map[edge.fromp()];
                ثابت صحیح toVnum = رقم_Map[edge.بالاترین()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.بالاترین()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                اگر (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    برای (ثابت auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        اگر (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } وگرنه {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        برای (auto it{bounds.first}; it != bounds.second; ++it) {
            اگر (it != bounds.first) *logp << ", ";
            *logp << 'n' << رقم_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

تهی V3Graph::hashGraphDebug(ثابت نویسه* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    اگر (!v3Global.opt.debugNondeterminism()) بازگشت;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3GraphVertex*, uint32_t> vx2Id;
    برای (ثابت V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    برای (ثابت V3GraphVertex& vtx : vertices()) {
        برای (ثابت V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.بالاترین()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.مقدار()));
}
