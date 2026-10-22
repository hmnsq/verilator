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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"

#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3_లేఖాచిత్రం_శీర్షం::V3_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = శూన్యసూచిక;
    graphp->vertices().link_వెనుక(ఇది);
}

V3_లేఖాచిత్రం_శీర్షం::V3_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = శూన్యసూచిక;
    graphp->vertices().link_వెనుక(ఇది);
}

శూన్య V3_లేఖాచిత్రం_శీర్షం::unlink_అంచులు(V3_లేఖాచిత్రం*) {
    అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

శూన్య V3_లేఖాచిత్రం_శీర్షం::unlinkDelete(V3_లేఖాచిత్రం* graphp) {
    // Delete edges
    unlink_అంచులు(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ఇది);
    // Delete
    తొలగించు ఇది;  // this=nullptr;
}

శూన్య V3_లేఖాచిత్రం_శీర్షం::reroute_అంచులు(V3_లేఖాచిత్రం* graphp) {
    // Make new edges for each from/to pair
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& iedge : లోపల_అంచులు()) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& oedge : బహిర్గతం_అంచులు()) {
            కొత్త V3_లేఖాచిత్రం_అంచు{graphp, iedge.fromp(), oedge.శిఖరం(),
                            std::min(iedge.బరువు_2(), oedge.బరువు_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_అంచులు(graphp);
}

// cppcheck-has-bug-suppress constParameter
శూన్య V3_లేఖాచిత్రం_శీర్షం::v3error_చివర(స్థిరరాశి std::ostringstream& str) స్థిరరాశి  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ఒకవేళ (debug()) nsstr << "\n-vertex: " << ఇది << '\n';
    ఒకవేళ (File_పంక్తి* స్థిరరాశి flp = fileline()) {
        flp->v3error_చివర(nsstr);
    } లేకపోతే {
        V3Error::v3error_చివర(nsstr, "", శూన్యసూచిక);
    }
}
శూన్య V3_లేఖాచిత్రం_శీర్షం::v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& str) స్థిరరాశి
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_చివర(str);
    ప్రతిజ్ఞా(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& క్రియాకారకం<<(std::ostream& os, V3_లేఖాచిత్రం_శీర్షం* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    ఒకవేళ (vertexp->rank()) os << " r" << vertexp->rank();
    ఒకవేళ (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    ఒకవేళ (vertexp->color()) os << " c" << vertexp->color();
    తిరిగి os;
}

//######################################################################
//######################################################################
//అంచులు

శూన్య V3_లేఖాచిత్రం_అంచు::init(V3_లేఖాచిత్రం* /*graphp*/, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2,
                       తార్కిక cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(శిఖరం, "Null to pointer");
    m_fromp = fromp;
    m_శిఖరం = శిఖరం;
    m_బరువు = బరువు_2;
    m_cutable = cutable;
    m_userp = శూన్యసూచిక;
    // Link vertices to this edge
    m_fromp->బహిర్గతం_అంచులు().link_వెనుక(ఇది);
    m_శిఖరం->లోపల_అంచులు().link_వెనుక(ఇది);
}

శూన్య V3_లేఖాచిత్రం_అంచు::relinkFromp(V3_లేఖాచిత్రం_శీర్షం* newFromp) {
    m_fromp->బహిర్గతం_అంచులు().unlink(ఇది);
    m_fromp = newFromp;
    m_fromp->బహిర్గతం_అంచులు().link_వెనుక(ఇది);
}

శూన్య V3_లేఖాచిత్రం_అంచు::relink_శిఖరం(V3_లేఖాచిత్రం_శీర్షం* new_శిఖరం) {
    m_శిఖరం->లోపల_అంచులు().unlink(ఇది);
    m_శిఖరం = new_శిఖరం;
    m_శిఖరం->లోపల_అంచులు().link_వెనుక(ఇది);
}

శూన్య V3_లేఖాచిత్రం_అంచు::unlinkDelete() {
    // Unlink from side
    m_fromp->బహిర్గతం_అంచులు().unlink(ఇది);
    // Unlink to side
    m_శిఖరం->లోపల_అంచులు().unlink(ఇది);
    // Delete
    తొలగించు ఇది;
}

std::string V3_లేఖాచిత్రం_అంచు::name() స్థిరరాశి { తిరిగి m_fromp->name() + "->" + m_శిఖరం->name(); }

పూర్ణాంకం V3_లేఖాచిత్రం_అంచు::క్రమీకరించు_Cmp(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* rhsp) స్థిరరాశి {
    ఒకవేళ (!m_బరువు || !rhsp->m_బరువు) తిరిగి 0;
    తిరిగి శిఖరం()->క్రమీకరించు_Cmp(rhsp->శిఖరం());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_లేఖాచిత్రం::V3_లేఖాచిత్రం() {}

V3_లేఖాచిత్రం::~V3_లేఖాచిత్రం() { clear(); }

శూన్య V3_లేఖాచిత్రం::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep = శీర్షం.బహిర్గతం_అంచులు().unlinkFront()) {
            VL_DO_DANGLING(తొలగించు edgep, edgep);
        }
    }
    // Delete the old vertices
    అయితే (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(తొలగించు vertexp, vertexp);
    }
}

