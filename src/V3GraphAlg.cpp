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

#ضمن "V3GraphAlg.h"

#ضمن "V3Global.h"
#ضمن "V3GraphPathChecker.h"
#ضمن "V3GraphStream.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <limits>
#ضمن <list>
#ضمن <map>
#ضمن <numeric>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

صنف گراف_Remove_Redundant final : گراف_Alg<> {
    ثابت منطقي m_جمع_Weights;  // Sum, rather then maximize weights
خاص:
    فراغ اصلی() {
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) رأس_Iterate(&رأس);
    }
    فراغ رأس_Iterate(V3_گراف_رأس* vertexp) {
        // Clear marks
        لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) یال.بالاترین()->userp(اشاره_تهی);

        // Mark edges and detect duplications

        لكل (V3_گراف_یال* ثابت edgep : vertexp->بیرون_یالها().unlinkable()) {
            إذا (follow_یال(edgep)) {
                V3_گراف_رأس* بیرون_Vertexp = edgep->بالاترین();
                V3_گراف_یال* prevEdgep = static_cast<V3_گراف_یال*>(بیرون_Vertexp->userp());
                إذا (!prevEdgep) {  // No previous assignment
                    بیرون_Vertexp->userp(edgep);
                } وإلا {  // Duplicate
                    منطقي saveOld = درست;
                    إذا (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = خطأ;  // new !cutable more important than old
                    } وإلا إذا (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = درست;  // old !cutable more important than new
                    } وإلا {
                        saveOld = درست;
                        إذا (!m_جمع_Weights
                            && (prevEdgep->وزن_2() < edgep->وزن_2())) {  // Keep max weight
                            prevEdgep->وزن_2(edgep->وزن_2());
                        }
                    }
                    إذا (saveOld) {
                        إذا (m_جمع_Weights) prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وإلا {
                        إذا (m_جمع_Weights) edgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        بیرون_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

عام:
    گراف_Remove_Redundant(V3_گراف* graphp, V3_یال_Func_P یال_Funcp, منطقي جمع_Weights)
        : گراف_Alg<>{graphp, یال_Funcp}
        , m_جمع_Weights{جمع_Weights} {
        اصلی();
    }
    ~گراف_Remove_Redundant() = افتراضي;
};

فراغ V3_گراف::remove_Redundant_یالها_Max(V3_یال_Func_P یال_Funcp) {
    گراف_Remove_Redundant{هذا, یال_Funcp, خطأ};
}
فراغ V3_گراف::remove_Redundant_یالها_جمع(V3_یال_Func_P یال_Funcp) {
    گراف_Remove_Redundant{هذا, یال_Funcp, درست};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

صنف گراف_Alg_Remove_متعدی_یالها final : گراف_Alg<> {
عام:
    صریح گراف_Alg_Remove_متعدی_یالها(V3_گراف* graphp)
        : گراف_Alg<>(graphp, اشاره_تهی) {}
    فراغ go() {
        گراف_مسیر_Checker checker{m_graphp};
        لكل (V3_گراف_رأس& vtx : m_graphp->vertices()) {
            V3_گراف_یال* deletep = اشاره_تهی;
            لكل (V3_گراف_یال& یال : vtx.بیرون_یالها()) {
                إذا (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = اشاره_تهی);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                إذا (checker.is_متعدی_یال(&یال)) deletep = &یال;
            }
            إذا (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خاص:
    VL_UNCOPYABLE(گراف_Alg_Remove_متعدی_یالها);
};

فراغ V3_گراف::remove_متعدی_یالها() { گراف_Alg_Remove_متعدی_یالها{هذا}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

صنف گراف_Alg_Weakly final : گراف_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساكن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_رقم_Colors = 0;  // Number of colors assigned

    فراغ اصلی() {
        // Initialize state
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) رأس.color(UNCOLORED);
        // Color graph, without gaps
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            إذا (رأس.color() == UNCOLORED) رأس_Iterate(&رأس, m_رقم_Colors++);
        }
    }

    فراغ رأس_Iterate(V3_گراف_رأس* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        إذا (vertexp->color() != UNCOLORED) أرجع;  // Already colored it
        vertexp->color(currentColor);
        لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
            إذا (follow_یال(&یال)) رأس_Iterate(یال.بالاترین(), currentColor);
        }
        لكل (V3_گراف_یال& یال : vertexp->درون_یالها()) {
            إذا (follow_یال(&یال)) رأس_Iterate(یال.fromp(), currentColor);
        }
    }

عام:
    گراف_Alg_Weakly(V3_گراف* graphp, V3_یال_Func_P یال_Funcp)
        : گراف_Alg<>(graphp, یال_Funcp) {
        اصلی();
    }
    ~گراف_Alg_Weakly() = افتراضي;

    uint32_t رقم_Colors() ثابت { أرجع m_رقم_Colors; }
};

uint32_t V3_گراف::weaklyConnected(V3_یال_Func_P یال_Funcp) {
    أرجع گراف_Alg_Weakly{هذا, یال_Funcp}.رقم_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

صنف گراف_Alg_Strongly final : گراف_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::وکتور<V3_گراف_رأس*> m_callTrace;  // List of everything we hit processing so far

    فراغ اصلی() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            رأس.color(0);
            رأس.user(0);
        }
        // Color graph
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            إذا (!رأس.user()) {
                m_currentDfs++;
                رأس_Iterate(&رأس);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            منطقي onecolor = درست;
            لكل (V3_گراف_یال& یال : رأس.بیرون_یالها()) {
                إذا (follow_یال(&یال)) {
                    إذا (رأس.color() == یال.بالاترین()->color()) {
                        onecolor = خطأ;
                        اكسر;
                    }
                }
            }
            إذا (onecolor) رأس.color(0);
        }
    }

    فراغ رأس_Iterate(V3_گراف_رأس* vertexp) {
        ثابت uint32_t this_Dfs_رقم = m_currentDfs++;
        vertexp->user(this_Dfs_رقم);
        vertexp->color(0);
        لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
            إذا (follow_یال(&یال)) {
                V3_گراف_رأس* بالاترین = یال.بالاترین();
                إذا (!بالاترین->user()) {  // Dest not computed yet
                    رأس_Iterate(بالاترین);
                }
                إذا (!بالاترین->color()) {  // Dest not in a component
                    إذا (vertexp->user() > بالاترین->user()) vertexp->user(بالاترین->user());
                }
            }
        }
        إذا (vertexp->user() == this_Dfs_رقم) {  // New head of subtree
            vertexp->color(this_Dfs_رقم);  // Mark as component
            طالما (!m_callTrace.خالی()) {
                V3_گراف_رأس* برداشت_Vertexp = m_callTrace.پشت();
                إذا (برداشت_Vertexp->user() >= this_Dfs_رقم) {  // Lower node is part of this subtree
                    m_callTrace.برداشت_پشت();
                    برداشت_Vertexp->color(this_Dfs_رقم);
                } وإلا {
                    اكسر;
                }
            }
        } وإلا {  // In another subtree (maybe...)
            m_callTrace.افزودن_پشت(vertexp);
        }
    }

عام:
    گراف_Alg_Strongly(V3_گراف* graphp, V3_یال_Func_P یال_Funcp)
        : گراف_Alg<>{graphp, یال_Funcp} {
        اصلی();
    }
    ~گراف_Alg_Strongly() = افتراضي;
};

فراغ V3_گراف::stronglyConnected(V3_یال_Func_P یال_Funcp) { گراف_Alg_Strongly{هذا, یال_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

صنف گراف_Alg_Rank final : گراف_Alg<> {
    فراغ اصلی() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            رأس.rank(0);
            رأس.user(0);
        }
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            إذا (!رأس.user()) رأس_Iterate(&رأس, 1);
        }
    }
    فراغ رأس_Iterate(V3_گراف_رأس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پیام_Cb(vertexp, m_یال_Funcp);
            أرجع;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        إذا (vertexp->rank() >= currentRank) أرجع;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
            إذا (follow_یال(&یال)) رأس_Iterate(یال.بالاترین(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_Rank(V3_گراف* graphp, V3_یال_Func_P یال_Funcp)
        : گراف_Alg<>{graphp, یال_Funcp} {
        اصلی();
    }
    ~گراف_Alg_Rank() = افتراضي;
};

فراغ V3_گراف::rank() { rank(&V3_گراف_یال::followAlwaysTrue); }
فراغ V3_گراف::rank(V3_یال_Func_P یال_Funcp) { گراف_Alg_Rank{هذا, یال_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

صنف گراف_Alg_Rank_Min final : گراف_Alg<> {
    فراغ اصلی() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            رأس.rank(0);
            رأس.user(0);
        }
        لكل (V3_گراف_رأس& رأس : m_graphp->vertices()) {
            إذا (!رأس.user()) رأس_Iterate(&رأس);
        }
    }
    uint32_t رأس_Iterate(V3_گراف_رأس* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پیام_Cb(vertexp, m_یال_Funcp);
            أرجع vertexp->rank();
        }
        إذا (vertexp->user()) أرجع vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        لكل (V3_گراف_یال& یال : vertexp->درون_یالها()) {
            إذا (follow_یال(&یال)) {
                ثابت uint32_t nrank = رأس_Iterate(یال.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        إذا (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        أرجع vertexp->rank();
    }

عام:
    گراف_Alg_Rank_Min(V3_گراف* graphp, V3_یال_Func_P یال_Funcp)
        : گراف_Alg<>{graphp, یال_Funcp} {
        اصلی();
    }
    ~گراف_Alg_Rank_Min() = افتراضي;
};

فراغ V3_گراف::rankMin() { rankMin(&V3_گراف_یال::followAlwaysTrue); }
فراغ V3_گراف::rankMin(V3_یال_Func_P یال_Funcp) { گراف_Alg_Rank_Min{هذا, یال_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

صنف گراف_Alg_RLoops final : گراف_Alg<> {
    std::وکتور<V3_گراف_رأس*> m_callTrace;  // List of everything we hit processing so far
    std::وکتور<string> m_msgs;  // Output messages
    منطقي m_انجامشده = خطأ;  // Exit algorithm

    فراغ اصلی(V3_گراف_رأس* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        رأس_Iterate(vertexp, 0);
    }

    فراغ رأس_Iterate(V3_گراف_رأس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        إذا (m_انجامشده) أرجع;

        // Can't just reserve(), unless we modify size() before setting array directly
        طالما (m_callTrace.اندازه() <= currentRank) m_callTrace.افزودن_پشت(vertexp);
        m_callTrace[currentRank++] = vertexp;

        إذا (vertexp->user() == 1) {
            لكل (بیعلامت i = 0; i < currentRank; i++)
                m_msgs.emplace_پشت(m_graphp->loops_رأس_Cb(m_callTrace[i]));
            m_انجامشده = درست;
            أرجع;
        }
        إذا (vertexp->user() == 2) أرجع;  // Already processed it
        vertexp->user(1);
        لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
            إذا (follow_یال(&یال)) رأس_Iterate(یال.بالاترین(), currentRank);
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_RLoops(V3_گراف* graphp, V3_یال_Func_P یال_Funcp, V3_گراف_رأس* vertexp)
        : گراف_Alg<>{graphp, یال_Funcp} {
        اصلی(vertexp);
    }
    ~گراف_Alg_RLoops() = افتراضي;
    string پیام() ثابت {
        أرجع std::accumulate(m_msgs.begin(), m_msgs.انتها(), std::string{""});
    }
};

string V3_گراف::reportLoops(V3_یال_Func_P یال_Funcp, V3_گراف_رأس* vertexp) {
    أرجع گراف_Alg_RLoops{هذا, یال_Funcp, vertexp}.پیام();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

صنف گراف_Alg_Subtrees final : گراف_Alg<> {
    V3_گراف* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_گراف_رأس* رأس_Iterate_All(V3_گراف_رأس* vertexp) {
        إذا (V3_گراف_رأس* newVertexp = static_cast<V3_گراف_رأس*>(vertexp->userp())) {
            أرجع newVertexp;
        } وإلا {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
                إذا (follow_یال(&یال)) {
                    V3_گراف_یال* newEdgep = static_cast<V3_گراف_یال*>(یال.userp());
                    إذا (!newEdgep) {
                        V3_گراف_رأس* new_بالاترین = رأس_Iterate_All(یال.بالاترین());
                        newEdgep = یال.clone(m_loopGraphp, newVertexp, new_بالاترین);
                        یال.userp(newEdgep);
                    }
                }
            }
            أرجع newVertexp;
        }
    }

عام:
    گراف_Alg_Subtrees(V3_گراف* graphp, V3_گراف* loopGraphp, V3_یال_Func_P یال_Funcp,
                     V3_گراف_رأس* vertexp)
        : گراف_Alg<>{graphp, یال_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_یالها();
        (فراغ)رأس_Iterate_All(vertexp);
    }
    ~گراف_Alg_Subtrees() = افتراضي;
};

//! Report the entire connected graph with a loop or loops
فراغ V3_گراف::subtreeLoops(V3_یال_Func_P یال_Funcp, V3_گراف_رأس* vertexp, V3_گراف* loopGraphp) {
    گراف_Alg_Subtrees{هذا, loopGraphp, یال_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

فراغ V3_گراف::مرتب_سازی_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::وکتور<V3_گراف_رأس*> vertexps;
    لكل (V3_گراف_رأس& رأس : m_vertices) vertexps.افزودن_پشت(&رأس);
    std::stable_مرتب_سازی(vertexps.begin(), vertexps.انتها(),
                     [](ثابت V3_گراف_رأس* lhsp, ثابت V3_گراف_رأس* rhsp) {  //
                         أرجع lhsp->مرتب_سازی_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    لكل (V3_گراف_رأس* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_پشت(vertexp);
    }
}

فراغ V3_گراف::مرتب_سازی_یالها() {
    // Sort edges by rank then fanout of node they point to
    std::وکتور<V3_گراف_یال*> یالها;
    لكل (V3_گراف_رأس& رأس : vertices()) {
        // Make a vector
        لكل (V3_گراف_یال& یال : رأس.بیرون_یالها()) یالها.افزودن_پشت(&یال);
        //مرتب_سازی
        std::stable_مرتب_سازی(یالها.begin(), یالها.انتها(),
                         [](ثابت V3_گراف_یال* lhsp, ثابت V3_گراف_یال* rhsp) {  //
                             أرجع lhsp->مرتب_سازی_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        لكل (V3_گراف_یال* ثابت edgep : یالها) edgep->relinkFromp(&رأس);
        // Prep for next
        یالها.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

فراغ V3_گراف::ترتیب() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_گراف_یال::followAlwaysTrue);
    ترتیب_Pre_Ranked();
}

فراغ V3_گراف::ترتیب_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    لكل (V3_گراف_رأس& رأس : vertices()) {
        إذا (!رأس.user()) ترتیب_DFSIterate(&رأس);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    مرتب_سازی_Vertices();
    // Sort edges by rank then fanout of node they point to
    مرتب_سازی_یالها();
}

مزدوج V3_گراف::ترتیب_DFSIterate(V3_گراف_رأس* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    إذا (vertexp->user() == 2) أرجع vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    مزدوج fanout = 0;
    لكل (V3_گراف_یال& یال : vertexp->بیرون_یالها()) {
        إذا (یال.وزن_2()) fanout += ترتیب_DFSIterate(یال.بالاترین());
    }
    // Just count inbound edges
    لكل (V3_گراف_یال& یال : vertexp->درون_یالها()) {
        إذا (یال.وزن_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    أرجع vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

صنف گراف_Alg_Parallelism_Report final {
    // MEMBERS
    V3_گراف& m_گراف;  // The graph
    ثابت std::function<uint64_t(ثابت V3_گراف_رأس*)> m_رأس_هزینه;  // vertex cost function
    V3_گراف::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    صریح گراف_Alg_Parallelism_Report(V3_گراف& گراف,
                                       std::function<uint64_t(ثابت V3_گراف_رأس*)> رأس_هزینه)
        : m_گراف{گراف}
        , m_رأس_هزینه{رأس_هزینه} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3_گراف_رأس*, uint64_t> critPaths;
        گراف_جریان_Unordered serialize{&m_گراف};
        لكل (ثابت V3_گراف_رأس* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_رأس_شمار;
            uint64_t cp_هزینه_مقصد_اینجا = 0;
            لكل (ثابت V3_گراف_یال& یال : vertexp->درون_یالها()) {
                ++m_report.m_یال_شمار;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_هزینه_مقصد_اینجا = std::max(cp_هزینه_مقصد_اینجا, critPaths[یال.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_هزینه_مقصد_اینجا += m_رأس_هزینه(vertexp);
            critPaths[vertexp] = cp_هزینه_مقصد_اینجا;
            m_report.m_critical_مسیر_هزینه = std::max(m_report.m_critical_مسیر_هزینه, cp_هزینه_مقصد_اینجا);
            // Tally the total cost contributed by vertices.
            m_report.m_مجموع_گراف_هزینه += m_رأس_هزینه(vertexp);
        }
    }
    ~گراف_Alg_Parallelism_Report() = افتراضي;
    VL_UNCOPYABLE(گراف_Alg_Parallelism_Report);
    VL_UNMOVABLE(گراف_Alg_Parallelism_Report);

عام:
    ساكن V3_گراف::ParallelismReport
    apply(V3_گراف& گراف, std::function<uint32_t(ثابت V3_گراف_رأس*)> رأس_هزینه) {
        أرجع گراف_Alg_Parallelism_Report(گراف, رأس_هزینه).m_report;
    }
};

V3_گراف::ParallelismReport
V3_گراف::parallelismReport(std::function<uint64_t(ثابت V3_گراف_رأس*)> رأس_هزینه) {
    أرجع گراف_Alg_Parallelism_Report::apply(*هذا, رأس_هزینه);
}
