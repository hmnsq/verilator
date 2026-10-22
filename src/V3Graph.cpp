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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"

#ضمن "V3File.h"
#ضمن "V3Global.h"

#ضمن <map>
#ضمن <memory>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, ثابت V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(هذا);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(هذا);
}

فراغ V3GraphVertex::unlinkEdges(V3Graph*) {
    طالما (V3GraphEdge* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    طالما (V3GraphEdge* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

فراغ V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(هذا);
    // Delete
    delete هذا;  // this=nullptr;
}

فراغ V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    لكل (ثابت V3GraphEdge& iedge : inEdges()) {
        لكل (ثابت V3GraphEdge& oedge : outEdges()) {
            جديد V3GraphEdge{graphp, iedge.fromp(), oedge.اتس(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
فراغ V3GraphVertex::v3errorEnd(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (debug()) nsstr << "\n-vertex: " << هذا << '\n';
    إذا (File_باريس* ثابت flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } وإلا {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
فراغ V3GraphVertex::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    إذا (vertexp->rank()) os << " r" << vertexp->rank();
    إذا (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    إذا (vertexp->color()) os << " c" << vertexp->color();
    أرجع os;
}

//######################################################################
//######################################################################
// Edges

فراغ V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* اتس, صحيح weight,
                       منطقي cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(اتس, "Null to pointer");
    m_fromp = fromp;
    m_اتس = اتس;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(هذا);
    m_اتس->inEdges().linkBack(هذا);
}

فراغ V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(هذا);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(هذا);
}

فراغ V3GraphEdge::relink_اتس(V3GraphVertex* new_اتس) {
    m_اتس->inEdges().unlink(هذا);
    m_اتس = new_اتس;
    m_اتس->inEdges().linkBack(هذا);
}

فراغ V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(هذا);
    // Unlink to side
    m_اتس->inEdges().unlink(هذا);
    // Delete
    delete هذا;
}

std::string V3GraphEdge::name() ثابت { أرجع m_fromp->name() + "->" + m_اتس->name(); }

صحيح V3GraphEdge::sortCmp(ثابت V3GraphEdge* rhsp) ثابت {
    إذا (!m_weight || !rhsp->m_weight) أرجع 0;
    أرجع اتس()->sortCmp(rhsp->اتس());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

فراغ V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    لكل (V3GraphVertex& vertex : vertices()) {
        طالما (V3GraphEdge* ثابت edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    طالما (V3GraphVertex* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

فراغ V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    لكل (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

فراغ V3Graph::userClearEdges() {
    // Clear user() in all of tree
    لكل (V3GraphVertex& vertex : vertices()) {
        لكل (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

فراغ V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    إذا (debug())
        أرجع "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    وإلا
        أرجع "";
}

فراغ V3Graph::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    لكل (ثابت V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        إذا (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        لكل (ثابت V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        لكل (ثابت V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

فراغ V3Graph::dumpEdge(std::ostream& os, ثابت V3GraphVertex& vertex,
                       ثابت V3GraphEdge& edge) ثابت {
    إذا (edge.weight() && (edge.fromp() == &vertex || edge.اتس() == &vertex)) {
        os << "\t\t";
        إذا (edge.fromp() == &vertex) os << "-> " << edge.اتس()->name();
        إذا (edge.اتس() == &vertex) os << "<- " << edge.fromp()->name();
        إذا (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
فراغ V3Graph::dumpEdges(std::ostream& os, ثابت V3GraphVertex& vertex) ثابت {
    لكل (ثابت V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    لكل (ثابت V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

فراغ V3Graph::dumpDotFilePrefixed(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
فراغ V3Graph::dumpDotFilePrefixedAlways(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

فراغ V3Graph::dumpDotFile(ثابت string& filename, منطقي colorAsSubgraph) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    إذا (logp->fail()) v3fatal("Can't write file: " << filename);

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
    لكل (ثابت V3GraphVertex& vertex : vertices()) {
        ثابت string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        ثابت string& dotRank = vertex.dotRank();
        إذا (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3GraphVertex*, صحيح> اڠک_Map;

    // Print vertices
    صحيح n = 0;
    string subgr;
    لكل (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string vertexSubgraph = it->first;
        ثابت V3GraphVertex* vertexp = it->second;
        اڠک_Map[vertexp] = n;
        إذا (subgr != vertexSubgraph) {
            إذا (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            إذا (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        إذا (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        إذا (vertexp->rank()) *logp << " r" << vertexp->rank();
        إذا (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        إذا (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        إذا (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        إذا (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    إذا (subgr != "") *logp << "\t};\n";

    // Print edges
    لكل (ثابت V3GraphVertex& vertex : vertices()) {
        لكل (ثابت V3GraphEdge& edge : vertex.outEdges()) {
            إذا (edge.weight()) {
                ثابت صحيح fromVnum = اڠک_Map[edge.fromp()];
                ثابت صحيح toVnum = اڠک_Map[edge.اتس()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.اتس()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                إذا (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    لكل (ثابت auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        إذا (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } وإلا {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        لكل (auto it{bounds.first}; it != bounds.second; ++it) {
            إذا (it != bounds.first) *logp << ", ";
            *logp << 'n' << اڠک_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

فراغ V3Graph::hashGraphDebug(ثابت حرف* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    إذا (!v3Global.opt.debugNondeterminism()) أرجع;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3GraphVertex*, uint32_t> vx2Id;
    لكل (ثابت V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    لكل (ثابت V3GraphVertex& vtx : vertices()) {
        لكل (ثابت V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.اتس()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.نيلاي()));
}
