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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

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

V3_ګراف_راس::V3_ګراف_راس(V3_ګراف* graphp, ثابت V3_ګراف_راس& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = تش_اشاره;
    graphp->vertices().link_شا(هذا);
}

V3_ګراف_راس::V3_ګراف_راس(V3_ګراف* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = تش_اشاره;
    graphp->vertices().link_شا(هذا);
}

فراغ V3_ګراف_راس::unlink_څنډې(V3_ګراف*) {
    طالما (V3_ګراف_څنډه* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    طالما (V3_ګراف_څنډه* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

فراغ V3_ګراف_راس::unlinkDelete(V3_ګراف* graphp) {
    // Delete edges
    unlink_څنډې(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(هذا);
    // Delete
    ړنګول هذا;  // this=nullptr;
}

فراغ V3_ګراف_راس::reroute_څنډې(V3_ګراف* graphp) {
    // Make new edges for each from/to pair
    لكل (ثابت V3_ګراف_څنډه& iedge : دننه_څنډې()) {
        لكل (ثابت V3_ګراف_څنډه& oedge : بهر_څنډې()) {
            جديد V3_ګراف_څنډه{graphp, iedge.fromp(), oedge.سر(),
                            std::min(iedge.وزن_2(), oedge.وزن_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_څنډې(graphp);
}

// cppcheck-has-bug-suppress constParameter
فراغ V3_ګراف_راس::v3error_پای_ټکی(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (debug()) nsstr << "\n-vertex: " << هذا << '\n';
    إذا (File_کرښه* ثابت flp = fileline()) {
        flp->v3error_پای_ټکی(nsstr);
    } وإلا {
        V3Error::v3error_پای_ټکی(nsstr, "", تش_اشاره);
    }
}
فراغ V3_ګراف_راس::v3error_پای_ټکی_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پای_ټکی(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& چلوونکی<<(std::ostream& os, V3_ګراف_راس* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    إذا (vertexp->rank()) os << " r" << vertexp->rank();
    إذا (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    إذا (vertexp->color()) os << " c" << vertexp->color();
    أرجع os;
}

//######################################################################
//######################################################################
//څنډې

فراغ V3_ګراف_څنډه::init(V3_ګراف* /*graphp*/, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2,
                       منطقي cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(سر, "Null to pointer");
    m_fromp = fromp;
    m_سر = سر;
    m_وزن = وزن_2;
    m_cutable = cutable;
    m_userp = تش_اشاره;
    // Link vertices to this edge
    m_fromp->بهر_څنډې().link_شا(هذا);
    m_سر->دننه_څنډې().link_شا(هذا);
}

فراغ V3_ګراف_څنډه::relinkFromp(V3_ګراف_راس* newFromp) {
    m_fromp->بهر_څنډې().unlink(هذا);
    m_fromp = newFromp;
    m_fromp->بهر_څنډې().link_شا(هذا);
}

فراغ V3_ګراف_څنډه::relink_سر(V3_ګراف_راس* new_سر) {
    m_سر->دننه_څنډې().unlink(هذا);
    m_سر = new_سر;
    m_سر->دننه_څنډې().link_شا(هذا);
}

فراغ V3_ګراف_څنډه::unlinkDelete() {
    // Unlink from side
    m_fromp->بهر_څنډې().unlink(هذا);
    // Unlink to side
    m_سر->دننه_څنډې().unlink(هذا);
    // Delete
    ړنګول هذا;
}

std::string V3_ګراف_څنډه::name() ثابت { أرجع m_fromp->name() + "->" + m_سر->name(); }

صحيح V3_ګراف_څنډه::ترتیبول_Cmp(ثابت V3_ګراف_څنډه* rhsp) ثابت {
    إذا (!m_وزن || !rhsp->m_وزن) أرجع 0;
    أرجع سر()->ترتیبول_Cmp(rhsp->سر());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_ګراف::V3_ګراف() {}

V3_ګراف::~V3_ګراف() { clear(); }

فراغ V3_ګراف::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    لكل (V3_ګراف_راس& راس : vertices()) {
        طالما (V3_ګراف_څنډه* ثابت edgep = راس.بهر_څنډې().unlinkFront()) {
            VL_DO_DANGLING(ړنګول edgep, edgep);
        }
    }
    // Delete the old vertices
    طالما (V3_ګراف_راس* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(ړنګول vertexp, vertexp);
    }
}

فراغ V3_ګراف::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    لكل (V3_ګراف_راس& راس : vertices()) {
        راس.user(0);
        راس.userp(تش_اشاره);  // It's a union, but might be different size than user()
    }
}

فراغ V3_ګراف::user_Clear_څنډې() {
    // Clear user() in all of tree
    لكل (V3_ګراف_راس& راس : vertices()) {
        لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) {
            څنډه.user(0);
            څنډه.userp(تش_اشاره);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

فراغ V3_ګراف::loops_پیغام_Cb(V3_ګراف_راس* vertexp, V3_څنډه_Func_P څنډه_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(څنډه_Funcp, vertexp));
}
string V3_ګراف::loops_راس_Cb(V3_ګراف_راس* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    إذا (debug())
        أرجع "-Info-Loop: "s + cvt_منزل_Hex(vertexp) + ' ' + cvt_منزل_Str(vertexp) + '\n';
    وإلا
        أرجع "";
}

فراغ V3_ګراف::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    لكل (ثابت V3_ګراف_راس& راس : vertices()) {
        os << "\tNode: " << راس.name();
        إذا (راس.color()) os << "  color=" << راس.color();
        os << '\n';
        // Print edges
        لكل (ثابت V3_ګراف_څنډه& څنډه : راس.دننه_څنډې()) dump_څنډه(os, راس, څنډه);
        لكل (ثابت V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) dump_څنډه(os, راس, څنډه);
    }
}

فراغ V3_ګراف::dump_څنډه(std::ostream& os, ثابت V3_ګراف_راس& راس,
                       ثابت V3_ګراف_څنډه& څنډه) ثابت {
    إذا (څنډه.وزن_2() && (څنډه.fromp() == &راس || څنډه.سر() == &راس)) {
        os << "\t\t";
        إذا (څنډه.fromp() == &راس) os << "-> " << څنډه.سر()->name();
        إذا (څنډه.سر() == &راس) os << "<- " << څنډه.fromp()->name();
        إذا (څنډه.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
فراغ V3_ګراف::dump_څنډې(std::ostream& os, ثابت V3_ګراف_راس& راس) ثابت {
    لكل (ثابت V3_ګراف_څنډه& څنډه : راس.دننه_څنډې()) dump_څنډه(os, راس, څنډه);
    لكل (ثابت V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) dump_څنډه(os, راس, څنډه);
}

فراغ V3_ګراف::dumpDotFilePrefixed(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
فراغ V3_ګراف::dumpDotFilePrefixedAlways(ثابت string& nameComment, منطقي colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

فراغ V3_ګراف::dumpDotFile(ثابت string& filename, منطقي colorAsSubgraph) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    إذا (logp->fail()) v3fatal("Can't write file: " << filename);

    //سرلیک
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_سېټ<std::string> ranks;
    std::unordered_multimap<std::string, ثابت V3_ګراف_راس*> rankSets;
    std::multimap<std::string, ثابت V3_ګراف_راس*> subgraphs;
    لكل (ثابت V3_ګراف_راس& راس : vertices()) {
        ثابت string راس_Subgraph
            = (colorAsSubgraph && راس.color()) ? cvt_منزل_Str(راس.color()) : "";
        subgraphs.emplace(راس_Subgraph, &راس);
        ثابت string& dotRank = راس.dotRank();
        إذا (!dotRank.تشه()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &راس);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3_ګراف_راس*, صحيح> عدد_Map;

    // Print vertices
    صحيح n = 0;
    string subgr;
    لكل (خپلکاری it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string راس_Subgraph = it->لومړی;
        ثابت V3_ګراف_راس* vertexp = it->دوهم;
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
    لكل (ثابت V3_ګراف_راس& راس : vertices()) {
        لكل (ثابت V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) {
            إذا (څنډه.وزن_2()) {
                ثابت صحيح fromVnum = عدد_Map[څنډه.fromp()];
                ثابت صحيح منزل_Vnum = عدد_Map[څنډه.سر()];
                *logp << "\tn" << څنډه.fromp()->dotName() << fromVnum << " -> n"
                      << څنډه.سر()->dotName() << منزل_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (څنډه.dotLabel() != "" ? څنډه.dotLabel() : "")
                      << "\"" << " weight=" << څنډه.وزن_2() << " color=" << څنډه.dotColor();
                إذا (څنډه.dotStyle() != "") *logp << " style=" << څنډه.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    لكل (ثابت خپلکاری& dotRank : ranks) {
        *logp << "\t{ rank=";
        إذا (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } وإلا {
            *logp << dotRank;
        }
        *logp << "; ";
        خپلکاری bounds = rankSets.equal_range(dotRank);
        لكل (خپلکاری it{bounds.لومړی}; it != bounds.دوهم; ++it) {
            إذا (it != bounds.لومړی) *logp << ", ";
            *logp << 'n' << عدد_Map[it->دوهم] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    وتنه << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

فراغ V3_ګراف::hash_ګراف_Debug(ثابت حرف* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    إذا (!v3Global.opt.debugNondeterminism()) أرجع;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3_ګراف_راس*, uint32_t> vx2Id;
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        لكل (ثابت V3_ګراف_څنډه& څنډه : vtx.بهر_څنډې()) hash += vx2Id[څنډه.سر()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_منزل_Hex(hash.ارزښت()));
}
