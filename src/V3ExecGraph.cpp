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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3ExecGraph.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3GraphStream.h"
#અન્તર્ભાવ "V3Hasher.h"
#અન્તર્ભાવ "V3InstrCount.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ગાંઠ_Stmt* stmtsp,
                                 uint32_t id) {
    અચલ std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* અચલ newp = નવું AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(સાચું);
    newp->voidSelf(સાચું);  // Called through VlExecGraph::Fnp
    newp->dontCombine(સાચું);
    newp->addStmtsp(stmtsp);
    જો (scopep) scopep->addBlocksp(newp);
    પાછા newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_ગાંઠ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

શૂન્ય ExecMTask::dump(std::ostream& str) અચલ {
    str << name() << "." << cvtToHex(આ);
    જો (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

નામાકાશ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
વર્ગ ThreadSchedule final {
    friend વર્ગ PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    સ્થિર uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<અચલ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

જાહેર:
    // CONSTANTS
    સ્થિર constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    સંરચના MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        અચલ ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<અચલ ExecMTask*>> m_threads;

    // Global state for each mtask.
    સ્થિર std::unordered_map<અચલ ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = મૂળભૂત;
    ThreadSchedule& operator=(ThreadSchedule&&) = મૂળભૂત;

ખાનગી:
    VL_UNCOPYABLE(ThreadSchedule);

    સ્થિર constexpr દ્વિગુણ s_threadBoxWidth = 2.5;
    સ્થિર constexpr દ્વિગુણ s_threadBoxHeight = 1.5;
    સ્થિર constexpr દ્વિગુણ s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    સ્થિર શૂન્ય dumpDotFilePrefixedAlways(અચલ std::vector<ThreadSchedule>& schedules,
                                          અચલ string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    સ્થિર શૂન્ય dumpDotFile(અચલ std::vector<ThreadSchedule>& schedules, અચલ string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        અચલ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        જો (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        માટે (uint32_t i = 0; i < nThreads; ++i) {
            અચલ string name = "t" + std::to_string(i);
            અચલ string label = "Thread " + std::to_string(i);
            constexpr દ્વિગુણ posX = -s_horizontalGap;
            અચલ દ્વિગુણ posY = -static_cast<દ્વિગુણ>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        માટે (અચલ auto& state : ThreadSchedule::s_mtaskState) {
            અચલ ExecMTask* અચલ mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        અચલ uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<દ્વિગુણ> offsets(nThreads, 0.0);
        માટે (અચલ ThreadSchedule& schedule : schedules) {
            જો (schedule.mtasks.empty()) ચાલુ;
            using Column = std::vector<અચલ ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            સંરચના Cmp final {
                તાર્કિક operator()(અચલ ExecMTask* અચલ a, અચલ ExecMTask* અચલ b) અચલ {
                    જો (startTime(a) == startTime(b)) પાછા threadId(a) < threadId(b);
                    પાછા startTime(a) < startTime(b);
                }
            };
            અચલ std::multiset<અચલ ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            માટે (અચલ ExecMTask* અચલ mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                તાર્કિક intersects = સાચું;
                માટે (અચલ ExecMTask* અચલ earlierMtask : column) {
                    જો (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = ખોટું;
                        તોડો;
                    }
                }
                જો (intersects) {
                    column.emplace_back(mtaskp);
                } નહીંતર {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            માટે (અચલ Column& column : columns) {
                દ્વિગુણ lastColumnOffset = 0;
                માટે (અચલ ExecMTask* અચલ mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            માટે (અચલ std::vector<અચલ ExecMTask*>& thread : schedule.m_threads) {
                જો (thread.empty()) તોડો;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<પૂર્ણાંક>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                માટે (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                માટે (અચલ ExecMTask* અચલ mtaskp : thread) {
                    માટે (અચલ V3GraphEdge& edge : mtaskp->outEdges()) {
                        અચલ ExecMTask* અચલ ટોચ_MTaskp = edge.ટોચ()->cast<અચલ ExecMTask>();
                        જો (ટોચ_MTaskp && schedule.contains(ટોચ_MTaskp)
                            && threadId(ટોચ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << ટોચ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    સ્થિર શૂન્ય dumpDotFileEmitBlock(અચલ std::unique_ptr<std::ofstream>& logp,
                                     અચલ string& name, અચલ string& label, દ્વિગુણ width,
                                     દ્વિગુણ height, દ્વિગુણ xPos, દ્વિગુણ yPos,
                                     અચલ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    સ્થિર શૂન્ય dumpDotFileEmitMTask(અચલ std::unique_ptr<std::ofstream>& logp,
                                     અચલ ExecMTask* અચલ mtaskp, અચલ ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<દ્વિગુણ>& offsets) {
        માટે (પૂર્ણાંક i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            અચલ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            અચલ string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            અચલ દ્વિગુણ width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<દ્વિગુણ>(mtaskp->cost()) / segmentCost);
            અચલ uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            અચલ દ્વિગુણ xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            અચલ દ્વિગુણ yPos = -s_threadBoxHeight * static_cast<દ્વિગુણ>(mtaskThreadId);
            અચલ string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    સ્થિર શૂન્ય dumpDotFileEmitFork(અચલ std::unique_ptr<std::ofstream>& logp, દ્વિગુણ offset,
                                    uint32_t nThreads) {
        અચલ string& name = "fork_" + std::to_string(static_cast<પૂર્ણાંક>(offset));
        constexpr દ્વિગુણ width = s_threadBoxWidth / 8;
        અચલ દ્વિગુણ height = s_threadBoxHeight * nThreads;
        અચલ દ્વિગુણ xPos = offset - s_horizontalGap / 2;
        અચલ દ્વિગુણ yPos
            = -static_cast<દ્વિગુણ>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

જાહેર:
    સ્થિર uint32_t threadId(અચલ ExecMTask* mtaskp) {
        અચલ auto& it = s_mtaskState.find(mtaskp);
        પાછા it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    સ્થિર uint32_t startTime(અચલ ExecMTask* mtaskp) {
        પાછા s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    સ્થિર uint32_t endTime(અચલ ExecMTask* mtaskp) {
        પાછા s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(અચલ ExecMTask* mtaskp) અચલ {
        અચલ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        માટે (અચલ V3GraphEdge& edge : mtaskp->inEdges()) {
            અચલ ExecMTask* અચલ prevp = edge.fromp()->as<ExecMTask>();
            જો (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        પાછા result;
    }

    uint32_t id() અચલ { પાછા m_id; }
    uint32_t scheduleOn(અચલ ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        અચલ uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<અચલ ExecMTask*>& bestThread = m_threads[bestThreadId];
        જો (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.ધકેલો_back(mtaskp);
        પાછા bestEndTime;
    }
    તાર્કિક contains(અચલ ExecMTask* mtaskp) અચલ { પાછા mtasks.ગણતરી(mtaskp); }
    uint32_t endTime() અચલ { પાછા m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<અચલ ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr દ્વિગુણ V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
વર્ગ PackThreads final {
    // TYPES
    સંરચના MTaskCmp final {
        તાર્કિક operator()(અચલ ExecMTask* ap, અચલ ExecMTask* bp) અચલ {
            પાછા ap->id() < bp->id();
        }
    };

    // MEMBERS
    અચલ uint32_t m_nThreads;  // Number of threads
    અચલ uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    અચલ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    અચલ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = મૂળભૂત;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(અચલ ThreadSchedule& schedule, અચલ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        જો (!schedule.contains(mtaskp)) પાછા 0;
        અચલ ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        જો (threadId == state.threadId) {
            // No overhead on same thread
            પાછા state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        જો (state.nextp) {
            અચલ uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            જો ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        પાછા sandbaggedEndTime;
    }

    સ્થિર તાર્કિક isReady(ThreadSchedule& schedule, અચલ ExecMTask* mtaskp) {
        માટે (અચલ V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            અચલ ExecMTask* અચલ prevp = edgeIn.fromp()->as<અચલ ExecMTask>();
            જો (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                પાછા ખોટું;
            }
        }
        પાછા સાચું;
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
        ગણના વર્ગ SchedulingMode : uint8_t {
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
        પૂર્ણાંક maxThreadWorkers = 1;

        // Build initial ready list
        માટે (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* અચલ mtaskp = vtx.as<ExecMTask>();
            જો (isReady(result.back(), mtaskp)) readyMTasks.ઉમેરો(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        જ્યારે (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            માટે (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                માટે (ExecMTask* અચલ mtaskp : readyMTasks) {
                    જો (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        ચાલુ;
                    }
                    જો (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        ચાલુ;

                    uint32_t timeBegin = busyUntil[threadId];
                    જો (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        તોડો;
                    }
                    માટે (અચલ V3GraphEdge& edge : mtaskp->inEdges()) {
                        અચલ ExecMTask* અચલ priorp = edge.fromp()->as<ExecMTask>();
                        અચલ uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        જો (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    જો ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            અચલ uint32_t endTime = schedule.endTime();

            જો (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                અચલ uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                જો (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ચાલુ;
            }

            જો (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ચાલુ;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            અચલ uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            અચલ size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            માટે (અચલ V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* અચલ nextp = edgeOut.ટોચ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                જો (isReady(schedule, nextp)) {
                    readyMTasks.ઉમેરો(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        જો (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        પાછા result;
    }

જાહેર:
    // SELF TEST
    સ્થિર શૂન્ય selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_લીટી* અચલ flp = v3Global.rootp()->fileline();
        AstVar* અચલ varp = નવું AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* અચલ scopep = નવું AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* અચલ trigVscp = નવું AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_વૃક્ષ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_વૃક્ષ(), scopep);
        VL_DO_DANGLING(varp->delete_વૃક્ષ(), varp);
    }
    સ્થિર શૂન્ય selfTestNormalFirst(AstVarScope* trigVscp) {
        File_લીટી* અચલ flp = v3Global.rootp()->fileline();
        AstExecGraph* અચલ execGraphp = નવું AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        અચલ auto makeBody = [&]() -> Ast_ગાંઠ_Stmt* { પાછા નવું AstComment{flp, ""}; };
        ExecMTask* અચલ t0 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* અચલ t1 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* અચલ t2 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* અચલ t3 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* અચલ t4 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* અચલ t5 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* અચલ t6 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
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
        નવું V3GraphEdge{&graph, t0, t1, 1};
        નવું V3GraphEdge{&graph, t0, t2, 1};
        નવું V3GraphEdge{&graph, t2, t3, 1};
        નવું V3GraphEdge{&graph, t2, t4, 1};
        નવું V3GraphEdge{&graph, t3, t5, 1};
        નવું V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        અચલ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        માટે (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        માટે (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_વૃક્ષ();
        VL_DO_DANGLING(execGraphp->delete_વૃક્ષ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    સ્થિર શૂન્ય selfTestHierFirst(AstVarScope* trigVscp) {
        File_લીટી* અચલ flp = v3Global.rootp()->fileline();
        AstExecGraph* અચલ execGraphp = નવું AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        અચલ auto makeBody = [&]() -> Ast_ગાંઠ_Stmt* { પાછા નવું AstComment{flp, ""}; };
        ExecMTask* અચલ t0 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* અચલ t1 = નવું ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        નવું V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        અચલ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        માટે (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        માટે (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        માટે (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_વૃક્ષ();
        VL_DO_DANGLING(execGraphp->delete_વૃક્ષ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    સ્થિર std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        પાછા PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

શૂન્ય normalizeCosts(Costs& costs) {
    અચલ auto scaleCost = [](uint64_t કિંમત, દ્વિગુણ multiplier) {
        દ્વિગુણ scaled = static_cast<દ્વિગુણ>(કિંમત) * multiplier;
        જો (કિંમત && scaled < 1) scaled = 1;
        પાછા static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    માટે (અચલ auto& est : costs) {
        જો (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    જો (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        અચલ દ્વિગુણ estToProfile
            = static_cast<દ્વિગુણ>(sumCostProfiled) / static_cast<દ્વિગુણ>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        માટે (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    માટે (auto& est : costs) {
        અચલ uint64_t& costEstimate = est.second.first;
        અચલ uint64_t& costProfiled = est.second.second;
        જો (maxCost < costEstimate) maxCost = costEstimate;
        જો (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    જો (maxCost > scaleDownTo) {
        અચલ દ્વિગુણ scaleup = static_cast<દ્વિગુણ>(scaleDownTo) / static_cast<દ્વિગુણ>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        માટે (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

શૂન્ય removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    માટે (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        જો (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_વૃક્ષ(), callp);
    }
    V3Graph* અચલ execMTaskGraphp = execGraphp->depGraphp();
    માટે (V3GraphVertex* અચલ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* અચલ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* અચલ funcp = mtaskp->funcp();
        જો (funcp->stmtsp()) ચાલુ;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_વૃક્ષ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

શૂન્ય fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    માટે (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* અચલ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        અચલ uint64_t costEstimate = V3_Instr_ગણતરી::ગણતરી(mtp->funcp(), ખોટું);
        અચલ uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        જો (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    પૂર્ણાંક totalEstimates = 0;
    પૂર્ણાંક missingProfiles = 0;
    માટે (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* અચલ mtp = vtx.as<ExecMTask>();
        અચલ uint32_t costEstimate = costs[mtp->id()].first;
        અચલ uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        અચલ uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        જો (!costProfiled32) {
            costToUse = costEstimate;
            જો (costEstimate != 0) ++missingProfiles;
        }
        જો (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    જો (missingProfiles) {
        જો (File_લીટી* અચલ fl = V3Control::get_Profile_Data_File_લીટી()) {
            જો (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

શૂન્ય finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    જ્યારે (અચલ V3GraphVertex* અચલ vxp = ser.nextp()) {
        ExecMTask* અચલ mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        માટે (V3GraphEdge& edge : mtp->outEdges()) {
            અચલ ExecMTask* અચલ followp = edge.ટોચ()->as<ExecMTask>();
            જો ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    અચલ auto report = execMTaskGraphp->parallelismReport([](અચલ V3GraphVertex* vtxp) {  //
        પાછા vtxp->as<અચલ ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_ગણતરી());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_ગણતરી());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    જો (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_ગણતરી());
        UINFO(0, "    Edge count = " << report.edge_ગણતરી());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(અચલ std::string& name, uint32_t nDependencies) {
    Ast_ગાંઠ_Module* અચલ modp = v3Global.rootp()->ટોચ_Modulep();
    File_લીટી* અચલ fl = modp->fileline();
    AstBasicDType* અચલ dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* અચલ varp = નવું AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(સાચું);
    varp->valuep(નવું AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    પાછા varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, અચલ std::string& method) {
    File_લીટી* અચલ fl = varp->fileline();
    AstVarRef* અચલ refp = નવું AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    અચલ AstScope* અચલ ટોચ_Scopep = v3Global.rootp()->ટોચ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, ટોચ_Scopep->nameDotless()});
    AstCStmt* અચલ stmtp = નવું AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    પાછા stmtp;
}

શૂન્ય addMTaskToFunction(અચલ ThreadSchedule& schedule, અચલ uint32_t threadId, AstCFunc* funcp,
                        અચલ ExecMTask* mtaskp,
                        અચલ std::unordered_map<અચલ ExecMTask*, AstVar*>& stateVars) {
    AstScope* અચલ scopep = v3Global.rootp()->ટોચ_Scopep()->scopep();
    Ast_ગાંઠ_Module* અચલ modp = v3Global.rootp()->ટોચ_Modulep();
    File_લીટી* અચલ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    અચલ auto addCStmt = [=](અચલ string& stmt) -> શૂન્ય {  //
        funcp->addStmtsp(નવું AstCStmt{fl, stmt});
    };

    જો (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        જો (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        જો (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    જો (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* અચલ callp = નવું AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    જો (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    માટે (અચલ V3GraphEdge& edge : mtaskp->outEdges()) {
        અચલ ExecMTask* અચલ nextp = edge.ટોચ()->as<ExecMTask>();
        જો (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

અચલ std::vector<AstCFunc*> createThreadFunctions(અચલ ThreadSchedule& schedule,
                                                   અચલ string& tag) {
    Ast_ગાંઠ_Module* અચલ modp = v3Global.rootp()->ટોચ_Modulep();
    File_લીટી* અચલ fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<અચલ ExecMTask*, AstVar*> stateVars;
    માટે (અચલ std::vector<અચલ ExecMTask*>& thread : schedule.m_threads) {
        માટે (અચલ ExecMTask* અચલ mtaskp : thread) {
            જો (અચલ uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    માટે (અચલ std::vector<અચલ ExecMTask*>& thread : schedule.m_threads) {
        જો (thread.empty()) ચાલુ;
        અચલ uint32_t threadId = schedule.threadId(thread.front());
        અચલ string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* અચલ funcp = નવું AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.ધકેલો_back(funcp);
        funcp->isStatic(સાચું);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(સાચું);
        funcp->entryPoint(સાચું);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(નવું AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(નવું AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        માટે (અચલ ExecMTask* અચલ mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    પાછા funcps;
}

શૂન્ય addThreadStartWrapper(AstExecGraph* અચલ execGraphp) {
    // FileLine used for constructing nodes below
    File_લીટી* અચલ fl = v3Global.rootp()->fileline();
    અચલ string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    અચલ auto addCStmt = [=](અચલ string& stmt) -> શૂન્ય {  //
        execGraphp->runfuncp()->addStmtsp(નવું AstCStmt{fl, stmt});
    };

    જો (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    જો (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

શૂન્ય addThreadEndWrapper(AstExecGraph* અચલ execGraphp) {
    // Add thread function invocations to execGraph
    અચલ auto addCStmt = [=](અચલ string& stmt) -> શૂન્ય {  //
        File_લીટી* અચલ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(નવું AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    જો (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
શૂન્ય addThreadStartToExecGraph(AstExecGraph* અચલ execGraphp,
                               અચલ std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_લીટી* અચલ fl = v3Global.rootp()->fileline();
    અચલ string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* અચલ finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    માટે (AstCFunc* અચલ funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    અચલ auto addCStmt = [=](અચલ string& stmt) -> શૂન્ય {  //
        execGraphp->runfuncp()->addStmtsp(નવું AstCStmt{fl, stmt});
    };

    અચલ uint32_t last = funcps.size() - 1;
    જો (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    માટે (AstCFunc* અચલ funcp : funcps) {
        જો (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* અચલ cstmtp = નવું AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            જો (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } નહીંતર {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(નવું AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } નહીંતર {
            // The last will run on the main thread.
            AstCCall* અચલ callp = નવું AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    જો (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    જો (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    જો (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

શૂન્ય processMTaskBodies(AstExecGraph* અચલ execGraphp) {
    માટે (V3GraphVertex* અચલ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* અચલ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* અચલ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ગાંઠ* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        અચલ auto addCStmt = [=](અચલ string& stmt) -> શૂન્ય {  //
            funcp->addStmtsp(નવું AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        જો (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            જો (v3Global.opt.hierChild()) args += v3Global.opt.ટોચ_Module();
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
        જો (v3Global.opt.profExec()) {
            અચલ std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

શૂન્ય implementExecGraph(AstExecGraph* અચલ execGraphp, અચલ ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    જો (execGraphp->depGraphp()->empty()) પાછા;

    // Create a function to be run by each thread.
    અચલ std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

શૂન્ય createRunFunction(AstExecGraph* અચલ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_લીટી* અચલ flp = execGraphp->fileline();
    Ast_ગાંઠ_Module* અચલ modp = v3Global.rootp()->ટોચ_Modulep();
    AstCFunc* અચલ funcp = નવું AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(સાચું);
    funcp->voidSelf(સાચું);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(સાચું);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

શૂન્ય addTriggerMasks(AstExecGraph* અચલ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_લીટી* અચલ flp = execGraphp->fileline();
    AstVar* અચલ triggersp = execGraphp->triggerp()->varp();
    માટે (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](અચલ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            માટે (અચલ uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* અચલ consp = નવું AstConsArray{flp, triggersp->dtypep(),
                                                     નવું AstConst{flp, AstConst::Unsized64{}, 0}};
        માટે (અચલ auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          નવું AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_વૃક્ષ(), consp);
    }
}

શૂન્ય addTriggerCosts(AstExecGraph* અચલ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_લીટી* અચલ flp = execGraphp->fileline();
    અચલ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    માટે (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            જો (અચલ AstTriggered* અચલ trigp = VN_CAST(ifp->condp(), Triggered)) {
                અચલ uint32_t cost = V3_Instr_ગણતરી::ગણતરી(ifp, ખોટું);
                માટે (અચલ uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* અચલ netlistp = v3Global.rootp();
    AstUnpackArrayDType* અચલ dtypep
        = નવું AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  નવું AstRange{flp, static_cast<પૂર્ણાંક>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* અચલ consp = નવું AstConsArray{flp, dtypep, નવું AstConst{flp, 0}};
    માટે (uint32_t index = 0; index < nTriggers; ++index) {
        જો (costs[index]) consp->valuep(index, નવું AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_વૃક્ષ(), consp);
}

// Called by Verilator top stage
શૂન્ય implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->ટોચ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    માટે (AstExecGraph* અચલ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        જો (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_વૃક્ષ(), execGraphp);
            પાછા;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        જો (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        અચલ std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<દ્વિગુણ>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        માટે (અચલ ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

શૂન્ય selfTest() {
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
