// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Multi-threaded MTask graph data hazard fixing
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
// The fine-grained graph from V3Order may contain data hazards which are not a problem for
// serial mode, but which would be a problem in parallel mode. There are two classes:
// unordered pairs of writes, and unordered write-read pairs. This transform adds edges here
// until no such unordered pair remains.
//
// ABOUT UNORDERED WRITE-WRITE PAIRS
//
//   The V3Order dependency graph treats these as unordered events:
//
//    a)  sig[15:8] = stuff;
//          ...
//    b)  sig[7:0]  = other_stuff;
//
//   They are writes to disjoint bits of the same signal. They can run in
//   either order, in serial mode, and the result will be the same.
//
//   The resulting C code for each of this isn't a pure write, it's actually an R-M-W
//   sequence:
//
//    a)  sig = (sig & 0xff)   | (0xff00 & (stuff << 8));
//          ...
//    b)  sig = (sig & 0xff00) | (0xff & other_stuff);
//
//   In serial mode, order doesn't matter as these run serially. In parallel mode,
//   they must be serialized to avoid a race.
//
//   This pass does not check if each write would involve an R-M-W, it just assumes that
//   it does. If this routine ever causes a drastic increase in critical path, it could be
//   optimized to make a better prediction (with all the risk that word implies!) about
//   whether a given write is likely to turn into an R-M-W.
//
// ABOUT UNORDERED WRITE-READ PAIRS
//
//   If we don't put unordered write-read pairs into some order at Verilation time, we risk
//   a runtime race.
//
//   These arise because the OrderGraph deliberately does not model every access. A read of
//   a variable that is in the reading block's own hybrid sensitivity list gets no edge, as
//   does a read ignored due to a force/release, or an access to a variable marked
//   'ignoreSchedWrite' and friends. For serial mode that is fine: whatever order the logic
//   ends up in, it runs one block at a time. In parallel mode two such blocks can run
//   concurrently, and if one of them writes what the other reads, that is an observable
//   data race.
//
// HOW TO FIX THEM
//
//   An arbitrary ordering is prescribed by adding edges between MTasks. The new edges
//   must not create a cycle, and should be added in a way that increases critical paths
//   as little as possible.
//
//   Every MTask is given a unique sequence number, in a topological order of the graph, so
//   that for every edge 'from -> to' we have seq(from) < seq(to). Adding a new edge that
//   runs in increasing sequence order therefore cannot create a cycle, and the property is
//   maintained by induction as more edges are added.
//
//   With that in hand, for each variable we take its accessors in sequence order, chain the
//   writers together, and bracket each reader between the writers either side of it.
//   Readers need no ordering with respect to one another.
//
//   The sequence numbers are assigned by a topological sort that emits the ready MTask with
//   the smallest forward critical path first. As the forward critical path of an MTask is at
//   least that of each of its predecessors, the smallest among the ready MTasks is also the
//   smallest among all not yet emitted ones, so MTasks come out in globally non-decreasing
//   forward critical path order. Chaining them in that order is what tends to grow the
//   critical path least: the longest path through a chain starts at the head of its first
//   MTask, so putting those with the shortest path to them first keeps that sum down.
//
//   The critical paths are also what makes finding the edges that are actually needed cheap.
//   An edge is only added between a pair that is not ordered already, and a pair can be
//   ruled ordered, or not, in constant time whenever their critical paths are inconsistent
//   with a path between them, which avoids searching the graph for most pairs.
//
//   SystemC variables are handled similarly, except that all SystemC variables are treated as
//   a single entity. Reasoning: writing a systemC var actually turns into a call to a var.write()
//   method, which under the hood is accessing some data structure that's shared by many SC vars.
//   It's not thread safe.
//
//   DPI calls are serialized similarly (they are assumed not thread safe), unless directed by
//   options.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// DpiImportCallVisitor

