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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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
    ثابت منطقي m_جوڑ_Weights;  // Sum, rather then maximize weights
خاص:
    فراغ مکھ() {
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) راس_Iterate(&راس);
    }
    فراغ راس_Iterate(V3_گراف_راس* vertexp) {
        // Clear marks
        لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) کنارا.چوٹی()->userp(خالی_سوچک);

        // Mark edges and detect duplications

        لكل (V3_گراف_کنارا* ثابت edgep : vertexp->باہرلا_کنارے().unlinkable()) {
            إذا (follow_کنارا(edgep)) {
                V3_گراف_راس* باہرلا_Vertexp = edgep->چوٹی();
                V3_گراف_کنارا* prevEdgep = static_cast<V3_گراف_کنارا*>(باہرلا_Vertexp->userp());
                إذا (!prevEdgep) {  // No previous assignment
                    باہرلا_Vertexp->userp(edgep);
                } وإلا {  // Duplicate
                    منطقي saveOld = سچ;
                    إذا (prevEdgep->cutable() && !edgep->cutable()) {
                        saveOld = خطأ;  // new !cutable more important than old
                    } وإلا إذا (!prevEdgep->cutable() && edgep->cutable()) {
                        saveOld = سچ;  // old !cutable more important than new
                    } وإلا {
                        saveOld = سچ;
                        إذا (!m_جوڑ_Weights
                            && (prevEdgep->بھار_2() < edgep->بھار_2())) {  // Keep max weight
                            prevEdgep->بھار_2(edgep->بھار_2());
                        }
                    }
                    إذا (saveOld) {
                        إذا (m_جوڑ_Weights) prevEdgep->بھار_2(prevEdgep->بھار_2() + edgep->بھار_2());
                        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    } وإلا {
                        إذا (m_جوڑ_Weights) edgep->بھار_2(prevEdgep->بھار_2() + edgep->بھار_2());
                        VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                        باہرلا_Vertexp->userp(edgep);
                    }
                }
            }
        }
    }

عام:
    گراف_Remove_Redundant(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp, منطقي جوڑ_Weights)
        : گراف_Alg<>{graphp, کنارا_Funcp}
        , m_جوڑ_Weights{جوڑ_Weights} {
        مکھ();
    }
    ~گراف_Remove_Redundant() = افتراضي;
};

فراغ V3_گراف::remove_Redundant_کنارے_Max(V3_کنارا_Func_P کنارا_Funcp) {
    گراف_Remove_Redundant{هذا, کنارا_Funcp, خطأ};
}
فراغ V3_گراف::remove_Redundant_کنارے_جوڑ(V3_کنارا_Func_P کنارا_Funcp) {
    گراف_Remove_Redundant{هذا, کنارا_Funcp, سچ};
}

//######################################################################
//######################################################################
// Algorithms - remove transitive

صنف گراف_Alg_Remove_متعدی_کنارے final : گراف_Alg<> {
عام:
    واضح گراف_Alg_Remove_متعدی_کنارے(V3_گراف* graphp)
        : گراف_Alg<>(graphp, خالی_سوچک) {}
    فراغ go() {
        گراف_رستہ_Checker checker{m_graphp};
        لكل (V3_گراف_راس& vtx : m_graphp->vertices()) {
            V3_گراف_کنارا* deletep = خالی_سوچک;
            لكل (V3_گراف_کنارا& کنارا : vtx.باہرلا_کنارے()) {
                إذا (deletep) VL_DO_CLEAR(deletep->unlinkDelete(), deletep = خالی_سوچک);
                // It should be safe to modify the graph, despite using
                // the GraphPathChecker, as none of the modifications will
                // change what can be reached from what, nor should they
                // change the rank or CP of any node.
                إذا (checker.is_متعدی_کنارا(&کنارا)) deletep = &کنارا;
            }
            إذا (deletep) VL_DO_DANGLING(deletep->unlinkDelete(), deletep);
        }
    }

خاص:
    VL_UNCOPYABLE(گراف_Alg_Remove_متعدی_کنارے);
};

فراغ V3_گراف::remove_متعدی_کنارے() { گراف_Alg_Remove_متعدی_کنارے{هذا}.go(); }

