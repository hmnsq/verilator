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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3_ನಕ್ಷೆ_ಶೃಂಗ::V3_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = ಶೂನ್ಯಸೂಚಕ;
    graphp->vertices().link_ಹಿಂಭಾಗ(ಇದು);
}

V3_ನಕ್ಷೆ_ಶೃಂಗ::V3_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = ಶೂನ್ಯಸೂಚಕ;
    graphp->vertices().link_ಹಿಂಭಾಗ(ಇದು);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಶೃಂಗ::unlink_ಅಂಚುಗಳು(V3_ನಕ್ಷೆ*) {
    ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಶೃಂಗ::unlinkDelete(V3_ನಕ್ಷೆ* graphp) {
    // Delete edges
    unlink_ಅಂಚುಗಳು(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ಇದು);
    // Delete
    ಅಳಿಸು ಇದು;  // this=nullptr;
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಶೃಂಗ::reroute_ಅಂಚುಗಳು(V3_ನಕ್ಷೆ* graphp) {
    // Make new edges for each from/to pair
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& iedge : ಒಳ_ಅಂಚುಗಳು()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& oedge : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{graphp, iedge.fromp(), oedge.ತುದಿ(),
                            std::min(iedge.ತೂಕ_2(), oedge.ತೂಕ_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_ಅಂಚುಗಳು(graphp);
}

// cppcheck-has-bug-suppress constParameter
ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಶೃಂಗ::v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ಆದರೆ (debug()) nsstr << "\n-vertex: " << ಇದು << '\n';
    ಆದರೆ (File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = fileline()) {
        flp->v3error_ಕೊನೆ(nsstr);
    } ಇಲ್ಲದಿದ್ದರೆ {
        V3Error::v3error_ಕೊನೆ(nsstr, "", ಶೂನ್ಯಸೂಚಕ);
    }
}
ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಶೃಂಗ::v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ಕೊನೆ(str);
    ಪ್ರತಿಜ್ಞಾ(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    ಆದರೆ (vertexp->rank()) os << " r" << vertexp->rank();
    ಆದರೆ (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    ಆದರೆ (vertexp->color()) os << " c" << vertexp->color();
    ಹಿಂತಿರುಗಿ os;
}

//######################################################################
//######################################################################
//ಅಂಚುಗಳು

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಅಂಚು::init(V3_ನಕ್ಷೆ* /*graphp*/, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2,
                       ತಾರ್ಕಿಕ cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(ತುದಿ, "Null to pointer");
    m_fromp = fromp;
    m_ತುದಿ = ತುದಿ;
    m_ತೂಕ = ತೂಕ_2;
    m_cutable = cutable;
    m_userp = ಶೂನ್ಯಸೂಚಕ;
    // Link vertices to this edge
    m_fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().link_ಹಿಂಭಾಗ(ಇದು);
    m_ತುದಿ->ಒಳ_ಅಂಚುಗಳು().link_ಹಿಂಭಾಗ(ಇದು);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಅಂಚು::relinkFromp(V3_ನಕ್ಷೆ_ಶೃಂಗ* newFromp) {
    m_fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlink(ಇದು);
    m_fromp = newFromp;
    m_fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().link_ಹಿಂಭಾಗ(ಇದು);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಅಂಚು::relink_ತುದಿ(V3_ನಕ್ಷೆ_ಶೃಂಗ* new_ತುದಿ) {
    m_ತುದಿ->ಒಳ_ಅಂಚುಗಳು().unlink(ಇದು);
    m_ತುದಿ = new_ತುದಿ;
    m_ತುದಿ->ಒಳ_ಅಂಚುಗಳು().link_ಹಿಂಭಾಗ(ಇದು);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ_ಅಂಚು::unlinkDelete() {
    // Unlink from side
    m_fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlink(ಇದು);
    // Unlink to side
    m_ತುದಿ->ಒಳ_ಅಂಚುಗಳು().unlink(ಇದು);
    // Delete
    ಅಳಿಸು ಇದು;
}

std::string V3_ನಕ್ಷೆ_ಅಂಚು::name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fromp->name() + "->" + m_ತುದಿ->name(); }

ಪೂರ್ಣಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು::ವಿಂಗಡಿಸು_Cmp(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* rhsp) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_ತೂಕ || !rhsp->m_ತೂಕ) ಹಿಂತಿರುಗಿ 0;
    ಹಿಂತಿರುಗಿ ತುದಿ()->ವಿಂಗಡಿಸು_Cmp(rhsp->ತುದಿ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_ನಕ್ಷೆ::V3_ನಕ್ಷೆ() {}

V3_ನಕ್ಷೆ::~V3_ನಕ್ಷೆ() { clear(); }

ಶೂನ್ಯ V3_ನಕ್ಷೆ::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkFront()) {
            VL_DO_DANGLING(ಅಳಿಸು edgep, edgep);
        }
    }
    // Delete the old vertices
    ಆಗ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(ಅಳಿಸು vertexp, vertexp);
    }
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಶೃಂಗ.user(0);
        ಶೃಂಗ.userp(ಶೂನ್ಯಸೂಚಕ);  // It's a union, but might be different size than user()
    }
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::user_Clear_ಅಂಚುಗಳು() {
    // Clear user() in all of tree
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಅಂಚು.user(0);
            ಅಂಚು.userp(ಶೂನ್ಯಸೂಚಕ);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

ಶೂನ್ಯ V3_ನಕ್ಷೆ::loops_ಸಂದೇಶ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(ಅಂಚು_Funcp, vertexp));
}
string V3_ನಕ್ಷೆ::loops_ಶೃಂಗ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    ಆದರೆ (debug())
        ಹಿಂತಿರುಗಿ "-Info-Loop: "s + cvt_ಗಮ್ಯ_Hex(vertexp) + ' ' + cvt_ಗಮ್ಯ_Str(vertexp) + '\n';
    ಇಲ್ಲದಿದ್ದರೆ
        ಹಿಂತಿರುಗಿ "";
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::dump(std::ostream& os) ಸ್ಥಿರಾಂಕ {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        os << "\tNode: " << ಶೃಂಗ.name();
        ಆದರೆ (ಶೃಂಗ.color()) os << "  color=" << ಶೃಂಗ.color();
        os << '\n';
        // Print edges
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಒಳ_ಅಂಚುಗಳು()) dump_ಅಂಚು(os, ಶೃಂಗ, ಅಂಚು);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) dump_ಅಂಚು(os, ಶೃಂಗ, ಅಂಚು);
    }
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::dump_ಅಂಚು(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ,
                       ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (ಅಂಚು.ತೂಕ_2() && (ಅಂಚು.fromp() == &ಶೃಂಗ || ಅಂಚು.ತುದಿ() == &ಶೃಂಗ)) {
        os << "\t\t";
        ಆದರೆ (ಅಂಚು.fromp() == &ಶೃಂಗ) os << "-> " << ಅಂಚು.ತುದಿ()->name();
        ಆದರೆ (ಅಂಚು.ತುದಿ() == &ಶೃಂಗ) os << "<- " << ಅಂಚು.fromp()->name();
        ಆದರೆ (ಅಂಚು.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
ಶೂನ್ಯ V3_ನಕ್ಷೆ::dump_ಅಂಚುಗಳು(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ) ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಒಳ_ಅಂಚುಗಳು()) dump_ಅಂಚು(os, ಶೃಂಗ, ಅಂಚು);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) dump_ಅಂಚು(os, ಶೃಂಗ, ಅಂಚು);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::dumpDotFilePrefixed(ಸ್ಥಿರಾಂಕ string& nameComment, ತಾರ್ಕಿಕ colorAsSubgraph) ಸ್ಥಿರಾಂಕ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
ಶೂನ್ಯ V3_ನಕ್ಷೆ::dumpDotFilePrefixedAlways(ಸ್ಥಿರಾಂಕ string& nameComment, ತಾರ್ಕಿಕ colorAsSubgraph) ಸ್ಥಿರಾಂಕ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::dumpDotFile(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ colorAsSubgraph) ಸ್ಥಿರಾಂಕ {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ಆದರೆ (logp->fail()) v3fatal("Can't write file: " << filename);

    //ಶೀರ್ಷಿಕೆ
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_ಗಣ<std::string> ranks;
    std::unordered_multimap<std::string, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> rankSets;
    std::multimap<std::string, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> subgraphs;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಸ್ಥಿರಾಂಕ string ಶೃಂಗ_Subgraph
            = (colorAsSubgraph && ಶೃಂಗ.color()) ? cvt_ಗಮ್ಯ_Str(ಶೃಂಗ.color()) : "";
        subgraphs.emplace(ಶೃಂಗ_Subgraph, &ಶೃಂಗ);
        ಸ್ಥಿರಾಂಕ string& dotRank = ಶೃಂಗ.dotRank();
        ಆದರೆ (!dotRank.ಖಾಲಿಗಳು()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &ಶೃಂಗ);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*, ಪೂರ್ಣಾಂಕ> ಅಂಕೆ_Map;

    // Print vertices
    ಪೂರ್ಣಾಂಕ n = 0;
    string subgr;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ಸ್ಥಿರಾಂಕ string ಶೃಂಗ_Subgraph = it->ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp = it->ಎರಡನೇ;
        ಅಂಕೆ_Map[vertexp] = n;
        ಆದರೆ (subgr != ಶೃಂಗ_Subgraph) {
            ಆದರೆ (subgr != "") *logp << "\t};\n";
            subgr = ಶೃಂಗ_Subgraph;
            ಆದರೆ (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        ಆದರೆ (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        ಆದರೆ (vertexp->rank()) *logp << " r" << vertexp->rank();
        ಆದರೆ (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        ಆದರೆ (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        ಆದರೆ (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        ಆದರೆ (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    ಆದರೆ (subgr != "") *logp << "\t};\n";

    // Print edges
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ : vertices()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಶೃಂಗ.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (ಅಂಚು.ತೂಕ_2()) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fromVnum = ಅಂಕೆ_Map[ಅಂಚು.fromp()];
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಗಮ್ಯ_Vnum = ಅಂಕೆ_Map[ಅಂಚು.ತುದಿ()];
                *logp << "\tn" << ಅಂಚು.fromp()->dotName() << fromVnum << " -> n"
                      << ಅಂಚು.ತುದಿ()->dotName() << ಗಮ್ಯ_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (ಅಂಚು.dotLabel() != "" ? ಅಂಚು.dotLabel() : "")
                      << "\"" << " weight=" << ಅಂಚು.ತೂಕ_2() << " color=" << ಅಂಚು.dotColor();
                ಆದರೆ (ಅಂಚು.dotStyle() != "") *logp << " style=" << ಅಂಚು.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dotRank : ranks) {
        *logp << "\t{ rank=";
        ಆದರೆ (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } ಇಲ್ಲದಿದ್ದರೆ {
            *logp << dotRank;
        }
        *logp << "; ";
        ಸ್ವಯಂಚಾಲಿತ bounds = rankSets.equal_range(dotRank);
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it{bounds.ಮೊದಲ}; it != bounds.ಎರಡನೇ; ++it) {
            ಆದರೆ (it != bounds.ಮೊದಲ) *logp << ", ";
            *logp << 'n' << ಅಂಕೆ_Map[it->ಎರಡನೇ] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    ಹೊರಗೆ << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

ಶೂನ್ಯ V3_ನಕ್ಷೆ::hash_ನಕ್ಷೆ_Debug(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* debugName) ಸ್ಥಿರಾಂಕ {
    // Disabled when there are no nondeterminism issues in flight.
    ಆದರೆ (!v3Global.opt.debugNondeterminism()) ಹಿಂತಿರುಗಿ;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*, uint32_t> vx2Id;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) hash += vx2Id[ಅಂಚು.ತುದಿ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_ಗಮ್ಯ_Hex(hash.ಮೌಲ್ಯ()));
}
