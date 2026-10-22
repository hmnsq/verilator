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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Vertices

V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& old)
    : m_fanout{old.m_fanout}
    , m_color{old.m_color}
    , m_rank{old.m_rank} {
    m_userp = ਖਾਲੀ_ਸੂਚਕ;
    graphp->vertices().link_ਪਿੱਠ(ਇਹ);
}

V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
    : m_fanout{0}
    , m_color{0}
    , m_rank{0} {
    m_userp = ਖਾਲੀ_ਸੂਚਕ;
    graphp->vertices().link_ਪਿੱਠ(ਇਹ);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::unlink_ਕਿਨਾਰੇ(V3_ਗ੍ਰਾਫ*) {
    ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ep = m_outs.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
    ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ep = m_ins.frontp()) VL_DO_DANGLING(ep->unlinkDelete(), ep);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::unlinkDelete(V3_ਗ੍ਰਾਫ* graphp) {
    // Delete edges
    unlink_ਕਿਨਾਰੇ(graphp);
    // Unlink from vertex list
    graphp->m_vertices.unlink(ਇਹ);
    // Delete
    ਮਿਟਾਓ ਇਹ;  // this=nullptr;
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::reroute_ਕਿਨਾਰੇ(V3_ਗ੍ਰਾਫ* graphp) {
    // Make new edges for each from/to pair
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& iedge : ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& oedge : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, iedge.fromp(), oedge.ਸਿਖਰ(),
                            std::min(iedge.ਭਾਰ_2(), oedge.ਭਾਰ_2()),
                            iedge.cutable() && oedge.cutable()};
        }
    }
    // Remove old edges
    unlink_ਕਿਨਾਰੇ(graphp);
}

// cppcheck-has-bug-suppress constParameter
ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& str) ਅਚਲ  // LCOV_EXCL_START
    VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ਜੇ (debug()) nsstr << "\n-vertex: " << ਇਹ << '\n';
    ਜੇ (File_ਸਤਰ* ਅਚਲ flp = fileline()) {
        flp->v3error_ਸਿਰਾ(nsstr);
    } ਨਹੀਂਤਾਂ {
        V3Error::v3error_ਸਿਰਾ(nsstr, "", ਖਾਲੀ_ਸੂਚਕ);
    }
}
ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& str) ਅਚਲ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ਸਿਰਾ(str);
    ਪ੍ਰਤਿਜ੍ਞਾ(0);
    VL_UNREACHABLE;
}  // LCOV_EXCL_STOP

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
    os << "  VERTEX=" << vertexp->name();
    ਜੇ (vertexp->rank()) os << " r" << vertexp->rank();
    ਜੇ (vertexp->fanout() != 0.0) os << " f" << vertexp->fanout();
    ਜੇ (vertexp->color()) os << " c" << vertexp->color();
    ਵਾਪਸ os;
}

