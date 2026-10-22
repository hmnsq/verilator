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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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

V3_گراف_راس::V3_گراف_راس(V3_گراف* graphp, ثابت V3_گراف_راس& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = خالی_سوچک;
    graphp->vertices().link_پٹھ(هذا);
}

V3_گراف_راس::V3_گراف_راس(V3_گراف* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = خالی_سوچک;
    graphp->vertices().link_پٹھ(هذا);
}

فراغ V3_گراف_راس::unlink_کنارے(V3_گراف*) {
    طالما (V3_گراف_کنارا* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    طالما (V3_گراف_کنارا* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

فراغ V3_گراف_راس::unlinkDelete(V3_گراف* graphp) {
    // Delete edges
    unlink_کنارے(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(هذا);
    // Delete
    مٹاؤ هذا;  // this=nullptr;
}

فراغ V3_گراف_راس::reroute_کنارے(V3_گراف* graphp) {
    // Make new edges for each from/to pair
    لكل (ثابت V3_گراف_کنارا& iedge : وچ_کنارے()) {
        لكل (ثابت V3_گراف_کنارا& oedge : باہرلا_کنارے()) {
            جديد V3_گراف_کنارا{graphp, iedge.fromp(), oedge.چوٹی(),
                            std::min(iedge.بھار_2(), oedge.بھار_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_کنارے(graphp);
}

// cppcheck-has-bug-suppress constParameter
فراغ V3_گراف_راس::v3error_سرا(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (debug()) nsstr << "\n-vertex: " << هذا << '\n';
    إذا (File_سطر* ثابت flp = fileline()) {
        flp->v3error_سرا(nsstr);
    } وإلا {
        V3Error::v3error_سرا(nsstr, "", خالی_سوچک);
    }
}
فراغ V3_گراف_راس::v3error_سرا_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_سرا(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& عامل<<(std::ostream& os, V3_گراف_راس* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    إذا (vertexp->rank()) os << " r" << vertexp->rank();
    إذا (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    إذا (vertexp->color()) os << " c" << vertexp->color();
    أرجع os;
}

//######################################################################
//######################################################################
//کنارے

فراغ V3_گراف_کنارا::init(V3_گراف* /*graphp*/, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2,
                       منطقي cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(چوٹی, "Null to pointer");
    m_fromp = fromp;
    m_چوٹی = چوٹی;
    m_بھار = بھار_2;
    m_cutable = cutable;
    m_userp = خالی_سوچک;
    // Link vertices to this edge
    m_fromp->باہرلا_کنارے().link_پٹھ(هذا);
    m_چوٹی->وچ_کنارے().link_پٹھ(هذا);
}

فراغ V3_گراف_کنارا::relinkFromp(V3_گراف_راس* newFromp) {
    m_fromp->باہرلا_کنارے().unlink(هذا);
    m_fromp = newFromp;
    m_fromp->باہرلا_کنارے().link_پٹھ(هذا);
}

فراغ V3_گراف_کنارا::relink_چوٹی(V3_گراف_راس* new_چوٹی) {
    m_چوٹی->وچ_کنارے().unlink(هذا);
    m_چوٹی = new_چوٹی;
    m_چوٹی->وچ_کنارے().link_پٹھ(هذا);
}

فراغ V3_گراف_کنارا::unlinkDelete() {
    // Unlink from side
    m_fromp->باہرلا_کنارے().unlink(هذا);
    // Unlink to side
    m_چوٹی->وچ_کنارے().unlink(هذا);
    // Delete
    مٹاؤ هذا;
}

std::string V3_گراف_کنارا::name() ثابت { أرجع m_fromp->name() + "->" + m_چوٹی->name(); }

صحيح V3_گراف_کنارا::ترتیب_دیو_Cmp(ثابت V3_گراف_کنارا* rhsp) ثابت {
    إذا (!m_بھار || !rhsp->m_بھار) أرجع 0;
    أرجع چوٹی()->ترتیب_دیو_Cmp(rhsp->چوٹی());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_گراف::V3_گراف() {}

V3_گراف::~V3_گراف() { clear(); }

فراغ V3_گراف::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    لكل (V3_گراف_راس& راس : vertices()) {
        طالما (V3_گراف_کنارا* ثابت edgep = راس.باہرلا_کنارے().unlinkFront()) {
            VL_DO_DANGLING(مٹاؤ edgep, edgep);
        }
    }
    // Delete the old vertices
    طالما (V3_گراف_راس* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(مٹاؤ vertexp, vertexp);
    }
}

فراغ V3_گراف::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    لكل (V3_گراف_راس& راس : vertices()) {
        راس.user(0);
        راس.userp(خالی_سوچک);  // It's a union, but might be different size than user()
    }
}

فراغ V3_گراف::user_Clear_کنارے() {
    // Clear user() in all of tree
    لكل (V3_گراف_راس& راس : vertices()) {
        لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) {
            کنارا.user(0);
            کنارا.userp(خالی_سوچک);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

فراغ V3_گراف::loops_سنیہا_Cb(V3_گراف_راس* vertexp, V3_کنارا_Func_P کنارا_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(کنارا_Funcp, vertexp));
}
string V3_گراف::loops_راس_Cb(V3_گراف_راس* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    إذا (debug())
        أرجع "-Info-Loop: "s + cvt_منزل_Hex(vertexp) + ' ' + cvt_منزل_Str(vertexp) + '\n';
    وإلا
        أرجع "";
}

فراغ V3_گراف::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    لكل (ثابت V3_گراف_راس& راس : vertices()) {
        os << "\tNode: " << راس.name();
        إذا (راس.color()) os << "  color=" << راس.color();
        os << '\n';
        // Print edges
        لكل (ثابت V3_گراف_کنارا& کنارا : راس.وچ_کنارے()) dump_کنارا(os, راس, کنارا);
        لكل (ثابت V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) dump_کنارا(os, راس, کنارا);
    }
}

فراغ V3_گراف::dump_کنارا(std::ostream& os, ثابت V3_گراف_راس& راس,
                       ثابت V3_گراف_کنارا& کنارا) ثابت {
    إذا (کنارا.بھار_2() && (کنارا.fromp() == &راس || کنارا.چوٹی() == &راس)) {
        os << "\t\t";
        إذا (کنارا.fromp() == &راس) os << "-> " << کنارا.چوٹی()->name();
        إذا (کنارا.چوٹی() == &راس) os << "<- " << کنارا.fromp()->name();
        إذا (کنارا.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
فراغ V3_گراف::dump_کنارے(std::ostream& os, ثابت V3_گراف_راس& راس) ثابت {
    لكل (ثابت V3_گراف_کنارا& کنارا : راس.وچ_کنارے()) dump_کنارا(os, راس, کنارا);
    لكل (ثابت V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) dump_کنارا(os, راس, کنارا);
}

فراغ V3_گراف::dumpDotFilePrefixed(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
فراغ V3_گراف::dumpDotFilePrefixedAlways(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

فراغ V3_گراف::dumpDotFile(ثابت string& filename, منطقي colorAsSubgraph) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    إذا (logp->fail()) v3fatal("Can't write file: " << filename);

    //سرخی
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_سیٹ<std::string> ranks;
    std::unordered_multimap<std::string, ثابت V3_گراف_راس*> rankSets;
    std::multimap<std::string, ثابت V3_گراف_راس*> subgraphs;
    لكل (ثابت V3_گراف_راس& راس : vertices()) {
        ثابت string راس_Subgraph
            = (colorAsSubgraph && راس.color()) ? cvt_منزل_Str(راس.color()) : "";
        subgraphs.emplace(راس_Subgraph, &راس);
        ثابت string& dotRank = راس.dotRank();
        إذا (!dotRank.خالی_تھاواں()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &راس);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3_گراف_راس*, صحيح> عدد_Map;

    // Print vertices
    صحيح n = 0;
    string subgr;
    لكل (خودکار it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string راس_Subgraph = it->پہلا;
        ثابت V3_گراف_راس* vertexp = it->دوجا;
        عدد_Map[vertexp] = n;
        إذا (subgr != راس_Subgraph) {
            إذا (subgr != "") *logp << "\t};\n";
            subgr = راس_Subgraph;
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
    لكل (ثابت V3_گراف_راس& راس : vertices()) {
        لكل (ثابت V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) {
            إذا (کنارا.بھار_2()) {
                ثابت صحيح fromVnum = عدد_Map[کنارا.fromp()];
                ثابت صحيح منزل_Vnum = عدد_Map[کنارا.چوٹی()];
                *logp << "\tn" << کنارا.fromp()->dotName() << fromVnum << " -> n"
                      << کنارا.چوٹی()->dotName() << منزل_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (کنارا.dotLabel() != "" ? کنارا.dotLabel() : "")
                      << "\"" << " weight=" << کنارا.بھار_2() << " color=" << کنارا.dotColor();
                إذا (کنارا.dotStyle() != "") *logp << " style=" << کنارا.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    لكل (ثابت خودکار& dotRank : ranks) {
        *logp << "\t{ rank=";
        إذا (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } وإلا {
            *logp << dotRank;
        }
        *logp << "; ";
        خودکار bounds = rankSets.equal_range(dotRank);
        لكل (خودکار it{bounds.پہلا}; it != bounds.دوجا; ++it) {
            إذا (it != bounds.پہلا) *logp << ", ";
            *logp << 'n' << عدد_Map[it->دوجا] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    باہر << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

فراغ V3_گراف::hash_گراف_Debug(ثابت حرف* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    إذا (!v3Global.opt.debugNondeterminism()) أرجع;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3_گراف_راس*, uint32_t> vx2Id;
    لكل (ثابت V3_گراف_راس& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        لكل (ثابت V3_گراف_کنارا& کنارا : vtx.باہرلا_کنارے()) hash += vx2Id[کنارا.چوٹی()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_منزل_Hex(hash.قدر()));
}
