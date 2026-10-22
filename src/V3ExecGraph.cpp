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

AstCFunc* ExecMTask::createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گره_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::مقصد_string(id);
    AstCFunc* ثابت newp = جديد AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(درست);
    newp->voidSelf(درست);  // Called through VlExecGraph::Fnp
    newp->dontCombine(درست);
    newp->جمع_کن_Stmtsp(stmtsp);
    إذا (scopep) scopep->جمع_کن_Blocksp(newp);
    أرجع newp;
}

ExecMTask::ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep,
                     Ast_گره_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_گراف_رأس{execGraphp->depGraphp()},
      m_id{s_بعدی_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).مقصد_String()} {}

فراغ ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvt_مقصد_Hex(هذا);
    إذا (اولویت_2() || هزینه()) str << " [pr=" << اولویت_2() << " c=" << cvt_مقصد_Str(هزینه()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_بعدی_Id{0};

نطاق V3_Exec_گراف {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
صنف Thread_زمانبندی final {
    دوست صنف Pack_نخها;

    uint32_t m_id;  // Unique ID of a schedule
    ساكن uint32_t s_بعدی_Id;  // Next ID number to use
    std::unordered_مجموعه<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_انتها_زمان = 0;  // Latest task end time in this schedule

عام:
    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    بنية MTaskState final {
        uint32_t completion_زمان = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = اشاره_تهی;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::وکتور<std::وکتور<ثابت ExecMTask*>> m_نخها;

    // Global state for each mtask.
    ساكن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    صریح Thread_زمانبندی(uint32_t n_نخها)
        : m_id{s_بعدی_Id++}
        , m_نخها{n_نخها} {}
    Thread_زمانبندی(Thread_زمانبندی&&) = افتراضي;
    Thread_زمانبندی& عملگر=(Thread_زمانبندی&&) = افتراضي;

خاص:
    VL_UNCOPYABLE(Thread_زمانبندی);

    ساكن constexpr مزدوج s_thread_Box_عرض = 2.5;
    ساكن constexpr مزدوج s_thread_Box_ارتفاع = 1.5;
    ساكن constexpr مزدوج s_horizontalGap = s_thread_Box_عرض / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساكن فراغ dumpDotFilePrefixedAlways(ثابت std::وکتور<Thread_زمانبندی>& schedules,
                                          ثابت string& nameComment, uint32_t n_نخها) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_نخها);
    }
    ساكن فراغ dumpDotFile(ثابت std::وکتور<Thread_زمانبندی>& schedules, ثابت string& filename,
                            uint32_t n_نخها) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        إذا (logp->fail()) v3fatal("Can't write file: " << filename);

        //سرآیند
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        لكل (uint32_t i = 0; i < n_نخها; ++i) {
            ثابت string name = "t" + std::مقصد_string(i);
            ثابت string label = "Thread " + std::مقصد_string(i);
            constexpr مزدوج posX = -s_horizontalGap;
            ثابت مزدوج posY = -static_cast<مزدوج>(i) * s_thread_Box_ارتفاع;
            dump_Dot_File_Emit_بلوک(logp, name, label, s_thread_Box_عرض, s_thread_Box_ارتفاع, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_هزینه = 0;
        لكل (ثابت خودکار& state : Thread_زمانبندی::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.اولی;
            max_هزینه = std::max(max_هزینه, mtaskp->هزینه());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_در_هر_Longest_Mtask = 4;
        ثابت uint32_t segment_هزینه = max_هزینه / segments_در_هر_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::وکتور<مزدوج> offsets(n_نخها, 0.0);
        لكل (ثابت Thread_زمانبندی& زمانبندی : schedules) {
            إذا (زمانبندی.mtasks.خالی()) استمر;
            استفاده ستونی = std::وکتور<ثابت ExecMTask*>;
            std::وکتور<ستونی> columns = {{}};

            // Order tasks based on their start time
            بنية Cmp final {
                منطقي عملگر()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    إذا (شروع_زمان(a) == شروع_زمان(b)) أرجع threadId(a) < threadId(b);
                    أرجع شروع_زمان(a) < شروع_زمان(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(زمانبندی.mtasks.begin(),
                                                             زمانبندی.mtasks.انتها());

            لكل (ثابت ExecMTask* ثابت mtaskp : tasks) {
                ستونی& ستونی_2 = columns.پشت();
                UASSERT(ستونی_2.اندازه() <= n_نخها, "Invalid partitioning");

                منطقي intersects = درست;
                لكل (ثابت ExecMTask* ثابت earlierMtask : ستونی_2) {
                    إذا (انتها_زمان(mtaskp) <= شروع_زمان(earlierMtask)
                        || شروع_زمان(mtaskp) >= انتها_زمان(earlierMtask)) {
                        intersects = خطأ;
                        اكسر;
                    }
                }
                إذا (intersects) {
                    ستونی_2.emplace_پشت(mtaskp);
                } وإلا {
                    columns.emplace_پشت(ستونی{mtaskp});
                }
            }

            UASSERT(!columns.front().خالی(), "Should be populated by mtasks");

            لكل (ثابت ستونی& ستونی_2 : columns) {
                مزدوج آخرین_ستونی_جابه_جایی = 0;
                لكل (ثابت ExecMTask* ثابت mtaskp : ستونی_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, زمانبندی, segment_هزینه, offsets);
                    آخرین_ستونی_جابه_جایی = std::max(آخرین_ستونی_جابه_جایی, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.انتها(), آخرین_ستونی_جابه_جایی);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_نخها);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            لكل (ثابت std::وکتور<ثابت ExecMTask*>& thread : زمانبندی.m_نخها) {
                إذا (thread.خالی()) اكسر;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.پشت()->name() << " -> fork_"
                      << static_cast<صحيح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                لكل (اندازه_t i = 1; i < thread.اندازه(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
                    لكل (ثابت V3_گراف_یال& یال : mtaskp->بیرون_یالها()) {
                        ثابت ExecMTask* ثابت بالاترین_MTaskp = یال.بالاترین()->cast<ثابت ExecMTask>();
                        إذا (بالاترین_MTaskp && زمانبندی.شامل_است(بالاترین_MTaskp)
                            && threadId(بالاترین_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << بالاترین_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ساكن فراغ dump_Dot_File_Emit_بلوک(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, مزدوج عرض,
                                     مزدوج ارتفاع, مزدوج xPos, مزدوج yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << عرض
              << " height=" << ارتفاع << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساكن فراغ dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت Thread_زمانبندی& زمانبندی,
                                     uint32_t segment_هزینه, std::وکتور<مزدوج>& offsets) {
        لكل (صحيح i = 0; i < mtaskp->نخها(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::مقصد_string(i);
            ثابت string label = mtaskp->name() + " (" + std::مقصد_string(شروع_زمان(mtaskp)) + ':'
                                 + std::مقصد_string(انتها_زمان(mtaskp)) + ')'
                                 + "\\ncost=" + std::مقصد_string(mtaskp->هزینه())
                                 + "\\npriority=" + std::مقصد_string(mtaskp->اولویت_2());
            ثابت مزدوج عرض
                = std::max(s_thread_Box_عرض,
                           s_thread_Box_عرض * static_cast<مزدوج>(mtaskp->هزینه()) / segment_هزینه);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * زمانبندی.m_نخها.اندازه();
            ثابت مزدوج xPos = عرض / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += عرض + s_horizontalGap;
            ثابت مزدوج yPos = -s_thread_Box_ارتفاع * static_cast<مزدوج>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_بلوک(logp, name, label, عرض, s_thread_Box_ارتفاع, xPos, yPos,
                                 fillColor);
        }
    }

    ساكن فراغ dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, مزدوج جابه_جایی,
                                    uint32_t n_نخها) {
        ثابت string& name = "fork_" + std::مقصد_string(static_cast<صحيح>(جابه_جایی));
        constexpr مزدوج عرض = s_thread_Box_عرض / 8;
        ثابت مزدوج ارتفاع = s_thread_Box_ارتفاع * n_نخها;
        ثابت مزدوج xPos = جابه_جایی - s_horizontalGap / 2;
        ثابت مزدوج yPos
            = -static_cast<مزدوج>(n_نخها) / 2 * s_thread_Box_ارتفاع + s_thread_Box_ارتفاع / 2;
        dump_Dot_File_Emit_بلوک(logp, name, "", عرض, ارتفاع, xPos, yPos, "black");
    }

عام:
    ساكن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت خودکار& it = s_mtaskState.پیدا_کن(mtaskp);
        أرجع it != s_mtaskState.انتها() ? it->دومی.threadId : UNASSIGNED;
    }
    ساكن uint32_t شروع_زمان(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_زمان - mtaskp->هزینه();
    }
    ساكن uint32_t انتها_زمان(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_زمان;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t نتیجه = 0;
        لكل (ثابت V3_گراف_یال& یال : mtaskp->درون_یالها()) {
            ثابت ExecMTask* ثابت prevp = یال.fromp()->as<ExecMTask>();
            إذا (threadId(prevp) != thisThreadId && شامل_است(prevp)) ++نتیجه;
        }
        أرجع نتیجه;
    }

    uint32_t id() ثابت { أرجع m_id; }
    uint32_t زمانبندی_On(ثابت ExecMTask* mtaskp, uint32_t بهترین_Thread_Id) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t بهترین_انتها_زمان = mtaskp->predict_شروع() + mtaskp->هزینه();
        m_انتها_زمان = std::max(m_انتها_زمان, بهترین_انتها_زمان);
        s_mtaskState[mtaskp].completion_زمان = بهترین_انتها_زمان;
        s_mtaskState[mtaskp].threadId = بهترین_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::وکتور<ثابت ExecMTask*>& بهترین_Thread = m_نخها[بهترین_Thread_Id];
        إذا (!بهترین_Thread.خالی()) s_mtaskState[بهترین_Thread.پشت()].nextp = mtaskp;

        // Add the MTask to the schedule
        بهترین_Thread.افزودن_پشت(mtaskp);
        أرجع بهترین_انتها_زمان;
    }
    منطقي شامل_است(ثابت ExecMTask* mtaskp) ثابت { أرجع mtasks.شمار(mtaskp); }
    uint32_t انتها_زمان() ثابت { أرجع m_انتها_زمان; }
};

uint32_t Thread_زمانبندی::s_بعدی_Id = 0;
std::unordered_map<ثابت ExecMTask*, Thread_زمانبندی::MTaskState> Thread_زمانبندی::s_mtaskState{};
constexpr مزدوج V3_Exec_گراف::Thread_زمانبندی::s_thread_Box_عرض;

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
صنف Pack_نخها final {
    // TYPES
    بنية MTaskCmp final {
        منطقي عملگر()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            أرجع ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_n_نخها;  // Number of threads
    ثابت uint32_t m_n_Hier_نخها;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    صریح Pack_نخها(uint32_t n_نخها = v3Global.opt.نخها(),
                         uint32_t n_Hier_نخها = v3Global.opt.hier_نخها(),
                         بیعلامت sandbagNumerator = 30, بیعلامت sandbagDenom = 100)
        : m_n_نخها{n_نخها}
        , m_n_Hier_نخها{n_Hier_نخها}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_نخها() = افتراضي;
    VL_UNCOPYABLE(Pack_نخها);

    // METHODS
    uint32_t completion_زمان(ثابت Thread_زمانبندی& زمانبندی, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        إذا (!زمانبندی.شامل_است(mtaskp)) أرجع 0;
        ثابت Thread_زمانبندی::MTaskState& state = زمانبندی.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_زمانبندی::UNASSIGNED, "Mtask should have assigned thread");
        إذا (threadId == state.threadId) {
            // No overhead on same thread
            أرجع state.completion_زمان;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_انتها_زمان
            = state.completion_زمان + (m_sandbagNumerator * mtaskp->هزینه()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        إذا (state.nextp) {
            ثابت uint32_t successor_انتها_زمان
                = completion_زمان(زمانبندی, state.nextp, state.threadId);
            إذا ((sandbagged_انتها_زمان >= successor_انتها_زمان) && (successor_انتها_زمان > 1)) {
                sandbagged_انتها_زمان = successor_انتها_زمان - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_انتها_زمان);
        أرجع sandbagged_انتها_زمان;
    }

    ساكن منطقي isReady(Thread_زمانبندی& زمانبندی, ثابت ExecMTask* mtaskp) {
        لكل (ثابت V3_گراف_یال& یال_درون : mtaskp->درون_یالها()) {
            ثابت ExecMTask* ثابت prevp = یال_درون.fromp()->as<ثابت ExecMTask>();
            إذا (زمانبندی.threadId(prevp) == Thread_زمانبندی::UNASSIGNED) {
                // This predecessor is not assigned yet
                أرجع خطأ;
            }
        }
        أرجع درست;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::وکتور<Thread_زمانبندی> pack(V3_گراف& mtask_گراف) {
        std::وکتور<Thread_زمانبندی> نتیجه;
        نتیجه.emplace_پشت(Thread_زمانبندی{m_n_نخها});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        تعداد صنف Scheduling_نما : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_نما نما = Scheduling_نما::SCHEDULING;

        // Time each thread is occupied until
        std::وکتور<uint32_t> busyUntil(std::max(m_n_نخها, m_n_Hier_نخها), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::مجموعه<ExecMTask*, MTaskCmp> readyMTasks;
        صحيح maxThreadWorkers = 1;

        // Build initial ready list
        لكل (V3_گراف_رأس& vtx : mtask_گراف.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            إذا (isReady(نتیجه.پشت(), mtaskp)) readyMTasks.درج(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->نخها());
        }

        طالما (!readyMTasks.خالی()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t بهترین_زمان = 0xffffffff;
            uint32_t بهترین_Thread_Id = 0;
            ExecMTask* بهترین_Mtaskp = اشاره_تهی;  // Todo: const ExecMTask*
            Thread_زمانبندی& زمانبندی = نتیجه.پشت();
            لكل (uint32_t threadId = 0; threadId < زمانبندی.m_نخها.اندازه(); ++threadId) {
                لكل (ExecMTask* ثابت mtaskp : readyMTasks) {
                    إذا (نما != Scheduling_نما::WIDE_TASK_SCHEDULING && mtaskp->نخها() > 1) {
                        نما = Scheduling_نما::WIDE_TASK_DISCOVERED;
                        استمر;
                    }
                    إذا (نما == Scheduling_نما::WIDE_TASK_SCHEDULING && mtaskp->نخها() <= 1)
                        استمر;

                    uint32_t زمان_Begin = busyUntil[threadId];
                    إذا (زمان_Begin > بهترین_زمان) {
                        UINFO(6, "th " << threadId << " busy until " << زمان_Begin
                                       << ", later than bestTime " << بهترین_زمان
                                       << ", skipping thread.");
                        اكسر;
                    }
                    لكل (ثابت V3_گراف_یال& یال : mtaskp->درون_یالها()) {
                        ثابت ExecMTask* ثابت priorp = یال.fromp()->as<ExecMTask>();
                        ثابت uint32_t پیشین_انتها_زمان = completion_زمان(زمانبندی, priorp, threadId);
                        إذا (پیشین_انتها_زمان > زمان_Begin) زمان_Begin = پیشین_انتها_زمان;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << زمان_Begin
                                     << " on thread " << threadId);
                    إذا ((زمان_Begin < بهترین_زمان)
                        || ((زمان_Begin == بهترین_زمان)
                            && بهترین_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->اولویت_2() > بهترین_Mtaskp->اولویت_2()))) {
                        بهترین_زمان = زمان_Begin;
                        بهترین_Thread_Id = threadId;
                        بهترین_Mtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t انتها_زمان = زمانبندی.انتها_زمان();

            إذا (!بهترین_Mtaskp && نما == Scheduling_نما::WIDE_TASK_DISCOVERED) {
                نما = Scheduling_نما::WIDE_TASK_SCHEDULING;
                ثابت uint32_t اندازه = m_n_Hier_نخها / maxThreadWorkers;
                UASSERT(اندازه, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                إذا (زمانبندی.s_mtaskState.خالی()) نتیجه.clear();
                نتیجه.emplace_پشت(Thread_زمانبندی{اندازه});
                std::fill(busyUntil.begin(), busyUntil.انتها(), انتها_زمان);
                استمر;
            }

            إذا (!بهترین_Mtaskp && نما == Scheduling_نما::WIDE_TASK_SCHEDULING) {
                نما = Scheduling_نما::SCHEDULING;
                UASSERT(!زمانبندی.s_mtaskState.خالی(), "Mtask should be added");
                نتیجه.emplace_پشت(Thread_زمانبندی{m_n_نخها});
                std::fill(busyUntil.begin(), busyUntil.انتها(), انتها_زمان);
                استمر;
            }

            UASSERT(بهترین_Mtaskp, "Should have found some task");

            بهترین_Mtaskp->predict_شروع(بهترین_زمان);
            ثابت uint32_t بهترین_انتها_زمان = زمانبندی.زمانبندی_On(بهترین_Mtaskp, بهترین_Thread_Id);
            busyUntil[بهترین_Thread_Id] = بهترین_انتها_زمان;

            // Update the ready list
            ثابت اندازه_t erased = readyMTasks.erase(بهترین_Mtaskp);
            UASSERT_OBJ(erased > 0, بهترین_Mtaskp, "Should have erased something?");
            لكل (ثابت V3_گراف_یال& یال_بیرون : بهترین_Mtaskp->بیرون_یالها()) {
                ExecMTask* ثابت nextp = یال_بیرون.بالاترین()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(زمانبندی.threadId(nextp) == Thread_زمانبندی::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.پیدا_کن(nextp) == readyMTasks.انتها(), nextp,
                            "Tasks after one being assigned should not be ready");
                إذا (isReady(زمانبندی, nextp)) {
                    readyMTasks.درج(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        إذا (dump_گراف_Level() >= 4)
            Thread_زمانبندی::dumpDotFilePrefixedAlways(نتیجه, "schedule", m_n_نخها);

        أرجع نتیجه;
    }

عام:
    // SELF TEST
    ساكن فراغ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->پیدا_کن_UInt64_DType()};
        AstScope* ثابت scopep = جديد AstScope{flp, اشاره_تهی, "test", اشاره_تهی, اشاره_تهی};
        AstVarScope* ثابت trigVscp = جديد AstVarScope{flp, scopep, varp};
        self_Test_Hier_اولی(trigVscp);
        self_Test_نرمال_اولی(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_درخت(), trigVscp);
        VL_DO_DANGLING(scopep->delete_درخت(), scopep);
        VL_DO_DANGLING(varp->delete_درخت(), varp);
    }
    ساكن فراغ self_Test_نرمال_اولی(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گره_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t0->هزینه(1000);
        t0->اولویت_2(1100);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t1->هزینه(100);
        t1->اولویت_2(100);
        ExecMTask* ثابت t2 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t2->هزینه(100);
        t2->اولویت_2(100);
        t2->نخها(2);
        ExecMTask* ثابت t3 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t3->هزینه(100);
        t3->اولویت_2(100);
        t3->نخها(3);
        ExecMTask* ثابت t4 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t4->هزینه(100);
        t4->اولویت_2(100);
        t4->نخها(3);
        ExecMTask* ثابت t5 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t5->هزینه(100);
        t5->اولویت_2(100);
        ExecMTask* ثابت t6 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t6->هزینه(100);
        t6->اولویت_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        جديد V3_گراف_یال{&گراف, t0, t1, 1};
        جديد V3_گراف_یال{&گراف, t0, t2, 1};
        جديد V3_گراف_یال{&گراف, t2, t3, 1};
        جديد V3_گراف_یال{&گراف, t2, t4, 1};
        جديد V3_گراف_یال{&گراف, t3, t5, 1};
        جديد V3_گراف_یال{&گراف, t4, t6, 1};

        constexpr uint32_t نخها = 2;
        constexpr uint32_t hier_نخها = 6;
        Pack_نخها packer{نخها, hier_نخها,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::وکتور<Thread_زمانبندی> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.اندازه(), 3);
        UASSERT_SELFTEST(scheduled[0].m_نخها.اندازه(), نخها);
        UASSERT_SELFTEST(scheduled[0].m_نخها[0].اندازه(), 2);
        لكل (اندازه_t i = 1; i < scheduled[0].m_نخها.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_نخها[i].اندازه(), 0);

        UASSERT_SELFTEST(scheduled[0].m_نخها[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_نخها[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_نخها.اندازه(), hier_نخها / 3);
        UASSERT_SELFTEST(scheduled[1].m_نخها[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_نخها[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_نخها[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_نخها.اندازه(), نخها);
        UASSERT_SELFTEST(scheduled[2].m_نخها[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_نخها[1][0], t6);

        UASSERT_SELFTEST(Thread_زمانبندی::s_mtaskState.اندازه(), 7);

        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t4, 5), 1360);

        لكل (V3_گراف_رأس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_زمانبندی::s_mtaskState.clear();
    }
    ساكن فراغ self_Test_Hier_اولی(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گره_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t0->هزینه(1000);
        t0->اولویت_2(1100);
        t0->نخها(2);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, اشاره_تهی, makeBody()};
        t1->هزینه(100);
        t1->اولویت_2(100);

        /*
                          0
                          |
                          1
        */
        جديد V3_گراف_یال{&گراف, t0, t1, 1};

        constexpr uint32_t نخها = 1;
        constexpr uint32_t hier_نخها = 2;
        Pack_نخها packer{نخها, hier_نخها,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::وکتور<Thread_زمانبندی> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.اندازه(), 2);
        UASSERT_SELFTEST(scheduled[0].m_نخها.اندازه(), hier_نخها / 2);
        UASSERT_SELFTEST(scheduled[0].m_نخها[0].اندازه(), 1);
        لكل (اندازه_t i = 1; i < scheduled[0].m_نخها.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_نخها[i].اندازه(), 0);

        UASSERT_SELFTEST(scheduled[0].m_نخها[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_نخها.اندازه(), نخها);
        UASSERT_SELFTEST(scheduled[1].m_نخها[0].اندازه(), 1);
        لكل (اندازه_t i = 1; i < scheduled[1].m_نخها.اندازه(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_نخها[i].اندازه(), 0);
        UASSERT_SELFTEST(scheduled[1].m_نخها[0][0], t1);

        UASSERT_SELFTEST(Thread_زمانبندی::s_mtaskState.اندازه(), 2);

        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_زمانبندی::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_زمان(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_زمان(scheduled[1], t1, 1), 1130);

        لكل (V3_گراف_رأس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_زمانبندی::s_mtaskState.clear();
    }

    ساكن std::وکتور<Thread_زمانبندی> apply(V3_گراف& mtask_گراف) {
        أرجع Pack_نخها{}.pack(mtask_گراف);
    }
};

استفاده EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
استفاده Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

فراغ normalizeCosts(Costs& costs) {
    ثابت خودکار scale_هزینه = [](uint64_t مقدار, مزدوج multiplier) {
        مزدوج scaled = static_cast<مزدوج>(مقدار) * multiplier;
        إذا (مقدار && scaled < 1) scaled = 1;
        أرجع static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t جمع_هزینه_Profiled = 0;  // For data with estimate and profile
    uint64_t جمع_هزینه_Estimate = 0;  // For data with estimate and profile
    لكل (ثابت خودکار& est : costs) {
        إذا (est.دومی.دومی) {
            جمع_هزینه_Estimate += est.دومی.اولی;
            جمع_هزینه_Profiled += est.دومی.دومی;
        }
    }

    إذا (جمع_هزینه_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت مزدوج est_مقصد_Profile
            = static_cast<مزدوج>(جمع_هزینه_Profiled) / static_cast<مزدوج>(جمع_هزینه_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_مقصد_Profile
                                                    << ", sumCostProfiled=" << جمع_هزینه_Profiled
                                                    << " sumCostEstimate=" << جمع_هزینه_Estimate);
        لكل (خودکار& est : costs) {
            uint64_t& هزینه_Estimate = est.دومی.اولی;
            هزینه_Estimate = scale_هزینه(هزینه_Estimate, est_مقصد_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_هزینه = 0;
    لكل (خودکار& est : costs) {
        ثابت uint64_t& هزینه_Estimate = est.دومی.اولی;
        ثابت uint64_t& هزینه_Profiled = est.دومی.دومی;
        إذا (max_هزینه < هزینه_Estimate) max_هزینه = هزینه_Estimate;
        إذا (max_هزینه < هزینه_Profiled) max_هزینه = هزینه_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.دومی.اولی << " cp=" << est.دومی.دومی);
    }
    constexpr uint64_t scale_پایین_مقصد = 10000000;  // Extra room for future algorithms to add costs
    إذا (max_هزینه > scale_پایین_مقصد) {
        ثابت مزدوج scaleup = static_cast<مزدوج>(scale_پایین_مقصد) / static_cast<مزدوج>(max_هزینه);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_هزینه);
        لكل (خودکار& est : costs) {
            est.دومی.اولی = scale_هزینه(est.دومی.اولی, scaleup);
            est.دومی.دومی = scale_هزینه(est.دومی.دومی, scaleup);
        }
    }
}

فراغ remove_خالی_MTasks(Ast_Exec_گراف* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    لكل (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        إذا (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_پشت()->delete_درخت(), callp);
    }
    V3_گراف* ثابت execMTaskGraphp = execGraphp->depGraphp();
    لكل (V3_گراف_رأس* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        إذا (funcp->stmtsp()) استمر;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_یالها(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_پشت()->delete_درخت(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);
}

فراغ fillinCosts(V3_گراف* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    لكل (V3_گراف_رأس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t هزینه_Estimate = V3_Instr_شمار::شمار(mtp->funcp(), خطأ);
        ثابت uint64_t هزینه_Profiled
            = V3Control::get_Profile_داده(v3Global.opt.پیشوند(), mtp->hashName());
        إذا (هزینه_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << هزینه_Profiled);
        }
        costs[mtp->id()] = std::make_pair(هزینه_Estimate, هزینه_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    صحيح مجموع_Estimates = 0;
    صحيح missingProfiles = 0;
    لكل (V3_گراف_رأس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t هزینه_Estimate = costs[mtp->id()].اولی;
        ثابت uint64_t هزینه_Profiled = costs[mtp->id()].دومی;
        UINFO(9, "ce = " << هزینه_Estimate << " cp=" << هزینه_Profiled);
        UASSERT(هزینه_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(هزینه_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t هزینه_Profiled32 = static_cast<uint32_t>(هزینه_Profiled);
        uint32_t هزینه_مقصد_Use = هزینه_Profiled32;
        إذا (!هزینه_Profiled32) {
            هزینه_مقصد_Use = هزینه_Estimate;
            إذا (هزینه_Estimate != 0) ++missingProfiles;
        }
        إذا (هزینه_Estimate != 0) ++مجموع_Estimates;
        mtp->هزینه(هزینه_مقصد_Use);
        mtp->اولویت_2(هزینه_مقصد_Use);
    }

    إذا (missingProfiles) {
        إذا (File_سطر* ثابت fl = V3Control::get_Profile_داده_File_سطر()) {
            إذا (V3Control::شامل_است_MTask_Profile_داده()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << مجموع_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

فراغ finalizeCosts(V3_گراف* execMTaskGraphp) {
    گراف_جریان_Unordered ser(execMTaskGraphp, گراف_Way::معکوس_2);
    طالما (ثابت V3_گراف_رأس* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3_گراف_رأس*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        لكل (V3_گراف_یال& یال : mtp->بیرون_یالها()) {
            ثابت ExecMTask* ثابت followp = یال.بالاترین()->as<ExecMTask>();
            إذا ((followp->اولویت_2() + mtp->هزینه()) > mtp->اولویت_2()) {
                mtp->اولویت_2(followp->اولویت_2() + mtp->هزینه());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_متعدی_یالها();

    // Record summary stats for final m_tasks graph.
    ثابت خودکار report = execMTaskGraphp->parallelismReport([](ثابت V3_گراف_رأس* vtxp) {  //
        أرجع vtxp->as<ثابت ExecMTask>()->هزینه();
    });
    V3Stats::جمع_کن_Stat("MTask graph, final, critical path cost", report.critical_مسیر_هزینه());
    V3Stats::جمع_کن_Stat("MTask graph, final, total graph cost", report.مجموع_گراف_هزینه());
    V3Stats::جمع_کن_Stat("MTask graph, final, mtask count", report.رأس_شمار());
    V3Stats::جمع_کن_Stat("MTask graph, final, edge count", report.یال_شمار());
    V3Stats::جمع_کن_Stat("MTask graph, final, parallelism factor", report.parallelism_ضریب());
    إذا (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_مسیر_هزینه());
        UINFO(0, "    Total graph cost = " << report.مجموع_گراف_هزینه());
        UINFO(0, "    MTask vertex count = " << report.رأس_شمار());
        UINFO(0, "    Edge count = " << report.یال_شمار());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ضریب());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_گره_Module* ثابت modp = v3Global.rootp()->بالاترین_Modulep();
    File_سطر* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->پیدا_کن_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(درست);
    varp->valuep(جديد AstConst{fl, nDependencies});
    modp->جمع_کن_Stmtsp(varp);
    أرجع varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& روش) {
    File_سطر* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت بالاترین_Scopep = v3Global.rootp()->بالاترین_Scopep()->scopep();
    refp->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::VlSyms{}, بالاترین_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = جديد AstCStmt{fl};
    stmtp->جمع_کن(refp);
    stmtp->جمع_کن("." + روش + ";");
    أرجع stmtp;
}

فراغ جمع_کن_MTask_مقصد_Function(ثابت Thread_زمانبندی& زمانبندی, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->بالاترین_Scopep()->scopep();
    Ast_گره_Module* ثابت modp = v3Global.rootp()->بالاترین_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت خودکار جمع_کن_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        funcp->جمع_کن_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (زمانبندی.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        إذا (v3Global.opt.profExec()) {
            جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->جمع_کن_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        إذا (v3Global.opt.profExec()) {
            جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    إذا (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        جمع_کن_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::مقصد_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = جديد AstCCall{fl, mtaskp->funcp()};
    callp->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::VlSyms{}, scopep->nameDotless()});
    callp->dtype_مجموعه_Void();
    funcp->جمع_کن_Stmtsp(callp->makeStmt());

    إذا (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        جمع_کن_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::مقصد_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    لكل (ثابت V3_گراف_یال& یال : mtaskp->بیرون_یالها()) {
        ثابت ExecMTask* ثابت nextp = یال.بالاترین()->as<ExecMTask>();
        إذا (زمانبندی.threadId(nextp) != threadId && زمانبندی.شامل_است(nextp)) {
            funcp->جمع_کن_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::وکتور<AstCFunc*> createThreadFunctions(ثابت Thread_زمانبندی& زمانبندی,
                                                   ثابت string& برچسب) {
    Ast_گره_Module* ثابت modp = v3Global.rootp()->بالاترین_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    std::وکتور<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    لكل (ثابت std::وکتور<ثابت ExecMTask*>& thread : زمانبندی.m_نخها) {
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            إذا (ثابت uint32_t nDependencies = زمانبندی.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_مقصد_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    لكل (ثابت std::وکتور<ثابت ExecMTask*>& thread : زمانبندی.m_نخها) {
        إذا (thread.خالی()) استمر;
        ثابت uint32_t threadId = زمانبندی.threadId(thread.front());
        ثابت string name{"__Vthread__" + برچسب + "__s" + cvt_مقصد_Str(زمانبندی.id()) + "__t"
                          + cvt_مقصد_Str(threadId)};
        AstCFunc* ثابت funcp = جديد AstCFunc{fl, name, اشاره_تهی, "void"};
        modp->جمع_کن_Stmtsp(funcp);
        funcps.افزودن_پشت(funcp);
        funcp->isStatic(درست);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(درست);
        funcp->مدخل_نقطه(درست);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->جمع_کن_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_تخصیص_بده(modp)});
        funcp->جمع_کن_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_تخصیص_بده()});

        // Invoke each mtask scheduled to this thread from the thread function
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            جمع_کن_MTask_مقصد_Function(زمانبندی, threadId, funcp, mtaskp, stateVars);
        }
    }

    أرجع funcps;
}

فراغ جمع_کن_Thread_شروع_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& برچسب = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت خودکار جمع_کن_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جمع_کن_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (v3Global.opt.profExec()) {
        جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    جمع_کن_CStmt("vlSymsp->__Vm_even_cycle__" + برچسب + " = !vlSymsp->__Vm_even_cycle__" + برچسب + ";");

    إذا (!v3Global.opt.hier_بلوکها().خالی()) جمع_کن_CStmt("std::vector<size_t> indexes;");
}

فراغ جمع_کن_Thread_انتها_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت خودکار جمع_کن_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->جمع_کن_Stmtsp(جديد AstCStmt{flp, stmt});
    };

    جمع_کن_CStmt("Verilated::mtaskId(0);");
    إذا (v3Global.opt.profExec()) { جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
فراغ جمع_کن_Thread_شروع_مقصد_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp,
                               ثابت std::وکتور<AstCFunc*>& funcps, uint32_t زمانبندی_Id) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& برچسب = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvt_مقصد_Str(زمانبندی_Id) + "__" + برچسب, funcps.اندازه());
    لكل (AstCFunc* ثابت funcp : funcps) {
        funcp->جمع_کن_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت خودکار جمع_کن_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جمع_کن_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    ثابت uint32_t آخرین = funcps.اندازه() - 1;
    إذا (!v3Global.opt.hier_بلوکها().خالی() && آخرین > 0) {
        جمع_کن_CStmt("for (size_t i = 0; i < " + std::مقصد_string(آخرین) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    لكل (AstCFunc* ثابت funcp : funcps) {
        إذا (i != آخرین) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            execGraphp->runfuncp()->جمع_کن_Stmtsp(cstmtp);
            cstmtp->جمع_کن("vlSymsp->__Vm_threadPoolp->workerp(");
            إذا (v3Global.opt.hier_فرزند() || !v3Global.opt.hier_بلوکها().خالی()) {
                cstmtp->جمع_کن("indexes[" + std::مقصد_string(i) + "]");
            } وإلا {
                cstmtp->جمع_کن(std::مقصد_string(i));
            }
            cstmtp->جمع_کن(")->addTask(");
            cstmtp->جمع_کن(جديد Ast_Addr_از_CFunc{fl, funcp});
            cstmtp->جمع_کن(", vlSelf, vlSymsp->__Vm_even_cycle__" + برچسب + ");");
        } وإلا {
            // The last will run on the main thread.
            AstCCall* ثابت callp = جديد AstCCall{fl, funcp};
            callp->dtype_مجموعه_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + برچسب);
            execGraphp->runfuncp()->جمع_کن_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::جمع_کن_Stat_جمع("Optimizations, Thread schedule total tasks", i);

    إذا (v3Global.opt.profExec()) {
        جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->جمع_کن_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + برچسب + ")"));
    إذا (v3Global.opt.profExec()) {
        جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    إذا (!v3Global.opt.hier_بلوکها().خالی() && آخرین > 0) {
        جمع_کن_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

فراغ processMTaskBodies(Ast_Exec_گراف* ثابت execGraphp) {
    لكل (V3_گراف_رأس* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_گره* stmtsp = funcp->stmtsp()->unlink_Fr_پشت_With_بعدی();

        // Helper function to make the code a bit more legible
        ثابت خودکار جمع_کن_CStmt = [=](ثابت string& stmt) -> فراغ {  //
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        إذا (v3Global.opt.profExec()) {
            std::string آرگومانها = std::مقصد_string(mtaskp->id());
            آرگومانها += ", " + std::مقصد_string(mtaskp->predict_شروع());
            آرگومانها += ", \"";
            إذا (v3Global.opt.hier_فرزند()) آرگومانها += v3Global.opt.بالاترین_Module();
            آرگومانها += "\"";
            جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + آرگومانها + ");");
        }
        // Set mtask ID in the run-time system
        جمع_کن_CStmt("Verilated::mtaskId(" + std::مقصد_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->جمع_کن_Stmtsp(stmtsp);
        // Flush message queue
        جمع_کن_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        إذا (v3Global.opt.profExec()) {
            ثابت std::string& آرگومانها = std::مقصد_string(mtaskp->هزینه());
            جمع_کن_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + آرگومانها + ");");
        }
    }
}

فراغ implement_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp, ثابت Thread_زمانبندی& زمانبندی) {
    // Nothing to be done if there are no MTasks in the graph at all.
    إذا (execGraphp->depGraphp()->خالی()) أرجع;

    // Create a function to be run by each thread.
    ثابت std::وکتور<AstCFunc*>& funcps = createThreadFunctions(زمانبندی, execGraphp->name());
    UASSERT(!funcps.خالی(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    جمع_کن_Thread_شروع_مقصد_Exec_گراف(execGraphp, funcps, زمانبندی.id());
}

فراغ create_پیاپی_Function(Ast_Exec_گراف* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_سطر* ثابت flp = execGraphp->fileline();
    Ast_گره_Module* ثابت modp = v3Global.rootp()->بالاترین_Modulep();
    AstCFunc* ثابت funcp = جديد AstCFunc{flp, "runExecGraph_" + execGraphp->name(), اشاره_تهی};
    funcp->isLoose(درست);
    funcp->voidSelf(درست);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(درست);
    modp->جمع_کن_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

فراغ جمع_کن_Trigger_Masks(Ast_Exec_گراف* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_سطر* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> کلمه_Masks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            لكل (ثابت uint32_t شاخص : trigp->indices()) {
                کلمه_Masks[VL_BITWORD_Q(شاخص)] |= VL_بیت_Q(شاخص);
            }
        });
        AstConsArray* ثابت consp = جديد AstConsArray{flp, triggersp->dtypep(),
                                                     جديد AstConst{flp, AstConst::Unsized64{}, 0}};
        لكل (ثابت خودکار& کلمه_ماسک : کلمه_Masks) {
            consp->valuep(کلمه_ماسک.اولی,
                          جديد AstConst{flp, AstConst::Unsized64{}, کلمه_ماسک.دومی});
        }
        execGraphp->جمع_کن_Maskps(V3ConstPool::پیدا_کن(consp));
        VL_DO_DANGLING(consp->delete_درخت(), consp);
    }
}

فراغ جمع_کن_Trigger_Costs(Ast_Exec_گراف* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_سطر* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::وکتور<uint32_t> costs(nTriggers, 0);
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            إذا (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t هزینه = V3_Instr_شمار::شمار(ifp, خطأ);
                لكل (ثابت uint32_t شاخص : trigp->indices()) costs.at(شاخص) += هزینه;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = جديد AstUnpackArrayDType{flp, netlistp->پیدا_کن_بیت_DType(32, 32, VSigning::UNSIGNED),
                                  جديد AstRange{flp, static_cast<صحيح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->جمع_کن_Typesp(dtypep);
    AstConsArray* ثابت consp = جديد AstConsArray{flp, dtypep, جديد AstConst{flp, 0}};
    لكل (uint32_t شاخص = 0; شاخص < nTriggers; ++شاخص) {
        إذا (costs[شاخص]) consp->valuep(شاخص, جديد AstConst{flp, costs[شاخص]});
    }
    execGraphp->triggerCostsp(V3ConstPool::پیدا_کن(consp));
    VL_DO_DANGLING(consp->delete_درخت(), consp);
}

// Called by Verilator top stage
فراغ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::وکتور<Ast_Exec_گراف*> execGraphps;
    netlistp->بالاترین_Modulep()->foreach([&](Ast_Exec_گراف* egp) { execGraphps.emplace_پشت(egp); });

    // Process each
    لكل (Ast_Exec_گراف* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_خالی_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        إذا (execGraphp->depGraphp()->خالی()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_پشت()->delete_درخت(), execGraphp);
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

        create_پیاپی_Function(execGraphp);
        جمع_کن_Thread_شروع_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::وکتور<Thread_زمانبندی> packed = Pack_نخها::apply(*execGraphp->depGraphp());
        V3Stats::جمع_کن_Stat_جمع("Optimizations, Thread schedule count",
                            static_cast<مزدوج>(packed.اندازه()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        لكل (ثابت Thread_زمانبندی& زمانبندی : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_گراف(execGraphp, زمانبندی);
        }

        جمع_کن_Thread_انتها_Wrapper(execGraphp);

        جمع_کن_Trigger_Masks(execGraphp);
        جمع_کن_Trigger_Costs(execGraphp);
    }
}

فراغ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].اولی, 1000);
        UASSERT_SELFTEST(costs[1].دومی, 1000);
        UASSERT_SELFTEST(costs[2].اولی, 2000);
        UASSERT_SELFTEST(costs[2].دومی, 0);
        UASSERT_SELFTEST(costs[3].اولی, 3000);
        UASSERT_SELFTEST(costs[3].دومی, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].اولی, 2500000);
        UASSERT_SELFTEST(costs[1].دومی, 5000000);
        UASSERT_SELFTEST(costs[2].اولی, 5000000);
        UASSERT_SELFTEST(costs[2].دومی, 10000000);
        UASSERT_SELFTEST(costs[3].اولی, 7500000);
        UASSERT_SELFTEST(costs[3].دومی, 1);
    }

    Pack_نخها::selfTest();
}

}  // namespace V3ExecGraph
