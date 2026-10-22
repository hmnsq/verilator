// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstExecGraph code construction
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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"
#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp,
                                 uint32_t id) {
    ಧ್ರುವ std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* ಧ್ರುವ newp = ನೂತನ AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(ಸತ್ಯ);
    newp->voidSelf(ಸತ್ಯ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(ಸತ್ಯ);
    newp->addStmtsp(stmtsp);
    ಯದಿ (scopep) scopep->addBlocksp(newp);
    ಪ್ರತ್ಯಾಗಮ newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_ಗಂಟು_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

ಶೂನ್ಯ ExecMTask::dump(std::ostream& str) ಧ್ರುವ {
    str << name() << "." << cvtToHex(ಏತತ್);
    ಯದಿ (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

ನಾಮಾಕಾಶ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
ವರ್ಗ ThreadSchedule final {
    friend ವರ್ಗ PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    ಸ್ಥಿರ uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<ಧ್ರುವ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

ಸಾರ್ವಜನಿಕ:
    // CONSTANTS
    ಸ್ಥಿರ constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ಸಂರಚನಾ MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ಧ್ರುವ ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<ಧ್ರುವ ExecMTask*>> m_threads;

    // Global state for each mtask.
    ಸ್ಥಿರ std::unordered_map<ಧ್ರುವ ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = ಸ್ವಭಾವ;
    ThreadSchedule& operator=(ThreadSchedule&&) = ಸ್ವಭಾವ;

ಗುಪ್ತ:
    VL_UNCOPYABLE(ThreadSchedule);

    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_threadBoxWidth = 2.5;
    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_threadBoxHeight = 1.5;
    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFilePrefixedAlways(ಧ್ರುವ std::vector<ThreadSchedule>& schedules,
                                          ಧ್ರುವ string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFile(ಧ್ರುವ std::vector<ThreadSchedule>& schedules, ಧ್ರುವ string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ಧ್ರುವ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        ಯದಿ (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        ಪ್ರತಿ (uint32_t i = 0; i < nThreads; ++i) {
            ಧ್ರುವ string name = "t" + std::to_string(i);
            ಧ್ರುವ string label = "Thread " + std::to_string(i);
            constexpr ದ್ವಿಗುಣ posX = -s_horizontalGap;
            ಧ್ರುವ ದ್ವಿಗುಣ posY = -static_cast<ದ್ವಿಗುಣ>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        ಪ್ರತಿ (ಧ್ರುವ auto& state : ThreadSchedule::s_mtaskState) {
            ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        ಧ್ರುವ uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<ದ್ವಿಗುಣ> offsets(nThreads, 0.0);
        ಪ್ರತಿ (ಧ್ರುವ ThreadSchedule& schedule : schedules) {
            ಯದಿ (schedule.mtasks.empty()) ಅನುವರ್ತ;
            using Column = std::vector<ಧ್ರುವ ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            ಸಂರಚನಾ Cmp final {
                ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ExecMTask* ಧ್ರುವ a, ಧ್ರುವ ExecMTask* ಧ್ರುವ b) ಧ್ರುವ {
                    ಯದಿ (startTime(a) == startTime(b)) ಪ್ರತ್ಯಾಗಮ threadId(a) < threadId(b);
                    ಪ್ರತ್ಯಾಗಮ startTime(a) < startTime(b);
                }
            };
            ಧ್ರುವ std::multiset<ಧ್ರುವ ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                ತಾರ್ಕಿಕ intersects = ಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ earlierMtask : column) {
                    ಯದಿ (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = ಅಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                }
                ಯದಿ (intersects) {
                    column.emplace_back(mtaskp);
                } ಅನ್ಯಥಾ {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            ಪ್ರತಿ (ಧ್ರುವ Column& column : columns) {
                ದ್ವಿಗುಣ lastColumnOffset = 0;
                ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            ಪ್ರತಿ (ಧ್ರುವ std::vector<ಧ್ರುವ ExecMTask*>& thread : schedule.m_threads) {
                ಯದಿ (thread.empty()) ಭಙ್ಗ;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<ಪೂರ್ಣಾಙ್ಕ>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                ಪ್ರತಿ (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp : thread) {
                    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : mtaskp->outEdges()) {
                        ಧ್ರುವ ExecMTask* ಧ್ರುವ ತುದಿ_MTaskp = edge.ತುದಿ()->cast<ಧ್ರುವ ExecMTask>();
                        ಯದಿ (ತುದಿ_MTaskp && schedule.contains(ತುದಿ_MTaskp)
                            && threadId(ತುದಿ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << ತುದಿ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFileEmitBlock(ಧ್ರುವ std::unique_ptr<std::ofstream>& logp,
                                     ಧ್ರುವ string& name, ಧ್ರುವ string& label, ದ್ವಿಗುಣ width,
                                     ದ್ವಿಗುಣ height, ದ್ವಿಗುಣ xPos, ದ್ವಿಗುಣ yPos,
                                     ಧ್ರುವ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFileEmitMTask(ಧ್ರುವ std::unique_ptr<std::ofstream>& logp,
                                     ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp, ಧ್ರುವ ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<ದ್ವಿಗುಣ>& offsets) {
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ಧ್ರುವ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            ಧ್ರುವ string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            ಧ್ರುವ ದ್ವಿಗುಣ width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<ದ್ವಿಗುಣ>(mtaskp->cost()) / segmentCost);
            ಧ್ರುವ uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            ಧ್ರುವ ದ್ವಿಗುಣ xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            ಧ್ರುವ ದ್ವಿಗುಣ yPos = -s_threadBoxHeight * static_cast<ದ್ವಿಗುಣ>(mtaskThreadId);
            ಧ್ರುವ string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFileEmitFork(ಧ್ರುವ std::unique_ptr<std::ofstream>& logp, ದ್ವಿಗುಣ offset,
                                    uint32_t nThreads) {
        ಧ್ರುವ string& name = "fork_" + std::to_string(static_cast<ಪೂರ್ಣಾಙ್ಕ>(offset));
        constexpr ದ್ವಿಗುಣ width = s_threadBoxWidth / 8;
        ಧ್ರುವ ದ್ವಿಗುಣ height = s_threadBoxHeight * nThreads;
        ಧ್ರುವ ದ್ವಿಗುಣ xPos = offset - s_horizontalGap / 2;
        ಧ್ರುವ ದ್ವಿಗುಣ yPos
            = -static_cast<ದ್ವಿಗುಣ>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ uint32_t threadId(ಧ್ರುವ ExecMTask* mtaskp) {
        ಧ್ರುವ auto& it = s_mtaskState.find(mtaskp);
        ಪ್ರತ್ಯಾಗಮ it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    ಸ್ಥಿರ uint32_t startTime(ಧ್ರುವ ExecMTask* mtaskp) {
        ಪ್ರತ್ಯಾಗಮ s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    ಸ್ಥಿರ uint32_t endTime(ಧ್ರುವ ExecMTask* mtaskp) {
        ಪ್ರತ್ಯಾಗಮ s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ಧ್ರುವ ExecMTask* mtaskp) ಧ್ರುವ {
        ಧ್ರುವ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : mtaskp->inEdges()) {
            ಧ್ರುವ ExecMTask* ಧ್ರುವ prevp = edge.fromp()->as<ExecMTask>();
            ಯದಿ (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }

    uint32_t id() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_id; }
    uint32_t scheduleOn(ಧ್ರುವ ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ಧ್ರುವ uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<ಧ್ರುವ ExecMTask*>& bestThread = m_threads[bestThreadId];
        ಯದಿ (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.ತಳ್ಳು_back(mtaskp);
        ಪ್ರತ್ಯಾಗಮ bestEndTime;
    }
    ತಾರ್ಕಿಕ contains(ಧ್ರುವ ExecMTask* mtaskp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ mtasks.ಎಣಿಕೆ(mtaskp); }
    uint32_t endTime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<ಧ್ರುವ ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr ದ್ವಿಗುಣ V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

//######################################################################
// PackThreads

// Statically pack tasks into threads.
//
// The simplest thing that could possibly work would be to assume that our
// predictions of task runtimes are precise, and that every thread will
// make progress at an equal rate. Simulate a single "clock", pack the
// highest priority ready task into whatever thread becomes ready earliest,
// repeating until no tasks remain.
//
// That doesn't work well, as our predictions of task runtimes have wide
// error bars (+/- 60% is typical.)
//
// So be a little more clever: let each task have a different end time,
// depending on which thread is looking. Be a little bit pessimistic when
// thread A checks the end time of an mtask running on thread B. This extra
// "padding" avoids tight "layovers" at cross-thread dependencies.
ವರ್ಗ PackThreads final {
    // TYPES
    ಸಂರಚನಾ MTaskCmp final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ ExecMTask* ap, ಧ್ರುವ ExecMTask* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ ap->id() < bp->id();
        }
    };

    // MEMBERS
    ಧ್ರುವ uint32_t m_nThreads;  // Number of threads
    ಧ್ರುವ uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    ಧ್ರುವ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ಧ್ರುವ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(ಧ್ರುವ ThreadSchedule& schedule, ಧ್ರುವ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        ಯದಿ (!schedule.contains(mtaskp)) ಪ್ರತ್ಯಾಗಮ 0;
        ಧ್ರುವ ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        ಯದಿ (threadId == state.threadId) {
            // No overhead on same thread
            ಪ್ರತ್ಯಾಗಮ state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        ಯದಿ (state.nextp) {
            ಧ್ರುವ uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            ಯದಿ ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        ಪ್ರತ್ಯಾಗಮ sandbaggedEndTime;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isReady(ThreadSchedule& schedule, ಧ್ರುವ ExecMTask* mtaskp) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            ಧ್ರುವ ExecMTask* ಧ್ರುವ prevp = edgeIn.fromp()->as<ಧ್ರುವ ExecMTask>();
            ಯದಿ (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::vector<ThreadSchedule> pack(V3Graph& mtaskGraph) {
        std::vector<ThreadSchedule> result;
        result.emplace_back(ThreadSchedule{m_nThreads});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        ಗಣನಾ ವರ್ಗ SchedulingMode : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        SchedulingMode mode = SchedulingMode::SCHEDULING;

        // Time each thread is occupied until
        std::vector<uint32_t> busyUntil(std::max(m_nThreads, m_nHierThreads), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::set<ExecMTask*, MTaskCmp> readyMTasks;
        ಪೂರ್ಣಾಙ್ಕ maxThreadWorkers = 1;

        // Build initial ready list
        ಪ್ರತಿ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ಧ್ರುವ mtaskp = vtx.as<ExecMTask>();
            ಯದಿ (isReady(result.back(), mtaskp)) readyMTasks.ಸೇರಿಸು(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        ಯಾವತ್ (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            ಪ್ರತಿ (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                ಪ್ರತಿ (ExecMTask* ಧ್ರುವ mtaskp : readyMTasks) {
                    ಯದಿ (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        ಅನುವರ್ತ;
                    }
                    ಯದಿ (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        ಅನುವರ್ತ;

                    uint32_t timeBegin = busyUntil[threadId];
                    ಯದಿ (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        ಭಙ್ಗ;
                    }
                    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : mtaskp->inEdges()) {
                        ಧ್ರುವ ExecMTask* ಧ್ರುವ priorp = edge.fromp()->as<ExecMTask>();
                        ಧ್ರುವ uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        ಯದಿ (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    ಯದಿ ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            ಧ್ರುವ uint32_t endTime = schedule.endTime();

            ಯದಿ (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                ಧ್ರುವ uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                ಯದಿ (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ಅನುವರ್ತ;
            }

            ಯದಿ (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ಅನುವರ್ತ;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ಧ್ರುವ uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            ಧ್ರುವ size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* ಧ್ರುವ nextp = edgeOut.ತುದಿ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                ಯದಿ (isReady(schedule, nextp)) {
                    readyMTasks.ಸೇರಿಸು(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        ಯದಿ (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        ಪ್ರತ್ಯಾಗಮ result;
    }

ಸಾರ್ವಜನಿಕ:
    // SELF TEST
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ಧ್ರುವ scopep = ನೂತನ AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* ಧ್ರುವ trigVscp = ನೂತನ AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_ವೃಕ್ಷ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_ವೃಕ್ಷ(), scopep);
        VL_DO_DANGLING(varp->delete_ವೃಕ್ಷ(), varp);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestNormalFirst(AstVarScope* trigVscp) {
        File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
        AstExecGraph* ಧ್ರುವ execGraphp = ನೂತನ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ಧ್ರುವ auto makeBody = [&]() -> Ast_ಗಂಟು_Stmt* { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstComment{flp, ""}; };
        ExecMTask* ಧ್ರುವ t0 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* ಧ್ರುವ t1 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* ಧ್ರುವ t2 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* ಧ್ರುವ t3 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* ಧ್ರುವ t4 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* ಧ್ರುವ t5 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* ಧ್ರುವ t6 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t6->cost(100);
        t6->priority(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        ನೂತನ V3GraphEdge{&graph, t0, t1, 1};
        ನೂತನ V3GraphEdge{&graph, t0, t2, 1};
        ನೂತನ V3GraphEdge{&graph, t2, t3, 1};
        ನೂತನ V3GraphEdge{&graph, t2, t4, 1};
        ನೂತನ V3GraphEdge{&graph, t3, t5, 1};
        ನೂತನ V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ಧ್ರುವ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        ಪ್ರತಿ (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_threads[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), hierThreads / 3);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_threads[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[2].m_threads[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_threads[1][0], t6);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 7);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t2), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t3), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t4), 1);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t5), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completionTime(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t4, 5), 1360);

        ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ವೃಕ್ಷ();
        VL_DO_DANGLING(execGraphp->delete_ವೃಕ್ಷ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestHierFirst(AstVarScope* trigVscp) {
        File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
        AstExecGraph* ಧ್ರುವ execGraphp = ನೂತನ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ಧ್ರುವ auto makeBody = [&]() -> Ast_ಗಂಟು_Stmt* { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstComment{flp, ""}; };
        ExecMTask* ಧ್ರುವ t0 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* ಧ್ರುವ t1 = ನೂತನ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        ನೂತನ V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ಧ್ರುವ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        ಪ್ರತಿ (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        ಪ್ರತಿ (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ವೃಕ್ಷ();
        VL_DO_DANGLING(execGraphp->delete_ವೃಕ್ಷ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    ಸ್ಥಿರ std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        ಪ್ರತ್ಯಾಗಮ PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

ಶೂನ್ಯ normalizeCosts(Costs& costs) {
    ಧ್ರುವ auto scaleCost = [](uint64_t ಮೌಲ್ಯ, ದ್ವಿಗುಣ multiplier) {
        ದ್ವಿಗುಣ scaled = static_cast<ದ್ವಿಗುಣ>(ಮೌಲ್ಯ) * multiplier;
        ಯದಿ (ಮೌಲ್ಯ && scaled < 1) scaled = 1;
        ಪ್ರತ್ಯಾಗಮ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    ಪ್ರತಿ (ಧ್ರುವ auto& est : costs) {
        ಯದಿ (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    ಯದಿ (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ಧ್ರುವ ದ್ವಿಗುಣ estToProfile
            = static_cast<ದ್ವಿಗುಣ>(sumCostProfiled) / static_cast<ದ್ವಿಗುಣ>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        ಪ್ರತಿ (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    ಪ್ರತಿ (auto& est : costs) {
        ಧ್ರುವ uint64_t& costEstimate = est.second.first;
        ಧ್ರುವ uint64_t& costProfiled = est.second.second;
        ಯದಿ (maxCost < costEstimate) maxCost = costEstimate;
        ಯದಿ (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    ಯದಿ (maxCost > scaleDownTo) {
        ಧ್ರುವ ದ್ವಿಗುಣ scaleup = static_cast<ದ್ವಿಗುಣ>(scaleDownTo) / static_cast<ದ್ವಿಗುಣ>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        ಪ್ರತಿ (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

ಶೂನ್ಯ removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    ಪ್ರತಿ (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        ಯದಿ (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_ವೃಕ್ಷ(), callp);
    }
    V3Graph* ಧ್ರುವ execMTaskGraphp = execGraphp->depGraphp();
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ಧ್ರುವ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ಧ್ರುವ funcp = mtaskp->funcp();
        ಯದಿ (funcp->stmtsp()) ಅನುವರ್ತ;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_ವೃಕ್ಷ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

ಶೂನ್ಯ fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    ಪ್ರತಿ (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ಧ್ರುವ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ಧ್ರುವ uint64_t costEstimate = V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ(mtp->funcp(), ಅಸತ್ಯ);
        ಧ್ರುವ uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        ಯದಿ (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    ಪೂರ್ಣಾಙ್ಕ totalEstimates = 0;
    ಪೂರ್ಣಾಙ್ಕ missingProfiles = 0;
    ಪ್ರತಿ (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ಧ್ರುವ mtp = vtx.as<ExecMTask>();
        ಧ್ರುವ uint32_t costEstimate = costs[mtp->id()].first;
        ಧ್ರುವ uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ಧ್ರುವ uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        ಯದಿ (!costProfiled32) {
            costToUse = costEstimate;
            ಯದಿ (costEstimate != 0) ++missingProfiles;
        }
        ಯದಿ (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    ಯದಿ (missingProfiles) {
        ಯದಿ (File_ಸಾಲು* ಧ್ರುವ fl = V3Control::get_Profile_Data_File_ಸಾಲು()) {
            ಯದಿ (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

ಶೂನ್ಯ finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    ಯಾವತ್ (ಧ್ರುವ V3GraphVertex* ಧ್ರುವ vxp = ser.nextp()) {
        ExecMTask* ಧ್ರುವ mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        ಪ್ರತಿ (V3GraphEdge& edge : mtp->outEdges()) {
            ಧ್ರುವ ExecMTask* ಧ್ರುವ followp = edge.ತುದಿ()->as<ExecMTask>();
            ಯದಿ ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    ಧ್ರುವ auto report = execMTaskGraphp->parallelismReport([](ಧ್ರುವ V3GraphVertex* vtxp) {  //
        ಪ್ರತ್ಯಾಗಮ vtxp->as<ಧ್ರುವ ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_ಎಣಿಕೆ());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_ಎಣಿಕೆ());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    ಯದಿ (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_ಎಣಿಕೆ());
        UINFO(0, "    Edge count = " << report.edge_ಎಣಿಕೆ());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ಧ್ರುವ std::string& name, uint32_t nDependencies) {
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಧ್ರುವ fl = modp->fileline();
    AstBasicDType* ಧ್ರುವ dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(ಸತ್ಯ);
    varp->valuep(ನೂತನ AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    ಪ್ರತ್ಯಾಗಮ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ಧ್ರುವ std::string& method) {
    File_ಸಾಲು* ಧ್ರುವ fl = varp->fileline();
    AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ಧ್ರುವ AstScope* ಧ್ರುವ ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, ತುದಿ_Scopep->nameDotless()});
    AstCStmt* ಧ್ರುವ stmtp = ನೂತನ AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    ಪ್ರತ್ಯಾಗಮ stmtp;
}

ಶೂನ್ಯ addMTaskToFunction(ಧ್ರುವ ThreadSchedule& schedule, ಧ್ರುವ uint32_t threadId, AstCFunc* funcp,
                        ಧ್ರುವ ExecMTask* mtaskp,
                        ಧ್ರುವ std::unordered_map<ಧ್ರುವ ExecMTask*, AstVar*>& stateVars) {
    AstScope* ಧ್ರುವ scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಧ್ರುವ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ಧ್ರುವ auto addCStmt = [=](ಧ್ರುವ string& stmt) -> ಶೂನ್ಯ {  //
        funcp->addStmtsp(ನೂತನ AstCStmt{fl, stmt});
    };

    ಯದಿ (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        ಯದಿ (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        ಯದಿ (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    ಯದಿ (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    ಯದಿ (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : mtaskp->outEdges()) {
        ಧ್ರುವ ExecMTask* ಧ್ರುವ nextp = edge.ತುದಿ()->as<ExecMTask>();
        ಯದಿ (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ಧ್ರುವ std::vector<AstCFunc*> createThreadFunctions(ಧ್ರುವ ThreadSchedule& schedule,
                                                   ಧ್ರುವ string& tag) {
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಧ್ರುವ fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ಧ್ರುವ ExecMTask*, AstVar*> stateVars;
    ಪ್ರತಿ (ಧ್ರುವ std::vector<ಧ್ರುವ ExecMTask*>& thread : schedule.m_threads) {
        ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp : thread) {
            ಯದಿ (ಧ್ರುವ uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    ಪ್ರತಿ (ಧ್ರುವ std::vector<ಧ್ರುವ ExecMTask*>& thread : schedule.m_threads) {
        ಯದಿ (thread.empty()) ಅನುವರ್ತ;
        ಧ್ರುವ uint32_t threadId = schedule.threadId(thread.front());
        ಧ್ರುವ string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.ತಳ್ಳು_back(funcp);
        funcp->isStatic(ಸತ್ಯ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(ಸತ್ಯ);
        funcp->entryPoint(ಸತ್ಯ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(ನೂತನ AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(ನೂತನ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        ಪ್ರತಿ (ಧ್ರುವ ExecMTask* ಧ್ರುವ mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    ಪ್ರತ್ಯಾಗಮ funcps;
}

ಶೂನ್ಯ addThreadStartWrapper(AstExecGraph* ಧ್ರುವ execGraphp) {
    // FileLine used for constructing nodes below
    File_ಸಾಲು* ಧ್ರುವ fl = v3Global.rootp()->fileline();
    ಧ್ರುವ string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    ಧ್ರುವ auto addCStmt = [=](ಧ್ರುವ string& stmt) -> ಶೂನ್ಯ {  //
        execGraphp->runfuncp()->addStmtsp(ನೂತನ AstCStmt{fl, stmt});
    };

    ಯದಿ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    ಯದಿ (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

ಶೂನ್ಯ addThreadEndWrapper(AstExecGraph* ಧ್ರುವ execGraphp) {
    // Add thread function invocations to execGraph
    ಧ್ರುವ auto addCStmt = [=](ಧ್ರುವ string& stmt) -> ಶೂನ್ಯ {  //
        File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(ನೂತನ AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    ಯದಿ (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
ಶೂನ್ಯ addThreadStartToExecGraph(AstExecGraph* ಧ್ರುವ execGraphp,
                               ಧ್ರುವ std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_ಸಾಲು* ಧ್ರುವ fl = v3Global.rootp()->fileline();
    ಧ್ರುವ string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ಧ್ರುವ finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    ಪ್ರತಿ (AstCFunc* ಧ್ರುವ funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ಧ್ರುವ auto addCStmt = [=](ಧ್ರುವ string& stmt) -> ಶೂನ್ಯ {  //
        execGraphp->runfuncp()->addStmtsp(ನೂತನ AstCStmt{fl, stmt});
    };

    ಧ್ರುವ uint32_t last = funcps.size() - 1;
    ಯದಿ (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    ಪ್ರತಿ (AstCFunc* ಧ್ರುವ funcp : funcps) {
        ಯದಿ (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            ಯದಿ (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } ಅನ್ಯಥಾ {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(ನೂತನ AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } ಅನ್ಯಥಾ {
            // The last will run on the main thread.
            AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    ಯದಿ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    ಯದಿ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    ಯದಿ (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

ಶೂನ್ಯ processMTaskBodies(AstExecGraph* ಧ್ರುವ execGraphp) {
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ಧ್ರುವ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ಧ್ರುವ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ಗಂಟು* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        ಧ್ರುವ auto addCStmt = [=](ಧ್ರುವ string& stmt) -> ಶೂನ್ಯ {  //
            funcp->addStmtsp(ನೂತನ AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        ಯದಿ (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            ಯದಿ (v3Global.opt.hierChild()) args += v3Global.opt.ತುದಿ_Module();
            args += "\"";
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + args + ");");
        }
        // Set mtask ID in the run-time system
        addCStmt("Verilated::mtaskId(" + std::to_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->addStmtsp(stmtsp);
        // Flush message queue
        addCStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        ಯದಿ (v3Global.opt.profExec()) {
            ಧ್ರುವ std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

ಶೂನ್ಯ implementExecGraph(AstExecGraph* ಧ್ರುವ execGraphp, ಧ್ರುವ ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    ಯದಿ (execGraphp->depGraphp()->empty()) ಪ್ರತ್ಯಾಗಮ;

    // Create a function to be run by each thread.
    ಧ್ರುವ std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

ಶೂನ್ಯ createRunFunction(AstExecGraph* ಧ್ರುವ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_ಸಾಲು* ಧ್ರುವ flp = execGraphp->fileline();
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = v3Global.rootp()->ತುದಿ_Modulep();
    AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(ಸತ್ಯ);
    funcp->voidSelf(ಸತ್ಯ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(ಸತ್ಯ);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

ಶೂನ್ಯ addTriggerMasks(AstExecGraph* ಧ್ರುವ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_ಸಾಲು* ಧ್ರುವ flp = execGraphp->fileline();
    AstVar* ಧ್ರುವ triggersp = execGraphp->triggerp()->varp();
    ಪ್ರತಿ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ಧ್ರುವ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            ಪ್ರತಿ (ಧ್ರುವ uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{flp, triggersp->dtypep(),
                                                     ನೂತನ AstConst{flp, AstConst::Unsized64{}, 0}};
        ಪ್ರತಿ (ಧ್ರುವ auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          ನೂತನ AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);
    }
}

ಶೂನ್ಯ addTriggerCosts(AstExecGraph* ಧ್ರುವ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_ಸಾಲು* ಧ್ರುವ flp = execGraphp->fileline();
    ಧ್ರುವ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    ಪ್ರತಿ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            ಯದಿ (ಧ್ರುವ AstTriggered* ಧ್ರುವ trigp = VN_CAST(ifp->condp(), Triggered)) {
                ಧ್ರುವ uint32_t cost = V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ(ifp, ಅಸತ್ಯ);
                ಪ್ರತಿ (ಧ್ರುವ uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
    AstUnpackArrayDType* ಧ್ರುವ dtypep
        = ನೂತನ AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  ನೂತನ AstRange{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{flp, dtypep, ನೂತನ AstConst{flp, 0}};
    ಪ್ರತಿ (uint32_t index = 0; index < nTriggers; ++index) {
        ಯದಿ (costs[index]) consp->valuep(index, ನೂತನ AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);
}

// Called by Verilator top stage
ಶೂನ್ಯ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->ತುದಿ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    ಪ್ರತಿ (AstExecGraph* ಧ್ರುವ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        ಯದಿ (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_ವೃಕ್ಷ(), execGraphp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        ಯದಿ (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ಧ್ರುವ std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<ದ್ವಿಗುಣ>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        ಪ್ರತಿ (ಧ್ರುವ ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

ಶೂನ್ಯ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].first, 1000);
        UASSERT_SELFTEST(costs[1].second, 1000);
        UASSERT_SELFTEST(costs[2].first, 2000);
        UASSERT_SELFTEST(costs[2].second, 0);
        UASSERT_SELFTEST(costs[3].first, 3000);
        UASSERT_SELFTEST(costs[3].second, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].first, 2500000);
        UASSERT_SELFTEST(costs[1].second, 5000000);
        UASSERT_SELFTEST(costs[2].first, 5000000);
        UASSERT_SELFTEST(costs[2].second, 10000000);
        UASSERT_SELFTEST(costs[3].first, 7500000);
        UASSERT_SELFTEST(costs[3].second, 1);
    }

    PackThreads::selfTest();
}

}  // namespace V3ExecGraph
