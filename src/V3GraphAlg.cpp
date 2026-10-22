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
    ثابت منطقي m_جوڙ_Weights;  // Sum, rather then maximize weights
خاص:
    فراغ مکيه() {
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) راس_Iterate(&راس);
    }
    فراغ راس_Iterate(V3_گراف_راس* vertexp) {
        // Clear marks
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) ڪنارو.چوٽي()->userp(خالي_اشارو);

        // Mark edges and detect duplications

        لكل (V3_گراف_ڪنارو* ثابت edgep : vertexp->ٻاهريون_ڪنارا().unlinkable()) {
            إذا (follow_ڪنارو(edgep)) {
                V3_گراف_راس* ٻاهريون_Vertexp = edgep->چوٽي();
                V3_گراف_ڪنارو* prevEdgep = static_cast<V3_گراف_ڪنارو*>(ٻاهريون_Vertexp->userp());
                إذا (!prevEdgep) {  // No previous assignment
                    ٻاهريون_Vertexp->userp(edgep);
                } وإلا {  // Duplicate
                    منطقي saveOld = سچ;
                    إذا (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = خطأ;  // new !cutable more important than old
                    } وإلا إذا (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = سچ;  // old !cutable more important than new
                    } وإلا {
                        saveOld = سچ;
                        إذا (!m_جوڙ_Weights
                            && (prevEdgep->وزن_2() < edgep->وزن_2())) {  // Keep max weight
                            prevEdgep->وزن_2(edgep->وزن_2());
                        }
                    }
                    إذا (saveOld) {
                        إذا (m_جوڙ_Weights) prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وإلا {
                        إذا (m_جوڙ_Weights) edgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        ٻاهريون_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

عام:
    گراف_Remove_Redundant(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp, منطقي جوڙ_Weights)
        : گراف_Alg<>{graphp, ڪنارو_Funcp}
        , m_جوڙ_Weights{جوڙ_Weights} {
        مکيه();
    }
    ~گراف_Remove_Redundant() = افتراضي;
};

فراغ V3_گراف::remove_Redundant_ڪنارا_Max(V3_ڪنارو_Func_P ڪنارو_Funcp) {
    گراف_Remove_Redundant{هذا, ڪنارو_Funcp, خطأ};
}
فراغ V3_گراف::remove_Redundant_ڪنارا_جوڙ(V3_ڪنارو_Func_P ڪنارو_Funcp) {
    گراف_Remove_Redundant{هذا, ڪنارو_Funcp, سچ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

صنف گراف_Alg_Remove_متعدي_ڪنارا final : گراف_Alg<> {
عام:
    واضح گراف_Alg_Remove_متعدي_ڪنارا(V3_گراف* graphp)
        : گراف_Alg<>(graphp, خالي_اشارو) {}
    فراغ go() {
        گراف_رستو_Checker checker{m_graphp};
        لكل (V3_گراف_راس& vtx : m_graphp->vertices()) {
            V3_گراف_ڪنارو* deletep = خالي_اشارو;
            لكل (V3_گراف_ڪنارو& ڪنارو : vtx.ٻاهريون_ڪنارا()) {
                إذا (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = خالي_اشارو);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                إذا (checker.is_متعدي_ڪنارو(&ڪنارو)) deletep = &ڪنارو;
            }
            إذا (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خاص:
    VL_UNCOPYABLE(گراف_Alg_Remove_متعدي_ڪنارا);
};

فراغ V3_گراف::remove_متعدي_ڪنارا() { گراف_Alg_Remove_متعدي_ڪنارا{هذا}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

صنف گراف_Alg_Weakly final : گراف_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساكن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_انگ_Colors = 0;  // Number of colors assigned

    فراغ مکيه() {
        // Initialize state
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) راس.color(UNCOLORED);
        // Color graph, without gaps
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (راس.color() == UNCOLORED) راس_Iterate(&راس, m_انگ_Colors++);
        }
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        إذا (vertexp->color() != UNCOLORED) أرجع;  // Already colored it
        vertexp->color(currentColor);
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) راس_Iterate(ڪنارو.چوٽي(), currentColor);
        }
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->اندريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) راس_Iterate(ڪنارو.fromp(), currentColor);
        }
    }

عام:
    گراف_Alg_Weakly(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
        : گراف_Alg<>(graphp, ڪنارو_Funcp) {
        مکيه();
    }
    ~گراف_Alg_Weakly() = افتراضي;

    uint32_t انگ_Colors() ثابت { أرجع m_انگ_Colors; }
};

uint32_t V3_گراف::weaklyConnected(V3_ڪنارو_Func_P ڪنارو_Funcp) {
    أرجع گراف_Alg_Weakly{هذا, ڪنارو_Funcp}.انگ_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

صنف گراف_Alg_Strongly final : گراف_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::ويڪٽر<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far

    فراغ مکيه() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.color(0);
            راس.user(0);
        }
        // Color graph
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) {
                m_currentDfs++;
                راس_Iterate(&راس);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            منطقي onecolor = سچ;
            لكل (V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) {
                إذا (follow_ڪنارو(&ڪنارو)) {
                    إذا (راس.color() == ڪنارو.چوٽي()->color()) {
                        onecolor = خطأ;
                        اكسر;
                    }
                }
            }
            إذا (onecolor) راس.color(0);
        }
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp) {
        ثابت uint32_t this_Dfs_انگ = m_currentDfs++;
        vertexp->user(this_Dfs_انگ);
        vertexp->color(0);
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) {
                V3_گراف_راس* چوٽي = ڪنارو.چوٽي();
                إذا (!چوٽي->user()) {  // Dest not computed yet
                    راس_Iterate(چوٽي);
                }
                إذا (!چوٽي->color()) {  // Dest not in a component
                    إذا (vertexp->user() > چوٽي->user()) vertexp->user(چوٽي->user());
                }
            }
        }
        إذا (vertexp->user() == this_Dfs_انگ) {  // New head of subtree
            vertexp->color(this_Dfs_انگ);  // Mark as component
            طالما (!m_callTrace.خالي_جايون()) {
                V3_گراف_راس* ڪڍو_Vertexp = m_callTrace.پٺ();
                إذا (ڪڍو_Vertexp->user() >= this_Dfs_انگ) {  // Lower node is part of this subtree
                    m_callTrace.ڪڍو_پٺ();
                    ڪڍو_Vertexp->color(this_Dfs_انگ);
                } وإلا {
                    اكسر;
                }
            }
        } وإلا {  // In another subtree (maybe...)
            m_callTrace.ڌڪو_پٺ(vertexp);
        }
    }

