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

AstCFunc* ExecMTask::createCFunc(Ast_Exec_ګراف* execGraphp, AstScope* scopep, Ast_غوټه_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::منزل_string(id);
    AstCFunc* ثابت newp = جديد AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(ریښتیا);
    newp->voidSelf(ریښتیا);  // Called through VlExecGraph::Fnp
    newp->dontCombine(ریښتیا);
    newp->جمع_کړه_Stmtsp(stmtsp);
    إذا (scopep) scopep->جمع_کړه_Blocksp(newp);
    أرجع newp;
}

ExecMTask::ExecMTask(Ast_Exec_ګراف* execGraphp, AstScope* scopep,
                     Ast_غوټه_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_ګراف_راس{execGraphp->depGraphp()},
      m_id{s_بل_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).منزل_String()} {}

فراغ ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvt_منزل_Hex(هذا);
    إذا (لومړیتوب_2() || لګښت()) str << " [pr=" << لومړیتوب_2() << " c=" << cvt_منزل_Str(لګښت()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_بل_Id{0};

نطاق V3_Exec_ګراف {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
صنف Thread_مهالوېش final {
    ملګری صنف Pack_تارونه;

    uint32_t m_id;  // Unique ID of a schedule
    ساكن uint32_t s_بل_Id;  // Next ID number to use
    std::unordered_سېټ<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_پای_ټکی_وخت = 0;  // Latest task end time in this schedule

عام:
    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    بنية MTaskState final {
        uint32_t completion_وخت = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = تش_اشاره;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::ویکټور<std::ویکټور<ثابت ExecMTask*>> m_تارونه;

    // Global state for each mtask.
    ساكن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    څرګند Thread_مهالوېش(uint32_t n_تارونه)
        : m_id{s_بل_Id++}
        , m_تارونه{n_تارونه} {}
    Thread_مهالوېش(Thread_مهالوېش&&) = افتراضي;
    Thread_مهالوېش& چلوونکی=(Thread_مهالوېش&&) = افتراضي;

خاص:
    VL_UNCOPYABLE(Thread_مهالوېش);

    ساكن constexpr مزدوج s_thread_Box_پلنوالی = 2.5;
    ساكن constexpr مزدوج s_thread_Box_لوړوالی = 1.5;
    ساكن constexpr مزدوج s_horizontalGap = s_thread_Box_پلنوالی / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساكن فراغ dumpDotFilePrefixedAlways(ثابت std::ویکټور<Thread_مهالوېش>& schedules,
                                          ثابت string& nameComment, uint32_t n_تارونه) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_تارونه);
    }
    ساكن فراغ dumpDotFile(ثابت std::ویکټور<Thread_مهالوېش>& schedules, ثابت string& filename,
                            uint32_t n_تارونه) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        إذا (logp->fail()) v3fatal("Can't write file: " << filename);

        //سرلیک
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        لكل (uint32_t i = 0; i < n_تارونه; ++i) {
            ثابت string name = "t" + std::منزل_string(i);
            ثابت string label = "Thread " + std::منزل_string(i);
            constexpr مزدوج posX = -s_horizontalGap;
            ثابت مزدوج posY = -static_cast<مزدوج>(i) * s_thread_Box_لوړوالی;
            dump_Dot_File_Emit_بلاک(logp, name, label, s_thread_Box_پلنوالی, s_thread_Box_لوړوالی, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_لګښت = 0;
        لكل (ثابت خپلکاری& state : Thread_مهالوېش::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.لومړی;
            max_لګښت = std::max(max_لګښت, mtaskp->لګښت());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_فی_Longest_Mtask = 4;
        ثابت uint32_t segment_لګښت = max_لګښت / segments_فی_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::ویکټور<مزدوج> offsets(n_تارونه, 0.0);
        لكل (ثابت Thread_مهالوېش& مهالوېش : schedules) {
            إذا (مهالوېش.mtasks.تشه()) استمر;
            کارول ستنه = std::ویکټور<ثابت ExecMTask*>;
            std::ویکټور<ستنه> columns = {{}};

            // Order tasks based on their start time
            بنية Cmp final {
                منطقي چلوونکی()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    إذا (پیل_وخت(a) == پیل_وخت(b)) أرجع threadId(a) < threadId(b);
                    أرجع پیل_وخت(a) < پیل_وخت(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(مهالوېش.mtasks.begin(),
                                                             مهالوېش.mtasks.پای_ټکی());

            لكل (ثابت ExecMTask* ثابت mtaskp : tasks) {
                ستنه& ستنه_2 = columns.شا();
                UASSERT(ستنه_2.اندازه() <= n_تارونه, "Invalid partitioning");

                منطقي intersects = ریښتیا;
                لكل (ثابت ExecMTask* ثابت earlierMtask : ستنه_2) {
                    إذا (پای_ټکی_وخت(mtaskp) <= پیل_وخت(earlierMtask)
                        || پیل_وخت(mtaskp) >= پای_ټکی_وخت(earlierMtask)) {
                        intersects = خطأ;
                        اكسر;
                    }
                }
                إذا (intersects) {
                    ستنه_2.emplace_شا(mtaskp);
                } وإلا {
                    columns.emplace_شا(ستنه{mtaskp});
                }
            }

            UASSERT(!columns.front().تشه(), "Should be populated by mtasks");

            لكل (ثابت ستنه& ستنه_2 : columns) {
                مزدوج وروستی_ستنه_دننه_واټن = 0;
                لكل (ثابت ExecMTask* ثابت mtaskp : ستنه_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, مهالوېش, segment_لګښت, offsets);
                    وروستی_ستنه_دننه_واټن = std::max(وروستی_ستنه_دننه_واټن, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.پای_ټکی(), وروستی_ستنه_دننه_واټن);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_تارونه);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            لكل (ثابت std::ویکټور<ثابت ExecMTask*>& thread : مهالوېش.m_تارونه) {
                إذا (thread.تشه()) اكسر;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.شا()->name() << " -> fork_"
                      << static_cast<صحيح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                لكل (اندازه_t i = 1; i < thread.اندازه(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
                    لكل (ثابت V3_ګراف_څنډه& څنډه : mtaskp->بهر_څنډې()) {
                        ثابت ExecMTask* ثابت سر_MTaskp = څنډه.سر()->cast<ثابت ExecMTask>();
                        إذا (سر_MTaskp && مهالوېش.لري(سر_MTaskp)
                            && threadId(سر_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << سر_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ساكن فراغ dump_Dot_File_Emit_بلاک(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, مزدوج پلنوالی,
                                     مزدوج لوړوالی, مزدوج xPos, مزدوج yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << پلنوالی
              << " height=" << لوړوالی << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساكن فراغ dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت Thread_مهالوېش& مهالوېش,
                                     uint32_t segment_لګښت, std::ویکټور<مزدوج>& offsets) {
        لكل (صحيح i = 0; i < mtaskp->تارونه(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::منزل_string(i);
            ثابت string label = mtaskp->name() + " (" + std::منزل_string(پیل_وخت(mtaskp)) + ':'
                                 + std::منزل_string(پای_ټکی_وخت(mtaskp)) + ')'
                                 + "\\ncost=" + std::منزل_string(mtaskp->لګښت())
                                 + "\\npriority=" + std::منزل_string(mtaskp->لومړیتوب_2());
            ثابت مزدوج پلنوالی
                = std::max(s_thread_Box_پلنوالی,
                           s_thread_Box_پلنوالی * static_cast<مزدوج>(mtaskp->لګښت()) / segment_لګښت);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * مهالوېش.m_تارونه.اندازه();
            ثابت مزدوج xPos = پلنوالی / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += پلنوالی + s_horizontalGap;
            ثابت مزدوج yPos = -s_thread_Box_لوړوالی * static_cast<مزدوج>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_بلاک(logp, name, label, پلنوالی, s_thread_Box_لوړوالی, xPos, yPos,
                                 fillColor);
        }
    }

    ساكن فراغ dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, مزدوج دننه_واټن,
                                    uint32_t n_تارونه) {
        ثابت string& name = "fork_" + std::منزل_string(static_cast<صحيح>(دننه_واټن));
        constexpr مزدوج پلنوالی = s_thread_Box_پلنوالی / 8;
        ثابت مزدوج لوړوالی = s_thread_Box_لوړوالی * n_تارونه;
        ثابت مزدوج xPos = دننه_واټن - s_horizontalGap / 2;
        ثابت مزدوج yPos
            = -static_cast<مزدوج>(n_تارونه) / 2 * s_thread_Box_لوړوالی + s_thread_Box_لوړوالی / 2;
        dump_Dot_File_Emit_بلاک(logp, name, "", پلنوالی, لوړوالی, xPos, yPos, "black");
    }

عام:
    ساكن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت خپلکاری& it = s_mtaskState.پیدا_کړه(mtaskp);
        أرجع it != s_mtaskState.پای_ټکی() ? it->دوهم.threadId : UNASSIGNED;
    }
    ساكن uint32_t پیل_وخت(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_وخت - mtaskp->لګښت();
    }
    ساكن uint32_t پای_ټکی_وخت(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_وخت;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t پایله = 0;
        لكل (ثابت V3_ګراف_څنډه& څنډه : mtaskp->دننه_څنډې()) {
            ثابت ExecMTask* ثابت prevp = څنډه.fromp()->as<ExecMTask>();
            إذا (threadId(prevp) != thisThreadId && لري(prevp)) ++پایله;
        }
        أرجع پایله;
    }

    uint32_t id() ثابت { أرجع m_id; }
    uint32_t مهالوېش_On(ثابت ExecMTask* mtaskp, uint32_t غوره_Thread_Id) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t غوره_پای_ټکی_وخت = mtaskp->predict_پیل() + mtaskp->لګښت();
        m_پای_ټکی_وخت = std::max(m_پای_ټکی_وخت, غوره_پای_ټکی_وخت);
        s_mtaskState[mtaskp].completion_وخت = غوره_پای_ټکی_وخت;
        s_mtaskState[mtaskp].threadId = غوره_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::ویکټور<ثابت ExecMTask*>& غوره_Thread = m_تارونه[غوره_Thread_Id];
        إذا (!غوره_Thread.تشه()) s_mtaskState[غوره_Thread.شا()].nextp = mtaskp;

        // Add the MTask to the schedule
        غوره_Thread.ټېل_شا(mtaskp);
        أرجع غوره_پای_ټکی_وخت;
    }
    منطقي لري(ثابت ExecMTask* mtaskp) ثابت { أرجع mtasks.شمېر(mtaskp); }
    uint32_t پای_ټکی_وخت() ثابت { أرجع m_پای_ټکی_وخت; }
};

uint32_t Thread_مهالوېش::s_بل_Id = 0;
std::unordered_map<ثابت ExecMTask*, Thread_مهالوېش::MTaskState> Thread_مهالوېش::s_mtaskState{};
constexpr مزدوج V3_Exec_ګراف::Thread_مهالوېش::s_thread_Box_پلنوالی;

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
صنف Pack_تارونه final {
    // TYPES
    بنية MTaskCmp final {
        منطقي چلوونکی()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            أرجع ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_n_تارونه;  // Number of threads
    ثابت uint32_t m_n_Hier_تارونه;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    څرګند Pack_تارونه(uint32_t n_تارونه = v3Global.opt.تارونه(),
                         uint32_t n_Hier_تارونه = v3Global.opt.hier_تارونه(),
                         بینښه sandbagNumerator = 30, بینښه sandbagDenom = 100)
        : m_n_تارونه{n_تارونه}
        , m_n_Hier_تارونه{n_Hier_تارونه}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_تارونه() = افتراضي;
    VL_UNCOPYABLE(Pack_تارونه);

    // METHODS
    uint32_t completion_وخت(ثابت Thread_مهالوېش& مهالوېش, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        إذا (!مهالوېش.لري(mtaskp)) أرجع 0;
        ثابت Thread_مهالوېش::MTaskState& state = مهالوېش.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_مهالوېش::UNASSIGNED, "Mtask should have assigned thread");
        إذا (threadId == state.threadId) {
            // No overhead on same thread
            أرجع state.completion_وخت;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_پای_ټکی_وخت
            = state.completion_وخت + (m_sandbagNumerator * mtaskp->لګښت()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        إذا (state.nextp) {
            ثابت uint32_t successor_پای_ټکی_وخت
                = completion_وخت(مهالوېش, state.nextp, state.threadId);
            إذا ((sandbagged_پای_ټکی_وخت >= successor_پای_ټکی_وخت) && (successor_پای_ټکی_وخت > 1)) {
                sandbagged_پای_ټکی_وخت = successor_پای_ټکی_وخت - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_پای_ټکی_وخت);
        أرجع sandbagged_پای_ټکی_وخت;
    }

    ساكن منطقي isReady(Thread_مهالوېش& مهالوېش, ثابت ExecMTask* mtaskp) {
        لكل (ثابت V3_ګراف_څنډه& څنډه_دننه : mtaskp->دننه_څنډې()) {
            ثابت ExecMTask* ثابت prevp = څنډه_دننه.fromp()->as<ثابت ExecMTask>();
            إذا (مهالوېش.threadId(prevp) == Thread_مهالوېش::UNASSIGNED) {
                // This predecessor is not assigned yet
                أرجع خطأ;
            }
        }
        أرجع ریښتیا;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::ویکټور<Thread_مهالوېش> pack(V3_ګراف& mtask_ګراف) {
        std::ویکټور<Thread_مهالوېش> پایله;
        پایله.emplace_شا(Thread_مهالوېش{m_n_تارونه});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        تعداد صنف Scheduling_ډېرپېښ : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_ډېرپېښ ډېرپېښ = Scheduling_ډېرپېښ::SCHEDULING;

        // Time each thread is occupied until
        std::ویکټور<uint32_t> busyUntil(std::max(m_n_تارونه, m_n_Hier_تارونه), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::سېټ<ExecMTask*, MTaskCmp> readyMTasks;
        صحيح maxThreadWorkers = 1;

        // Build initial ready list
        لكل (V3_ګراف_راس& vtx : mtask_ګراف.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            إذا (isReady(پایله.شا(), mtaskp)) readyMTasks.ورننباسه(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->تارونه());
        }

        طالما (!readyMTasks.تشه()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t غوره_وخت = 0xffffffff;
            uint32_t غوره_Thread_Id = 0;
            ExecMTask* غوره_Mtaskp = تش_اشاره;  // Todo: const ExecMTask*
            Thread_مهالوېش& مهالوېش = پایله.شا();
            لكل (uint32_t threadId = 0; threadId < مهالوېش.m_تارونه.اندازه(); ++threadId) {
                لكل (ExecMTask* ثابت mtaskp : readyMTasks) {
                    إذا (ډېرپېښ != Scheduling_ډېرپېښ::WIDE_TASK_SCHEDULING && mtaskp->تارونه() > 1) {
                        ډېرپېښ = Scheduling_ډېرپېښ::WIDE_TASK_DISCOVERED;
                        استمر;
                    }
                    إذا (ډېرپېښ == Scheduling_ډېرپېښ::WIDE_TASK_SCHEDULING && mtaskp->تارونه() <= 1)
                        استمر;

                    uint32_t وخت_Begin = busyUntil[threadId];
                    إذا (وخت_Begin > غوره_وخت) {
                        UINFO(6, "th " << threadId << " busy until " << وخت_Begin
                                       << ", later than bestTime " << غوره_وخت
                                       << ", skipping thread.");
                        اكسر;
                    }
                    لكل (ثابت V3_ګراف_څنډه& څنډه : mtaskp->دننه_څنډې()) {
                        ثابت ExecMTask* ثابت priorp = څنډه.fromp()->as<ExecMTask>();
                        ثابت uint32_t مخکینی_پای_ټکی_وخت = completion_وخت(مهالوېش, priorp, threadId);
                        إذا (مخکینی_پای_ټکی_وخت > وخت_Begin) وخت_Begin = مخکینی_پای_ټکی_وخت;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << وخت_Begin
                                     << " on thread " << threadId);
                    إذا ((وخت_Begin < غوره_وخت)
                        || ((وخت_Begin == غوره_وخت)
                            && غوره_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->لومړیتوب_2() > غوره_Mtaskp->لومړیتوب_2()))) {
                        غوره_وخت = وخت_Begin;
                        غوره_Thread_Id = threadId;
                        غوره_Mtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t پای_ټکی_وخت = مهالوېش.پای_ټکی_وخت();

            إذا (!غوره_Mtaskp && ډېرپېښ == Scheduling_ډېرپېښ::WIDE_TASK_DISCOVERED) {
                ډېرپېښ = Scheduling_ډېرپېښ::WIDE_TASK_SCHEDULING;
                ثابت uint32_t اندازه = m_n_Hier_تارونه / maxThreadWorkers;
                UASSERT(اندازه, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                إذا (مهالوېش.s_mtaskState.تشه()) پایله.clear();
                پایله.emplace_شا(Thread_مهالوېش{اندازه});
                std::fill(busyUntil.begin(), busyUntil.پای_ټکی(), پای_ټکی_وخت);
                استمر;
            }

            إذا (!غوره_Mtaskp && ډېرپېښ == Scheduling_ډېرپېښ::WIDE_TASK_SCHEDULING) {
                ډېرپېښ = Scheduling_ډېرپېښ::SCHEDULING;
                UASSERT(!مهالوېش.s_mtaskState.تشه(), "Mtask should be added");
                پایله.emplace_شا(Thread_مهالوېش{m_n_تارونه});
                std::fill(busyUntil.begin(), busyUntil.پای_ټکی(), پای_ټکی_وخت);
                استمر;
            }

            UASSERT(غوره_Mtaskp, "Should have found some task");

            غوره_Mtaskp->predict_پیل(غوره_وخت);
            ثابت uint32_t غوره_پای_ټکی_وخت = مهالوېش.مهالوېش_On(غوره_Mtaskp, غوره_Thread_Id);
            busyUntil[غوره_Thread_Id] = غوره_پای_ټکی_وخت;

            // Update the ready list
            ثابت اندازه_t erased = readyMTasks.erase(غوره_Mtaskp);
            UASSERT_OBJ(erased > 0, غوره_Mtaskp, "Should have erased something?");
            لكل (ثابت V3_ګراف_څنډه& څنډه_بهر : غوره_Mtaskp->بهر_څنډې()) {
                ExecMTask* ثابت nextp = څنډه_بهر.سر()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(مهالوېش.threadId(nextp) == Thread_مهالوېش::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.پیدا_کړه(nextp) == readyMTasks.پای_ټکی(), nextp,
                            "Tasks after one being assigned should not be ready");
                إذا (isReady(مهالوېش, nextp)) {
                    readyMTasks.ورننباسه(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        إذا (dump_ګراف_Level() >= 4)
            Thread_مهالوېش::dumpDotFilePrefixedAlways(پایله, "schedule", m_n_تارونه);

        أرجع پایله;
    }

عام:
    // SELF TEST
    ساكن فراغ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_کرښه* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->پیدا_کړه_UInt64_DType()};
        AstScope* ثابت scopep = جديد AstScope{flp, تش_اشاره, "test", تش_اشاره, تش_اشاره};
        AstVarScope* ثابت trigVscp = جديد AstVarScope{flp, scopep, varp};
        self_Test_Hier_لومړی(trigVscp);
        self_Test_نورمال_لومړی(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_ونه(), trigVscp);
        VL_DO_DANGLING(scopep->delete_ونه(), scopep);
        VL_DO_DANGLING(varp->delete_ونه(), varp);
    }
    ساكن فراغ self_Test_نورمال_لومړی(AstVarScope* trigVscp) {
        File_کرښه* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_ګراف* ثابت execGraphp = جديد Ast_Exec_ګراف{flp, "test", trigVscp};
        V3_ګراف& ګراف = *execGraphp->depGraphp();
        ثابت خپلکاری makeBody = [&]() -> Ast_غوټه_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t0->لګښت(1000);
        t0->لومړیتوب_2(1100);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t1->لګښت(100);
        t1->لومړیتوب_2(100);
        ExecMTask* ثابت t2 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t2->لګښت(100);
        t2->لومړیتوب_2(100);
        t2->تارونه(2);
        ExecMTask* ثابت t3 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t3->لګښت(100);
        t3->لومړیتوب_2(100);
        t3->تارونه(3);
        ExecMTask* ثابت t4 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t4->لګښت(100);
        t4->لومړیتوب_2(100);
        t4->تارونه(3);
        ExecMTask* ثابت t5 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t5->لګښت(100);
        t5->لومړیتوب_2(100);
        ExecMTask* ثابت t6 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t6->لګښت(100);
        t6->لومړیتوب_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        جديد V3_ګراف_څنډه{&ګراف, t0, t1, 1};
        جديد V3_ګراف_څنډه{&ګراف, t0, t2, 1};
        جديد V3_ګراف_څنډه{&ګراف, t2, t3, 1};
        جديد V3_ګراف_څنډه{&ګراف, t2, t4, 1};
        جديد V3_ګراف_څنډه{&ګراف, t3, t5, 1};
        جديد V3_ګراف_څنډه{&ګراف, t4, t6, 1};

        constexpr uint32_t تارونه = 2;
        constexpr uint32_t hier_تارونه = 6;
        Pack_تارونه packer{تارونه, hier_تارونه,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::ویکټور<Thread_مهالوېش> scheduled = packer.pack(ګراف);
        UASSERT_SELFTEST(scheduled.اندازه(), 3);
        UASSERT_SELFTEST(scheduled[0].m_تارونه.اندازه(), تارونه);
        UASSERT_SELFTEST(scheduled[0].m_تارونه[0].اندازه(), 2);
        لكل (اندازه_t i = 1; i < scheduled[0].m_تارونه.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_تارونه[i].اندازه(), 0);

        UASSERT_SELFTEST(scheduled[0].m_تارونه[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_تارونه[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_تارونه.اندازه(), hier_تارونه / 3);
        UASSERT_SELFTEST(scheduled[1].m_تارونه[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_تارونه[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_تارونه[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_تارونه.اندازه(), تارونه);
        UASSERT_SELFTEST(scheduled[2].m_تارونه[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_تارونه[1][0], t6);

        UASSERT_SELFTEST(Thread_مهالوېش::s_mtaskState.اندازه(), 7);

        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t4, 5), 1360);

        لكل (V3_ګراف_راس& vtx : ګراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ونه();
        VL_DO_DANGLING(execGraphp->delete_ونه(), execGraphp);
        Thread_مهالوېش::s_mtaskState.clear();
    }
    ساكن فراغ self_Test_Hier_لومړی(AstVarScope* trigVscp) {
        File_کرښه* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_ګراف* ثابت execGraphp = جديد Ast_Exec_ګراف{flp, "test", trigVscp};
        V3_ګراف& ګراف = *execGraphp->depGraphp();
        ثابت خپلکاری makeBody = [&]() -> Ast_غوټه_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t0->لګښت(1000);
        t0->لومړیتوب_2(1100);
        t0->تارونه(2);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, تش_اشاره, makeBody()};
        t1->لګښت(100);
        t1->لومړیتوب_2(100);

        /*
                          0
                          |
                          1
        */
        جديد V3_ګراف_څنډه{&ګراف, t0, t1, 1};

        constexpr uint32_t تارونه = 1;
        constexpr uint32_t hier_تارونه = 2;
        Pack_تارونه packer{تارونه, hier_تارونه,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::ویکټور<Thread_مهالوېش> scheduled = packer.pack(ګراف);
        UASSERT_SELFTEST(scheduled.اندازه(), 2);
        UASSERT_SELFTEST(scheduled[0].m_تارونه.اندازه(), hier_تارونه / 2);
        UASSERT_SELFTEST(scheduled[0].m_تارونه[0].اندازه(), 1);
        لكل (اندازه_t i = 1; i < scheduled[0].m_تارونه.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_تارونه[i].اندازه(), 0);

        UASSERT_SELFTEST(scheduled[0].m_تارونه[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_تارونه.اندازه(), تارونه);
        UASSERT_SELFTEST(scheduled[1].m_تارونه[0].اندازه(), 1);
        لكل (اندازه_t i = 1; i < scheduled[1].m_تارونه.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_تارونه[i].اندازه(), 0);
        UASSERT_SELFTEST(scheduled[1].m_تارونه[0][0], t1);

        UASSERT_SELFTEST(Thread_مهالوېش::s_mtaskState.اندازه(), 2);

        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_مهالوېش::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_وخت(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_وخت(scheduled[1], t1, 1), 1130);

        لكل (V3_ګراف_راس& vtx : ګراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ونه();
        VL_DO_DANGLING(execGraphp->delete_ونه(), execGraphp);
        Thread_مهالوېش::s_mtaskState.clear();
    }

    ساكن std::ویکټور<Thread_مهالوېش> apply(V3_ګراف& mtask_ګراف) {
        أرجع Pack_تارونه{}.pack(mtask_ګراف);
    }
};

کارول EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
کارول Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

فراغ normalizeCosts(Costs& costs) {
    ثابت خپلکاری scale_لګښت = [](uint64_t ارزښت, مزدوج multiplier) {
        مزدوج scaled = static_cast<مزدوج>(ارزښت) * multiplier;
        إذا (ارزښت && scaled < 1) scaled = 1;
        أرجع static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t مجموعه_لګښت_Profiled = 0;  // For data with estimate and profile
    uint64_t مجموعه_لګښت_Estimate = 0;  // For data with estimate and profile
    لكل (ثابت خپلکاری& est : costs) {
        إذا (est.دوهم.دوهم) {
            مجموعه_لګښت_Estimate += est.دوهم.لومړی;
            مجموعه_لګښت_Profiled += est.دوهم.دوهم;
        }
    }

    إذا (مجموعه_لګښت_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت مزدوج est_منزل_Profile
            = static_cast<مزدوج>(مجموعه_لګښت_Profiled) / static_cast<مزدوج>(مجموعه_لګښت_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_منزل_Profile
                                                    << ", sumCostProfiled=" << مجموعه_لګښت_Profiled
                                                    << " sumCostEstimate=" << مجموعه_لګښت_Estimate);
        لكل (خپلکاری& est : costs) {
            uint64_t& لګښت_Estimate = est.دوهم.لومړی;
            لګښت_Estimate = scale_لګښت(لګښت_Estimate, est_منزل_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_لګښت = 0;
    لكل (خپلکاری& est : costs) {
        ثابت uint64_t& لګښت_Estimate = est.دوهم.لومړی;
        ثابت uint64_t& لګښت_Profiled = est.دوهم.دوهم;
        إذا (max_لګښت < لګښت_Estimate) max_لګښت = لګښت_Estimate;
        إذا (max_لګښت < لګښت_Profiled) max_لګښت = لګښت_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.دوهم.لومړی << " cp=" << est.دوهم.دوهم);
    }
    constexpr uint64_t scale_ښکته_منزل = 10000000;  // Extra room for future algorithms to add costs
    إذا (max_لګښت > scale_ښکته_منزل) {
        ثابت مزدوج scaleup = static_cast<مزدوج>(scale_ښکته_منزل) / static_cast<مزدوج>(max_لګښت);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_لګښت);
        لكل (خپلکاری& est : costs) {
            est.دوهم.لومړی = scale_لګښت(est.دوهم.لومړی, scaleup);
            est.دوهم.دوهم = scale_لګښت(est.دوهم.دوهم, scaleup);
        }
    }
}

فراغ remove_تشه_MTasks(Ast_Exec_ګراف* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    لكل (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        إذا (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_شا()->delete_ونه(), callp);
    }
    V3_ګراف* ثابت execMTaskGraphp = execGraphp->depGraphp();
    لكل (V3_ګراف_راس* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        إذا (funcp->stmtsp()) استمر;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_څنډې(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_شا()->delete_ونه(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);
}

فراغ fillinCosts(V3_ګراف* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    لكل (V3_ګراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t لګښت_Estimate = V3_Instr_شمېر::شمېر(mtp->funcp(), خطأ);
        ثابت uint64_t لګښت_Profiled
            = V3Control::get_Profile_معلومات(v3Global.opt.مختاړی(), mtp->hashName());
        إذا (لګښت_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << لګښت_Profiled);
        }
        costs[mtp->id()] = std::make_pair(لګښت_Estimate, لګښت_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    صحيح ټول_Estimates = 0;
    صحيح missingProfiles = 0;
    لكل (V3_ګراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t لګښت_Estimate = costs[mtp->id()].لومړی;
        ثابت uint64_t لګښت_Profiled = costs[mtp->id()].دوهم;
        UINFO(9, "ce = " << لګښت_Estimate << " cp=" << لګښت_Profiled);
        UASSERT(لګښت_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(لګښت_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t لګښت_Profiled32 = static_cast<uint32_t>(لګښت_Profiled);
        uint32_t لګښت_منزل_Use = لګښت_Profiled32;
        إذا (!لګښت_Profiled32) {
            لګښت_منزل_Use = لګښت_Estimate;
            إذا (لګښت_Estimate != 0) ++missingProfiles;
        }
        إذا (لګښت_Estimate != 0) ++ټول_Estimates;
        mtp->لګښت(لګښت_منزل_Use);
        mtp->لومړیتوب_2(لګښت_منزل_Use);
    }

    إذا (missingProfiles) {
        إذا (File_کرښه* ثابت fl = V3Control::get_Profile_معلومات_File_کرښه()) {
            إذا (V3Control::لري_MTask_Profile_معلومات()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << ټول_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

فراغ finalizeCosts(V3_ګراف* execMTaskGraphp) {
    ګراف_بهیر_Unordered ser(execMTaskGraphp, ګراف_Way::سرچپه_2);
    طالما (ثابت V3_ګراف_راس* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3_ګراف_راس*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        لكل (V3_ګراف_څنډه& څنډه : mtp->بهر_څنډې()) {
            ثابت ExecMTask* ثابت followp = څنډه.سر()->as<ExecMTask>();
            إذا ((followp->لومړیتوب_2() + mtp->لګښت()) > mtp->لومړیتوب_2()) {
                mtp->لومړیتوب_2(followp->لومړیتوب_2() + mtp->لګښت());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_متعدي_څنډې();

    // Record summary stats for final m_tasks graph.
    ثابت خپلکاری report = execMTaskGraphp->parallelismReport([](ثابت V3_ګراف_راس* vtxp) {  //
        أرجع vtxp->as<ثابت ExecMTask>()->لګښت();
    });
    V3Stats::جمع_کړه_Stat("MTask graph, final, critical path cost", report.critical_لار_لګښت());
    V3Stats::جمع_کړه_Stat("MTask graph, final, total graph cost", report.ټول_ګراف_لګښت());
    V3Stats::جمع_کړه_Stat("MTask graph, final, mtask count", report.راس_شمېر());
    V3Stats::جمع_کړه_Stat("MTask graph, final, edge count", report.څنډه_شمېر());
    V3Stats::جمع_کړه_Stat("MTask graph, final, parallelism factor", report.parallelism_ضریب());
    إذا (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_لار_لګښت());
        UINFO(0, "    Total graph cost = " << report.ټول_ګراف_لګښت());
        UINFO(0, "    MTask vertex count = " << report.راس_شمېر());
        UINFO(0, "    Edge count = " << report.څنډه_شمېر());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ضریب());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_غوټه_Module* ثابت modp = v3Global.rootp()->سر_Modulep();
    File_کرښه* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->پیدا_کړه_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(ریښتیا);
    varp->valuep(جديد AstConst{fl, nDependencies});
    modp->جمع_کړه_Stmtsp(varp);
    أرجع varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& لاره) {
    File_کرښه* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت سر_Scopep = v3Global.rootp()->سر_Scopep()->scopep();
    refp->self_اشاره_ګر(VSelf_اشاره_ګر_متن{VSelf_اشاره_ګر_متن::VlSyms{}, سر_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = جديد AstCStmt{fl};
    stmtp->جمع_کړه(refp);
    stmtp->جمع_کړه("." + لاره + ";");
    أرجع stmtp;
}

فراغ جمع_کړه_MTask_منزل_Function(ثابت Thread_مهالوېش& مهالوېش, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->سر_Scopep()->scopep();
    Ast_غوټه_Module* ثابت modp = v3Global.rootp()->سر_Modulep();
    File_کرښه* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت خپلکاری جمع_کړه_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        funcp->جمع_کړه_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (مهالوېش.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        إذا (v3Global.opt.profExec()) {
            جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->جمع_کړه_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        إذا (v3Global.opt.profExec()) {
            جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    إذا (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        جمع_کړه_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = جديد AstCCall{fl, mtaskp->funcp()};
    callp->self_اشاره_ګر(VSelf_اشاره_ګر_متن{VSelf_اشاره_ګر_متن::VlSyms{}, scopep->nameDotless()});
    callp->dtype_سېټ_Void();
    funcp->جمع_کړه_Stmtsp(callp->makeStmt());

    إذا (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        جمع_کړه_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    لكل (ثابت V3_ګراف_څنډه& څنډه : mtaskp->بهر_څنډې()) {
        ثابت ExecMTask* ثابت nextp = څنډه.سر()->as<ExecMTask>();
        إذا (مهالوېش.threadId(nextp) != threadId && مهالوېش.لري(nextp)) {
            funcp->جمع_کړه_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::ویکټور<AstCFunc*> createThreadFunctions(ثابت Thread_مهالوېش& مهالوېش,
                                                   ثابت string& نښان) {
    Ast_غوټه_Module* ثابت modp = v3Global.rootp()->سر_Modulep();
    File_کرښه* ثابت fl = modp->fileline();

    std::ویکټور<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    لكل (ثابت std::ویکټور<ثابت ExecMTask*>& thread : مهالوېش.m_تارونه) {
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            إذا (ثابت uint32_t nDependencies = مهالوېش.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_منزل_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    لكل (ثابت std::ویکټور<ثابت ExecMTask*>& thread : مهالوېش.m_تارونه) {
        إذا (thread.تشه()) استمر;
        ثابت uint32_t threadId = مهالوېش.threadId(thread.front());
        ثابت string name{"__Vthread__" + نښان + "__s" + cvt_منزل_Str(مهالوېش.id()) + "__t"
                          + cvt_منزل_Str(threadId)};
        AstCFunc* ثابت funcp = جديد AstCFunc{fl, name, تش_اشاره, "void"};
        modp->جمع_کړه_Stmtsp(funcp);
        funcps.ټېل_شا(funcp);
        funcp->isStatic(ریښتیا);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(ریښتیا);
        funcp->ثبت_ټکی(ریښتیا);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->جمع_کړه_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_ټاکل(modp)});
        funcp->جمع_کړه_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_ټاکل()});

        // Invoke each mtask scheduled to this thread from the thread function
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            جمع_کړه_MTask_منزل_Function(مهالوېش, threadId, funcp, mtaskp, stateVars);
        }
    }

    أرجع funcps;
}

فراغ جمع_کړه_Thread_پیل_Wrapper(Ast_Exec_ګراف* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_کرښه* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& نښان = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت خپلکاری جمع_کړه_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جمع_کړه_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (v3Global.opt.profExec()) {
        جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    جمع_کړه_CStmt("vlSymsp->__Vm_even_cycle__" + نښان + " = !vlSymsp->__Vm_even_cycle__" + نښان + ";");

    إذا (!v3Global.opt.hier_بلاکونه().تشه()) جمع_کړه_CStmt("std::vector<size_t> indexes;");
}

فراغ جمع_کړه_Thread_پای_ټکی_Wrapper(Ast_Exec_ګراف* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت خپلکاری جمع_کړه_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        File_کرښه* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->جمع_کړه_Stmtsp(جديد AstCStmt{flp, stmt});
    };

    جمع_کړه_CStmt("Verilated::mtaskId(0);");
    إذا (v3Global.opt.profExec()) { جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
فراغ جمع_کړه_Thread_پیل_منزل_Exec_ګراف(Ast_Exec_ګراف* ثابت execGraphp,
                               ثابت std::ویکټور<AstCFunc*>& funcps, uint32_t مهالوېش_Id) {
    // FileLine used for constructing nodes below
    File_کرښه* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& نښان = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvt_منزل_Str(مهالوېش_Id) + "__" + نښان, funcps.اندازه());
    لكل (AstCFunc* ثابت funcp : funcps) {
        funcp->جمع_کړه_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت خپلکاری جمع_کړه_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جمع_کړه_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    ثابت uint32_t وروستی = funcps.اندازه() - 1;
    إذا (!v3Global.opt.hier_بلاکونه().تشه() && وروستی > 0) {
        جمع_کړه_CStmt("for (size_t i = 0; i < " + std::منزل_string(وروستی) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    لكل (AstCFunc* ثابت funcp : funcps) {
        إذا (i != وروستی) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            execGraphp->runfuncp()->جمع_کړه_Stmtsp(cstmtp);
            cstmtp->جمع_کړه("vlSymsp->__Vm_threadPoolp->workerp(");
            إذا (v3Global.opt.hier_ماشوم() || !v3Global.opt.hier_بلاکونه().تشه()) {
                cstmtp->جمع_کړه("indexes[" + std::منزل_string(i) + "]");
            } وإلا {
                cstmtp->جمع_کړه(std::منزل_string(i));
            }
            cstmtp->جمع_کړه(")->addTask(");
            cstmtp->جمع_کړه(جديد Ast_Addr_د_CFunc{fl, funcp});
            cstmtp->جمع_کړه(", vlSelf, vlSymsp->__Vm_even_cycle__" + نښان + ");");
        } وإلا {
            // The last will run on the main thread.
            AstCCall* ثابت callp = جديد AstCCall{fl, funcp};
            callp->dtype_سېټ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + نښان);
            execGraphp->runfuncp()->جمع_کړه_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Thread schedule total tasks", i);

    إذا (v3Global.opt.profExec()) {
        جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->جمع_کړه_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + نښان + ")"));
    إذا (v3Global.opt.profExec()) {
        جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    إذا (!v3Global.opt.hier_بلاکونه().تشه() && وروستی > 0) {
        جمع_کړه_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

فراغ processMTaskBodies(Ast_Exec_ګراف* ثابت execGraphp) {
    لكل (V3_ګراف_راس* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_غوټه* stmtsp = funcp->stmtsp()->unlink_Fr_شا_With_بل();

        // Helper function to make the code a bit more legible
        ثابت خپلکاری جمع_کړه_CStmt = [=](ثابت string& stmt) -> فراغ {  //
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        إذا (v3Global.opt.profExec()) {
            std::string دلیلونه = std::منزل_string(mtaskp->id());
            دلیلونه += ", " + std::منزل_string(mtaskp->predict_پیل());
            دلیلونه += ", \"";
            إذا (v3Global.opt.hier_ماشوم()) دلیلونه += v3Global.opt.سر_Module();
            دلیلونه += "\"";
            جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + دلیلونه + ");");
        }
        // Set mtask ID in the run-time system
        جمع_کړه_CStmt("Verilated::mtaskId(" + std::منزل_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->جمع_کړه_Stmtsp(stmtsp);
        // Flush message queue
        جمع_کړه_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        إذا (v3Global.opt.profExec()) {
            ثابت std::string& دلیلونه = std::منزل_string(mtaskp->لګښت());
            جمع_کړه_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + دلیلونه + ");");
        }
    }
}

فراغ implement_Exec_ګراف(Ast_Exec_ګراف* ثابت execGraphp, ثابت Thread_مهالوېش& مهالوېش) {
    // Nothing to be done if there are no MTasks in the graph at all.
    إذا (execGraphp->depGraphp()->تشه()) أرجع;

    // Create a function to be run by each thread.
    ثابت std::ویکټور<AstCFunc*>& funcps = createThreadFunctions(مهالوېش, execGraphp->name());
    UASSERT(!funcps.تشه(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    جمع_کړه_Thread_پیل_منزل_Exec_ګراف(execGraphp, funcps, مهالوېش.id());
}

فراغ create_پرلپسې_Function(Ast_Exec_ګراف* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_کرښه* ثابت flp = execGraphp->fileline();
    Ast_غوټه_Module* ثابت modp = v3Global.rootp()->سر_Modulep();
    AstCFunc* ثابت funcp = جديد AstCFunc{flp, "runExecGraph_" + execGraphp->name(), تش_اشاره};
    funcp->isLoose(ریښتیا);
    funcp->voidSelf(ریښتیا);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(ریښتیا);
    modp->جمع_کړه_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

فراغ جمع_کړه_Trigger_Masks(Ast_Exec_ګراف* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_کرښه* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> کلمه_Masks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            لكل (ثابت uint32_t شاخص : trigp->indices()) {
                کلمه_Masks[VL_BITWORD_Q(شاخص)] |= VL_بټ_Q(شاخص);
            }
        });
        AstConsArray* ثابت consp = جديد AstConsArray{flp, triggersp->dtypep(),
                                                     جديد AstConst{flp, AstConst::Unsized64{}, 0}};
        لكل (ثابت خپلکاری& کلمه_ماسک : کلمه_Masks) {
            consp->valuep(کلمه_ماسک.لومړی,
                          جديد AstConst{flp, AstConst::Unsized64{}, کلمه_ماسک.دوهم});
        }
        execGraphp->جمع_کړه_Maskps(V3ConstPool::پیدا_کړه(consp));
        VL_DO_DANGLING(consp->delete_ونه(), consp);
    }
}

فراغ جمع_کړه_Trigger_Costs(Ast_Exec_ګراف* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_کرښه* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::ویکټور<uint32_t> costs(nTriggers, 0);
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            إذا (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t لګښت = V3_Instr_شمېر::شمېر(ifp, خطأ);
                لكل (ثابت uint32_t شاخص : trigp->indices()) costs.at(شاخص) += لګښت;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = جديد AstUnpackArrayDType{flp, netlistp->پیدا_کړه_بټ_DType(32, 32, VSigning::UNSIGNED),
                                  جديد AstRange{flp, static_cast<صحيح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->جمع_کړه_Typesp(dtypep);
    AstConsArray* ثابت consp = جديد AstConsArray{flp, dtypep, جديد AstConst{flp, 0}};
    لكل (uint32_t شاخص = 0; شاخص < nTriggers; ++شاخص) {
        إذا (costs[شاخص]) consp->valuep(شاخص, جديد AstConst{flp, costs[شاخص]});
    }
    execGraphp->triggerCostsp(V3ConstPool::پیدا_کړه(consp));
    VL_DO_DANGLING(consp->delete_ونه(), consp);
}

// Called by Verilator top stage
فراغ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::ویکټور<Ast_Exec_ګراف*> execGraphps;
    netlistp->سر_Modulep()->foreach([&](Ast_Exec_ګراف* egp) { execGraphps.emplace_شا(egp); });

    // Process each
    لكل (Ast_Exec_ګراف* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_تشه_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        إذا (execGraphp->depGraphp()->تشه()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_شا()->delete_ونه(), execGraphp);
            أرجع;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        إذا (dump_ګراف_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_پرلپسې_Function(execGraphp);
        جمع_کړه_Thread_پیل_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::ویکټور<Thread_مهالوېش> packed = Pack_تارونه::apply(*execGraphp->depGraphp());
        V3Stats::جمع_کړه_Stat_مجموعه("Optimizations, Thread schedule count",
                            static_cast<مزدوج>(packed.اندازه()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        لكل (ثابت Thread_مهالوېش& مهالوېش : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_ګراف(execGraphp, مهالوېش);
        }

        جمع_کړه_Thread_پای_ټکی_Wrapper(execGraphp);

        جمع_کړه_Trigger_Masks(execGraphp);
        جمع_کړه_Trigger_Costs(execGraphp);
    }
}

فراغ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].لومړی, 1000);
        UASSERT_SELFTEST(costs[1].دوهم, 1000);
        UASSERT_SELFTEST(costs[2].لومړی, 2000);
        UASSERT_SELFTEST(costs[2].دوهم, 0);
        UASSERT_SELFTEST(costs[3].لومړی, 3000);
        UASSERT_SELFTEST(costs[3].دوهم, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].لومړی, 2500000);
        UASSERT_SELFTEST(costs[1].دوهم, 5000000);
        UASSERT_SELFTEST(costs[2].لومړی, 5000000);
        UASSERT_SELFTEST(costs[2].دوهم, 10000000);
        UASSERT_SELFTEST(costs[3].لومړی, 7500000);
        UASSERT_SELFTEST(costs[3].دوهم, 1);
    }

    Pack_تارونه::selfTest();
}

}  // namespace V3ExecGraph
