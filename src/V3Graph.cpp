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

#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

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

V3_گراف_رأس::V3_گراف_رأس(V3_گراف* graphp, ثابت V3_گراف_رأس& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = اشاره_تهی;
    graphp->vertices().link_پشت(هذا);
}

V3_گراف_رأس::V3_گراف_رأس(V3_گراف* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = اشاره_تهی;
    graphp->vertices().link_پشت(هذا);
}

فراغ V3_گراف_رأس::unlink_یالها(V3_گراف*) {
    طالما (V3_گراف_یال* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    طالما (V3_گراف_یال* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

فراغ V3_گراف_رأس::unlinkDelete(V3_گراف* graphp) {
    // Delete edges
    unlink_یالها(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(هذا);
    // Delete
    حذف هذا;  // this=nullptr;
}

فراغ V3_گراف_رأس::reroute_یالها(V3_گراف* graphp) {
    // Make new edges for each from/to pair
    لكل (ثابت V3_گراف_یال& iedge : درون_یالها()) {
        لكل (ثابت V3_گراف_یال& oedge : بیرون_یالها()) {
            جديد V3_گراف_یال{graphp, iedge.fromp(), oedge.بالاترین(),
                            std::min(iedge.وزن_2(), oedge.وزن_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_یالها(graphp);
}

// cppcheck-has-bug-suppress constParameter
فراغ V3_گراف_رأس::v3error_انتها(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (debug()) nsstr << "\n-vertex: " << هذا << '\n';
    إذا (File_سطر* ثابت flp = fileline()) {
        flp->v3error_انتها(nsstr);
    } وإلا {
        V3Error::v3error_انتها(nsstr, "", اشاره_تهی);
    }
}
فراغ V3_گراف_رأس::v3error_انتها_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_انتها(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& عملگر<<(std::ostream& os, V3_گراف_رأس* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    إذا (vertexp->rank()) os << " r" << vertexp->rank();
    إذا (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    إذا (vertexp->color()) os << " c" << vertexp->color();
    أرجع os;
}

//######################################################################
//######################################################################
//یالها

فراغ V3_گراف_یال::init(V3_گراف* /*graphp*/, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, صحيح وزن_2,
                       منطقي cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(بالاترین, "Null to pointer");
    m_fromp = fromp;
    m_بالاترین = بالاترین;
    m_وزن = وزن_2;
    m_cutable = cutable;
    m_userp = اشاره_تهی;
    // Link vertices to this edge
    m_fromp->بیرون_یالها().link_پشت(هذا);
    m_بالاترین->درون_یالها().link_پشت(هذا);
}

فراغ V3_گراف_یال::relinkFromp(V3_گراف_رأس* newFromp) {
    m_fromp->بیرون_یالها().unlink(هذا);
    m_fromp = newFromp;
    m_fromp->بیرون_یالها().link_پشت(هذا);
}

فراغ V3_گراف_یال::relink_بالاترین(V3_گراف_رأس* new_بالاترین) {
    m_بالاترین->درون_یالها().unlink(هذا);
    m_بالاترین = new_بالاترین;
    m_بالاترین->درون_یالها().link_پشت(هذا);
}

فراغ V3_گراف_یال::unlinkDelete() {
    // Unlink from side
    m_fromp->بیرون_یالها().unlink(هذا);
    // Unlink to side
    m_بالاترین->درون_یالها().unlink(هذا);
    // Delete
    حذف هذا;
}

std::string V3_گراف_یال::name() ثابت { أرجع m_fromp->name() + "->" + m_بالاترین->name(); }

صحيح V3_گراف_یال::مرتب_سازی_Cmp(ثابت V3_گراف_یال* rhsp) ثابت {
    إذا (!m_وزن || !rhsp->m_وزن) أرجع 0;
    أرجع بالاترین()->مرتب_سازی_Cmp(rhsp->بالاترین());
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
    لكل (V3_گراف_رأس& رأس : vertices()) {
        طالما (V3_گراف_یال* ثابت edgep = رأس.بیرون_یالها().unlinkFront()) {
            VL_DO_DANGLING(حذف edgep, edgep);
        }
    }
    // Delete the old vertices
    طالما (V3_گراف_رأس* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(حذف vertexp, vertexp);
    }
}

فراغ V3_گراف::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    لكل (V3_گراف_رأس& رأس : vertices()) {
        رأس.user(0);
        رأس.userp(اشاره_تهی);  // It's a union, but might be different size than user()
    }
}

فراغ V3_گراف::user_Clear_یالها() {
    // Clear user() in all of tree
    لكل (V3_گراف_رأس& رأس : vertices()) {
        لكل (V3_گراف_یال& یال : رأس.بیرون_یالها()) {
            یال.user(0);
            یال.userp(اشاره_تهی);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

فراغ V3_گراف::loops_پیام_Cb(V3_گراف_رأس* vertexp, V3_یال_Func_P یال_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(یال_Funcp, vertexp));
}
string V3_گراف::loops_رأس_Cb(V3_گراف_رأس* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    إذا (debug())
        أرجع "-Info-Loop: "s + cvt_مقصد_Hex(vertexp) + ' ' + cvt_مقصد_Str(vertexp) + '\n';
    وإلا
        أرجع "";
}

فراغ V3_گراف::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    لكل (ثابت V3_گراف_رأس& رأس : vertices()) {
        os << "\tNode: " << رأس.name();
        إذا (رأس.color()) os << "  color=" << رأس.color();
        os << '\n';
        // Print edges
        لكل (ثابت V3_گراف_یال& یال : رأس.درون_یالها()) dump_یال(os, رأس, یال);
        لكل (ثابت V3_گراف_یال& یال : رأس.بیرون_یالها()) dump_یال(os, رأس, یال);
    }
}

فراغ V3_گراف::dump_یال(std::ostream& os, ثابت V3_گراف_رأس& رأس,
                       ثابت V3_گراف_یال& یال) ثابت {
    إذا (یال.وزن_2() && (یال.fromp() == &رأس || یال.بالاترین() == &رأس)) {
        os << "\t\t";
        إذا (یال.fromp() == &رأس) os << "-> " << یال.بالاترین()->name();
        إذا (یال.بالاترین() == &رأس) os << "<- " << یال.fromp()->name();
        إذا (یال.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
فراغ V3_گراف::dump_یالها(std::ostream& os, ثابت V3_گراف_رأس& رأس) ثابت {
    لكل (ثابت V3_گراف_یال& یال : رأس.درون_یالها()) dump_یال(os, رأس, یال);
    لكل (ثابت V3_گراف_یال& یال : رأس.بیرون_یالها()) dump_یال(os, رأس, یال);
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

    //سرآیند
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_مجموعه<std::string> ranks;
    std::unordered_multimap<std::string, ثابت V3_گراف_رأس*> rankSets;
    std::multimap<std::string, ثابت V3_گراف_رأس*> subgraphs;
    لكل (ثابت V3_گراف_رأس& رأس : vertices()) {
        ثابت string رأس_Subgraph
            = (colorAsSubgraph && رأس.color()) ? cvt_مقصد_Str(رأس.color()) : "";
        subgraphs.emplace(رأس_Subgraph, &رأس);
        ثابت string& dotRank = رأس.dotRank();
        إذا (!dotRank.خالی()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &رأس);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3_گراف_رأس*, صحيح> رقم_Map;

    // Print vertices
    صحيح n = 0;
    string subgr;
    لكل (خودکار it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string رأس_Subgraph = it->اولی;
        ثابت V3_گراف_رأس* vertexp = it->دومی;
        رقم_Map[vertexp] = n;
        إذا (subgr != رأس_Subgraph) {
            إذا (subgr != "") *logp << "\t};\n";
            subgr = رأس_Subgraph;
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
    لكل (ثابت V3_گراف_رأس& رأس : vertices()) {
        لكل (ثابت V3_گراف_یال& یال : رأس.بیرون_یالها()) {
            إذا (یال.وزن_2()) {
                ثابت صحيح fromVnum = رقم_Map[یال.fromp()];
                ثابت صحيح مقصد_Vnum = رقم_Map[یال.بالاترین()];
                *logp << "\tn" << یال.fromp()->dotName() << fromVnum << " -> n"
                      << یال.بالاترین()->dotName() << مقصد_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (یال.dotLabel() != "" ? یال.dotLabel() : "")
                      << "\"" << " weight=" << یال.وزن_2() << " color=" << یال.dotColor();
                إذا (یال.dotStyle() != "") *logp << " style=" << یال.dotStyle();
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
        لكل (خودکار it{bounds.اولی}; it != bounds.دومی; ++it) {
            إذا (it != bounds.اولی) *logp << ", ";
            *logp << 'n' << رقم_Map[it->دومی] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    خروجی << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

فراغ V3_گراف::hash_گراف_Debug(ثابت حرف* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    إذا (!v3Global.opt.debugNondeterminism()) أرجع;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3_گراف_رأس*, uint32_t> vx2Id;
    لكل (ثابت V3_گراف_رأس& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    لكل (ثابت V3_گراف_رأس& vtx : vertices()) {
        لكل (ثابت V3_گراف_یال& یال : vtx.بیرون_یالها()) hash += vx2Id[یال.بالاترین()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_مقصد_Hex(hash.مقدار()));
}
