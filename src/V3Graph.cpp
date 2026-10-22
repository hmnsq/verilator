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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3GraphVertex::V3GraphVertex(V3Graph* graphp, സ്ഥിരം V3GraphVertex& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ഇത്);
}

V3GraphVertex::V3GraphVertex(V3Graph* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = nullptr;
    graphp->vertices().linkBack(ഇത്);
}

ശൂന്യം V3GraphVertex::unlinkEdges(V3Graph*) {
    ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

ശൂന്യം V3GraphVertex::unlinkDelete(V3Graph* graphp) {
    // Delete edges
    unlinkEdges(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ഇത്);
    // Delete
    delete ഇത്;  // this=nullptr;
}

ശൂന്യം V3GraphVertex::rerouteEdges(V3Graph* graphp) {
    // Make new edges for each from/to pair
    വേണ്ടി (സ്ഥിരം V3GraphEdge& iedge : inEdges()) {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& oedge : outEdges()) {
            പുതിയ V3GraphEdge{graphp, iedge.fromp(), oedge.മുകൾ(),
                            std::min(iedge.weight(), oedge.weight()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlinkEdges(graphp);
}

// cppcheck-has-bug-suppress constParameter
ശൂന്യം V3GraphVertex::v3errorEnd(സ്ഥിരം std::ostringstream& str) സ്ഥിരം  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    എങ്കിൽ (debug()) nsstr << "\n-vertex: " << ഇത് << '\n';
    എങ്കിൽ (File_വരി* സ്ഥിരം flp = fileline()) {
        flp->v3errorEnd(nsstr);
    } അല്ലെങ്കിൽ {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}
ശൂന്യം V3GraphVertex::v3errorEndFatal(സ്ഥിരം std::ostringstream& str) സ്ഥിരം
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    പ്രതിജ്ഞാ(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    എങ്കിൽ (vertexp->rank()) os << " r" << vertexp->rank();
    എങ്കിൽ (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    എങ്കിൽ (vertexp->color()) os << " c" << vertexp->color();
    തിരികെ os;
}

//######################################################################
//######################################################################
// Edges

ശൂന്യം V3GraphEdge::init(V3Graph* /*graphp*/, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight,
                       തർക്കം cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(മുകൾ, "Null to pointer");
    m_fromp = fromp;
    m_മുകൾ = മുകൾ;
    m_weight = weight;
    m_cutable = cutable;
    m_userp = nullptr;
    // Link vertices to this edge
    m_fromp->outEdges().linkBack(ഇത്);
    m_മുകൾ->inEdges().linkBack(ഇത്);
}

ശൂന്യം V3GraphEdge::relinkFromp(V3GraphVertex* newFromp) {
    m_fromp->outEdges().unlink(ഇത്);
    m_fromp = newFromp;
    m_fromp->outEdges().linkBack(ഇത്);
}

ശൂന്യം V3GraphEdge::relink_മുകൾ(V3GraphVertex* new_മുകൾ) {
    m_മുകൾ->inEdges().unlink(ഇത്);
    m_മുകൾ = new_മുകൾ;
    m_മുകൾ->inEdges().linkBack(ഇത്);
}

ശൂന്യം V3GraphEdge::unlinkDelete() {
    // Unlink from side
    m_fromp->outEdges().unlink(ഇത്);
    // Unlink to side
    m_മുകൾ->inEdges().unlink(ഇത്);
    // Delete
    delete ഇത്;
}

std::string V3GraphEdge::name() സ്ഥിരം { തിരികെ m_fromp->name() + "->" + m_മുകൾ->name(); }

പൂർണ്ണസംഖ്യ V3GraphEdge::sortCmp(സ്ഥിരം V3GraphEdge* rhsp) സ്ഥിരം {
    എങ്കിൽ (!m_weight || !rhsp->m_weight) തിരികെ 0;
    തിരികെ മുകൾ()->sortCmp(rhsp->മുകൾ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3Graph::V3Graph() {}

V3Graph::~V3Graph() { clear(); }

ശൂന്യം V3Graph::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    വേണ്ടി (V3GraphVertex& vertex : vertices()) {
        ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം edgep = vertex.outEdges().unlinkFront()) {
            VL_DO_DANGLING(delete edgep, edgep);
        }
    }
    // Delete the old vertices
    ആകുമ്പോൾ (V3GraphVertex* സ്ഥിരം vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(delete vertexp, vertexp);
    }
}

ശൂന്യം V3Graph::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    വേണ്ടി (V3GraphVertex& vertex : vertices()) {
        vertex.user(0);
        vertex.userp(nullptr);  // It's a union, but might be different size than user()
    }
}

ശൂന്യം V3Graph::userClearEdges() {
    // Clear user() in all of tree
    വേണ്ടി (V3GraphVertex& vertex : vertices()) {
        വേണ്ടി (V3GraphEdge& edge : vertex.outEdges()) {
            edge.user(0);
            edge.userp(nullptr);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

ശൂന്യം V3Graph::loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(edgeFuncp, vertexp));
}
string V3Graph::loopsVertexCb(V3GraphVertex* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    എങ്കിൽ (debug())
        തിരികെ "-Info-Loop: "s + cvtToHex(vertexp) + ' ' + cvtToStr(vertexp) + '\n';
    അല്ലെങ്കിൽ
        തിരികെ "";
}

ശൂന്യം V3Graph::dump(std::ostream& os) സ്ഥിരം {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vertex : vertices()) {
        os << "\tNode: " << vertex.name();
        എങ്കിൽ (vertex.color()) os << "  color=" << vertex.color();
        os << '\n';
        // Print edges
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
    }
}

ശൂന്യം V3Graph::dumpEdge(std::ostream& os, സ്ഥിരം V3GraphVertex& vertex,
                       സ്ഥിരം V3GraphEdge& edge) സ്ഥിരം {
    എങ്കിൽ (edge.weight() && (edge.fromp() == &vertex || edge.മുകൾ() == &vertex)) {
        os << "\t\t";
        എങ്കിൽ (edge.fromp() == &vertex) os << "-> " << edge.മുകൾ()->name();
        എങ്കിൽ (edge.മുകൾ() == &vertex) os << "<- " << edge.fromp()->name();
        എങ്കിൽ (edge.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
ശൂന്യം V3Graph::dumpEdges(std::ostream& os, സ്ഥിരം V3GraphVertex& vertex) സ്ഥിരം {
    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertex.inEdges()) dumpEdge(os, vertex, edge);
    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertex.outEdges()) dumpEdge(os, vertex, edge);
}

ശൂന്യം V3Graph::dumpDotFilePrefixed(സ്ഥിരം string& nameComment, തർക്കം colorAsSubgraph) സ്ഥിരം {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
ശൂന്യം V3Graph::dumpDotFilePrefixedAlways(സ്ഥിരം string& nameComment, തർക്കം colorAsSubgraph) സ്ഥിരം {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

ശൂന്യം V3Graph::dumpDotFile(സ്ഥിരം string& filename, തർക്കം colorAsSubgraph) സ്ഥിരം {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    സ്ഥിരം std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    എങ്കിൽ (logp->fail()) v3fatal("Can't write file: " << filename);

    // Header
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_set<std::string> ranks;
    std::unordered_multimap<std::string, സ്ഥിരം V3GraphVertex*> rankSets;
    std::multimap<std::string, സ്ഥിരം V3GraphVertex*> subgraphs;
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vertex : vertices()) {
        സ്ഥിരം string vertexSubgraph
            = (colorAsSubgraph && vertex.color()) ? cvtToStr(vertex.color()) : "";
        subgraphs.emplace(vertexSubgraph, &vertex);
        സ്ഥിരം string& dotRank = vertex.dotRank();
        എങ്കിൽ (!dotRank.empty()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &vertex);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<സ്ഥിരം V3GraphVertex*, പൂർണ്ണസംഖ്യ> അക്കം_Map;

    // Print vertices
    പൂർണ്ണസംഖ്യ n = 0;
    string subgr;
    വേണ്ടി (auto it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        സ്ഥിരം string vertexSubgraph = it->first;
        സ്ഥിരം V3GraphVertex* vertexp = it->second;
        അക്കം_Map[vertexp] = n;
        എങ്കിൽ (subgr != vertexSubgraph) {
            എങ്കിൽ (subgr != "") *logp << "\t};\n";
            subgr = vertexSubgraph;
            എങ്കിൽ (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        എങ്കിൽ (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        എങ്കിൽ (vertexp->rank()) *logp << " r" << vertexp->rank();
        എങ്കിൽ (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        എങ്കിൽ (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        എങ്കിൽ (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        എങ്കിൽ (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    എങ്കിൽ (subgr != "") *logp << "\t};\n";

    // Print edges
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vertex : vertices()) {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vertex.outEdges()) {
            എങ്കിൽ (edge.weight()) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ fromVnum = അക്കം_Map[edge.fromp()];
                സ്ഥിരം പൂർണ്ണസംഖ്യ toVnum = അക്കം_Map[edge.മുകൾ()];
                *logp << "\tn" << edge.fromp()->dotName() << fromVnum << " -> n"
                      << edge.മുകൾ()->dotName() << toVnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (edge.dotLabel() != "" ? edge.dotLabel() : "")
                      << "\"" << " weight=" << edge.weight() << " color=" << edge.dotColor();
                എങ്കിൽ (edge.dotStyle() != "") *logp << " style=" << edge.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    വേണ്ടി (സ്ഥിരം auto& dotRank : ranks) {
        *logp << "\t{ rank=";
        എങ്കിൽ (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } അല്ലെങ്കിൽ {
            *logp << dotRank;
        }
        *logp << "; ";
        auto bounds = rankSets.equal_range(dotRank);
        വേണ്ടി (auto it{bounds.first}; it != bounds.second; ++it) {
            എങ്കിൽ (it != bounds.first) *logp << ", ";
            *logp << 'n' << അക്കം_Map[it->second] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    cout << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

ശൂന്യം V3Graph::hashGraphDebug(സ്ഥിരം അക്ഷരം* debugName) സ്ഥിരം {
    // Disabled when there are no nondeterminism issues in flight.
    എങ്കിൽ (!v3Global.opt.debugNondeterminism()) തിരികെ;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<സ്ഥിരം V3GraphVertex*, uint32_t> vx2Id;
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vtx.outEdges()) hash += vx2Id[edge.മുകൾ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvtToHex(hash.മൂല്യം()));
}
