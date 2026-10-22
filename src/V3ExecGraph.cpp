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

AstCFunc* ExecMTask::createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ग्रन्थिः_Stmt* stmtsp,
                                 uint32_t id) {
    ध्रुव std::string newName = execGraphp->name() + "_mtask" + std::गन्तव्यम्_string(id);
    AstCFunc* ध्रुव newp = नूतन AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(सत्य);
    newp->voidSelf(सत्य);  // Called through VlExecGraph::Fnp
    newp->dontCombine(सत्य);
    newp->सङ्कलय_Stmtsp(stmtsp);
    यदि (scopep) scopep->सङ्कलय_Blocksp(newp);
    प्रत्यागम newp;
}

ExecMTask::ExecMTask(AstExecGraph* execGraphp, AstScope* scopep,
                     Ast_ग्रन्थिः_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3GraphVertex{execGraphp->depGraphp()},
      m_id{s_अग्र_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).गन्तव्यम्_String()} {}

शून्य ExecMTask::dump(std::ostream& str) ध्रुव {
    str << name() << "." << cvt_गन्तव्यम्_Hex(एतत्);
    यदि (प्राथमिकता_2() || cost()) str << " [pr=" << प्राथमिकता_2() << " c=" << cvt_गन्तव्यम्_Str(cost()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_अग्र_Id{0};

नामाकाश V3ExecGraph {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
वर्ग Thread_कालसारणी final {
    मित्रम् वर्ग Pack_तन्तवः;

    uint32_t m_id;  // Unique ID of a schedule
    स्थिर uint32_t s_अग्र_Id;  // Next ID number to use
    std::unordered_समुच्चयः<ध्रुव ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_अन्तः_Time = 0;  // Latest task end time in this schedule

सार्वजनिक:
    // CONSTANTS
    स्थिर constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    संरचना MTaskState final {
        uint32_t completionTime = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ध्रुव ExecMTask* nextp = शून्यसूचकः;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::सदिशम्<std::सदिशम्<ध्रुव ExecMTask*>> m_तन्तवः;

    // Global state for each mtask.
    स्थिर std::unordered_map<ध्रुव ExecMTask*, MTaskState> s_mtaskState;

    स्पष्ट Thread_कालसारणी(uint32_t n_तन्तवः)
        : m_id{s_अग्र_Id++}
        , m_तन्तवः{n_तन्तवः} {}
    Thread_कालसारणी(Thread_कालसारणी&&) = स्वभाव;
    Thread_कालसारणी& संक्रियकः=(Thread_कालसारणी&&) = स्वभाव;

गुप्त:
    VL_UNCOPYABLE(Thread_कालसारणी);

    स्थिर constexpr द्विगुण s_thread_Box_विस्तारः = 2.5;
    स्थिर constexpr द्विगुण s_threadBoxHeight = 1.5;
    स्थिर constexpr द्विगुण s_horizontalGap = s_thread_Box_विस्तारः / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    स्थिर शून्य dumpDotFilePrefixedAlways(ध्रुव std::सदिशम्<Thread_कालसारणी>& schedules,
                                          ध्रुव string& nameComment, uint32_t n_तन्तवः) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_तन्तवः);
    }
    स्थिर शून्य dumpDotFile(ध्रुव std::सदिशम्<Thread_कालसारणी>& schedules, ध्रुव string& filename,
                            uint32_t n_तन्तवः) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ध्रुव std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        यदि (logp->fail()) v3fatal("Can't write file: " << filename);

        //शीर्षकम्
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        प्रति (uint32_t i = 0; i < n_तन्तवः; ++i) {
            ध्रुव string name = "t" + std::गन्तव्यम्_string(i);
            ध्रुव string label = "Thread " + std::गन्तव्यम्_string(i);
            constexpr द्विगुण posX = -s_horizontalGap;
            ध्रुव द्विगुण posY = -static_cast<द्विगुण>(i) * s_threadBoxHeight;
            dump_Dot_File_Emit_खण्डम्(logp, name, label, s_thread_Box_विस्तारः, s_threadBoxHeight, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t maxCost = 0;
        प्रति (ध्रुव स्वचालित& state : Thread_कालसारणी::s_mtaskState) {
            ध्रुव ExecMTask* ध्रुव mtaskp = state.first;
            maxCost = std::max(maxCost, mtaskp->cost());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_प्रत्येकम्_Longest_Mtask = 4;
        ध्रुव uint32_t segmentCost = maxCost / segments_प्रत्येकम्_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::सदिशम्<द्विगुण> offsets(n_तन्तवः, 0.0);
        प्रति (ध्रुव Thread_कालसारणी& कालसारणी : schedules) {
            यदि (कालसारणी.mtasks.रिक्तानि()) अनुवर्त;
            उपयुज्य Column = std::सदिशम्<ध्रुव ExecMTask*>;
            std::सदिशम्<Column> columns = {{}};

            // Order tasks based on their start time
            संरचना Cmp final {
                तार्किक संक्रियकः()(ध्रुव ExecMTask* ध्रुव a, ध्रुव ExecMTask* ध्रुव b) ध्रुव {
                    यदि (startTime(a) == startTime(b)) प्रत्यागम threadId(a) < threadId(b);
                    प्रत्यागम startTime(a) < startTime(b);
                }
            };
            ध्रुव std::multiset<ध्रुव ExecMTask*, Cmp> tasks(कालसारणी.mtasks.begin(),
                                                             कालसारणी.mtasks.अन्तः_2());

            प्रति (ध्रुव ExecMTask* ध्रुव mtaskp : tasks) {
                Column& column = columns.पृष्ठभागः();
                UASSERT(column.आकार() <= n_तन्तवः, "Invalid partitioning");

                तार्किक intersects = सत्य;
                प्रति (ध्रुव ExecMTask* ध्रुव earlierMtask : column) {
                    यदि (अन्तः_Time(mtaskp) <= startTime(earlierMtask)
                        || startTime(mtaskp) >= अन्तः_Time(earlierMtask)) {
                        intersects = असत्य;
                        भङ्ग;
                    }
                }
                यदि (intersects) {
                    column.emplace_पृष्ठभागः(mtaskp);
                } अन्यथा {
                    columns.emplace_पृष्ठभागः(Column{mtaskp});
                }
            }

            UASSERT(!columns.front().रिक्तानि(), "Should be populated by mtasks");

            प्रति (ध्रुव Column& column : columns) {
                द्विगुण चरमः_Column_विस्थापनम् = 0;
                प्रति (ध्रुव ExecMTask* ध्रुव mtaskp : column) {
                    dumpDotFileEmitMTask(logp, mtaskp, कालसारणी, segmentCost, offsets);
                    चरमः_Column_विस्थापनम् = std::max(चरमः_Column_विस्थापनम्, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.अन्तः_2(), चरमः_Column_विस्थापनम्);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_तन्तवः);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            प्रति (ध्रुव std::सदिशम्<ध्रुव ExecMTask*>& thread : कालसारणी.m_तन्तवः) {
                यदि (thread.रिक्तानि()) भङ्ग;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.पृष्ठभागः()->name() << " -> fork_"
                      << static_cast<पूर्णाङ्क>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                प्रति (आकार_t i = 1; i < thread.आकार(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                प्रति (ध्रुव ExecMTask* ध्रुव mtaskp : thread) {
                    प्रति (ध्रुव V3GraphEdge& edge : mtaskp->बहिर्गमनम्_धाराः()) {
                        ध्रुव ExecMTask* ध्रुव शिखरम्_MTaskp = edge.शिखरम्()->cast<ध्रुव ExecMTask>();
                        यदि (शिखरम्_MTaskp && कालसारणी.अन्तर्भवति(शिखरम्_MTaskp)
                            && threadId(शिखरम्_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << शिखरम्_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    स्थिर शून्य dump_Dot_File_Emit_खण्डम्(ध्रुव std::unique_ptr<std::ofstream>& logp,
                                     ध्रुव string& name, ध्रुव string& label, द्विगुण विस्तारः,
                                     द्विगुण height, द्विगुण xPos, द्विगुण yPos,
                                     ध्रुव string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << विस्तारः
              << " height=" << height << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    स्थिर शून्य dumpDotFileEmitMTask(ध्रुव std::unique_ptr<std::ofstream>& logp,
                                     ध्रुव ExecMTask* ध्रुव mtaskp, ध्रुव Thread_कालसारणी& कालसारणी,
                                     uint32_t segmentCost, std::सदिशम्<द्विगुण>& offsets) {
        प्रति (पूर्णाङ्क i = 0; i < mtaskp->तन्तवः(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ध्रुव string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::गन्तव्यम्_string(i);
            ध्रुव string label = mtaskp->name() + " (" + std::गन्तव्यम्_string(startTime(mtaskp)) + ':'
                                 + std::गन्तव्यम्_string(अन्तः_Time(mtaskp)) + ')'
                                 + "\\ncost=" + std::गन्तव्यम्_string(mtaskp->cost())
                                 + "\\npriority=" + std::गन्तव्यम्_string(mtaskp->प्राथमिकता_2());
            ध्रुव द्विगुण विस्तारः
                = std::max(s_thread_Box_विस्तारः,
                           s_thread_Box_विस्तारः * static_cast<द्विगुण>(mtaskp->cost()) / segmentCost);
            ध्रुव uint32_t mtaskThreadId = threadId(mtaskp) + i * कालसारणी.m_तन्तवः.आकार();
            ध्रुव द्विगुण xPos = विस्तारः / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += विस्तारः + s_horizontalGap;
            ध्रुव द्विगुण yPos = -s_threadBoxHeight * static_cast<द्विगुण>(mtaskThreadId);
            ध्रुव string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_खण्डम्(logp, name, label, विस्तारः, s_threadBoxHeight, xPos, yPos,
                                 fillColor);
        }
    }

    स्थिर शून्य dumpDotFileEmitFork(ध्रुव std::unique_ptr<std::ofstream>& logp, द्विगुण विस्थापनम्,
                                    uint32_t n_तन्तवः) {
        ध्रुव string& name = "fork_" + std::गन्तव्यम्_string(static_cast<पूर्णाङ्क>(विस्थापनम्));
        constexpr द्विगुण विस्तारः = s_thread_Box_विस्तारः / 8;
        ध्रुव द्विगुण height = s_threadBoxHeight * n_तन्तवः;
        ध्रुव द्विगुण xPos = विस्थापनम् - s_horizontalGap / 2;
        ध्रुव द्विगुण yPos
            = -static_cast<द्विगुण>(n_तन्तवः) / 2 * s_threadBoxHeight + s_threadBoxHeight / 2;
        dump_Dot_File_Emit_खण्डम्(logp, name, "", विस्तारः, height, xPos, yPos, "black");
    }

सार्वजनिक:
    स्थिर uint32_t threadId(ध्रुव ExecMTask* mtaskp) {
        ध्रुव स्वचालित& it = s_mtaskState.find(mtaskp);
        प्रत्यागम it != s_mtaskState.अन्तः_2() ? it->second.threadId : UNASSIGNED;
    }
    स्थिर uint32_t startTime(ध्रुव ExecMTask* mtaskp) {
        प्रत्यागम s_mtaskState.at(mtaskp).completionTime - mtaskp->cost();
    }
    स्थिर uint32_t अन्तः_Time(ध्रुव ExecMTask* mtaskp) {
        प्रत्यागम s_mtaskState.at(mtaskp).completionTime;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ध्रुव ExecMTask* mtaskp) ध्रुव {
        ध्रुव uint32_t thisThreadId = threadId(mtaskp);
        uint32_t परिणाम = 0;
        प्रति (ध्रुव V3GraphEdge& edge : mtaskp->अन्तर्_धाराः()) {
            ध्रुव ExecMTask* ध्रुव prevp = edge.fromp()->as<ExecMTask>();
            यदि (threadId(prevp) != thisThreadId && अन्तर्भवति(prevp)) ++परिणाम;
        }
        प्रत्यागम परिणाम;
    }

    uint32_t id() ध्रुव { प्रत्यागम m_id; }
    uint32_t कालसारणी_On(ध्रुव ExecMTask* mtaskp, uint32_t bestThreadId) {
        mtasks.emplace(mtaskp);
        ध्रुव uint32_t best_अन्तः_Time = mtaskp->predictStart() + mtaskp->cost();
        m_अन्तः_Time = std::max(m_अन्तः_Time, best_अन्तः_Time);
        s_mtaskState[mtaskp].completionTime = best_अन्तः_Time;
        s_mtaskState[mtaskp].threadId = bestThreadId;

        // Reference to thread in schedule we are assigning this MTask to.
        std::सदिशम्<ध्रुव ExecMTask*>& bestThread = m_तन्तवः[bestThreadId];
        यदि (!bestThread.रिक्तानि()) s_mtaskState[bestThread.पृष्ठभागः()].nextp = mtaskp;

        // Add the MTask to the schedule
        bestThread.प्रक्षिप_पृष्ठभागः(mtaskp);
        प्रत्यागम best_अन्तः_Time;
    }
    तार्किक अन्तर्भवति(ध्रुव ExecMTask* mtaskp) ध्रुव { प्रत्यागम mtasks.गणनम्(mtaskp); }
    uint32_t अन्तः_Time() ध्रुव { प्रत्यागम m_अन्तः_Time; }
};

uint32_t Thread_कालसारणी::s_अग्र_Id = 0;
std::unordered_map<ध्रुव ExecMTask*, Thread_कालसारणी::MTaskState> Thread_कालसारणी::s_mtaskState{};
constexpr द्विगुण V3ExecGraph::Thread_कालसारणी::s_thread_Box_विस्तारः;

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
वर्ग Pack_तन्तवः final {
    // TYPES
    संरचना MTaskCmp final {
        तार्किक संक्रियकः()(ध्रुव ExecMTask* ap, ध्रुव ExecMTask* bp) ध्रुव {
            प्रत्यागम ap->id() < bp->id();
        }
    };

    // MEMBERS
    ध्रुव uint32_t m_n_तन्तवः;  // Number of threads
    ध्रुव uint32_t m_n_Hier_तन्तवः;  // Number of threads used for hierarchical tasks
    ध्रुव uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ध्रुव uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    स्पष्ट Pack_तन्तवः(uint32_t n_तन्तवः = v3Global.opt.तन्तवः(),
                         uint32_t n_Hier_तन्तवः = v3Global.opt.hier_तन्तवः(),
                         अचिह्नित sandbagNumerator = 30, अचिह्नित sandbagDenom = 100)
        : m_n_तन्तवः{n_तन्तवः}
        , m_n_Hier_तन्तवः{n_Hier_तन्तवः}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_तन्तवः() = स्वभाव;
    VL_UNCOPYABLE(Pack_तन्तवः);

    // METHODS
    uint32_t completionTime(ध्रुव Thread_कालसारणी& कालसारणी, ध्रुव ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        यदि (!कालसारणी.अन्तर्भवति(mtaskp)) प्रत्यागम 0;
        ध्रुव Thread_कालसारणी::MTaskState& state = कालसारणी.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_कालसारणी::UNASSIGNED, "Mtask should have assigned thread");
        यदि (threadId == state.threadId) {
            // No overhead on same thread
            प्रत्यागम state.completionTime;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_अन्तः_Time
            = state.completionTime + (m_sandbagNumerator * mtaskp->cost()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        यदि (state.nextp) {
            ध्रुव uint32_t successor_अन्तः_Time
                = completionTime(कालसारणी, state.nextp, state.threadId);
            यदि ((sandbagged_अन्तः_Time >= successor_अन्तः_Time) && (successor_अन्तः_Time > 1)) {
                sandbagged_अन्तः_Time = successor_अन्तः_Time - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_अन्तः_Time);
        प्रत्यागम sandbagged_अन्तः_Time;
    }

    स्थिर तार्किक isReady(Thread_कालसारणी& कालसारणी, ध्रुव ExecMTask* mtaskp) {
        प्रति (ध्रुव V3GraphEdge& edge_अन्तर् : mtaskp->अन्तर्_धाराः()) {
            ध्रुव ExecMTask* ध्रुव prevp = edge_अन्तर्.fromp()->as<ध्रुव ExecMTask>();
            यदि (कालसारणी.threadId(prevp) == Thread_कालसारणी::UNASSIGNED) {
                // This predecessor is not assigned yet
                प्रत्यागम असत्य;
            }
        }
        प्रत्यागम सत्य;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::सदिशम्<Thread_कालसारणी> pack(V3Graph& mtaskGraph) {
        std::सदिशम्<Thread_कालसारणी> परिणाम;
        परिणाम.emplace_पृष्ठभागः(Thread_कालसारणी{m_n_तन्तवः});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        गणना वर्ग Scheduling_बहुलकम् : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_बहुलकम् बहुलकम् = Scheduling_बहुलकम्::SCHEDULING;

        // Time each thread is occupied until
        std::सदिशम्<uint32_t> busyUntil(std::max(m_n_तन्तवः, m_n_Hier_तन्तवः), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::समुच्चयः<ExecMTask*, MTaskCmp> readyMTasks;
        पूर्णाङ्क maxThreadWorkers = 1;

        // Build initial ready list
        प्रति (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            ExecMTask* ध्रुव mtaskp = vtx.as<ExecMTask>();
            यदि (isReady(परिणाम.पृष्ठभागः(), mtaskp)) readyMTasks.निवेशय(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->तन्तवः());
        }

        यावत् (!readyMTasks.रिक्तानि()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t bestTime = 0xffffffff;
            uint32_t bestThreadId = 0;
            ExecMTask* bestMtaskp = शून्यसूचकः;  // Todo: const ExecMTask*
            Thread_कालसारणी& कालसारणी = परिणाम.पृष्ठभागः();
            प्रति (uint32_t threadId = 0; threadId < कालसारणी.m_तन्तवः.आकार(); ++threadId) {
                प्रति (ExecMTask* ध्रुव mtaskp : readyMTasks) {
                    यदि (बहुलकम् != Scheduling_बहुलकम्::WIDE_TASK_SCHEDULING && mtaskp->तन्तवः() > 1) {
                        बहुलकम् = Scheduling_बहुलकम्::WIDE_TASK_DISCOVERED;
                        अनुवर्त;
                    }
                    यदि (बहुलकम् == Scheduling_बहुलकम्::WIDE_TASK_SCHEDULING && mtaskp->तन्तवः() <= 1)
                        अनुवर्त;

                    uint32_t timeBegin = busyUntil[threadId];
                    यदि (timeBegin > bestTime) {
                        UINFO(6, "th " << threadId << " busy until " << timeBegin
                                       << ", later than bestTime " << bestTime
                                       << ", skipping thread.");
                        भङ्ग;
                    }
                    प्रति (ध्रुव V3GraphEdge& edge : mtaskp->अन्तर्_धाराः()) {
                        ध्रुव ExecMTask* ध्रुव priorp = edge.fromp()->as<ExecMTask>();
                        ध्रुव uint32_t प्राक्_अन्तः_Time = completionTime(कालसारणी, priorp, threadId);
                        यदि (प्राक्_अन्तः_Time > timeBegin) timeBegin = प्राक्_अन्तः_Time;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << timeBegin
                                     << " on thread " << threadId);
                    यदि ((timeBegin < bestTime)
                        || ((timeBegin == bestTime)
                            && bestMtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->प्राथमिकता_2() > bestMtaskp->प्राथमिकता_2()))) {
                        bestTime = timeBegin;
                        bestThreadId = threadId;
                        bestMtaskp = mtaskp;
                    }
                }
            }

            ध्रुव uint32_t अन्तः_Time = कालसारणी.अन्तः_Time();

            यदि (!bestMtaskp && बहुलकम् == Scheduling_बहुलकम्::WIDE_TASK_DISCOVERED) {
                बहुलकम् = Scheduling_बहुलकम्::WIDE_TASK_SCHEDULING;
                ध्रुव uint32_t आकार = m_n_Hier_तन्तवः / maxThreadWorkers;
                UASSERT(आकार, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                यदि (कालसारणी.s_mtaskState.रिक्तानि()) परिणाम.clear();
                परिणाम.emplace_पृष्ठभागः(Thread_कालसारणी{आकार});
                std::fill(busyUntil.begin(), busyUntil.अन्तः_2(), अन्तः_Time);
                अनुवर्त;
            }

            यदि (!bestMtaskp && बहुलकम् == Scheduling_बहुलकम्::WIDE_TASK_SCHEDULING) {
                बहुलकम् = Scheduling_बहुलकम्::SCHEDULING;
                UASSERT(!कालसारणी.s_mtaskState.रिक्तानि(), "Mtask should be added");
                परिणाम.emplace_पृष्ठभागः(Thread_कालसारणी{m_n_तन्तवः});
                std::fill(busyUntil.begin(), busyUntil.अन्तः_2(), अन्तः_Time);
                अनुवर्त;
            }

            UASSERT(bestMtaskp, "Should have found some task");

            bestMtaskp->predictStart(bestTime);
            ध्रुव uint32_t best_अन्तः_Time = कालसारणी.कालसारणी_On(bestMtaskp, bestThreadId);
            busyUntil[bestThreadId] = best_अन्तः_Time;

            // Update the ready list
            ध्रुव आकार_t erased = readyMTasks.erase(bestMtaskp);
            UASSERT_OBJ(erased > 0, bestMtaskp, "Should have erased something?");
            प्रति (ध्रुव V3GraphEdge& edge_बहिर्गमनम् : bestMtaskp->बहिर्गमनम्_धाराः()) {
                ExecMTask* ध्रुव nextp = edge_बहिर्गमनम्.शिखरम्()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(कालसारणी.threadId(nextp) == Thread_कालसारणी::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.find(nextp) == readyMTasks.अन्तः_2(), nextp,
                            "Tasks after one being assigned should not be ready");
                यदि (isReady(कालसारणी, nextp)) {
                    readyMTasks.निवेशय(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        यदि (dumpGraphLevel() >= 4)
            Thread_कालसारणी::dumpDotFilePrefixedAlways(परिणाम, "schedule", m_n_तन्तवः);

        प्रत्यागम परिणाम;
    }

सार्वजनिक:
    // SELF TEST
    स्थिर शून्य selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->fileline();
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->findUInt64DType()};
        AstScope* ध्रुव scopep = नूतन AstScope{flp, शून्यसूचकः, "test", शून्यसूचकः, शून्यसूचकः};
        AstVarScope* ध्रुव trigVscp = नूतन AstVarScope{flp, scopep, varp};
        selfTestHierFirst(trigVscp);
        self_Test_प्रसामान्यम्_First(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_वृक्षः(), trigVscp);
        VL_DO_DANGLING(scopep->delete_वृक्षः(), scopep);
        VL_DO_DANGLING(varp->delete_वृक्षः(), varp);
    }
    स्थिर शून्य self_Test_प्रसामान्यम्_First(AstVarScope* trigVscp) {
        File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->fileline();
        AstExecGraph* ध्रुव execGraphp = नूतन AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ध्रुव स्वचालित makeBody = [&]() -> Ast_ग्रन्थिः_Stmt* { प्रत्यागम नूतन AstComment{flp, ""}; };
        ExecMTask* ध्रुव t0 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t0->cost(1000);
        t0->प्राथमिकता_2(1100);
        ExecMTask* ध्रुव t1 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t1->cost(100);
        t1->प्राथमिकता_2(100);
        ExecMTask* ध्रुव t2 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t2->cost(100);
        t2->प्राथमिकता_2(100);
        t2->तन्तवः(2);
        ExecMTask* ध्रुव t3 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t3->cost(100);
        t3->प्राथमिकता_2(100);
        t3->तन्तवः(3);
        ExecMTask* ध्रुव t4 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t4->cost(100);
        t4->प्राथमिकता_2(100);
        t4->तन्तवः(3);
        ExecMTask* ध्रुव t5 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t5->cost(100);
        t5->प्राथमिकता_2(100);
        ExecMTask* ध्रुव t6 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t6->cost(100);
        t6->प्राथमिकता_2(100);

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

        constexpr uint32_t तन्तवः = 2;
        constexpr uint32_t hier_तन्तवः = 6;
        Pack_तन्तवः packer{तन्तवः, hier_तन्तवः,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ध्रुव std::सदिशम्<Thread_कालसारणी> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.आकार(), 3);
        UASSERT_SELFTEST(scheduled[0].m_तन्तवः.आकार(), तन्तवः);
        UASSERT_SELFTEST(scheduled[0].m_तन्तवः[0].आकार(), 2);
        प्रति (आकार_t i = 1; i < scheduled[0].m_तन्तवः.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_तन्तवः[i].आकार(), 0);

        UASSERT_SELFTEST(scheduled[0].m_तन्तवः[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_तन्तवः[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_तन्तवः.आकार(), hier_तन्तवः / 3);
        UASSERT_SELFTEST(scheduled[1].m_तन्तवः[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_तन्तवः[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_तन्तवः[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_तन्तवः.आकार(), तन्तवः);
        UASSERT_SELFTEST(scheduled[2].m_तन्तवः[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_तन्तवः[1][0], t6);

        UASSERT_SELFTEST(Thread_कालसारणी::s_mtaskState.आकार(), 7);

        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t6), 1);

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

        प्रति (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_वृक्षः();
        VL_DO_DANGLING(execGraphp->delete_वृक्षः(), execGraphp);
        Thread_कालसारणी::s_mtaskState.clear();
    }
    स्थिर शून्य selfTestHierFirst(AstVarScope* trigVscp) {
        File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->fileline();
        AstExecGraph* ध्रुव execGraphp = नूतन AstExecGraph{flp, "test", trigVscp};
        V3Graph& graph = *execGraphp->depGraphp();
        ध्रुव स्वचालित makeBody = [&]() -> Ast_ग्रन्थिः_Stmt* { प्रत्यागम नूतन AstComment{flp, ""}; };
        ExecMTask* ध्रुव t0 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t0->cost(1000);
        t0->प्राथमिकता_2(1100);
        t0->तन्तवः(2);
        ExecMTask* ध्रुव t1 = नूतन ExecMTask{execGraphp, शून्यसूचकः, makeBody()};
        t1->cost(100);
        t1->प्राथमिकता_2(100);

        /*
                          0
                          |
                          1
        */
        नूतन V3GraphEdge{&graph, t0, t1, 1};

        constexpr uint32_t तन्तवः = 1;
        constexpr uint32_t hier_तन्तवः = 2;
        Pack_तन्तवः packer{तन्तवः, hier_तन्तवः,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ध्रुव std::सदिशम्<Thread_कालसारणी> scheduled = packer.pack(graph);
        UASSERT_SELFTEST(scheduled.आकार(), 2);
        UASSERT_SELFTEST(scheduled[0].m_तन्तवः.आकार(), hier_तन्तवः / 2);
        UASSERT_SELFTEST(scheduled[0].m_तन्तवः[0].आकार(), 1);
        प्रति (आकार_t i = 1; i < scheduled[0].m_तन्तवः.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_तन्तवः[i].आकार(), 0);

        UASSERT_SELFTEST(scheduled[0].m_तन्तवः[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_तन्तवः.आकार(), तन्तवः);
        UASSERT_SELFTEST(scheduled[1].m_तन्तवः[0].आकार(), 1);
        प्रति (आकार_t i = 1; i < scheduled[1].m_तन्तवः.आकार(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_तन्तवः[i].आकार(), 0);
        UASSERT_SELFTEST(scheduled[1].m_तन्तवः[0][0], t1);

        UASSERT_SELFTEST(Thread_कालसारणी::s_mtaskState.आकार(), 2);

        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_कालसारणी::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completionTime(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completionTime(scheduled[1], t1, 1), 1130);

        प्रति (V3GraphVertex& vtx : graph.vertices()) vtx.as<ExecMTask>()->funcp()->delete_वृक्षः();
        VL_DO_DANGLING(execGraphp->delete_वृक्षः(), execGraphp);
        Thread_कालसारणी::s_mtaskState.clear();
    }

    स्थिर std::सदिशम्<Thread_कालसारणी> apply(V3Graph& mtaskGraph) {
        प्रत्यागम Pack_तन्तवः{}.pack(mtaskGraph);
    }
};

उपयुज्य EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
उपयुज्य Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

शून्य normalizeCosts(Costs& costs) {
    ध्रुव स्वचालित scaleCost = [](uint64_t मानम्, द्विगुण multiplier) {
        द्विगुण scaled = static_cast<द्विगुण>(मानम्) * multiplier;
        यदि (मानम् && scaled < 1) scaled = 1;
        प्रत्यागम static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t योग_Cost_Profiled = 0;  // For data with estimate and profile
    uint64_t योग_Cost_Estimate = 0;  // For data with estimate and profile
    प्रति (ध्रुव स्वचालित& est : costs) {
        यदि (est.second.second) {
            योग_Cost_Estimate += est.second.first;
            योग_Cost_Profiled += est.second.second;
        }
    }

    यदि (योग_Cost_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ध्रुव द्विगुण est_गन्तव्यम्_Profile
            = static_cast<द्विगुण>(योग_Cost_Profiled) / static_cast<द्विगुण>(योग_Cost_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_गन्तव्यम्_Profile
                                                    << ", sumCostProfiled=" << योग_Cost_Profiled
                                                    << " sumCostEstimate=" << योग_Cost_Estimate);
        प्रति (स्वचालित& est : costs) {
            uint64_t& costEstimate = est.second.first;
            costEstimate = scaleCost(costEstimate, est_गन्तव्यम्_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t maxCost = 0;
    प्रति (स्वचालित& est : costs) {
        ध्रुव uint64_t& costEstimate = est.second.first;
        ध्रुव uint64_t& costProfiled = est.second.second;
        यदि (maxCost < costEstimate) maxCost = costEstimate;
        यदि (maxCost < costProfiled) maxCost = costProfiled;
        UINFO(9, "Post uint scale: ce = " << est.second.first << " cp=" << est.second.second);
    }
    constexpr uint64_t scale_अधस्तात्_गन्तव्यम् = 10000000;  // Extra room for future algorithms to add costs
    यदि (maxCost > scale_अधस्तात्_गन्तव्यम्) {
        ध्रुव द्विगुण scaleup = static_cast<द्विगुण>(scale_अधस्तात्_गन्तव्यम्) / static_cast<द्विगुण>(maxCost);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << maxCost);
        प्रति (स्वचालित& est : costs) {
            est.second.first = scaleCost(est.second.first, scaleup);
            est.second.second = scaleCost(est.second.second, scaleup);
        }
    }
}

शून्य remove_रिक्तानि_MTasks(AstExecGraph* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    प्रति (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        यदि (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), callp);
    }
    V3Graph* ध्रुव execMTaskGraphp = execGraphp->depGraphp();
    प्रति (V3GraphVertex* ध्रुव vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ध्रुव mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ध्रुव funcp = mtaskp->funcp();
        यदि (funcp->stmtsp()) अनुवर्त;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_धाराः(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);
}

शून्य fillinCosts(V3Graph* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    प्रति (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ध्रुव mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ध्रुव uint64_t costEstimate = V3_Instr_गणनम्::गणनम्(mtp->funcp(), असत्य);
        ध्रुव uint64_t costProfiled
            = V3Control::get_Profile_दत्त(v3Global.opt.उपसर्गः(), mtp->hashName());
        यदि (costProfiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << costProfiled);
        }
        costs[mtp->id()] = std::make_pair(costEstimate, costProfiled);
    }

    normalizeCosts(costs /*ref*/);

    पूर्णाङ्क योग_Estimates = 0;
    पूर्णाङ्क missingProfiles = 0;
    प्रति (V3GraphVertex& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ध्रुव mtp = vtx.as<ExecMTask>();
        ध्रुव uint32_t costEstimate = costs[mtp->id()].first;
        ध्रुव uint64_t costProfiled = costs[mtp->id()].second;
        UINFO(9, "ce = " << costEstimate << " cp=" << costProfiled);
        UASSERT(costEstimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(costProfiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ध्रुव uint64_t costProfiled32 = static_cast<uint32_t>(costProfiled);
        uint32_t cost_गन्तव्यम्_Use = costProfiled32;
        यदि (!costProfiled32) {
            cost_गन्तव्यम्_Use = costEstimate;
            यदि (costEstimate != 0) ++missingProfiles;
        }
        यदि (costEstimate != 0) ++योग_Estimates;
        mtp->cost(cost_गन्तव्यम्_Use);
        mtp->प्राथमिकता_2(cost_गन्तव्यम्_Use);
    }

    यदि (missingProfiles) {
        यदि (File_पङ्क्तिः* ध्रुव fl = V3Control::get_Profile_दत्त_File_पङ्क्तिः()) {
            यदि (V3Control::अन्तर्भवति_MTask_Profile_दत्त()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << योग_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

शून्य finalizeCosts(V3Graph* execMTaskGraphp) {
    Graph_प्रवाहः_Unordered ser(execMTaskGraphp, GraphWay::REVERSE);
    यावत् (ध्रुव V3GraphVertex* ध्रुव vxp = ser.nextp()) {
        ExecMTask* ध्रुव mtp = const_cast<V3GraphVertex*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        प्रति (V3GraphEdge& edge : mtp->बहिर्गमनम्_धाराः()) {
            ध्रुव ExecMTask* ध्रुव followp = edge.शिखरम्()->as<ExecMTask>();
            यदि ((followp->प्राथमिकता_2() + mtp->cost()) > mtp->प्राथमिकता_2()) {
                mtp->प्राथमिकता_2(followp->प्राथमिकता_2() + mtp->cost());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_संक्रामकम्_धाराः();

    // Record summary stats for final m_tasks graph.
    ध्रुव स्वचालित report = execMTaskGraphp->parallelismReport([](ध्रुव V3GraphVertex* vtxp) {  //
        प्रत्यागम vtxp->as<ध्रुव ExecMTask>()->cost();
    });
    V3Stats::सङ्कलय_Stat("MTask graph, final, critical path cost", report.critical_मार्गः_Cost());
    V3Stats::सङ्कलय_Stat("MTask graph, final, total graph cost", report.योग_Graph_Cost());
    V3Stats::सङ्कलय_Stat("MTask graph, final, mtask count", report.vertex_गणनम्());
    V3Stats::सङ्कलय_Stat("MTask graph, final, edge count", report.edge_गणनम्());
    V3Stats::सङ्कलय_Stat("MTask graph, final, parallelism factor", report.parallelism_गुणकः());
    यदि (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_मार्गः_Cost());
        UINFO(0, "    Total graph cost = " << report.योग_Graph_Cost());
        UINFO(0, "    MTask vertex count = " << report.vertex_गणनम्());
        UINFO(0, "    Edge count = " << report.edge_गणनम्());
        UINFO(0, "    Parallelism factor = " << report.parallelism_गुणकः());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ध्रुव std::string& name, uint32_t nDependencies) {
    Ast_ग्रन्थिः_Module* ध्रुव modp = v3Global.rootp()->शिखरम्_Modulep();
    File_पङ्क्तिः* ध्रुव fl = modp->fileline();
    AstBasicDType* ध्रुव dtypep
        = v3Global.rootp()->typeTablep()->findBasicDType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(सत्य);
    varp->valuep(नूतन AstConst{fl, nDependencies});
    modp->सङ्कलय_Stmtsp(varp);
    प्रत्यागम varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ध्रुव std::string& विधानम्) {
    File_पङ्क्तिः* ध्रुव fl = varp->fileline();
    AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ध्रुव AstScope* ध्रुव शिखरम्_Scopep = v3Global.rootp()->शिखरम्_Scopep()->scopep();
    refp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, शिखरम्_Scopep->nameDotless()});
    AstCStmt* ध्रुव stmtp = नूतन AstCStmt{fl};
    stmtp->सङ्कलय(refp);
    stmtp->सङ्कलय("." + विधानम् + ";");
    प्रत्यागम stmtp;
}

शून्य सङ्कलय_MTask_गन्तव्यम्_Function(ध्रुव Thread_कालसारणी& कालसारणी, ध्रुव uint32_t threadId, AstCFunc* funcp,
                        ध्रुव ExecMTask* mtaskp,
                        ध्रुव std::unordered_map<ध्रुव ExecMTask*, AstVar*>& stateVars) {
    AstScope* ध्रुव scopep = v3Global.rootp()->शिखरम्_Scopep()->scopep();
    Ast_ग्रन्थिः_Module* ध्रुव modp = v3Global.rootp()->शिखरम्_Modulep();
    File_पङ्क्तिः* ध्रुव fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ध्रुव स्वचालित सङ्कलय_CStmt = [=](ध्रुव string& stmt) -> शून्य {  //
        funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, stmt});
    };

    यदि (कालसारणी.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        यदि (v3Global.opt.profExec()) {
            सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->सङ्कलय_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        यदि (v3Global.opt.profExec()) {
            सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    यदि (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        सङ्कलय_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::गन्तव्यम्_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ध्रुव callp = नूतन AstCCall{fl, mtaskp->funcp()};
    callp->selfPointer(VSelfPointerText{VSelfPointerText::VlSyms{}, scopep->nameDotless()});
    callp->dtype_समुच्चयः_Void();
    funcp->सङ्कलय_Stmtsp(callp->makeStmt());

    यदि (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        सङ्कलय_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::गन्तव्यम्_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    प्रति (ध्रुव V3GraphEdge& edge : mtaskp->बहिर्गमनम्_धाराः()) {
        ध्रुव ExecMTask* ध्रुव nextp = edge.शिखरम्()->as<ExecMTask>();
        यदि (कालसारणी.threadId(nextp) != threadId && कालसारणी.अन्तर्भवति(nextp)) {
            funcp->सङ्कलय_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ध्रुव std::सदिशम्<AstCFunc*> createThreadFunctions(ध्रुव Thread_कालसारणी& कालसारणी,
                                                   ध्रुव string& चिह्नकम्) {
    Ast_ग्रन्थिः_Module* ध्रुव modp = v3Global.rootp()->शिखरम्_Modulep();
    File_पङ्क्तिः* ध्रुव fl = modp->fileline();

    std::सदिशम्<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ध्रुव ExecMTask*, AstVar*> stateVars;
    प्रति (ध्रुव std::सदिशम्<ध्रुव ExecMTask*>& thread : कालसारणी.m_तन्तवः) {
        प्रति (ध्रुव ExecMTask* ध्रुव mtaskp : thread) {
            यदि (ध्रुव uint32_t nDependencies = कालसारणी.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_गन्तव्यम्_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    प्रति (ध्रुव std::सदिशम्<ध्रुव ExecMTask*>& thread : कालसारणी.m_तन्तवः) {
        यदि (thread.रिक्तानि()) अनुवर्त;
        ध्रुव uint32_t threadId = कालसारणी.threadId(thread.front());
        ध्रुव string name{"__Vthread__" + चिह्नकम् + "__s" + cvt_गन्तव्यम्_Str(कालसारणी.id()) + "__t"
                          + cvt_गन्तव्यम्_Str(threadId)};
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{fl, name, शून्यसूचकः, "void"};
        modp->सङ्कलय_Stmtsp(funcp);
        funcps.प्रक्षिप_पृष्ठभागः(funcp);
        funcp->isStatic(सत्य);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(सत्य);
        funcp->प्रविष्टिः_Point(सत्य);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, EmitCUtil::void_Self_नियोजय(modp)});
        funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, EmitCUtil::sym_Class_नियोजय()});

        // Invoke each mtask scheduled to this thread from the thread function
        प्रति (ध्रुव ExecMTask* ध्रुव mtaskp : thread) {
            सङ्कलय_MTask_गन्तव्यम्_Function(कालसारणी, threadId, funcp, mtaskp, stateVars);
        }
    }

    प्रत्यागम funcps;
}

शून्य सङ्कलय_Thread_Start_Wrapper(AstExecGraph* ध्रुव execGraphp) {
    // FileLine used for constructing nodes below
    File_पङ्क्तिः* ध्रुव fl = v3Global.rootp()->fileline();
    ध्रुव string& चिह्नकम् = execGraphp->name();

    // Add thread function invocations to execGraph
    ध्रुव स्वचालित सङ्कलय_CStmt = [=](ध्रुव string& stmt) -> शून्य {  //
        execGraphp->runfuncp()->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, stmt});
    };

    यदि (v3Global.opt.profExec()) {
        सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    सङ्कलय_CStmt("vlSymsp->__Vm_even_cycle__" + चिह्नकम् + " = !vlSymsp->__Vm_even_cycle__" + चिह्नकम् + ";");

    यदि (!v3Global.opt.hier_खण्डानि().रिक्तानि()) सङ्कलय_CStmt("std::vector<size_t> indexes;");
}

शून्य सङ्कलय_Thread_अन्तः_Wrapper(AstExecGraph* ध्रुव execGraphp) {
    // Add thread function invocations to execGraph
    ध्रुव स्वचालित सङ्कलय_CStmt = [=](ध्रुव string& stmt) -> शून्य {  //
        File_पङ्क्तिः* ध्रुव flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, stmt});
    };

    सङ्कलय_CStmt("Verilated::mtaskId(0);");
    यदि (v3Global.opt.profExec()) { सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
शून्य सङ्कलय_Thread_Start_गन्तव्यम्_Exec_Graph(AstExecGraph* ध्रुव execGraphp,
                               ध्रुव std::सदिशम्<AstCFunc*>& funcps, uint32_t कालसारणी_Id) {
    // FileLine used for constructing nodes below
    File_पङ्क्तिः* ध्रुव fl = v3Global.rootp()->fileline();
    ध्रुव string& चिह्नकम् = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ध्रुव finalVarp
        = newMTaskStateVar("final__" + cvt_गन्तव्यम्_Str(कालसारणी_Id) + "__" + चिह्नकम्, funcps.आकार());
    प्रति (AstCFunc* ध्रुव funcp : funcps) {
        funcp->सङ्कलय_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ध्रुव स्वचालित सङ्कलय_CStmt = [=](ध्रुव string& stmt) -> शून्य {  //
        execGraphp->runfuncp()->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, stmt});
    };

    ध्रुव uint32_t चरमः = funcps.आकार() - 1;
    यदि (!v3Global.opt.hier_खण्डानि().रिक्तानि() && चरमः > 0) {
        सङ्कलय_CStmt("for (size_t i = 0; i < " + std::गन्तव्यम्_string(चरमः) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    प्रति (AstCFunc* ध्रुव funcp : funcps) {
        यदि (i != चरमः) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            execGraphp->runfuncp()->सङ्कलय_Stmtsp(cstmtp);
            cstmtp->सङ्कलय("vlSymsp->__Vm_threadPoolp->workerp(");
            यदि (v3Global.opt.hierChild() || !v3Global.opt.hier_खण्डानि().रिक्तानि()) {
                cstmtp->सङ्कलय("indexes[" + std::गन्तव्यम्_string(i) + "]");
            } अन्यथा {
                cstmtp->सङ्कलय(std::गन्तव्यम्_string(i));
            }
            cstmtp->सङ्कलय(")->addTask(");
            cstmtp->सङ्कलय(नूतन Ast_Addr_सम्बन्धि_CFunc{fl, funcp});
            cstmtp->सङ्कलय(", vlSelf, vlSymsp->__Vm_even_cycle__" + चिह्नकम् + ");");
        } अन्यथा {
            // The last will run on the main thread.
            AstCCall* ध्रुव callp = नूतन AstCCall{fl, funcp};
            callp->dtype_समुच्चयः_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + चिह्नकम्);
            execGraphp->runfuncp()->सङ्कलय_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::सङ्कलय_Stat_योग("Optimizations, Thread schedule total tasks", i);

    यदि (v3Global.opt.profExec()) {
        सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->सङ्कलय_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + चिह्नकम् + ")"));
    यदि (v3Global.opt.profExec()) {
        सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    यदि (!v3Global.opt.hier_खण्डानि().रिक्तानि() && चरमः > 0) {
        सङ्कलय_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

शून्य processMTaskBodies(AstExecGraph* ध्रुव execGraphp) {
    प्रति (V3GraphVertex* ध्रुव vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ध्रुव mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ध्रुव funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ग्रन्थिः* stmtsp = funcp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();

        // Helper function to make the code a bit more legible
        ध्रुव स्वचालित सङ्कलय_CStmt = [=](ध्रुव string& stmt) -> शून्य {  //
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        यदि (v3Global.opt.profExec()) {
            std::string तर्काः = std::गन्तव्यम्_string(mtaskp->id());
            तर्काः += ", " + std::गन्तव्यम्_string(mtaskp->predictStart());
            तर्काः += ", \"";
            यदि (v3Global.opt.hierChild()) तर्काः += v3Global.opt.शिखरम्_Module();
            तर्काः += "\"";
            सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + तर्काः + ");");
        }
        // Set mtask ID in the run-time system
        सङ्कलय_CStmt("Verilated::mtaskId(" + std::गन्तव्यम्_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->सङ्कलय_Stmtsp(stmtsp);
        // Flush message queue
        सङ्कलय_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        यदि (v3Global.opt.profExec()) {
            ध्रुव std::string& तर्काः = std::गन्तव्यम्_string(mtaskp->cost());
            सङ्कलय_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + तर्काः + ");");
        }
    }
}

शून्य implementExecGraph(AstExecGraph* ध्रुव execGraphp, ध्रुव Thread_कालसारणी& कालसारणी) {
    // Nothing to be done if there are no MTasks in the graph at all.
    यदि (execGraphp->depGraphp()->रिक्तानि()) प्रत्यागम;

    // Create a function to be run by each thread.
    ध्रुव std::सदिशम्<AstCFunc*>& funcps = createThreadFunctions(कालसारणी, execGraphp->name());
    UASSERT(!funcps.रिक्तानि(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    सङ्कलय_Thread_Start_गन्तव्यम्_Exec_Graph(execGraphp, funcps, कालसारणी.id());
}

शून्य create_सततम्_Function(AstExecGraph* ध्रुव execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_पङ्क्तिः* ध्रुव flp = execGraphp->fileline();
    Ast_ग्रन्थिः_Module* ध्रुव modp = v3Global.rootp()->शिखरम्_Modulep();
    AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, "runExecGraph_" + execGraphp->name(), शून्यसूचकः};
    funcp->isLoose(सत्य);
    funcp->voidSelf(सत्य);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(सत्य);
    modp->सङ्कलय_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

शून्य सङ्कलय_Trigger_Masks(AstExecGraph* ध्रुव execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_पङ्क्तिः* ध्रुव flp = execGraphp->fileline();
    AstVar* ध्रुव triggersp = execGraphp->triggerp()->varp();
    प्रति (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> wordMasks;
        callp->funcp()->foreach([&](ध्रुव AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            प्रति (ध्रुव uint32_t सूचकांक : trigp->indices()) {
                wordMasks[VL_BITWORD_Q(सूचकांक)] |= VL_BIT_Q(सूचकांक);
            }
        });
        AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, triggersp->dtypep(),
                                                     नूतन AstConst{flp, AstConst::Unsized64{}, 0}};
        प्रति (ध्रुव स्वचालित& word_आवरणम् : wordMasks) {
            consp->valuep(word_आवरणम्.first,
                          नूतन AstConst{flp, AstConst::Unsized64{}, word_आवरणम्.second});
        }
        execGraphp->सङ्कलय_Maskps(V3ConstPool::find(consp));
        VL_DO_DANGLING(consp->delete_वृक्षः(), consp);
    }
}

शून्य सङ्कलय_Trigger_Costs(AstExecGraph* ध्रुव execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_पङ्क्तिः* ध्रुव flp = execGraphp->fileline();
    ध्रुव uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::सदिशम्<uint32_t> costs(nTriggers, 0);
    प्रति (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            यदि (ध्रुव AstTriggered* ध्रुव trigp = VN_CAST(ifp->condp(), Triggered)) {
                ध्रुव uint32_t cost = V3_Instr_गणनम्::गणनम्(ifp, असत्य);
                प्रति (ध्रुव uint32_t सूचकांक : trigp->indices()) costs.at(सूचकांक) += cost;
            }
        });
    }
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    AstUnpackArrayDType* ध्रुव dtypep
        = नूतन AstUnpackArrayDType{flp, netlistp->findBitDType(32, 32, VSigning::UNSIGNED),
                                  नूतन AstRange{flp, static_cast<पूर्णाङ्क>(nTriggers - 1), 0}};
    netlistp->typeTablep()->सङ्कलय_Typesp(dtypep);
    AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, dtypep, नूतन AstConst{flp, 0}};
    प्रति (uint32_t सूचकांक = 0; सूचकांक < nTriggers; ++सूचकांक) {
        यदि (costs[सूचकांक]) consp->valuep(सूचकांक, नूतन AstConst{flp, costs[सूचकांक]});
    }
    execGraphp->triggerCostsp(V3ConstPool::find(consp));
    VL_DO_DANGLING(consp->delete_वृक्षः(), consp);
}

// Called by Verilator top stage
शून्य implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::सदिशम्<AstExecGraph*> execGraphps;
    netlistp->शिखरम्_Modulep()->foreach([&](AstExecGraph* egp) { execGraphps.emplace_पृष्ठभागः(egp); });

    // Process each
    प्रति (AstExecGraph* ध्रुव execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_रिक्तानि_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        यदि (execGraphp->depGraphp()->रिक्तानि()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), execGraphp);
            प्रत्यागम;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        यदि (dumpGraphLevel() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_सततम्_Function(execGraphp);
        सङ्कलय_Thread_Start_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ध्रुव std::सदिशम्<Thread_कालसारणी> packed = Pack_तन्तवः::apply(*execGraphp->depGraphp());
        V3Stats::सङ्कलय_Stat_योग("Optimizations, Thread schedule count",
                            static_cast<द्विगुण>(packed.आकार()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        प्रति (ध्रुव Thread_कालसारणी& कालसारणी : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implementExecGraph(execGraphp, कालसारणी);
        }

        सङ्कलय_Thread_अन्तः_Wrapper(execGraphp);

        सङ्कलय_Trigger_Masks(execGraphp);
        सङ्कलय_Trigger_Costs(execGraphp);
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

    Pack_तन्तवः::selfTest();
}

}  // namespace V3ExecGraph
