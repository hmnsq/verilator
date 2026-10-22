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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3ExecGraph.h"

#ضمن "V3ConstPool.h"
#ضمن "V3Control.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3File.h"
#ضمن "V3GraphStream.h"
#ضمن "V3Hasher.h"
#ضمن "V3InstrCount.h"
#ضمن "V3Os.h"
#ضمن "V3Stats.h"

#ضمن <map>
#ضمن <memory>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_گرێ_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* ثابت newp = جديد AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(true);
    newp->voidSelf(true);  // Called through VlExecGraph::Fnp
    newp->dontCombine(true);
    newp->addStmtsp(stmtsp);
    إذا (scopep) scopep->addBlocksp(newp);
    أرجع newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_گرێ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

فراغ ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvtToHex(هذا);
    إذا (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

نطاق V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
صنف ThreadSchedule final {
    friend صنف PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    ساكن uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

عام:
    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    بنية MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<ثابت ExecMTask*>> m_threads;

    // Global state for each mtask.
    ساكن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = افتراضي;
    ThreadSchedule& operator=(ThreadSchedule&&) = افتراضي;

خاص:
    VL_UNCOPYABLE(ThreadSchedule);

    ساكن constexpr مزدوج s_threadBoxWidth = 2.5;
    ساكن constexpr مزدوج s_threadBoxHeight = 1.5;
    ساكن constexpr مزدوج s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساكن فراغ dumpDotFilePrefixedAlways(ثابت std::vector<ThreadSchedule>& schedules,
                                          ثابت string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    ساكن فراغ dumpDotFile(ثابت std::vector<ThreadSchedule>& schedules, ثابت string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        إذا (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        لكل (uint32_t i = 0; i < nThreads; ++i) {
            ثابت string name = "t" + std::to_string(i);
            ثابت string label = "Thread " + std::to_string(i);
            constexpr مزدوج posX = -s_horizontalGap;
            ثابت مزدوج posY = -static_cast<مزدوج>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        لكل (ثابت auto& state : ThreadSchedule::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        ثابت uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<مزدوج> offsets(nThreads, 0.0);
        لكل (ثابت ThreadSchedule& schedule : schedules) {
            إذا (schedule.mtasks.empty()) استمر;
            using Column = std::vector<ثابت ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            بنية Cmp final {
                منطقي operator()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    إذا (startTime(a) == startTime(b)) أرجع threadId(a) < threadId(b);
                    أرجع startTime(a) < startTime(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            لكل (ثابت ExecMTask* ثابت mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                منطقي intersects = true;
                لكل (ثابت ExecMTask* ثابت earlierMtask : column) {
                    إذا (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = خطأ;
                        اكسر;
                    }
                }
                إذا (intersects) {
                    column.emplace_back(mtaskp);
                } وإلا {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            لكل (ثابت Column& column : columns) {
                مزدوج lastColumnOffset = 0;
                لكل (ثابت ExecMTask* ثابت mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            لكل (ثابت std::vector<ثابت ExecMTask*>& thread : schedule.m_threads) {
                إذا (thread.empty()) اكسر;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<صحيح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                لكل (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
                    لكل (ثابت V3GraphEdge& edge : mtaskp->outEdges()) {
                        ثابت ExecMTask* ثابت لووتکە_MTaskp = edge.لووتکە()->cast<ثابت ExecMTask>();
                        إذا (لووتکە_MTaskp && schedule.contains(لووتکە_MTaskp)
                            && threadId(لووتکە_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << لووتکە_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ساكن فراغ dumpDotFileEmitBlock(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, مزدوج width,
                                     مزدوج height, مزدوج xPos, مزدوج yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساكن فراغ dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<مزدوج>& offsets) {
        لكل (صحيح i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            ثابت string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            ثابت مزدوج width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<مزدوج>(mtaskp->cost()) / segmentCost);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            ثابت مزدوج xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            ثابت مزدوج yPos = -s_threadBoxHeight * static_cast<مزدوج>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    ساكن فراغ dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, مزدوج offset,
                                    uint32_t nThreads) {
        ثابت string& name = "fork_" + std::to_string(static_cast<صحيح>(offset));
        constexpr مزدوج width = s_threadBoxWidth / 8;
        ثابت مزدوج height = s_threadBoxHeight * nThreads;
        ثابت مزدوج xPos = offset - s_horizontalGap / 2;
        ثابت مزدوج yPos
            = -static_cast<مزدوج>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

عام:
    ساكن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت auto& it = s_mtaskState.find(mtaskp);
        أرجع it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    ساكن uint32_t startTime(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    ساكن uint32_t endTime(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        لكل (ثابت V3GraphEdge& edge : mtaskp->inEdges()) {
            ثابت ExecMTask* ثابت prevp = edge.fromp()->as<ExecMTask>();
            إذا (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        أرجع result;
    }

    uint32_t id() ثابت { أرجع m_id; }
    uint32_t scheduleOn(ثابت ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<ثابت ExecMTask*>& bestThread = m_threads[bestThreadId];
        إذا (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.پاڵنان_back(mtaskp);
        أرجع bestEndTime;
    }
    منطقي contains(ثابت ExecMTask* mtaskp) ثابت { أرجع mtasks.ژماردن(mtaskp); }
    uint32_t endTime() ثابت { أرجع m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<ثابت ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr مزدوج V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
صنف PackThreads final {
    // TYPES
    بنية MTaskCmp final {
        منطقي operator()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            أرجع ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_nThreads;  // Number of threads
    ثابت uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = افتراضي;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(ثابت ThreadSchedule& schedule, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        إذا (!schedule.contains(mtaskp)) أرجع 0;
        ثابت ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        إذا (threadId == state.threadId) {
            // No overhead on same thread
            أرجع state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        إذا (state.nextp) {
            ثابت uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            إذا ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        أرجع sandbaggedEndTime;
    }

    ساكن منطقي isReady(ThreadSchedule& schedule, ثابت ExecMTask* mtaskp) {
        لكل (ثابت V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            ثابت ExecMTask* ثابت prevp = edgeIn.fromp()->as<ثابت ExecMTask>();
            إذا (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                أرجع خطأ;
            }
        }
        أرجع true;
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
        تعداد صنف SchedulingMode : uint8_t {
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
        صحيح maxThreadWorkers = 1;

        // Build initial ready list
        لكل (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            إذا (isReady(result.back(), mtaskp)) readyMTasks.زیادکردن(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        طالما (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            لكل (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                لكل (ExecMTask* ثابت mtaskp : readyMTasks) {
                    إذا (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        استمر;
                    }
                    إذا (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        استمر;

                    uint32_t timeBegin = busyUntil[threadId];
                    إذا (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        اكسر;
                    }
                    لكل (ثابت V3GraphEdge& edge : mtaskp->inEdges()) {
                        ثابت ExecMTask* ثابت priorp = edge.fromp()->as<ExecMTask>();
                        ثابت uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        إذا (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    إذا ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t endTime = schedule.endTime();

            إذا (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                ثابت uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                إذا (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                استمر;
            }

            إذا (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                استمر;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ثابت uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            ثابت size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            لكل (ثابت V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* ثابت nextp = edgeOut.لووتکە()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                إذا (isReady(schedule, nextp)) {
                    readyMTasks.زیادکردن(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        إذا (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        أرجع result;
    }

عام:
    // SELF TEST
    ساكن فراغ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_هێڵ* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ثابت scopep = جديد AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* ثابت trigVscp = جديد AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_درەخت(), trigVscp);
        VL_DO_DANGLING(scopep->delete_درەخت(), scopep);
        VL_DO_DANGLING(varp->delete_درەخت(), varp);
    }
    ساكن فراغ selfTestNormalFirst(AstVarScope* trigVscp) {
        File_هێڵ* ثابت flp = v3Global.rootp()->fileline();
        AstExecGraph* ثابت execGraphp = جديد AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ثابت auto makeBody = [&]() -> Ast_گرێ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* ثابت t2 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* ثابت t3 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* ثابت t4 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* ثابت t5 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* ثابت t6 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
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
        جديد V3GraphEdge{&graph, t0, t1, 1};
        جديد V3GraphEdge{&graph, t0, t2, 1};
        جديد V3GraphEdge{&graph, t2, t3, 1};
        جديد V3GraphEdge{&graph, t2, t4, 1};
        جديد V3GraphEdge{&graph, t3, t5, 1};
        جديد V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        لكل (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        لكل (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درەخت();
        VL_DO_DANGLING(execGraphp->delete_درەخت(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    ساكن فراغ selfTestHierFirst(AstVarScope* trigVscp) {
        File_هێڵ* ثابت flp = v3Global.rootp()->fileline();
        AstExecGraph* ثابت execGraphp = جديد AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ثابت auto makeBody = [&]() -> Ast_گرێ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        جديد V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        لكل (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        لكل (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        لكل (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درەخت();
        VL_DO_DANGLING(execGraphp->delete_درەخت(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    ساكن std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        أرجع PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

فراغ normalizeCosts(Costs& costs) {
    ثابت auto scaleCost = [](uint64_t بەها, مزدوج multiplier) {
        مزدوج scaled = static_cast<مزدوج>(بەها) * multiplier;
        إذا (بەها && scaled < 1) scaled = 1;
        أرجع static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    لكل (ثابت auto& est : costs) {
        إذا (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    إذا (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت مزدوج estToProfile
            = static_cast<مزدوج>(sumCostProfiled) / static_cast<مزدوج>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        لكل (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    لكل (auto& est : costs) {
        ثابت uint64_t& costEstimate = est.second.first;
        ثابت uint64_t& costProfiled = est.second.second;
        إذا (maxCost < costEstimate) maxCost = costEstimate;
        إذا (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    إذا (maxCost > scaleDownTo) {
        ثابت مزدوج scaleup = static_cast<مزدوج>(scaleDownTo) / static_cast<مزدوج>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        لكل (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

فراغ removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    لكل (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        إذا (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_درەخت(), callp);
    }
    V3Graph* ثابت execMTaskGraphp = execGraphp->depGraphp();
    لكل (V3GraphVertex* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        إذا (funcp->stmtsp()) استمر;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_درەخت(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

فراغ fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    لكل (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t costEstimate = V3_Instr_ژماردن::ژماردن(mtp->funcp(), خطأ);
        ثابت uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        إذا (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    صحيح totalEstimates = 0;
    صحيح missingProfiles = 0;
    لكل (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t costEstimate = costs[mtp->id()].first;
        ثابت uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        إذا (!costProfiled32) {
            costToUse = costEstimate;
            إذا (costEstimate != 0) ++missingProfiles;
        }
        إذا (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    إذا (missingProfiles) {
        إذا (File_هێڵ* ثابت fl = V3Control::get_Profile_Data_File_هێڵ()) {
            إذا (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

فراغ finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    طالما (ثابت V3GraphVertex* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        لكل (V3GraphEdge& edge : mtp->outEdges()) {
            ثابت ExecMTask* ثابت followp = edge.لووتکە()->as<ExecMTask>();
            إذا ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    ثابت auto report = execMTaskGraphp->parallelismReport([](ثابت V3GraphVertex* vtxp) {  //
        أرجع vtxp->as<ثابت ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_ژماردن());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_ژماردن());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    إذا (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_ژماردن());
        UINFO(0, "    Edge count = " << report.edge_ژماردن());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_گرێ_Module* ثابت modp = v3Global.rootp()->لووتکە_Modulep();
    File_هێڵ* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(true);
    varp->valuep(جديد AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    أرجع varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& method) {
    File_هێڵ* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت لووتکە_Scopep = v3Global.rootp()->لووتکە_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, لووتکە_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = جديد AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    أرجع stmtp;
}

فراغ addMTaskToFunction(ثابت ThreadSchedule& schedule, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->لووتکە_Scopep()->scopep();
    Ast_گرێ_Module* ثابت modp = v3Global.rootp()->لووتکە_Modulep();
    File_هێڵ* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت auto addCStmt = [=](ثابت string& stmt) -> فراغ {  //
        funcp->addStmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        إذا (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        إذا (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    إذا (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = جديد AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    إذا (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    لكل (ثابت V3GraphEdge& edge : mtaskp->outEdges()) {
        ثابت ExecMTask* ثابت nextp = edge.لووتکە()->as<ExecMTask>();
        إذا (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::vector<AstCFunc*> createThreadFunctions(ثابت ThreadSchedule& schedule,
                                                   ثابت string& tag) {
    Ast_گرێ_Module* ثابت modp = v3Global.rootp()->لووتکە_Modulep();
    File_هێڵ* ثابت fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    لكل (ثابت std::vector<ثابت ExecMTask*>& thread : schedule.m_threads) {
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            إذا (ثابت uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    لكل (ثابت std::vector<ثابت ExecMTask*>& thread : schedule.m_threads) {
        إذا (thread.empty()) استمر;
        ثابت uint32_t threadId = schedule.threadId(thread.front());
        ثابت string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* ثابت funcp = جديد AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.پاڵنان_back(funcp);
        funcp->isStatic(true);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(true);
        funcp->entryPoint(true);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(جديد AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(جديد AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    أرجع funcps;
}

فراغ addThreadStartWrapper(AstExecGraph* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_هێڵ* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت auto addCStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->addStmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    إذا (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

فراغ addThreadEndWrapper(AstExecGraph* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت auto addCStmt = [=](ثابت string& stmt) -> فراغ {  //
        File_هێڵ* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(جديد AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    إذا (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
فراغ addThreadStartToExecGraph(AstExecGraph* ثابت execGraphp,
                               ثابت std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_هێڵ* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    لكل (AstCFunc* ثابت funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت auto addCStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->addStmtsp(جديد AstCStmt{fl, stmt});
    };

    ثابت uint32_t last = funcps.size() - 1;
    إذا (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    لكل (AstCFunc* ثابت funcp : funcps) {
        إذا (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            إذا (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } وإلا {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(جديد AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } وإلا {
            // The last will run on the main thread.
            AstCCall* ثابت callp = جديد AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    إذا (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    إذا (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    إذا (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

فراغ processMTaskBodies(AstExecGraph* ثابت execGraphp) {
    لكل (V3GraphVertex* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_گرێ* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        ثابت auto addCStmt = [=](ثابت string& stmt) -> فراغ {  //
            funcp->addStmtsp(جديد AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        إذا (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            إذا (v3Global.opt.hierChild()) args += v3Global.opt.لووتکە_Module();
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
        إذا (v3Global.opt.profExec()) {
            ثابت std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

فراغ implementExecGraph(AstExecGraph* ثابت execGraphp, ثابت ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    إذا (execGraphp->depGraphp()->empty()) أرجع;

    // Create a function to be run by each thread.
    ثابت std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

فراغ createRunFunction(AstExecGraph* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_هێڵ* ثابت flp = execGraphp->fileline();
    Ast_گرێ_Module* ثابت modp = v3Global.rootp()->لووتکە_Modulep();
    AstCFunc* ثابت funcp = جديد AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(true);
    funcp->voidSelf(true);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(true);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

فراغ addTriggerMasks(AstExecGraph* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_هێڵ* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            لكل (ثابت uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* ثابت consp = جديد AstConsArray{flp, triggersp->dtypep(),
                                                     جديد AstConst{flp, AstConst::Unsized64{}, 0}};
        لكل (ثابت auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          جديد AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_درەخت(), consp);
    }
}

فراغ addTriggerCosts(AstExecGraph* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_هێڵ* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            إذا (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t cost = V3_Instr_ژماردن::ژماردن(ifp, خطأ);
                لكل (ثابت uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = جديد AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  جديد AstRange{flp, static_cast<صحيح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* ثابت consp = جديد AstConsArray{flp, dtypep, جديد AstConst{flp, 0}};
    لكل (uint32_t index = 0; index < nTriggers; ++index) {
        إذا (costs[index]) consp->valuep(index, جديد AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_درەخت(), consp);
}

// Called by Verilator top stage
فراغ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->لووتکە_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    لكل (AstExecGraph* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        إذا (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_درەخت(), execGraphp);
            أرجع;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        إذا (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<مزدوج>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        لكل (ثابت ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

فراغ selfTest() {
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
