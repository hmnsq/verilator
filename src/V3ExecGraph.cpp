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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3GraphStream.h"
#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"
#ଅନ୍ତର୍ଭାଵ "V3InstrCount.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ଗଣ୍ଠି_Stmt* stmtsp,
                                 uint32_t id) {
    ଅଚଳ std::string newName = execGraphp->name() + "_mtask" + std::to_string(id);
    AstCFunc* ଅଚଳ newp = ନୂତନ AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(ସତ୍ୟ);
    newp->voidSelf(ସତ୍ୟ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(ସତ୍ୟ);
    newp->addStmtsp(stmtsp);
    ଯଦି (scopep) scopep->addBlocksp(newp);
    ଫେରନ୍ତୁ newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_ଗଣ୍ଠି_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_nextId++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).toString()} {}

ଶୂନ୍ୟ ExecMTask::dump(std::ostream& str) ଅଚଳ {
    str << name() << "." << cvtToHex(ଏହି);
    ଯଦି (priority() || cost()) str << " [pr=" << priority() << " c=" << cvtToStr(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_nextId{0};

ନାମାକାଶ V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
ଶ୍ରେଣୀ ThreadSchedule final {
    friend ଶ୍ରେଣୀ PackThreads;

    uint32_t m_id;  // Unique ID of a schedule
    ସ୍ଥିର uint32_t s_nextId;  // Next ID number to use
    std::unordered_set<ଅଚଳ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_endTime = 0;  // Latest task end time in this schedule

ସର୍ବସାଧାରଣ:
    // CONSTANTS
    ସ୍ଥିର constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ସଂରଚନା MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ଅଚଳ ExecMTask* nextp = nullptr;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::vector<std::vector<ଅଚଳ ExecMTask*>> m_threads;

    // Global state for each mtask.
    ସ୍ଥିର std::unordered_map<ଅଚଳ ExecMTask*, MTaskState> s_mtaskState;

    explicit ThreadSchedule(uint32_t nThreads)
        : m_id{s_nextId++}
        , m_threads{nThreads} {}
    ThreadSchedule(ThreadSchedule&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ThreadSchedule& operator=(ThreadSchedule&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ବ୍ୟକ୍ତିଗତ:
    VL_UNCOPYABLE(ThreadSchedule);

    ସ୍ଥିର constexpr ଦ୍ୱିଗୁଣ s_threadBoxWidth = 2.5;
    ସ୍ଥିର constexpr ଦ୍ୱିଗୁଣ s_threadBoxHeight = 1.5;
    ସ୍ଥିର constexpr ଦ୍ୱିଗୁଣ s_horizontalGap = s_threadBoxWidth / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ସ୍ଥିର ଶୂନ୍ୟ dumpDotFilePrefixedAlways(ଅଚଳ std::vector<ThreadSchedule>& schedules,
                                          ଅଚଳ string& nameComment, uint32_t nThreads) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", nThreads);
    }
    ସ୍ଥିର ଶୂନ୍ୟ dumpDotFile(ଅଚଳ std::vector<ThreadSchedule>& schedules, ଅଚଳ string& filename,
                            uint32_t nThreads) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ଅଚଳ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        ଯଦି (logp->fail()) v3fatal("Can't write file: " << filename);

        // Header
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        ପାଇଁ (uint32_t i = 0; i < nThreads; ++i) {
            ଅଚଳ string name = "t" + std::to_string(i);
            ଅଚଳ string label = "Thread " + std::to_string(i);
            constexpr ଦ୍ୱିଗୁଣ posX = -s_horizontalGap;
            ଅଚଳ ଦ୍ୱିଗୁଣ posY = -static_cast<ଦ୍ୱିଗୁଣ>(i) * s_threadBoxHeight;
            dumpDotFileEmitBlock(logp, name, label, s_threadBoxWidth, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        ପାଇଁ (ଅଚଳ auto& state : ThreadSchedule::s_mtaskState) {
            ଅଚଳ ExecMTask* ଅଚଳ mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segmentsPerLongestMtask = 4;
        ଅଚଳ uint32_t segmentCost = maxCost / segmentsPerLongestMtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::vector<ଦ୍ୱିଗୁଣ> offsets(nThreads, 0.0);
        ପାଇଁ (ଅଚଳ ThreadSchedule& schedule : schedules) {
            ଯଦି (schedule.mtasks.empty()) ଜାରି;
            using Column = std::vector<ଅଚଳ ExecMTask*>;
            std::vector<Column> columns = {{}};

            // Order tasks based on their start time
            ସଂରଚନା Cmp final {
                ତାର୍କିକ operator()(ଅଚଳ ExecMTask* ଅଚଳ a, ଅଚଳ ExecMTask* ଅଚଳ b) ଅଚଳ {
                    ଯଦି (startTime(a) == startTime(b)) ଫେରନ୍ତୁ threadId(a) < threadId(b);
                    ଫେରନ୍ତୁ startTime(a) < startTime(b);
                }
            };
            ଅଚଳ std::multiset<ଅଚଳ ExecMTask*, Cmp> tasks(schedule.mtasks.begin(),
                                                             schedule.mtasks.end());

            ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ mtaskp : tasks) {
                Column& column = columns.back();
                UASSERT(column.size() <= nThreads, "Invalid partitioning");

                ତାର୍କିକ intersects = ସତ୍ୟ;
                ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ earlierMtask : column) {
                    ଯଦି (endTime(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= endTime(earlierMtask)) {
                        intersects = ମିଥ୍ୟା;
                        ଭାଙ୍ଗ;
                    }
                }
                ଯଦି (intersects) {
                    column.emplace_back(mtaskp);
                } ଅନ୍ୟଥା {
                    columns.emplace_back(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().empty(), "Should be populated by mtasks");

            ପାଇଁ (ଅଚଳ Column& column : columns) {
                ଦ୍ୱିଗୁଣ lastColumnOffset = 0;
                ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, schedule, segmentCost, offsets);
                    lastColumnOffset = std::max(lastColumnOffset, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.end(), lastColumnOffset);
            }

            dumpDotFileEmitFork(logp, offsets.front(), nThreads);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            ପାଇଁ (ଅଚଳ std::vector<ଅଚଳ ExecMTask*>& thread : schedule.m_threads) {
                ଯଦି (thread.empty()) ଭାଙ୍ଗ;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.back()->name() << " -> fork_"
                      << static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                ପାଇଁ (size_t i = 1; i < thread.size(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ mtaskp : thread) {
                    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : mtaskp->outEdges()) {
                        ଅଚଳ ExecMTask* ଅଚଳ ଶୀର୍ଷ_MTaskp = edge.ଶୀର୍ଷ()->cast<ଅଚଳ ExecMTask>();
                        ଯଦି (ଶୀର୍ଷ_MTaskp && schedule.contains(ଶୀର୍ଷ_MTaskp)
                            && threadId(ଶୀର୍ଷ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << ଶୀର୍ଷ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ସ୍ଥିର ଶୂନ୍ୟ dumpDotFileEmitBlock(ଅଚଳ std::unique_ptr<std::ofstream>& logp,
                                     ଅଚଳ string& name, ଅଚଳ string& label, ଦ୍ୱିଗୁଣ width,
                                     ଦ୍ୱିଗୁଣ height, ଦ୍ୱିଗୁଣ xPos, ଦ୍ୱିଗୁଣ yPos,
                                     ଅଚଳ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << width
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ସ୍ଥିର ଶୂନ୍ୟ dumpDotFileEmitMTask(ଅଚଳ std::unique_ptr<std::ofstream>& logp,
                                     ଅଚଳ ExecMTask* ଅଚଳ mtaskp, ଅଚଳ ThreadSchedule& schedule,
                                     uint32_t segmentCost, std::vector<ଦ୍ୱିଗୁଣ>& offsets) {
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < mtaskp->threads(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ଅଚଳ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::to_string(i);
            ଅଚଳ string label = mtaskp->name() + " (" + std::to_string(startTime(mtaskp)) + ':'
                                 + std::to_string(endTime(mtaskp)) + ')'
                                 + "\\ncost=" + std::to_string(mtaskp->cost())
                                 + "\\npriority=" + std::to_string(mtaskp->priority());
            ଅଚଳ ଦ୍ୱିଗୁଣ width
                = std::max(s_threadBoxWidth,
                           s_threadBoxWidth * static_cast<ଦ୍ୱିଗୁଣ>(mtaskp->cost()) / segmentCost);
            ଅଚଳ uint32_t mtaskThreadId = threadId(mtaskp) + i * schedule.m_threads.size();
            ଅଚଳ ଦ୍ୱିଗୁଣ xPos = width / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += width + s_horizontalGap;
            ଅଚଳ ଦ୍ୱିଗୁଣ yPos = -s_threadBoxHeight * static_cast<ଦ୍ୱିଗୁଣ>(mtaskThreadId);
            ଅଚଳ string fillColor = i == 0 ? "white" : "lightgreen";
            dumpDotFileEmitBlock(logp, name, label, width, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ dumpDotFileEmitFork(ଅଚଳ std::unique_ptr<std::ofstream>& logp, ଦ୍ୱିଗୁଣ offset,
                                    uint32_t nThreads) {
        ଅଚଳ string& name = "fork_" + std::to_string(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(offset));
        constexpr ଦ୍ୱିଗୁଣ width = s_threadBoxWidth / 8;
        ଅଚଳ ଦ୍ୱିଗୁଣ height = s_threadBoxHeight * nThreads;
        ଅଚଳ ଦ୍ୱିଗୁଣ xPos = offset - s_horizontalGap / 2;
        ଅଚଳ ଦ୍ୱିଗୁଣ yPos
            = -static_cast<ଦ୍ୱିଗୁଣ>(nThreads) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dumpDotFileEmitBlock(logp, name, "", width, height, xPos, yPos, "black");
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର uint32_t threadId(ଅଚଳ ExecMTask* mtaskp) {
        ଅଚଳ auto& it = s_mtaskState.find(mtaskp);
        ଫେରନ୍ତୁ it != s_mtaskState.end() ? it->second.threadId : UNASSIGNED;
    }
    ସ୍ଥିର uint32_t startTime(ଅଚଳ ExecMTask* mtaskp) {
        ଫେରନ୍ତୁ s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    ସ୍ଥିର uint32_t endTime(ଅଚଳ ExecMTask* mtaskp) {
        ଫେରନ୍ତୁ s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ଅଚଳ ExecMTask* mtaskp) ଅଚଳ {
        ଅଚଳ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t result = 0;
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : mtaskp->inEdges()) {
            ଅଚଳ ExecMTask* ଅଚଳ prevp = edge.fromp()->as<ExecMTask>();
            ଯଦି (threadId(prevp) != thisThreadId && contains(prevp)) ++result;
        }
        ଫେରନ୍ତୁ result;
    }

    uint32_t id() ଅଚଳ { ଫେରନ୍ତୁ m_id; }
    uint32_t scheduleOn(ଅଚଳ ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ଅଚଳ uint32_t bestEndTime = mtaskp->predictStart() + mtaskp->cost();
        m_endTime = std::max(m_endTime, bestEndTime);
        s_mtaskState[mtaskp].completionTime = bestEndTime;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::vector<ଅଚଳ ExecMTask*>& bestThread = m_threads[bestThreadId];
        ଯଦି (!bestThread.empty()) s_mtaskState[bestThread.back()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.ଠେଲ_back(mtaskp);
        ଫେରନ୍ତୁ bestEndTime;
    }
    ତାର୍କିକ contains(ଅଚଳ ExecMTask* mtaskp) ଅଚଳ { ଫେରନ୍ତୁ mtasks.ଗଣନା_2(mtaskp); }
    uint32_t endTime() ଅଚଳ { ଫେରନ୍ତୁ m_endTime; }
};

uint32_t ThreadSchedule::s_nextId = 0;
std::unordered_map<ଅଚଳ ExecMTask*, ThreadSchedule::MTaskState> ThreadSchedule::s_mtaskState{};
constexpr ଦ୍ୱିଗୁଣ V3ExecGraph::ThreadSchedule::s_threadBoxWidth;

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
ଶ୍ରେଣୀ PackThreads final {
    // TYPES
    ସଂରଚନା MTaskCmp final {
        ତାର୍କିକ operator()(ଅଚଳ ExecMTask* ap, ଅଚଳ ExecMTask* bp) ଅଚଳ {
            ଫେରନ୍ତୁ ap->id() < bp->id();
        }
    };

    // MEMBERS
    ଅଚଳ uint32_t m_nThreads;  // Number of threads
    ଅଚଳ uint32_t m_nHierThreads;  // Number of threads used for hierarchical tasks
    ଅଚଳ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ଅଚଳ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    explicit PackThreads(uint32_t nThreads = v3Global.opt.threads(),
                         uint32_t nHierThreads = v3Global.opt.hierThreads(),
                         unsigned sandbagNumerator = 30, unsigned sandbagDenom = 100)
        : m_nThreads{nThreads}
        , m_nHierThreads{nHierThreads}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~PackThreads() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(PackThreads);

    // METHODS
    uint32_t completionTime(ଅଚଳ ThreadSchedule& schedule, ଅଚଳ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        ଯଦି (!schedule.contains(mtaskp)) ଫେରନ୍ତୁ 0;
        ଅଚଳ ThreadSchedule::MTaskState& state = schedule.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != ThreadSchedule::UNASSIGNED, "Mtask should have assigned thread");
        ଯଦି (threadId == state.threadId) {
            // No overhead on same thread
            ଫେରନ୍ତୁ state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbaggedEndTime
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        ଯଦି (state.nextp) {
            ଅଚଳ uint32_t successorEndTime
                = completionTime(schedule, state.nextp, state.threadId);
            ଯଦି ((sandbaggedEndTime >= successorEndTime) && (successorEndTime > 1)) {
                sandbaggedEndTime = successorEndTime - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbaggedEndTime);
        ଫେରନ୍ତୁ sandbaggedEndTime;
    }

    ସ୍ଥିର ତାର୍କିକ isReady(ThreadSchedule& schedule, ଅଚଳ ExecMTask* mtaskp) {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edgeIn : mtaskp->inEdges()) {
            ଅଚଳ ExecMTask* ଅଚଳ prevp = edgeIn.fromp()->as<ଅଚଳ ExecMTask>();
            ଯଦି (schedule.threadId(prevp) == ThreadSchedule::UNASSIGNED) {
                // This predecessor is not assigned yet
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
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
        ଗଣନା ଶ୍ରେଣୀ SchedulingMode : uint8_t {
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
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxThreadWorkers = 1;

        // Build initial ready list
        ପାଇଁ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ଅଚଳ mtaskp = vtx.as<ExecMTask>();
            ଯଦି (isReady(result.back(), mtaskp)) readyMTasks.ଭର୍ତ୍ତି(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->threads());
        }

        ଯେତେବେଳେ (!readyMTasks.empty()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = nullptr;  // Todo: const ExecMTask*
            ThreadSchedule& schedule = result.back();
            ପାଇଁ (uint32_t threadId = 0; threadId < schedule.m_threads.size(); ++threadId) {
                ପାଇଁ (ExecMTask* ଅଚଳ mtaskp : readyMTasks) {
                    ଯଦି (mode != SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() > 1) {
                        mode = SchedulingMode::WIDE_TASK_DISCOVERED;
                        ଜାରି;
                    }
                    ଯଦି (mode == SchedulingMode::WIDE_TASK_SCHEDULING && mtaskp->threads() <= 1)
                        ଜାରି;

                    uint32_t timeBegin = busyUntil[threadId];
                    ଯଦି (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        ଭାଙ୍ଗ;
                    }
                    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : mtaskp->inEdges()) {
                        ଅଚଳ ExecMTask* ଅଚଳ priorp = edge.fromp()->as<ExecMTask>();
                        ଅଚଳ uint32_t priorEndTime = completionTime(schedule, priorp, threadId);
                        ଯଦି (priorEndTime > timeBegin) timeBegin = priorEndTime;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    ଯଦି ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->priority() > bestMtaskp->priority()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            ଅଚଳ uint32_t endTime = schedule.endTime();

            ଯଦି (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_DISCOVERED) {
                mode = SchedulingMode::WIDE_TASK_SCHEDULING;
                ଅଚଳ uint32_t size = m_nHierThreads / maxThreadWorkers;
                UASSERT(size, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                ଯଦି (schedule.s_mtaskState.empty()) result.clear();
                result.emplace_back(ThreadSchedule{size});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ଜାରି;
            }

            ଯଦି (!bestMtaskp && mode == SchedulingMode::WIDE_TASK_SCHEDULING) {
                mode = SchedulingMode::SCHEDULING;
                UASSERT(!schedule.s_mtaskState.empty(), "Mtask should be added");
                result.emplace_back(ThreadSchedule{m_nThreads});
                std::fill(busyUntil.begin(), busyUntil.end(), endTime);
                ଜାରି;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ଅଚଳ uint32_t bestEndTime = schedule.scheduleOn(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = bestEndTime;

            // Update the ready list
            ଅଚଳ size_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            ପାଇଁ (ଅଚଳ V3GraphEdge& edgeOut : bestMtaskp->outEdges()) {
                ExecMTask* ଅଚଳ nextp = edgeOut.ଶୀର୍ଷ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(schedule.threadId(nextp) == ThreadSchedule::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.end(), nextp,
                            "Tasks after one being assigned should not be ready");
                ଯଦି (isReady(schedule, nextp)) {
                    readyMTasks.ଭର୍ତ୍ତି(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        ଯଦି (dumpGraphLevel() >= 4)
            ThreadSchedule::dumpDotFilePrefixedAlways(result, "schedule", m_nThreads);

        ଫେରନ୍ତୁ result;
    }

ସର୍ବସାଧାରଣ:
    // SELF TEST
    ସ୍ଥିର ଶୂନ୍ୟ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ଅଚଳ scopep = ନୂତନ AstScope{flp, nullptr, "test", nullptr, nullptr};
        AstVarScope* ଅଚଳ trigVscp = ନୂତନ AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        selfTestNormalFirst(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_ବୃକ୍ଷ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_ବୃକ୍ଷ(), scopep);
        VL_DO_DANGLING(varp->delete_ବୃକ୍ଷ(), varp);
    }
    ସ୍ଥିର ଶୂନ୍ୟ selfTestNormalFirst(AstVarScope* trigVscp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
        AstExecGraph* ଅଚଳ execGraphp = ନୂତନ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ଅଚଳ auto makeBody = [&]() -> Ast_ଗଣ୍ଠି_Stmt* { ଫେରନ୍ତୁ ନୂତନ AstComment{flp, ""}; };
        ExecMTask* ଅଚଳ t0 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        ExecMTask* ଅଚଳ t1 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);
        ExecMTask* ଅଚଳ t2 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t2->cost(100);
        t2->priority(100);
        t2->threads(2);
        ExecMTask* ଅଚଳ t3 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t3->cost(100);
        t3->priority(100);
        t3->threads(3);
        ExecMTask* ଅଚଳ t4 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t4->cost(100);
        t4->priority(100);
        t4->threads(3);
        ExecMTask* ଅଚଳ t5 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t5->cost(100);
        t5->priority(100);
        ExecMTask* ଅଚଳ t6 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
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
        ନୂତନ V3GraphEdge{&graph, t0, t1, 1};
        ନୂତନ V3GraphEdge{&graph, t0, t2, 1};
        ନୂତନ V3GraphEdge{&graph, t2, t3, 1};
        ନୂତନ V3GraphEdge{&graph, t2, t4, 1};
        ନୂତନ V3GraphEdge{&graph, t3, t5, 1};
        ନୂତନ V3GraphEdge{&graph, t4, t6, 1};

        constexpr uint32_t threads = 2;
        constexpr uint32_t hierThreads = 6;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ଅଚଳ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 3);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 2);
        ପାଇଁ (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
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

        ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ବୃକ୍ଷ();
        VL_DO_DANGLING(execGraphp->delete_ବୃକ୍ଷ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }
    ସ୍ଥିର ଶୂନ୍ୟ selfTestHierFirst(AstVarScope* trigVscp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
        AstExecGraph* ଅଚଳ execGraphp = ନୂତନ AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ଅଚଳ auto makeBody = [&]() -> Ast_ଗଣ୍ଠି_Stmt* { ଫେରନ୍ତୁ ନୂତନ AstComment{flp, ""}; };
        ExecMTask* ଅଚଳ t0 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t0->cost(1000);
        t0->priority(1100);
        t0->threads(2);
        ExecMTask* ଅଚଳ t1 = ନୂତନ ExecMTask{execGraphp, nullptr, makeBody()};
        t1->cost(100);
        t1->priority(100);

        /*
                          0
                          |
                          1
        */
        ନୂତନ V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t threads = 1;
        constexpr uint32_t hierThreads = 2;
        PackThreads packer{threads, hierThreads,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ଅଚଳ std::vector<ThreadSchedule> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.size(), 2);
        UASSERT_SELFTEST(scheduled[0].m_threads.size(), hierThreads / 2);
        UASSERT_SELFTEST(scheduled[0].m_threads[0].size(), 1);
        ପାଇଁ (size_t i = 1; i < scheduled[0].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_threads[i].size(), 0);

        UASSERT_SELFTEST(scheduled[0].m_threads[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_threads.size(), threads);
        UASSERT_SELFTEST(scheduled[1].m_threads[0].size(), 1);
        ପାଇଁ (size_t i = 1; i < scheduled[1].m_threads.size(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_threads[i].size(), 0);
        UASSERT_SELFTEST(scheduled[1].m_threads[0][0], t1);

        UASSERT_SELFTEST(ThreadSchedule::s_mtaskState.size(), 2);

        UASSERT_SELFTEST(ThreadSchedule::threadId(t0), 0);
        UASSERT_SELFTEST(ThreadSchedule::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ବୃକ୍ଷ();
        VL_DO_DANGLING(execGraphp->delete_ବୃକ୍ଷ(), execGraphp);
        ThreadSchedule::s_mtaskState.clear();
    }

    ସ୍ଥିର std::vector<ThreadSchedule> apply(V3Graph& mtaskGraph) {
        ଫେରନ୍ତୁ PackThreads{}.pack(mtaskGraph);
    }
};

using EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
using Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

ଶୂନ୍ୟ normalizeCosts(Costs& costs) {
    ଅଚଳ auto scaleCost = [](uint64_t ମୂଲ୍ୟ, ଦ୍ୱିଗୁଣ multiplier) {
        ଦ୍ୱିଗୁଣ scaled = static_cast<ଦ୍ୱିଗୁଣ>(ମୂଲ୍ୟ) * multiplier;
        ଯଦି (ମୂଲ୍ୟ && scaled < 1) scaled = 1;
        ଫେରନ୍ତୁ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t sumCostProfiled = 0;  // For data with estimate and profile
    uint64_t sumCostEstimate = 0;  // For data with estimate and profile
    ପାଇଁ (ଅଚଳ auto& est : costs) {
        ଯଦି (est.second.second) {
            sumCostEstimate += est.second.first;
            sumCostProfiled += est.second.second;
        }
    }

    ଯଦି (sumCostEstimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ଅଚଳ ଦ୍ୱିଗୁଣ estToProfile
            = static_cast<ଦ୍ୱିଗୁଣ>(sumCostProfiled) / static_cast<ଦ୍ୱିଗୁଣ>(sumCostEstimate);
        UINFO(5, "Estimated data needs scaling by " << estToProfile
                                                    << ", sumCostProfiled=" << sumCostProfiled
                                                    << " sumCostEstimate=" << sumCostEstimate);
        ପାଇଁ (auto& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, estToProfile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    ପାଇଁ (auto& est : costs) {
        ଅଚଳ uint64_t& costEstimate = est.second.first;
        ଅଚଳ uint64_t& costProfiled = est.second.second;
        ଯଦି (maxCost < costEstimate) maxCost = costEstimate;
        ଯଦି (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scaleDownTo = 10000000;  // Extra room for future algorithms to add costs
    ଯଦି (maxCost > scaleDownTo) {
        ଅଚଳ ଦ୍ୱିଗୁଣ scaleup = static_cast<ଦ୍ୱିଗୁଣ>(scaleDownTo) / static_cast<ଦ୍ୱିଗୁଣ>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        ପାଇଁ (auto& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

ଶୂନ୍ୟ removeEmptyMTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    ପାଇଁ (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        ଯଦି (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlinkFrBack()->delete_ବୃକ୍ଷ(), callp);
    }
    V3Graph* ଅଚଳ execMTaskGraphp = execGraphp->depGraphp();
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ଅଚଳ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ଅଚଳ funcp = mtaskp->funcp();
        ଯଦି (funcp->stmtsp()) ଜାରି;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->rerouteEdges(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlinkFrBack()->delete_ବୃକ୍ଷ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
}

ଶୂନ୍ୟ fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    ପାଇଁ (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ଅଚଳ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ଅଚଳ uint64_t costEstimate = V3_Instr_ଗଣନା::ଗଣନା_2(mtp->funcp(), ମିଥ୍ୟା);
        ଅଚଳ uint64_t costProfiled
            = V3Control::getProfileData(v3Global.opt.prefix(), mtp->hashName());
        ଯଦି (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା totalEstimates = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା missingProfiles = 0;
    ପାଇଁ (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ଅଚଳ mtp = vtx.as<ExecMTask>();
        ଅଚଳ uint32_t costEstimate = costs[mtp->id()].first;
        ଅଚଳ uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ଅଚଳ uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t costToUse = costProfiled32;
        ଯଦି (!costProfiled32) {
            costToUse = costEstimate;
            ଯଦି (costEstimate != 0) ++missingProfiles;
        }
        ଯଦି (costEstimate != 0) ++totalEstimates;
        mtp->cost(costToUse);
        mtp->priority(costToUse);
    }

    ଯଦି (missingProfiles) {
        ଯଦି (File_ଧାଡ଼ି* ଅଚଳ fl = V3Control::get_Profile_Data_File_ଧାଡ଼ି()) {
            ଯଦି (V3Control::containsMTaskProfileData()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << totalEstimates
                                              << " mtasks had no data");
            }
        }
    }
}

ଶୂନ୍ୟ finalizeCosts(V3Graph* execMTaskGraphp) {
    GraphStreamUnordered ser(execMTaskGraphp, GraphWay::REVERSE);
    ଯେତେବେଳେ (ଅଚଳ V3GraphVertex* ଅଚଳ vxp = ser.nextp()) {
        ExecMTask* ଅଚଳ mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        ପାଇଁ (V3GraphEdge& edge : mtp->outEdges()) {
            ଅଚଳ ExecMTask* ଅଚଳ followp = edge.ଶୀର୍ଷ()->as<ExecMTask>();
            ଯଦି ((followp->priority() + mtp->cost()) > mtp->priority()) {
                mtp->priority(followp->priority() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->removeTransitiveEdges();

    // Record summary stats for final m_tasks graph.
    ଅଚଳ auto report = execMTaskGraphp->parallelismReport([](ଅଚଳ V3GraphVertex* vtxp) {  //
        ଫେରନ୍ତୁ vtxp->as<ଅଚଳ ExecMTask>()->cost();
    });
    V3Stats::addStat("MTask graph, final, critical path cost", report.criticalPathCost());
    V3Stats::addStat("MTask graph, final, total graph cost", report.totalGraphCost());
    V3Stats::addStat("MTask graph, final, mtask count", report.vertex_ଗଣନା());
    V3Stats::addStat("MTask graph, final, edge count", report.edge_ଗଣନା());
    V3Stats::addStat("MTask graph, final, parallelism factor", report.parallelismFactor());
    ଯଦି (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.criticalPathCost());
        UINFO(0, "    Total graph cost = " << report.totalGraphCost());
        UINFO(0, "    MTask vertex count = " << report.vertex_ଗଣନା());
        UINFO(0, "    Edge count = " << report.edge_ଗଣନା());
        UINFO(0, "    Parallelism factor = " << report.parallelismFactor());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ଅଚଳ std::string& name, uint32_t nDependencies) {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
    File_ଧାଡ଼ି* ଅଚଳ fl = modp->fileline();
    AstBasicDType* ଅଚଳ dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(ସତ୍ୟ);
    varp->valuep(ନୂତନ AstConst{fl, nDependencies});
    modp->addStmtsp(varp);
    ଫେରନ୍ତୁ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ଅଚଳ std::string& method) {
    File_ଧାଡ଼ି* ଅଚଳ fl = varp->fileline();
    AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ଅଚଳ AstScope* ଅଚଳ ଶୀର୍ଷ_Scopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, ଶୀର୍ଷ_Scopep->nameDotless()});
    AstCStmt* ଅଚଳ stmtp = ନୂତନ AstCStmt{fl};
    stmtp->add(refp);
    stmtp->add("." + method + ";");
    ଫେରନ୍ତୁ stmtp;
}

ଶୂନ୍ୟ addMTaskToFunction(ଅଚଳ ThreadSchedule& schedule, ଅଚଳ uint32_t threadId, AstCFunc* funcp,
                        ଅଚଳ ExecMTask* mtaskp,
                        ଅଚଳ std::unordered_map<ଅଚଳ ExecMTask*, AstVar*>& stateVars) {
    AstScope* ଅଚଳ scopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
    File_ଧାଡ଼ି* ଅଚଳ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ଅଚଳ auto addCStmt = [=](ଅଚଳ string& stmt) -> ଶୂନ୍ୟ {  //
        funcp->addStmtsp(ନୂତନ AstCStmt{fl, stmt});
    };

    ଯଦି (schedule.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        ଯଦି (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->addStmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        ଯଦି (v3Global.opt.profExec()) {
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    ଯଦି (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtypeSetVoid();
    funcp->addStmtsp(callp->makeStmt());

    ଯଦି (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        addCStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::to_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : mtaskp->outEdges()) {
        ଅଚଳ ExecMTask* ଅଚଳ nextp = edge.ଶୀର୍ଷ()->as<ExecMTask>();
        ଯଦି (schedule.threadId(nextp) != threadId && schedule.contains(nextp)) {
            funcp->addStmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ଅଚଳ std::vector<AstCFunc*> createThreadFunctions(ଅଚଳ ThreadSchedule& schedule,
                                                   ଅଚଳ string& tag) {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
    File_ଧାଡ଼ି* ଅଚଳ fl = modp->fileline();

    std::vector<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ଅଚଳ ExecMTask*, AstVar*> stateVars;
    ପାଇଁ (ଅଚଳ std::vector<ଅଚଳ ExecMTask*>& thread : schedule.m_threads) {
        ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ mtaskp : thread) {
            ଯଦି (ଅଚଳ uint32_t nDependencies = schedule.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvtToStr(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    ପାଇଁ (ଅଚଳ std::vector<ଅଚଳ ExecMTask*>& thread : schedule.m_threads) {
        ଯଦି (thread.empty()) ଜାରି;
        ଅଚଳ uint32_t threadId = schedule.threadId(thread.front());
        ଅଚଳ string name{"__Vthread__" + tag + "__s" + cvtToStr(schedule.id()) + "__t"
                          + cvtToStr(threadId)};
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{fl, name, nullptr, "void"};
        modp->addStmtsp(funcp);
        funcps.ଠେଲ_back(funcp);
        funcp->isStatic(ସତ୍ୟ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(ସତ୍ୟ);
        funcp->entryPoint(ସତ୍ୟ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->addStmtsp(ନୂତନ AstCStmt{fl, EmitCUtil::voidSelfAssign(modp)});
        funcp->addStmtsp(ନୂତନ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Invoke each mtask scheduled to this thread from the thread function
        ପାଇଁ (ଅଚଳ ExecMTask* ଅଚଳ mtaskp : thread) {
            addMTaskToFunction(schedule, threadId, funcp, mtaskp, stateVars);
        }
    }

    ଫେରନ୍ତୁ funcps;
}

ଶୂନ୍ୟ addThreadStartWrapper(AstExecGraph* ଅଚଳ execGraphp) {
    // FileLine used for constructing nodes below
    File_ଧାଡ଼ି* ଅଚଳ fl = v3Global.rootp()->fileline();
    ଅଚଳ string& tag = execGraphp->name();

    // Add thread function invocations to execGraph
    ଅଚଳ auto addCStmt = [=](ଅଚଳ string& stmt) -> ଶୂନ୍ୟ {  //
        execGraphp->runfuncp()->addStmtsp(ନୂତନ AstCStmt{fl, stmt});
    };

    ଯଦି (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    addCStmt("vlSymsp->__Vm_even_cycle__" + tag + " = !vlSymsp->__Vm_even_cycle__" + tag + ";");

    ଯଦି (!v3Global.opt.hierBlocks().empty()) addCStmt("std::vector<size_t> indexes;");
}

ଶୂନ୍ୟ addThreadEndWrapper(AstExecGraph* ଅଚଳ execGraphp) {
    // Add thread function invocations to execGraph
    ଅଚଳ auto addCStmt = [=](ଅଚଳ string& stmt) -> ଶୂନ୍ୟ {  //
        File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->addStmtsp(ନୂତନ AstCStmt{flp, stmt});
    };

    addCStmt("Verilated::mtaskId(0);");
    ଯଦି (v3Global.opt.profExec()) { addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
ଶୂନ୍ୟ addThreadStartToExecGraph(AstExecGraph* ଅଚଳ execGraphp,
                               ଅଚଳ std::vector<AstCFunc*>& funcps, uint32_t scheduleId) {
    // FileLine used for constructing nodes below
    File_ଧାଡ଼ି* ଅଚଳ fl = v3Global.rootp()->fileline();
    ଅଚଳ string& tag = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ଅଚଳ finalVarp
        = newMTaskStateVar("final__" + cvtToStr(scheduleId) + "__" + tag, funcps.size());
    ପାଇଁ (AstCFunc* ଅଚଳ funcp : funcps) {
        funcp->addStmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ଅଚଳ auto addCStmt = [=](ଅଚଳ string& stmt) -> ଶୂନ୍ୟ {  //
        execGraphp->runfuncp()->addStmtsp(ନୂତନ AstCStmt{fl, stmt});
    };

    ଅଚଳ uint32_t last = funcps.size() - 1;
    ଯଦି (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("for (size_t i = 0; i < " + std::to_string(last) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    ପାଇଁ (AstCFunc* ଅଚଳ funcp : funcps) {
        ଯଦି (i != last) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{fl};
            execGraphp->runfuncp()->addStmtsp(cstmtp);
            cstmtp->add("vlSymsp->__Vm_threadPoolp->workerp(");
            ଯଦି (v3Global.opt.hierChild() || !v3Global.opt.hierBlocks().empty()) {
                cstmtp->add("indexes[" + std::to_string(i) + "]");
            } ଅନ୍ୟଥା {
                cstmtp->add(std::to_string(i));
            }
            cstmtp->add(")->addTask(");
            cstmtp->add(ନୂତନ AstAddrOfCFunc{fl, funcp});
            cstmtp->add(", vlSelf, vlSymsp->__Vm_even_cycle__" + tag + ");");
        } ଅନ୍ୟଥା {
            // The last will run on the main thread.
            AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{fl, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + tag);
            execGraphp->runfuncp()->addStmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::addStatSum("Optimizations, Thread schedule total tasks", i);

    ଯଦି (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->addStmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + tag + ")"));
    ଯଦି (v3Global.opt.profExec()) {
        addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    ଯଦି (!v3Global.opt.hierBlocks().empty() && last > 0) {
        addCStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

ଶୂନ୍ୟ processMTaskBodies(AstExecGraph* ଅଚଳ execGraphp) {
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ଅଚଳ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ଅଚଳ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ଗଣ୍ଠି* stmtsp = funcp->stmtsp()->unlinkFrBackWithNext();

        // Helper function to make the code a bit more legible
        ଅଚଳ auto addCStmt = [=](ଅଚଳ string& stmt) -> ଶୂନ୍ୟ {  //
            funcp->addStmtsp(ନୂତନ AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        ଯଦି (v3Global.opt.profExec()) {
            std::string args = std::to_string(mtaskp->id());
            args += ", " + std::to_string(mtaskp->predictStart());
            args += ", \"";
            ଯଦି (v3Global.opt.hierChild()) args += v3Global.opt.ଶୀର୍ଷ_Module();
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
        ଯଦି (v3Global.opt.profExec()) {
            ଅଚଳ std::string& args = std::to_string(mtaskp->cost());
            addCStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + args + ");");
        }
    }
}

ଶୂନ୍ୟ implementExecGraph(AstExecGraph* ଅଚଳ execGraphp, ଅଚଳ ThreadSchedule& schedule) {
    // Nothing to be done if there are no MTasks in the graph at all.
    ଯଦି (execGraphp->depGraphp()->empty()) ଫେରନ୍ତୁ;

    // Create a function to be run by each thread.
    ଅଚଳ std::vector<AstCFunc*>& funcps = createThreadFunctions(schedule, execGraphp->name());
    UASSERT(!funcps.empty(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    addThreadStartToExecGraph(execGraphp, funcps, schedule.id());
}

ଶୂନ୍ୟ createRunFunction(AstExecGraph* ଅଚଳ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_ଧାଡ଼ି* ଅଚଳ flp = execGraphp->fileline();
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = v3Global.rootp()->ଶୀର୍ଷ_Modulep();
    AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{flp, "runExecGraph_" + execGraphp->name(), nullptr};
    funcp->isLoose(ସତ୍ୟ);
    funcp->voidSelf(ସତ୍ୟ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(ସତ୍ୟ);
    modp->addStmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

ଶୂନ୍ୟ addTriggerMasks(AstExecGraph* ଅଚଳ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_ଧାଡ଼ି* ଅଚଳ flp = execGraphp->fileline();
    AstVar* ଅଚଳ triggersp = execGraphp->triggerp()->varp();
    ପାଇଁ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ଅଚଳ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            ପାଇଁ (ଅଚଳ uint32_t index : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(index)] |= VL_BIT_Q(index);
            }
        });
        AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{flp, triggersp->dtypep(),
                                                     ନୂତନ AstConst{flp, AstConst::Unsized64{}, 0}};
        ପାଇଁ (ଅଚଳ auto& wordMask : wordMasks) {
            consp->valuep(wordMask.first,
                          ନୂତନ AstConst{flp, AstConst::Unsized64{}, wordMask.second});
        }
        execGraphp->addMaskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_ବୃକ୍ଷ(), consp);
    }
}

ଶୂନ୍ୟ addTriggerCosts(AstExecGraph* ଅଚଳ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_ଧାଡ଼ି* ଅଚଳ flp = execGraphp->fileline();
    ଅଚଳ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::vector<uint32_t> costs(nTriggers, 0);
    ପାଇଁ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            ଯଦି (ଅଚଳ AstTriggered* ଅଚଳ trigp = VN_CAST(ifp->condp(), Triggered)) {
                ଅଚଳ uint32_t cost = V3_Instr_ଗଣନା::ଗଣନା_2(ifp, ମିଥ୍ୟା);
                ପାଇଁ (ଅଚଳ uint32_t index : trigp->indices()) costs.at(index) += cost;
            }
        });
    }
    AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
    AstUnpackArrayDType* ଅଚଳ dtypep
        = ନୂତନ AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  ନୂତନ AstRange{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nTriggers - 1), 0}};
    netlistp->typeTablep()->addTypesp(dtypep);
    AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{flp, dtypep, ନୂତନ AstConst{flp, 0}};
    ପାଇଁ (uint32_t index = 0; index < nTriggers; ++index) {
        ଯଦି (costs[index]) consp->valuep(index, ନୂତନ AstConst{flp, costs[index]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_ବୃକ୍ଷ(), consp);
}

// Called by Verilator top stage
ଶୂନ୍ୟ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::vector<AstExecGraph*> execGraphps;
    netlistp->ଶୀର୍ଷ_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_back(egp); });

    // Process each
    ପାଇଁ (AstExecGraph* ଅଚଳ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        removeEmptyMTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        ଯଦି (execGraphp->depGraphp()->empty()) {
            VL_DO_DANGLING(execGraphp->unlinkFrBack()->delete_ବୃକ୍ଷ(), execGraphp);
            ଫେରନ୍ତୁ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        ଯଦି (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        createRunFunction(execGraphp);
        addThreadStartWrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ଅଚଳ std::vector<ThreadSchedule> packed = PackThreads::apply(*execGraphp->depGraphp());
        V3Stats::addStatSum("Optimizations, Thread schedule count",
                            static_cast<ଦ୍ୱିଗୁଣ>(packed.size()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        ପାଇଁ (ଅଚଳ ThreadSchedule& schedule : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, schedule);
        }

        addThreadEndWrapper(execGraphp);

        addTriggerMasks(execGraphp);
        addTriggerCosts(execGraphp);
    }
}

ଶୂନ୍ୟ selfTest() {
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