//######################################################################
//######################################################################
//ਕਿਨਾਰੇ

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::init(V3_ਗ੍ਰਾਫ* /*graphp*/, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2,
                       ਤਾਰਕਿਕ cutable) {
    UASSERT(fromp, "Null from pointer");
    UASSERT(ਸਿਖਰ, "Null to pointer");
    m_fromp = fromp;
    m_ਸਿਖਰ = ਸਿਖਰ;
    m_ਭਾਰ = ਭਾਰ_2;
    m_cutable = cutable;
    m_userp = ਖਾਲੀ_ਸੂਚਕ;
    // Link vertices to this edge
    m_fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().link_ਪਿੱਠ(ਇਹ);
    m_ਸਿਖਰ->ਵਿੱਚ_ਕਿਨਾਰੇ().link_ਪਿੱਠ(ਇਹ);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::relinkFromp(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* newFromp) {
    m_fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlink(ਇਹ);
    m_fromp = newFromp;
    m_fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().link_ਪਿੱਠ(ਇਹ);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::relink_ਸਿਖਰ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* new_ਸਿਖਰ) {
    m_ਸਿਖਰ->ਵਿੱਚ_ਕਿਨਾਰੇ().unlink(ਇਹ);
    m_ਸਿਖਰ = new_ਸਿਖਰ;
    m_ਸਿਖਰ->ਵਿੱਚ_ਕਿਨਾਰੇ().link_ਪਿੱਠ(ਇਹ);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::unlinkDelete() {
    // Unlink from side
    m_fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlink(ਇਹ);
    // Unlink to side
    m_ਸਿਖਰ->ਵਿੱਚ_ਕਿਨਾਰੇ().unlink(ਇਹ);
    // Delete
    ਮਿਟਾਓ ਇਹ;
}

std::string V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::name() ਅਚਲ { ਵਾਪਸ m_fromp->name() + "->" + m_ਸਿਖਰ->name(); }

ਪੂਰਨ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::ਛਾਂਟੋ_Cmp(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* rhsp) ਅਚਲ {
    ਜੇ (!m_ਭਾਰ || !rhsp->m_ਭਾਰ) ਵਾਪਸ 0;
    ਵਾਪਸ ਸਿਖਰ()->ਛਾਂਟੋ_Cmp(rhsp->ਸਿਖਰ());
}

//######################################################################
//######################################################################
// Graph top level
// Constructors

V3_ਗ੍ਰਾਫ::V3_ਗ੍ਰਾਫ() {}

V3_ਗ੍ਰਾਫ::~V3_ਗ੍ਰਾਫ() { clear(); }

ਖਾਲੀ V3_ਗ੍ਰਾਫ::clear() {
    // Empty it of all points, as if making a new object
    // Delete the old edges
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep = ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkFront()) {
            VL_DO_DANGLING(ਮਿਟਾਓ edgep, edgep);
        }
    }
    // Delete the old vertices
    ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = vertices().unlinkFront()) {
        VL_DO_DANGLING(ਮਿਟਾਓ vertexp, vertexp);
    }
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::userClearVertices() {
    // Clear user() in all of tree
    // We may use the userCnt trick in V3Ast later... (but gblCnt would be
    // in V3Graph instead of static - which has the complication of finding
    // the graph pointer given a vertex.)  For now, we don't call this often, and
    // the extra code on each read of user() would probably slow things
    // down more than help.
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਸ਼ੀਰਸ਼.user(0);
        ਸ਼ੀਰਸ਼.userp(ਖਾਲੀ_ਸੂਚਕ);  // It's a union, but might be different size than user()
    }
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::user_Clear_ਕਿਨਾਰੇ() {
    // Clear user() in all of tree
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਕਿਨਾਰਾ.user(0);
            ਕਿਨਾਰਾ.userp(ਖਾਲੀ_ਸੂਚਕ);  // It's a union, but might be different size than user()
        }
    }
}

//======================================================================
// Dumping

ਖਾਲੀ V3_ਗ੍ਰਾਫ::loops_ਸੁਨੇਹਾ_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) {
    vertexp->v3fatalSrc("Loops detected in graph: " << vertexp << "\n"
                                                    << reportLoops(ਕਿਨਾਰਾ_Funcp, vertexp));
}
string V3_ਗ੍ਰਾਫ::loops_ਸ਼ੀਰਸ਼_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
    // Needed here as V3GraphVertex<< isn't defined until later in header
    ਜੇ (debug())
        ਵਾਪਸ "-Info-Loop: "s + cvt_ਮੰਜ਼ਿਲ_Hex(vertexp) + ' ' + cvt_ਮੰਜ਼ਿਲ_Str(vertexp) + '\n';
    ਨਹੀਂਤਾਂ
        ਵਾਪਸ "";
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::dump(std::ostream& os) ਅਚਲ {
    // This generates a file used by graphviz, https://www.graphviz.org
    os << " Graph:\n";
    // Print vertices
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        os << "\tNode: " << ਸ਼ੀਰਸ਼.name();
        ਜੇ (ਸ਼ੀਰਸ਼.color()) os << "  color=" << ਸ਼ੀਰਸ਼.color();
        os << '\n';
        // Print edges
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਵਿੱਚ_ਕਿਨਾਰੇ()) dump_ਕਿਨਾਰਾ(os, ਸ਼ੀਰਸ਼, ਕਿਨਾਰਾ);
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) dump_ਕਿਨਾਰਾ(os, ਸ਼ੀਰਸ਼, ਕਿਨਾਰਾ);
    }
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::dump_ਕਿਨਾਰਾ(std::ostream& os, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼,
                       ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ) ਅਚਲ {
    ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() && (ਕਿਨਾਰਾ.fromp() == &ਸ਼ੀਰਸ਼ || ਕਿਨਾਰਾ.ਸਿਖਰ() == &ਸ਼ੀਰਸ਼)) {
        os << "\t\t";
        ਜੇ (ਕਿਨਾਰਾ.fromp() == &ਸ਼ੀਰਸ਼) os << "-> " << ਕਿਨਾਰਾ.ਸਿਖਰ()->name();
        ਜੇ (ਕਿਨਾਰਾ.ਸਿਖਰ() == &ਸ਼ੀਰਸ਼) os << "<- " << ਕਿਨਾਰਾ.fromp()->name();
        ਜੇ (ਕਿਨਾਰਾ.cutable()) os << "  [CUTABLE]";
        os << '\n';
    }
}
ਖਾਲੀ V3_ਗ੍ਰਾਫ::dump_ਕਿਨਾਰੇ(std::ostream& os, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼) ਅਚਲ {
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਵਿੱਚ_ਕਿਨਾਰੇ()) dump_ਕਿਨਾਰਾ(os, ਸ਼ੀਰਸ਼, ਕਿਨਾਰਾ);
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) dump_ਕਿਨਾਰਾ(os, ਸ਼ੀਰਸ਼, ਕਿਨਾਰਾ);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::dumpDotFilePrefixed(ਅਚਲ string& nameComment, ਤਾਰਕਿਕ colorAsSubgraph) ਅਚਲ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

