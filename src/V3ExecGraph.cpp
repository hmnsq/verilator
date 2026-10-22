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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3ExecGraph.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3GraphStream.h"
#अन्तर्भाव "V3Hasher.h"
#अन्तर्भाव "V3InstrCount.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठो_Stmt* stmtsp,
                                 uint32_t id) {
    अचर std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* अचर newp = नूतन AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(सत्य);
    newp->voidSelf(सत्य);  // Called through VlExecGraph::Fnp
    newp->dontCombine(सत्य);
    newp->addStmtsp(stmtsp);
    यदि (scopep) scopep->addBlocksp(newp);
    फर्काऊ newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_गाँठो_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

शून्य ExecMTask::dump(std::ostream& str) अचर {
    str << name() << "." << cvtToHex(एतत्);
    यदि (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

नामाकाश V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
वर्ग ThreadSchedule final {
    मित्र वर्ग PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    स्थिर uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<अचर ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

सार्वजनिक:
    // CONSTANTS
    स्थिर constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    संरचना MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        अचर ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<अचर ExecMTask*>> m_threads;

    // Global state for each mtask.
    स्थिर std::unordered_map<अचर ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = स्वभाव;
    ThreadSchedule& चालक=(ThreadSchedule&&) = स्वभाव;

गुप्त:
    VL_UNCOPYABLE(ThreadSchedule);

    स्थिर constexpr द्विगुण s_threadBoxWidth = 2.5;
    स्थिर constexpr द्विगुण s_threadBoxHeight = 1.5;
    स्थिर constexpr द्विगुण s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    स्थिर शून्य dumpDotFilePrefixedAlways(अचर std::vector<ThreadSchedule>& schedules,
                                          अचर string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    स्थिर शून्य dumpDotFile(अचर std::vector<ThreadSchedule>& schedules, अचर string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        अचर std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        यदि (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        लागि (uint32_t i = 0; i < nThreads; ++i) {
            अचर string name = "t" + std::to_string(i);
            अचर string label = "Thread " + std::to_string(i);
            constexpr द्विगुण posX = -s_horizontalGap;
            अचर द्विगुण posY = -static_cast<द्विगुण>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        लागि (अचर स्वतः& state : ThreadSchedule::s_mtaskState) {
            अचर ExecMTask* अचर mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        अचर uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<द्विगुण> offsets(nThreads, 0.0);
        लागि (अचर ThreadSchedule& schedule : schedules) {
            यदि (schedule.mtasks.empty()) जारी;
            using Column = std::vector<अचर ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            संरचना Cmp final {
                तार्किक चालक()(अचर ExecMTask* अचर a, अचर ExecMTask* अचर b) अचर {
                    यदि (startTime(a) == startTime(b)) फर्काऊ threadId(a) < threadId(b);
                    फर्काऊ startTime(a) < startTime(b);
                }
            };
            अचर std::multiset<अचर ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            लागि (अचर ExecMTask* अचर mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                तार्किक intersects = सत्य;
                लागि (अचर ExecMTask* अचर earlierMtask : column) {
                    यदि (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = असत्य;
                        रोक;
                    }
                }
                यदि (intersects) {
                    column.emplace_back(mtaskp);
                } अन्यथा {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            लागि (अचर Column& column : columns) {
                द्विगुण lastColumnOffset = 0;
                लागि (अचर ExecMTask* अचर mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            लागि (अचर std::vector<अचर ExecMTask*>& thread : schedule.m_threads) {
                यदि (thread.empty()) रोक;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<पूर्णाङ्क>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                लागि (माप_प्रकार i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                लागि (अचर ExecMTask* अचर mtaskp : thread) {
                    लागि (अचर V3GraphEdge& edge : mtaskp->outEdges()) {
                        अचर ExecMTask* अचर शीर्ष_MTaskp = edge.शीर्ष()->cast<अचर ExecMTask>();
                        यदि (शीर्ष_MTaskp && schedule.contains(शीर्ष_MTaskp)
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
    स्थिर शून्य dumpDotFileEmitBlock(अचर std::unique_ptr<std::ofstream>& logp,
                                     अचर string& name, अचर string& label, द्विगुण width,
                                     द्विगुण height, द्विगुण xPos, द्विगुण yPos,
                                     अचर string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    स्थिर शून्य dumpDotFileEmitMTask(अचर std::unique_ptr<std::ofstream>& logp,
                                     अचर ExecMTask* अचर mtaskp, अचर ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<द्विगुण>& offsets) {
        लागि (पूर्णाङ्क i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            अचर string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            अचर string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            अचर द्विगुण width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<द्विगुण>(mtaskp->cost()) / segmentCost);
            अचर uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            अचर द्विगुण xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            अचर द्विगुण yPos = -s_threadBoxHeight * static_cast<द्विगुण>(mtaskThreadId);
            अचर string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    स्थिर शून्य dumpDotFileEmitFork(अचर std::unique_ptr<std::ofstream>& logp, द्विगुण offset,
                                    uint32_t nThreads) {
        अचर string& name = "fork_" + std::to_string(static_cast<पूर्णाङ्क>(offset));
        constexpr द्विगुण width = s_threadBoxWidth / 8;
        अचर द्विगुण height = s_threadBoxHeight * nThreads;
        अचर द्विगुण xPos = offset - s_horizontalGap / 2;
        अचर द्विगुण yPos
            = -static_cast<द्विगुण>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

सार्वजनिक:
    स्थिर uint32_t threadId(अचर ExecMTask* mtaskp) {
        अचर स्वतः& it = s_mtaskState.find(mtaskp);
        फर्काऊ it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    स्थिर uint32_t startTime(अचर ExecMTask* mtaskp) {
        फर्काऊ s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    स्थिर uint32_t endTime(अचर ExecMTask* mtaskp) {
        फर्काऊ s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(अचर ExecMTask* mtaskp) अचर {
        अचर uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        लागि (अचर V3GraphEdge& edge : mtaskp->inEdges()) {
            अचर ExecMTask* अचर prevp = edge.fromp()->as<ExecMTask>();
            यदि (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        फर्काऊ result;
    }

    uint32_t id() अचर { फर्काऊ m_id; }
    uint32_t scheduleOn(अचर ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        अचर uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<अचर ExecMTask*>& bestThread = m_threads[bestThreadId];
        यदि (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.थप_back(mtaskp);
        फर्काऊ bestEndTime;
    }
    तार्किक contains(अचर ExecMTask* mtaskp) अचर { फर्काऊ mtasks.गन्ती(mtaskp); }
    uint32_t endTime() अचर { फर्काऊ m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<अचर ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr द्विगुण V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
वर्ग PackThreads final {
    // TYPES
    संरचना MTaskCmp final {
        तार्किक चालक()(अचर ExecMTask* ap, अचर ExecMTask* bp) अचर {
            फर्काऊ ap->id() < bp->id();
        }
    };

    // MEMBERS
    अचर uint32_t m_nThreads;  // Number of threads
    अचर uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    अचर uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    अचर uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         अचिन्हित sandbagNumerator = 30, अचिन्हित sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = स्वभाव;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(अचर ThreadSchedule& schedule, अचर ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        यदि (!schedule.contains(mtaskp)) फर्काऊ 0;
        अचर ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        यदि (threadId == state.threadId) {
            // No overhead on same thread
            फर्काऊ state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        यदि (state.nextp) {
            अचर uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            यदि ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        फर्काऊ sandbaggedEndTime;
    }

    स्थिर तार्किक isReady(ThreadSchedule& schedule, अचर ExecMTask* mtaskp) {
        लागि (अचर V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            अचर ExecMTask* अचर prevp = edgeIn.fromp()->as<अचर ExecMTask>();
            यदि (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                फर्काऊ असत्य;
            }
        }
        फर्काऊ सत्य;
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
        गणना वर्ग SchedulingMode : uint8_t {
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
        पूर्णाङ्क maxThreadWorkers = 1;

        // Build initial ready list
        लागि (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* अचर mtaskp = vtx.as<ExecMTask>();
            यदि (isReady(result.back(), mtaskp)) readyMTasks.घुसाउ(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        जबसम्म (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            लागि (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                लागि (ExecMTask* अचर mtaskp : readyMTasks) {
                    यदि (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        जारी;
                    }
                    यदि (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        जारी;

                    uint32_t timeBegin = busyUntil[threadId];
                    यदि (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        रोक;
                    }
                    लागि (अचर V3GraphEdge& edge : mtaskp->inEdges()) {
                        अचर ExecMTask* अचर priorp = edge.fromp()->as<ExecMTask>();
                        अचर uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        यदि (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    यदि ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            अचर uint32_t endTime = schedule.endTime();

            यदि (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                अचर uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                यदि (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                जारी;
            }

            यदि (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                जारी;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            अचर uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            अचर माप_प्रकार erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            लागि (अचर V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* अचर nextp = edgeOut.शीर्ष()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                यदि (isReady(schedule, nextp)) {
                    readyMTasks.घुसाउ(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        यदि (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        फर्काऊ result;
    }

सार्वजनिक:
    // SELF TEST
    स्थिर शून्य selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_पङ्क्ति* अचर flp = v3Global.rootp()->fileline();
        AstVar* अचर varp = नूतन AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* अचर scopep = नूतन AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* अचर trigVscp = नूतन AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_रूख(), trigVscp);
        VL_DO_DANGLING(scopep->delete_रूख(), scopep);
        VL_DO_DANGLING(varp->delete_रूख(), varp);
    }
    स्थिर शून्य selfTestNormalFirst(AstVarScope* trigVscp) {
        File_पङ्क्ति* अचर flp = v3Global.rootp()->fileline();
        AstExecGraph* अचर execGraphp = नूतन AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        अचर स्वतः makeBody = [&]() -> Ast_गाँठो_Stmt* { फर्काऊ नूतन AstComment{flp, ""}; };
        ExecMTask* अचर t0 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* अचर t1 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* अचर t2 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* अचर t3 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* अचर t4 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* अचर t5 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* अचर t6 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
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
        नूतन V3GraphEdge{&graph, t0, t1, 1};
        नूतन V3GraphEdge{&graph, t0, t2, 1};
        नूतन V3GraphEdge{&graph, t2, t3, 1};
        नूतन V3GraphEdge{&graph, t2, t4, 1};
        नूतन V3GraphEdge{&graph, t3, t5, 1};
        नूतन V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        अचर std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        लागि (माप_प्रकार i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        लागि (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_रूख();
        VL_DO_DANGLING(execGraphp->delete_रूख(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    स्थिर शून्य selfTestHierFirst(AstVarScope* trigVscp) {
        File_पङ्क्ति* अचर flp = v3Global.rootp()->fileline();
        AstExecGraph* अचर execGraphp = नूतन AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        अचर स्वतः makeBody = [&]() -> Ast_गाँठो_Stmt* { फर्काऊ नूतन AstComment{flp, ""}; };
        ExecMTask* अचर t0 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* अचर t1 = नूतन ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        नूतन V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        अचर std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        लागि (माप_प्रकार i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        लागि (माप_प्रकार i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        लागि (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_रूख();
        VL_DO_DANGLING(execGraphp->delete_रूख(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    स्थिर std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        फर्काऊ PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

शून्य normalizeCosts(Costs& costs) {
    अचर स्वतः scaleCost = [](uint64_t मान, द्विगुण multiplier) {
        द्विगुण scaled = static_cast<द्विगुण>(मान) * multiplier;
        यदि (मान && scaled < 1) scaled = 1;
        फर्काऊ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    लागि (अचर स्वतः& est : costs) {
        यदि (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    यदि (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        अचर द्विगुण estToProfile
            = static_cast<द्विगुण>(sumCostProfiled) / static_cast<द्विगुण>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        लागि (स्वतः& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    लागि (स्वतः& est : costs) {
        अचर uint64_t& costEstimate = est.second.first;
        अचर uint64_t& costProfiled = est.second.second;
        यदि (maxCost < costEstimate) maxCost = costEstimate;
        यदि (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    यदि (maxCost > scaleDownTo) {
        अचर द्विगुण scaleup = static_cast<द्विगुण>(scaleDownTo) / static_cast<द्विगुण>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        लागि (स्वतः& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

शून्य removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    लागि (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        यदि (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_रूख(), callp);
    }
    V3Graph* अचर execMTaskGraphp = execGraphp->depGraphp();
    लागि (V3GraphVertex* अचर vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* अचर mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* अचर funcp = mtaskp->funcp();
        यदि (funcp->stmtsp()) जारी;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_रूख(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

शून्य fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    लागि (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* अचर mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        अचर uint64_t costEstimate = V3_Instr_गन्ती::गन्ती(mtp->funcp(), असत्य);
        अचर uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        यदि (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    पूर्णाङ्क totalEstimates = 0;
    पूर्णाङ्क missingProfiles = 0;
    लागि (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* अचर mtp = vtx.as<ExecMTask>();
        अचर uint32_t costEstimate = costs[mtp->id()].first;
        अचर uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        अचर uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        यदि (!costProfiled32) {
            costToUse = costEstimate;
            यदि (costEstimate != 0) ++missingProfiles;
        }
        यदि (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    यदि (missingProfiles) {
        यदि (File_पङ्क्ति* अचर fl = V3Control::get_Profile_Data_File_पङ्क्ति()) {
            यदि (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

शून्य finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    जबसम्म (अचर V3GraphVertex* अचर vxp = ser.nextp()) {
        ExecMTask* अचर mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        लागि (V3GraphEdge& edge : mtp->outEdges()) {
            अचर ExecMTask* अचर followp = edge.शीर्ष()->as<ExecMTask>();
            यदि ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    अचर स्वतः report = execMTaskGraphp->parallelismReport([](अचर V3GraphVertex* vtxp) {  //
        फर्काऊ vtxp->as<अचर ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_गन्ती());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_गन्ती());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    यदि (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_गन्ती());
        UINFO(0, "    Edge count = " << report.edge_गन्ती());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(अचर std::string& name, uint32_t nDependencies) {
    Ast_गाँठो_Module* अचर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पङ्क्ति* अचर fl = modp->fileline();
    AstBasicDType* अचर dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* अचर varp = नूतन AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(सत्य);
    varp->valuep(नूतन AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    फर्काऊ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, अचर std::string& method) {
    File_पङ्क्ति* अचर fl = varp->fileline();
    AstVarRef* अचर refp = नूतन AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    अचर AstScope* अचर शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, शीर्ष_Scopep->nameDotless()});
    AstCStmt* अचर stmtp = नूतन AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    फर्काऊ stmtp;
}

शून्य addMTaskToFunction(अचर ThreadSchedule& schedule, अचर uint32_t threadId, AstCFunc* funcp,
                        अचर ExecMTask* mtaskp,
                        अचर std::unordered_map<अचर ExecMTask*, AstVar*>& stateVars) {
    AstScope* अचर scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    Ast_गाँठो_Module* अचर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पङ्क्ति* अचर fl = modp->fileline();

    // Helper function to make the code a bit more legible
    अचर स्वतः addCStmt = [=](अचर string& stmt) -> शून्य {  //
        funcp->addStmtsp(नूतन AstCStmt{fl, stmt});
    };

    यदि (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        यदि (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        यदि (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    यदि (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* अचर callp = नूतन AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    यदि (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    लागि (अचर V3GraphEdge& edge : mtaskp->outEdges()) {
        अचर ExecMTask* अचर nextp = edge.शीर्ष()->as<ExecMTask>();
        यदि (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

अचर std::vector<AstCFunc*> createThreadFunctions(अचर ThreadSchedule& schedule,
                                                   अचर string& tag) {
    Ast_गाँठो_Module* अचर modp = v3Global.rootp()->शीर्ष_Modulep();
    File_पङ्क्ति* अचर fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<अचर ExecMTask*, AstVar*> stateVars;
    लागि (अचर std::vector<अचर ExecMTask*>& thread : schedule.m_threads) {
        लागि (अचर ExecMTask* अचर mtaskp : thread) {
            यदि (अचर uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    लागि (अचर std::vector<अचर ExecMTask*>& thread : schedule.m_threads) {
        यदि (thread.empty()) जारी;
        अचर uint32_t threadId = schedule.threadId(thread.front());
        अचर string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* अचर funcp = नूतन AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.थप_back(funcp);
        funcp->isStatic(सत्य);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(सत्य);
        funcp->entryPoint(सत्य);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        लागि (अचर ExecMTask* अचर mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    फर्काऊ funcps;
}

शून्य addThreadStartWrapper(AstExecGraph* अचर execGraphp) {
    // FileLine used for constructing nodes below
    File_पङ्क्ति* अचर fl = v3Global.rootp()->fileline();
    अचर string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    अचर स्वतः addCStmt = [=](अचर string& stmt) -> शून्य {  //
        execGraphp->runfuncp()->addStmtsp(नूतन AstCStmt{fl, stmt});
    };

    यदि (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    यदि (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

शून्य addThreadEndWrapper(AstExecGraph* अचर execGraphp) {
    // Add thread function invocations to execGraph
    अचर स्वतः addCStmt = [=](अचर string& stmt) -> शून्य {  //
        File_पङ्क्ति* अचर flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(नूतन AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    यदि (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
शून्य addThreadStartToExecGraph(AstExecGraph* अचर execGraphp,
                               अचर std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_पङ्क्ति* अचर fl = v3Global.rootp()->fileline();
    अचर string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* अचर finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    लागि (AstCFunc* अचर funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    अचर स्वतः addCStmt = [=](अचर string& stmt) -> शून्य {  //
        execGraphp->runfuncp()->addStmtsp(नूतन AstCStmt{fl, stmt});
    };

    अचर uint32_t last = funcps.size() - 1;
    यदि (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    लागि (AstCFunc* अचर funcp : funcps) {
        यदि (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* अचर cstmtp = नूतन AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            यदि (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } अन्यथा {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(नूतन AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } अन्यथा {
            // The last will run on the main thread.
            AstCCall* अचर callp = नूतन AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    यदि (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    यदि (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    यदि (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

शून्य processMTaskBodies(AstExecGraph* अचर execGraphp) {
    लागि (V3GraphVertex* अचर vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* अचर mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* अचर funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_गाँठो* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        अचर स्वतः addCStmt = [=](अचर string& stmt) -> शून्य {  //
            funcp->addStmtsp(नूतन AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        यदि (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            यदि (v3Global.opt.hierChild()) args += v3Global.opt.शीर्ष_Module();
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
        यदि (v3Global.opt.profExec()) {
            अचर std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

शून्य implementExecGraph(AstExecGraph* अचर execGraphp, अचर ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    यदि (execGraphp->depGraphp()->empty()) फर्काऊ;

    // Create a function to be run by each thread.
    अचर std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

शून्य createRunFunction(AstExecGraph* अचर execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_पङ्क्ति* अचर flp = execGraphp->fileline();
    Ast_गाँठो_Module* अचर modp = v3Global.rootp()->शीर्ष_Modulep();
    AstCFunc* अचर funcp = नूतन AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(सत्य);
    funcp->voidSelf(सत्य);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(सत्य);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

शून्य addTriggerMasks(AstExecGraph* अचर execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_पङ्क्ति* अचर flp = execGraphp->fileline();
    AstVar* अचर triggersp = execGraphp->triggerp()->varp();
    लागि (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](अचर AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            लागि (अचर uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* अचर consp = नूतन AstConsArray{flp, triggersp->dtypep(),
                                                     नूतन AstConst{flp, AstConst::Unsized64{}, 0}};
        लागि (अचर स्वतः& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          नूतन AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_रूख(), consp);
    }
}

शून्य addTriggerCosts(AstExecGraph* अचर execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_पङ्क्ति* अचर flp = execGraphp->fileline();
    अचर uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    लागि (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            यदि (अचर AstTriggered* अचर trigp = VN_CAST(ifp->condp(), Triggered)) {
                अचर uint32_t cost = V3_Instr_गन्ती::गन्ती(ifp, असत्य);
                लागि (अचर uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* अचर netlistp = v3Global.rootp();
    AstUnpackArrayDType* अचर dtypep
        = नूतन AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  नूतन AstRange{flp, static_cast<पूर्णाङ्क>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* अचर consp = नूतन AstConsArray{flp, dtypep, नूतन AstConst{flp, 0}};
    लागि (uint32_t index = 0; index < nTriggers; ++index) {
        यदि (costs[index]) consp->valuep(index, नूतन AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_रूख(), consp);
}

// Called by Verilator top stage
शून्य implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->शीर्ष_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    लागि (AstExecGraph* अचर execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        यदि (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_रूख(), execGraphp);
            फर्काऊ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        यदि (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        अचर std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<द्विगुण>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        लागि (अचर ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

शून्य selfTest() {
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
