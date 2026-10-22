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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

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

V3_گراف_راس::V3_گراف_راس(V3_گراف* graphp, ثابت V3_گراف_راس& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = خالی_اشاریہ;
    graphp->vertices().link_عقب(یہ);
}

V3_گراف_راس::V3_گراف_راس(V3_گراف* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = خالی_اشاریہ;
    graphp->vertices().link_عقب(یہ);
}

خالی V3_گراف_راس::unlink_کنارے(V3_گراف*) {
    جب (V3_گراف_کنارہ* ثابت ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    جب (V3_گراف_کنارہ* ثابت ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

خالی V3_گراف_راس::unlinkDelete(V3_گراف* graphp) {
    // Delete edges
    unlink_کنارے(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(یہ);
    // Delete
    حذف یہ;  // this=nullptr;
}

خالی V3_گراف_راس::reroute_کنارے(V3_گراف* graphp) {
    // Make new edges for each from/to pair
    برائے (ثابت V3_گراف_کنارہ& iedge : اندر_کنارے()) {
        برائے (ثابت V3_گراف_کنارہ& oedge : برآمد_کنارے()) {
            نیا V3_گراف_کنارہ{graphp, iedge.fromp(), oedge.چوٹی(),
                            std::min(iedge.وزن_2(), oedge.وزن_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_کنارے(graphp);
}

// cppcheck-has-bug-suppress constParameter
خالی V3_گراف_راس::v3error_انتہا(ثابت std::ostringstream& str) ثابت  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    اگر (debug()) nsstr << "\n-vertex: " << یہ << '\n';
    اگر (File_سطر* ثابت flp = fileline()) {
        flp->v3error_انتہا(nsstr);
    } ورنہ {
        V3Error::v3error_انتہا(nsstr, "", خالی_اشاریہ);
    }
}
خالی V3_گراف_راس::v3error_انتہا_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_انتہا(str);
    أكد(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& عامل<<(std::ostream& os, V3_گراف_راس* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    اگر (vertexp->rank()) os << " r" << vertexp->rank();
    اگر (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    اگر (vertexp->color()) os << " c" << vertexp->color();
    واپس os;
}

//######################################################################
//######################################################################
//کنارے

خالی V3_گراف_کنارہ::init(V3_گراف* /*graphp*/, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2,
                       منطقی cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(چوٹی, "Null to pointer");
    m_fromp = fromp;
    m_چوٹی = چوٹی;
    m_وزن = وزن_2;
    m_cutable = cutable;
    m_userp = خالی_اشاریہ;
    // Link vertices to this edge
    m_fromp->برآمد_کنارے().link_عقب(یہ);
    m_چوٹی->اندر_کنارے().link_عقب(یہ);
}

خالی V3_گراف_کنارہ::relinkFromp(V3_گراف_راس* newFromp) {
    m_fromp->برآمد_کنارے().unlink(یہ);
    m_fromp = newFromp;
    m_fromp->برآمد_کنارے().link_عقب(یہ);
}

خالی V3_گراف_کنارہ::relink_چوٹی(V3_گراف_راس* new_چوٹی) {
    m_چوٹی->اندر_کنارے().unlink(یہ);
    m_چوٹی = new_چوٹی;
    m_چوٹی->اندر_کنارے().link_عقب(یہ);
}

خالی V3_گراف_کنارہ::unlinkDelete() {
    // Unlink from side
    m_fromp->برآمد_کنارے().unlink(یہ);
    // Unlink to side
    m_چوٹی->اندر_کنارے().unlink(یہ);
    // Delete
    حذف یہ;
}

std::string V3_گراف_کنارہ::name() ثابت { واپس m_fromp->name() + "->" + m_چوٹی->name(); }

صحیح V3_گراف_کنارہ::ترتیب_دو_Cmp(ثابت V3_گراف_کنارہ* rhsp) ثابت {
    اگر (!m_وزن || !rhsp->m_وزن) واپس 0;
    واپس چوٹی()->ترتیب_دو_Cmp(rhsp->چوٹی());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_گراف::V3_گراف() {}

V3_گراف::~V3_گراف() { clear(); }

خالی V3_گراف::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    برائے (V3_گراف_راس& راس : vertices()) {
        جب (V3_گراف_کنارہ* ثابت edgep = راس.برآمد_کنارے().unlinkFront()) {
            VL_DO_DANGLING(حذف edgep, edgep);
        }
    }
    // Delete the old vertices
    جب (V3_گراف_راس* ثابت vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(حذف vertexp, vertexp);
    }
}

خالی V3_گراف::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    برائے (V3_گراف_راس& راس : vertices()) {
        راس.user(0);
        راس.userp(خالی_اشاریہ);  // It's a union, but might be different size than user()
    }
}

خالی V3_گراف::user_Clear_کنارے() {
    // Clear user() in all of tree
    برائے (V3_گراف_راس& راس : vertices()) {
        برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) {
            کنارہ.user(0);
            کنارہ.userp(خالی_اشاریہ);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

خالی V3_گراف::loops_پیغام_Cb(V3_گراف_راس* vertexp, V3_کنارہ_Func_P کنارہ_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(کنارہ_Funcp, vertexp));
}
string V3_گراف::loops_راس_Cb(V3_گراف_راس* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    اگر (debug())
        واپس "-Info-Loop: "s + cvt_مقصد_Hex(vertexp) + ' ' + cvt_مقصد_Str(vertexp) + '\n';
    ورنہ
        واپس "";
}

خالی V3_گراف::dump(std::ostream& os) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    برائے (ثابت V3_گراف_راس& راس : vertices()) {
        os << "\tNode: " << راس.name();
        اگر (راس.color()) os << "  color=" << راس.color();
        os << '\n';
        // Print edges
        برائے (ثابت V3_گراف_کنارہ& کنارہ : راس.اندر_کنارے()) dump_کنارہ(os, راس, کنارہ);
        برائے (ثابت V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) dump_کنارہ(os, راس, کنارہ);
    }
}

خالی V3_گراف::dump_کنارہ(std::ostream& os, ثابت V3_گراف_راس& راس,
                       ثابت V3_گراف_کنارہ& کنارہ) ثابت {
    اگر (کنارہ.وزن_2() && (کنارہ.fromp() == &راس || کنارہ.چوٹی() == &راس)) {
        os << "\t\t";
        اگر (کنارہ.fromp() == &راس) os << "-> " << کنارہ.چوٹی()->name();
        اگر (کنارہ.چوٹی() == &راس) os << "<- " << کنارہ.fromp()->name();
        اگر (کنارہ.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
خالی V3_گراف::dump_کنارے(std::ostream& os, ثابت V3_گراف_راس& راس) ثابت {
    برائے (ثابت V3_گراف_کنارہ& کنارہ : راس.اندر_کنارے()) dump_کنارہ(os, راس, کنارہ);
    برائے (ثابت V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) dump_کنارہ(os, راس, کنارہ);
}

خالی V3_گراف::dumpDotFilePrefixed(ثابت string& nameComment, منطقی colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
خالی V3_گراف::dumpDotFilePrefixedAlways(ثابت string& nameComment, منطقی colorAsSubgraph) ثابت {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

خالی V3_گراف::dumpDotFile(ثابت string& filename, منطقی colorAsSubgraph) ثابت {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    اگر (logp->fail()) v3fatal("Can't write file: " << filename);

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
    برائے (ثابت V3_گراف_راس& راس : vertices()) {
        ثابت string راس_Subgraph
            = (colorAsSubgraph && راس.color()) ? cvt_مقصد_Str(راس.color()) : "";
        subgraphs.emplace(راس_Subgraph, &راس);
        ثابت string& dotRank = راس.dotRank();
        اگر (!dotRank.خالی_خانے()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &راس);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ثابت V3_گراف_راس*, صحیح> عدد_Map;

    // Print vertices
    صحیح n = 0;
    string subgr;
    برائے (خودکار it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ثابت string راس_Subgraph = it->پہلا;
        ثابت V3_گراف_راس* vertexp = it->دوسرا;
        عدد_Map[vertexp] = n;
        اگر (subgr != راس_Subgraph) {
            اگر (subgr != "") *logp << "\t};\n";
            subgr = راس_Subgraph;
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
    برائے (ثابت V3_گراف_راس& راس : vertices()) {
        برائے (ثابت V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) {
            اگر (کنارہ.وزن_2()) {
                ثابت صحیح fromVnum = عدد_Map[کنارہ.fromp()];
                ثابت صحیح مقصد_Vnum = عدد_Map[کنارہ.چوٹی()];
                *logp << "\tn" << کنارہ.fromp()->dotName() << fromVnum << " -> n"
                      << کنارہ.چوٹی()->dotName() << مقصد_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (کنارہ.dotLabel() != "" ? کنارہ.dotLabel() : "")
                      << "\"" << " weight=" << کنارہ.وزن_2() << " color=" << کنارہ.dotColor();
                اگر (کنارہ.dotStyle() != "") *logp << " style=" << کنارہ.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    برائے (ثابت خودکار& dotRank : ranks) {
        *logp << "\t{ rank=";
        اگر (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } ورنہ {
            *logp << dotRank;
        }
        *logp << "; ";
        خودکار bounds = rankSets.equal_range(dotRank);
        برائے (خودکار it{bounds.پہلا}; it != bounds.دوسرا; ++it) {
            اگر (it != bounds.پہلا) *logp << ", ";
            *logp << 'n' << عدد_Map[it->دوسرا] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    خارج << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

خالی V3_گراف::hash_گراف_Debug(ثابت حرف* debugName) ثابت {
    // Disabled when there are no nondeterminism issues in flight.
    اگر (!v3Global.opt.debugNondeterminism()) واپس;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ثابت V3_گراف_راس*, uint32_t> vx2Id;
    برائے (ثابت V3_گراف_راس& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        برائے (ثابت V3_گراف_کنارہ& کنارہ : vtx.برآمد_کنارے()) hash += vx2Id[کنارہ.چوٹی()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_مقصد_Hex(hash.قدر()));
}
