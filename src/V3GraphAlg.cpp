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

#شامل "V3GraphAlg.h"

#شامل "V3Global.h"
#شامل "V3GraphPathChecker.h"
#شامل "V3GraphStream.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <limits>
#شامل <list>
#شامل <map>
#شامل <numeric>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - Remove redundancies
// Changes user() and weight()

جماعت گراف_Remove_Redundant final : گراف_Alg<> {
    ثابت منطقی m_مجموع_Weights;  // Sum, rather then maximize weights
نجی:
    خالی مین() {
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) راس_Iterate(&راس);
    }
    خالی راس_Iterate(V3_گراف_راس* vertexp) {
        // Clear marks
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) کنارہ.چوٹی()->userp(خالی_اشاریہ);

        // Mark edges and detect duplications

        برائے (V3_گراف_کنارہ* ثابت edgep : vertexp->برآمد_کنارے().unlinkable()) {
            اگر (follow_کنارہ(edgep)) {
                V3_گراف_راس* برآمد_Vertexp = edgep->چوٹی();
                V3_گراف_کنارہ* پچھلا_Edgep = static_cast<V3_گراف_کنارہ*>(برآمد_Vertexp->userp());
                اگر (!پچھلا_Edgep) {  // No previous assignment
                    برآمد_Vertexp->userp(edgep);
                } ورنہ {  // Duplicate
                    منطقی saveOld = سچ;
                    اگر (پچھلا_Edgep->cutable() && !edgep->cutable()) {
                        saveOld = جھوٹ;  // new !cutable more important than old
                    } ورنہ اگر (!پچھلا_Edgep->cutable() && edgep->cutable()) {
                        saveOld = سچ;  // old !cutable more important than new
                    } ورنہ {
                        saveOld = سچ;
                        اگر (!m_مجموع_Weights
                            && (پچھلا_Edgep->وزن_2() < edgep->وزن_2())) {  // Keep max weight
                            پچھلا_Edgep->وزن_2(edgep->وزن_2());
                        }
                    }
                    اگر (saveOld) {
                        اگر (m_مجموع_Weights) پچھلا_Edgep->وزن_2(پچھلا_Edgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } ورنہ {
                        اگر (m_مجموع_Weights) edgep->وزن_2(پچھلا_Edgep->وزن_2() + edgep->وزن_2());
                        VL_DO_DANGLING(پچھلا_Edgep->unlinkDelete(), پچھلا_Edgep);
                        برآمد_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

عوامی:
    گراف_Remove_Redundant(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp, منطقی مجموع_Weights)
        : گراف_Alg<>{graphp, کنارہ_Funcp}
        , m_مجموع_Weights{مجموع_Weights} {
        مین();
    }
    ~گراف_Remove_Redundant() = طےشدہ;
};

خالی V3_گراف::remove_Redundant_کنارے_Max(V3_کنارہ_Func_P کنارہ_Funcp) {
    گراف_Remove_Redundant{یہ, کنارہ_Funcp, جھوٹ};
}
خالی V3_گراف::remove_Redundant_کنارے_مجموع(V3_کنارہ_Func_P کنارہ_Funcp) {
    گراف_Remove_Redundant{یہ, کنارہ_Funcp, سچ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

جماعت گراف_Alg_Remove_متعدی_کنارے final : گراف_Alg<> {
عوامی:
    صریح گراف_Alg_Remove_متعدی_کنارے(V3_گراف* graphp)
        : گراف_Alg<>(graphp, خالی_اشاریہ) {}
    خالی go() {
        گراف_راستہ_Checker checker{m_graphp};
        برائے (V3_گراف_راس& vtx : m_graphp->vertices()) {
            V3_گراف_کنارہ* deletep = خالی_اشاریہ;
            برائے (V3_گراف_کنارہ& کنارہ : vtx.برآمد_کنارے()) {
                اگر (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = خالی_اشاریہ);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                اگر (checker.is_متعدی_کنارہ(&کنارہ)) deletep = &کنارہ;
            }
            اگر (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

نجی:
    VL_UNCOPYABLE(گراف_Alg_Remove_متعدی_کنارے);
};

خالی V3_گراف::remove_متعدی_کنارے() { گراف_Alg_Remove_متعدی_کنارے{یہ}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

جماعت گراف_Alg_Weakly final : گراف_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساکن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_عدد_Colors = 0;  // Number of colors assigned

    خالی مین() {
        // Initialize state
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) راس.color(UNCOLORED);
        // Color graph, without gaps
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            اگر (راس.color() == UNCOLORED) راس_Iterate(&راس, m_عدد_Colors++);
        }
    }

    خالی راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        اگر (vertexp->color() != UNCOLORED) واپس;  // Already colored it
        vertexp->color(currentColor);
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) راس_Iterate(کنارہ.چوٹی(), currentColor);
        }
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->اندر_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) راس_Iterate(کنارہ.fromp(), currentColor);
        }
    }

عوامی:
    گراف_Alg_Weakly(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : گراف_Alg<>(graphp, کنارہ_Funcp) {
        مین();
    }
    ~گراف_Alg_Weakly() = طےشدہ;

    uint32_t عدد_Colors() ثابت { واپس m_عدد_Colors; }
};

uint32_t V3_گراف::weaklyConnected(V3_کنارہ_Func_P کنارہ_Funcp) {
    واپس گراف_Alg_Weakly{یہ, کنارہ_Funcp}.عدد_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

جماعت گراف_Alg_Strongly final : گراف_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::سمتیہ<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far

    خالی مین() {
        // Use Pearce's algorithm to color the strongly connected components. For reference see
        // "An Improved Algorithm for Finding the Strongly Connected Components of a Directed
        // Graph", David J.Pearce, 2005
        //
        // Node State:
        //     Vertex::user     // DFS number indicating possible root of subtree, 0=not iterated
        //     Vertex::color    // Output subtree number (fully processed)

        // Clear info
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.color(0);
            راس.user(0);
        }
        // Color graph
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            اگر (!راس.user()) {
                m_currentDfs++;
                راس_Iterate(&راس);
            }
        }
        // If there's a single vertex of a color, it doesn't need a subgraph
        // This simplifies the consumer's code, and reduces graph debugging clutter
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            منطقی onecolor = سچ;
            برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) {
                اگر (follow_کنارہ(&کنارہ)) {
                    اگر (راس.color() == کنارہ.چوٹی()->color()) {
                        onecolor = جھوٹ;
                        توڑو;
                    }
                }
            }
            اگر (onecolor) راس.color(0);
        }
    }

    خالی راس_Iterate(V3_گراف_راس* vertexp) {
        ثابت uint32_t this_Dfs_عدد = m_currentDfs++;
        vertexp->user(this_Dfs_عدد);
        vertexp->color(0);
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) {
                V3_گراف_راس* چوٹی = کنارہ.چوٹی();
                اگر (!چوٹی->user()) {  // Dest not computed yet
                    راس_Iterate(چوٹی);
                }
                اگر (!چوٹی->color()) {  // Dest not in a component
                    اگر (vertexp->user() > چوٹی->user()) vertexp->user(چوٹی->user());
                }
            }
        }
        اگر (vertexp->user() == this_Dfs_عدد) {  // New head of subtree
            vertexp->color(this_Dfs_عدد);  // Mark as component
            جب (!m_callTrace.خالی_خانے()) {
                V3_گراف_راس* نکالو_Vertexp = m_callTrace.عقب();
                اگر (نکالو_Vertexp->user() >= this_Dfs_عدد) {  // Lower node is part of this subtree
                    m_callTrace.نکالو_عقب();
                    نکالو_Vertexp->color(this_Dfs_عدد);
                } ورنہ {
                    توڑو;
                }
            }
        } ورنہ {  // In another subtree (maybe...)
            m_callTrace.ڈالو_عقب(vertexp);
        }
    }

