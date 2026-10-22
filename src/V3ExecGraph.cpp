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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3ExecGraph.h"

#समावेश "V3ConstPool.h"
#समावेश "V3Control.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3File.h"
#समावेश "V3GraphStream.h"
#समावेश "V3Hasher.h"
#समावेश "V3InstrCount.h"
#समावेश "V3Os.h"
#समावेश "V3Stats.h"

#समावेश <map>
#समावेश <memory>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठ_Stmt* stmtsp,
                                 uint32_t id) {
    स्थिर std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* स्थिर newp = नया AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(true);
    newp->voidSelf(true);  // Called through VlExecGraph::Fnp
    newp->dontCombine(true);
    newp->addStmtsp(stmtsp);
    अगर (scopep) scopep->addBlocksp(newp);
    वापस newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_गाँठ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_अग्र_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

व्योम ExecMTask::dump(std::ब_प्रवाह& str) स्थिर {
    str << name() << "." << cvtToHex(यह);
    अगर (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_अग्र_Id{0};

namespace V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
श्रेणी ThreadSchedule final {
    मित्र श्रेणी PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    अटल uint32_t s_अग्र_Id;  // Next ID number to use
    std::unordered_set<स्थिर ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

खुला:
    // CONSTANTS
    अटल constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    काष्ठा MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        स्थिर ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<स्थिर ExecMTask*>> m_threads;

    // Global state for each mtask.
    अटल std::unordered_map<स्थिर ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_अग्र_Id++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = शेष;
    ThreadSchedule& चालक=(ThreadSchedule&&) = शेष;

निजी:
    VL_UNCOPYABLE(ThreadSchedule);

    अटल constexpr द्विगुन s_threadBoxWidth = 2.5;
    अटल constexpr द्विगुन s_threadBoxHeight = 1.5;
    अटल constexpr द्विगुन s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    अटल व्योम dumpDotFilePrefixedAlways(स्थिर std::vector<ThreadSchedule>& schedules,
                                          स्थिर string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    अटल व्योम dumpDotFile(स्थिर std::vector<ThreadSchedule>& schedules, स्थिर string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        स्थिर std::unique_ptr<std::बख_प्रवाह> logp{V3File::new_ofstream(filename)};
        अगर (logp->हार()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        क्रम (uint32_t i = 0; i < nThreads; ++i) {
            स्थिर string name = "t" + std::to_string(i);
            स्थिर string label = "Thread " + std::to_string(i);
            constexpr द्विगुन posX = -s_horizontalGap;
            स्थिर द्विगुन posY = -static_cast<द्विगुन>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        क्रम (स्थिर स्वतः& state : ThreadSchedule::s_mtaskState) {
            स्थिर ExecMTask* स्थिर mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        स्थिर uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<द्विगुन> offsets(nThreads, 0.0);
        क्रम (स्थिर ThreadSchedule& schedule : schedules) {
            अगर (schedule.mtasks.empty()) जारी;
            using Column = std::vector<स्थिर ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            काष्ठा Cmp final {
                bool चालक()(स्थिर ExecMTask* स्थिर a, स्थिर ExecMTask* स्थिर b) स्थिर {
                    अगर (startTime(a) == startTime(b)) वापस threadId(a) < threadId(b);
                    वापस startTime(a) < startTime(b);
                }
            };
            स्थिर std::multiset<स्थिर ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            क्रम (स्थिर ExecMTask* स्थिर mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.आकार() <= nThreads, "Invalid partitioning");

                bool intersects = true;
                क्रम (स्थिर ExecMTask* स्थिर earlierMtask : column) {
                    अगर (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = false;
                        अवरोध;
                    }
                }
                अगर (intersects) {
                    column.emplace_back(mtaskp);
                } अन्यथा {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            क्रम (स्थिर Column& column : columns) {
                द्विगुन lastColumnOffset = 0;
                क्रम (स्थिर ExecMTask* स्थिर mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            क्रम (स्थिर std::vector<स्थिर ExecMTask*>& thread : schedule.m_threads) {
                अगर (thread.empty()) अवरोध;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<पूर्णांक>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                क्रम (माप_प्रकार i = 1; i < thread.आकार(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                क्रम (स्थिर ExecMTask* स्थिर mtaskp : thread) {
                    क्रम (स्थिर V3GraphEdge& edge : mtaskp->outEdges()) {
                        स्थिर ExecMTask* स्थिर शीर्ष_MTaskp = edge.शीर्ष()->cast<स्थिर ExecMTask>();
                        अगर (शीर्ष_MTaskp && schedule.contains(शीर्ष_MTaskp)
                            && threadId(शीर्ष_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << शीर्ष_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->बंद();
    }
    अटल व्योम dumpDotFileEmitBlock(स्थिर std::unique_ptr<std::बख_प्रवाह>& logp,
                                     स्थिर string& name, स्थिर string& label, द्विगुन width,
                                     द्विगुन height, द्विगुन xPos, द्विगुन yPos,
                                     स्थिर string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    अटल व्योम dumpDotFileEmitMTask(स्थिर std::unique_ptr<std::बख_प्रवाह>& logp,
                                     स्थिर ExecMTask* स्थिर mtaskp, स्थिर ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<द्विगुन>& offsets) {
        क्रम (पूर्णांक i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            स्थिर string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            स्थिर string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            स्थिर द्विगुन width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<द्विगुन>(mtaskp->cost()) / segmentCost);
            स्थिर uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.आकार();
            स्थिर द्विगुन xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            स्थिर द्विगुन yPos = -s_threadBoxHeight * static_cast<द्विगुन>(mtaskThreadId);
            स्थिर string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    अटल व्योम dumpDotFileEmitFork(स्थिर std::unique_ptr<std::बख_प्रवाह>& logp, द्विगुन offset,
                                    uint32_t nThreads) {
        स्थिर string& name = "fork_" + std::to_string(static_cast<पूर्णांक>(offset));
        constexpr द्विगुन width = s_threadBoxWidth / 8;
        स्थिर द्विगुन height = s_threadBoxHeight * nThreads;
        स्थिर द्विगुन xPos = offset - s_horizontalGap / 2;
        स्थिर द्विगुन yPos
            = -static_cast<द्विगुन>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

खुला:
    अटल uint32_t threadId(स्थिर ExecMTask* mtaskp) {
        स्थिर स्वतः& it = s_mtaskState.find(mtaskp);
        वापस it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    अटल uint32_t startTime(स्थिर ExecMTask* mtaskp) {
        वापस s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    अटल uint32_t endTime(स्थिर ExecMTask* mtaskp) {
        वापस s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(स्थिर ExecMTask* mtaskp) स्थिर {
        स्थिर uint32_t thisThreadId = threadId(mtaskp);
        uint32_t परिणाम = 0;
        क्रम (स्थिर V3GraphEdge& edge : mtaskp->inEdges()) {
            स्थिर ExecMTask* स्थिर prevp = edge.fromp()->as<ExecMTask>();
            अगर (threadId(prevp) != thisThreadId && contains(prevp)) ++परिणाम;
        }
        वापस परिणाम;
    }

    uint32_t id() स्थिर { वापस m_id; }
    uint32_t scheduleOn(स्थिर ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        स्थिर uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<स्थिर ExecMTask*>& bestThread = m_threads[bestThreadId];
        अगर (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.धकेलो_back(mtaskp);
        वापस bestEndTime;
    }
    bool contains(स्थिर ExecMTask* mtaskp) स्थिर { वापस mtasks.गिनती(mtaskp); }
    uint32_t endTime() स्थिर { वापस m_endTime; }
};

uint32_t ThreadSchedule::s_अग्र_Id = 0;
std::unordered_map<स्थिर ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr द्विगुन V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
श्रेणी PackThreads final {
    // TYPES
    काष्ठा MTaskCmp final {
        bool चालक()(स्थिर ExecMTask* ap, स्थिर ExecMTask* bp) स्थिर {
            वापस ap->id() < bp->id();
        }
    };

    // MEMBERS
    स्थिर uint32_t m_nThreads;  // Number of threads
    स्थिर uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    स्थिर uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    स्थिर uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         अचिन्हित sandbagNumerator = 30, अचिन्हित sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = शेष;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(स्थिर ThreadSchedule& schedule, स्थिर ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        अगर (!schedule.contains(mtaskp)) वापस 0;
        स्थिर ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        अगर (threadId == state.threadId) {
            // No overhead on same thread
            वापस state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        अगर (state.nextp) {
            स्थिर uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            अगर ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        वापस sandbaggedEndTime;
    }

    अटल bool isReady(ThreadSchedule& schedule, स्थिर ExecMTask* mtaskp) {
        क्रम (स्थिर V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            स्थिर ExecMTask* स्थिर prevp = edgeIn.fromp()->as<स्थिर ExecMTask>();
            अगर (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                वापस false;
            }
        }
        वापस true;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::vector<ThreadSchedule> pack(V3Graph& mtaskGraph) {
        std::vector<ThreadSchedule> परिणाम;
        परिणाम.emplace_back(ThreadSchedule{m_nThreads});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        क्रमागत श्रेणी SchedulingMode : uint8_t {
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
        पूर्णांक maxThreadWorkers = 1;

        // Build initial ready list
        क्रम (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* स्थिर mtaskp = vtx.as<ExecMTask>();
            अगर (isReady(परिणाम.back(), mtaskp)) readyMTasks.जोड़ो(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        जबतक (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = परिणाम.back();
            क्रम (uint32_t threadId = 0; threadId < schedule.m_threads.आकार(); ++threadId) {
                क्रम (ExecMTask* स्थिर mtaskp : readyMTasks) {
                    अगर (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        जारी;
                    }
                    अगर (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        जारी;

                    uint32_t timeBegin = busyUntil[threadId];
                    अगर (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        अवरोध;
                    }
                    क्रम (स्थिर V3GraphEdge& edge : mtaskp->inEdges()) {
                        स्थिर ExecMTask* स्थिर priorp = edge.fromp()->as<ExecMTask>();
                        स्थिर uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        अगर (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    अगर ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            स्थिर uint32_t endTime = schedule.endTime();

            अगर (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                स्थिर uint32_t आकार = m_nHierThreads / maxThreadWorkers;
                UASSERT(आकार, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                अगर (schedule.s_mtaskState.empty()) परिणाम.सफा();
                परिणाम.emplace_back(ThreadSchedule{आकार});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                जारी;
            }

            अगर (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                परिणाम.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                जारी;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            स्थिर uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            स्थिर माप_प्रकार erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            क्रम (स्थिर V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* स्थिर nextp = edgeOut.शीर्ष()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                अगर (isReady(schedule, nextp)) {
                    readyMTasks.जोड़ो(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        अगर (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(परिणाम, "schedule", m_nThreads);

        वापस परिणाम;
    }

खुला:
    // SELF TEST
    अटल व्योम selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
        AstVar* स्थिर varp = नया AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* स्थिर scopep = नया AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* स्थिर trigVscp = नया AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_वृक्ष(), trigVscp);
        VL_DO_DANGLING(scopep->delete_वृक्ष(), scopep);
        VL_DO_DANGLING(varp->delete_वृक्ष(), varp);
    }
    अटल व्योम selfTestNormalFirst(AstVarScope* trigVscp) {
        File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
        AstExecGraph* स्थिर execGraphp = नया AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        स्थिर स्वतः makeBody = [&]() -> Ast_गाँठ_Stmt* { वापस नया AstComment{flp, ""}; };
        ExecMTask* स्थिर t0 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* स्थिर t1 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* स्थिर t2 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* स्थिर t3 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* स्थिर t4 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* स्थिर t5 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* स्थिर t6 = नया ExecMTask{execGraphp, nullptr, makeBody()};
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
        नया V3GraphEdge{&graph, t0, t1, 1};
        नया V3GraphEdge{&graph, t0, t2, 1};
        नया V3GraphEdge{&graph, t2, t3, 1};
        नया V3GraphEdge{&graph, t2, t4, 1};
        नया V3GraphEdge{&graph, t3, t5, 1};
        नया V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        स्थिर std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.आकार(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.आकार(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].आकार(), 2);
        क्रम (माप_प्रकार i = 1; i < scheduled[0].m_threads.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].आकार(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_threads[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_threads.आकार(), hierThreads / 3);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_threads[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_threads.आकार(), threads);
        UASSERT_SELFTEST(scheduled[2].m_threads[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_threads[1][0], t6);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.आकार(), 7);

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

        क्रम (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_वृक्ष();
        VL_DO_DANGLING(execGraphp->delete_वृक्ष(), execGraphp);
        ThreadSchedule::s_mtaskState.सफा();
    }
    अटल व्योम selfTestHierFirst(AstVarScope* trigVscp) {
        File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
        AstExecGraph* स्थिर execGraphp = नया AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        स्थिर स्वतः makeBody = [&]() -> Ast_गाँठ_Stmt* { वापस नया AstComment{flp, ""}; };
        ExecMTask* स्थिर t0 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* स्थिर t1 = नया ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        नया V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        स्थिर std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.आकार(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.आकार(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].आकार(), 1);
        क्रम (माप_प्रकार i = 1; i < scheduled[0].m_threads.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].आकार(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.आकार(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].आकार(), 1);
        क्रम (माप_प्रकार i = 1; i < scheduled[1].m_threads.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].आकार(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.आकार(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        क्रम (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_वृक्ष();
        VL_DO_DANGLING(execGraphp->delete_वृक्ष(), execGraphp);
        ThreadSchedule::s_mtaskState.सफा();
    }

    अटल std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        वापस PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

व्योम normalizeCosts(Costs& costs) {
    स्थिर स्वतः scaleCost = [](uint64_t मूल्य, द्विगुन multiplier) {
        द्विगुन scaled = static_cast<द्विगुन>(मूल्य) * multiplier;
        अगर (मूल्य && scaled < 1) scaled = 1;
        वापस static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t योग_Cost_Profiled = 0;  // For data with estimate and profile
    uint64_t योग_Cost_Estimate = 0;  // For data with estimate and profile
    क्रम (स्थिर स्वतः& est : costs) {
        अगर (est.second.second) {
            योग_Cost_Estimate += est.second.first;
            योग_Cost_Profiled += est.second.second;
        }
    }

    अगर (योग_Cost_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        स्थिर द्विगुन estToProfile
            = static_cast<द्विगुन>(योग_Cost_Profiled) / static_cast<द्विगुन>(योग_Cost_Estimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << योग_Cost_Profiled
                                                    << " sumCostEstimate=" << योग_Cost_Estimate);
        क्रम (स्वतः& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    क्रम (स्वतः& est : costs) {
        स्थिर uint64_t& costEstimate = est.second.first;
        स्थिर uint64_t& costProfiled = est.second.second;
        अगर (maxCost < costEstimate) maxCost = costEstimate;
        अगर (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    अगर (maxCost > scaleDownTo) {
        स्थिर द्विगुन scaleup = static_cast<द्विगुन>(scaleDownTo) / static_cast<द्विगुन>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        क्रम (स्वतः& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

व्योम removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    क्रम (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        अगर (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_वृक्ष(), callp);
    }
    V3Graph* स्थिर execMTaskGraphp = execGraphp->depGraphp();
    क्रम (V3GraphVertex* स्थिर vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* स्थिर mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* स्थिर funcp = mtaskp->funcp();
        अगर (funcp->stmtsp()) जारी;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_वृक्ष(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

व्योम fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    क्रम (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* स्थिर mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        स्थिर uint64_t costEstimate = V3_Instr_गिनती::गिनती(mtp->funcp(), false);
        स्थिर uint64_t costProfiled
            = V3Control::get_Profile_तथ्य(v3Global.opt.prefix(), mtp->hashName());
        अगर (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    पूर्णांक योग_Estimates = 0;
    पूर्णांक missingProfiles = 0;
    क्रम (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* स्थिर mtp = vtx.as<ExecMTask>();
        स्थिर uint32_t costEstimate = costs[mtp->id()].first;
        स्थिर uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        स्थिर uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        अगर (!costProfiled32) {
            costToUse = costEstimate;
            अगर (costEstimate != 0) ++missingProfiles;
        }
        अगर (costEstimate != 0) ++योग_Estimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    अगर (missingProfiles) {
        अगर (File_पंक्ति* स्थिर fl = V3Control::get_Profile_तथ्य_File_पंक्ति()) {
            अगर (V3Control::contains_MTask_Profile_तथ्य()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << योग_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

व्योम finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    जबतक (स्थिर V3GraphVertex* स्थिर vxp = ser.nextp()) {
        ExecMTask* स्थिर mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        क्रम (V3GraphEdge& edge : mtp->outEdges()) {
            स्थिर ExecMTask* स्थिर followp = edge.शीर्ष()->as<ExecMTask>();
            अगर ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    स्थिर स्वतः report = execMTaskGraphp->parallelismReport([](स्थिर V3GraphVertex* vtxp) {  //
        वापस vtxp->as<स्थिर ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.योग_Graph_Cost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_गिनती());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_गिनती());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    अगर (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.योग_Graph_Cost());
        UINFO(0, "    MTask vertex count = " << report.vertex_गिनती());
        UINFO(0, "    Edge count = " << report.edge_गिनती());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(स्थिर std::string& name, uint32_t nDependencies) {
    Ast_गाँठ_Module* स्थिर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पंक्ति* स्थिर fl = modp->fileline();
    AstBasicDType* स्थिर dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* स्थिर varp = नया AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(true);
    varp->valuep(नया AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    वापस varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, स्थिर std::string& method) {
    File_पंक्ति* स्थिर fl = varp->fileline();
    AstVarRef* स्थिर refp = नया AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    स्थिर AstScope* स्थिर शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, शीर्ष_Scopep->nameDotless()});
    AstCStmt* स्थिर stmtp = नया AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    वापस stmtp;
}

व्योम addMTaskToFunction(स्थिर ThreadSchedule& schedule, स्थिर uint32_t threadId, AstCFunc* funcp,
                        स्थिर ExecMTask* mtaskp,
                        स्थिर std::unordered_map<स्थिर ExecMTask*, AstVar*>& stateVars) {
    AstScope* स्थिर scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    Ast_गाँठ_Module* स्थिर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पंक्ति* स्थिर fl = modp->fileline();

    // Helper function to make the code a bit more legible
    स्थिर स्वतः addCStmt = [=](स्थिर string& stmt) -> व्योम {  //
        funcp->addStmtsp(नया AstCStmt{fl, stmt});
    };

    अगर (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        अगर (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        अगर (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    अगर (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* स्थिर callp = नया AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    अगर (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    क्रम (स्थिर V3GraphEdge& edge : mtaskp->outEdges()) {
        स्थिर ExecMTask* स्थिर nextp = edge.शीर्ष()->as<ExecMTask>();
        अगर (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

स्थिर std::vector<AstCFunc*> createThreadFunctions(स्थिर ThreadSchedule& schedule,
                                                   स्थिर string& tag) {
    Ast_गाँठ_Module* स्थिर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पंक्ति* स्थिर fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<स्थिर ExecMTask*, AstVar*> stateVars;
    क्रम (स्थिर std::vector<स्थिर ExecMTask*>& thread : schedule.m_threads) {
        क्रम (स्थिर ExecMTask* स्थिर mtaskp : thread) {
            अगर (स्थिर uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    क्रम (स्थिर std::vector<स्थिर ExecMTask*>& thread : schedule.m_threads) {
        अगर (thread.empty()) जारी;
        स्थिर uint32_t threadId = schedule.threadId(thread.front());
        स्थिर string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* स्थिर funcp = नया AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.धकेलो_back(funcp);
        funcp->isStatic(true);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(true);
        funcp->entryPoint(true);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(नया AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(नया AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        क्रम (स्थिर ExecMTask* स्थिर mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    वापस funcps;
}

व्योम addThreadStartWrapper(AstExecGraph* स्थिर execGraphp) {
    // FileLine used for constructing nodes below
    File_पंक्ति* स्थिर fl = v3Global.rootp()->fileline();
    स्थिर string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    स्थिर स्वतः addCStmt = [=](स्थिर string& stmt) -> व्योम {  //
        execGraphp->runfuncp()->addStmtsp(नया AstCStmt{fl, stmt});
    };

    अगर (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    अगर (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

व्योम addThreadEndWrapper(AstExecGraph* स्थिर execGraphp) {
    // Add thread function invocations to execGraph
    स्थिर स्वतः addCStmt = [=](स्थिर string& stmt) -> व्योम {  //
        File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(नया AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    अगर (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
व्योम addThreadStartToExecGraph(AstExecGraph* स्थिर execGraphp,
                               स्थिर std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_पंक्ति* स्थिर fl = v3Global.rootp()->fileline();
    स्थिर string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* स्थिर finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.आकार());
    क्रम (AstCFunc* स्थिर funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    स्थिर स्वतः addCStmt = [=](स्थिर string& stmt) -> व्योम {  //
        execGraphp->runfuncp()->addStmtsp(नया AstCStmt{fl, stmt});
    };

    स्थिर uint32_t last = funcps.आकार() - 1;
    अगर (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    क्रम (AstCFunc* स्थिर funcp : funcps) {
        अगर (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* स्थिर cstmtp = नया AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            अगर (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } अन्यथा {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(नया AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } अन्यथा {
            // The last will run on the main thread.
            AstCCall* स्थिर callp = नया AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::add_Stat_योग("Optimizations, Thread schedule total tasks", i);

    अगर (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    अगर (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    अगर (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

व्योम processMTaskBodies(AstExecGraph* स्थिर execGraphp) {
    क्रम (V3GraphVertex* स्थिर vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* स्थिर mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* स्थिर funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_गाँठ* stmtsp = funcp->stmtsp()->unlink_Fr_Back_With_अग्र();

        // Helper function to make the code a bit more legible
        स्थिर स्वतः addCStmt = [=](स्थिर string& stmt) -> व्योम {  //
            funcp->addStmtsp(नया AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        अगर (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            अगर (v3Global.opt.hierChild()) args += v3Global.opt.शीर्ष_Module();
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
        अगर (v3Global.opt.profExec()) {
            स्थिर std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

व्योम implementExecGraph(AstExecGraph* स्थिर execGraphp, स्थिर ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    अगर (execGraphp->depGraphp()->empty()) वापस;

    // Create a function to be run by each thread.
    स्थिर std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

व्योम createRunFunction(AstExecGraph* स्थिर execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_पंक्ति* स्थिर flp = execGraphp->fileline();
    Ast_गाँठ_Module* स्थिर modp = v3Global.rootp()->शीर्ष_Modulep();
    AstCFunc* स्थिर funcp = नया AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(true);
    funcp->voidSelf(true);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(true);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

व्योम addTriggerMasks(AstExecGraph* स्थिर execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_पंक्ति* स्थिर flp = execGraphp->fileline();
    AstVar* स्थिर triggersp = execGraphp->triggerp()->varp();
    क्रम (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](स्थिर AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            क्रम (स्थिर uint32_t सूचकांक_2 : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(सूचकांक_2)] |= VL_BIT_Q(सूचकांक_2);
            }
        });
        AstConsArray* स्थिर consp = नया AstConsArray{flp, triggersp->dtypep(),
                                                     नया AstConst{flp, AstConst::Unsized64{}, 0}};
        क्रम (स्थिर स्वतः& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          नया AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_वृक्ष(), consp);
    }
}

व्योम addTriggerCosts(AstExecGraph* स्थिर execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_पंक्ति* स्थिर flp = execGraphp->fileline();
    स्थिर uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    क्रम (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            अगर (स्थिर AstTriggered* स्थिर trigp = VN_CAST(ifp->condp(), Triggered)) {
                स्थिर uint32_t cost = V3_Instr_गिनती::गिनती(ifp, false);
                क्रम (स्थिर uint32_t सूचकांक_2 : trigp->indices()) costs.at(सूचकांक_2) += cost;
            }
        });
    }
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    AstUnpackArrayDType* स्थिर dtypep
        = नया AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  नया AstRange{flp, static_cast<पूर्णांक>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* स्थिर consp = नया AstConsArray{flp, dtypep, नया AstConst{flp, 0}};
    क्रम (uint32_t सूचकांक_2 = 0; सूचकांक_2 < nTriggers; ++सूचकांक_2) {
        अगर (costs[सूचकांक_2]) consp->valuep(सूचकांक_2, नया AstConst{flp, costs[सूचकांक_2]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_वृक्ष(), consp);
}

// Called by Verilator top stage
व्योम implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->शीर्ष_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    क्रम (AstExecGraph* स्थिर execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        अगर (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_वृक्ष(), execGraphp);
            वापस;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        अगर (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        स्थिर std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::add_Stat_योग("Optimizations, Thread schedule count",
                            static_cast<द्विगुन>(packed.आकार()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        क्रम (स्थिर ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

व्योम selfTest() {
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
