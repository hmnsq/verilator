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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3ExecGraph.h"

#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3GraphStream.h"
#অন্তর্ভাব "V3Hasher.h"
#অন্তর্ভাব "V3InstrCount.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্রন্থি_Stmt* stmtsp,
                                 uint32_t id) {
    ধ্রুবক std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* ধ্রুবক newp = নতুন AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(সত্য);
    newp->voidSelf(সত্য);  // Called through VlExecGraph::Fnp
    newp->dontCombine(সত্য);
    newp->addStmtsp(stmtsp);
    যদি (scopep) scopep->addBlocksp(newp);
    ফেরত newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_গ্রন্থি_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_পরবর্তী_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

শূন্য ExecMTask::dump(std::ostream& str) ধ্রুবক {
    str << name() << "." << cvtToHex(এই);
    যদি (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_পরবর্তী_Id{0};

নামাকাশ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
শ্রেণী ThreadSchedule final {
    friend শ্রেণী PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    স্থির uint32_t s_পরবর্তী_Id;  // Next ID number to use
    std::unordered_set<ধ্রুবক ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

সর্বজনীন:
    // CONSTANTS
    স্থির constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    কাঠামো MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ধ্রুবক ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<ধ্রুবক ExecMTask*>> m_threads;

    // Global state for each mtask.
    স্থির std::unordered_map<ধ্রুবক ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_পরবর্তী_Id++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = পূর্বনির্ধারিত;
    ThreadSchedule& operator=(ThreadSchedule&&) = পূর্বনির্ধারিত;

ব্যক্তিগত:
    VL_UNCOPYABLE(ThreadSchedule);

    স্থির constexpr দ্বিগুণ s_threadBoxWidth = 2.5;
    স্থির constexpr দ্বিগুণ s_threadBoxHeight = 1.5;
    স্থির constexpr দ্বিগুণ s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    স্থির শূন্য dumpDotFilePrefixedAlways(ধ্রুবক std::vector<ThreadSchedule>& schedules,
                                          ধ্রুবক string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    স্থির শূন্য dumpDotFile(ধ্রুবক std::vector<ThreadSchedule>& schedules, ধ্রুবক string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ধ্রুবক std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        যদি (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        জন্য (uint32_t i = 0; i < nThreads; ++i) {
            ধ্রুবক string name = "t" + std::to_string(i);
            ধ্রুবক string label = "Thread " + std::to_string(i);
            constexpr দ্বিগুণ posX = -s_horizontalGap;
            ধ্রুবক দ্বিগুণ posY = -static_cast<দ্বিগুণ>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        জন্য (ধ্রুবক auto& state : ThreadSchedule::s_mtaskState) {
            ধ্রুবক ExecMTask* ধ্রুবক mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        ধ্রুবক uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<দ্বিগুণ> offsets(nThreads, 0.0);
        জন্য (ধ্রুবক ThreadSchedule& schedule : schedules) {
            যদি (schedule.mtasks.empty()) চালু;
            using Column = std::vector<ধ্রুবক ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            কাঠামো Cmp final {
                যৌক্তিক operator()(ধ্রুবক ExecMTask* ধ্রুবক a, ধ্রুবক ExecMTask* ধ্রুবক b) ধ্রুবক {
                    যদি (startTime(a) == startTime(b)) ফেরত threadId(a) < threadId(b);
                    ফেরত startTime(a) < startTime(b);
                }
            };
            ধ্রুবক std::multiset<ধ্রুবক ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            জন্য (ধ্রুবক ExecMTask* ধ্রুবক mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.আকার_2() <= nThreads, "Invalid partitioning");

                যৌক্তিক intersects = সত্য;
                জন্য (ধ্রুবক ExecMTask* ধ্রুবক earlierMtask : column) {
                    যদি (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = মিথ্যা;
                        বিরতি;
                    }
                }
                যদি (intersects) {
                    column.emplace_back(mtaskp);
                } অন্যথা {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            জন্য (ধ্রুবক Column& column : columns) {
                দ্বিগুণ lastColumnOffset = 0;
                জন্য (ধ্রুবক ExecMTask* ধ্রুবক mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            জন্য (ধ্রুবক std::vector<ধ্রুবক ExecMTask*>& thread : schedule.m_threads) {
                যদি (thread.empty()) বিরতি;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<পূর্ণসংখ্যা>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                জন্য (আকার_t i = 1; i < thread.আকার_2(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                জন্য (ধ্রুবক ExecMTask* ধ্রুবক mtaskp : thread) {
                    জন্য (ধ্রুবক V3GraphEdge& edge : mtaskp->outEdges()) {
                        ধ্রুবক ExecMTask* ধ্রুবক শীর্ষ_MTaskp = edge.শীর্ষ()->cast<ধ্রুবক ExecMTask>();
                        যদি (শীর্ষ_MTaskp && schedule.contains(শীর্ষ_MTaskp)
                            && threadId(শীর্ষ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << শীর্ষ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    স্থির শূন্য dumpDotFileEmitBlock(ধ্রুবক std::unique_ptr<std::ofstream>& logp,
                                     ধ্রুবক string& name, ধ্রুবক string& label, দ্বিগুণ width,
                                     দ্বিগুণ height, দ্বিগুণ xPos, দ্বিগুণ yPos,
                                     ধ্রুবক string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    স্থির শূন্য dumpDotFileEmitMTask(ধ্রুবক std::unique_ptr<std::ofstream>& logp,
                                     ধ্রুবক ExecMTask* ধ্রুবক mtaskp, ধ্রুবক ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<দ্বিগুণ>& offsets) {
        জন্য (পূর্ণসংখ্যা i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ধ্রুবক string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            ধ্রুবক string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            ধ্রুবক দ্বিগুণ width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<দ্বিগুণ>(mtaskp->cost()) / segmentCost);
            ধ্রুবক uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.আকার_2();
            ধ্রুবক দ্বিগুণ xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            ধ্রুবক দ্বিগুণ yPos = -s_threadBoxHeight * static_cast<দ্বিগুণ>(mtaskThreadId);
            ধ্রুবক string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    স্থির শূন্য dumpDotFileEmitFork(ধ্রুবক std::unique_ptr<std::ofstream>& logp, দ্বিগুণ offset,
                                    uint32_t nThreads) {
        ধ্রুবক string& name = "fork_" + std::to_string(static_cast<পূর্ণসংখ্যা>(offset));
        constexpr দ্বিগুণ width = s_threadBoxWidth / 8;
        ধ্রুবক দ্বিগুণ height = s_threadBoxHeight * nThreads;
        ধ্রুবক দ্বিগুণ xPos = offset - s_horizontalGap / 2;
        ধ্রুবক দ্বিগুণ yPos
            = -static_cast<দ্বিগুণ>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

সর্বজনীন:
    স্থির uint32_t threadId(ধ্রুবক ExecMTask* mtaskp) {
        ধ্রুবক auto& it = s_mtaskState.find(mtaskp);
        ফেরত it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    স্থির uint32_t startTime(ধ্রুবক ExecMTask* mtaskp) {
        ফেরত s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    স্থির uint32_t endTime(ধ্রুবক ExecMTask* mtaskp) {
        ফেরত s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ধ্রুবক ExecMTask* mtaskp) ধ্রুবক {
        ধ্রুবক uint32_t thisThreadId = threadId(mtaskp);
        uint32_t ফলাফল = 0;
        জন্য (ধ্রুবক V3GraphEdge& edge : mtaskp->inEdges()) {
            ধ্রুবক ExecMTask* ধ্রুবক prevp = edge.fromp()->as<ExecMTask>();
            যদি (threadId(prevp) != thisThreadId && contains(prevp)) ++ফলাফল;
        }
        ফেরত ফলাফল;
    }

    uint32_t id() ধ্রুবক { ফেরত m_id; }
    uint32_t scheduleOn(ধ্রুবক ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ধ্রুবক uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<ধ্রুবক ExecMTask*>& bestThread = m_threads[bestThreadId];
        যদি (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.ঠেলো_back(mtaskp);
        ফেরত bestEndTime;
    }
    যৌক্তিক contains(ধ্রুবক ExecMTask* mtaskp) ধ্রুবক { ফেরত mtasks.গুনতি(mtaskp); }
    uint32_t endTime() ধ্রুবক { ফেরত m_endTime; }
};

uint32_t ThreadSchedule::s_পরবর্তী_Id = 0;
std::unordered_map<ধ্রুবক ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr দ্বিগুণ V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
শ্রেণী PackThreads final {
    // TYPES
    কাঠামো MTaskCmp final {
        যৌক্তিক operator()(ধ্রুবক ExecMTask* ap, ধ্রুবক ExecMTask* bp) ধ্রুবক {
            ফেরত ap->id() < bp->id();
        }
    };

    // MEMBERS
    ধ্রুবক uint32_t m_nThreads;  // Number of threads
    ধ্রুবক uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    ধ্রুবক uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ধ্রুবক uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(ধ্রুবক ThreadSchedule& schedule, ধ্রুবক ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        যদি (!schedule.contains(mtaskp)) ফেরত 0;
        ধ্রুবক ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        যদি (threadId == state.threadId) {
            // No overhead on same thread
            ফেরত state.completionTime;
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
            ধ্রুবক uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            যদি ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        ফেরত sandbaggedEndTime;
    }

    স্থির যৌক্তিক isReady(ThreadSchedule& schedule, ধ্রুবক ExecMTask* mtaskp) {
        জন্য (ধ্রুবক V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            ধ্রুবক ExecMTask* ধ্রুবক prevp = edgeIn.fromp()->as<ধ্রুবক ExecMTask>();
            যদি (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                ফেরত মিথ্যা;
            }
        }
        ফেরত সত্য;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::vector<ThreadSchedule> pack(V3Graph& mtaskGraph) {
        std::vector<ThreadSchedule> ফলাফল;
        ফলাফল.emplace_back(ThreadSchedule{m_nThreads});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        গণনা শ্রেণী SchedulingMode : uint8_t {
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
        পূর্ণসংখ্যা maxThreadWorkers = 1;

        // Build initial ready list
        জন্য (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ধ্রুবক mtaskp = vtx.as<ExecMTask>();
            যদি (isReady(ফলাফল.back(), mtaskp)) readyMTasks.ঢোকাও(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        যখন (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = ফলাফল.back();
            জন্য (uint32_t threadId = 0; threadId < schedule.m_threads.আকার_2(); ++threadId) {
                জন্য (ExecMTask* ধ্রুবক mtaskp : readyMTasks) {
                    যদি (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        চালু;
                    }
                    যদি (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        চালু;

                    uint32_t timeBegin = busyUntil[threadId];
                    যদি (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        বিরতি;
                    }
                    জন্য (ধ্রুবক V3GraphEdge& edge : mtaskp->inEdges()) {
                        ধ্রুবক ExecMTask* ধ্রুবক priorp = edge.fromp()->as<ExecMTask>();
                        ধ্রুবক uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
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

            ধ্রুবক uint32_t endTime = schedule.endTime();

            যদি (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                ধ্রুবক uint32_t আকার_2 = m_nHierThreads / maxThreadWorkers;
                UASSERT(আকার_2, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                যদি (schedule.s_mtaskState.empty()) ফলাফল.clear();
                ফলাফল.emplace_back(ThreadSchedule{আকার_2});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                চালু;
            }

            যদি (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                ফলাফল.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                চালু;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ধ্রুবক uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            ধ্রুবক আকার_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            জন্য (ধ্রুবক V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* ধ্রুবক nextp = edgeOut.শীর্ষ()->as<ExecMTask>();
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
            ThreadSchedule::dumpDotFilePrefixedAlways(ফলাফল, "schedule", m_nThreads);

        ফেরত ফলাফল;
    }

সর্বজনীন:
    // SELF TEST
    স্থির শূন্য selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_সারি* ধ্রুবক flp = v3Global.rootp()->fileline();
        AstVar* ধ্রুবক varp = নতুন AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ধ্রুবক scopep = নতুন AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* ধ্রুবক trigVscp = নতুন AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_বৃক্ষ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_বৃক্ষ(), scopep);
        VL_DO_DANGLING(varp->delete_বৃক্ষ(), varp);
    }
    স্থির শূন্য selfTestNormalFirst(AstVarScope* trigVscp) {
        File_সারি* ধ্রুবক flp = v3Global.rootp()->fileline();
        AstExecGraph* ধ্রুবক execGraphp = নতুন AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ধ্রুবক auto makeBody = [&]() -> Ast_গ্রন্থি_Stmt* { ফেরত নতুন AstComment{flp, ""}; };
        ExecMTask* ধ্রুবক t0 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* ধ্রুবক t1 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* ধ্রুবক t2 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* ধ্রুবক t3 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* ধ্রুবক t4 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* ধ্রুবক t5 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* ধ্রুবক t6 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
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
        নতুন V3GraphEdge{&graph, t0, t1, 1};
        নতুন V3GraphEdge{&graph, t0, t2, 1};
        নতুন V3GraphEdge{&graph, t2, t3, 1};
        নতুন V3GraphEdge{&graph, t2, t4, 1};
        নতুন V3GraphEdge{&graph, t3, t5, 1};
        নতুন V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ধ্রুবক std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.আকার_2(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.আকার_2(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].আকার_2(), 2);
        জন্য (আকার_t i = 1; i < scheduled[0].m_threads.আকার_2(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].আকার_2(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_threads[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_threads.আকার_2(), hierThreads / 3);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_threads[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_threads.আকার_2(), threads);
        UASSERT_SELFTEST(scheduled[2].m_threads[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_threads[1][0], t6);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.আকার_2(), 7);

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

        জন্য (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_বৃক্ষ();
        VL_DO_DANGLING(execGraphp->delete_বৃক্ষ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    স্থির শূন্য selfTestHierFirst(AstVarScope* trigVscp) {
        File_সারি* ধ্রুবক flp = v3Global.rootp()->fileline();
        AstExecGraph* ধ্রুবক execGraphp = নতুন AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ধ্রুবক auto makeBody = [&]() -> Ast_গ্রন্থি_Stmt* { ফেরত নতুন AstComment{flp, ""}; };
        ExecMTask* ধ্রুবক t0 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* ধ্রুবক t1 = নতুন ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        নতুন V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ধ্রুবক std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.আকার_2(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.আকার_2(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].আকার_2(), 1);
        জন্য (আকার_t i = 1; i < scheduled[0].m_threads.আকার_2(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].আকার_2(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.আকার_2(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].আকার_2(), 1);
        জন্য (আকার_t i = 1; i < scheduled[1].m_threads.আকার_2(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].আকার_2(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.আকার_2(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        জন্য (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_বৃক্ষ();
        VL_DO_DANGLING(execGraphp->delete_বৃক্ষ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    স্থির std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        ফেরত PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

শূন্য normalizeCosts(Costs& costs) {
    ধ্রুবক auto scaleCost = [](uint64_t মান, দ্বিগুণ multiplier) {
        দ্বিগুণ scaled = static_cast<দ্বিগুণ>(মান) * multiplier;
        যদি (মান && scaled < 1) scaled = 1;
        ফেরত static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t যোগ_Cost_Profiled = 0;  // For data with estimate and profile
    uint64_t যোগ_Cost_Estimate = 0;  // For data with estimate and profile
    জন্য (ধ্রুবক auto& est : costs) {
        যদি (est.second.second) {
            যোগ_Cost_Estimate += est.second.first;
            যোগ_Cost_Profiled += est.second.second;
        }
    }

    যদি (যোগ_Cost_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ধ্রুবক দ্বিগুণ estToProfile
            = static_cast<দ্বিগুণ>(যোগ_Cost_Profiled) / static_cast<দ্বিগুণ>(যোগ_Cost_Estimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << যোগ_Cost_Profiled
                                                    << " sumCostEstimate=" << যোগ_Cost_Estimate);
        জন্য (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    জন্য (auto& est : costs) {
        ধ্রুবক uint64_t& costEstimate = est.second.first;
        ধ্রুবক uint64_t& costProfiled = est.second.second;
        যদি (maxCost < costEstimate) maxCost = costEstimate;
        যদি (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    যদি (maxCost > scaleDownTo) {
        ধ্রুবক দ্বিগুণ scaleup = static_cast<দ্বিগুণ>(scaleDownTo) / static_cast<দ্বিগুণ>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        জন্য (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

শূন্য removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    জন্য (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        যদি (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_বৃক্ষ(), callp);
    }
    V3Graph* ধ্রুবক execMTaskGraphp = execGraphp->depGraphp();
    জন্য (V3GraphVertex* ধ্রুবক vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ধ্রুবক mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ধ্রুবক funcp = mtaskp->funcp();
        যদি (funcp->stmtsp()) চালু;

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

    জন্য (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ধ্রুবক mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ধ্রুবক uint64_t costEstimate = V3_Instr_গুনতি::গুনতি(mtp->funcp(), মিথ্যা);
        ধ্রুবক uint64_t costProfiled
            = V3Control::get_Profile_তথ্য(v3Global.opt.prefix(), mtp->hashName());
        যদি (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    পূর্ণসংখ্যা যোগ_Estimates = 0;
    পূর্ণসংখ্যা missingProfiles = 0;
    জন্য (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ধ্রুবক mtp = vtx.as<ExecMTask>();
        ধ্রুবক uint32_t costEstimate = costs[mtp->id()].first;
        ধ্রুবক uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ধ্রুবক uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        যদি (!costProfiled32) {
            costToUse = costEstimate;
            যদি (costEstimate != 0) ++missingProfiles;
        }
        যদি (costEstimate != 0) ++যোগ_Estimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    যদি (missingProfiles) {
        যদি (File_সারি* ধ্রুবক fl = V3Control::get_Profile_তথ্য_File_সারি()) {
            যদি (V3Control::contains_MTask_Profile_তথ্য()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << যোগ_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

শূন্য finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    যখন (ধ্রুবক V3GraphVertex* ধ্রুবক vxp = ser.nextp()) {
        ExecMTask* ধ্রুবক mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        জন্য (V3GraphEdge& edge : mtp->outEdges()) {
            ধ্রুবক ExecMTask* ধ্রুবক followp = edge.শীর্ষ()->as<ExecMTask>();
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
    ধ্রুবক auto report = execMTaskGraphp->parallelismReport([](ধ্রুবক V3GraphVertex* vtxp) {  //
        ফেরত vtxp->as<ধ্রুবক ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.যোগ_Graph_Cost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_গুনতি());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_গুনতি());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    যদি (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.যোগ_Graph_Cost());
        UINFO(0, "    MTask vertex count = " << report.vertex_গুনতি());
        UINFO(0, "    Edge count = " << report.edge_গুনতি());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ধ্রুবক std::string& name, uint32_t nDependencies) {
    Ast_গ্রন্থি_Module* ধ্রুবক modp = v3Global.rootp()->শীর্ষ_Modulep();
    File_সারি* ধ্রুবক fl = modp->fileline();
    AstBasicDType* ধ্রুবক dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ধ্রুবক varp = নতুন AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(সত্য);
    varp->valuep(নতুন AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    ফেরত varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ধ্রুবক std::string& method) {
    File_সারি* ধ্রুবক fl = varp->fileline();
    AstVarRef* ধ্রুবক refp = নতুন AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ধ্রুবক AstScope* ধ্রুবক শীর্ষ_Scopep = v3Global.rootp()->শীর্ষ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, শীর্ষ_Scopep->nameDotless()});
    AstCStmt* ধ্রুবক stmtp = নতুন AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    ফেরত stmtp;
}

শূন্য addMTaskToFunction(ধ্রুবক ThreadSchedule& schedule, ধ্রুবক uint32_t threadId, AstCFunc* funcp,
                        ধ্রুবক ExecMTask* mtaskp,
                        ধ্রুবক std::unordered_map<ধ্রুবক ExecMTask*, AstVar*>& stateVars) {
    AstScope* ধ্রুবক scopep = v3Global.rootp()->শীর্ষ_Scopep()->scopep();
    Ast_গ্রন্থি_Module* ধ্রুবক modp = v3Global.rootp()->শীর্ষ_Modulep();
    File_সারি* ধ্রুবক fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ধ্রুবক auto addCStmt = [=](ধ্রুবক string& stmt) -> শূন্য {  //
        funcp->addStmtsp(নতুন AstCStmt{fl, stmt});
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
    AstCCall* ধ্রুবক callp = নতুন AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    যদি (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    জন্য (ধ্রুবক V3GraphEdge& edge : mtaskp->outEdges()) {
        ধ্রুবক ExecMTask* ধ্রুবক nextp = edge.শীর্ষ()->as<ExecMTask>();
        যদি (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ধ্রুবক std::vector<AstCFunc*> createThreadFunctions(ধ্রুবক ThreadSchedule& schedule,
                                                   ধ্রুবক string& tag) {
    Ast_গ্রন্থি_Module* ধ্রুবক modp = v3Global.rootp()->শীর্ষ_Modulep();
    File_সারি* ধ্রুবক fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ধ্রুবক ExecMTask*, AstVar*> stateVars;
    জন্য (ধ্রুবক std::vector<ধ্রুবক ExecMTask*>& thread : schedule.m_threads) {
        জন্য (ধ্রুবক ExecMTask* ধ্রুবক mtaskp : thread) {
            যদি (ধ্রুবক uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    জন্য (ধ্রুবক std::vector<ধ্রুবক ExecMTask*>& thread : schedule.m_threads) {
        যদি (thread.empty()) চালু;
        ধ্রুবক uint32_t threadId = schedule.threadId(thread.front());
        ধ্রুবক string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.ঠেলো_back(funcp);
        funcp->isStatic(সত্য);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(সত্য);
        funcp->entryPoint(সত্য);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(নতুন AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(নতুন AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        জন্য (ধ্রুবক ExecMTask* ধ্রুবক mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    ফেরত funcps;
}

শূন্য addThreadStartWrapper(AstExecGraph* ধ্রুবক execGraphp) {
    // FileLine used for constructing nodes below
    File_সারি* ধ্রুবক fl = v3Global.rootp()->fileline();
    ধ্রুবক string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    ধ্রুবক auto addCStmt = [=](ধ্রুবক string& stmt) -> শূন্য {  //
        execGraphp->runfuncp()->addStmtsp(নতুন AstCStmt{fl, stmt});
    };

    যদি (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    যদি (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

শূন্য addThreadEndWrapper(AstExecGraph* ধ্রুবক execGraphp) {
    // Add thread function invocations to execGraph
    ধ্রুবক auto addCStmt = [=](ধ্রুবক string& stmt) -> শূন্য {  //
        File_সারি* ধ্রুবক flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(নতুন AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    যদি (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
শূন্য addThreadStartToExecGraph(AstExecGraph* ধ্রুবক execGraphp,
                               ধ্রুবক std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_সারি* ধ্রুবক fl = v3Global.rootp()->fileline();
    ধ্রুবক string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ধ্রুবক finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.আকার_2());
    জন্য (AstCFunc* ধ্রুবক funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ধ্রুবক auto addCStmt = [=](ধ্রুবক string& stmt) -> শূন্য {  //
        execGraphp->runfuncp()->addStmtsp(নতুন AstCStmt{fl, stmt});
    };

    ধ্রুবক uint32_t last = funcps.আকার_2() - 1;
    যদি (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    জন্য (AstCFunc* ধ্রুবক funcp : funcps) {
        যদি (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ধ্রুবক cstmtp = নতুন AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            যদি (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } অন্যথা {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(নতুন AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } অন্যথা {
            // The last will run on the main thread.
            AstCCall* ধ্রুবক callp = নতুন AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::add_Stat_যোগ("Optimizations, Thread schedule total tasks", i);

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

শূন্য processMTaskBodies(AstExecGraph* ধ্রুবক execGraphp) {
    জন্য (V3GraphVertex* ধ্রুবক vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ধ্রুবক mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ধ্রুবক funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_গ্রন্থি* stmtsp = funcp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();

        // Helper function to make the code a bit more legible
        ধ্রুবক auto addCStmt = [=](ধ্রুবক string& stmt) -> শূন্য {  //
            funcp->addStmtsp(নতুন AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        যদি (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            যদি (v3Global.opt.hierChild()) args += v3Global.opt.শীর্ষ_Module();
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
            ধ্রুবক std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

শূন্য implementExecGraph(AstExecGraph* ধ্রুবক execGraphp, ধ্রুবক ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    যদি (execGraphp->depGraphp()->empty()) ফেরত;

    // Create a function to be run by each thread.
    ধ্রুবক std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

শূন্য createRunFunction(AstExecGraph* ধ্রুবক execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_সারি* ধ্রুবক flp = execGraphp->fileline();
    Ast_গ্রন্থি_Module* ধ্রুবক modp = v3Global.rootp()->শীর্ষ_Modulep();
    AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(সত্য);
    funcp->voidSelf(সত্য);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(সত্য);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

শূন্য addTriggerMasks(AstExecGraph* ধ্রুবক execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_সারি* ধ্রুবক flp = execGraphp->fileline();
    AstVar* ধ্রুবক triggersp = execGraphp->triggerp()->varp();
    জন্য (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ধ্রুবক AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            জন্য (ধ্রুবক uint32_t সূচক : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(সূচক)] |= VL_BIT_Q(সূচক);
            }
        });
        AstConsArray* ধ্রুবক consp = নতুন AstConsArray{flp, triggersp->dtypep(),
                                                     নতুন AstConst{flp, AstConst::Unsized64{}, 0}};
        জন্য (ধ্রুবক auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          নতুন AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);
    }
}

শূন্য addTriggerCosts(AstExecGraph* ধ্রুবক execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_সারি* ধ্রুবক flp = execGraphp->fileline();
    ধ্রুবক uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    জন্য (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            যদি (ধ্রুবক AstTriggered* ধ্রুবক trigp = VN_CAST(ifp->condp(), Triggered)) {
                ধ্রুবক uint32_t cost = V3_Instr_গুনতি::গুনতি(ifp, মিথ্যা);
                জন্য (ধ্রুবক uint32_t সূচক : trigp->indices()) costs.at(সূচক) += cost;
            }
        });
    }
    AstNetlist* ধ্রুবক netlistp = v3Global.rootp();
    AstUnpackArrayDType* ধ্রুবক dtypep
        = নতুন AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  নতুন AstRange{flp, static_cast<পূর্ণসংখ্যা>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* ধ্রুবক consp = নতুন AstConsArray{flp, dtypep, নতুন AstConst{flp, 0}};
    জন্য (uint32_t সূচক = 0; সূচক < nTriggers; ++সূচক) {
        যদি (costs[সূচক]) consp->valuep(সূচক, নতুন AstConst{flp, costs[সূচক]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);
}

// Called by Verilator top stage
শূন্য implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->শীর্ষ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    জন্য (AstExecGraph* ধ্রুবক execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        যদি (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_বৃক্ষ(), execGraphp);
            ফেরত;
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
        ধ্রুবক std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::add_Stat_যোগ("Optimizations, Thread schedule count",
                            static_cast<দ্বিগুণ>(packed.আকার_2()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        জন্য (ধ্রুবক ThreadSchedule& schedule : packed) {
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
