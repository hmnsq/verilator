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

AstCFunc* ExecMTask::createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::منزل_string(id);
    AstCFunc* ثابت newp = جديد AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(سچ);
    newp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(سچ);
    newp->جوڑو_Stmtsp(stmtsp);
    إذا (scopep) scopep->جوڑو_Blocksp(newp);
    أرجع newp;
}

ExecMTask::ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep,
                     Ast_گرہ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_گراف_راس{execGraphp->depGraphp()},
      m_id{s_اگلا_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).منزل_String()} {}

فراغ ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvt_منزل_Hex(هذا);
    إذا (ترجیح_2() || لاگت()) str << " [pr=" << ترجیح_2() << " c=" << cvt_منزل_Str(لاگت()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_اگلا_Id{0};

نطاق V3_Exec_گراف {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
صنف Thread_ویلا_سارنی final {
    یار صنف Pack_دھاگے;

    uint32_t m_id;  // Unique ID of a schedule
    ساكن uint32_t s_اگلا_Id;  // Next ID number to use
    std::unordered_سیٹ<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_سرا_ویلا = 0;  // Latest task end time in this schedule

عام:
    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    بنية MTaskState final {
        uint32_t completion_ویلا = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = خالی_سوچک;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::سمتیہ<std::سمتیہ<ثابت ExecMTask*>> m_دھاگے;

    // Global state for each mtask.
    ساكن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    واضح Thread_ویلا_سارنی(uint32_t n_دھاگے)
        : m_id{s_اگلا_Id++}
        , m_دھاگے{n_دھاگے} {}
    Thread_ویلا_سارنی(Thread_ویلا_سارنی&&) = افتراضي;
    Thread_ویلا_سارنی& عامل=(Thread_ویلا_سارنی&&) = افتراضي;

خاص:
    VL_UNCOPYABLE(Thread_ویلا_سارنی);

    ساكن constexpr مزدوج s_thread_Box_چوڑائی = 2.5;
    ساكن constexpr مزدوج s_thread_Box_اچائی = 1.5;
    ساكن constexpr مزدوج s_horizontalGap = s_thread_Box_چوڑائی / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساكن فراغ dumpDotFilePrefixedAlways(ثابت std::سمتیہ<Thread_ویلا_سارنی>& schedules,
                                          ثابت string& nameComment, uint32_t n_دھاگے) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_دھاگے);
    }
    ساكن فراغ dumpDotFile(ثابت std::سمتیہ<Thread_ویلا_سارنی>& schedules, ثابت string& filename,
                            uint32_t n_دھاگے) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        إذا (logp->fail()) v3fatal("Can't write file: " << filename);

        //سرخی
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        لكل (uint32_t i = 0; i < n_دھاگے; ++i) {
            ثابت string name = "t" + std::منزل_string(i);
            ثابت string label = "Thread " + std::منزل_string(i);
            constexpr مزدوج posX = -s_horizontalGap;
            ثابت مزدوج posY = -static_cast<مزدوج>(i) * s_thread_Box_اچائی;
            dump_Dot_File_Emit_بلاک(logp, name, label, s_thread_Box_چوڑائی, s_thread_Box_اچائی, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_لاگت = 0;
        لكل (ثابت خودکار& state : Thread_ویلا_سارنی::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.پہلا;
            max_لاگت = std::max(max_لاگت, mtaskp->لاگت());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_فی_Longest_Mtask = 4;
        ثابت uint32_t segment_لاگت = max_لاگت / segments_فی_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::سمتیہ<مزدوج> offsets(n_دھاگے, 0.0);
        لكل (ثابت Thread_ویلا_سارنی& ویلا_سارنی : schedules) {
            إذا (ویلا_سارنی.mtasks.خالی_تھاواں()) استمر;
            ورتوں کالم = std::سمتیہ<ثابت ExecMTask*>;
            std::سمتیہ<کالم> columns = {{}};

            // Order tasks based on their start time
            بنية Cmp final {
                منطقي عامل()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    إذا (شروع_ویلا(a) == شروع_ویلا(b)) أرجع threadId(a) < threadId(b);
                    أرجع شروع_ویلا(a) < شروع_ویلا(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(ویلا_سارنی.mtasks.begin(),
                                                             ویلا_سارنی.mtasks.سرا());

            لكل (ثابت ExecMTask* ثابت mtaskp : tasks) {
                کالم& کالم_2 = columns.پٹھ();
                UASSERT(کالم_2.ناپ() <= n_دھاگے, "Invalid partitioning");

                منطقي intersects = سچ;
                لكل (ثابت ExecMTask* ثابت earlierMtask : کالم_2) {
                    إذا (سرا_ویلا(mtaskp) <= شروع_ویلا(earlierMtask)
                        || شروع_ویلا(mtaskp) >= سرا_ویلا(earlierMtask)) {
                        intersects = خطأ;
                        اكسر;
                    }
                }
                إذا (intersects) {
                    کالم_2.emplace_پٹھ(mtaskp);
                } وإلا {
                    columns.emplace_پٹھ(کالم{mtaskp});
                }
            }

            UASSERT(!columns.front().خالی_تھاواں(), "Should be populated by mtasks");

            لكل (ثابت کالم& کالم_2 : columns) {
                مزدوج آخری_کالم_وتھ = 0;
                لكل (ثابت ExecMTask* ثابت mtaskp : کالم_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, ویلا_سارنی, segment_لاگت, offsets);
                    آخری_کالم_وتھ = std::max(آخری_کالم_وتھ, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.سرا(), آخری_کالم_وتھ);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_دھاگے);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            لكل (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : ویلا_سارنی.m_دھاگے) {
                إذا (thread.خالی_تھاواں()) اكسر;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.پٹھ()->name() << " -> fork_"
                      << static_cast<صحيح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                لكل (ناپ_t i = 1; i < thread.ناپ(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
                    لكل (ثابت V3_گراف_کنارا& کنارا : mtaskp->باہرلا_کنارے()) {
                        ثابت ExecMTask* ثابت چوٹی_MTaskp = کنارا.چوٹی()->cast<ثابت ExecMTask>();
                        إذا (چوٹی_MTaskp && ویلا_سارنی.شامل_اے(چوٹی_MTaskp)
                            && threadId(چوٹی_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << چوٹی_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ساكن فراغ dump_Dot_File_Emit_بلاک(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, مزدوج چوڑائی,
                                     مزدوج اچائی, مزدوج xPos, مزدوج yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << چوڑائی
              << " height=" << اچائی << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساكن فراغ dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت Thread_ویلا_سارنی& ویلا_سارنی,
                                     uint32_t segment_لاگت, std::سمتیہ<مزدوج>& offsets) {
        لكل (صحيح i = 0; i < mtaskp->دھاگے(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::منزل_string(i);
            ثابت string label = mtaskp->name() + " (" + std::منزل_string(شروع_ویلا(mtaskp)) + ':'
                                 + std::منزل_string(سرا_ویلا(mtaskp)) + ')'
                                 + "\\ncost=" + std::منزل_string(mtaskp->لاگت())
                                 + "\\npriority=" + std::منزل_string(mtaskp->ترجیح_2());
            ثابت مزدوج چوڑائی
                = std::max(s_thread_Box_چوڑائی,
                           s_thread_Box_چوڑائی * static_cast<مزدوج>(mtaskp->لاگت()) / segment_لاگت);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * ویلا_سارنی.m_دھاگے.ناپ();
            ثابت مزدوج xPos = چوڑائی / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += چوڑائی + s_horizontalGap;
            ثابت مزدوج yPos = -s_thread_Box_اچائی * static_cast<مزدوج>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_بلاک(logp, name, label, چوڑائی, s_thread_Box_اچائی, xPos, yPos,
                                 fillColor);
        }
    }

    ساكن فراغ dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, مزدوج وتھ,
                                    uint32_t n_دھاگے) {
        ثابت string& name = "fork_" + std::منزل_string(static_cast<صحيح>(وتھ));
        constexpr مزدوج چوڑائی = s_thread_Box_چوڑائی / 8;
        ثابت مزدوج اچائی = s_thread_Box_اچائی * n_دھاگے;
        ثابت مزدوج xPos = وتھ - s_horizontalGap / 2;
        ثابت مزدوج yPos
            = -static_cast<مزدوج>(n_دھاگے) / 2 * s_thread_Box_اچائی + s_thread_Box_اچائی / 2;
        dump_Dot_File_Emit_بلاک(logp, name, "", چوڑائی, اچائی, xPos, yPos, "black");
    }

عام:
    ساكن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت خودکار& it = s_mtaskState.لبھو(mtaskp);
        أرجع it != s_mtaskState.سرا() ? it->دوجا.threadId : UNASSIGNED;
    }
    ساكن uint32_t شروع_ویلا(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_ویلا - mtaskp->لاگت();
    }
    ساكن uint32_t سرا_ویلا(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_ویلا;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t نتیجہ = 0;
        لكل (ثابت V3_گراف_کنارا& کنارا : mtaskp->وچ_کنارے()) {
            ثابت ExecMTask* ثابت prevp = کنارا.fromp()->as<ExecMTask>();
            إذا (threadId(prevp) != thisThreadId && شامل_اے(prevp)) ++نتیجہ;
        }
        أرجع نتیجہ;
    }

    uint32_t id() ثابت { أرجع m_id; }
    uint32_t ویلا_سارنی_On(ثابت ExecMTask* mtaskp, uint32_t بہترین_Thread_Id) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t بہترین_سرا_ویلا = mtaskp->predict_شروع() + mtaskp->لاگت();
        m_سرا_ویلا = std::max(m_سرا_ویلا, بہترین_سرا_ویلا);
        s_mtaskState[mtaskp].completion_ویلا = بہترین_سرا_ویلا;
        s_mtaskState[mtaskp].threadId = بہترین_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::سمتیہ<ثابت ExecMTask*>& بہترین_Thread = m_دھاگے[بہترین_Thread_Id];
        إذا (!بہترین_Thread.خالی_تھاواں()) s_mtaskState[بہترین_Thread.پٹھ()].nextp = mtaskp;

        // Add the MTask to the schedule
        بہترین_Thread.ڈالو_پٹھ(mtaskp);
        أرجع بہترین_سرا_ویلا;
    }
    منطقي شامل_اے(ثابت ExecMTask* mtaskp) ثابت { أرجع mtasks.گنتی(mtaskp); }
    uint32_t سرا_ویلا() ثابت { أرجع m_سرا_ویلا; }
};

uint32_t Thread_ویلا_سارنی::s_اگلا_Id = 0;
std::unordered_map<ثابت ExecMTask*, Thread_ویلا_سارنی::MTaskState> Thread_ویلا_سارنی::s_mtaskState{};
constexpr مزدوج V3_Exec_گراف::Thread_ویلا_سارنی::s_thread_Box_چوڑائی;

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
صنف Pack_دھاگے final {
    // TYPES
    بنية MTaskCmp final {
        منطقي عامل()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            أرجع ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_n_دھاگے;  // Number of threads
    ثابت uint32_t m_n_Hier_دھاگے;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    واضح Pack_دھاگے(uint32_t n_دھاگے = v3Global.opt.دھاگے(),
                         uint32_t n_Hier_دھاگے = v3Global.opt.hier_دھاگے(),
                         بنانشانی sandbagNumerator = 30, بنانشانی sandbagDenom = 100)
        : m_n_دھاگے{n_دھاگے}
        , m_n_Hier_دھاگے{n_Hier_دھاگے}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_دھاگے() = افتراضي;
    VL_UNCOPYABLE(Pack_دھاگے);

    // METHODS
    uint32_t completion_ویلا(ثابت Thread_ویلا_سارنی& ویلا_سارنی, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        إذا (!ویلا_سارنی.شامل_اے(mtaskp)) أرجع 0;
        ثابت Thread_ویلا_سارنی::MTaskState& state = ویلا_سارنی.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_ویلا_سارنی::UNASSIGNED, "Mtask should have assigned thread");
        إذا (threadId == state.threadId) {
            // No overhead on same thread
            أرجع state.completion_ویلا;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_سرا_ویلا
            = state.completion_ویلا + (m_sandbagNumerator * mtaskp->لاگت()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        إذا (state.nextp) {
            ثابت uint32_t successor_سرا_ویلا
                = completion_ویلا(ویلا_سارنی, state.nextp, state.threadId);
            إذا ((sandbagged_سرا_ویلا >= successor_سرا_ویلا) && (successor_سرا_ویلا > 1)) {
                sandbagged_سرا_ویلا = successor_سرا_ویلا - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_سرا_ویلا);
        أرجع sandbagged_سرا_ویلا;
    }

    ساكن منطقي isReady(Thread_ویلا_سارنی& ویلا_سارنی, ثابت ExecMTask* mtaskp) {
        لكل (ثابت V3_گراف_کنارا& کنارا_وچ : mtaskp->وچ_کنارے()) {
            ثابت ExecMTask* ثابت prevp = کنارا_وچ.fromp()->as<ثابت ExecMTask>();
            إذا (ویلا_سارنی.threadId(prevp) == Thread_ویلا_سارنی::UNASSIGNED) {
                // This predecessor is not assigned yet
                أرجع خطأ;
            }
        }
        أرجع سچ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::سمتیہ<Thread_ویلا_سارنی> pack(V3_گراف& mtask_گراف) {
        std::سمتیہ<Thread_ویلا_سارنی> نتیجہ;
        نتیجہ.emplace_پٹھ(Thread_ویلا_سارنی{m_n_دھاگے});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        تعداد صنف Scheduling_منوال : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_منوال منوال = Scheduling_منوال::SCHEDULING;

        // Time each thread is occupied until
        std::سمتیہ<uint32_t> busyUntil(std::max(m_n_دھاگے, m_n_Hier_دھاگے), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::سیٹ<ExecMTask*, MTaskCmp> readyMTasks;
        صحيح maxThreadWorkers = 1;

        // Build initial ready list
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            إذا (isReady(نتیجہ.پٹھ(), mtaskp)) readyMTasks.داخل_کرو(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->دھاگے());
        }

        طالما (!readyMTasks.خالی_تھاواں()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t بہترین_ویلا = 0xffffffff;
            uint32_t بہترین_Thread_Id = 0;
            ExecMTask* بہترین_Mtaskp = خالی_سوچک;  // Todo: const ExecMTask*
            Thread_ویلا_سارنی& ویلا_سارنی = نتیجہ.پٹھ();
            لكل (uint32_t threadId = 0; threadId < ویلا_سارنی.m_دھاگے.ناپ(); ++threadId) {
                لكل (ExecMTask* ثابت mtaskp : readyMTasks) {
                    إذا (منوال != Scheduling_منوال::WIDE_TASK_SCHEDULING && mtaskp->دھاگے() > 1) {
                        منوال = Scheduling_منوال::WIDE_TASK_DISCOVERED;
                        استمر;
                    }
                    إذا (منوال == Scheduling_منوال::WIDE_TASK_SCHEDULING && mtaskp->دھاگے() <= 1)
                        استمر;

                    uint32_t ویلا_Begin = busyUntil[threadId];
                    إذا (ویلا_Begin > بہترین_ویلا) {
                        UINFO(6, "th " << threadId << " busy until " << ویلا_Begin
                                       << ", later than bestTime " << بہترین_ویلا
                                       << ", skipping thread.");
                        اكسر;
                    }
                    لكل (ثابت V3_گراف_کنارا& کنارا : mtaskp->وچ_کنارے()) {
                        ثابت ExecMTask* ثابت priorp = کنارا.fromp()->as<ExecMTask>();
                        ثابت uint32_t پہلوں_سرا_ویلا = completion_ویلا(ویلا_سارنی, priorp, threadId);
                        إذا (پہلوں_سرا_ویلا > ویلا_Begin) ویلا_Begin = پہلوں_سرا_ویلا;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << ویلا_Begin
                                     << " on thread " << threadId);
                    إذا ((ویلا_Begin < بہترین_ویلا)
                        || ((ویلا_Begin == بہترین_ویلا)
                            && بہترین_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ترجیح_2() > بہترین_Mtaskp->ترجیح_2()))) {
                        بہترین_ویلا = ویلا_Begin;
                        بہترین_Thread_Id = threadId;
                        بہترین_Mtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t سرا_ویلا = ویلا_سارنی.سرا_ویلا();

            إذا (!بہترین_Mtaskp && منوال == Scheduling_منوال::WIDE_TASK_DISCOVERED) {
                منوال = Scheduling_منوال::WIDE_TASK_SCHEDULING;
                ثابت uint32_t ناپ = m_n_Hier_دھاگے / maxThreadWorkers;
                UASSERT(ناپ, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                إذا (ویلا_سارنی.s_mtaskState.خالی_تھاواں()) نتیجہ.clear();
                نتیجہ.emplace_پٹھ(Thread_ویلا_سارنی{ناپ});
                std::fill(busyUntil.begin(), busyUntil.سرا(), سرا_ویلا);
                استمر;
            }

            إذا (!بہترین_Mtaskp && منوال == Scheduling_منوال::WIDE_TASK_SCHEDULING) {
                منوال = Scheduling_منوال::SCHEDULING;
                UASSERT(!ویلا_سارنی.s_mtaskState.خالی_تھاواں(), "Mtask should be added");
                نتیجہ.emplace_پٹھ(Thread_ویلا_سارنی{m_n_دھاگے});
                std::fill(busyUntil.begin(), busyUntil.سرا(), سرا_ویلا);
                استمر;
            }

            UASSERT(بہترین_Mtaskp, "Should have found some task");

            بہترین_Mtaskp->predict_شروع(بہترین_ویلا);
            ثابت uint32_t بہترین_سرا_ویلا = ویلا_سارنی.ویلا_سارنی_On(بہترین_Mtaskp, بہترین_Thread_Id);
            busyUntil[بہترین_Thread_Id] = بہترین_سرا_ویلا;

            // Update the ready list
            ثابت ناپ_t erased = readyMTasks.erase(بہترین_Mtaskp);
            UASSERT_OBJ(erased > 0, بہترین_Mtaskp, "Should have erased something?");
            لكل (ثابت V3_گراف_کنارا& کنارا_باہرلا : بہترین_Mtaskp->باہرلا_کنارے()) {
                ExecMTask* ثابت nextp = کنارا_باہرلا.چوٹی()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(ویلا_سارنی.threadId(nextp) == Thread_ویلا_سارنی::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.لبھو(nextp) == readyMTasks.سرا(), nextp,
                            "Tasks after one being assigned should not be ready");
                إذا (isReady(ویلا_سارنی, nextp)) {
                    readyMTasks.داخل_کرو(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        إذا (dump_گراف_Level() >= 4)
            Thread_ویلا_سارنی::dumpDotFilePrefixedAlways(نتیجہ, "schedule", m_n_دھاگے);

        أرجع نتیجہ;
    }

عام:
    // SELF TEST
    ساكن فراغ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->لبھو_UInt64_DType()};
        AstScope* ثابت scopep = جديد AstScope{flp, خالی_سوچک, "test", خالی_سوچک, خالی_سوچک};
        AstVarScope* ثابت trigVscp = جديد AstVarScope{flp, scopep, varp};
        self_Test_Hier_پہلا(trigVscp);
        self_Test_عام_پہلا(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_درخت(), trigVscp);
        VL_DO_DANGLING(scopep->delete_درخت(), scopep);
        VL_DO_DANGLING(varp->delete_درخت(), varp);
    }
    ساكن فراغ self_Test_عام_پہلا(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گرہ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t0->لاگت(1000);
        t0->ترجیح_2(1100);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t1->لاگت(100);
        t1->ترجیح_2(100);
        ExecMTask* ثابت t2 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t2->لاگت(100);
        t2->ترجیح_2(100);
        t2->دھاگے(2);
        ExecMTask* ثابت t3 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t3->لاگت(100);
        t3->ترجیح_2(100);
        t3->دھاگے(3);
        ExecMTask* ثابت t4 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t4->لاگت(100);
        t4->ترجیح_2(100);
        t4->دھاگے(3);
        ExecMTask* ثابت t5 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t5->لاگت(100);
        t5->ترجیح_2(100);
        ExecMTask* ثابت t6 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t6->لاگت(100);
        t6->ترجیح_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        جديد V3_گراف_کنارا{&گراف, t0, t1, 1};
        جديد V3_گراف_کنارا{&گراف, t0, t2, 1};
        جديد V3_گراف_کنارا{&گراف, t2, t3, 1};
        جديد V3_گراف_کنارا{&گراف, t2, t4, 1};
        جديد V3_گراف_کنارا{&گراف, t3, t5, 1};
        جديد V3_گراف_کنارا{&گراف, t4, t6, 1};

        constexpr uint32_t دھاگے = 2;
        constexpr uint32_t hier_دھاگے = 6;
        Pack_دھاگے packer{دھاگے, hier_دھاگے,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::سمتیہ<Thread_ویلا_سارنی> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.ناپ(), 3);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے.ناپ(), دھاگے);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0].ناپ(), 2);
        لكل (ناپ_t i = 1; i < scheduled[0].m_دھاگے.ناپ(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_دھاگے[i].ناپ(), 0);

        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_دھاگے.ناپ(), hier_دھاگے / 3);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_دھاگے.ناپ(), دھاگے);
        UASSERT_SELFTEST(scheduled[2].m_دھاگے[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_دھاگے[1][0], t6);

        UASSERT_SELFTEST(Thread_ویلا_سارنی::s_mtaskState.ناپ(), 7);

        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t4, 5), 1360);

        لكل (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_ویلا_سارنی::s_mtaskState.clear();
    }
    ساكن فراغ self_Test_Hier_پہلا(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گرہ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t0->لاگت(1000);
        t0->ترجیح_2(1100);
        t0->دھاگے(2);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, خالی_سوچک, makeBody()};
        t1->لاگت(100);
        t1->ترجیح_2(100);

        /*
                          0
                          |
                          1
        */
        جديد V3_گراف_کنارا{&گراف, t0, t1, 1};

        constexpr uint32_t دھاگے = 1;
        constexpr uint32_t hier_دھاگے = 2;
        Pack_دھاگے packer{دھاگے, hier_دھاگے,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::سمتیہ<Thread_ویلا_سارنی> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.ناپ(), 2);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے.ناپ(), hier_دھاگے / 2);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0].ناپ(), 1);
        لكل (ناپ_t i = 1; i < scheduled[0].m_دھاگے.ناپ(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_دھاگے[i].ناپ(), 0);

        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_دھاگے.ناپ(), دھاگے);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0].ناپ(), 1);
        لكل (ناپ_t i = 1; i < scheduled[1].m_دھاگے.ناپ(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_دھاگے[i].ناپ(), 0);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][0], t1);

        UASSERT_SELFTEST(Thread_ویلا_سارنی::s_mtaskState.ناپ(), 2);

        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ویلا_سارنی::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_ویلا(scheduled[1], t1, 1), 1130);

        لكل (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_ویلا_سارنی::s_mtaskState.clear();
    }

    ساكن std::سمتیہ<Thread_ویلا_سارنی> apply(V3_گراف& mtask_گراف) {
        أرجع Pack_دھاگے{}.pack(mtask_گراف);
    }
};

ورتوں EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
ورتوں Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

فراغ normalizeCosts(Costs& costs) {
    ثابت خودکار scale_لاگت = [](uint64_t قدر, مزدوج multiplier) {
        مزدوج scaled = static_cast<مزدوج>(قدر) * multiplier;
        إذا (قدر && scaled < 1) scaled = 1;
        أرجع static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t جوڑ_لاگت_Profiled = 0;  // For data with estimate and profile
    uint64_t جوڑ_لاگت_Estimate = 0;  // For data with estimate and profile
    لكل (ثابت خودکار& est : costs) {
        إذا (est.دوجا.دوجا) {
            جوڑ_لاگت_Estimate += est.دوجا.پہلا;
            جوڑ_لاگت_Profiled += est.دوجا.دوجا;
        }
    }

    إذا (جوڑ_لاگت_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت مزدوج est_منزل_Profile
            = static_cast<مزدوج>(جوڑ_لاگت_Profiled) / static_cast<مزدوج>(جوڑ_لاگت_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_منزل_Profile
                                                    << ", sumCostProfiled=" << جوڑ_لاگت_Profiled
                                                    << " sumCostEstimate=" << جوڑ_لاگت_Estimate);
        لكل (خودکار& est : costs) {
            uint64_t& لاگت_Estimate = est.دوجا.پہلا;
            لاگت_Estimate = scale_لاگت(لاگت_Estimate, est_منزل_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_لاگت = 0;
    لكل (خودکار& est : costs) {
        ثابت uint64_t& لاگت_Estimate = est.دوجا.پہلا;
        ثابت uint64_t& لاگت_Profiled = est.دوجا.دوجا;
        إذا (max_لاگت < لاگت_Estimate) max_لاگت = لاگت_Estimate;
        إذا (max_لاگت < لاگت_Profiled) max_لاگت = لاگت_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.دوجا.پہلا << " cp=" << est.دوجا.دوجا);
    }
    constexpr uint64_t scale_تھلے_منزل = 10000000;  // Extra room for future algorithms to add costs
    إذا (max_لاگت > scale_تھلے_منزل) {
        ثابت مزدوج scaleup = static_cast<مزدوج>(scale_تھلے_منزل) / static_cast<مزدوج>(max_لاگت);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_لاگت);
        لكل (خودکار& est : costs) {
            est.دوجا.پہلا = scale_لاگت(est.دوجا.پہلا, scaleup);
            est.دوجا.دوجا = scale_لاگت(est.دوجا.دوجا, scaleup);
        }
    }
}

فراغ remove_خالی_تھاواں_MTasks(Ast_Exec_گراف* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    لكل (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        إذا (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_پٹھ()->delete_درخت(), callp);
    }
    V3_گراف* ثابت execMTaskGraphp = execGraphp->depGraphp();
    لكل (V3_گراف_راس* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        إذا (funcp->stmtsp()) استمر;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_کنارے(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_پٹھ()->delete_درخت(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);
}

فراغ fillinCosts(V3_گراف* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    لكل (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t لاگت_Estimate = V3_Instr_گنتی::گنتی(mtp->funcp(), خطأ);
        ثابت uint64_t لاگت_Profiled
            = V3Control::get_Profile_انکڑے(v3Global.opt.اگیتر(), mtp->hashName());
        إذا (لاگت_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << لاگت_Profiled);
        }
        costs[mtp->id()] = std::make_pair(لاگت_Estimate, لاگت_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    صحيح کل_Estimates = 0;
    صحيح missingProfiles = 0;
    لكل (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t لاگت_Estimate = costs[mtp->id()].پہلا;
        ثابت uint64_t لاگت_Profiled = costs[mtp->id()].دوجا;
        UINFO(9, "ce = " << لاگت_Estimate << " cp=" << لاگت_Profiled);
        UASSERT(لاگت_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(لاگت_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t لاگت_Profiled32 = static_cast<uint32_t>(لاگت_Profiled);
        uint32_t لاگت_منزل_Use = لاگت_Profiled32;
        إذا (!لاگت_Profiled32) {
            لاگت_منزل_Use = لاگت_Estimate;
            إذا (لاگت_Estimate != 0) ++missingProfiles;
        }
        إذا (لاگت_Estimate != 0) ++کل_Estimates;
        mtp->لاگت(لاگت_منزل_Use);
        mtp->ترجیح_2(لاگت_منزل_Use);
    }

    إذا (missingProfiles) {
        إذا (File_سطر* ثابت fl = V3Control::get_Profile_انکڑے_File_سطر()) {
            إذا (V3Control::شامل_اے_MTask_Profile_انکڑے()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << کل_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

فراغ finalizeCosts(V3_گراف* execMTaskGraphp) {
    گراف_دھارا_Unordered ser(execMTaskGraphp, گراف_Way::الٹ_2);
    طالما (ثابت V3_گراف_راس* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3_گراف_راس*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        لكل (V3_گراف_کنارا& کنارا : mtp->باہرلا_کنارے()) {
            ثابت ExecMTask* ثابت followp = کنارا.چوٹی()->as<ExecMTask>();
            إذا ((followp->ترجیح_2() + mtp->لاگت()) > mtp->ترجیح_2()) {
                mtp->ترجیح_2(followp->ترجیح_2() + mtp->لاگت());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_متعدی_کنارے();

    // Record summary stats for final m_tasks graph.
    ثابت خودکار report = execMTaskGraphp->parallelismReport([](ثابت V3_گراف_راس* vtxp) {  //
        أرجع vtxp->as<ثابت ExecMTask>()->لاگت();
    });
    V3Stats::جوڑو_Stat("MTask graph, final, critical path cost", report.critical_رستہ_لاگت());
    V3Stats::جوڑو_Stat("MTask graph, final, total graph cost", report.کل_گراف_لاگت());
    V3Stats::جوڑو_Stat("MTask graph, final, mtask count", report.راس_گنتی());
    V3Stats::جوڑو_Stat("MTask graph, final, edge count", report.کنارا_گنتی());
    V3Stats::جوڑو_Stat("MTask graph, final, parallelism factor", report.parallelism_ضارب());
    إذا (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_رستہ_لاگت());
        UINFO(0, "    Total graph cost = " << report.کل_گراف_لاگت());
        UINFO(0, "    MTask vertex count = " << report.راس_گنتی());
        UINFO(0, "    Edge count = " << report.کنارا_گنتی());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ضارب());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->لبھو_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(سچ);
    varp->valuep(جديد AstConst{fl, nDependencies});
    modp->جوڑو_Stmtsp(varp);
    أرجع varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& طریقہ) {
    File_سطر* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت چوٹی_Scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();
    refp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::VlSyms{}, چوٹی_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = جديد AstCStmt{fl};
    stmtp->جوڑو(refp);
    stmtp->جوڑو("." + طریقہ + ";");
    أرجع stmtp;
}

فراغ جوڑو_MTask_منزل_Function(ثابت Thread_ویلا_سارنی& ویلا_سارنی, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        funcp->جوڑو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (ویلا_سارنی.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        إذا (v3Global.opt.profExec()) {
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->جوڑو_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        إذا (v3Global.opt.profExec()) {
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    إذا (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        جوڑو_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = جديد AstCCall{fl, mtaskp->funcp()};
    callp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::VlSyms{}, scopep->nameDotless()});
    callp->dtype_سیٹ_Void();
    funcp->جوڑو_Stmtsp(callp->makeStmt());

    إذا (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        جوڑو_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    لكل (ثابت V3_گراف_کنارا& کنارا : mtaskp->باہرلا_کنارے()) {
        ثابت ExecMTask* ثابت nextp = کنارا.چوٹی()->as<ExecMTask>();
        إذا (ویلا_سارنی.threadId(nextp) != threadId && ویلا_سارنی.شامل_اے(nextp)) {
            funcp->جوڑو_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::سمتیہ<AstCFunc*> createThreadFunctions(ثابت Thread_ویلا_سارنی& ویلا_سارنی,
                                                   ثابت string& ٹیگ) {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    std::سمتیہ<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    لكل (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : ویلا_سارنی.m_دھاگے) {
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            إذا (ثابت uint32_t nDependencies = ویلا_سارنی.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_منزل_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    لكل (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : ویلا_سارنی.m_دھاگے) {
        إذا (thread.خالی_تھاواں()) استمر;
        ثابت uint32_t threadId = ویلا_سارنی.threadId(thread.front());
        ثابت string name{"__Vthread__" + ٹیگ + "__s" + cvt_منزل_Str(ویلا_سارنی.id()) + "__t"
                          + cvt_منزل_Str(threadId)};
        AstCFunc* ثابت funcp = جديد AstCFunc{fl, name, خالی_سوچک, "void"};
        modp->جوڑو_Stmtsp(funcp);
        funcps.ڈالو_پٹھ(funcp);
        funcp->isStatic(سچ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(سچ);
        funcp->اندراج_نقطہ(سچ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->جوڑو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_سونپو(modp)});
        funcp->جوڑو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_سونپو()});

        // Invoke each mtask scheduled to this thread from the thread function
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            جوڑو_MTask_منزل_Function(ویلا_سارنی, threadId, funcp, mtaskp, stateVars);
        }
    }

    أرجع funcps;
}

فراغ جوڑو_Thread_شروع_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& ٹیگ = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جوڑو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    جوڑو_CStmt("vlSymsp->__Vm_even_cycle__" + ٹیگ + " = !vlSymsp->__Vm_even_cycle__" + ٹیگ + ";");

    إذا (!v3Global.opt.hier_بلاکاں().خالی_تھاواں()) جوڑو_CStmt("std::vector<size_t> indexes;");
}

فراغ جوڑو_Thread_سرا_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->جوڑو_Stmtsp(جديد AstCStmt{flp, stmt});
    };

    جوڑو_CStmt("Verilated::mtaskId(0);");
    إذا (v3Global.opt.profExec()) { جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
فراغ جوڑو_Thread_شروع_منزل_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp,
                               ثابت std::سمتیہ<AstCFunc*>& funcps, uint32_t ویلا_سارنی_Id) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& ٹیگ = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvt_منزل_Str(ویلا_سارنی_Id) + "__" + ٹیگ, funcps.ناپ());
    لكل (AstCFunc* ثابت funcp : funcps) {
        funcp->جوڑو_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جوڑو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    ثابت uint32_t آخری = funcps.ناپ() - 1;
    إذا (!v3Global.opt.hier_بلاکاں().خالی_تھاواں() && آخری > 0) {
        جوڑو_CStmt("for (size_t i = 0; i < " + std::منزل_string(آخری) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    لكل (AstCFunc* ثابت funcp : funcps) {
        إذا (i != آخری) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            execGraphp->runfuncp()->جوڑو_Stmtsp(cstmtp);
            cstmtp->جوڑو("vlSymsp->__Vm_threadPoolp->workerp(");
            إذا (v3Global.opt.hier_بچہ() || !v3Global.opt.hier_بلاکاں().خالی_تھاواں()) {
                cstmtp->جوڑو("indexes[" + std::منزل_string(i) + "]");
            } وإلا {
                cstmtp->جوڑو(std::منزل_string(i));
            }
            cstmtp->جوڑو(")->addTask(");
            cstmtp->جوڑو(جديد Ast_Addr_دا_CFunc{fl, funcp});
            cstmtp->جوڑو(", vlSelf, vlSymsp->__Vm_even_cycle__" + ٹیگ + ");");
        } وإلا {
            // The last will run on the main thread.
            AstCCall* ثابت callp = جديد AstCCall{fl, funcp};
            callp->dtype_سیٹ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + ٹیگ);
            execGraphp->runfuncp()->جوڑو_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::جوڑو_Stat_جوڑ("Optimizations, Thread schedule total tasks", i);

    إذا (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->جوڑو_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + ٹیگ + ")"));
    إذا (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    إذا (!v3Global.opt.hier_بلاکاں().خالی_تھاواں() && آخری > 0) {
        جوڑو_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

فراغ processMTaskBodies(Ast_Exec_گراف* ثابت execGraphp) {
    لكل (V3_گراف_راس* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_گرہ* stmtsp = funcp->stmtsp()->unlink_Fr_پٹھ_With_اگلا();

        // Helper function to make the code a bit more legible
        ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
            funcp->جوڑو_Stmtsp(جديد AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        إذا (v3Global.opt.profExec()) {
            std::string دلیلاں = std::منزل_string(mtaskp->id());
            دلیلاں += ", " + std::منزل_string(mtaskp->predict_شروع());
            دلیلاں += ", \"";
            إذا (v3Global.opt.hier_بچہ()) دلیلاں += v3Global.opt.چوٹی_Module();
            دلیلاں += "\"";
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + دلیلاں + ");");
        }
        // Set mtask ID in the run-time system
        جوڑو_CStmt("Verilated::mtaskId(" + std::منزل_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->جوڑو_Stmtsp(stmtsp);
        // Flush message queue
        جوڑو_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        إذا (v3Global.opt.profExec()) {
            ثابت std::string& دلیلاں = std::منزل_string(mtaskp->لاگت());
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + دلیلاں + ");");
        }
    }
}

فراغ implement_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp, ثابت Thread_ویلا_سارنی& ویلا_سارنی) {
    // Nothing to be done if there are no MTasks in the graph at all.
    إذا (execGraphp->depGraphp()->خالی_تھاواں()) أرجع;

    // Create a function to be run by each thread.
    ثابت std::سمتیہ<AstCFunc*>& funcps = createThreadFunctions(ویلا_سارنی, execGraphp->name());
    UASSERT(!funcps.خالی_تھاواں(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    جوڑو_Thread_شروع_منزل_Exec_گراف(execGraphp, funcps, ویلا_سارنی.id());
}

فراغ create_لگاتار_Function(Ast_Exec_گراف* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_سطر* ثابت flp = execGraphp->fileline();
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    AstCFunc* ثابت funcp = جديد AstCFunc{flp, "runExecGraph_" + execGraphp->name(), خالی_سوچک};
    funcp->isLoose(سچ);
    funcp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(سچ);
    modp->جوڑو_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

فراغ جوڑو_Trigger_Masks(Ast_Exec_گراف* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_سطر* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> لفظ_Masks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            لكل (ثابت uint32_t اشاریہ : trigp->indices()) {
                لفظ_Masks[VL_BITWORD_Q(اشاریہ)] |= VL_ثنائی_ہندسہ_Q(اشاریہ);
            }
        });
        AstConsArray* ثابت consp = جديد AstConsArray{flp, triggersp->dtypep(),
                                                     جديد AstConst{flp, AstConst::Unsized64{}, 0}};
        لكل (ثابت خودکار& لفظ_نقاب : لفظ_Masks) {
            consp->valuep(لفظ_نقاب.پہلا,
                          جديد AstConst{flp, AstConst::Unsized64{}, لفظ_نقاب.دوجا});
        }
        execGraphp->جوڑو_Maskps(V3ConstPool::لبھو(consp));
        VL_DO_DANGLING(consp->delete_درخت(), consp);
    }
}

فراغ جوڑو_Trigger_Costs(Ast_Exec_گراف* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_سطر* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::سمتیہ<uint32_t> costs(nTriggers, 0);
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            إذا (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t لاگت = V3_Instr_گنتی::گنتی(ifp, خطأ);
                لكل (ثابت uint32_t اشاریہ : trigp->indices()) costs.at(اشاریہ) += لاگت;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = جديد AstUnpackArrayDType{flp, netlistp->لبھو_ثنائی_ہندسہ_DType(32, 32, VSigning::UNSIGNED),
                                  جديد AstRange{flp, static_cast<صحيح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->جوڑو_Typesp(dtypep);
    AstConsArray* ثابت consp = جديد AstConsArray{flp, dtypep, جديد AstConst{flp, 0}};
    لكل (uint32_t اشاریہ = 0; اشاریہ < nTriggers; ++اشاریہ) {
        إذا (costs[اشاریہ]) consp->valuep(اشاریہ, جديد AstConst{flp, costs[اشاریہ]});
    }
    execGraphp->triggerCostsp(V3ConstPool::لبھو(consp));
    VL_DO_DANGLING(consp->delete_درخت(), consp);
}

// Called by Verilator top stage
فراغ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::سمتیہ<Ast_Exec_گراف*> execGraphps;
    netlistp->چوٹی_Modulep()->foreach([&](Ast_Exec_گراف* egp) { execGraphps.emplace_پٹھ(egp); });

    // Process each
    لكل (Ast_Exec_گراف* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_خالی_تھاواں_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        إذا (execGraphp->depGraphp()->خالی_تھاواں()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_پٹھ()->delete_درخت(), execGraphp);
            أرجع;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        إذا (dump_گراف_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_لگاتار_Function(execGraphp);
        جوڑو_Thread_شروع_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::سمتیہ<Thread_ویلا_سارنی> packed = Pack_دھاگے::apply(*execGraphp->depGraphp());
        V3Stats::جوڑو_Stat_جوڑ("Optimizations, Thread schedule count",
                            static_cast<مزدوج>(packed.ناپ()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        لكل (ثابت Thread_ویلا_سارنی& ویلا_سارنی : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_گراف(execGraphp, ویلا_سارنی);
        }

        جوڑو_Thread_سرا_Wrapper(execGraphp);

        جوڑو_Trigger_Masks(execGraphp);
        جوڑو_Trigger_Costs(execGraphp);
    }
}

فراغ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پہلا, 1000);
        UASSERT_SELFTEST(costs[1].دوجا, 1000);
        UASSERT_SELFTEST(costs[2].پہلا, 2000);
        UASSERT_SELFTEST(costs[2].دوجا, 0);
        UASSERT_SELFTEST(costs[3].پہلا, 3000);
        UASSERT_SELFTEST(costs[3].دوجا, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پہلا, 2500000);
        UASSERT_SELFTEST(costs[1].دوجا, 5000000);
        UASSERT_SELFTEST(costs[2].پہلا, 5000000);
        UASSERT_SELFTEST(costs[2].دوجا, 10000000);
        UASSERT_SELFTEST(costs[3].پہلا, 7500000);
        UASSERT_SELFTEST(costs[3].دوجا, 1);
    }

    Pack_دھاگے::selfTest();
}

}  // namespace V3ExecGraph
