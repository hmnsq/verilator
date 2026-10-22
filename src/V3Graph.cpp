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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

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
    m_userp = خالي_اشارو;
    graphp->vertices().link_پٺ(هذا);
}

V3_گراف_راس::V3_گراف_راس(V3_گراف* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = خالي_اشارو;
    graphp->vertices().link_پٺ(هذا);
}

فراغ V3_گراف_راس::unlink_ڪنارا(V3_گراف*) {
    طالما (V3_گراف_ڪنارو* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    طالما (V3_گراف_ڪنارو* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

فراغ V3_گراف_راس::unlinkDelete(V3_گراف* graphp) {
    // Delete edges
    unlink_ڪنارا(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(هذا);
    // Delete
    ميٽيو هذا;  // this=nullptr;
}

فراغ V3_گراف_راس::reroute_ڪنارا(V3_گراف* graphp) {
    // Make new edges for each from/to pair
    لكل (ثابت V3_گراف_ڪنارو& iedge : اندريون_ڪنارا()) {
        لكل (ثابت V3_گراف_ڪنارو& oedge : ٻاهريون_ڪنارا()) {
            جديد V3_گراف_ڪنارو{graphp, iedge.fromp(), oedge.چوٽي(),
                            std::min(iedge.وزن_2(), oedge.وزن_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_ڪنارا(graphp);
}

// cppcheck-has-bug-suppress constParameter
فراغ V3_گراف_راس::v3error_پڇاڙي(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (debug()) nsstr << "\n-vertex: " << هذا << '\n';
    إذا (File_سٽ* ثابت flp = fileline()) {
        flp->v3error_پڇاڙي(nsstr);
    } وإلا {
        V3Error::v3error_پڇاڙي(nsstr, "", خالي_اشارو);
    }
}
فراغ V3_گراف_راس::v3error_پڇاڙي_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پڇاڙي(str);
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
//ڪنارا

فراغ V3_گراف_ڪنارو::init(V3_گراف* /*graphp*/, V3_گراف_راس* fromp, V3_گراف_راس* چوٽي, صحيح وزن_2,
                       منطقي cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(چوٽي, "Null to pointer");
    m_fromp = fromp;
    m_چوٽي = چوٽي;
    m_وزن = وزن_2;
    m_cutable = cutable;
    m_userp = خالي_اشارو;
    // Link vertices to this edge
    m_fromp->ٻاهريون_ڪنارا().link_پٺ(هذا);
    m_چوٽي->اندريون_ڪنارا().link_پٺ(هذا);
}

فراغ V3_گراف_ڪنارو::relinkFromp(V3_گراف_راس* newFromp) {
    m_fromp->ٻاهريون_ڪنارا().unlink(هذا);
    m_fromp = newFromp;
    m_fromp->ٻاهريون_ڪنارا().link_پٺ(هذا);
}

فراغ V3_گراف_ڪنارو::relink_چوٽي(V3_گراف_راس* new_چوٽي) {
    m_چوٽي->اندريون_ڪنارا().unlink(هذا);
    m_چوٽي = new_چوٽي;
    m_چوٽي->اندريون_ڪنارا().link_پٺ(هذا);
}

فراغ V3_گراف_ڪنارو::unlinkDelete() {
    // Unlink from side
    m_fromp->ٻاهريون_ڪنارا().unlink(هذا);
    // Unlink to side
    m_چوٽي->اندريون_ڪنارا().unlink(هذا);
    // Delete
    ميٽيو هذا;
}

std::string V3_گراف_ڪنارو::name() ثابت { أرجع m_fromp->name() + "->" + m_چوٽي->name(); }

صحيح V3_گراف_ڪنارو::ترتيب_ڏيو_Cmp(ثابت V3_گراف_ڪنارو* rhsp) ثابت {
    إذا (!m_وزن || !rhsp->m_وزن) أرجع 0;
    أرجع چوٽي()->ترتيب_ڏيو_Cmp(rhsp->چوٽي());
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
        طالما (V3_گراف_ڪنارو* ثابت edgep = راس.ٻاهريون_ڪنارا().unlinkFront()) {
            VL_DO_DANGLING(ميٽيو edgep, edgep);
        }
    }
    // Delete the old vertices
    طالما (V3_گراف_راس* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(ميٽيو vertexp, vertexp);
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
        راس.userp(خالي_اشارو);  // It's a union, but might be different size than user()
    }
}

فراغ V3_گراف::user_Clear_ڪنارا() {
    // Clear user() in all of tree
    لكل (V3_گراف_راس& راس : vertices()) {
        لكل (V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) {
            ڪنارو.user(0);
            ڪنارو.userp(خالي_اشارو);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

فراغ V3_گراف::loops_پيغام_Cb(V3_گراف_راس* vertexp, V3_ڪنارو_Func_P ڪنارو_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(ڪنارو_Funcp, vertexp));
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
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : راس.اندريون_ڪنارا()) dump_ڪنارو(os, راس, ڪنارو);
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) dump_ڪنارو(os, راس, ڪنارو);
    }
}

فراغ V3_گراف::dump_ڪنارو(std::ostream& os, ثابت V3_گراف_راس& راس,
                       ثابت V3_گراف_ڪنارو& ڪنارو) ثابت {
    إذا (ڪنارو.وزن_2() && (ڪنارو.fromp() == &راس || ڪنارو.چوٽي() == &راس)) {
        os << "\t\t";
        إذا (ڪنارو.fromp() == &راس) os << "-> " << ڪنارو.چوٽي()->name();
        إذا (ڪنارو.چوٽي() == &راس) os << "<- " << ڪنارو.fromp()->name();
        إذا (ڪنارو.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
فراغ V3_گراف::dump_ڪنارا(std::ostream& os, ثابت V3_گراف_راس& راس) ثابت {
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : راس.اندريون_ڪنارا()) dump_ڪنارو(os, راس, ڪنارو);
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) dump_ڪنارو(os, راس, ڪنارو);
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

    //سرو
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_سيٽ<std::string> ranks;
    std::unordered_multimap<std::string, ثابت V3_گراف_راس*> rankSets;
    std::multimap<std::string, ثابت V3_گراف_راس*> subgraphs;
    لكل (ثابت V3_گراف_راس& راس : vertices()) {
        ثابت string راس_Subgraph
            = (colorAsSubgraph && راس.color()) ? cvt_منزل_Str(راس.color()) : "";
        subgraphs.emplace(راس_Subgraph, &راس);
        ثابت string& dotRank = راس.dotRank();
        إذا (!dotRank.خالي_جايون()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &راس);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3_گراف_راس*, صحيح> انگ_Map;

    // Print vertices
    صحيح n = 0;
    string subgr;
    لكل (خودڪار it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string راس_Subgraph = it->پهريون;
        ثابت V3_گراف_راس* vertexp = it->ٻيو;
        انگ_Map[vertexp] = n;
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
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) {
            إذا (ڪنارو.وزن_2()) {
                ثابت صحيح fromVnum = انگ_Map[ڪنارو.fromp()];
                ثابت صحيح منزل_Vnum = انگ_Map[ڪنارو.چوٽي()];
                *logp << "\tn" << ڪنارو.fromp()->dotName() << fromVnum << " -> n"
                      << ڪنارو.چوٽي()->dotName() << منزل_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (ڪنارو.dotLabel() != "" ? ڪنارو.dotLabel() : "")
                      << "\"" << " weight=" << ڪنارو.وزن_2() << " color=" << ڪنارو.dotColor();
                إذا (ڪنارو.dotStyle() != "") *logp << " style=" << ڪنارو.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    لكل (ثابت خودڪار& dotRank : ranks) {
        *logp << "\t{ rank=";
        إذا (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } وإلا {
            *logp << dotRank;
        }
        *logp << "; ";
        خودڪار bounds = rankSets.equal_range(dotRank);
        لكل (خودڪار it{bounds.پهريون}; it != bounds.ٻيو; ++it) {
            إذا (it != bounds.پهريون) *logp << ", ";
            *logp << 'n' << انگ_Map[it->ٻيو] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    ٻاهر << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
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
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vtx.ٻاهريون_ڪنارا()) hash += vx2Id[ڪنارو.چوٽي()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_منزل_Hex(hash.قدر()));
}
