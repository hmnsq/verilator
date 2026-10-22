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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3ExecGraph.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3GraphStream.h"
#அந்தர்பாவ "V3Hasher.h"
#அந்தர்பாவ "V3InstrCount.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_முனை_Stmt* stmtsp,
                                 uint32_t id) {
    மாறிலி std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* மாறிலி newp = புதிய AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(உண்மை);
    newp->voidSelf(உண்மை);  // Called through VlExecGraph::Fnp
    newp->dontCombine(உண்மை);
    newp->addStmtsp(stmtsp);
    எனில் (scopep) scopep->addBlocksp(newp);
    திரும்பு newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_முனை_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

வெறுமை ExecMTask::dump(std::ostream& str) மாறிலி {
    str << name() << "." << cvtToHex(இது);
    எனில் (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

நாமாகாஶ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
வகுப்பு ThreadSchedule final {
    friend வகுப்பு PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    நிலையான uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<மாறிலி ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

பொது:
    // CONSTANTS
    நிலையான constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    கட்டமைப்பு MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        மாறிலி ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<மாறிலி ExecMTask*>> m_threads;

    // Global state for each mtask.
    நிலையான std::unordered_map<மாறிலி ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = இயல்பு;
    ThreadSchedule& operator=(ThreadSchedule&&) = இயல்பு;

தனிப்பட்ட:
    VL_UNCOPYABLE(ThreadSchedule);

    நிலையான constexpr இரட்டை s_threadBoxWidth = 2.5;
    நிலையான constexpr இரட்டை s_threadBoxHeight = 1.5;
    நிலையான constexpr இரட்டை s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    நிலையான வெறுமை dumpDotFilePrefixedAlways(மாறிலி std::vector<ThreadSchedule>& schedules,
                                          மாறிலி string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    நிலையான வெறுமை dumpDotFile(மாறிலி std::vector<ThreadSchedule>& schedules, மாறிலி string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        மாறிலி std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        எனில் (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        க்கு (uint32_t i = 0; i < nThreads; ++i) {
            மாறிலி string name = "t" + std::to_string(i);
            மாறிலி string label = "Thread " + std::to_string(i);
            constexpr இரட்டை posX = -s_horizontalGap;
            மாறிலி இரட்டை posY = -static_cast<இரட்டை>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        க்கு (மாறிலி auto& state : ThreadSchedule::s_mtaskState) {
            மாறிலி ExecMTask* மாறிலி mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        மாறிலி uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<இரட்டை> offsets(nThreads, 0.0);
        க்கு (மாறிலி ThreadSchedule& schedule : schedules) {
            எனில் (schedule.mtasks.empty()) தொடர்;
            using Column = std::vector<மாறிலி ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            கட்டமைப்பு Cmp final {
                தருக்க operator()(மாறிலி ExecMTask* மாறிலி a, மாறிலி ExecMTask* மாறிலி b) மாறிலி {
                    எனில் (startTime(a) == startTime(b)) திரும்பு threadId(a) < threadId(b);
                    திரும்பு startTime(a) < startTime(b);
                }
            };
            மாறிலி std::multiset<மாறிலி ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            க்கு (மாறிலி ExecMTask* மாறிலி mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                தருக்க intersects = உண்மை;
                க்கு (மாறிலி ExecMTask* மாறிலி earlierMtask : column) {
                    எனில் (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = பொய்;
                        நிறுத்து;
                    }
                }
                எனில் (intersects) {
                    column.emplace_back(mtaskp);
                } இல்லையெனில் {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            க்கு (மாறிலி Column& column : columns) {
                இரட்டை lastColumnOffset = 0;
                க்கு (மாறிலி ExecMTask* மாறிலி mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            க்கு (மாறிலி std::vector<மாறிலி ExecMTask*>& thread : schedule.m_threads) {
                எனில் (thread.empty()) நிறுத்து;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<முழு>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                க்கு (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                க்கு (மாறிலி ExecMTask* மாறிலி mtaskp : thread) {
                    க்கு (மாறிலி V3GraphEdge& edge : mtaskp->outEdges()) {
                        மாறிலி ExecMTask* மாறிலி உச்சி_MTaskp = edge.உச்சி()->cast<மாறிலி ExecMTask>();
                        எனில் (உச்சி_MTaskp && schedule.contains(உச்சி_MTaskp)
                            && threadId(உச்சி_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << உச்சி_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    நிலையான வெறுமை dumpDotFileEmitBlock(மாறிலி std::unique_ptr<std::ofstream>& logp,
                                     மாறிலி string& name, மாறிலி string& label, இரட்டை width,
                                     இரட்டை height, இரட்டை xPos, இரட்டை yPos,
                                     மாறிலி string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    நிலையான வெறுமை dumpDotFileEmitMTask(மாறிலி std::unique_ptr<std::ofstream>& logp,
                                     மாறிலி ExecMTask* மாறிலி mtaskp, மாறிலி ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<இரட்டை>& offsets) {
        க்கு (முழு i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            மாறிலி string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            மாறிலி string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            மாறிலி இரட்டை width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<இரட்டை>(mtaskp->cost()) / segmentCost);
            மாறிலி uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            மாறிலி இரட்டை xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            மாறிலி இரட்டை yPos = -s_threadBoxHeight * static_cast<இரட்டை>(mtaskThreadId);
            மாறிலி string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    நிலையான வெறுமை dumpDotFileEmitFork(மாறிலி std::unique_ptr<std::ofstream>& logp, இரட்டை offset,
                                    uint32_t nThreads) {
        மாறிலி string& name = "fork_" + std::to_string(static_cast<முழு>(offset));
        constexpr இரட்டை width = s_threadBoxWidth / 8;
        மாறிலி இரட்டை height = s_threadBoxHeight * nThreads;
        மாறிலி இரட்டை xPos = offset - s_horizontalGap / 2;
        மாறிலி இரட்டை yPos
            = -static_cast<இரட்டை>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

பொது:
    நிலையான uint32_t threadId(மாறிலி ExecMTask* mtaskp) {
        மாறிலி auto& it = s_mtaskState.find(mtaskp);
        திரும்பு it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    நிலையான uint32_t startTime(மாறிலி ExecMTask* mtaskp) {
        திரும்பு s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    நிலையான uint32_t endTime(மாறிலி ExecMTask* mtaskp) {
        திரும்பு s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(மாறிலி ExecMTask* mtaskp) மாறிலி {
        மாறிலி uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        க்கு (மாறிலி V3GraphEdge& edge : mtaskp->inEdges()) {
            மாறிலி ExecMTask* மாறிலி prevp = edge.fromp()->as<ExecMTask>();
            எனில் (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        திரும்பு result;
    }

    uint32_t id() மாறிலி { திரும்பு m_id; }
    uint32_t scheduleOn(மாறிலி ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        மாறிலி uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<மாறிலி ExecMTask*>& bestThread = m_threads[bestThreadId];
        எனில் (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.தள்ளு_back(mtaskp);
        திரும்பு bestEndTime;
    }
    தருக்க contains(மாறிலி ExecMTask* mtaskp) மாறிலி { திரும்பு mtasks.மொத்தம்(mtaskp); }
    uint32_t endTime() மாறிலி { திரும்பு m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<மாறிலி ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr இரட்டை V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
வகுப்பு PackThreads final {
    // TYPES
    கட்டமைப்பு MTaskCmp final {
        தருக்க operator()(மாறிலி ExecMTask* ap, மாறிலி ExecMTask* bp) மாறிலி {
            திரும்பு ap->id() < bp->id();
        }
    };

    // MEMBERS
    மாறிலி uint32_t m_nThreads;  // Number of threads
    மாறிலி uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    மாறிலி uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    மாறிலி uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = இயல்பு;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(மாறிலி ThreadSchedule& schedule, மாறிலி ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        எனில் (!schedule.contains(mtaskp)) திரும்பு 0;
        மாறிலி ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        எனில் (threadId == state.threadId) {
            // No overhead on same thread
            திரும்பு state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        எனில் (state.nextp) {
            மாறிலி uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            எனில் ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        திரும்பு sandbaggedEndTime;
    }

    நிலையான தருக்க isReady(ThreadSchedule& schedule, மாறிலி ExecMTask* mtaskp) {
        க்கு (மாறிலி V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            மாறிலி ExecMTask* மாறிலி prevp = edgeIn.fromp()->as<மாறிலி ExecMTask>();
            எனில் (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                திரும்பு பொய்;
            }
        }
        திரும்பு உண்மை;
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
        எண்ணிக்கை வகுப்பு SchedulingMode : uint8_t {
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
        முழு maxThreadWorkers = 1;

        // Build initial ready list
        க்கு (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* மாறிலி mtaskp = vtx.as<ExecMTask>();
            எனில் (isReady(result.back(), mtaskp)) readyMTasks.செருகு(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        போது (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            க்கு (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                க்கு (ExecMTask* மாறிலி mtaskp : readyMTasks) {
                    எனில் (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        தொடர்;
                    }
                    எனில் (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        தொடர்;

                    uint32_t timeBegin = busyUntil[threadId];
                    எனில் (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        நிறுத்து;
                    }
                    க்கு (மாறிலி V3GraphEdge& edge : mtaskp->inEdges()) {
                        மாறிலி ExecMTask* மாறிலி priorp = edge.fromp()->as<ExecMTask>();
                        மாறிலி uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        எனில் (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    எனில் ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            மாறிலி uint32_t endTime = schedule.endTime();

            எனில் (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                மாறிலி uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                எனில் (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                தொடர்;
            }

            எனில் (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                தொடர்;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            மாறிலி uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            மாறிலி size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            க்கு (மாறிலி V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* மாறிலி nextp = edgeOut.உச்சி()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                எனில் (isReady(schedule, nextp)) {
                    readyMTasks.செருகு(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        எனில் (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        திரும்பு result;
    }

பொது:
    // SELF TEST
    நிலையான வெறுமை selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
        AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* மாறிலி scopep = புதிய AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* மாறிலி trigVscp = புதிய AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_மரம்(), trigVscp);
        VL_DO_DANGLING(scopep->delete_மரம்(), scopep);
        VL_DO_DANGLING(varp->delete_மரம்(), varp);
    }
    நிலையான வெறுமை selfTestNormalFirst(AstVarScope* trigVscp) {
        File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
        AstExecGraph* மாறிலி execGraphp = புதிய AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        மாறிலி auto makeBody = [&]() -> Ast_முனை_Stmt* { திரும்பு புதிய AstComment{flp, ""}; };
        ExecMTask* மாறிலி t0 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* மாறிலி t1 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* மாறிலி t2 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* மாறிலி t3 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* மாறிலி t4 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* மாறிலி t5 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* மாறிலி t6 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
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
        புதிய V3GraphEdge{&graph, t0, t1, 1};
        புதிய V3GraphEdge{&graph, t0, t2, 1};
        புதிய V3GraphEdge{&graph, t2, t3, 1};
        புதிய V3GraphEdge{&graph, t2, t4, 1};
        புதிய V3GraphEdge{&graph, t3, t5, 1};
        புதிய V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        மாறிலி std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        க்கு (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        க்கு (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_மரம்();
        VL_DO_DANGLING(execGraphp->delete_மரம்(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    நிலையான வெறுமை selfTestHierFirst(AstVarScope* trigVscp) {
        File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
        AstExecGraph* மாறிலி execGraphp = புதிய AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        மாறிலி auto makeBody = [&]() -> Ast_முனை_Stmt* { திரும்பு புதிய AstComment{flp, ""}; };
        ExecMTask* மாறிலி t0 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* மாறிலி t1 = புதிய ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        புதிய V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        மாறிலி std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        க்கு (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        க்கு (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        க்கு (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_மரம்();
        VL_DO_DANGLING(execGraphp->delete_மரம்(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    நிலையான std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        திரும்பு PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

வெறுமை normalizeCosts(Costs& costs) {
    மாறிலி auto scaleCost = [](uint64_t மதிப்பு, இரட்டை multiplier) {
        இரட்டை scaled = static_cast<இரட்டை>(மதிப்பு) * multiplier;
        எனில் (மதிப்பு && scaled < 1) scaled = 1;
        திரும்பு static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    க்கு (மாறிலி auto& est : costs) {
        எனில் (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    எனில் (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        மாறிலி இரட்டை estToProfile
            = static_cast<இரட்டை>(sumCostProfiled) / static_cast<இரட்டை>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        க்கு (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    க்கு (auto& est : costs) {
        மாறிலி uint64_t& costEstimate = est.second.first;
        மாறிலி uint64_t& costProfiled = est.second.second;
        எனில் (maxCost < costEstimate) maxCost = costEstimate;
        எனில் (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    எனில் (maxCost > scaleDownTo) {
        மாறிலி இரட்டை scaleup = static_cast<இரட்டை>(scaleDownTo) / static_cast<இரட்டை>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        க்கு (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

வெறுமை removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    க்கு (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        எனில் (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_மரம்(), callp);
    }
    V3Graph* மாறிலி execMTaskGraphp = execGraphp->depGraphp();
    க்கு (V3GraphVertex* மாறிலி vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* மாறிலி mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* மாறிலி funcp = mtaskp->funcp();
        எனில் (funcp->stmtsp()) தொடர்;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_மரம்(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

வெறுமை fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    க்கு (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* மாறிலி mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        மாறிலி uint64_t costEstimate = V3_Instr_மொத்தம்::மொத்தம்(mtp->funcp(), பொய்);
        மாறிலி uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        எனில் (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    முழு totalEstimates = 0;
    முழு missingProfiles = 0;
    க்கு (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* மாறிலி mtp = vtx.as<ExecMTask>();
        மாறிலி uint32_t costEstimate = costs[mtp->id()].first;
        மாறிலி uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        மாறிலி uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        எனில் (!costProfiled32) {
            costToUse = costEstimate;
            எனில் (costEstimate != 0) ++missingProfiles;
        }
        எனில் (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    எனில் (missingProfiles) {
        எனில் (File_வரி* மாறிலி fl = V3Control::get_Profile_Data_File_வரி()) {
            எனில் (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

வெறுமை finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    போது (மாறிலி V3GraphVertex* மாறிலி vxp = ser.nextp()) {
        ExecMTask* மாறிலி mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        க்கு (V3GraphEdge& edge : mtp->outEdges()) {
            மாறிலி ExecMTask* மாறிலி followp = edge.உச்சி()->as<ExecMTask>();
            எனில் ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    மாறிலி auto report = execMTaskGraphp->parallelismReport([](மாறிலி V3GraphVertex* vtxp) {  //
        திரும்பு vtxp->as<மாறிலி ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_மொத்தம்());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_மொத்தம்());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    எனில் (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_மொத்தம்());
        UINFO(0, "    Edge count = " << report.edge_மொத்தம்());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(மாறிலி std::string& name, uint32_t nDependencies) {
    Ast_முனை_Module* மாறிலி modp = v3Global.rootp()->உச்சி_Modulep();
    File_வரி* மாறிலி fl = modp->fileline();
    AstBasicDType* மாறிலி dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* மாறிலி varp = புதிய AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(உண்மை);
    varp->valuep(புதிய AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    திரும்பு varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, மாறிலி std::string& method) {
    File_வரி* மாறிலி fl = varp->fileline();
    AstVarRef* மாறிலி refp = புதிய AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    மாறிலி AstScope* மாறிலி உச்சி_Scopep = v3Global.rootp()->உச்சி_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, உச்சி_Scopep->nameDotless()});
    AstCStmt* மாறிலி stmtp = புதிய AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    திரும்பு stmtp;
}

வெறுமை addMTaskToFunction(மாறிலி ThreadSchedule& schedule, மாறிலி uint32_t threadId, AstCFunc* funcp,
                        மாறிலி ExecMTask* mtaskp,
                        மாறிலி std::unordered_map<மாறிலி ExecMTask*, AstVar*>& stateVars) {
    AstScope* மாறிலி scopep = v3Global.rootp()->உச்சி_Scopep()->scopep();
    Ast_முனை_Module* மாறிலி modp = v3Global.rootp()->உச்சி_Modulep();
    File_வரி* மாறிலி fl = modp->fileline();

    // Helper function to make the code a bit more legible
    மாறிலி auto addCStmt = [=](மாறிலி string& stmt) -> வெறுமை {  //
        funcp->addStmtsp(புதிய AstCStmt{fl, stmt});
    };

    எனில் (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        எனில் (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        எனில் (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    எனில் (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* மாறிலி callp = புதிய AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    எனில் (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    க்கு (மாறிலி V3GraphEdge& edge : mtaskp->outEdges()) {
        மாறிலி ExecMTask* மாறிலி nextp = edge.உச்சி()->as<ExecMTask>();
        எனில் (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

மாறிலி std::vector<AstCFunc*> createThreadFunctions(மாறிலி ThreadSchedule& schedule,
                                                   மாறிலி string& tag) {
    Ast_முனை_Module* மாறிலி modp = v3Global.rootp()->உச்சி_Modulep();
    File_வரி* மாறிலி fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<மாறிலி ExecMTask*, AstVar*> stateVars;
    க்கு (மாறிலி std::vector<மாறிலி ExecMTask*>& thread : schedule.m_threads) {
        க்கு (மாறிலி ExecMTask* மாறிலி mtaskp : thread) {
            எனில் (மாறிலி uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    க்கு (மாறிலி std::vector<மாறிலி ExecMTask*>& thread : schedule.m_threads) {
        எனில் (thread.empty()) தொடர்;
        மாறிலி uint32_t threadId = schedule.threadId(thread.front());
        மாறிலி string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.தள்ளு_back(funcp);
        funcp->isStatic(உண்மை);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(உண்மை);
        funcp->entryPoint(உண்மை);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(புதிய AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(புதிய AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        க்கு (மாறிலி ExecMTask* மாறிலி mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    திரும்பு funcps;
}

வெறுமை addThreadStartWrapper(AstExecGraph* மாறிலி execGraphp) {
    // FileLine used for constructing nodes below
    File_வரி* மாறிலி fl = v3Global.rootp()->fileline();
    மாறிலி string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    மாறிலி auto addCStmt = [=](மாறிலி string& stmt) -> வெறுமை {  //
        execGraphp->runfuncp()->addStmtsp(புதிய AstCStmt{fl, stmt});
    };

    எனில் (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    எனில் (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

வெறுமை addThreadEndWrapper(AstExecGraph* மாறிலி execGraphp) {
    // Add thread function invocations to execGraph
    மாறிலி auto addCStmt = [=](மாறிலி string& stmt) -> வெறுமை {  //
        File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(புதிய AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    எனில் (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
வெறுமை addThreadStartToExecGraph(AstExecGraph* மாறிலி execGraphp,
                               மாறிலி std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_வரி* மாறிலி fl = v3Global.rootp()->fileline();
    மாறிலி string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* மாறிலி finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    க்கு (AstCFunc* மாறிலி funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    மாறிலி auto addCStmt = [=](மாறிலி string& stmt) -> வெறுமை {  //
        execGraphp->runfuncp()->addStmtsp(புதிய AstCStmt{fl, stmt});
    };

    மாறிலி uint32_t last = funcps.size() - 1;
    எனில் (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    க்கு (AstCFunc* மாறிலி funcp : funcps) {
        எனில் (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            எனில் (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } இல்லையெனில் {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(புதிய AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } இல்லையெனில் {
            // The last will run on the main thread.
            AstCCall* மாறிலி callp = புதிய AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    எனில் (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    எனில் (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    எனில் (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

வெறுமை processMTaskBodies(AstExecGraph* மாறிலி execGraphp) {
    க்கு (V3GraphVertex* மாறிலி vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* மாறிலி mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* மாறிலி funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_முனை* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        மாறிலி auto addCStmt = [=](மாறிலி string& stmt) -> வெறுமை {  //
            funcp->addStmtsp(புதிய AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        எனில் (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            எனில் (v3Global.opt.hierChild()) args += v3Global.opt.உச்சி_Module();
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
        எனில் (v3Global.opt.profExec()) {
            மாறிலி std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

வெறுமை implementExecGraph(AstExecGraph* மாறிலி execGraphp, மாறிலி ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    எனில் (execGraphp->depGraphp()->empty()) திரும்பு;

    // Create a function to be run by each thread.
    மாறிலி std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

வெறுமை createRunFunction(AstExecGraph* மாறிலி execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_வரி* மாறிலி flp = execGraphp->fileline();
    Ast_முனை_Module* மாறிலி modp = v3Global.rootp()->உச்சி_Modulep();
    AstCFunc* மாறிலி funcp = புதிய AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(உண்மை);
    funcp->voidSelf(உண்மை);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(உண்மை);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

வெறுமை addTriggerMasks(AstExecGraph* மாறிலி execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_வரி* மாறிலி flp = execGraphp->fileline();
    AstVar* மாறிலி triggersp = execGraphp->triggerp()->varp();
    க்கு (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](மாறிலி AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            க்கு (மாறிலி uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* மாறிலி consp = புதிய AstConsArray{flp, triggersp->dtypep(),
                                                     புதிய AstConst{flp, AstConst::Unsized64{}, 0}};
        க்கு (மாறிலி auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          புதிய AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_மரம்(), consp);
    }
}

வெறுமை addTriggerCosts(AstExecGraph* மாறிலி execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_வரி* மாறிலி flp = execGraphp->fileline();
    மாறிலி uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    க்கு (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            எனில் (மாறிலி AstTriggered* மாறிலி trigp = VN_CAST(ifp->condp(), Triggered)) {
                மாறிலி uint32_t cost = V3_Instr_மொத்தம்::மொத்தம்(ifp, பொய்);
                க்கு (மாறிலி uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    AstUnpackArrayDType* மாறிலி dtypep
        = புதிய AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  புதிய AstRange{flp, static_cast<முழு>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* மாறிலி consp = புதிய AstConsArray{flp, dtypep, புதிய AstConst{flp, 0}};
    க்கு (uint32_t index = 0; index < nTriggers; ++index) {
        எனில் (costs[index]) consp->valuep(index, புதிய AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_மரம்(), consp);
}

// Called by Verilator top stage
வெறுமை implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->உச்சி_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    க்கு (AstExecGraph* மாறிலி execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        எனில் (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_மரம்(), execGraphp);
            திரும்பு;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        எனில் (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        மாறிலி std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<இரட்டை>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        க்கு (மாறிலி ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

வெறுமை selfTest() {
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
