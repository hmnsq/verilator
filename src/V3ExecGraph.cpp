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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3ExecGraph.h"

#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3GraphStream.h"
#অন্তর্ভাৱ "V3Hasher.h"
#অন্তর্ভাৱ "V3InstrCount.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্ৰন্থি_Stmt* stmtsp,
                                 uint32_t id) {
    ধ্রুৱ std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* ধ্রুৱ newp = নূতন AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(সঁচা);
    newp->voidSelf(সঁচা);  // Called through VlExecGraph::Fnp
    newp->dontCombine(সঁচা);
    newp->addStmtsp(stmtsp);
    যদি (scopep) scopep->addBlocksp(newp);
    ঘূৰাই newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_গ্ৰন্থি_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

শূন্য ExecMTask::dump(std::ostream& str) ধ্রুৱ {
    str << name() << "." << cvtToHex(এতত্);
    যদি (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

নামাকাশ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
শ্ৰেণী ThreadSchedule final {
    friend শ্ৰেণী PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    স্থির uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<ধ্রুৱ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

সার্ৱজনিক:
    // CONSTANTS
    স্থির constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    সংরচনা MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ধ্রুৱ ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<ধ্রুৱ ExecMTask*>> m_threads;

    // Global state for each mtask.
    স্থির std::unordered_map<ধ্রুৱ ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = স্ৱভাৱ;
    ThreadSchedule& operator=(ThreadSchedule&&) = স্ৱভাৱ;

গুপ্ত:
    VL_UNCOPYABLE(ThreadSchedule);

    স্থির constexpr দ্ৱিগুণ s_threadBoxWidth = 2.5;
    স্থির constexpr দ্ৱিগুণ s_threadBoxHeight = 1.5;
    স্থির constexpr দ্ৱিগুণ s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    স্থির শূন্য dumpDotFilePrefixedAlways(ধ্রুৱ std::vector<ThreadSchedule>& schedules,
                                          ধ্রুৱ string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    স্থির শূন্য dumpDotFile(ধ্রুৱ std::vector<ThreadSchedule>& schedules, ধ্রুৱ string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ধ্রুৱ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        যদি (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        বাবে (uint32_t i = 0; i < nThreads; ++i) {
            ধ্রুৱ string name = "t" + std::to_string(i);
            ধ্রুৱ string label = "Thread " + std::to_string(i);
            constexpr দ্ৱিগুণ posX = -s_horizontalGap;
            ধ্রুৱ দ্ৱিগুণ posY = -static_cast<দ্ৱিগুণ>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        বাবে (ধ্রুৱ auto& state : ThreadSchedule::s_mtaskState) {
            ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        ধ্রুৱ uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<দ্ৱিগুণ> offsets(nThreads, 0.0);
        বাবে (ধ্রুৱ ThreadSchedule& schedule : schedules) {
            যদি (schedule.mtasks.empty()) অনুৱর্ত;
            using Column = std::vector<ধ্রুৱ ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            সংরচনা Cmp final {
                তার্কিক operator()(ধ্রুৱ ExecMTask* ধ্রুৱ a, ধ্রুৱ ExecMTask* ধ্রুৱ b) ধ্রুৱ {
                    যদি (startTime(a) == startTime(b)) ঘূৰাই threadId(a) < threadId(b);
                    ঘূৰাই startTime(a) < startTime(b);
                }
            };
            ধ্রুৱ std::multiset<ধ্রুৱ ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                তার্কিক intersects = সঁচা;
                বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ earlierMtask : column) {
                    যদি (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = মিছা;
                        ভঙ্গ;
                    }
                }
                যদি (intersects) {
                    column.emplace_back(mtaskp);
                } অন্যথা {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            বাবে (ধ্রুৱ Column& column : columns) {
                দ্ৱিগুণ lastColumnOffset = 0;
                বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            বাবে (ধ্রুৱ std::vector<ধ্রুৱ ExecMTask*>& thread : schedule.m_threads) {
                যদি (thread.empty()) ভঙ্গ;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<পূর্ণাঙ্ক>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                বাবে (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp : thread) {
                    বাবে (ধ্রুৱ V3GraphEdge& edge : mtaskp->outEdges()) {
                        ধ্রুৱ ExecMTask* ধ্রুৱ শীৰ্ষ_MTaskp = edge.শীৰ্ষ()->cast<ধ্রুৱ ExecMTask>();
                        যদি (শীৰ্ষ_MTaskp && schedule.contains(শীৰ্ষ_MTaskp)
                            && threadId(শীৰ্ষ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << শীৰ্ষ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    স্থির শূন্য dumpDotFileEmitBlock(ধ্রুৱ std::unique_ptr<std::ofstream>& logp,
                                     ধ্রুৱ string& name, ধ্রুৱ string& label, দ্ৱিগুণ width,
                                     দ্ৱিগুণ height, দ্ৱিগুণ xPos, দ্ৱিগুণ yPos,
                                     ধ্রুৱ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    স্থির শূন্য dumpDotFileEmitMTask(ধ্রুৱ std::unique_ptr<std::ofstream>& logp,
                                     ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp, ধ্রুৱ ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<দ্ৱিগুণ>& offsets) {
        বাবে (পূর্ণাঙ্ক i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ধ্রুৱ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            ধ্রুৱ string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            ধ্রুৱ দ্ৱিগুণ width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<দ্ৱিগুণ>(mtaskp->cost()) / segmentCost);
            ধ্রুৱ uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            ধ্রুৱ দ্ৱিগুণ xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            ধ্রুৱ দ্ৱিগুণ yPos = -s_threadBoxHeight * static_cast<দ্ৱিগুণ>(mtaskThreadId);
            ধ্রুৱ string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    স্থির শূন্য dumpDotFileEmitFork(ধ্রুৱ std::unique_ptr<std::ofstream>& logp, দ্ৱিগুণ offset,
                                    uint32_t nThreads) {
        ধ্রুৱ string& name = "fork_" + std::to_string(static_cast<পূর্ণাঙ্ক>(offset));
        constexpr দ্ৱিগুণ width = s_threadBoxWidth / 8;
        ধ্রুৱ দ্ৱিগুণ height = s_threadBoxHeight * nThreads;
        ধ্রুৱ দ্ৱিগুণ xPos = offset - s_horizontalGap / 2;
        ধ্রুৱ দ্ৱিগুণ yPos
            = -static_cast<দ্ৱিগুণ>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

সার্ৱজনিক:
    স্থির uint32_t threadId(ধ্রুৱ ExecMTask* mtaskp) {
        ধ্রুৱ auto& it = s_mtaskState.find(mtaskp);
        ঘূৰাই it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    স্থির uint32_t startTime(ধ্রুৱ ExecMTask* mtaskp) {
        ঘূৰাই s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    স্থির uint32_t endTime(ধ্রুৱ ExecMTask* mtaskp) {
        ঘূৰাই s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ধ্রুৱ ExecMTask* mtaskp) ধ্রুৱ {
        ধ্রুৱ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        বাবে (ধ্রুৱ V3GraphEdge& edge : mtaskp->inEdges()) {
            ধ্রুৱ ExecMTask* ধ্রুৱ prevp = edge.fromp()->as<ExecMTask>();
            যদি (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        ঘূৰাই result;
    }

    uint32_t id() ধ্রুৱ { ঘূৰাই m_id; }
    uint32_t scheduleOn(ধ্রুৱ ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ধ্রুৱ uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<ধ্রুৱ ExecMTask*>& bestThread = m_threads[bestThreadId];
        যদি (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.ঠেলো_back(mtaskp);
        ঘূৰাই bestEndTime;
    }
    তার্কিক contains(ধ্রুৱ ExecMTask* mtaskp) ধ্রুৱ { ঘূৰাই mtasks.গুনতি(mtaskp); }
    uint32_t endTime() ধ্রুৱ { ঘূৰাই m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<ধ্রুৱ ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr দ্ৱিগুণ V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
শ্ৰেণী PackThreads final {
    // TYPES
    সংরচনা MTaskCmp final {
        তার্কিক operator()(ধ্রুৱ ExecMTask* ap, ধ্রুৱ ExecMTask* bp) ধ্রুৱ {
            ঘূৰাই ap->id() < bp->id();
        }
    };

    // MEMBERS
    ধ্রুৱ uint32_t m_nThreads;  // Number of threads
    ধ্রুৱ uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    ধ্রুৱ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ধ্রুৱ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = স্ৱভাৱ;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(ধ্রুৱ ThreadSchedule& schedule, ধ্রুৱ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        যদি (!schedule.contains(mtaskp)) ঘূৰাই 0;
        ধ্রুৱ ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        যদি (threadId == state.threadId) {
            // No overhead on same thread
            ঘূৰাই state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        যদি (state.nextp) {
            ধ্রুৱ uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            যদি ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        ঘূৰাই sandbaggedEndTime;
    }

    স্থির তার্কিক isReady(ThreadSchedule& schedule, ধ্রুৱ ExecMTask* mtaskp) {
        বাবে (ধ্রুৱ V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            ধ্রুৱ ExecMTask* ধ্রুৱ prevp = edgeIn.fromp()->as<ধ্রুৱ ExecMTask>();
            যদি (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                ঘূৰাই মিছা;
            }
        }
        ঘূৰাই সঁচা;
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
        গণনা শ্ৰেণী SchedulingMode : uint8_t {
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
        পূর্ণাঙ্ক maxThreadWorkers = 1;

        // Build initial ready list
        বাবে (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ধ্রুৱ mtaskp = vtx.as<ExecMTask>();
            যদি (isReady(result.back(), mtaskp)) readyMTasks.ঢোকাও(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        যেতিয়ালৈ (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            বাবে (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                বাবে (ExecMTask* ধ্রুৱ mtaskp : readyMTasks) {
                    যদি (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        অনুৱর্ত;
                    }
                    যদি (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        অনুৱর্ত;

                    uint32_t timeBegin = busyUntil[threadId];
                    যদি (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        ভঙ্গ;
                    }
                    বাবে (ধ্রুৱ V3GraphEdge& edge : mtaskp->inEdges()) {
                        ধ্রুৱ ExecMTask* ধ্রুৱ priorp = edge.fromp()->as<ExecMTask>();
                        ধ্রুৱ uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        যদি (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    যদি ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            ধ্রুৱ uint32_t endTime = schedule.endTime();

            যদি (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                ধ্রুৱ uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                যদি (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                অনুৱর্ত;
            }

            যদি (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                অনুৱর্ত;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ধ্রুৱ uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            ধ্রুৱ size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            বাবে (ধ্রুৱ V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* ধ্রুৱ nextp = edgeOut.শীৰ্ষ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                যদি (isReady(schedule, nextp)) {
                    readyMTasks.ঢোকাও(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        যদি (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        ঘূৰাই result;
    }

সার্ৱজনিক:
    // SELF TEST
    স্থির শূন্য selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
        AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ধ্রুৱ scopep = নূতন AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* ধ্রুৱ trigVscp = নূতন AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_বৃক্ষ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_বৃক্ষ(), scopep);
        VL_DO_DANGLING(varp->delete_বৃক্ষ(), varp);
    }
    স্থির শূন্য selfTestNormalFirst(AstVarScope* trigVscp) {
        File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
        AstExecGraph* ধ্রুৱ execGraphp = নূতন AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ধ্রুৱ auto makeBody = [&]() -> Ast_গ্ৰন্থি_Stmt* { ঘূৰাই নূতন AstComment{flp, ""}; };
        ExecMTask* ধ্রুৱ t0 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* ধ্রুৱ t1 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* ধ্রুৱ t2 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* ধ্রুৱ t3 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* ধ্রুৱ t4 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* ধ্রুৱ t5 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* ধ্রুৱ t6 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
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
        নূতন V3GraphEdge{&graph, t0, t1, 1};
        নূতন V3GraphEdge{&graph, t0, t2, 1};
        নূতন V3GraphEdge{&graph, t2, t3, 1};
        নূতন V3GraphEdge{&graph, t2, t4, 1};
        নূতন V3GraphEdge{&graph, t3, t5, 1};
        নূতন V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ধ্রুৱ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        বাবে (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        বাবে (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_বৃক্ষ();
        VL_DO_DANGLING(execGraphp->delete_বৃক্ষ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    স্থির শূন্য selfTestHierFirst(AstVarScope* trigVscp) {
        File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
        AstExecGraph* ধ্রুৱ execGraphp = নূতন AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ধ্রুৱ auto makeBody = [&]() -> Ast_গ্ৰন্থি_Stmt* { ঘূৰাই নূতন AstComment{flp, ""}; };
        ExecMTask* ধ্রুৱ t0 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* ধ্রুৱ t1 = নূতন ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        নূতন V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ধ্রুৱ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        বাবে (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        বাবে (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        বাবে (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_বৃক্ষ();
        VL_DO_DANGLING(execGraphp->delete_বৃক্ষ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    স্থির std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        ঘূৰাই PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

শূন্য normalizeCosts(Costs& costs) {
    ধ্রুৱ auto scaleCost = [](uint64_t মান, দ্ৱিগুণ multiplier) {
        দ্ৱিগুণ scaled = static_cast<দ্ৱিগুণ>(মান) * multiplier;
        যদি (মান && scaled < 1) scaled = 1;
        ঘূৰাই static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    বাবে (ধ্রুৱ auto& est : costs) {
        যদি (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    যদি (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ধ্রুৱ দ্ৱিগুণ estToProfile
            = static_cast<দ্ৱিগুণ>(sumCostProfiled) / static_cast<দ্ৱিগুণ>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        বাবে (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    বাবে (auto& est : costs) {
        ধ্রুৱ uint64_t& costEstimate = est.second.first;
        ধ্রুৱ uint64_t& costProfiled = est.second.second;
        যদি (maxCost < costEstimate) maxCost = costEstimate;
        যদি (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    যদি (maxCost > scaleDownTo) {
        ধ্রুৱ দ্ৱিগুণ scaleup = static_cast<দ্ৱিগুণ>(scaleDownTo) / static_cast<দ্ৱিগুণ>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        বাবে (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

শূন্য removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    বাবে (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        যদি (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_বৃক্ষ(), callp);
    }
    V3Graph* ধ্রুৱ execMTaskGraphp = execGraphp->depGraphp();
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ধ্রুৱ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ধ্রুৱ funcp = mtaskp->funcp();
        যদি (funcp->stmtsp()) অনুৱর্ত;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_বৃক্ষ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

শূন্য fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    বাবে (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ধ্রুৱ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ধ্রুৱ uint64_t costEstimate = V3_Instr_গুনতি::গুনতি(mtp->funcp(), মিছা);
        ধ্রুৱ uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        যদি (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    পূর্ণাঙ্ক totalEstimates = 0;
    পূর্ণাঙ্ক missingProfiles = 0;
    বাবে (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ধ্রুৱ mtp = vtx.as<ExecMTask>();
        ধ্রুৱ uint32_t costEstimate = costs[mtp->id()].first;
        ধ্রুৱ uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ধ্রুৱ uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        যদি (!costProfiled32) {
            costToUse = costEstimate;
            যদি (costEstimate != 0) ++missingProfiles;
        }
        যদি (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    যদি (missingProfiles) {
        যদি (File_সাৰি* ধ্রুৱ fl = V3Control::get_Profile_Data_File_সাৰি()) {
            যদি (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

শূন্য finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    যেতিয়ালৈ (ধ্রুৱ V3GraphVertex* ধ্রুৱ vxp = ser.nextp()) {
        ExecMTask* ধ্রুৱ mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        বাবে (V3GraphEdge& edge : mtp->outEdges()) {
            ধ্রুৱ ExecMTask* ধ্রুৱ followp = edge.শীৰ্ষ()->as<ExecMTask>();
            যদি ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    ধ্রুৱ auto report = execMTaskGraphp->parallelismReport([](ধ্রুৱ V3GraphVertex* vtxp) {  //
        ঘূৰাই vtxp->as<ধ্রুৱ ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_গুনতি());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_গুনতি());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    যদি (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_গুনতি());
        UINFO(0, "    Edge count = " << report.edge_গুনতি());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ধ্রুৱ std::string& name, uint32_t nDependencies) {
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = v3Global.rootp()->শীৰ্ষ_Modulep();
    File_সাৰি* ধ্রুৱ fl = modp->fileline();
    AstBasicDType* ধ্রুৱ dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ধ্রুৱ varp = নূতন AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(সঁচা);
    varp->valuep(নূতন AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    ঘূৰাই varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ধ্রুৱ std::string& method) {
    File_সাৰি* ধ্রুৱ fl = varp->fileline();
    AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ধ্রুৱ AstScope* ধ্রুৱ শীৰ্ষ_Scopep = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, শীৰ্ষ_Scopep->nameDotless()});
    AstCStmt* ধ্রুৱ stmtp = নূতন AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    ঘূৰাই stmtp;
}

শূন্য addMTaskToFunction(ধ্রুৱ ThreadSchedule& schedule, ধ্রুৱ uint32_t threadId, AstCFunc* funcp,
                        ধ্রুৱ ExecMTask* mtaskp,
                        ধ্রুৱ std::unordered_map<ধ্রুৱ ExecMTask*, AstVar*>& stateVars) {
    AstScope* ধ্রুৱ scopep = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = v3Global.rootp()->শীৰ্ষ_Modulep();
    File_সাৰি* ধ্রুৱ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ধ্রুৱ auto addCStmt = [=](ধ্রুৱ string& stmt) -> শূন্য {  //
        funcp->addStmtsp(নূতন AstCStmt{fl, stmt});
    };

    যদি (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        যদি (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        যদি (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    যদি (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    যদি (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    বাবে (ধ্রুৱ V3GraphEdge& edge : mtaskp->outEdges()) {
        ধ্রুৱ ExecMTask* ধ্রুৱ nextp = edge.শীৰ্ষ()->as<ExecMTask>();
        যদি (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ধ্রুৱ std::vector<AstCFunc*> createThreadFunctions(ধ্রুৱ ThreadSchedule& schedule,
                                                   ধ্রুৱ string& tag) {
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = v3Global.rootp()->শীৰ্ষ_Modulep();
    File_সাৰি* ধ্রুৱ fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ধ্রুৱ ExecMTask*, AstVar*> stateVars;
    বাবে (ধ্রুৱ std::vector<ধ্রুৱ ExecMTask*>& thread : schedule.m_threads) {
        বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp : thread) {
            যদি (ধ্রুৱ uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    বাবে (ধ্রুৱ std::vector<ধ্রুৱ ExecMTask*>& thread : schedule.m_threads) {
        যদি (thread.empty()) অনুৱর্ত;
        ধ্রুৱ uint32_t threadId = schedule.threadId(thread.front());
        ধ্রুৱ string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.ঠেলো_back(funcp);
        funcp->isStatic(সঁচা);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(সঁচা);
        funcp->entryPoint(সঁচা);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        বাবে (ধ্রুৱ ExecMTask* ধ্রুৱ mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    ঘূৰাই funcps;
}

শূন্য addThreadStartWrapper(AstExecGraph* ধ্রুৱ execGraphp) {
    // FileLine used for constructing nodes below
    File_সাৰি* ধ্রুৱ fl = v3Global.rootp()->fileline();
    ধ্রুৱ string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    ধ্রুৱ auto addCStmt = [=](ধ্রুৱ string& stmt) -> শূন্য {  //
        execGraphp->runfuncp()->addStmtsp(নূতন AstCStmt{fl, stmt});
    };

    যদি (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    যদি (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

শূন্য addThreadEndWrapper(AstExecGraph* ধ্রুৱ execGraphp) {
    // Add thread function invocations to execGraph
    ধ্রুৱ auto addCStmt = [=](ধ্রুৱ string& stmt) -> শূন্য {  //
        File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(নূতন AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    যদি (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
শূন্য addThreadStartToExecGraph(AstExecGraph* ধ্রুৱ execGraphp,
                               ধ্রুৱ std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_সাৰি* ধ্রুৱ fl = v3Global.rootp()->fileline();
    ধ্রুৱ string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ধ্রুৱ finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    বাবে (AstCFunc* ধ্রুৱ funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ধ্রুৱ auto addCStmt = [=](ধ্রুৱ string& stmt) -> শূন্য {  //
        execGraphp->runfuncp()->addStmtsp(নূতন AstCStmt{fl, stmt});
    };

    ধ্রুৱ uint32_t last = funcps.size() - 1;
    যদি (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    বাবে (AstCFunc* ধ্রুৱ funcp : funcps) {
        যদি (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            যদি (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } অন্যথা {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(নূতন AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } অন্যথা {
            // The last will run on the main thread.
            AstCCall* ধ্রুৱ callp = নূতন AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    যদি (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    যদি (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    যদি (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

শূন্য processMTaskBodies(AstExecGraph* ধ্রুৱ execGraphp) {
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ধ্রুৱ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ধ্রুৱ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_গ্ৰন্থি* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        ধ্রুৱ auto addCStmt = [=](ধ্রুৱ string& stmt) -> শূন্য {  //
            funcp->addStmtsp(নূতন AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        যদি (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            যদি (v3Global.opt.hierChild()) args += v3Global.opt.শীৰ্ষ_Module();
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
        যদি (v3Global.opt.profExec()) {
            ধ্রুৱ std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

শূন্য implementExecGraph(AstExecGraph* ধ্রুৱ execGraphp, ধ্রুৱ ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    যদি (execGraphp->depGraphp()->empty()) ঘূৰাই;

    // Create a function to be run by each thread.
    ধ্রুৱ std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

শূন্য createRunFunction(AstExecGraph* ধ্রুৱ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_সাৰি* ধ্রুৱ flp = execGraphp->fileline();
    Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = v3Global.rootp()->শীৰ্ষ_Modulep();
    AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(সঁচা);
    funcp->voidSelf(সঁচা);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(সঁচা);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

শূন্য addTriggerMasks(AstExecGraph* ধ্রুৱ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_সাৰি* ধ্রুৱ flp = execGraphp->fileline();
    AstVar* ধ্রুৱ triggersp = execGraphp->triggerp()->varp();
    বাবে (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ধ্রুৱ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            বাবে (ধ্রুৱ uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{flp, triggersp->dtypep(),
                                                     নূতন AstConst{flp, AstConst::Unsized64{}, 0}};
        বাবে (ধ্রুৱ auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          নূতন AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);
    }
}

শূন্য addTriggerCosts(AstExecGraph* ধ্রুৱ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_সাৰি* ধ্রুৱ flp = execGraphp->fileline();
    ধ্রুৱ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    বাবে (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            যদি (ধ্রুৱ AstTriggered* ধ্রুৱ trigp = VN_CAST(ifp->condp(), Triggered)) {
                ধ্রুৱ uint32_t cost = V3_Instr_গুনতি::গুনতি(ifp, মিছা);
                বাবে (ধ্রুৱ uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
    AstUnpackArrayDType* ধ্রুৱ dtypep
        = নূতন AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  নূতন AstRange{flp, static_cast<পূর্ণাঙ্ক>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{flp, dtypep, নূতন AstConst{flp, 0}};
    বাবে (uint32_t index = 0; index < nTriggers; ++index) {
        যদি (costs[index]) consp->valuep(index, নূতন AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);
}

// Called by Verilator top stage
শূন্য implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->শীৰ্ষ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    বাবে (AstExecGraph* ধ্রুৱ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        যদি (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_বৃক্ষ(), execGraphp);
            ঘূৰাই;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        যদি (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ধ্রুৱ std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<দ্ৱিগুণ>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        বাবে (ধ্রুৱ ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

শূন্য selfTest() {
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
