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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, மாறிலி V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(இது);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(இது);
}

வெறுமை V3GraphVertex::unlinkEdges(V3Graph*) {
    போது (V3GraphEdge* மாறிலி ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    போது (V3GraphEdge* மாறிலி ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

வெறுமை V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(இது);
    // Delete
    delete இது;  // this=nullptr;
}

வெறுமை V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    க்கு (மாறிலி V3GraphEdge& iedge : inEdges()) {
        க்கு (மாறிலி V3GraphEdge& oedge : outEdges()) {
            புதிய V3GraphEdge{graphp, iedge.fromp(), oedge.உச்சி(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
வெறுமை V3GraphVertex::v3errorEnd(மாறிலி std::ostringstream& str) மாறிலி  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    எனில் (debug()) nsstr << "\n-vertex: " << இது << '\n';
    எனில் (File_வரி* மாறிலி flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } இல்லையெனில் {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
வெறுமை V3GraphVertex::v3errorEndFatal(மாறிலி std::ostringstream& str) மாறிலி
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ப்ரதிஜ்ஞா(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    எனில் (vertexp->rank()) os << " r" << vertexp->rank();
    எனில் (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    எனில் (vertexp->color()) os << " c" << vertexp->color();
    திரும்பு os;
}

//######################################################################
//######################################################################
// Edges

வெறுமை V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight,
                       தருக்க cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(உச்சி, "Null to pointer");
    m_fromp = fromp;
    m_உச்சி = உச்சி;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(இது);
    m_உச்சி->inEdges().linkBack(இது);
}

வெறுமை V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(இது);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(இது);
}

வெறுமை V3GraphEdge::relink_உச்சி(V3GraphVertex* new_உச்சி) {
    m_உச்சி->inEdges().unlink(இது);
    m_உச்சி = new_உச்சி;
    m_உச்சி->inEdges().linkBack(இது);
}

வெறுமை V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(இது);
    // Unlink to side
    m_உச்சி->inEdges().unlink(இது);
    // Delete
    delete இது;
}

std::string V3GraphEdge::name() மாறிலி { திரும்பு m_fromp->name() + "->" + m_உச்சி->name(); }

முழு V3GraphEdge::sortCmp(மாறிலி V3GraphEdge* rhsp) மாறிலி {
    எனில் (!m_weight || !rhsp->m_weight) திரும்பு 0;
    திரும்பு உச்சி()->sortCmp(rhsp->உச்சி());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

வெறுமை V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    க்கு (V3GraphVertex& vertex : vertices()) {
        போது (V3GraphEdge* மாறிலி edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    போது (V3GraphVertex* மாறிலி vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

வெறுமை V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    க்கு (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

வெறுமை V3Graph::userClearEdges() {
    // Clear user() in all of tree
    க்கு (V3GraphVertex& vertex : vertices()) {
        க்கு (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

வெறுமை V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    எனில் (debug())
        திரும்பு "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    இல்லையெனில்
        திரும்பு "";
}

வெறுமை V3Graph::dump(std::ostream& os) மாறிலி {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    க்கு (மாறிலி V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        எனில் (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        க்கு (மாறிலி V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        க்கு (மாறிலி V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

வெறுமை V3Graph::dumpEdge(std::ostream& os, மாறிலி V3GraphVertex& vertex,
                       மாறிலி V3GraphEdge& edge) மாறிலி {
    எனில் (edge.weight() && (edge.fromp() == &vertex || edge.உச்சி() == &vertex)) {
        os << "\t\t";
        எனில் (edge.fromp() == &vertex) os << "-> " << edge.உச்சி()->name();
        எனில் (edge.உச்சி() == &vertex) os << "<- " << edge.fromp()->name();
        எனில் (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
வெறுமை V3Graph::dumpEdges(std::ostream& os, மாறிலி V3GraphVertex& vertex) மாறிலி {
    க்கு (மாறிலி V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    க்கு (மாறிலி V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

வெறுமை V3Graph::dumpDotFilePrefixed(மாறிலி string& nameComment, தருக்க colorAsSubgraph) மாறிலி {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
வெறுமை V3Graph::dumpDotFilePrefixedAlways(மாறிலி string& nameComment, தருக்க colorAsSubgraph) மாறிலி {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

வெறுமை V3Graph::dumpDotFile(மாறிலி string& filename, தருக்க colorAsSubgraph) மாறிலி {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    மாறிலி std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    எனில் (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, மாறிலி V3GraphVertex*> rankSets;
    std::multimap<std::string, மாறிலி V3GraphVertex*> subgraphs;
    க்கு (மாறிலி V3GraphVertex& vertex : vertices()) {
        மாறிலி string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        மாறிலி string& dotRank = vertex.dotRank();
        எனில் (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<மாறிலி V3GraphVertex*, முழு> இலக்கம்_Map;

    // Print vertices
    முழு n = 0;
    string subgr;
    க்கு (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        மாறிலி string vertexSubgraph = it->first;
        மாறிலி V3GraphVertex* vertexp = it->second;
        இலக்கம்_Map[vertexp] = n;
        எனில் (subgr != vertexSubgraph) {
            எனில் (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            எனில் (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        எனில் (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        எனில் (vertexp->rank()) *logp << " r" << vertexp->rank();
        எனில் (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        எனில் (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        எனில் (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        எனில் (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    எனில் (subgr != "") *logp << "\t};\n";

    // Print edges
    க்கு (மாறிலி V3GraphVertex& vertex : vertices()) {
        க்கு (மாறிலி V3GraphEdge& edge : vertex.outEdges()) {
            எனில் (edge.weight()) {
                மாறிலி முழு fromVnum = இலக்கம்_Map[edge.fromp()];
                மாறிலி முழு toVnum = இலக்கம்_Map[edge.உச்சி()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.உச்சி()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                எனில் (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    க்கு (மாறிலி auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        எனில் (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } இல்லையெனில் {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        க்கு (auto it{bounds.first}; it != bounds.second; ++it) {
            எனில் (it != bounds.first) *logp << ", ";
            *logp << 'n' << இலக்கம்_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

வெறுமை V3Graph::hashGraphDebug(மாறிலி எழுத்து* debugName) மாறிலி {
    // Disabled when there are no nondeterminism issues in flight.
    எனில் (!v3Global.opt.debugNondeterminism()) திரும்பு;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<மாறிலி V3GraphVertex*, uint32_t> vx2Id;
    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    க்கு (மாறிலி V3GraphVertex& vtx : vertices()) {
        க்கு (மாறிலி V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.உச்சி()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.மதிப்பு()));
}