// Scan node, indicate whether it contains a call to a DPI imported routine.
ವರ್ಗ DpiImportCallVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ತಾರ್ಕಿಕ m_hasDpiHazard = ಸುಳ್ಳು;  // Found a DPI import call.
    ತಾರ್ಕಿಕ m_tracingCall = ಸುಳ್ಳು;  // Iterating into a CCall to a CFunc
    // METHODS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (!m_tracingCall) ಹಿಂತಿರುಗಿ;
        m_tracingCall = ಸುಳ್ಳು;
        ಆದರೆ (nodep->dpiImportWrapper()) {
            ಆದರೆ (nodep->dpi_ಶುದ್ಧ() ? !v3Global.opt.ಎಳೆಗಳು_Dpi_ಶುದ್ಧ()
                                 : !v3Global.opt.ಎಳೆಗಳು_Dpi_Unpure()) {
                // If hierarchical DPI wrapper cost is not found or is of a 0 cost,
                // we have a normal DPI which induces DPI hazard by default.
                m_hasDpiHazard = V3Control::get_Profile_ದತ್ತಾಂಶ(nodep->cname()) == 0;
                UINFO(9, "DPI wrapper '" << nodep->cname()
                                         << "' has dpi hazard = " << m_hasDpiHazard);
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildren(nodep);
        // Enter the function and trace it
        m_tracingCall = ನಿಜ;
        iterate(nodep->funcp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DpiImportCallVisitor(Ast_ಗಂಟು* nodep) { iterate(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasDpiHazard(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ DpiImportCallVisitor{nodep}.m_hasDpiHazard; }
};

//######################################################################
// FixDataHazards

ವರ್ಗ Fix_ದತ್ತಾಂಶ_Hazards final {
    // TYPES

    // Access to one variable, with the MTask performing it
    ರಚನೆ Access final {
        ಸ್ಥಿರಾಂಕ AstVarScope* m_vscp;  // The variable accessed, its index is in 'user1'
        LogicMTask* m_mtaskp;  // The accessing MTask
        VAccess m_access;  // The kind of access
    };

    // NODE STATE
    //  AstVarScope::user1    -> int: Variable index for stable sorting
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // MEMBERS
    ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_m_Task_ನಕ್ಷೆ;  // The Mtask graph
    std::ಸದಿಶ<LogicMTask*> m_scratch;  // Scratch MTask list, reused to avoid reallocation

    // METHODS

    // Assign each MTask a unique sequence number, held in 'user()', such that for every edge
    // 'from -> to' we have seq(from) < seq(to). See the note at top of file on why this matters.
    // This is a topological sort using Kahn's algorithm with MTasks enumerated in a globally
    // non-decreasing critical path order.
    ಶೂನ್ಯ ನಿಯೋಜಿಸು_Sequence_Numbers() {
        ರಚನೆ MTaskCmp final {
            ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ LogicMTask* ap, ಸ್ಥಿರಾಂಕ LogicMTask* bp) ಸ್ಥಿರಾಂಕ {
                // Order by critical path
                ಸ್ಥಿರಾಂಕ uint64_t aCp = ap->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>();
                ಸ್ಥಿರಾಂಕ uint64_t bCp = bp->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>();
                ಆದರೆ (aCp != bCp) ಹಿಂತಿರುಗಿ aCp < bCp;
                // Break ties by stable id
                ಹಿಂತಿರುಗಿ *ap < *bp;
            }
        };
        // Set of ready vertices. Initialized to the entry MTask.
        std::ಗಣ<LogicMTask*, MTaskCmp> ready{m_m_Task_ನಕ್ಷೆ.entryp()};
        // 'user' also used to count remaining dependencies of each MTask. Initialize it.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_m_Task_ನಕ್ಷೆ.vertices()) {
            vtx.user(static_cast<uint32_t>(vtx.ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2()));
        }
        // Next sequence number to assign to each MTask.
        uint32_t seq = 0;
        // Process ready vertices in critical path order.
        ಆಗ (!ready.ಖಾಲಿಗಳು()) {
            // Pick up and detach the ready MTask with the smallest critical path.
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ready.begin();
            LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = *it;
            ready.erase(it);
            // Assign the next sequence number to the MTask
            mtaskp->user(++seq);
            // Decrement edge count of successors and add to ready set if no dependencies left
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                LogicMTask* ಸ್ಥಿರಾಂಕ ತುದಿ = static_cast<LogicMTask*>(ಅಂಚು.ತುದಿ());
                ಸ್ಥಿರಾಂಕ uint32_t nDeps = ತುದಿ->user() - 1;
                ತುದಿ->user(nDeps);
                ಆದರೆ (!nDeps) ready.ಸೇರಿಸು(ತುದಿ);
            }
        }
        UASSERT(seq == m_m_Task_ನಕ್ಷೆ.vertices().ಗಾತ್ರ_2(), "MTask graph is cyclic");
    }

    // Gather every variable access, resolved to the MTask performing it.
    std::ಸದಿಶ<Access> gatherAccesses() {
        std::ಸದಿಶ<Access> accesses;
        ಪೂರ್ಣಾಂಕ nVars = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_m_Task_ನಕ್ಷೆ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Move_ಶೃಂಗ& mVtx : mtask.ಶೃಂಗ_List()) {
                ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = mVtx.logicp();
                ಆದರೆ (!lVtxp) ಮುಂದುವರಿ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ::VarAccess& acc : lVtxp->varAccesses()) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = acc.m_vscp;
                    ಆದರೆ (!vscp->user1()) vscp->user1(++nVars);
                    accesses.ತಳ್ಳು_ಹಿಂಭಾಗ({vscp, &mtask, acc.m_access});
                }
            }
        }
        ಹಿಂತಿರುಗಿ accesses;
    }

    // Add an edge ordering 'fromp' before 'top', unless they are already ordered
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು_If_Needed(LogicMTask* fromp, LogicMTask* ತುದಿ) {
        // Nothing to order within a single MTask
        ಆದರೆ (fromp == ತುದಿ) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(fromp->user() < ತುದಿ->user(), fromp,
                    "Edge must run in increasing sequence order");
        // Already directly ordered. This is an O(1) set lookup, and catches the common case of
        // a dependency the OrderGraph already provided.
        ಆದರೆ (fromp->has_ಅಂಚು_ಗಮ್ಯ(ತುದಿ)) ಹಿಂತಿರುಗಿ;
        // Otherwise check if already ordered
        ಆದರೆ (m_m_Task_ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(fromp, ತುದಿ, ಶೂನ್ಯಸೂಚಕ)) ಹಿಂತಿರುಗಿ;
        // Unordered. Add an edge between them.
        m_m_Task_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(fromp, ತುದಿ);
    }

    // Add the edges required to make the accesses of one variable race free. 'beginp'/'endp'
    // delimit the accesses of a single variable, in sequence number order.
    ಶೂನ್ಯ serializeVariable(ಸ್ಥಿರಾಂಕ Access* beginp, ಸ್ಥಿರಾಂಕ Access* endp) {
        // Nothing to order if at most one MTask accesses this variable. Each MTask appears at
        // most once in the range, see the assertion below, so this is just the range size.
        ಆದರೆ (endp - beginp < 2) ಹಿಂತಿರುಗಿ;

        std::ಸದಿಶ<LogicMTask*>& pendingReaders = m_scratch;
        pendingReaders.clear();
        LogicMTask* prevWriterp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Access* accp = beginp; accp != endp; ++accp) {
            // The OrderLogicVertices record one access per variable, and at this point each MTask
            // holds exactly one logic block, so an MTask cannot appear twice here.
            UASSERT_OBJ(accp == beginp || accp[-1].m_mtaskp != accp->m_mtaskp, accp->m_mtaskp,
                        "Multiple accesses of a variable in an MTask");
            LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = accp->m_mtaskp;
            ಆದರೆ (accp->m_access.isWriteOrRW()) {
                // Reads since the previous write must complete before this write
                ಗಾಗಿ (LogicMTask* ಸ್ಥಿರಾಂಕ readerp : pendingReaders) ಕೂಡಿಸು_ಅಂಚು_If_Needed(readerp, mtaskp);
                // Consecutive writes must be ordered. If a read came between them, the edges to
                // and from that read imply it already.
                ಆದರೆ (prevWriterp && pendingReaders.ಖಾಲಿಗಳು()) ಕೂಡಿಸು_ಅಂಚು_If_Needed(prevWriterp, mtaskp);
                pendingReaders.clear();
                prevWriterp = mtaskp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // This read must happen after the preceding write
                ಆದರೆ (prevWriterp) ಕೂಡಿಸು_ಅಂಚು_If_Needed(prevWriterp, mtaskp);
                pendingReaders.ತಳ್ಳು_ಹಿಂಭಾಗ(mtaskp);
            }
        }
    }

    // Serialize the given MTasks, in sequence number order
    ಶೂನ್ಯ serializeMTasks(std::ಸದಿಶ<LogicMTask*>& mtaskps) {
        std::ವಿಂಗಡಿಸು(mtaskps.begin(), mtaskps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ LogicMTask* ap, ಸ್ಥಿರಾಂಕ LogicMTask* bp) {
            ಹಿಂತಿರುಗಿ ap->user() < bp->user();
        });
        mtaskps.erase(std::unique(mtaskps.begin(), mtaskps.ಕೊನೆ()), mtaskps.ಕೊನೆ());
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < mtaskps.ಗಾತ್ರ_2(); ++i) ಕೂಡಿಸು_ಅಂಚು_If_Needed(mtaskps[i - 1], mtaskps[i]);
    }

    ತಾರ್ಕಿಕ hasDpiHazard(LogicMTask* mtaskp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Move_ಶೃಂಗ& mVtx : mtaskp->ಶೃಂಗ_List()) {
            ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = mVtx.logicp();
            ಆದರೆ (!lvtxp) ಮುಂದುವರಿ;
            // NOTE: We don't handle DPI exports. If testbench code calls a DPI-exported function
            // at any time during eval() we may have a data hazard. (Likewise in non-threaded mode
            // if an export messes with an ordered variable we're broken.)

            // Find all calls to DPI-imported functions, we can put those into a serial order at
            // least. That should solve the most likely DPI-related data hazards.
            ಆದರೆ (DpiImportCallVisitor::hasDpiHazard(lvtxp->nodep())) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // CONSTRUCTOR
    Fix_ದತ್ತಾಂಶ_Hazards(ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_Task_ನಕ್ಷೆ)
        : m_m_Task_ನಕ್ಷೆ{m_Task_ನಕ್ಷೆ} {
        // Give the MTasks a total order consistent with their dependencies
        ನಿಯೋಜಿಸು_Sequence_Numbers();

        // Gather variable accesses made by each MTask
        std::ಸದಿಶ<Access> accesses = gatherAccesses();
        // Sort by variable (to group by variable), then by sequence number of the accessing MTask
        std::ವಿಂಗಡಿಸು(accesses.begin(), accesses.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Access& a, ಸ್ಥಿರಾಂಕ Access& b) {
            ಆದರೆ (a.m_vscp != b.m_vscp) ಹಿಂತಿರುಗಿ a.m_vscp->user1() < b.m_vscp->user1();
            ಹಿಂತಿರುಗಿ a.m_mtaskp->user() < b.m_mtaskp->user();
        });

        // Serialize the accesses of each variable
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < accesses.ಗಾತ್ರ_2();) {
            ಗಾತ್ರ_t ಕೊನೆ = i + 1;
            ಆಗ (ಕೊನೆ < accesses.ಗಾತ್ರ_2() && accesses[ಕೊನೆ].m_vscp == accesses[i].m_vscp) ++ಕೊನೆ;
            serializeVariable(accesses.ದತ್ತಾಂಶ() + i, accesses.ದತ್ತಾಂಶ() + ಕೊನೆ);
            i = ಕೊನೆ;
        }

        // Serialize all writes to SystemC vars. Note the reads of an individual SC var are already
        // ordered against its writes by the per variable pass above, it is only the writes between
        // different SC vars that need this extra serialization. Only top level ports are SC vars,
        // so this should not hurt performance too much.
        {
            m_scratch.clear();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Access& access : accesses) {
                ಆದರೆ (!access.m_access.isWriteOrRW()) ಮುಂದುವರಿ;
                ಆದರೆ (!access.m_vscp->varp()->isSc()) ಮುಂದುವರಿ;
                m_scratch.ತಳ್ಳು_ಹಿಂಭಾಗ(access.m_mtaskp);
            }
            serializeMTasks(m_scratch);
        }

        // Serialize DPI calls unless user gave '--threads-dpi none'.
        ಆದರೆ (!v3Global.opt.ಎಳೆಗಳು_Dpi_ಶುದ್ಧ() || !v3Global.opt.ಎಳೆಗಳು_Dpi_Unpure()) {
            m_scratch.clear();
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_m_Task_ನಕ್ಷೆ.vertices()) {
                LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
                ಆದರೆ (hasDpiHazard(&mtask)) m_scratch.ತಳ್ಳು_ಹಿಂಭಾಗ(&mtask);
            }
            serializeMTasks(m_scratch);
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_Task_ನಕ್ಷೆ) { Fix_ದತ್ತಾಂಶ_Hazards{m_Task_ನಕ್ಷೆ}; }
};

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::fix_ದತ್ತಾಂಶ_Hazards(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ) {
    Fix_ದತ್ತಾಂಶ_Hazards::apply(mtask_ನಕ್ಷೆ);
    // The critical paths are maintained as the graph is mutated, check them
    mtask_ನಕ್ಷೆ.validate();
}