//######################################################################
//######################################################################
// Algorithms - weakly connected components
// Changes color()

صنف گراف_Alg_Weakly final : گراف_Alg<> {
    // Sentinel color, meaning not colored yet. Colors themselves are 0 .. m_numColors-1.
    ساكن constexpr uint32_t UNCOLORED = std::numeric_limits<uint32_t>::max();

    uint32_t m_عدد_Colors = 0;  // Number of colors assigned

    فراغ مکھ() {
        // Initialize state
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) راس.color(UNCOLORED);
        // Color graph, without gaps
        لكل (V3_گراف_راس& راس : m_graphp->vertices()) {
            إذا (راس.color() == UNCOLORED) راس_Iterate(&راس, m_عدد_Colors++);
        }
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentColor) {
        // Assign new color to each unvisited node
        // then visit each of its edges, giving them the same color
        إذا (vertexp->color() != UNCOLORED) أرجع;  // Already colored it
        vertexp->color(currentColor);
        لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
            إذا (follow_کنارا(&کنارا)) راس_Iterate(کنارا.چوٹی(), currentColor);
        }
        لكل (V3_گراف_کنارا& کنارا : vertexp->وچ_کنارے()) {
            إذا (follow_کنارا(&کنارا)) راس_Iterate(کنارا.fromp(), currentColor);
        }
    }

عام:
    گراف_Alg_Weakly(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp)
        : گراف_Alg<>(graphp, کنارا_Funcp) {
        مکھ();
    }
    ~گراف_Alg_Weakly() = افتراضي;

    uint32_t عدد_Colors() ثابت { أرجع m_عدد_Colors; }
};

uint32_t V3_گراف::weaklyConnected(V3_کنارا_Func_P کنارا_Funcp) {
    أرجع گراف_Alg_Weakly{هذا, کنارا_Funcp}.عدد_Colors();
}

//######################################################################
//######################################################################
// Algorithms - strongly connected components
// Changes user() and color()

صنف گراف_Alg_Strongly final : گراف_Alg<> {
    uint32_t m_currentDfs = 0;  // DFS count
    std::سمتیہ<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far

    فراغ مکھ() {
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
            لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) {
                إذا (follow_کنارا(&کنارا)) {
                    إذا (راس.color() == کنارا.چوٹی()->color()) {
                        onecolor = خطأ;
                        اكسر;
                    }
                }
            }
            إذا (onecolor) راس.color(0);
        }
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp) {
        ثابت uint32_t this_Dfs_عدد = m_currentDfs++;
        vertexp->user(this_Dfs_عدد);
        vertexp->color(0);
        لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
            إذا (follow_کنارا(&کنارا)) {
                V3_گراف_راس* چوٹی = کنارا.چوٹی();
                إذا (!چوٹی->user()) {  // Dest not computed yet
                    راس_Iterate(چوٹی);
                }
                إذا (!چوٹی->color()) {  // Dest not in a component
                    إذا (vertexp->user() > چوٹی->user()) vertexp->user(چوٹی->user());
                }
            }
        }
        إذا (vertexp->user() == this_Dfs_عدد) {  // New head of subtree
            vertexp->color(this_Dfs_عدد);  // Mark as component
            طالما (!m_callTrace.خالی_تھاواں()) {
                V3_گراف_راس* نکالو_Vertexp = m_callTrace.پٹھ();
                إذا (نکالو_Vertexp->user() >= this_Dfs_عدد) {  // Lower node is part of this subtree
                    m_callTrace.نکالو_پٹھ();
                    نکالو_Vertexp->color(this_Dfs_عدد);
                } وإلا {
                    اكسر;
                }
            }
        } وإلا {  // In another subtree (maybe...)
            m_callTrace.ڈالو_پٹھ(vertexp);
        }
    }

عام:
    گراف_Alg_Strongly(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp)
        : گراف_Alg<>{graphp, کنارا_Funcp} {
        مکھ();
    }
    ~گراف_Alg_Strongly() = افتراضي;
};