عوامی:
    گراف_Alg_Strongly(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : گراف_Alg<>{graphp, کنارہ_Funcp} {
        مین();
    }
    ~گراف_Alg_Strongly() = طےشدہ;
};

خالی V3_گراف::stronglyConnected(V3_کنارہ_Func_P کنارہ_Funcp) { گراف_Alg_Strongly{یہ, کنارہ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

جماعت گراف_Alg_Rank final : گراف_Alg<> {
    خالی مین() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            اگر (!راس.user()) راس_Iterate(&راس, 1);
        }
    }
    خالی راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // If larger rank is found, assign it and loop back through
        // If we hit a back node make a list of all loops
        اگر (vertexp->user() == 1) {
            m_graphp->loops_پیغام_Cb(vertexp, m_کنارہ_Funcp);
            واپس;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        اگر (vertexp->rank() >= currentRank) واپس;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) راس_Iterate(کنارہ.چوٹی(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عوامی:
    گراف_Alg_Rank(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : گراف_Alg<>{graphp, کنارہ_Funcp} {
        مین();
    }
    ~گراف_Alg_Rank() = طےشدہ;
};

خالی V3_گراف::rank() { rank(&V3_گراف_کنارہ::followAlwaysTrue); }
خالی V3_گراف::rank(V3_کنارہ_Func_P کنارہ_Funcp) { گراف_Alg_Rank{یہ, کنارہ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

جماعت گراف_Alg_Rank_Min final : گراف_Alg<> {
    خالی مین() {
        // Rank each vertex, ignoring cutable edges
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            راس.rank(0);
            راس.user(0);
        }
        برائے (V3_گراف_راس& راس : m_graphp->vertices()) {
            اگر (!راس.user()) راس_Iterate(&راس);
        }
    }
    uint32_t راس_Iterate(V3_گراف_راس* vertexp) {
        // Assign rank to each unvisited node
        // If we hit a back node make a list of all loops
        اگر (vertexp->user() == 1) {
            m_graphp->loops_پیغام_Cb(vertexp, m_کنارہ_Funcp);
            واپس vertexp->rank();
        }
        اگر (vertexp->user()) واپس vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->اندر_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) {
                ثابت uint32_t nrank = راس_Iterate(کنارہ.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        اگر (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        واپس vertexp->rank();
    }

عوامی:
    گراف_Alg_Rank_Min(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
        : گراف_Alg<>{graphp, کنارہ_Funcp} {
        مین();
    }
    ~گراف_Alg_Rank_Min() = طےشدہ;
};

خالی V3_گراف::rankMin() { rankMin(&V3_گراف_کنارہ::followAlwaysTrue); }
خالی V3_گراف::rankMin(V3_کنارہ_Func_P کنارہ_Funcp) { گراف_Alg_Rank_Min{یہ, کنارہ_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

جماعت گراف_Alg_RLoops final : گراف_Alg<> {
    std::سمتیہ<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far
    std::سمتیہ<string> m_msgs;  // Output messages
    منطقی m_مکمل = جھوٹ;  // Exit algorithm

    خالی مین(V3_گراف_راس* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        راس_Iterate(vertexp, 0);
    }

    خالی راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        اگر (m_مکمل) واپس;

        // Can't just reserve(), unless we modify size() before setting array directly
        جب (m_callTrace.سائز() <= currentRank) m_callTrace.ڈالو_عقب(vertexp);
        m_callTrace[currentRank++] = vertexp;

        اگر (vertexp->user() == 1) {
            برائے (غیرعلامتی i = 0; i < currentRank; i++)
                m_msgs.emplace_عقب(m_graphp->loops_راس_Cb(m_callTrace[i]));
            m_مکمل = سچ;
            واپس;
        }
        اگر (vertexp->user() == 2) واپس;  // Already processed it
        vertexp->user(1);
        برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
            اگر (follow_کنارہ(&کنارہ)) راس_Iterate(کنارہ.چوٹی(), currentRank);
        }
        vertexp->user(2);
    }

عوامی:
    گراف_Alg_RLoops(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp, V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, کنارہ_Funcp} {
        مین(vertexp);
    }
    ~گراف_Alg_RLoops() = طےشدہ;
    string پیغام() ثابت {
        واپس std::accumulate(m_msgs.begin(), m_msgs.انتہا(), std::string{""});
    }
};

string V3_گراف::reportLoops(V3_کنارہ_Func_P کنارہ_Funcp, V3_گراف_راس* vertexp) {
    واپس گراف_Alg_RLoops{یہ, کنارہ_Funcp, vertexp}.پیغام();
}

//######################################################################
//######################################################################
// Algorithms - subtrees
// Changes user()

جماعت گراف_Alg_Subtrees final : گراف_Alg<> {
    V3_گراف* ثابت m_loopGraphp;

    //! Iterate through all connected nodes of a graph with a loop or loops.
    V3_گراف_راس* راس_Iterate_All(V3_گراف_راس* vertexp) {
        اگر (V3_گراف_راس* newVertexp = static_cast<V3_گراف_راس*>(vertexp->userp())) {
            واپس newVertexp;
        } ورنہ {
            newVertexp = vertexp->clone(m_loopGraphp);
            vertexp->userp(newVertexp);

            برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
                اگر (follow_کنارہ(&کنارہ)) {
                    V3_گراف_کنارہ* newEdgep = static_cast<V3_گراف_کنارہ*>(کنارہ.userp());
                    اگر (!newEdgep) {
                        V3_گراف_راس* new_چوٹی = راس_Iterate_All(کنارہ.چوٹی());
                        newEdgep = کنارہ.clone(m_loopGraphp, newVertexp, new_چوٹی);
                        کنارہ.userp(newEdgep);
                    }
                }
            }
            واپس newVertexp;
        }
    }

عوامی:
    گراف_Alg_Subtrees(V3_گراف* graphp, V3_گراف* loopGraphp, V3_کنارہ_Func_P کنارہ_Funcp,
                     V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, کنارہ_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_کنارے();
        (خالی)راس_Iterate_All(vertexp);
    }
    ~گراف_Alg_Subtrees() = طےشدہ;
};

//! Report the entire connected graph with a loop or loops
خالی V3_گراف::subtreeLoops(V3_کنارہ_Func_P کنارہ_Funcp, V3_گراف_راس* vertexp, V3_گراف* loopGraphp) {
    گراف_Alg_Subtrees{یہ, loopGraphp, کنارہ_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

خالی V3_گراف::ترتیب_دو_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::سمتیہ<V3_گراف_راس*> vertexps;
    برائے (V3_گراف_راس& راس : m_vertices) vertexps.ڈالو_عقب(&راس);
    std::stable_ترتیب_دو(vertexps.begin(), vertexps.انتہا(),
                     [](ثابت V3_گراف_راس* lhsp, ثابت V3_گراف_راس* rhsp) {  //
                         واپس lhsp->ترتیب_دو_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    برائے (V3_گراف_راس* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_عقب(vertexp);
    }
}

خالی V3_گراف::ترتیب_دو_کنارے() {
    // Sort edges by rank then fanout of node they point to
    std::سمتیہ<V3_گراف_کنارہ*> کنارے;
    برائے (V3_گراف_راس& راس : vertices()) {
        // Make a vector
        برائے (V3_گراف_کنارہ& کنارہ : راس.برآمد_کنارے()) کنارے.ڈالو_عقب(&کنارہ);
        //ترتیب_دو
        std::stable_ترتیب_دو(کنارے.begin(), کنارے.انتہا(),
                         [](ثابت V3_گراف_کنارہ* lhsp, ثابت V3_گراف_کنارہ* rhsp) {  //
                             واپس lhsp->ترتیب_دو_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        برائے (V3_گراف_کنارہ* ثابت edgep : کنارے) edgep->relinkFromp(&راس);
        // Prep for next
        کنارے.clear();
    }
}

//######################################################################
//######################################################################
// Algorithms - ordering
//      Compute near optimal ordering of the nodes, where:
//          If a required    edge is A->B, rank(A)<rank(B)
//          Visit edges and assign ranks to keep minimal crossings
//              (Results in better dcache packing.)

خالی V3_گراف::ترتیب() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_گراف_کنارہ::followAlwaysTrue);
    ترتیب_Pre_Ranked();
}

خالی V3_گراف::ترتیب_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    برائے (V3_گراف_راس& راس : vertices()) {
        اگر (!راس.user()) ترتیب_DFSIterate(&راس);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ترتیب_دو_Vertices();
    // Sort edges by rank then fanout of node they point to
    ترتیب_دو_کنارے();
}

دوہرا V3_گراف::ترتیب_DFSIterate(V3_گراف_راس* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    اگر (vertexp->user() == 2) واپس vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    دوہرا fanout = 0;
    برائے (V3_گراف_کنارہ& کنارہ : vertexp->برآمد_کنارے()) {
        اگر (کنارہ.وزن_2()) fanout += ترتیب_DFSIterate(کنارہ.چوٹی());
    }
    // Just count inbound edges
    برائے (V3_گراف_کنارہ& کنارہ : vertexp->اندر_کنارے()) {
        اگر (کنارہ.وزن_2()) ++fanout;
    }
    vertexp->fanout(fanout);
    vertexp->user(2);
    واپس vertexp->fanout();
}

//######################################################################
//######################################################################
// Algorithms - parallelism report

جماعت گراف_Alg_Parallelism_Report final {
    // MEMBERS
    V3_گراف& m_گراف;  // The graph
    ثابت std::function<uint64_t(ثابت V3_گراف_راس*)> m_راس_لاگت;  // vertex cost function
    V3_گراف::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    صریح گراف_Alg_Parallelism_Report(V3_گراف& گراف,
                                       std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت)
        : m_گراف{گراف}
        , m_راس_لاگت{راس_لاگت} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3_گراف_راس*, uint64_t> critPaths;
        گراف_دھارا_Unordered serialize{&m_گراف};
        برائے (ثابت V3_گراف_راس* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_راس_گنتی;
            uint64_t cp_لاگت_مقصد_یہاں = 0;
            برائے (ثابت V3_گراف_کنارہ& کنارہ : vertexp->اندر_کنارے()) {
                ++m_report.m_کنارہ_گنتی;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_لاگت_مقصد_یہاں = std::max(cp_لاگت_مقصد_یہاں, critPaths[کنارہ.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_لاگت_مقصد_یہاں += m_راس_لاگت(vertexp);
            critPaths[vertexp] = cp_لاگت_مقصد_یہاں;
            m_report.m_critical_راستہ_لاگت = std::max(m_report.m_critical_راستہ_لاگت, cp_لاگت_مقصد_یہاں);
            // Tally the total cost contributed by vertices.
            m_report.m_کل_گراف_لاگت += m_راس_لاگت(vertexp);
        }
    }
    ~گراف_Alg_Parallelism_Report() = طےشدہ;
    VL_UNCOPYABLE(گراف_Alg_Parallelism_Report);
    VL_UNMOVABLE(گراف_Alg_Parallelism_Report);

عوامی:
    ساکن V3_گراف::ParallelismReport
    apply(V3_گراف& گراف, std::function<uint32_t(ثابت V3_گراف_راس*)> راس_لاگت) {
        واپس گراف_Alg_Parallelism_Report(گراف, راس_لاگت).m_report;
    }
};

V3_گراف::ParallelismReport
V3_گراف::parallelismReport(std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت) {
    واپس گراف_Alg_Parallelism_Report::apply(*یہ, راس_لاگت);
}
