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

صنف ګراف_Remove_Redundant final : ګراف_Alg<> {
    ثابت منطقي m_مجموعه_Weights;  // Sum, rather then maximize weights
خاص:
    فراغ اصلي() {
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) راس_Iterate(&راس);
    }
    فراغ راس_Iterate(V3_ګراف_راس* vertexp) {
        // Clear marks
        لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) څنډه.سر()->userp(تش_اشاره);

        // Mark edges and detect duplications

        لكل (V3_ګراف_څنډه* ثابت edgep : vertexp->بهر_څنډې().unlinkable()) {
            إذا (follow_څنډه(edgep)) {
                V3_ګراف_راس* بهر_Vertexp = edgep->سر();
                V3_ګراف_څنډه* prevEdgep = static_cast<V3_ګراف_څنډه*>(بهر_Vertexp->userp());
                إذا (!prevEdgep) {  // No previous assignment
                    بهر_Vertexp->userp(edgep);
                } وإلا {  // Duplicate
                    منطقي saveOld = ریښتیا;
                    إذا (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = خطأ;  // new !cutable more important than old
                    } وإلا إذا (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = ریښتیا;  // old !cutable more important than new
                    } وإلا {
                        saveOld = ریښتیا;
                        إذا (!m_مجموعه_Weights
                            && (prevEdgep->وزن_2() < edgep->وزن_2())) {  // Keep max weight
                            prevEdgep->وزن_2(edgep->وزن_2());
                        }
                    }
                    إذا (saveOld) {
                        إذا (m_مجموعه_Weights) prevEdgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وإلا {
                        إذا (m_مجموعه_Weights) edgep->وزن_2(prevEdgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        بهر_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

عام:
    ګراف_Remove_Redundant(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp, منطقي مجموعه_Weights)
        : ګراف_Alg<>{graphp, څنډه_Funcp}
        , m_مجموعه_Weights{مجموعه_Weights} {
        اصلي();
    }
    ~ګراف_Remove_Redundant() = افتراضي;
};

فراغ V3_ګراف::remove_Redundant_څنډې_Max(V3_څنډه_Func_P څنډه_Funcp) {
    ګراف_Remove_Redundant{هذا, څنډه_Funcp, خطأ};
}
فراغ V3_ګراف::remove_Redundant_څنډې_مجموعه(V3_څنډه_Func_P څنډه_Funcp) {
    ګراف_Remove_Redundant{هذا, څنډه_Funcp, ریښتیا};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

صنف ګراف_Alg_Remove_متعدي_څنډې final : ګراف_Alg<> {
عام:
    څرګند ګراف_Alg_Remove_متعدي_څنډې(V3_ګراف* graphp)
        : ګراف_Alg<>(graphp, تش_اشاره) {}
    فراغ go() {
        ګراف_لار_Checker checker{m_graphp};
        لكل (V3_ګراف_راس& vtx : m_graphp->vertices()) {
            V3_ګراف_څنډه* deletep = تش_اشاره;
            لكل (V3_ګراف_څنډه& څنډه : vtx.بهر_څنډې()) {
                إذا (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = تش_اشاره);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                إذا (checker.is_متعدي_څنډه(&څنډه)) deletep = &څنډه;
            }
            إذا (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خاص:
    VL_UNCOPYABLE(ګراف_Alg_Remove_متعدي_څنډې);
};

فراغ V3_ګراف::remove_متعدي_څنډې() { ګراف_Alg_Remove_متعدي_څنډې{هذا}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

صنف ګراف_Alg_Weakly final : ګراف_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساكن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_عدد_Colors = 0;  // Number of colors assigned

    فراغ اصلي() {
        // Initialize state
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) راس.color(UNCOLORED);
        // Color graph, without gaps
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            إذا (راس.color() == UNCOLORED) راس_Iterate(&راس, m_عدد_Colors++);
        }
    }

    فراغ راس_Iterate(V3_ګراف_راس* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        إذا (vertexp->color() != UNCOLORED) أرجع;  // Already colored it
        vertexp->color(currentColor);
        لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
            إذا (follow_څنډه(&څنډه)) راس_Iterate(څنډه.سر(), currentColor);
        }
        لكل (V3_ګراف_څنډه& څنډه : vertexp->دننه_څنډې()) {
            إذا (follow_څنډه(&څنډه)) راس_Iterate(څنډه.fromp(), currentColor);
        }
    }

عام:
    ګراف_Alg_Weakly(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
        : ګراف_Alg<>(graphp, څنډه_Funcp) {
        اصلي();
    }
    ~ګراف_Alg_Weakly() = افتراضي;

    uint32_t عدد_Colors() ثابت { أرجع m_عدد_Colors; }
};

uint32_t V3_ګراف::weaklyConnected(V3_څنډه_Func_P څنډه_Funcp) {
    أرجع ګراف_Alg_Weakly{هذا, څنډه_Funcp}.عدد_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

صنف ګراف_Alg_Strongly final : ګراف_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::ویکټور<V3_ګراف_راس*> m_callTrace;  // List of everything we hit processing so far

    فراغ اصلي() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            راس.color(0);
            راس.user(0);
        }
        // Color graph
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) {
                m_currentDfs++;
                راس_Iterate(&راس);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            منطقي onecolor = ریښتیا;
            لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) {
                إذا (follow_څنډه(&څنډه)) {
                    إذا (راس.color() == څنډه.سر()->color()) {
                        onecolor = خطأ;
                        اكسر;
                    }
                }
            }
            إذا (onecolor) راس.color(0);
        }
    }

    فراغ راس_Iterate(V3_ګراف_راس* vertexp) {
        ثابت uint32_t this_Dfs_عدد = m_currentDfs++;
        vertexp->user(this_Dfs_عدد);
        vertexp->color(0);
        لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
            إذا (follow_څنډه(&څنډه)) {
                V3_ګراف_راس* سر = څنډه.سر();
                إذا (!سر->user()) {  // Dest not computed yet
                    راس_Iterate(سر);
                }
                إذا (!سر->color()) {  // Dest not in a component
                    إذا (vertexp->user() > سر->user()) vertexp->user(سر->user());
                }
            }
        }
        إذا (vertexp->user() == this_Dfs_عدد) {  // New head of subtree
            vertexp->color(this_Dfs_عدد);  // Mark as component
            طالما (!m_callTrace.تشه()) {
                V3_ګراف_راس* راوباسه_Vertexp = m_callTrace.شا();
                إذا (راوباسه_Vertexp->user() >= this_Dfs_عدد) {  // Lower node is part of this subtree
                    m_callTrace.راوباسه_شا();
                    راوباسه_Vertexp->color(this_Dfs_عدد);
                } وإلا {
                    اكسر;
                }
            }
        } وإلا {  // In another subtree (maybe...)
            m_callTrace.ټېل_شا(vertexp);
        }
    }

عام:
    ګراف_Alg_Strongly(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
        : ګراف_Alg<>{graphp, څنډه_Funcp} {
        اصلي();
    }
    ~ګراف_Alg_Strongly() = افتراضي;
};

فراغ V3_ګراف::stronglyConnected(V3_څنډه_Func_P څنډه_Funcp) { ګراف_Alg_Strongly{هذا, څنډه_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

صنف ګراف_Alg_Rank final : ګراف_Alg<> {
    فراغ اصلي() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) راس_Iterate(&راس, 1);
        }
    }
    فراغ راس_Iterate(V3_ګراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پیغام_Cb(vertexp, m_څنډه_Funcp);
            أرجع;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        إذا (vertexp->rank() >= currentRank) أرجع;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
            إذا (follow_څنډه(&څنډه)) راس_Iterate(څنډه.سر(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عام:
    ګراف_Alg_Rank(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
        : ګراف_Alg<>{graphp, څنډه_Funcp} {
        اصلي();
    }
    ~ګراف_Alg_Rank() = افتراضي;
};

فراغ V3_ګراف::rank() { rank(&V3_ګراف_څنډه::followAlwaysTrue); }
فراغ V3_ګراف::rank(V3_څنډه_Func_P څنډه_Funcp) { ګراف_Alg_Rank{هذا, څنډه_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

صنف ګراف_Alg_Rank_Min final : ګراف_Alg<> {
    فراغ اصلي() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        لكل (V3_ګراف_راس& راس : m_graphp->vertices()) {
            إذا (!راس.user()) راس_Iterate(&راس);
        }
    }
    uint32_t راس_Iterate(V3_ګراف_راس* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        إذا (vertexp->user() == 1) {
            m_graphp->loops_پیغام_Cb(vertexp, m_څنډه_Funcp);
            أرجع vertexp->rank();
        }
        إذا (vertexp->user()) أرجع vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        لكل (V3_ګراف_څنډه& څنډه : vertexp->دننه_څنډې()) {
            إذا (follow_څنډه(&څنډه)) {
                ثابت uint32_t nrank = راس_Iterate(څنډه.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        إذا (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        أرجع vertexp->rank();
    }

عام:
    ګراف_Alg_Rank_Min(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
        : ګراف_Alg<>{graphp, څنډه_Funcp} {
        اصلي();
    }
    ~ګراف_Alg_Rank_Min() = افتراضي;
};

فراغ V3_ګراف::rankMin() { rankMin(&V3_ګراف_څنډه::followAlwaysTrue); }
فراغ V3_ګراف::rankMin(V3_څنډه_Func_P څنډه_Funcp) { ګراف_Alg_Rank_Min{هذا, څنډه_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

صنف ګراف_Alg_RLoops final : ګراف_Alg<> {
    std::ویکټور<V3_ګراف_راس*> m_callTrace;  // List of everything we hit processing so far
    std::ویکټور<string> m_msgs;  // Output messages
    منطقي m_بشپړ = خطأ;  // Exit algorithm

    فراغ اصلي(V3_ګراف_راس* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        راس_Iterate(vertexp, 0);
    }

    فراغ راس_Iterate(V3_ګراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        إذا (m_بشپړ) أرجع;

        // Can't just reserve(), unless we modify size() before setting array directly
        طالما (m_callTrace.اندازه() <= currentRank) m_callTrace.ټېل_شا(vertexp);
        m_callTrace[currentRank++] = vertexp;

        إذا (vertexp->user() == 1) {
            لكل (بینښه i = 0; i < currentRank; i++)
                m_msgs.emplace_شا(m_graphp->loops_راس_Cb(m_callTrace[i]));
            m_بشپړ = ریښتیا;
            أرجع;
        }
        إذا (vertexp->user() == 2) أرجع;  // Already processed it
        vertexp->user(1);
        لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
            إذا (follow_څنډه(&څنډه)) راس_Iterate(څنډه.سر(), currentRank);
        }
        vertexp->user(2);
    }

عام:
    ګراف_Alg_RLoops(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp, V3_ګراف_راس* vertexp)
        : ګراف_Alg<>{graphp, څنډه_Funcp} {
        اصلي(vertexp);
    }
    ~ګراف_Alg_RLoops() = افتراضي;
    string پیغام() ثابت {
        أرجع std::accumulate(m_msgs.begin(), m_msgs.پای_ټکی(), std::string{""});
    }
};

string V3_ګراف::reportLoops(V3_څنډه_Func_P څنډه_Funcp, V3_ګراف_راس* vertexp) {
    أرجع ګراف_Alg_RLoops{هذا, څنډه_Funcp, vertexp}.پیغام();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

صنف ګراف_Alg_Subtrees final : ګراف_Alg<> {
    V3_ګراف* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_ګراف_راس* راس_Iterate_All(V3_ګراف_راس* vertexp) {
        إذا (V3_ګراف_راس* newVertexp = static_cast<V3_ګراف_راس*>(vertexp->userp())) {
            أرجع newVertexp;
        } وإلا {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
                إذا (follow_څنډه(&څنډه)) {
                    V3_ګراف_څنډه* newEdgep = static_cast<V3_ګراف_څنډه*>(څنډه.userp());
                    إذا (!newEdgep) {
                        V3_ګراف_راس* new_سر = راس_Iterate_All(څنډه.سر());
                        newEdgep = څنډه.clone(m_loopGraphp, newVertexp, new_سر);
                        څنډه.userp(newEdgep);
                    }
                }
            }
            أرجع newVertexp;
        }
    }

عام:
    ګراف_Alg_Subtrees(V3_ګراف* graphp, V3_ګراف* loopGraphp, V3_څنډه_Func_P څنډه_Funcp,
                     V3_ګراف_راس* vertexp)
        : ګراف_Alg<>{graphp, څنډه_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_څنډې();
        (فراغ)راس_Iterate_All(vertexp);
    }
    ~ګراف_Alg_Subtrees() = افتراضي;
};

//! Report the entire connected graph with a loop or loops
فراغ V3_ګراف::subtreeLoops(V3_څنډه_Func_P څنډه_Funcp, V3_ګراف_راس* vertexp, V3_ګراف* loopGraphp) {
    ګراف_Alg_Subtrees{هذا, loopGraphp, څنډه_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

فراغ V3_ګراف::ترتیبول_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::ویکټور<V3_ګراف_راس*> vertexps;
    لكل (V3_ګراف_راس& راس : m_vertices) vertexps.ټېل_شا(&راس);
    std::stable_ترتیبول(vertexps.begin(), vertexps.پای_ټکی(),
                     [](ثابت V3_ګراف_راس* lhsp, ثابت V3_ګراف_راس* rhsp) {  //
                         أرجع lhsp->ترتیبول_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    لكل (V3_ګراف_راس* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_شا(vertexp);
    }
}

فراغ V3_ګراف::ترتیبول_څنډې() {
    // Sort edges by rank then fanout of node they point to
    std::ویکټور<V3_ګراف_څنډه*> څنډې;
    لكل (V3_ګراف_راس& راس : vertices()) {
        // Make a vector
        لكل (V3_ګراف_څنډه& څنډه : راس.بهر_څنډې()) څنډې.ټېل_شا(&څنډه);
        //ترتیبول
        std::stable_ترتیبول(څنډې.begin(), څنډې.پای_ټکی(),
                         [](ثابت V3_ګراف_څنډه* lhsp, ثابت V3_ګراف_څنډه* rhsp) {  //
                             أرجع lhsp->ترتیبول_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        لكل (V3_ګراف_څنډه* ثابت edgep : څنډې) edgep->relinkFromp(&راس);
        // Prep for next
        څنډې.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

فراغ V3_ګراف::ترتیب() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_ګراف_څنډه::followAlwaysTrue);
    ترتیب_Pre_Ranked();
}

فراغ V3_ګراف::ترتیب_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    لكل (V3_ګراف_راس& راس : vertices()) {
        إذا (!راس.user()) ترتیب_DFSIterate(&راس);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ترتیبول_Vertices();
    // Sort edges by rank then fanout of node they point to
    ترتیبول_څنډې();
}

مزدوج V3_ګراف::ترتیب_DFSIterate(V3_ګراف_راس* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    إذا (vertexp->user() == 2) أرجع vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    مزدوج fanout = 0;
    لكل (V3_ګراف_څنډه& څنډه : vertexp->بهر_څنډې()) {
        إذا (څنډه.وزن_2()) fanout += ترتیب_DFSIterate(څنډه.سر());
    }
    // Just count inbound edges
    لكل (V3_ګراف_څنډه& څنډه : vertexp->دننه_څنډې()) {
        إذا (څنډه.وزن_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    أرجع vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

صنف ګراف_Alg_Parallelism_Report final {
    // MEMBERS
    V3_ګراف& m_ګراف;  // The graph
    ثابت std::function<uint64_t(ثابت V3_ګراف_راس*)> m_راس_لګښت;  // vertex cost function
    V3_ګراف::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    څرګند ګراف_Alg_Parallelism_Report(V3_ګراف& ګراف,
                                       std::function<uint64_t(ثابت V3_ګراف_راس*)> راس_لګښت)
        : m_ګراف{ګراف}
        , m_راس_لګښت{راس_لګښت} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3_ګراف_راس*, uint64_t> critPaths;
        ګراف_بهیر_Unordered serialize{&m_ګراف};
        لكل (ثابت V3_ګراف_راس* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_راس_شمېر;
            uint64_t cp_لګښت_منزل_دلته = 0;
            لكل (ثابت V3_ګراف_څنډه& څنډه : vertexp->دننه_څنډې()) {
                ++m_report.m_څنډه_شمېر;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_لګښت_منزل_دلته = std::max(cp_لګښت_منزل_دلته, critPaths[څنډه.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_لګښت_منزل_دلته += m_راس_لګښت(vertexp);
            critPaths[vertexp] = cp_لګښت_منزل_دلته;
            m_report.m_critical_لار_لګښت = std::max(m_report.m_critical_لار_لګښت, cp_لګښت_منزل_دلته);
            // Tally the total cost contributed by vertices.
            m_report.m_ټول_ګراف_لګښت += m_راس_لګښت(vertexp);
        }
    }
    ~ګراف_Alg_Parallelism_Report() = افتراضي;
    VL_UNCOPYABLE(ګراف_Alg_Parallelism_Report);
    VL_UNMOVABLE(ګراف_Alg_Parallelism_Report);

عام:
    ساكن V3_ګراف::ParallelismReport
    apply(V3_ګراف& ګراف, std::function<uint32_t(ثابت V3_ګراف_راس*)> راس_لګښت) {
        أرجع ګراف_Alg_Parallelism_Report(ګراف, راس_لګښت).m_report;
    }
};

V3_ګراف::ParallelismReport
V3_ګراف::parallelismReport(std::function<uint64_t(ثابت V3_ګراف_راس*)> راس_لګښت) {
    أرجع ګراف_Alg_Parallelism_Report::apply(*هذا, راس_لګښت);
}