فراغ V3_گراف::stronglyConnected(V3_کنارا_Func_P کنارا_Funcp) { گراف_Alg_Strongly{هذا, کنارا_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking
// Changes user() and rank()

صنف گراف_Alg_Rank final : گراف_Alg<> {
    فراغ مکھ() {
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
            m_graphp->loops_سنیہا_Cb(vertexp, m_کنارا_Funcp);
            أرجع;  // LCOV_EXCL_LINE  // gcc gprof bug misses this return
        }
        إذا (vertexp->rank() >= currentRank) أرجع;  // Already processed it
        vertexp->user(1);
        vertexp->rank(currentRank);
        لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
            إذا (follow_کنارا(&کنارا)) راس_Iterate(کنارا.چوٹی(), currentRank + vertexp->rankAdder());
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_Rank(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp)
        : گراف_Alg<>{graphp, کنارا_Funcp} {
        مکھ();
    }
    ~گراف_Alg_Rank() = افتراضي;
};

فراغ V3_گراف::rank() { rank(&V3_گراف_کنارا::followAlwaysTrue); }
فراغ V3_گراف::rank(V3_کنارا_Func_P کنارا_Funcp) { گراف_Alg_Rank{هذا, کنارا_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - ranking min
// Changes user() and rank()

صنف گراف_Alg_Rank_Min final : گراف_Alg<> {
    فراغ مکھ() {
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
            m_graphp->loops_سنیہا_Cb(vertexp, m_کنارا_Funcp);
            أرجع vertexp->rank();
        }
        إذا (vertexp->user()) أرجع vertexp->rank();  // Done earlier
        vertexp->user(1);
        vertexp->rank(1);  // In case loop
        // If no input edges, then rank 1 (+ adder)
        // Otherwise, get minimum from following all inputs.
        uint32_t minrank = ~0U;
        لكل (V3_گراف_کنارا& کنارا : vertexp->وچ_کنارے()) {
            إذا (follow_کنارا(&کنارا)) {
                ثابت uint32_t nrank = راس_Iterate(کنارا.fromp());
                minrank = std::min(minrank, nrank);
            }
        }
        إذا (minrank == ~0U) minrank = 0;
        vertexp->rank(minrank + vertexp->rankAdder());
        vertexp->user(2);
        أرجع vertexp->rank();
    }

عام:
    گراف_Alg_Rank_Min(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp)
        : گراف_Alg<>{graphp, کنارا_Funcp} {
        مکھ();
    }
    ~گراف_Alg_Rank_Min() = افتراضي;
};

فراغ V3_گراف::rankMin() { rankMin(&V3_گراف_کنارا::followAlwaysTrue); }
فراغ V3_گراف::rankMin(V3_کنارا_Func_P کنارا_Funcp) { گراف_Alg_Rank_Min{هذا, کنارا_Funcp}; }

//######################################################################
//######################################################################
// Algorithms - report loops
// Changes user()

صنف گراف_Alg_RLoops final : گراف_Alg<> {
    std::سمتیہ<V3_گراف_راس*> m_callTrace;  // List of everything we hit processing so far
    std::سمتیہ<string> m_msgs;  // Output messages
    منطقي m_پورا = خطأ;  // Exit algorithm

    فراغ مکھ(V3_گراف_راس* vertexp) {
        // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
        // Clear existing ranks
        m_graphp->userClearVertices();
        m_callTrace.reserve(100);
        راس_Iterate(vertexp, 0);
    }

    فراغ راس_Iterate(V3_گراف_راس* vertexp, uint32_t currentRank) {
        // Assign rank to each unvisited node
        // When we hit ourself again, return the list of all loops
        إذا (m_پورا) أرجع;

        // Can't just reserve(), unless we modify size() before setting array directly
        طالما (m_callTrace.ناپ() <= currentRank) m_callTrace.ڈالو_پٹھ(vertexp);
        m_callTrace[currentRank++] = vertexp;

        إذا (vertexp->user() == 1) {
            لكل (بنانشانی i = 0; i < currentRank; i++)
                m_msgs.emplace_پٹھ(m_graphp->loops_راس_Cb(m_callTrace[i]));
            m_پورا = سچ;
            أرجع;
        }
        إذا (vertexp->user() == 2) أرجع;  // Already processed it
        vertexp->user(1);
        لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
            إذا (follow_کنارا(&کنارا)) راس_Iterate(کنارا.چوٹی(), currentRank);
        }
        vertexp->user(2);
    }

عام:
    گراف_Alg_RLoops(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp, V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, کنارا_Funcp} {
        مکھ(vertexp);
    }
    ~گراف_Alg_RLoops() = افتراضي;
    string سنیہا() ثابت {
        أرجع std::accumulate(m_msgs.begin(), m_msgs.سرا(), std::string{""});
    }
};

string V3_گراف::reportLoops(V3_کنارا_Func_P کنارا_Funcp, V3_گراف_راس* vertexp) {
    أرجع گراف_Alg_RLoops{هذا, کنارا_Funcp, vertexp}.سنیہا();
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

            لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
                إذا (follow_کنارا(&کنارا)) {
                    V3_گراف_کنارا* newEdgep = static_cast<V3_گراف_کنارا*>(کنارا.userp());
                    إذا (!newEdgep) {
                        V3_گراف_راس* new_چوٹی = راس_Iterate_All(کنارا.چوٹی());
                        newEdgep = کنارا.clone(m_loopGraphp, newVertexp, new_چوٹی);
                        کنارا.userp(newEdgep);
                    }
                }
            }
            أرجع newVertexp;
        }
    }