శూన్య V3_లేఖాచిత్రం::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        శీర్షం.user(0);
        శీర్షం.userp(శూన్యసూచిక);  // It's a union, but might be different size than user()
    }
}

శూన్య V3_లేఖాచిత్రం::user_Clear_అంచులు() {
    // Clear user() in all of tree
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) {
            అంచు.user(0);
            అంచు.userp(శూన్యసూచిక);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

శూన్య V3_లేఖాచిత్రం::loops_సందేశం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp, V3_అంచు_Func_P అంచు_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(అంచు_Funcp, vertexp));
}
string V3_లేఖాచిత్రం::loops_శీర్షం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    ఒకవేళ (debug())
        తిరిగి "-Info-Loop: "s + cvt_గమ్యం_Hex(vertexp) + ' ' + cvt_గమ్యం_Str(vertexp) + '\n';
    లేకపోతే
        తిరిగి "";
}

శూన్య V3_లేఖాచిత్రం::dump(std::ostream& os) స్థిరరాశి {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        os << "\tNode: " << శీర్షం.name();
        ఒకవేళ (శీర్షం.color()) os << "  color=" << శీర్షం.color();
        os << '\n';
        // Print edges
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.లోపల_అంచులు()) dump_అంచు(os, శీర్షం, అంచు);
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) dump_అంచు(os, శీర్షం, అంచు);
    }
}