//! Variant of dumpDotFilePrefixed without --dump option check
ਖਾਲੀ V3_ਗ੍ਰਾਫ::dumpDotFilePrefixedAlways(ਅਚਲ string& nameComment, ਤਾਰਕਿਕ colorAsSubgraph) ਅਚਲ {
    dumpDotFile(v3Global.debugFilename(nameComment) + ".dot", colorAsSubgraph);
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::dumpDotFile(ਅਚਲ string& filename, ਤਾਰਕਿਕ colorAsSubgraph) ਅਚਲ {
    // This generates a file used by graphviz, https://www.graphviz.org
    // "hardcoded" parameters:
    ਅਚਲ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
    ਜੇ (logp->fail()) v3fatal("Can't write file: " << filename);

    //ਸਿਰਲੇਖ
    *logp << "digraph v3graph {\n";
    *logp << "\tgraph\t[label=\"" << filename << "\",\n";
    *logp << "\t\t labelloc=t, labeljust=l,\n";
    *logp << "\t\t //size=\"7.5,10\",\n";
    *logp << "\t\t rankdir=" << dotRankDir() << "];\n";

    // List of all possible subgraphs
    // Collections of explicit ranks
    std::unordered_ਸੈੱਟ<std::string> ranks;
    std::unordered_multimap<std::string, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> rankSets;
    std::multimap<std::string, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> subgraphs;
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਅਚਲ string ਸ਼ੀਰਸ਼_Subgraph
            = (colorAsSubgraph && ਸ਼ੀਰਸ਼.color()) ? cvt_ਮੰਜ਼ਿਲ_Str(ਸ਼ੀਰਸ਼.color()) : "";
        subgraphs.emplace(ਸ਼ੀਰਸ਼_Subgraph, &ਸ਼ੀਰਸ਼);
        ਅਚਲ string& dotRank = ਸ਼ੀਰਸ਼.dotRank();
        ਜੇ (!dotRank.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ranks.emplace(dotRank);
            rankSets.emplace(dotRank, &ਸ਼ੀਰਸ਼);
        }
    }

    // We use a map here, as we don't want to corrupt anything (userp) in the graph,
    // and we don't care if this is slow.
    std::unordered_map<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*, ਪੂਰਨ> ਅੰਕ_Map;

    // Print vertices
    ਪੂਰਨ n = 0;
    string subgr;
    ਲਈ (ਸਵੈਚਾਲਿਤ it = subgraphs.cbegin(); it != subgraphs.cend(); ++it) {
        ਅਚਲ string ਸ਼ੀਰਸ਼_Subgraph = it->ਪਹਿਲਾ;
        ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp = it->ਦੂਜਾ;
        ਅੰਕ_Map[vertexp] = n;
        ਜੇ (subgr != ਸ਼ੀਰਸ਼_Subgraph) {
            ਜੇ (subgr != "") *logp << "\t};\n";
            subgr = ਸ਼ੀਰਸ਼_Subgraph;
            ਜੇ (subgr != "") {
                *logp << "\tsubgraph cluster_" << subgr << " {\n";
                *logp << "\tlabel=\"" << subgr << "\"\n";
            }
        }
        ਜੇ (subgr != "") *logp << "\t";
        *logp << "\tn" << vertexp->dotName() << (n++) << "\t[fontsize=8 " << "label=\""
              << (vertexp->name() != "" ? vertexp->name() : "\\N");
        ਜੇ (vertexp->rank()) *logp << " r" << vertexp->rank();
        ਜੇ (vertexp->fanout() != 0.0) *logp << " f" << vertexp->fanout();
        ਜੇ (vertexp->color()) *logp << "\\n c" << vertexp->color();
        *logp << "\"";
        *logp << ", color=" << vertexp->dotColor();
        ਜੇ (vertexp->dotStyle() != "") *logp << ", style=" << vertexp->dotStyle();
        ਜੇ (vertexp->dotShape() != "") *logp << ", shape=" << vertexp->dotShape();
        *logp << "];\n";
    }
    ਜੇ (subgr != "") *logp << "\t};\n";

    // Print edges
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼ : vertices()) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਸ਼ੀਰਸ਼.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2()) {
                ਅਚਲ ਪੂਰਨ fromVnum = ਅੰਕ_Map[ਕਿਨਾਰਾ.fromp()];
                ਅਚਲ ਪੂਰਨ ਮੰਜ਼ਿਲ_Vnum = ਅੰਕ_Map[ਕਿਨਾਰਾ.ਸਿਖਰ()];
                *logp << "\tn" << ਕਿਨਾਰਾ.fromp()->dotName() << fromVnum << " -> n"
                      << ਕਿਨਾਰਾ.ਸਿਖਰ()->dotName() << ਮੰਜ਼ਿਲ_Vnum
                      << " ["
                      // <<"fontsize=8 label=\""<<(edge.name()!="" ? edge.name() : "\\E")<<"\""
                      << "fontsize=8 label=\"" << (ਕਿਨਾਰਾ.dotLabel() != "" ? ਕਿਨਾਰਾ.dotLabel() : "")
                      << "\"" << " weight=" << ਕਿਨਾਰਾ.ਭਾਰ_2() << " color=" << ਕਿਨਾਰਾ.dotColor();
                ਜੇ (ਕਿਨਾਰਾ.dotStyle() != "") *logp << " style=" << ਕਿਨਾਰਾ.dotStyle();
                // if (edge.cutable()) *logp << ",constraint=false";  // to rank without
                // following edges
                *logp << "];\n";
            }
        }
    }

    // Print ranks
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dotRank : ranks) {
        *logp << "\t{ rank=";
        ਜੇ (dotRank != "sink" && dotRank != "source" && dotRank != "min" && dotRank != "max") {
            *logp << "same";
        } ਨਹੀਂਤਾਂ {
            *logp << dotRank;
        }
        *logp << "; ";
        ਸਵੈਚਾਲਿਤ bounds = rankSets.equal_range(dotRank);
        ਲਈ (ਸਵੈਚਾਲਿਤ it{bounds.ਪਹਿਲਾ}; it != bounds.ਦੂਜਾ; ++it) {
            ਜੇ (it != bounds.ਪਹਿਲਾ) *logp << ", ";
            *logp << 'n' << ਅੰਕ_Map[it->ਦੂਜਾ] << "";
        }
        *logp << " }\n";
    }

    // Vertex::m_user end, now unused

    // Trailer
    *logp << "}\n";
    logp->close();

    ਬਾਹਰ << "dot -Tpdf -o ~/a.pdf " << filename << "\n";
}

ਖਾਲੀ V3_ਗ੍ਰਾਫ::hash_ਗ੍ਰਾਫ_Debug(ਅਚਲ ਅੱਖਰ* debugName) ਅਚਲ {
    // Disabled when there are no nondeterminism issues in flight.
    ਜੇ (!v3Global.opt.debugNondeterminism()) ਵਾਪਸ;

    // Assign a unique ID to each vertex for pointer stability, then hash
    uint32_t id = 1;
    std::unordered_map<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*, uint32_t> vx2Id;
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) vx2Id[&vtx] = ++id;
    V3Hash hash;
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) hash += vx2Id[ਕਿਨਾਰਾ.ਸਿਖਰ()];
    }
    UINFO(0, "Hash of shape (not contents) of " << debugName << " = " << cvt_ਮੰਜ਼ਿਲ_Hex(hash.ਮੁੱਲ()));
}