عام:
    گراف_Alg_Subtrees(V3_گراف* graphp, V3_گراف* loopGraphp, V3_کنارا_Func_P کنارا_Funcp,
                     V3_گراف_راس* vertexp)
        : گراف_Alg<>{graphp, کنارا_Funcp}
        , m_loopGraphp{loopGraphp} {
        // Vertex::m_userp - New vertex if we have seen this vertex already
        // Edge::m_userp - New edge if we have seen this edge already
        m_graphp->userClearVertices();
        m_graphp->user_Clear_کنارے();
        (فراغ)راس_Iterate_All(vertexp);
    }
    ~گراف_Alg_Subtrees() = افتراضي;
};

//! Report the entire connected graph with a loop or loops
فراغ V3_گراف::subtreeLoops(V3_کنارا_Func_P کنارا_Funcp, V3_گراف_راس* vertexp, V3_گراف* loopGraphp) {
    گراف_Alg_Subtrees{هذا, loopGraphp, کنارا_Funcp, vertexp};
}

//######################################################################
//######################################################################
// Algorithms - sorting

فراغ V3_گراف::ترتیب_دیو_Vertices() {
    // Sort list of vertices by rank, then fanout
    std::سمتیہ<V3_گراف_راس*> vertexps;
    لكل (V3_گراف_راس& راس : m_vertices) vertexps.ڈالو_پٹھ(&راس);
    std::stable_ترتیب_دیو(vertexps.begin(), vertexps.سرا(),
                     [](ثابت V3_گراف_راس* lhsp, ثابت V3_گراف_راس* rhsp) {  //
                         أرجع lhsp->ترتیب_دیو_Cmp(rhsp) < 0;
                     });
    // Re-insert in sorted order
    لكل (V3_گراف_راس* ثابت vertexp : vertexps) {
        m_vertices.unlink(vertexp);
        m_vertices.link_پٹھ(vertexp);
    }
}