عام:
    گراف_Alg_Strongly(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
        : گراف_Alg<>{graphp, ڪنارو_Funcp} {
        مکيه();
    }
    ~گراف_Alg_Strongly() = افتراضي;
};

فراغ V3_گراف::stronglyConnected(V3_ڪنارو_Func_P ڪنارو_Funcp) { گراف_Alg_Strongly{هذا, ڪنارو_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

صنف گراف_Alg_Rank final : گراف_Alg<> {
    فراغ مکيه() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) راس_Iterate(&راس, 1);
        }
    }
    فراغ راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پيغام_Cb(vertexp, m_ڪنارو_Funcp);
            أرجع;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        إذا (vertexp->rank() >= currentRank) أرجع;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) راس_Iterate(ڪنارو.چوٽي(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_Rank(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
        : گراف_Alg<>{graphp, ڪنارو_Funcp} {
        مکيه();
    }
    ~گراف_Alg_Rank() = افتراضي;
};

فراغ V3_گراف::rank() { rank(&V3_گراف_ڪنارو::followAlwaysTrue); }
فراغ V3_گراف::rank(V3_ڪنارو_Func_P ڪنارو_Funcp) { گراف_Alg_Rank{هذا, ڪنارو_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

صنف گراف_Alg_Rank_Min final : گراف_Alg<> {
    فراغ مکيه() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) راس_Iterate(&راس);
        }
    }
    uint32_t راس_Iterate(V3_گراف_راس* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پيغام_Cb(vertexp, m_ڪنارو_Funcp);
            أرجع vertexp->rank();
        }
        إذا (vertexp->user()) أرجع vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->اندريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) {
                ثابت uint32_t nrank = راس_Iterate(ڪنارو.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        إذا (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        أرجع vertexp->rank();
    }

عام:
    گراف_Alg_Rank_Min(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
        : گراف_Alg<>{graphp, ڪنارو_Funcp} {
        مکيه();
    }
    ~گراف_Alg_Rank_Min() = افتراضي;
};

فراغ V3_گراف::rankMin() { rankMin(&V3_گراف_ڪنارو::followAlwaysTrue); }
فراغ V3_گراف::rankMin(V3_ڪنارو_Func_P ڪنارو_Funcp) { گراف_Alg_Rank_Min{هذا, ڪنارو_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

صنف گراف_Alg_RLoops final : گراف_Alg<> {
    std::ويڪٽر<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far
    std::ويڪٽر<string> m_msgs;  // Output messages
    منطقي m_پورو = خطأ;  // Exit algorithm

    فراغ مکيه(V3_گراف_راس* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        راس_Iterate(vertexp, 0);
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        إذا (m_پورو) أرجع;

        // Can't just reserve(), unless we modify size() before setting array directly
        طالما (m_callTrace.سائيز() <= currentRank) m_callTrace.ڌڪو_پٺ(vertexp);
        m_callTrace[currentRank++] = vertexp;

        إذا (vertexp->user() == 1) {
            لكل (بنانشان i = 0; i < currentRank; i++)
                m_msgs.emplace_پٺ(m_graphp->loops_راس_Cb(m_callTrace[i]));
            m_پورو = سچ;
            أرجع;
        }
        إذا (vertexp->user() == 2) أرجع;  // Already processed it
        vertexp->user(1);
        لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
            إذا (follow_ڪنارو(&ڪنارو)) راس_Iterate(ڪنارو.چوٽي(), currentRank);
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_RLoops(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp, V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, ڪنارو_Funcp} {
        مکيه(vertexp);
    }
    ~گراف_Alg_RLoops() = افتراضي;
    string پيغام() ثابت {
        أرجع std::accumulate(m_msgs.begin(), m_msgs.پڇاڙي(), std::string{""});
    }
};

string V3_گراف::reportLoops(V3_ڪنارو_Func_P ڪنارو_Funcp, V3_گراف_راس* vertexp) {
    أرجع گراف_Alg_RLoops{هذا, ڪنارو_Funcp, vertexp}.پيغام();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

صنف گراف_Alg_Subtrees final : گراف_Alg<> {
    V3_گراف* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_گراف_راس* راس_Iterate_All(V3_گراف_راس* vertexp) {
        إذا (V3_گراف_راس* newVertexp = static_cast<V3_گراف_راس*>(vertexp->userp())) {
            أرجع newVertexp;
        } وإلا {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
                إذا (follow_ڪنارو(&ڪنارو)) {
                    V3_گراف_ڪنارو* newEdgep = static_cast<V3_گراف_ڪنارو*>(ڪنارو.userp());
                    إذا (!newEdgep) {
                        V3_گراف_راس* new_چوٽي = راس_Iterate_All(ڪنارو.چوٽي());
                        newEdgep = ڪنارو.clone(m_loopGraphp, newVertexp, new_چوٽي);
                        ڪنارو.userp(newEdgep);
                    }
                }
            }
            أرجع newVertexp;
        }
    }

عام:
    گراف_Alg_Subtrees(V3_گراف* graphp, V3_گراف* loopGraphp, V3_ڪنارو_Func_P ڪنارو_Funcp,
                     V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, ڪنارو_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_ڪنارا();
        (فراغ)راس_Iterate_All(vertexp);
    }
    ~گراف_Alg_Subtrees() = افتراضي;
};

//! Report the entire connected graph with a loop or loops
فراغ V3_گراف::subtreeLoops(V3_ڪنارو_Func_P ڪنارو_Funcp, V3_گراف_راس* vertexp, V3_گراف* loopGraphp) {
    گراف_Alg_Subtrees{هذا, loopGraphp, ڪنارو_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

فراغ V3_گراف::ترتيب_ڏيو_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::ويڪٽر<V3_گراف_راس*> vertexps;
    لكل (V3_گراف_راس& راس : m_vertices) vertexps.ڌڪو_پٺ(&راس);
    std::stable_ترتيب_ڏيو(vertexps.begin(), vertexps.پڇاڙي(),
                     [](ثابت V3_گراف_راس* lhsp, ثابت V3_گراف_راس* rhsp) {  //
                         أرجع lhsp->ترتيب_ڏيو_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    لكل (V3_گراف_راس* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_پٺ(vertexp);
    }
}

فراغ V3_گراف::ترتيب_ڏيو_ڪنارا() {
    // Sort edges by rank then fanout of node they point to
    std::ويڪٽر<V3_گراف_ڪنارو*> ڪنارا;
    لكل (V3_گراف_راس& راس : vertices()) {
        // Make a vector
        لكل (V3_گراف_ڪنارو& ڪنارو : راس.ٻاهريون_ڪنارا()) ڪنارا.ڌڪو_پٺ(&ڪنارو);
        //ترتيب_ڏيو
        std::stable_ترتيب_ڏيو(ڪنارا.begin(), ڪنارا.پڇاڙي(),
                         [](ثابت V3_گراف_ڪنارو* lhsp, ثابت V3_گراف_ڪنارو* rhsp) {  //
                             أرجع lhsp->ترتيب_ڏيو_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        لكل (V3_گراف_ڪنارو* ثابت edgep : ڪنارا) edgep->relinkFromp(&راس);
        // Prep for next
        ڪنارا.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

فراغ V3_گراف::ترتيب() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_گراف_ڪنارو::followAlwaysTrue);
    ترتيب_Pre_Ranked();
}

فراغ V3_گراف::ترتيب_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    لكل (V3_گراف_راس& راس : vertices()) {
        إذا (!راس.user()) ترتيب_DFSIterate(&راس);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ترتيب_ڏيو_Vertices();
    // Sort edges by rank then fanout of node they point to
    ترتيب_ڏيو_ڪنارا();
}

مزدوج V3_گراف::ترتيب_DFSIterate(V3_گراف_راس* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    إذا (vertexp->user() == 2) أرجع vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    مزدوج fanout = 0;
    لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->ٻاهريون_ڪنارا()) {
        إذا (ڪنارو.وزن_2()) fanout += ترتيب_DFSIterate(ڪنارو.چوٽي());
    }
    // Just count inbound edges
    لكل (V3_گراف_ڪنارو& ڪنارو : vertexp->اندريون_ڪنارا()) {
        إذا (ڪنارو.وزن_2()) ++fanout;
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
    ثابت std::function<uint64_t(ثابت V3_گراف_راس*)> m_راس_لاڳت;  // vertex cost function
    V3_گراف::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    واضح گراف_Alg_Parallelism_Report(V3_گراف& گراف,
                                       std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاڳت)
        : m_گراف{گراف}
        , m_راس_لاڳت{راس_لاڳت} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3_گراف_راس*, uint64_t> critPaths;
        گراف_وهڪرو_Unordered serialize{&m_گراف};
        لكل (ثابت V3_گراف_راس* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_راس_ڳڻپ;
            uint64_t cp_لاڳت_منزل_هتي = 0;
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vertexp->اندريون_ڪنارا()) {
                ++m_report.m_ڪنارو_ڳڻپ;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_لاڳت_منزل_هتي = std::max(cp_لاڳت_منزل_هتي, critPaths[ڪنارو.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_لاڳت_منزل_هتي += m_راس_لاڳت(vertexp);
            critPaths[vertexp] = cp_لاڳت_منزل_هتي;
            m_report.m_critical_رستو_لاڳت = std::max(m_report.m_critical_رستو_لاڳت, cp_لاڳت_منزل_هتي);
            // Tally the total cost contributed by vertices.
            m_report.m_ڪل_گراف_لاڳت += m_راس_لاڳت(vertexp);
        }
    }
    ~گراف_Alg_Parallelism_Report() = افتراضي;
    VL_UNCOPYABLE(گراف_Alg_Parallelism_Report);
    VL_UNMOVABLE(گراف_Alg_Parallelism_Report);

عام:
    ساكن V3_گراف::ParallelismReport
    apply(V3_گراف& گراف, std::function<uint32_t(ثابت V3_گراف_راس*)> راس_لاڳت) {
        أرجع گراف_Alg_Parallelism_Report(گراف, راس_لاڳت).m_report;
    }
};

V3_گراف::ParallelismReport
V3_گراف::parallelismReport(std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاڳت) {
    أرجع گراف_Alg_Parallelism_Report::apply(*هذا, راس_لاڳت);
}
