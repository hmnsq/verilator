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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3ExecGraph.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3GraphStream.h"
#അന്തര്ഭാവ "V3Hasher.h"
#അന്തര്ഭാവ "V3InstrCount.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_കണ്ണി_Stmt* stmtsp,
                                 uint32_t id) {
    സ്ഥിരം std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* സ്ഥിരം newp = പുതിയ AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(സത്യം);
    newp->voidSelf(സത്യം);  // Called through VlExecGraph::Fnp
    newp->dontCombine(സത്യം);
    newp->addStmtsp(stmtsp);
    എങ്കിൽ (scopep) scopep->addBlocksp(newp);
    തിരികെ newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_കണ്ണി_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

ശൂന്യം ExecMTask::dump(std::ostream& str) സ്ഥിരം {
    str << name() << "." << cvtToHex(ഇത്);
    എങ്കിൽ (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

നാമാകാശ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
വർഗ്ഗം ThreadSchedule final {
    friend വർഗ്ഗം PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    സ്ഥിതം uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<സ്ഥിരം ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

പൊതു:
    // CONSTANTS
    സ്ഥിതം constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ഘടന MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        സ്ഥിരം ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<സ്ഥിരം ExecMTask*>> m_threads;

    // Global state for each mtask.
    സ്ഥിതം std::unordered_map<സ്ഥിരം ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = സ്ഥിരസ്ഥിതി;
    ThreadSchedule& operator=(ThreadSchedule&&) = സ്ഥിരസ്ഥിതി;

സ്വകാര്യ:
    VL_UNCOPYABLE(ThreadSchedule);

    സ്ഥിതം constexpr ഇരട്ട s_threadBoxWidth = 2.5;
    സ്ഥിതം constexpr ഇരട്ട s_threadBoxHeight = 1.5;
    സ്ഥിതം constexpr ഇരട്ട s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    സ്ഥിതം ശൂന്യം dumpDotFilePrefixedAlways(സ്ഥിരം std::vector<ThreadSchedule>& schedules,
                                          സ്ഥിരം string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    സ്ഥിതം ശൂന്യം dumpDotFile(സ്ഥിരം std::vector<ThreadSchedule>& schedules, സ്ഥിരം string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        സ്ഥിരം std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        എങ്കിൽ (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        വേണ്ടി (uint32_t i = 0; i < nThreads; ++i) {
            സ്ഥിരം string name = "t" + std::to_string(i);
            സ്ഥിരം string label = "Thread " + std::to_string(i);
            constexpr ഇരട്ട posX = -s_horizontalGap;
            സ്ഥിരം ഇരട്ട posY = -static_cast<ഇരട്ട>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        വേണ്ടി (സ്ഥിരം auto& state : ThreadSchedule::s_mtaskState) {
            സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        സ്ഥിരം uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<ഇരട്ട> offsets(nThreads, 0.0);
        വേണ്ടി (സ്ഥിരം ThreadSchedule& schedule : schedules) {
            എങ്കിൽ (schedule.mtasks.empty()) തുടരുക;
            using Column = std::vector<സ്ഥിരം ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            ഘടന Cmp final {
                തർക്കം operator()(സ്ഥിരം ExecMTask* സ്ഥിരം a, സ്ഥിരം ExecMTask* സ്ഥിരം b) സ്ഥിരം {
                    എങ്കിൽ (startTime(a) == startTime(b)) തിരികെ threadId(a) < threadId(b);
                    തിരികെ startTime(a) < startTime(b);
                }
            };
            സ്ഥിരം std::multiset<സ്ഥിരം ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                തർക്കം intersects = സത്യം;
                വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം earlierMtask : column) {
                    എങ്കിൽ (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = അസത്യം;
                        നിർത്തുക;
                    }
                }
                എങ്കിൽ (intersects) {
                    column.emplace_back(mtaskp);
                } അല്ലെങ്കിൽ {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            വേണ്ടി (സ്ഥിരം Column& column : columns) {
                ഇരട്ട lastColumnOffset = 0;
                വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            വേണ്ടി (സ്ഥിരം std::vector<സ്ഥിരം ExecMTask*>& thread : schedule.m_threads) {
                എങ്കിൽ (thread.empty()) നിർത്തുക;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<പൂർണ്ണസംഖ്യ>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                വേണ്ടി (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp : thread) {
                    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : mtaskp->outEdges()) {
                        സ്ഥിരം ExecMTask* സ്ഥിരം മുകൾ_MTaskp = edge.മുകൾ()->cast<സ്ഥിരം ExecMTask>();
                        എങ്കിൽ (മുകൾ_MTaskp && schedule.contains(മുകൾ_MTaskp)
                            && threadId(മുകൾ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << മുകൾ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    സ്ഥിതം ശൂന്യം dumpDotFileEmitBlock(സ്ഥിരം std::unique_ptr<std::ofstream>& logp,
                                     സ്ഥിരം string& name, സ്ഥിരം string& label, ഇരട്ട width,
                                     ഇരട്ട height, ഇരട്ട xPos, ഇരട്ട yPos,
                                     സ്ഥിരം string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    സ്ഥിതം ശൂന്യം dumpDotFileEmitMTask(സ്ഥിരം std::unique_ptr<std::ofstream>& logp,
                                     സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp, സ്ഥിരം ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<ഇരട്ട>& offsets) {
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            സ്ഥിരം string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            സ്ഥിരം string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            സ്ഥിരം ഇരട്ട width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<ഇരട്ട>(mtaskp->cost()) / segmentCost);
            സ്ഥിരം uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            സ്ഥിരം ഇരട്ട xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            സ്ഥിരം ഇരട്ട yPos = -s_threadBoxHeight * static_cast<ഇരട്ട>(mtaskThreadId);
            സ്ഥിരം string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    സ്ഥിതം ശൂന്യം dumpDotFileEmitFork(സ്ഥിരം std::unique_ptr<std::ofstream>& logp, ഇരട്ട offset,
                                    uint32_t nThreads) {
        സ്ഥിരം string& name = "fork_" + std::to_string(static_cast<പൂർണ്ണസംഖ്യ>(offset));
        constexpr ഇരട്ട width = s_threadBoxWidth / 8;
        സ്ഥിരം ഇരട്ട height = s_threadBoxHeight * nThreads;
        സ്ഥിരം ഇരട്ട xPos = offset - s_horizontalGap / 2;
        സ്ഥിരം ഇരട്ട yPos
            = -static_cast<ഇരട്ട>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

പൊതു:
    സ്ഥിതം uint32_t threadId(സ്ഥിരം ExecMTask* mtaskp) {
        സ്ഥിരം auto& it = s_mtaskState.find(mtaskp);
        തിരികെ it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    സ്ഥിതം uint32_t startTime(സ്ഥിരം ExecMTask* mtaskp) {
        തിരികെ s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    സ്ഥിതം uint32_t endTime(സ്ഥിരം ExecMTask* mtaskp) {
        തിരികെ s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(സ്ഥിരം ExecMTask* mtaskp) സ്ഥിരം {
        സ്ഥിരം uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : mtaskp->inEdges()) {
            സ്ഥിരം ExecMTask* സ്ഥിരം prevp = edge.fromp()->as<ExecMTask>();
            എങ്കിൽ (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        തിരികെ result;
    }

    uint32_t id() സ്ഥിരം { തിരികെ m_id; }
    uint32_t scheduleOn(സ്ഥിരം ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        സ്ഥിരം uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<സ്ഥിരം ExecMTask*>& bestThread = m_threads[bestThreadId];
        എങ്കിൽ (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.തള്ളുക_back(mtaskp);
        തിരികെ bestEndTime;
    }
    തർക്കം contains(സ്ഥിരം ExecMTask* mtaskp) സ്ഥിരം { തിരികെ mtasks.എണ്ണം(mtaskp); }
    uint32_t endTime() സ്ഥിരം { തിരികെ m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<സ്ഥിരം ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr ഇരട്ട V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
വർഗ്ഗം PackThreads final {
    // TYPES
    ഘടന MTaskCmp final {
        തർക്കം operator()(സ്ഥിരം ExecMTask* ap, സ്ഥിരം ExecMTask* bp) സ്ഥിരം {
            തിരികെ ap->id() < bp->id();
        }
    };

    // MEMBERS
    സ്ഥിരം uint32_t m_nThreads;  // Number of threads
    സ്ഥിരം uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    സ്ഥിരം uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    സ്ഥിരം uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(സ്ഥിരം ThreadSchedule& schedule, സ്ഥിരം ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        എങ്കിൽ (!schedule.contains(mtaskp)) തിരികെ 0;
        സ്ഥിരം ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        എങ്കിൽ (threadId == state.threadId) {
            // No overhead on same thread
            തിരികെ state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        എങ്കിൽ (state.nextp) {
            സ്ഥിരം uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            എങ്കിൽ ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        തിരികെ sandbaggedEndTime;
    }

    സ്ഥിതം തർക്കം isReady(ThreadSchedule& schedule, സ്ഥിരം ExecMTask* mtaskp) {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            സ്ഥിരം ExecMTask* സ്ഥിരം prevp = edgeIn.fromp()->as<സ്ഥിരം ExecMTask>();
            എങ്കിൽ (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                തിരികെ അസത്യം;
            }
        }
        തിരികെ സത്യം;
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
        എണ്ണൽ വർഗ്ഗം SchedulingMode : uint8_t {
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
        പൂർണ്ണസംഖ്യ maxThreadWorkers = 1;

        // Build initial ready list
        വേണ്ടി (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* സ്ഥിരം mtaskp = vtx.as<ExecMTask>();
            എങ്കിൽ (isReady(result.back(), mtaskp)) readyMTasks.തിരുകുക(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        ആകുമ്പോൾ (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            വേണ്ടി (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                വേണ്ടി (ExecMTask* സ്ഥിരം mtaskp : readyMTasks) {
                    എങ്കിൽ (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        തുടരുക;
                    }
                    എങ്കിൽ (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        തുടരുക;

                    uint32_t timeBegin = busyUntil[threadId];
                    എങ്കിൽ (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        നിർത്തുക;
                    }
                    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : mtaskp->inEdges()) {
                        സ്ഥിരം ExecMTask* സ്ഥിരം priorp = edge.fromp()->as<ExecMTask>();
                        സ്ഥിരം uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        എങ്കിൽ (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    എങ്കിൽ ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            സ്ഥിരം uint32_t endTime = schedule.endTime();

            എങ്കിൽ (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                സ്ഥിരം uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                എങ്കിൽ (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                തുടരുക;
            }

            എങ്കിൽ (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                തുടരുക;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            സ്ഥിരം uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            സ്ഥിരം size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* സ്ഥിരം nextp = edgeOut.മുകൾ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                എങ്കിൽ (isReady(schedule, nextp)) {
                    readyMTasks.തിരുകുക(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        എങ്കിൽ (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        തിരികെ result;
    }

പൊതു:
    // SELF TEST
    സ്ഥിതം ശൂന്യം selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* സ്ഥിരം scopep = പുതിയ AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* സ്ഥിരം trigVscp = പുതിയ AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_വൃക്ഷം(), trigVscp);
        VL_DO_DANGLING(scopep->delete_വൃക്ഷം(), scopep);
        VL_DO_DANGLING(varp->delete_വൃക്ഷം(), varp);
    }
    സ്ഥിതം ശൂന്യം selfTestNormalFirst(AstVarScope* trigVscp) {
        File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
        AstExecGraph* സ്ഥിരം execGraphp = പുതിയ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        സ്ഥിരം auto makeBody = [&]() -> Ast_കണ്ണി_Stmt* { തിരികെ പുതിയ AstComment{flp, ""}; };
        ExecMTask* സ്ഥിരം t0 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* സ്ഥിരം t1 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* സ്ഥിരം t2 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* സ്ഥിരം t3 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* സ്ഥിരം t4 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* സ്ഥിരം t5 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* സ്ഥിരം t6 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
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
        പുതിയ V3GraphEdge{&graph, t0, t1, 1};
        പുതിയ V3GraphEdge{&graph, t0, t2, 1};
        പുതിയ V3GraphEdge{&graph, t2, t3, 1};
        പുതിയ V3GraphEdge{&graph, t2, t4, 1};
        പുതിയ V3GraphEdge{&graph, t3, t5, 1};
        പുതിയ V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        സ്ഥിരം std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        വേണ്ടി (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_വൃക്ഷം();
        VL_DO_DANGLING(execGraphp->delete_വൃക്ഷം(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    സ്ഥിതം ശൂന്യം selfTestHierFirst(AstVarScope* trigVscp) {
        File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
        AstExecGraph* സ്ഥിരം execGraphp = പുതിയ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        സ്ഥിരം auto makeBody = [&]() -> Ast_കണ്ണി_Stmt* { തിരികെ പുതിയ AstComment{flp, ""}; };
        ExecMTask* സ്ഥിരം t0 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* സ്ഥിരം t1 = പുതിയ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        പുതിയ V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        സ്ഥിരം std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        വേണ്ടി (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        വേണ്ടി (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_വൃക്ഷം();
        VL_DO_DANGLING(execGraphp->delete_വൃക്ഷം(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    സ്ഥിതം std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        തിരികെ PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

ശൂന്യം normalizeCosts(Costs& costs) {
    സ്ഥിരം auto scaleCost = [](uint64_t മൂല്യം, ഇരട്ട multiplier) {
        ഇരട്ട scaled = static_cast<ഇരട്ട>(മൂല്യം) * multiplier;
        എങ്കിൽ (മൂല്യം && scaled < 1) scaled = 1;
        തിരികെ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    വേണ്ടി (സ്ഥിരം auto& est : costs) {
        എങ്കിൽ (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    എങ്കിൽ (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        സ്ഥിരം ഇരട്ട estToProfile
            = static_cast<ഇരട്ട>(sumCostProfiled) / static_cast<ഇരട്ട>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        വേണ്ടി (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    വേണ്ടി (auto& est : costs) {
        സ്ഥിരം uint64_t& costEstimate = est.second.first;
        സ്ഥിരം uint64_t& costProfiled = est.second.second;
        എങ്കിൽ (maxCost < costEstimate) maxCost = costEstimate;
        എങ്കിൽ (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    എങ്കിൽ (maxCost > scaleDownTo) {
        സ്ഥിരം ഇരട്ട scaleup = static_cast<ഇരട്ട>(scaleDownTo) / static_cast<ഇരട്ട>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        വേണ്ടി (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

ശൂന്യം removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    വേണ്ടി (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        എങ്കിൽ (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_വൃക്ഷം(), callp);
    }
    V3Graph* സ്ഥിരം execMTaskGraphp = execGraphp->depGraphp();
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* സ്ഥിരം mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* സ്ഥിരം funcp = mtaskp->funcp();
        എങ്കിൽ (funcp->stmtsp()) തുടരുക;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_വൃക്ഷം(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

ശൂന്യം fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    വേണ്ടി (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* സ്ഥിരം mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        സ്ഥിരം uint64_t costEstimate = V3_Instr_എണ്ണം::എണ്ണം(mtp->funcp(), അസത്യം);
        സ്ഥിരം uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        എങ്കിൽ (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    പൂർണ്ണസംഖ്യ totalEstimates = 0;
    പൂർണ്ണസംഖ്യ missingProfiles = 0;
    വേണ്ടി (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* സ്ഥിരം mtp = vtx.as<ExecMTask>();
        സ്ഥിരം uint32_t costEstimate = costs[mtp->id()].first;
        സ്ഥിരം uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        സ്ഥിരം uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        എങ്കിൽ (!costProfiled32) {
            costToUse = costEstimate;
            എങ്കിൽ (costEstimate != 0) ++missingProfiles;
        }
        എങ്കിൽ (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    എങ്കിൽ (missingProfiles) {
        എങ്കിൽ (File_വരി* സ്ഥിരം fl = V3Control::get_Profile_Data_File_വരി()) {
            എങ്കിൽ (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

ശൂന്യം finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    ആകുമ്പോൾ (സ്ഥിരം V3GraphVertex* സ്ഥിരം vxp = ser.nextp()) {
        ExecMTask* സ്ഥിരം mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        വേണ്ടി (V3GraphEdge& edge : mtp->outEdges()) {
            സ്ഥിരം ExecMTask* സ്ഥിരം followp = edge.മുകൾ()->as<ExecMTask>();
            എങ്കിൽ ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    സ്ഥിരം auto report = execMTaskGraphp->parallelismReport([](സ്ഥിരം V3GraphVertex* vtxp) {  //
        തിരികെ vtxp->as<സ്ഥിരം ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_എണ്ണം());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_എണ്ണം());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    എങ്കിൽ (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_എണ്ണം());
        UINFO(0, "    Edge count = " << report.edge_എണ്ണം());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(സ്ഥിരം std::string& name, uint32_t nDependencies) {
    Ast_കണ്ണി_Module* സ്ഥിരം modp = v3Global.rootp()->മുകൾ_Modulep();
    File_വരി* സ്ഥിരം fl = modp->fileline();
    AstBasicDType* സ്ഥിരം dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(സത്യം);
    varp->valuep(പുതിയ AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    തിരികെ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, സ്ഥിരം std::string& method) {
    File_വരി* സ്ഥിരം fl = varp->fileline();
    AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    സ്ഥിരം AstScope* സ്ഥിരം മുകൾ_Scopep = v3Global.rootp()->മുകൾ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, മുകൾ_Scopep->nameDotless()});
    AstCStmt* സ്ഥിരം stmtp = പുതിയ AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    തിരികെ stmtp;
}

ശൂന്യം addMTaskToFunction(സ്ഥിരം ThreadSchedule& schedule, സ്ഥിരം uint32_t threadId, AstCFunc* funcp,
                        സ്ഥിരം ExecMTask* mtaskp,
                        സ്ഥിരം std::unordered_map<സ്ഥിരം ExecMTask*, AstVar*>& stateVars) {
    AstScope* സ്ഥിരം scopep = v3Global.rootp()->മുകൾ_Scopep()->scopep();
    Ast_കണ്ണി_Module* സ്ഥിരം modp = v3Global.rootp()->മുകൾ_Modulep();
    File_വരി* സ്ഥിരം fl = modp->fileline();

    // Helper function to make the code a bit more legible
    സ്ഥിരം auto addCStmt = [=](സ്ഥിരം string& stmt) -> ശൂന്യം {  //
        funcp->addStmtsp(പുതിയ AstCStmt{fl, stmt});
    };

    എങ്കിൽ (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        എങ്കിൽ (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        എങ്കിൽ (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    എങ്കിൽ (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    എങ്കിൽ (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : mtaskp->outEdges()) {
        സ്ഥിരം ExecMTask* സ്ഥിരം nextp = edge.മുകൾ()->as<ExecMTask>();
        എങ്കിൽ (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

സ്ഥിരം std::vector<AstCFunc*> createThreadFunctions(സ്ഥിരം ThreadSchedule& schedule,
                                                   സ്ഥിരം string& tag) {
    Ast_കണ്ണി_Module* സ്ഥിരം modp = v3Global.rootp()->മുകൾ_Modulep();
    File_വരി* സ്ഥിരം fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<സ്ഥിരം ExecMTask*, AstVar*> stateVars;
    വേണ്ടി (സ്ഥിരം std::vector<സ്ഥിരം ExecMTask*>& thread : schedule.m_threads) {
        വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp : thread) {
            എങ്കിൽ (സ്ഥിരം uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    വേണ്ടി (സ്ഥിരം std::vector<സ്ഥിരം ExecMTask*>& thread : schedule.m_threads) {
        എങ്കിൽ (thread.empty()) തുടരുക;
        സ്ഥിരം uint32_t threadId = schedule.threadId(thread.front());
        സ്ഥിരം string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.തള്ളുക_back(funcp);
        funcp->isStatic(സത്യം);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(സത്യം);
        funcp->entryPoint(സത്യം);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(പുതിയ AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(പുതിയ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        വേണ്ടി (സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    തിരികെ funcps;
}

ശൂന്യം addThreadStartWrapper(AstExecGraph* സ്ഥിരം execGraphp) {
    // FileLine used for constructing nodes below
    File_വരി* സ്ഥിരം fl = v3Global.rootp()->fileline();
    സ്ഥിരം string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    സ്ഥിരം auto addCStmt = [=](സ്ഥിരം string& stmt) -> ശൂന്യം {  //
        execGraphp->runfuncp()->addStmtsp(പുതിയ AstCStmt{fl, stmt});
    };

    എങ്കിൽ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    എങ്കിൽ (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

ശൂന്യം addThreadEndWrapper(AstExecGraph* സ്ഥിരം execGraphp) {
    // Add thread function invocations to execGraph
    സ്ഥിരം auto addCStmt = [=](സ്ഥിരം string& stmt) -> ശൂന്യം {  //
        File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(പുതിയ AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    എങ്കിൽ (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
ശൂന്യം addThreadStartToExecGraph(AstExecGraph* സ്ഥിരം execGraphp,
                               സ്ഥിരം std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_വരി* സ്ഥിരം fl = v3Global.rootp()->fileline();
    സ്ഥിരം string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* സ്ഥിരം finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    വേണ്ടി (AstCFunc* സ്ഥിരം funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    സ്ഥിരം auto addCStmt = [=](സ്ഥിരം string& stmt) -> ശൂന്യം {  //
        execGraphp->runfuncp()->addStmtsp(പുതിയ AstCStmt{fl, stmt});
    };

    സ്ഥിരം uint32_t last = funcps.size() - 1;
    എങ്കിൽ (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    വേണ്ടി (AstCFunc* സ്ഥിരം funcp : funcps) {
        എങ്കിൽ (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            എങ്കിൽ (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } അല്ലെങ്കിൽ {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(പുതിയ AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } അല്ലെങ്കിൽ {
            // The last will run on the main thread.
            AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    എങ്കിൽ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    എങ്കിൽ (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    എങ്കിൽ (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

ശൂന്യം processMTaskBodies(AstExecGraph* സ്ഥിരം execGraphp) {
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* സ്ഥിരം mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* സ്ഥിരം funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_കണ്ണി* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        സ്ഥിരം auto addCStmt = [=](സ്ഥിരം string& stmt) -> ശൂന്യം {  //
            funcp->addStmtsp(പുതിയ AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        എങ്കിൽ (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            എങ്കിൽ (v3Global.opt.hierChild()) args += v3Global.opt.മുകൾ_Module();
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
        എങ്കിൽ (v3Global.opt.profExec()) {
            സ്ഥിരം std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

ശൂന്യം implementExecGraph(AstExecGraph* സ്ഥിരം execGraphp, സ്ഥിരം ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    എങ്കിൽ (execGraphp->depGraphp()->empty()) തിരികെ;

    // Create a function to be run by each thread.
    സ്ഥിരം std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

ശൂന്യം createRunFunction(AstExecGraph* സ്ഥിരം execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_വരി* സ്ഥിരം flp = execGraphp->fileline();
    Ast_കണ്ണി_Module* സ്ഥിരം modp = v3Global.rootp()->മുകൾ_Modulep();
    AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(സത്യം);
    funcp->voidSelf(സത്യം);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(സത്യം);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

ശൂന്യം addTriggerMasks(AstExecGraph* സ്ഥിരം execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_വരി* സ്ഥിരം flp = execGraphp->fileline();
    AstVar* സ്ഥിരം triggersp = execGraphp->triggerp()->varp();
    വേണ്ടി (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](സ്ഥിരം AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            വേണ്ടി (സ്ഥിരം uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{flp, triggersp->dtypep(),
                                                     പുതിയ AstConst{flp, AstConst::Unsized64{}, 0}};
        വേണ്ടി (സ്ഥിരം auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          പുതിയ AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_വൃക്ഷം(), consp);
    }
}

ശൂന്യം addTriggerCosts(AstExecGraph* സ്ഥിരം execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_വരി* സ്ഥിരം flp = execGraphp->fileline();
    സ്ഥിരം uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    വേണ്ടി (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            എങ്കിൽ (സ്ഥിരം AstTriggered* സ്ഥിരം trigp = VN_CAST(ifp->condp(), Triggered)) {
                സ്ഥിരം uint32_t cost = V3_Instr_എണ്ണം::എണ്ണം(ifp, അസത്യം);
                വേണ്ടി (സ്ഥിരം uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
    AstUnpackArrayDType* സ്ഥിരം dtypep
        = പുതിയ AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  പുതിയ AstRange{flp, static_cast<പൂർണ്ണസംഖ്യ>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{flp, dtypep, പുതിയ AstConst{flp, 0}};
    വേണ്ടി (uint32_t index = 0; index < nTriggers; ++index) {
        എങ്കിൽ (costs[index]) consp->valuep(index, പുതിയ AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_വൃക്ഷം(), consp);
}

// Called by Verilator top stage
ശൂന്യം implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->മുകൾ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    വേണ്ടി (AstExecGraph* സ്ഥിരം execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        എങ്കിൽ (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_വൃക്ഷം(), execGraphp);
            തിരികെ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        എങ്കിൽ (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        സ്ഥിരം std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<ഇരട്ട>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        വേണ്ടി (സ്ഥിരം ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

ശൂന്യം selfTest() {
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
