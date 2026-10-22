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

AstCFunc* ExecMTask::createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_ڳنڍ_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::منزل_string(id);
    AstCFunc* ثابت newp = جديد AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(سچ);
    newp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(سچ);
    newp->جوڙيو_Stmtsp(stmtsp);
    إذا (scopep) scopep->جوڙيو_Blocksp(newp);
    أرجع newp;
}

ExecMTask::ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep,
                     Ast_ڳنڍ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_گراف_راس{execGraphp->depGraphp()},
      m_id{s_ايندڙ_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).منزل_String()} {}

فراغ ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvt_منزل_Hex(هذا);
    إذا (ترجيح_2() || لاڳت()) str << " [pr=" << ترجيح_2() << " c=" << cvt_منزل_Str(لاڳت()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_ايندڙ_Id{0};

نطاق V3_Exec_گراف {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
صنف Thread_وقت_جدول final {
    دوست صنف Pack_ڌاڳا;

    uint32_t m_id;  // Unique ID of a schedule
    ساكن uint32_t s_ايندڙ_Id;  // Next ID number to use
    std::unordered_سيٽ<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_پڇاڙي_وقت = 0;  // Latest task end time in this schedule

عام:
    // CONSTANTS
    ساكن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    بنية MTaskState final {
        uint32_t completion_وقت = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = خالي_اشارو;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::ويڪٽر<std::ويڪٽر<ثابت ExecMTask*>> m_ڌاڳا;

    // Global state for each mtask.
    ساكن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    واضح Thread_وقت_جدول(uint32_t n_ڌاڳا)
        : m_id{s_ايندڙ_Id++}
        , m_ڌاڳا{n_ڌاڳا} {}
    Thread_وقت_جدول(Thread_وقت_جدول&&) = افتراضي;
    Thread_وقت_جدول& عامل=(Thread_وقت_جدول&&) = افتراضي;

خاص:
    VL_UNCOPYABLE(Thread_وقت_جدول);

    ساكن constexpr مزدوج s_thread_Box_ويڪر = 2.5;
    ساكن constexpr مزدوج s_thread_Box_اوچائي = 1.5;
    ساكن constexpr مزدوج s_horizontalGap = s_thread_Box_ويڪر / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساكن فراغ dumpDotFilePrefixedAlways(ثابت std::ويڪٽر<Thread_وقت_جدول>& schedules,
                                          ثابت string& nameComment, uint32_t n_ڌاڳا) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_ڌاڳا);
    }
    ساكن فراغ dumpDotFile(ثابت std::ويڪٽر<Thread_وقت_جدول>& schedules, ثابت string& filename,
                            uint32_t n_ڌاڳا) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        إذا (logp->fail()) v3fatal("Can't write file: " << filename);

        //سرو
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        لكل (uint32_t i = 0; i < n_ڌاڳا; ++i) {
            ثابت string name = "t" + std::منزل_string(i);
            ثابت string label = "Thread " + std::منزل_string(i);
            constexpr مزدوج posX = -s_horizontalGap;
            ثابت مزدوج posY = -static_cast<مزدوج>(i) * s_thread_Box_اوچائي;
            dump_Dot_File_Emit_بلاڪ(logp, name, label, s_thread_Box_ويڪر, s_thread_Box_اوچائي, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_لاڳت = 0;
        لكل (ثابت خودڪار& state : Thread_وقت_جدول::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.پهريون;
            max_لاڳت = std::max(max_لاڳت, mtaskp->لاڳت());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_في_Longest_Mtask = 4;
        ثابت uint32_t segment_لاڳت = max_لاڳت / segments_في_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::ويڪٽر<مزدوج> offsets(n_ڌاڳا, 0.0);
        لكل (ثابت Thread_وقت_جدول& وقت_جدول : schedules) {
            إذا (وقت_جدول.mtasks.خالي_جايون()) استمر;
            استعمال ڪالم = std::ويڪٽر<ثابت ExecMTask*>;
            std::ويڪٽر<ڪالم> columns = {{}};

            // Order tasks based on their start time
            بنية Cmp final {
                منطقي عامل()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    إذا (شروعات_وقت(a) == شروعات_وقت(b)) أرجع threadId(a) < threadId(b);
                    أرجع شروعات_وقت(a) < شروعات_وقت(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(وقت_جدول.mtasks.begin(),
                                                             وقت_جدول.mtasks.پڇاڙي());

            لكل (ثابت ExecMTask* ثابت mtaskp : tasks) {
                ڪالم& ڪالم_2 = columns.پٺ();
                UASSERT(ڪالم_2.سائيز() <= n_ڌاڳا, "Invalid partitioning");

                منطقي intersects = سچ;
                لكل (ثابت ExecMTask* ثابت earlierMtask : ڪالم_2) {
                    إذا (پڇاڙي_وقت(mtaskp) <= شروعات_وقت(earlierMtask)
                        || شروعات_وقت(mtaskp) >= پڇاڙي_وقت(earlierMtask)) {
                        intersects = خطأ;
                        اكسر;
                    }
                }
                إذا (intersects) {
                    ڪالم_2.emplace_پٺ(mtaskp);
                } وإلا {
                    columns.emplace_پٺ(ڪالم{mtaskp});
                }
            }

            UASSERT(!columns.front().خالي_جايون(), "Should be populated by mtasks");

            لكل (ثابت ڪالم& ڪالم_2 : columns) {
                مزدوج آخري_ڪالم_وٿي = 0;
                لكل (ثابت ExecMTask* ثابت mtaskp : ڪالم_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, وقت_جدول, segment_لاڳت, offsets);
                    آخري_ڪالم_وٿي = std::max(آخري_ڪالم_وٿي, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.پڇاڙي(), آخري_ڪالم_وٿي);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_ڌاڳا);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            لكل (ثابت std::ويڪٽر<ثابت ExecMTask*>& thread : وقت_جدول.m_ڌاڳا) {
                إذا (thread.خالي_جايون()) اكسر;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.پٺ()->name() << " -> fork_"
                      << static_cast<صحيح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                لكل (سائيز_t i = 1; i < thread.سائيز(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
                    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : mtaskp->ٻاهريون_ڪنارا()) {
                        ثابت ExecMTask* ثابت چوٽي_MTaskp = ڪنارو.چوٽي()->cast<ثابت ExecMTask>();
                        إذا (چوٽي_MTaskp && وقت_جدول.شامل_آهي(چوٽي_MTaskp)
                            && threadId(چوٽي_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << چوٽي_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ساكن فراغ dump_Dot_File_Emit_بلاڪ(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, مزدوج ويڪر,
                                     مزدوج اوچائي, مزدوج xPos, مزدوج yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << ويڪر
              << " height=" << اوچائي << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساكن فراغ dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت Thread_وقت_جدول& وقت_جدول,
                                     uint32_t segment_لاڳت, std::ويڪٽر<مزدوج>& offsets) {
        لكل (صحيح i = 0; i < mtaskp->ڌاڳا(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::منزل_string(i);
            ثابت string label = mtaskp->name() + " (" + std::منزل_string(شروعات_وقت(mtaskp)) + ':'
                                 + std::منزل_string(پڇاڙي_وقت(mtaskp)) + ')'
                                 + "\\ncost=" + std::منزل_string(mtaskp->لاڳت())
                                 + "\\npriority=" + std::منزل_string(mtaskp->ترجيح_2());
            ثابت مزدوج ويڪر
                = std::max(s_thread_Box_ويڪر,
                           s_thread_Box_ويڪر * static_cast<مزدوج>(mtaskp->لاڳت()) / segment_لاڳت);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * وقت_جدول.m_ڌاڳا.سائيز();
            ثابت مزدوج xPos = ويڪر / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += ويڪر + s_horizontalGap;
            ثابت مزدوج yPos = -s_thread_Box_اوچائي * static_cast<مزدوج>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_بلاڪ(logp, name, label, ويڪر, s_thread_Box_اوچائي, xPos, yPos,
                                 fillColor);
        }
    }

    ساكن فراغ dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, مزدوج وٿي,
                                    uint32_t n_ڌاڳا) {
        ثابت string& name = "fork_" + std::منزل_string(static_cast<صحيح>(وٿي));
        constexpr مزدوج ويڪر = s_thread_Box_ويڪر / 8;
        ثابت مزدوج اوچائي = s_thread_Box_اوچائي * n_ڌاڳا;
        ثابت مزدوج xPos = وٿي - s_horizontalGap / 2;
        ثابت مزدوج yPos
            = -static_cast<مزدوج>(n_ڌاڳا) / 2 * s_thread_Box_اوچائي + s_thread_Box_اوچائي / 2;
        dump_Dot_File_Emit_بلاڪ(logp, name, "", ويڪر, اوچائي, xPos, yPos, "black");
    }

عام:
    ساكن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت خودڪار& it = s_mtaskState.ڳوليو(mtaskp);
        أرجع it != s_mtaskState.پڇاڙي() ? it->ٻيو.threadId : UNASSIGNED;
    }
    ساكن uint32_t شروعات_وقت(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_وقت - mtaskp->لاڳت();
    }
    ساكن uint32_t پڇاڙي_وقت(ثابت ExecMTask* mtaskp) {
        أرجع s_mtaskState.at(mtaskp).completion_وقت;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t نتيجو = 0;
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : mtaskp->اندريون_ڪنارا()) {
            ثابت ExecMTask* ثابت prevp = ڪنارو.fromp()->as<ExecMTask>();
            إذا (threadId(prevp) != thisThreadId && شامل_آهي(prevp)) ++نتيجو;
        }
        أرجع نتيجو;
    }

    uint32_t id() ثابت { أرجع m_id; }
    uint32_t وقت_جدول_On(ثابت ExecMTask* mtaskp, uint32_t بهترين_Thread_Id) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t بهترين_پڇاڙي_وقت = mtaskp->predict_شروعات() + mtaskp->لاڳت();
        m_پڇاڙي_وقت = std::max(m_پڇاڙي_وقت, بهترين_پڇاڙي_وقت);
        s_mtaskState[mtaskp].completion_وقت = بهترين_پڇاڙي_وقت;
        s_mtaskState[mtaskp].threadId = بهترين_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::ويڪٽر<ثابت ExecMTask*>& بهترين_Thread = m_ڌاڳا[بهترين_Thread_Id];
        إذا (!بهترين_Thread.خالي_جايون()) s_mtaskState[بهترين_Thread.پٺ()].nextp = mtaskp;

        // Add the MTask to the schedule
        بهترين_Thread.ڌڪو_پٺ(mtaskp);
        أرجع بهترين_پڇاڙي_وقت;
    }
    منطقي شامل_آهي(ثابت ExecMTask* mtaskp) ثابت { أرجع mtasks.ڳڻپ(mtaskp); }
    uint32_t پڇاڙي_وقت() ثابت { أرجع m_پڇاڙي_وقت; }
};

uint32_t Thread_وقت_جدول::s_ايندڙ_Id = 0;
std::unordered_map<ثابت ExecMTask*, Thread_وقت_جدول::MTaskState> Thread_وقت_جدول::s_mtaskState{};
constexpr مزدوج V3_Exec_گراف::Thread_وقت_جدول::s_thread_Box_ويڪر;

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
صنف Pack_ڌاڳا final {
    // TYPES
    بنية MTaskCmp final {
        منطقي عامل()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            أرجع ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_n_ڌاڳا;  // Number of threads
    ثابت uint32_t m_n_Hier_ڌاڳا;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    واضح Pack_ڌاڳا(uint32_t n_ڌاڳا = v3Global.opt.ڌاڳا(),
                         uint32_t n_Hier_ڌاڳا = v3Global.opt.hier_ڌاڳا(),
                         بنانشان sandbagNumerator = 30, بنانشان sandbagDenom = 100)
        : m_n_ڌاڳا{n_ڌاڳا}
        , m_n_Hier_ڌاڳا{n_Hier_ڌاڳا}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_ڌاڳا() = افتراضي;
    VL_UNCOPYABLE(Pack_ڌاڳا);

    // METHODS
    uint32_t completion_وقت(ثابت Thread_وقت_جدول& وقت_جدول, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        إذا (!وقت_جدول.شامل_آهي(mtaskp)) أرجع 0;
        ثابت Thread_وقت_جدول::MTaskState& state = وقت_جدول.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_وقت_جدول::UNASSIGNED, "Mtask should have assigned thread");
        إذا (threadId == state.threadId) {
            // No overhead on same thread
            أرجع state.completion_وقت;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_پڇاڙي_وقت
            = state.completion_وقت + (m_sandbagNumerator * mtaskp->لاڳت()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        إذا (state.nextp) {
            ثابت uint32_t successor_پڇاڙي_وقت
                = completion_وقت(وقت_جدول, state.nextp, state.threadId);
            إذا ((sandbagged_پڇاڙي_وقت >= successor_پڇاڙي_وقت) && (successor_پڇاڙي_وقت > 1)) {
                sandbagged_پڇاڙي_وقت = successor_پڇاڙي_وقت - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_پڇاڙي_وقت);
        أرجع sandbagged_پڇاڙي_وقت;
    }

    ساكن منطقي isReady(Thread_وقت_جدول& وقت_جدول, ثابت ExecMTask* mtaskp) {
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو_اندريون : mtaskp->اندريون_ڪنارا()) {
            ثابت ExecMTask* ثابت prevp = ڪنارو_اندريون.fromp()->as<ثابت ExecMTask>();
            إذا (وقت_جدول.threadId(prevp) == Thread_وقت_جدول::UNASSIGNED) {
                // This predecessor is not assigned yet
                أرجع خطأ;
            }
        }
        أرجع سچ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::ويڪٽر<Thread_وقت_جدول> pack(V3_گراف& mtask_گراف) {
        std::ويڪٽر<Thread_وقت_جدول> نتيجو;
        نتيجو.emplace_پٺ(Thread_وقت_جدول{m_n_ڌاڳا});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        تعداد صنف Scheduling_گهڻائي : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_گهڻائي گهڻائي = Scheduling_گهڻائي::SCHEDULING;

        // Time each thread is occupied until
        std::ويڪٽر<uint32_t> busyUntil(std::max(m_n_ڌاڳا, m_n_Hier_ڌاڳا), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::سيٽ<ExecMTask*, MTaskCmp> readyMTasks;
        صحيح maxThreadWorkers = 1;

        // Build initial ready list
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            إذا (isReady(نتيجو.پٺ(), mtaskp)) readyMTasks.وجهو(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->ڌاڳا());
        }

        طالما (!readyMTasks.خالي_جايون()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t بهترين_وقت = 0xffffffff;
            uint32_t بهترين_Thread_Id = 0;
            ExecMTask* بهترين_Mtaskp = خالي_اشارو;  // Todo: const ExecMTask*
            Thread_وقت_جدول& وقت_جدول = نتيجو.پٺ();
            لكل (uint32_t threadId = 0; threadId < وقت_جدول.m_ڌاڳا.سائيز(); ++threadId) {
                لكل (ExecMTask* ثابت mtaskp : readyMTasks) {
                    إذا (گهڻائي != Scheduling_گهڻائي::WIDE_TASK_SCHEDULING && mtaskp->ڌاڳا() > 1) {
                        گهڻائي = Scheduling_گهڻائي::WIDE_TASK_DISCOVERED;
                        استمر;
                    }
                    إذا (گهڻائي == Scheduling_گهڻائي::WIDE_TASK_SCHEDULING && mtaskp->ڌاڳا() <= 1)
                        استمر;

                    uint32_t وقت_Begin = busyUntil[threadId];
                    إذا (وقت_Begin > بهترين_وقت) {
                        UINFO(6, "th " << threadId << " busy until " << وقت_Begin
                                       << ", later than bestTime " << بهترين_وقت
                                       << ", skipping thread.");
                        اكسر;
                    }
                    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : mtaskp->اندريون_ڪنارا()) {
                        ثابت ExecMTask* ثابت priorp = ڪنارو.fromp()->as<ExecMTask>();
                        ثابت uint32_t اڳواٽ_پڇاڙي_وقت = completion_وقت(وقت_جدول, priorp, threadId);
                        إذا (اڳواٽ_پڇاڙي_وقت > وقت_Begin) وقت_Begin = اڳواٽ_پڇاڙي_وقت;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << وقت_Begin
                                     << " on thread " << threadId);
                    إذا ((وقت_Begin < بهترين_وقت)
                        || ((وقت_Begin == بهترين_وقت)
                            && بهترين_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ترجيح_2() > بهترين_Mtaskp->ترجيح_2()))) {
                        بهترين_وقت = وقت_Begin;
                        بهترين_Thread_Id = threadId;
                        بهترين_Mtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t پڇاڙي_وقت = وقت_جدول.پڇاڙي_وقت();

            إذا (!بهترين_Mtaskp && گهڻائي == Scheduling_گهڻائي::WIDE_TASK_DISCOVERED) {
                گهڻائي = Scheduling_گهڻائي::WIDE_TASK_SCHEDULING;
                ثابت uint32_t سائيز = m_n_Hier_ڌاڳا / maxThreadWorkers;
                UASSERT(سائيز, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                إذا (وقت_جدول.s_mtaskState.خالي_جايون()) نتيجو.clear();
                نتيجو.emplace_پٺ(Thread_وقت_جدول{سائيز});
                std::fill(busyUntil.begin(), busyUntil.پڇاڙي(), پڇاڙي_وقت);
                استمر;
            }

            إذا (!بهترين_Mtaskp && گهڻائي == Scheduling_گهڻائي::WIDE_TASK_SCHEDULING) {
                گهڻائي = Scheduling_گهڻائي::SCHEDULING;
                UASSERT(!وقت_جدول.s_mtaskState.خالي_جايون(), "Mtask should be added");
                نتيجو.emplace_پٺ(Thread_وقت_جدول{m_n_ڌاڳا});
                std::fill(busyUntil.begin(), busyUntil.پڇاڙي(), پڇاڙي_وقت);
                استمر;
            }

            UASSERT(بهترين_Mtaskp, "Should have found some task");

            بهترين_Mtaskp->predict_شروعات(بهترين_وقت);
            ثابت uint32_t بهترين_پڇاڙي_وقت = وقت_جدول.وقت_جدول_On(بهترين_Mtaskp, بهترين_Thread_Id);
            busyUntil[بهترين_Thread_Id] = بهترين_پڇاڙي_وقت;

            // Update the ready list
            ثابت سائيز_t erased = readyMTasks.erase(بهترين_Mtaskp);
            UASSERT_OBJ(erased > 0, بهترين_Mtaskp, "Should have erased something?");
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو_ٻاهريون : بهترين_Mtaskp->ٻاهريون_ڪنارا()) {
                ExecMTask* ثابت nextp = ڪنارو_ٻاهريون.چوٽي()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(وقت_جدول.threadId(nextp) == Thread_وقت_جدول::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.ڳوليو(nextp) == readyMTasks.پڇاڙي(), nextp,
                            "Tasks after one being assigned should not be ready");
                إذا (isReady(وقت_جدول, nextp)) {
                    readyMTasks.وجهو(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        إذا (dump_گراف_Level() >= 4)
            Thread_وقت_جدول::dumpDotFilePrefixedAlways(نتيجو, "schedule", m_n_ڌاڳا);

        أرجع نتيجو;
    }

عام:
    // SELF TEST
    ساكن فراغ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_سٽ* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->ڳوليو_UInt64_DType()};
        AstScope* ثابت scopep = جديد AstScope{flp, خالي_اشارو, "test", خالي_اشارو, خالي_اشارو};
        AstVarScope* ثابت trigVscp = جديد AstVarScope{flp, scopep, varp};
        self_Test_Hier_پهريون(trigVscp);
        self_Test_عام_پهريون(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_وڻ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_وڻ(), scopep);
        VL_DO_DANGLING(varp->delete_وڻ(), varp);
    }
    ساكن فراغ self_Test_عام_پهريون(AstVarScope* trigVscp) {
        File_سٽ* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودڪار makeBody = [&]() -> Ast_ڳنڍ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t0->لاڳت(1000);
        t0->ترجيح_2(1100);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t1->لاڳت(100);
        t1->ترجيح_2(100);
        ExecMTask* ثابت t2 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t2->لاڳت(100);
        t2->ترجيح_2(100);
        t2->ڌاڳا(2);
        ExecMTask* ثابت t3 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t3->لاڳت(100);
        t3->ترجيح_2(100);
        t3->ڌاڳا(3);
        ExecMTask* ثابت t4 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t4->لاڳت(100);
        t4->ترجيح_2(100);
        t4->ڌاڳا(3);
        ExecMTask* ثابت t5 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t5->لاڳت(100);
        t5->ترجيح_2(100);
        ExecMTask* ثابت t6 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t6->لاڳت(100);
        t6->ترجيح_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        جديد V3_گراف_ڪنارو{&گراف, t0, t1, 1};
        جديد V3_گراف_ڪنارو{&گراف, t0, t2, 1};
        جديد V3_گراف_ڪنارو{&گراف, t2, t3, 1};
        جديد V3_گراف_ڪنارو{&گراف, t2, t4, 1};
        جديد V3_گراف_ڪنارو{&گراف, t3, t5, 1};
        جديد V3_گراف_ڪنارو{&گراف, t4, t6, 1};

        constexpr uint32_t ڌاڳا = 2;
        constexpr uint32_t hier_ڌاڳا = 6;
        Pack_ڌاڳا packer{ڌاڳا, hier_ڌاڳا,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::ويڪٽر<Thread_وقت_جدول> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.سائيز(), 3);
        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا.سائيز(), ڌاڳا);
        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[0].سائيز(), 2);
        لكل (سائيز_t i = 1; i < scheduled[0].m_ڌاڳا.سائيز(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[i].سائيز(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا.سائيز(), hier_ڌاڳا / 3);
        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_ڌاڳا.سائيز(), ڌاڳا);
        UASSERT_SELFTEST(scheduled[2].m_ڌاڳا[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_ڌاڳا[1][0], t6);

        UASSERT_SELFTEST(Thread_وقت_جدول::s_mtaskState.سائيز(), 7);

        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t4, 5), 1360);

        لكل (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_وڻ();
        VL_DO_DANGLING(execGraphp->delete_وڻ(), execGraphp);
        Thread_وقت_جدول::s_mtaskState.clear();
    }
    ساكن فراغ self_Test_Hier_پهريون(AstVarScope* trigVscp) {
        File_سٽ* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودڪار makeBody = [&]() -> Ast_ڳنڍ_Stmt* { أرجع جديد AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t0->لاڳت(1000);
        t0->ترجيح_2(1100);
        t0->ڌاڳا(2);
        ExecMTask* ثابت t1 = جديد ExecMTask{execGraphp, خالي_اشارو, makeBody()};
        t1->لاڳت(100);
        t1->ترجيح_2(100);

        /*
                          0
                          |
                          1
        */
        جديد V3_گراف_ڪنارو{&گراف, t0, t1, 1};

        constexpr uint32_t ڌاڳا = 1;
        constexpr uint32_t hier_ڌاڳا = 2;
        Pack_ڌاڳا packer{ڌاڳا, hier_ڌاڳا,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::ويڪٽر<Thread_وقت_جدول> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.سائيز(), 2);
        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا.سائيز(), hier_ڌاڳا / 2);
        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[0].سائيز(), 1);
        لكل (سائيز_t i = 1; i < scheduled[0].m_ڌاڳا.سائيز(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[i].سائيز(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ڌاڳا[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا.سائيز(), ڌاڳا);
        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[0].سائيز(), 1);
        لكل (سائيز_t i = 1; i < scheduled[1].m_ڌاڳا.سائيز(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[i].سائيز(), 0);
        UASSERT_SELFTEST(scheduled[1].m_ڌاڳا[0][0], t1);

        UASSERT_SELFTEST(Thread_وقت_جدول::s_mtaskState.سائيز(), 2);

        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_وقت_جدول::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t1, 1), 1130);

        لكل (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_وڻ();
        VL_DO_DANGLING(execGraphp->delete_وڻ(), execGraphp);
        Thread_وقت_جدول::s_mtaskState.clear();
    }

    ساكن std::ويڪٽر<Thread_وقت_جدول> apply(V3_گراف& mtask_گراف) {
        أرجع Pack_ڌاڳا{}.pack(mtask_گراف);
    }
};

استعمال EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
استعمال Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

فراغ normalizeCosts(Costs& costs) {
    ثابت خودڪار scale_لاڳت = [](uint64_t قدر, مزدوج multiplier) {
        مزدوج scaled = static_cast<مزدوج>(قدر) * multiplier;
        إذا (قدر && scaled < 1) scaled = 1;
        أرجع static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t جوڙ_لاڳت_Profiled = 0;  // For data with estimate and profile
    uint64_t جوڙ_لاڳت_Estimate = 0;  // For data with estimate and profile
    لكل (ثابت خودڪار& est : costs) {
        إذا (est.ٻيو.ٻيو) {
            جوڙ_لاڳت_Estimate += est.ٻيو.پهريون;
            جوڙ_لاڳت_Profiled += est.ٻيو.ٻيو;
        }
    }

    إذا (جوڙ_لاڳت_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت مزدوج est_منزل_Profile
            = static_cast<مزدوج>(جوڙ_لاڳت_Profiled) / static_cast<مزدوج>(جوڙ_لاڳت_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_منزل_Profile
                                                    << ", sumCostProfiled=" << جوڙ_لاڳت_Profiled
                                                    << " sumCostEstimate=" << جوڙ_لاڳت_Estimate);
        لكل (خودڪار& est : costs) {
            uint64_t& لاڳت_Estimate = est.ٻيو.پهريون;
            لاڳت_Estimate = scale_لاڳت(لاڳت_Estimate, est_منزل_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_لاڳت = 0;
    لكل (خودڪار& est : costs) {
        ثابت uint64_t& لاڳت_Estimate = est.ٻيو.پهريون;
        ثابت uint64_t& لاڳت_Profiled = est.ٻيو.ٻيو;
        إذا (max_لاڳت < لاڳت_Estimate) max_لاڳت = لاڳت_Estimate;
        إذا (max_لاڳت < لاڳت_Profiled) max_لاڳت = لاڳت_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.ٻيو.پهريون << " cp=" << est.ٻيو.ٻيو);
    }
    constexpr uint64_t scale_هيٺ_ڏانهن_منزل = 10000000;  // Extra room for future algorithms to add costs
    إذا (max_لاڳت > scale_هيٺ_ڏانهن_منزل) {
        ثابت مزدوج scaleup = static_cast<مزدوج>(scale_هيٺ_ڏانهن_منزل) / static_cast<مزدوج>(max_لاڳت);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_لاڳت);
        لكل (خودڪار& est : costs) {
            est.ٻيو.پهريون = scale_لاڳت(est.ٻيو.پهريون, scaleup);
            est.ٻيو.ٻيو = scale_لاڳت(est.ٻيو.ٻيو, scaleup);
        }
    }
}

فراغ remove_خالي_جايون_MTasks(Ast_Exec_گراف* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    لكل (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        إذا (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_پٺ()->delete_وڻ(), callp);
    }
    V3_گراف* ثابت execMTaskGraphp = execGraphp->depGraphp();
    لكل (V3_گراف_راس* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        إذا (funcp->stmtsp()) استمر;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_ڪنارا(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_پٺ()->delete_وڻ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);
}

فراغ fillinCosts(V3_گراف* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    لكل (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t لاڳت_Estimate = V3_Instr_ڳڻپ::ڳڻپ(mtp->funcp(), خطأ);
        ثابت uint64_t لاڳت_Profiled
            = V3Control::get_Profile_معلومات(v3Global.opt.اڳياڙي(), mtp->hashName());
        إذا (لاڳت_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << لاڳت_Profiled);
        }
        costs[mtp->id()] = std::make_pair(لاڳت_Estimate, لاڳت_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    صحيح ڪل_Estimates = 0;
    صحيح missingProfiles = 0;
    لكل (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t لاڳت_Estimate = costs[mtp->id()].پهريون;
        ثابت uint64_t لاڳت_Profiled = costs[mtp->id()].ٻيو;
        UINFO(9, "ce = " << لاڳت_Estimate << " cp=" << لاڳت_Profiled);
        UASSERT(لاڳت_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(لاڳت_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t لاڳت_Profiled32 = static_cast<uint32_t>(لاڳت_Profiled);
        uint32_t لاڳت_منزل_Use = لاڳت_Profiled32;
        إذا (!لاڳت_Profiled32) {
            لاڳت_منزل_Use = لاڳت_Estimate;
            إذا (لاڳت_Estimate != 0) ++missingProfiles;
        }
        إذا (لاڳت_Estimate != 0) ++ڪل_Estimates;
        mtp->لاڳت(لاڳت_منزل_Use);
        mtp->ترجيح_2(لاڳت_منزل_Use);
    }

    إذا (missingProfiles) {
        إذا (File_سٽ* ثابت fl = V3Control::get_Profile_معلومات_File_سٽ()) {
            إذا (V3Control::شامل_آهي_MTask_Profile_معلومات()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << ڪل_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

فراغ finalizeCosts(V3_گراف* execMTaskGraphp) {
    گراف_وهڪرو_Unordered ser(execMTaskGraphp, گراف_Way::ابتو_2);
    طالما (ثابت V3_گراف_راس* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3_گراف_راس*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        لكل (V3_گراف_ڪنارو& ڪنارو : mtp->ٻاهريون_ڪنارا()) {
            ثابت ExecMTask* ثابت followp = ڪنارو.چوٽي()->as<ExecMTask>();
            إذا ((followp->ترجيح_2() + mtp->لاڳت()) > mtp->ترجيح_2()) {
                mtp->ترجيح_2(followp->ترجيح_2() + mtp->لاڳت());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_متعدي_ڪنارا();

    // Record summary stats for final m_tasks graph.
    ثابت خودڪار report = execMTaskGraphp->parallelismReport([](ثابت V3_گراف_راس* vtxp) {  //
        أرجع vtxp->as<ثابت ExecMTask>()->لاڳت();
    });
    V3Stats::جوڙيو_Stat("MTask graph, final, critical path cost", report.critical_رستو_لاڳت());
    V3Stats::جوڙيو_Stat("MTask graph, final, total graph cost", report.ڪل_گراف_لاڳت());
    V3Stats::جوڙيو_Stat("MTask graph, final, mtask count", report.راس_ڳڻپ());
    V3Stats::جوڙيو_Stat("MTask graph, final, edge count", report.ڪنارو_ڳڻپ());
    V3Stats::جوڙيو_Stat("MTask graph, final, parallelism factor", report.parallelism_ضرب_ڪندڙ());
    إذا (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_رستو_لاڳت());
        UINFO(0, "    Total graph cost = " << report.ڪل_گراف_لاڳت());
        UINFO(0, "    MTask vertex count = " << report.راس_ڳڻپ());
        UINFO(0, "    Edge count = " << report.ڪنارو_ڳڻپ());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ضرب_ڪندڙ());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_ڳنڍ_Module* ثابت modp = v3Global.rootp()->چوٽي_Modulep();
    File_سٽ* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->ڳوليو_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(سچ);
    varp->valuep(جديد AstConst{fl, nDependencies});
    modp->جوڙيو_Stmtsp(varp);
    أرجع varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& طريقو) {
    File_سٽ* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت چوٽي_Scopep = v3Global.rootp()->چوٽي_Scopep()->scopep();
    refp->self_اشارو(VSelf_اشارو_متن{VSelf_اشارو_متن::VlSyms{}, چوٽي_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = جديد AstCStmt{fl};
    stmtp->جوڙيو(refp);
    stmtp->جوڙيو("." + طريقو + ";");
    أرجع stmtp;
}

فراغ جوڙيو_MTask_منزل_Function(ثابت Thread_وقت_جدول& وقت_جدول, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->چوٽي_Scopep()->scopep();
    Ast_ڳنڍ_Module* ثابت modp = v3Global.rootp()->چوٽي_Modulep();
    File_سٽ* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت خودڪار جوڙيو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        funcp->جوڙيو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (وقت_جدول.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        إذا (v3Global.opt.profExec()) {
            جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->جوڙيو_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        إذا (v3Global.opt.profExec()) {
            جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    إذا (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        جوڙيو_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = جديد AstCCall{fl, mtaskp->funcp()};
    callp->self_اشارو(VSelf_اشارو_متن{VSelf_اشارو_متن::VlSyms{}, scopep->nameDotless()});
    callp->dtype_سيٽ_Void();
    funcp->جوڙيو_Stmtsp(callp->makeStmt());

    إذا (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        جوڙيو_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::منزل_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : mtaskp->ٻاهريون_ڪنارا()) {
        ثابت ExecMTask* ثابت nextp = ڪنارو.چوٽي()->as<ExecMTask>();
        إذا (وقت_جدول.threadId(nextp) != threadId && وقت_جدول.شامل_آهي(nextp)) {
            funcp->جوڙيو_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::ويڪٽر<AstCFunc*> createThreadFunctions(ثابت Thread_وقت_جدول& وقت_جدول,
                                                   ثابت string& ٽيگ) {
    Ast_ڳنڍ_Module* ثابت modp = v3Global.rootp()->چوٽي_Modulep();
    File_سٽ* ثابت fl = modp->fileline();

    std::ويڪٽر<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    لكل (ثابت std::ويڪٽر<ثابت ExecMTask*>& thread : وقت_جدول.m_ڌاڳا) {
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            إذا (ثابت uint32_t nDependencies = وقت_جدول.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_منزل_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    لكل (ثابت std::ويڪٽر<ثابت ExecMTask*>& thread : وقت_جدول.m_ڌاڳا) {
        إذا (thread.خالي_جايون()) استمر;
        ثابت uint32_t threadId = وقت_جدول.threadId(thread.front());
        ثابت string name{"__Vthread__" + ٽيگ + "__s" + cvt_منزل_Str(وقت_جدول.id()) + "__t"
                          + cvt_منزل_Str(threadId)};
        AstCFunc* ثابت funcp = جديد AstCFunc{fl, name, خالي_اشارو, "void"};
        modp->جوڙيو_Stmtsp(funcp);
        funcps.ڌڪو_پٺ(funcp);
        funcp->isStatic(سچ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(سچ);
        funcp->داخلا_نقطو(سچ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->جوڙيو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_سونپيو(modp)});
        funcp->جوڙيو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_سونپيو()});

        // Invoke each mtask scheduled to this thread from the thread function
        لكل (ثابت ExecMTask* ثابت mtaskp : thread) {
            جوڙيو_MTask_منزل_Function(وقت_جدول, threadId, funcp, mtaskp, stateVars);
        }
    }

    أرجع funcps;
}

فراغ جوڙيو_Thread_شروعات_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_سٽ* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& ٽيگ = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت خودڪار جوڙيو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جوڙيو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    إذا (v3Global.opt.profExec()) {
        جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    جوڙيو_CStmt("vlSymsp->__Vm_even_cycle__" + ٽيگ + " = !vlSymsp->__Vm_even_cycle__" + ٽيگ + ";");

    إذا (!v3Global.opt.hier_بلاڪون().خالي_جايون()) جوڙيو_CStmt("std::vector<size_t> indexes;");
}

فراغ جوڙيو_Thread_پڇاڙي_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت خودڪار جوڙيو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        File_سٽ* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->جوڙيو_Stmtsp(جديد AstCStmt{flp, stmt});
    };

    جوڙيو_CStmt("Verilated::mtaskId(0);");
    إذا (v3Global.opt.profExec()) { جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
فراغ جوڙيو_Thread_شروعات_منزل_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp,
                               ثابت std::ويڪٽر<AstCFunc*>& funcps, uint32_t وقت_جدول_Id) {
    // FileLine used for constructing nodes below
    File_سٽ* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& ٽيگ = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvt_منزل_Str(وقت_جدول_Id) + "__" + ٽيگ, funcps.سائيز());
    لكل (AstCFunc* ثابت funcp : funcps) {
        funcp->جوڙيو_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت خودڪار جوڙيو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
        execGraphp->runfuncp()->جوڙيو_Stmtsp(جديد AstCStmt{fl, stmt});
    };

    ثابت uint32_t آخري = funcps.سائيز() - 1;
    إذا (!v3Global.opt.hier_بلاڪون().خالي_جايون() && آخري > 0) {
        جوڙيو_CStmt("for (size_t i = 0; i < " + std::منزل_string(آخري) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    لكل (AstCFunc* ثابت funcp : funcps) {
        إذا (i != آخري) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            execGraphp->runfuncp()->جوڙيو_Stmtsp(cstmtp);
            cstmtp->جوڙيو("vlSymsp->__Vm_threadPoolp->workerp(");
            إذا (v3Global.opt.hier_ٻار() || !v3Global.opt.hier_بلاڪون().خالي_جايون()) {
                cstmtp->جوڙيو("indexes[" + std::منزل_string(i) + "]");
            } وإلا {
                cstmtp->جوڙيو(std::منزل_string(i));
            }
            cstmtp->جوڙيو(")->addTask(");
            cstmtp->جوڙيو(جديد Ast_Addr_جو_CFunc{fl, funcp});
            cstmtp->جوڙيو(", vlSelf, vlSymsp->__Vm_even_cycle__" + ٽيگ + ");");
        } وإلا {
            // The last will run on the main thread.
            AstCCall* ثابت callp = جديد AstCCall{fl, funcp};
            callp->dtype_سيٽ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + ٽيگ);
            execGraphp->runfuncp()->جوڙيو_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Thread schedule total tasks", i);

    إذا (v3Global.opt.profExec()) {
        جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->جوڙيو_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + ٽيگ + ")"));
    إذا (v3Global.opt.profExec()) {
        جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    إذا (!v3Global.opt.hier_بلاڪون().خالي_جايون() && آخري > 0) {
        جوڙيو_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

فراغ processMTaskBodies(Ast_Exec_گراف* ثابت execGraphp) {
    لكل (V3_گراف_راس* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ڳنڍ* stmtsp = funcp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();

        // Helper function to make the code a bit more legible
        ثابت خودڪار جوڙيو_CStmt = [=](ثابت string& stmt) -> فراغ {  //
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        إذا (v3Global.opt.profExec()) {
            std::string دليلون = std::منزل_string(mtaskp->id());
            دليلون += ", " + std::منزل_string(mtaskp->predict_شروعات());
            دليلون += ", \"";
            إذا (v3Global.opt.hier_ٻار()) دليلون += v3Global.opt.چوٽي_Module();
            دليلون += "\"";
            جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + دليلون + ");");
        }
        // Set mtask ID in the run-time system
        جوڙيو_CStmt("Verilated::mtaskId(" + std::منزل_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->جوڙيو_Stmtsp(stmtsp);
        // Flush message queue
        جوڙيو_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        إذا (v3Global.opt.profExec()) {
            ثابت std::string& دليلون = std::منزل_string(mtaskp->لاڳت());
            جوڙيو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + دليلون + ");");
        }
    }
}

فراغ implement_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp, ثابت Thread_وقت_جدول& وقت_جدول) {
    // Nothing to be done if there are no MTasks in the graph at all.
    إذا (execGraphp->depGraphp()->خالي_جايون()) أرجع;

    // Create a function to be run by each thread.
    ثابت std::ويڪٽر<AstCFunc*>& funcps = createThreadFunctions(وقت_جدول, execGraphp->name());
    UASSERT(!funcps.خالي_جايون(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    جوڙيو_Thread_شروعات_منزل_Exec_گراف(execGraphp, funcps, وقت_جدول.id());
}

فراغ create_لاڳيتو_Function(Ast_Exec_گراف* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_سٽ* ثابت flp = execGraphp->fileline();
    Ast_ڳنڍ_Module* ثابت modp = v3Global.rootp()->چوٽي_Modulep();
    AstCFunc* ثابت funcp = جديد AstCFunc{flp, "runExecGraph_" + execGraphp->name(), خالي_اشارو};
    funcp->isLoose(سچ);
    funcp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(سچ);
    modp->جوڙيو_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

فراغ جوڙيو_Trigger_Masks(Ast_Exec_گراف* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_سٽ* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> لفظ_Masks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            لكل (ثابت uint32_t اشاريو : trigp->indices()) {
                لفظ_Masks[VL_BITWORD_Q(اشاريو)] |= VL_ٻٽو_انگ_Q(اشاريو);
            }
        });
        AstConsArray* ثابت consp = جديد AstConsArray{flp, triggersp->dtypep(),
                                                     جديد AstConst{flp, AstConst::Unsized64{}, 0}};
        لكل (ثابت خودڪار& لفظ_نقاب : لفظ_Masks) {
            consp->valuep(لفظ_نقاب.پهريون,
                          جديد AstConst{flp, AstConst::Unsized64{}, لفظ_نقاب.ٻيو});
        }
        execGraphp->جوڙيو_Maskps(V3ConstPool::ڳوليو(consp));
        VL_DO_DANGLING(consp->delete_وڻ(), consp);
    }
}

فراغ جوڙيو_Trigger_Costs(Ast_Exec_گراف* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_سٽ* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::ويڪٽر<uint32_t> costs(nTriggers, 0);
    لكل (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            إذا (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t لاڳت = V3_Instr_ڳڻپ::ڳڻپ(ifp, خطأ);
                لكل (ثابت uint32_t اشاريو : trigp->indices()) costs.at(اشاريو) += لاڳت;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = جديد AstUnpackArrayDType{flp, netlistp->ڳوليو_ٻٽو_انگ_DType(32, 32, VSigning::UNSIGNED),
                                  جديد AstRange{flp, static_cast<صحيح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->جوڙيو_Typesp(dtypep);
    AstConsArray* ثابت consp = جديد AstConsArray{flp, dtypep, جديد AstConst{flp, 0}};
    لكل (uint32_t اشاريو = 0; اشاريو < nTriggers; ++اشاريو) {
        إذا (costs[اشاريو]) consp->valuep(اشاريو, جديد AstConst{flp, costs[اشاريو]});
    }
    execGraphp->triggerCostsp(V3ConstPool::ڳوليو(consp));
    VL_DO_DANGLING(consp->delete_وڻ(), consp);
}

// Called by Verilator top stage
فراغ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::ويڪٽر<Ast_Exec_گراف*> execGraphps;
    netlistp->چوٽي_Modulep()->foreach([&](Ast_Exec_گراف* egp) { execGraphps.emplace_پٺ(egp); });

    // Process each
    لكل (Ast_Exec_گراف* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_خالي_جايون_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        إذا (execGraphp->depGraphp()->خالي_جايون()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_پٺ()->delete_وڻ(), execGraphp);
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

        create_لاڳيتو_Function(execGraphp);
        جوڙيو_Thread_شروعات_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::ويڪٽر<Thread_وقت_جدول> packed = Pack_ڌاڳا::apply(*execGraphp->depGraphp());
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, Thread schedule count",
                            static_cast<مزدوج>(packed.سائيز()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        لكل (ثابت Thread_وقت_جدول& وقت_جدول : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_گراف(execGraphp, وقت_جدول);
        }

        جوڙيو_Thread_پڇاڙي_Wrapper(execGraphp);

        جوڙيو_Trigger_Masks(execGraphp);
        جوڙيو_Trigger_Costs(execGraphp);
    }
}

فراغ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پهريون, 1000);
        UASSERT_SELFTEST(costs[1].ٻيو, 1000);
        UASSERT_SELFTEST(costs[2].پهريون, 2000);
        UASSERT_SELFTEST(costs[2].ٻيو, 0);
        UASSERT_SELFTEST(costs[3].پهريون, 3000);
        UASSERT_SELFTEST(costs[3].ٻيو, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پهريون, 2500000);
        UASSERT_SELFTEST(costs[1].ٻيو, 5000000);
        UASSERT_SELFTEST(costs[2].پهريون, 5000000);
        UASSERT_SELFTEST(costs[2].ٻيو, 10000000);
        UASSERT_SELFTEST(costs[3].پهريون, 7500000);
        UASSERT_SELFTEST(costs[3].ٻيو, 1);
    }

    Pack_ڌاڳا::selfTest();
}

}  // namespace V3ExecGraph