శూన్య V3_లేఖాచిత్రం::dump_అంచు(std::ostream& os, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం,
                       స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు) స్థిరరాశి {
    ఒకవేళ (అంచు.బరువు_2() && (అంచు.fromp() == &శీర్షం || అంచు.శిఖరం() == &శీర్షం)) {
        os << "\t\t";
        ఒకవేళ (అంచు.fromp() == &శీర్షం) os << "-> " << అంచు.శిఖరం()->name();
        ఒకవేళ (అంచు.శిఖరం() == &శీర్షం) os << "<- " << అంచు.fromp()->name();
        ఒకవేళ (అంచు.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
శూన్య V3_లేఖాచిత్రం::dump_అంచులు(std::ostream& os, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం) స్థిరరాశి {
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.లోపల_అంచులు()) dump_అంచు(os, శీర్షం, అంచు);
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) dump_అంచు(os, శీర్షం, అంచు);
}

శూన్య V3_లేఖాచిత్రం::dumpDotFilePrefixed(స్థిరరాశి string& nameComment, తార్కిక colorAsSubgraph) స్థిరరాశి {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
శూన్య V3_లేఖాచిత్రం::dumpDotFilePrefixedAlways(స్థిరరాశి string& nameComment, తార్కిక colorAsSubgraph) స్థిరరాశి {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

శూన్య V3_లేఖాచిత్రం::dumpDotFile(స్థిరరాశి string& filename, తార్కిక colorAsSubgraph) స్థిరరాశి {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    స్థిరరాశి std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ఒకవేళ (logp->fail()) v3fatal("Can't write file: " << filename);

    //శీర్షిక
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_సమితి<std::string> ranks;
    std::unordered_multimap<std::string, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> rankSets;
    std::multimap<std::string, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> subgraphs;
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        స్థిరరాశి string శీర్షం_Subgraph
            = (colorAsSubgraph && శీర్షం.color()) ? cvt_గమ్యం_Str(శీర్షం.color()) : "";
        subgraphs.emplace(శీర్షం_Subgraph, &శీర్షం);
        స్థిరరాశి string& dotRank = శీర్షం.dotRank();
        ఒకవేళ (!dotRank.ఖాళీలు()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &శీర్షం);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*, పూర్ణాంకం> అంకె_Map;

    // Print vertices
    పూర్ణాంకం n = 0;
    string subgr;
    కొరకు (స్వయంచాలక it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        స్థిరరాశి string శీర్షం_Subgraph = it->మొదటి;
        స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vertexp = it->రెండవ;
        అంకె_Map[vertexp] = n;
        ఒకవేళ (subgr != శీర్షం_Subgraph) {
            ఒకవేళ (subgr != "") *logp << "\t};\n";
            subgr = శీర్షం_Subgraph;
            ఒకవేళ (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        ఒకవేళ (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        ఒకవేళ (vertexp->rank()) *logp << " r" << vertexp->rank();
        ఒకవేళ (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        ఒకవేళ (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        ఒకవేళ (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        ఒకవేళ (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    ఒకవేళ (subgr != "") *logp << "\t};\n";

    // Print edges
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం : vertices()) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : శీర్షం.బహిర్గతం_అంచులు()) {
            ఒకవేళ (అంచు.బరువు_2()) {
                స్థిరరాశి పూర్ణాంకం fromVnum = అంకె_Map[అంచు.fromp()];
                స్థిరరాశి పూర్ణాంకం గమ్యం_Vnum = అంకె_Map[అంచు.శిఖరం()];
                *logp << "\tn" << అంచు.fromp()->dotName() << fromVnum << " -> n"
                      << అంచు.శిఖరం()->dotName() << గమ్యం_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (అంచు.dotLabel() != "" ? అంచు.dotLabel() : "")
                      << "\"" << " weight=" << అంచు.బరువు_2() << " color=" << అంచు.dotColor();
                ఒకవేళ (అంచు.dotStyle() != "") *logp << " style=" << అంచు.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    కొరకు (స్థిరరాశి స్వయంచాలక& dotRank : ranks) {
        *logp << "\t{ rank=";
        ఒకవేళ (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } లేకపోతే {
            *logp << dotRank;
        }
        *logp << "; ";
        స్వయంచాలక bounds = rankSets.equal_range(dotRank);
        కొరకు (స్వయంచాలక it{bounds.మొదటి}; it != bounds.రెండవ; ++it) {
            ఒకవేళ (it != bounds.మొదటి) *logp << ", ";
            *logp << 'n' << అంకె_Map[it->రెండవ] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    బయటకు << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

శూన్య V3_లేఖాచిత్రం::hash_లేఖాచిత్రం_Debug(స్థిరరాశి అక్షరం* debugName) స్థిరరాశి {
    // Disabled when there are no nondeterminism issues in flight.
    ఒకవేళ (!v3Global.opt.debugNondeterminism()) తిరిగి;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*, uint32_t> vx2Id;
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtx.బహిర్గతం_అంచులు()) hash += vx2Id[అంచు.శిఖరం()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_గమ్యం_Hex(hash.విలువ()));
}