فراغ V3_گراف::ترتیب_دیو_کنارے() {
    // Sort edges by rank then fanout of node they point to
    std::سمتیہ<V3_گراف_کنارا*> کنارے;
    لكل (V3_گراف_راس& راس : vertices()) {
        // Make a vector
        لكل (V3_گراف_کنارا& کنارا : راس.باہرلا_کنارے()) کنارے.ڈالو_پٹھ(&کنارا);
        //ترتیب_دیو
        std::stable_ترتیب_دیو(کنارے.begin(), کنارے.سرا(),
                         [](ثابت V3_گراف_کنارا* lhsp, ثابت V3_گراف_کنارا* rhsp) {  //
                             أرجع lhsp->ترتیب_دیو_Cmp(rhsp) < 0;
                         });
        // Relink edges in specified order
        لكل (V3_گراف_کنارا* ثابت edgep : کنارے) edgep->relinkFromp(&راس);
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

فراغ V3_گراف::ترتیب() {
    UINFO(2, "Order:");

    // Compute rankings again
    rank(&V3_گراف_کنارا::followAlwaysTrue);
    ترتیب_Pre_Ranked();
}

فراغ V3_گراف::ترتیب_Pre_Ranked() {
    // Compute fanouts
    // Vertex::m_user begin: 1 indicates processing, 2 indicates completed
    userClearVertices();
    لكل (V3_گراف_راس& راس : vertices()) {
        إذا (!راس.user()) ترتیب_DFSIterate(&راس);
    }

    // Sort list of vertices by rank, then fanout. Fanout is a bit of a
    // misnomer. It is the sum of all the fanouts of nodes reached from a node
    // *plus* the count of edges in to that node.
    ترتیب_دیو_Vertices();
    // Sort edges by rank then fanout of node they point to
    ترتیب_دیو_کنارے();
}

مزدوج V3_گراف::ترتیب_DFSIterate(V3_گراف_راس* vertexp) {
    // Compute fanouts of each node
    // If forward edge, don't double count that fanout
    إذا (vertexp->user() == 2) أرجع vertexp->fanout();  // Already processed it
    UASSERT_OBJ(vertexp->user() != 1, vertexp, "Loop found, backward edges should be dead");
    vertexp->user(1);
    مزدوج fanout = 0;
    لكل (V3_گراف_کنارا& کنارا : vertexp->باہرلا_کنارے()) {
        إذا (کنارا.بھار_2()) fanout += ترتیب_DFSIterate(کنارا.چوٹی());
    }
    // Just count inbound edges
    لكل (V3_گراف_کنارا& کنارا : vertexp->وچ_کنارے()) {
        إذا (کنارا.بھار_2()) ++fanout;
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
    ثابت std::function<uint64_t(ثابت V3_گراف_راس*)> m_راس_لاگت;  // vertex cost function
    V3_گراف::ParallelismReport m_report;  // The result report

    // CONSTRUCTORS
    واضح گراف_Alg_Parallelism_Report(V3_گراف& گراف,
                                       std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت)
        : m_گراف{گراف}
        , m_راس_لاگت{راس_لاگت} {
        // For each node, record the critical path cost from the start
        // of the graph through the end of the node.
        std::unordered_map<ثابت V3_گراف_راس*, uint64_t> critPaths;
        گراف_دھارا_Unordered serialize{&m_گراف};
        لكل (ثابت V3_گراف_راس* vertexp; (vertexp = serialize.nextp());) {
            ++m_report.m_راس_گنتی;
            uint64_t cp_لاگت_منزل_ایتھے = 0;
            لكل (ثابت V3_گراف_کنارا& کنارا : vertexp->وچ_کنارے()) {
                ++m_report.m_کنارا_گنتی;
                // For each upstream item, add its critical path cost to
                // the cost of this edge, to form a new candidate critical
                // path cost to the current node. Whichever is largest is
                // the critical path to reach the start of this node.
                cp_لاگت_منزل_ایتھے = std::max(cp_لاگت_منزل_ایتھے, critPaths[کنارا.fromp()]);
            }
            // Include the cost of the current vertex in the critical
            // path, so it represents the critical path to the end of
            // this vertex.
            cp_لاگت_منزل_ایتھے += m_راس_لاگت(vertexp);
            critPaths[vertexp] = cp_لاگت_منزل_ایتھے;
            m_report.m_critical_رستہ_لاگت = std::max(m_report.m_critical_رستہ_لاگت, cp_لاگت_منزل_ایتھے);
            // Tally the total cost contributed by vertices.
            m_report.m_کل_گراف_لاگت += m_راس_لاگت(vertexp);
        }
    }
    ~گراف_Alg_Parallelism_Report() = افتراضي;
    VL_UNCOPYABLE(گراف_Alg_Parallelism_Report);
    VL_UNMOVABLE(گراف_Alg_Parallelism_Report);

عام:
    ساكن V3_گراف::ParallelismReport
    apply(V3_گراف& گراف, std::function<uint32_t(ثابت V3_گراف_راس*)> راس_لاگت) {
        أرجع گراف_Alg_Parallelism_Report(گراف, راس_لاگت).m_report;
    }
};

V3_گراف::ParallelismReport
V3_گراف::parallelismReport(std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت) {
    أرجع گراف_Alg_Parallelism_Report::apply(*هذا, راس_لاگت);
}
