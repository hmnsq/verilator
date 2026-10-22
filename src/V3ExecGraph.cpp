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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3ExecGraph.h"

#شامل "V3ConstPool.h"
#شامل "V3Control.h"
#شامل "V3EmitCBase.h"
#شامل "V3File.h"
#شامل "V3GraphStream.h"
#شامل "V3Hasher.h"
#شامل "V3InstrCount.h"
#شامل "V3Os.h"
#شامل "V3Stats.h"

#شامل <map>
#شامل <memory>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp,
                                 uint32_t id) {
    ثابت std::string newName = execGraphp->name() + "_mtask" + std::مقصد_string(id);
    AstCFunc* ثابت newp = نیا AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(سچ);
    newp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(سچ);
    newp->جوڑو_Stmtsp(stmtsp);
    اگر (scopep) scopep->جوڑو_Blocksp(newp);
    واپس newp;
}

ExecMTask::ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep,
                     Ast_گرہ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_گراف_راس{execGraphp->depGraphp()},
      m_id{s_اگلا_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).مقصد_String()} {}

خالی ExecMTask::dump(std::ostream& str) ثابت {
    str << name() << "." << cvt_مقصد_Hex(یہ);
    اگر (ترجیح_2() || لاگت()) str << " [pr=" << ترجیح_2() << " c=" << cvt_مقصد_Str(لاگت()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_اگلا_Id{0};

نامخلا V3_Exec_گراف {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
جماعت Thread_جدول_وقت final {
    دوست جماعت Pack_دھاگے;

    uint32_t m_id;  // Unique ID of a schedule
    ساکن uint32_t s_اگلا_Id;  // Next ID number to use
    std::unordered_سیٹ<ثابت ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_انتہا_وقت = 0;  // Latest task end time in this schedule

عوامی:
    // CONSTANTS
    ساکن constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ساخت MTaskState final {
        uint32_t completion_وقت = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ثابت ExecMTask* nextp = خالی_اشاریہ;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::سمتیہ<std::سمتیہ<ثابت ExecMTask*>> m_دھاگے;

    // Global state for each mtask.
    ساکن std::unordered_map<ثابت ExecMTask*, MTaskState> s_mtaskState;

    صریح Thread_جدول_وقت(uint32_t n_دھاگے)
        : m_id{s_اگلا_Id++}
        , m_دھاگے{n_دھاگے} {}
    Thread_جدول_وقت(Thread_جدول_وقت&&) = طےشدہ;
    Thread_جدول_وقت& عامل=(Thread_جدول_وقت&&) = طےشدہ;

نجی:
    VL_UNCOPYABLE(Thread_جدول_وقت);

    ساکن constexpr دوہرا s_thread_Box_چوڑائی = 2.5;
    ساکن constexpr دوہرا s_thread_Box_اونچائی = 1.5;
    ساکن constexpr دوہرا s_horizontalGap = s_thread_Box_چوڑائی / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ساکن خالی dumpDotFilePrefixedAlways(ثابت std::سمتیہ<Thread_جدول_وقت>& schedules,
                                          ثابت string& nameComment, uint32_t n_دھاگے) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_دھاگے);
    }
    ساکن خالی dumpDotFile(ثابت std::سمتیہ<Thread_جدول_وقت>& schedules, ثابت string& filename,
                            uint32_t n_دھاگے) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        اگر (logp->fail()) v3fatal("Can't write file: " << filename);

        //سرخی
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        برائے (uint32_t i = 0; i < n_دھاگے; ++i) {
            ثابت string name = "t" + std::مقصد_string(i);
            ثابت string label = "Thread " + std::مقصد_string(i);
            constexpr دوہرا posX = -s_horizontalGap;
            ثابت دوہرا posY = -static_cast<دوہرا>(i) * s_thread_Box_اونچائی;
            dump_Dot_File_Emit_بلاک(logp, name, label, s_thread_Box_چوڑائی, s_thread_Box_اونچائی, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_لاگت = 0;
        برائے (ثابت خودکار& state : Thread_جدول_وقت::s_mtaskState) {
            ثابت ExecMTask* ثابت mtaskp = state.پہلا;
            max_لاگت = std::max(max_لاگت, mtaskp->لاگت());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_فی_Longest_Mtask = 4;
        ثابت uint32_t segment_لاگت = max_لاگت / segments_فی_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::سمتیہ<دوہرا> offsets(n_دھاگے, 0.0);
        برائے (ثابت Thread_جدول_وقت& جدول_وقت : schedules) {
            اگر (جدول_وقت.mtasks.خالی_خانے()) جاری;
            استعمال عمود = std::سمتیہ<ثابت ExecMTask*>;
            std::سمتیہ<عمود> columns = {{}};

            // Order tasks based on their start time
            ساخت Cmp final {
                منطقی عامل()(ثابت ExecMTask* ثابت a, ثابت ExecMTask* ثابت b) ثابت {
                    اگر (آغاز_وقت(a) == آغاز_وقت(b)) واپس threadId(a) < threadId(b);
                    واپس آغاز_وقت(a) < آغاز_وقت(b);
                }
            };
            ثابت std::multiset<ثابت ExecMTask*, Cmp> tasks(جدول_وقت.mtasks.begin(),
                                                             جدول_وقت.mtasks.انتہا());

            برائے (ثابت ExecMTask* ثابت mtaskp : tasks) {
                عمود& عمود_2 = columns.عقب();
                UASSERT(عمود_2.سائز() <= n_دھاگے, "Invalid partitioning");

                منطقی intersects = سچ;
                برائے (ثابت ExecMTask* ثابت earlierMtask : عمود_2) {
                    اگر (انتہا_وقت(mtaskp) <= آغاز_وقت(earlierMtask)
                        || آغاز_وقت(mtaskp) >= انتہا_وقت(earlierMtask)) {
                        intersects = جھوٹ;
                        توڑو;
                    }
                }
                اگر (intersects) {
                    عمود_2.emplace_عقب(mtaskp);
                } ورنہ {
                    columns.emplace_عقب(عمود{mtaskp});
                }
            }

            UASSERT(!columns.front().خالی_خانے(), "Should be populated by mtasks");

            برائے (ثابت عمود& عمود_2 : columns) {
                دوہرا آخری_عمود_انحراف = 0;
                برائے (ثابت ExecMTask* ثابت mtaskp : عمود_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, جدول_وقت, segment_لاگت, offsets);
                    آخری_عمود_انحراف = std::max(آخری_عمود_انحراف, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.انتہا(), آخری_عمود_انحراف);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_دھاگے);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            برائے (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : جدول_وقت.m_دھاگے) {
                اگر (thread.خالی_خانے()) توڑو;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.عقب()->name() << " -> fork_"
                      << static_cast<صحیح>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                برائے (سائز_t i = 1; i < thread.سائز(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                برائے (ثابت ExecMTask* ثابت mtaskp : thread) {
                    برائے (ثابت V3_گراف_کنارہ& کنارہ : mtaskp->برآمد_کنارے()) {
                        ثابت ExecMTask* ثابت چوٹی_MTaskp = کنارہ.چوٹی()->cast<ثابت ExecMTask>();
                        اگر (چوٹی_MTaskp && جدول_وقت.شامل_ہے(چوٹی_MTaskp)
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
    ساکن خالی dump_Dot_File_Emit_بلاک(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت string& name, ثابت string& label, دوہرا چوڑائی,
                                     دوہرا اونچائی, دوہرا xPos, دوہرا yPos,
                                     ثابت string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << چوڑائی
              << " height=" << اونچائی << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ساکن خالی dumpDotFileEmitMTask(ثابت std::unique_ptr<std::ofstream>& logp,
                                     ثابت ExecMTask* ثابت mtaskp, ثابت Thread_جدول_وقت& جدول_وقت,
                                     uint32_t segment_لاگت, std::سمتیہ<دوہرا>& offsets) {
        برائے (صحیح i = 0; i < mtaskp->دھاگے(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ثابت string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::مقصد_string(i);
            ثابت string label = mtaskp->name() + " (" + std::مقصد_string(آغاز_وقت(mtaskp)) + ':'
                                 + std::مقصد_string(انتہا_وقت(mtaskp)) + ')'
                                 + "\\ncost=" + std::مقصد_string(mtaskp->لاگت())
                                 + "\\npriority=" + std::مقصد_string(mtaskp->ترجیح_2());
            ثابت دوہرا چوڑائی
                = std::max(s_thread_Box_چوڑائی,
                           s_thread_Box_چوڑائی * static_cast<دوہرا>(mtaskp->لاگت()) / segment_لاگت);
            ثابت uint32_t mtaskThreadId = threadId(mtaskp) + i * جدول_وقت.m_دھاگے.سائز();
            ثابت دوہرا xPos = چوڑائی / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += چوڑائی + s_horizontalGap;
            ثابت دوہرا yPos = -s_thread_Box_اونچائی * static_cast<دوہرا>(mtaskThreadId);
            ثابت string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_بلاک(logp, name, label, چوڑائی, s_thread_Box_اونچائی, xPos, yPos,
                                 fillColor);
        }
    }

    ساکن خالی dumpDotFileEmitFork(ثابت std::unique_ptr<std::ofstream>& logp, دوہرا انحراف,
                                    uint32_t n_دھاگے) {
        ثابت string& name = "fork_" + std::مقصد_string(static_cast<صحیح>(انحراف));
        constexpr دوہرا چوڑائی = s_thread_Box_چوڑائی / 8;
        ثابت دوہرا اونچائی = s_thread_Box_اونچائی * n_دھاگے;
        ثابت دوہرا xPos = انحراف - s_horizontalGap / 2;
        ثابت دوہرا yPos
            = -static_cast<دوہرا>(n_دھاگے) / 2 * s_thread_Box_اونچائی + s_thread_Box_اونچائی / 2;
        dump_Dot_File_Emit_بلاک(logp, name, "", چوڑائی, اونچائی, xPos, yPos, "black");
    }

عوامی:
    ساکن uint32_t threadId(ثابت ExecMTask* mtaskp) {
        ثابت خودکار& it = s_mtaskState.ڈھونڈو(mtaskp);
        واپس it != s_mtaskState.انتہا() ? it->دوسرا.threadId : UNASSIGNED;
    }
    ساکن uint32_t آغاز_وقت(ثابت ExecMTask* mtaskp) {
        واپس s_mtaskState.at(mtaskp).completion_وقت - mtaskp->لاگت();
    }
    ساکن uint32_t انتہا_وقت(ثابت ExecMTask* mtaskp) {
        واپس s_mtaskState.at(mtaskp).completion_وقت;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ثابت ExecMTask* mtaskp) ثابت {
        ثابت uint32_t thisThreadId = threadId(mtaskp);
        uint32_t نتیجہ = 0;
        برائے (ثابت V3_گراف_کنارہ& کنارہ : mtaskp->اندر_کنارے()) {
            ثابت ExecMTask* ثابت prevp = کنارہ.fromp()->as<ExecMTask>();
            اگر (threadId(prevp) != thisThreadId && شامل_ہے(prevp)) ++نتیجہ;
        }
        واپس نتیجہ;
    }

    uint32_t id() ثابت { واپس m_id; }
    uint32_t جدول_وقت_On(ثابت ExecMTask* mtaskp, uint32_t بہترین_Thread_Id) {
        mtasks.emplace(mtaskp);
        ثابت uint32_t بہترین_انتہا_وقت = mtaskp->predict_آغاز() + mtaskp->لاگت();
        m_انتہا_وقت = std::max(m_انتہا_وقت, بہترین_انتہا_وقت);
        s_mtaskState[mtaskp].completion_وقت = بہترین_انتہا_وقت;
        s_mtaskState[mtaskp].threadId = بہترین_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::سمتیہ<ثابت ExecMTask*>& بہترین_Thread = m_دھاگے[بہترین_Thread_Id];
        اگر (!بہترین_Thread.خالی_خانے()) s_mtaskState[بہترین_Thread.عقب()].nextp = mtaskp;

        // Add the MTask to the schedule
        بہترین_Thread.ڈالو_عقب(mtaskp);
        واپس بہترین_انتہا_وقت;
    }
    منطقی شامل_ہے(ثابت ExecMTask* mtaskp) ثابت { واپس mtasks.گنتی(mtaskp); }
    uint32_t انتہا_وقت() ثابت { واپس m_انتہا_وقت; }
};

uint32_t Thread_جدول_وقت::s_اگلا_Id = 0;
std::unordered_map<ثابت ExecMTask*, Thread_جدول_وقت::MTaskState> Thread_جدول_وقت::s_mtaskState{};
constexpr دوہرا V3_Exec_گراف::Thread_جدول_وقت::s_thread_Box_چوڑائی;

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
جماعت Pack_دھاگے final {
    // TYPES
    ساخت MTaskCmp final {
        منطقی عامل()(ثابت ExecMTask* ap, ثابت ExecMTask* bp) ثابت {
            واپس ap->id() < bp->id();
        }
    };

    // MEMBERS
    ثابت uint32_t m_n_دھاگے;  // Number of threads
    ثابت uint32_t m_n_Hier_دھاگے;  // Number of threads used for hierarchical tasks
    ثابت uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ثابت uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    صریح Pack_دھاگے(uint32_t n_دھاگے = v3Global.opt.دھاگے(),
                         uint32_t n_Hier_دھاگے = v3Global.opt.hier_دھاگے(),
                         غیرعلامتی sandbagNumerator = 30, غیرعلامتی sandbagDenom = 100)
        : m_n_دھاگے{n_دھاگے}
        , m_n_Hier_دھاگے{n_Hier_دھاگے}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_دھاگے() = طےشدہ;
    VL_UNCOPYABLE(Pack_دھاگے);

    // METHODS
    uint32_t completion_وقت(ثابت Thread_جدول_وقت& جدول_وقت, ثابت ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        اگر (!جدول_وقت.شامل_ہے(mtaskp)) واپس 0;
        ثابت Thread_جدول_وقت::MTaskState& state = جدول_وقت.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_جدول_وقت::UNASSIGNED, "Mtask should have assigned thread");
        اگر (threadId == state.threadId) {
            // No overhead on same thread
            واپس state.completion_وقت;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_انتہا_وقت
            = state.completion_وقت + (m_sandbagNumerator * mtaskp->لاگت()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        اگر (state.nextp) {
            ثابت uint32_t successor_انتہا_وقت
                = completion_وقت(جدول_وقت, state.nextp, state.threadId);
            اگر ((sandbagged_انتہا_وقت >= successor_انتہا_وقت) && (successor_انتہا_وقت > 1)) {
                sandbagged_انتہا_وقت = successor_انتہا_وقت - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_انتہا_وقت);
        واپس sandbagged_انتہا_وقت;
    }

    ساکن منطقی isReady(Thread_جدول_وقت& جدول_وقت, ثابت ExecMTask* mtaskp) {
        برائے (ثابت V3_گراف_کنارہ& کنارہ_اندر : mtaskp->اندر_کنارے()) {
            ثابت ExecMTask* ثابت prevp = کنارہ_اندر.fromp()->as<ثابت ExecMTask>();
            اگر (جدول_وقت.threadId(prevp) == Thread_جدول_وقت::UNASSIGNED) {
                // This predecessor is not assigned yet
                واپس جھوٹ;
            }
        }
        واپس سچ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::سمتیہ<Thread_جدول_وقت> pack(V3_گراف& mtask_گراف) {
        std::سمتیہ<Thread_جدول_وقت> نتیجہ;
        نتیجہ.emplace_عقب(Thread_جدول_وقت{m_n_دھاگے});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        شمار جماعت Scheduling_منوال : uint8_t {
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
        صحیح maxThreadWorkers = 1;

        // Build initial ready list
        برائے (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            ExecMTask* ثابت mtaskp = vtx.as<ExecMTask>();
            اگر (isReady(نتیجہ.عقب(), mtaskp)) readyMTasks.داخل_کرو(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->دھاگے());
        }

        جب (!readyMTasks.خالی_خانے()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t بہترین_وقت = 0xffffffff;
            uint32_t بہترین_Thread_Id = 0;
            ExecMTask* بہترین_Mtaskp = خالی_اشاریہ;  // Todo: const ExecMTask*
            Thread_جدول_وقت& جدول_وقت = نتیجہ.عقب();
            برائے (uint32_t threadId = 0; threadId < جدول_وقت.m_دھاگے.سائز(); ++threadId) {
                برائے (ExecMTask* ثابت mtaskp : readyMTasks) {
                    اگر (منوال != Scheduling_منوال::WIDE_TASK_SCHEDULING && mtaskp->دھاگے() > 1) {
                        منوال = Scheduling_منوال::WIDE_TASK_DISCOVERED;
                        جاری;
                    }
                    اگر (منوال == Scheduling_منوال::WIDE_TASK_SCHEDULING && mtaskp->دھاگے() <= 1)
                        جاری;

                    uint32_t وقت_Begin = busyUntil[threadId];
                    اگر (وقت_Begin > بہترین_وقت) {
                        UINFO(6, "th " << threadId << " busy until " << وقت_Begin
                                       << ", later than bestTime " << بہترین_وقت
                                       << ", skipping thread.");
                        توڑو;
                    }
                    برائے (ثابت V3_گراف_کنارہ& کنارہ : mtaskp->اندر_کنارے()) {
                        ثابت ExecMTask* ثابت priorp = کنارہ.fromp()->as<ExecMTask>();
                        ثابت uint32_t پیشگی_انتہا_وقت = completion_وقت(جدول_وقت, priorp, threadId);
                        اگر (پیشگی_انتہا_وقت > وقت_Begin) وقت_Begin = پیشگی_انتہا_وقت;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << وقت_Begin
                                     << " on thread " << threadId);
                    اگر ((وقت_Begin < بہترین_وقت)
                        || ((وقت_Begin == بہترین_وقت)
                            && بہترین_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ترجیح_2() > بہترین_Mtaskp->ترجیح_2()))) {
                        بہترین_وقت = وقت_Begin;
                        بہترین_Thread_Id = threadId;
                        بہترین_Mtaskp = mtaskp;
                    }
                }
            }

            ثابت uint32_t انتہا_وقت = جدول_وقت.انتہا_وقت();

            اگر (!بہترین_Mtaskp && منوال == Scheduling_منوال::WIDE_TASK_DISCOVERED) {
                منوال = Scheduling_منوال::WIDE_TASK_SCHEDULING;
                ثابت uint32_t سائز = m_n_Hier_دھاگے / maxThreadWorkers;
                UASSERT(سائز, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                اگر (جدول_وقت.s_mtaskState.خالی_خانے()) نتیجہ.clear();
                نتیجہ.emplace_عقب(Thread_جدول_وقت{سائز});
                std::fill(busyUntil.begin(), busyUntil.انتہا(), انتہا_وقت);
                جاری;
            }

            اگر (!بہترین_Mtaskp && منوال == Scheduling_منوال::WIDE_TASK_SCHEDULING) {
                منوال = Scheduling_منوال::SCHEDULING;
                UASSERT(!جدول_وقت.s_mtaskState.خالی_خانے(), "Mtask should be added");
                نتیجہ.emplace_عقب(Thread_جدول_وقت{m_n_دھاگے});
                std::fill(busyUntil.begin(), busyUntil.انتہا(), انتہا_وقت);
                جاری;
            }

            UASSERT(بہترین_Mtaskp, "Should have found some task");

            بہترین_Mtaskp->predict_آغاز(بہترین_وقت);
            ثابت uint32_t بہترین_انتہا_وقت = جدول_وقت.جدول_وقت_On(بہترین_Mtaskp, بہترین_Thread_Id);
            busyUntil[بہترین_Thread_Id] = بہترین_انتہا_وقت;

            // Update the ready list
            ثابت سائز_t erased = readyMTasks.erase(بہترین_Mtaskp);
            UASSERT_OBJ(erased > 0, بہترین_Mtaskp, "Should have erased something?");
            برائے (ثابت V3_گراف_کنارہ& کنارہ_برآمد : بہترین_Mtaskp->برآمد_کنارے()) {
                ExecMTask* ثابت nextp = کنارہ_برآمد.چوٹی()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(جدول_وقت.threadId(nextp) == Thread_جدول_وقت::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.ڈھونڈو(nextp) == readyMTasks.انتہا(), nextp,
                            "Tasks after one being assigned should not be ready");
                اگر (isReady(جدول_وقت, nextp)) {
                    readyMTasks.داخل_کرو(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        اگر (dump_گراف_Level() >= 4)
            Thread_جدول_وقت::dumpDotFilePrefixedAlways(نتیجہ, "schedule", m_n_دھاگے);

        واپس نتیجہ;
    }

عوامی:
    // SELF TEST
    ساکن خالی selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        AstVar* ثابت varp = نیا AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->ڈھونڈو_UInt64_DType()};
        AstScope* ثابت scopep = نیا AstScope{flp, خالی_اشاریہ, "test", خالی_اشاریہ, خالی_اشاریہ};
        AstVarScope* ثابت trigVscp = نیا AstVarScope{flp, scopep, varp};
        self_Test_Hier_پہلا(trigVscp);
        self_Test_معمول_پہلا(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_درخت(), trigVscp);
        VL_DO_DANGLING(scopep->delete_درخت(), scopep);
        VL_DO_DANGLING(varp->delete_درخت(), varp);
    }
    ساکن خالی self_Test_معمول_پہلا(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = نیا Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گرہ_Stmt* { واپس نیا AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t0->لاگت(1000);
        t0->ترجیح_2(1100);
        ExecMTask* ثابت t1 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t1->لاگت(100);
        t1->ترجیح_2(100);
        ExecMTask* ثابت t2 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t2->لاگت(100);
        t2->ترجیح_2(100);
        t2->دھاگے(2);
        ExecMTask* ثابت t3 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t3->لاگت(100);
        t3->ترجیح_2(100);
        t3->دھاگے(3);
        ExecMTask* ثابت t4 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t4->لاگت(100);
        t4->ترجیح_2(100);
        t4->دھاگے(3);
        ExecMTask* ثابت t5 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t5->لاگت(100);
        t5->ترجیح_2(100);
        ExecMTask* ثابت t6 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
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
        نیا V3_گراف_کنارہ{&گراف, t0, t1, 1};
        نیا V3_گراف_کنارہ{&گراف, t0, t2, 1};
        نیا V3_گراف_کنارہ{&گراف, t2, t3, 1};
        نیا V3_گراف_کنارہ{&گراف, t2, t4, 1};
        نیا V3_گراف_کنارہ{&گراف, t3, t5, 1};
        نیا V3_گراف_کنارہ{&گراف, t4, t6, 1};

        constexpr uint32_t دھاگے = 2;
        constexpr uint32_t hier_دھاگے = 6;
        Pack_دھاگے packer{دھاگے, hier_دھاگے,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::سمتیہ<Thread_جدول_وقت> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.سائز(), 3);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے.سائز(), دھاگے);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0].سائز(), 2);
        برائے (سائز_t i = 1; i < scheduled[0].m_دھاگے.سائز(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_دھاگے[i].سائز(), 0);

        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_دھاگے.سائز(), hier_دھاگے / 3);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_دھاگے.سائز(), دھاگے);
        UASSERT_SELFTEST(scheduled[2].m_دھاگے[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_دھاگے[1][0], t6);

        UASSERT_SELFTEST(Thread_جدول_وقت::s_mtaskState.سائز(), 7);

        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t6), 1);

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

        برائے (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_جدول_وقت::s_mtaskState.clear();
    }
    ساکن خالی self_Test_Hier_پہلا(AstVarScope* trigVscp) {
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        Ast_Exec_گراف* ثابت execGraphp = نیا Ast_Exec_گراف{flp, "test", trigVscp};
        V3_گراف& گراف = *execGraphp->depGraphp();
        ثابت خودکار makeBody = [&]() -> Ast_گرہ_Stmt* { واپس نیا AstComment{flp, ""}; };
        ExecMTask* ثابت t0 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t0->لاگت(1000);
        t0->ترجیح_2(1100);
        t0->دھاگے(2);
        ExecMTask* ثابت t1 = نیا ExecMTask{execGraphp, خالی_اشاریہ, makeBody()};
        t1->لاگت(100);
        t1->ترجیح_2(100);

        /*
                          0
                          |
                          1
        */
        نیا V3_گراف_کنارہ{&گراف, t0, t1, 1};

        constexpr uint32_t دھاگے = 1;
        constexpr uint32_t hier_دھاگے = 2;
        Pack_دھاگے packer{دھاگے, hier_دھاگے,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ثابت std::سمتیہ<Thread_جدول_وقت> scheduled = packer.pack(گراف);
        UASSERT_SELFTEST(scheduled.سائز(), 2);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے.سائز(), hier_دھاگے / 2);
        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0].سائز(), 1);
        برائے (سائز_t i = 1; i < scheduled[0].m_دھاگے.سائز(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_دھاگے[i].سائز(), 0);

        UASSERT_SELFTEST(scheduled[0].m_دھاگے[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_دھاگے.سائز(), دھاگے);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0].سائز(), 1);
        برائے (سائز_t i = 1; i < scheduled[1].m_دھاگے.سائز(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_دھاگے[i].سائز(), 0);
        UASSERT_SELFTEST(scheduled[1].m_دھاگے[0][0], t1);

        UASSERT_SELFTEST(Thread_جدول_وقت::s_mtaskState.سائز(), 2);

        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_جدول_وقت::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_وقت(scheduled[1], t1, 1), 1130);

        برائے (V3_گراف_راس& vtx : گراف.vertices()) vtx.as<ExecMTask>()->funcp()->delete_درخت();
        VL_DO_DANGLING(execGraphp->delete_درخت(), execGraphp);
        Thread_جدول_وقت::s_mtaskState.clear();
    }

    ساکن std::سمتیہ<Thread_جدول_وقت> apply(V3_گراف& mtask_گراف) {
        واپس Pack_دھاگے{}.pack(mtask_گراف);
    }
};

استعمال EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
استعمال Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

خالی normalizeCosts(Costs& costs) {
    ثابت خودکار scale_لاگت = [](uint64_t قدر, دوہرا multiplier) {
        دوہرا scaled = static_cast<دوہرا>(قدر) * multiplier;
        اگر (قدر && scaled < 1) scaled = 1;
        واپس static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t مجموع_لاگت_Profiled = 0;  // For data with estimate and profile
    uint64_t مجموع_لاگت_Estimate = 0;  // For data with estimate and profile
    برائے (ثابت خودکار& est : costs) {
        اگر (est.دوسرا.دوسرا) {
            مجموع_لاگت_Estimate += est.دوسرا.پہلا;
            مجموع_لاگت_Profiled += est.دوسرا.دوسرا;
        }
    }

    اگر (مجموع_لاگت_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ثابت دوہرا est_مقصد_Profile
            = static_cast<دوہرا>(مجموع_لاگت_Profiled) / static_cast<دوہرا>(مجموع_لاگت_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_مقصد_Profile
                                                    << ", sumCostProfiled=" << مجموع_لاگت_Profiled
                                                    << " sumCostEstimate=" << مجموع_لاگت_Estimate);
        برائے (خودکار& est : costs) {
            uint64_t& لاگت_Estimate = est.دوسرا.پہلا;
            لاگت_Estimate = scale_لاگت(لاگت_Estimate, est_مقصد_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_لاگت = 0;
    برائے (خودکار& est : costs) {
        ثابت uint64_t& لاگت_Estimate = est.دوسرا.پہلا;
        ثابت uint64_t& لاگت_Profiled = est.دوسرا.دوسرا;
        اگر (max_لاگت < لاگت_Estimate) max_لاگت = لاگت_Estimate;
        اگر (max_لاگت < لاگت_Profiled) max_لاگت = لاگت_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.دوسرا.پہلا << " cp=" << est.دوسرا.دوسرا);
    }
    constexpr uint64_t scale_نیچے_کو_مقصد = 10000000;  // Extra room for future algorithms to add costs
    اگر (max_لاگت > scale_نیچے_کو_مقصد) {
        ثابت دوہرا scaleup = static_cast<دوہرا>(scale_نیچے_کو_مقصد) / static_cast<دوہرا>(max_لاگت);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_لاگت);
        برائے (خودکار& est : costs) {
            est.دوسرا.پہلا = scale_لاگت(est.دوسرا.پہلا, scaleup);
            est.دوسرا.دوسرا = scale_لاگت(est.دوسرا.دوسرا, scaleup);
        }
    }
}

خالی remove_خالی_خانے_MTasks(Ast_Exec_گراف* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    برائے (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        اگر (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_عقب()->delete_درخت(), callp);
    }
    V3_گراف* ثابت execMTaskGraphp = execGraphp->depGraphp();
    برائے (V3_گراف_راس* ثابت vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        اگر (funcp->stmtsp()) جاری;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_کنارے(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_عقب()->delete_درخت(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
}

خالی fillinCosts(V3_گراف* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    برائے (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ثابت uint64_t لاگت_Estimate = V3_Instr_گنتی::گنتی(mtp->funcp(), جھوٹ);
        ثابت uint64_t لاگت_Profiled
            = V3Control::get_Profile_ڈیٹا(v3Global.opt.سابقہ(), mtp->hashName());
        اگر (لاگت_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << لاگت_Profiled);
        }
        costs[mtp->id()] = std::make_pair(لاگت_Estimate, لاگت_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    صحیح کل_Estimates = 0;
    صحیح missingProfiles = 0;
    برائے (V3_گراف_راس& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ثابت mtp = vtx.as<ExecMTask>();
        ثابت uint32_t لاگت_Estimate = costs[mtp->id()].پہلا;
        ثابت uint64_t لاگت_Profiled = costs[mtp->id()].دوسرا;
        UINFO(9, "ce = " << لاگت_Estimate << " cp=" << لاگت_Profiled);
        UASSERT(لاگت_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(لاگت_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ثابت uint64_t لاگت_Profiled32 = static_cast<uint32_t>(لاگت_Profiled);
        uint32_t لاگت_مقصد_Use = لاگت_Profiled32;
        اگر (!لاگت_Profiled32) {
            لاگت_مقصد_Use = لاگت_Estimate;
            اگر (لاگت_Estimate != 0) ++missingProfiles;
        }
        اگر (لاگت_Estimate != 0) ++کل_Estimates;
        mtp->لاگت(لاگت_مقصد_Use);
        mtp->ترجیح_2(لاگت_مقصد_Use);
    }

    اگر (missingProfiles) {
        اگر (File_سطر* ثابت fl = V3Control::get_Profile_ڈیٹا_File_سطر()) {
            اگر (V3Control::شامل_ہے_MTask_Profile_ڈیٹا()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << کل_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

خالی finalizeCosts(V3_گراف* execMTaskGraphp) {
    گراف_دھارا_Unordered ser(execMTaskGraphp, گراف_Way::الٹ_2);
    جب (ثابت V3_گراف_راس* ثابت vxp = ser.nextp()) {
        ExecMTask* ثابت mtp = const_cast<V3_گراف_راس*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        برائے (V3_گراف_کنارہ& کنارہ : mtp->برآمد_کنارے()) {
            ثابت ExecMTask* ثابت followp = کنارہ.چوٹی()->as<ExecMTask>();
            اگر ((followp->ترجیح_2() + mtp->لاگت()) > mtp->ترجیح_2()) {
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
        واپس vtxp->as<ثابت ExecMTask>()->لاگت();
    });
    V3Stats::جوڑو_Stat("MTask graph, final, critical path cost", report.critical_راستہ_لاگت());
    V3Stats::جوڑو_Stat("MTask graph, final, total graph cost", report.کل_گراف_لاگت());
    V3Stats::جوڑو_Stat("MTask graph, final, mtask count", report.راس_گنتی());
    V3Stats::جوڑو_Stat("MTask graph, final, edge count", report.کنارہ_گنتی());
    V3Stats::جوڑو_Stat("MTask graph, final, parallelism factor", report.parallelism_ضربی_عدد());
    اگر (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_راستہ_لاگت());
        UINFO(0, "    Total graph cost = " << report.کل_گراف_لاگت());
        UINFO(0, "    MTask vertex count = " << report.راس_گنتی());
        UINFO(0, "    Edge count = " << report.کنارہ_گنتی());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ضربی_عدد());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ثابت std::string& name, uint32_t nDependencies) {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();
    AstBasicDType* ثابت dtypep
        = v3Global.rootp()->typeTablep()->ڈھونڈو_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ثابت varp = نیا AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(سچ);
    varp->valuep(نیا AstConst{fl, nDependencies});
    modp->جوڑو_Stmtsp(varp);
    واپس varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ثابت std::string& طریقہ) {
    File_سطر* ثابت fl = varp->fileline();
    AstVarRef* ثابت refp = نیا AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ثابت AstScope* ثابت چوٹی_Scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();
    refp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::VlSyms{}, چوٹی_Scopep->nameDotless()});
    AstCStmt* ثابت stmtp = نیا AstCStmt{fl};
    stmtp->جوڑو(refp);
    stmtp->جوڑو("." + طریقہ + ";");
    واپس stmtp;
}

خالی جوڑو_MTask_مقصد_Function(ثابت Thread_جدول_وقت& جدول_وقت, ثابت uint32_t threadId, AstCFunc* funcp,
                        ثابت ExecMTask* mtaskp,
                        ثابت std::unordered_map<ثابت ExecMTask*, AstVar*>& stateVars) {
    AstScope* ثابت scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> خالی {  //
        funcp->جوڑو_Stmtsp(نیا AstCStmt{fl, stmt});
    };

    اگر (جدول_وقت.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        اگر (v3Global.opt.profExec()) {
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->جوڑو_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        اگر (v3Global.opt.profExec()) {
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    اگر (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        جوڑو_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::مقصد_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ثابت callp = نیا AstCCall{fl, mtaskp->funcp()};
    callp->self_اشارہ_گر(VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::VlSyms{}, scopep->nameDotless()});
    callp->dtype_سیٹ_Void();
    funcp->جوڑو_Stmtsp(callp->makeStmt());

    اگر (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        جوڑو_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::مقصد_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    برائے (ثابت V3_گراف_کنارہ& کنارہ : mtaskp->برآمد_کنارے()) {
        ثابت ExecMTask* ثابت nextp = کنارہ.چوٹی()->as<ExecMTask>();
        اگر (جدول_وقت.threadId(nextp) != threadId && جدول_وقت.شامل_ہے(nextp)) {
            funcp->جوڑو_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ثابت std::سمتیہ<AstCFunc*> createThreadFunctions(ثابت Thread_جدول_وقت& جدول_وقت,
                                                   ثابت string& لیبل) {
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    File_سطر* ثابت fl = modp->fileline();

    std::سمتیہ<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ثابت ExecMTask*, AstVar*> stateVars;
    برائے (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : جدول_وقت.m_دھاگے) {
        برائے (ثابت ExecMTask* ثابت mtaskp : thread) {
            اگر (ثابت uint32_t nDependencies = جدول_وقت.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_مقصد_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    برائے (ثابت std::سمتیہ<ثابت ExecMTask*>& thread : جدول_وقت.m_دھاگے) {
        اگر (thread.خالی_خانے()) جاری;
        ثابت uint32_t threadId = جدول_وقت.threadId(thread.front());
        ثابت string name{"__Vthread__" + لیبل + "__s" + cvt_مقصد_Str(جدول_وقت.id()) + "__t"
                          + cvt_مقصد_Str(threadId)};
        AstCFunc* ثابت funcp = نیا AstCFunc{fl, name, خالی_اشاریہ, "void"};
        modp->جوڑو_Stmtsp(funcp);
        funcps.ڈالو_عقب(funcp);
        funcp->isStatic(سچ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(سچ);
        funcp->اندراج_نقطہ(سچ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->جوڑو_Stmtsp(نیا AstCStmt{fl, EmitCUtil::void_Self_تفویض(modp)});
        funcp->جوڑو_Stmtsp(نیا AstCStmt{fl, EmitCUtil::sym_Class_تفویض()});

        // Invoke each mtask scheduled to this thread from the thread function
        برائے (ثابت ExecMTask* ثابت mtaskp : thread) {
            جوڑو_MTask_مقصد_Function(جدول_وقت, threadId, funcp, mtaskp, stateVars);
        }
    }

    واپس funcps;
}

خالی جوڑو_Thread_آغاز_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& لیبل = execGraphp->name();

    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> خالی {  //
        execGraphp->runfuncp()->جوڑو_Stmtsp(نیا AstCStmt{fl, stmt});
    };

    اگر (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    جوڑو_CStmt("vlSymsp->__Vm_even_cycle__" + لیبل + " = !vlSymsp->__Vm_even_cycle__" + لیبل + ";");

    اگر (!v3Global.opt.hier_بلاکیں().خالی_خانے()) جوڑو_CStmt("std::vector<size_t> indexes;");
}

خالی جوڑو_Thread_انتہا_Wrapper(Ast_Exec_گراف* ثابت execGraphp) {
    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> خالی {  //
        File_سطر* ثابت flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->جوڑو_Stmtsp(نیا AstCStmt{flp, stmt});
    };

    جوڑو_CStmt("Verilated::mtaskId(0);");
    اگر (v3Global.opt.profExec()) { جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
خالی جوڑو_Thread_آغاز_مقصد_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp,
                               ثابت std::سمتیہ<AstCFunc*>& funcps, uint32_t جدول_وقت_Id) {
    // FileLine used for constructing nodes below
    File_سطر* ثابت fl = v3Global.rootp()->fileline();
    ثابت string& لیبل = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ثابت finalVarp
        = newMTaskStateVar("final__" + cvt_مقصد_Str(جدول_وقت_Id) + "__" + لیبل, funcps.سائز());
    برائے (AstCFunc* ثابت funcp : funcps) {
        funcp->جوڑو_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> خالی {  //
        execGraphp->runfuncp()->جوڑو_Stmtsp(نیا AstCStmt{fl, stmt});
    };

    ثابت uint32_t آخری = funcps.سائز() - 1;
    اگر (!v3Global.opt.hier_بلاکیں().خالی_خانے() && آخری > 0) {
        جوڑو_CStmt("for (size_t i = 0; i < " + std::مقصد_string(آخری) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    برائے (AstCFunc* ثابت funcp : funcps) {
        اگر (i != آخری) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ثابت cstmtp = نیا AstCStmt{fl};
            execGraphp->runfuncp()->جوڑو_Stmtsp(cstmtp);
            cstmtp->جوڑو("vlSymsp->__Vm_threadPoolp->workerp(");
            اگر (v3Global.opt.hier_بچہ() || !v3Global.opt.hier_بلاکیں().خالی_خانے()) {
                cstmtp->جوڑو("indexes[" + std::مقصد_string(i) + "]");
            } ورنہ {
                cstmtp->جوڑو(std::مقصد_string(i));
            }
            cstmtp->جوڑو(")->addTask(");
            cstmtp->جوڑو(نیا Ast_Addr_کا_CFunc{fl, funcp});
            cstmtp->جوڑو(", vlSelf, vlSymsp->__Vm_even_cycle__" + لیبل + ");");
        } ورنہ {
            // The last will run on the main thread.
            AstCCall* ثابت callp = نیا AstCCall{fl, funcp};
            callp->dtype_سیٹ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + لیبل);
            execGraphp->runfuncp()->جوڑو_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::جوڑو_Stat_مجموع("Optimizations, Thread schedule total tasks", i);

    اگر (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->جوڑو_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + لیبل + ")"));
    اگر (v3Global.opt.profExec()) {
        جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    اگر (!v3Global.opt.hier_بلاکیں().خالی_خانے() && آخری > 0) {
        جوڑو_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

خالی processMTaskBodies(Ast_Exec_گراف* ثابت execGraphp) {
    برائے (V3_گراف_راس* ثابت vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ثابت mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ثابت funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_گرہ* stmtsp = funcp->stmtsp()->unlink_Fr_عقب_With_اگلا();

        // Helper function to make the code a bit more legible
        ثابت خودکار جوڑو_CStmt = [=](ثابت string& stmt) -> خالی {  //
            funcp->جوڑو_Stmtsp(نیا AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        اگر (v3Global.opt.profExec()) {
            std::string دلائل = std::مقصد_string(mtaskp->id());
            دلائل += ", " + std::مقصد_string(mtaskp->predict_آغاز());
            دلائل += ", \"";
            اگر (v3Global.opt.hier_بچہ()) دلائل += v3Global.opt.چوٹی_Module();
            دلائل += "\"";
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + دلائل + ");");
        }
        // Set mtask ID in the run-time system
        جوڑو_CStmt("Verilated::mtaskId(" + std::مقصد_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->جوڑو_Stmtsp(stmtsp);
        // Flush message queue
        جوڑو_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        اگر (v3Global.opt.profExec()) {
            ثابت std::string& دلائل = std::مقصد_string(mtaskp->لاگت());
            جوڑو_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + دلائل + ");");
        }
    }
}

خالی implement_Exec_گراف(Ast_Exec_گراف* ثابت execGraphp, ثابت Thread_جدول_وقت& جدول_وقت) {
    // Nothing to be done if there are no MTasks in the graph at all.
    اگر (execGraphp->depGraphp()->خالی_خانے()) واپس;

    // Create a function to be run by each thread.
    ثابت std::سمتیہ<AstCFunc*>& funcps = createThreadFunctions(جدول_وقت, execGraphp->name());
    UASSERT(!funcps.خالی_خانے(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    جوڑو_Thread_آغاز_مقصد_Exec_گراف(execGraphp, funcps, جدول_وقت.id());
}

خالی create_تسلسل_Function(Ast_Exec_گراف* ثابت execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_سطر* ثابت flp = execGraphp->fileline();
    Ast_گرہ_Module* ثابت modp = v3Global.rootp()->چوٹی_Modulep();
    AstCFunc* ثابت funcp = نیا AstCFunc{flp, "runExecGraph_" + execGraphp->name(), خالی_اشاریہ};
    funcp->isLoose(سچ);
    funcp->voidSelf(سچ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(سچ);
    modp->جوڑو_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

خالی جوڑو_Trigger_Masks(Ast_Exec_گراف* ثابت execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_سطر* ثابت flp = execGraphp->fileline();
    AstVar* ثابت triggersp = execGraphp->triggerp()->varp();
    برائے (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> لفظ_Masks;
        callp->funcp()->foreach([&](ثابت AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            برائے (ثابت uint32_t اشاریہ : trigp->indices()) {
                لفظ_Masks[VL_BITWORD_Q(اشاریہ)] |= VL_ثنائی_ہندسہ_Q(اشاریہ);
            }
        });
        AstConsArray* ثابت consp = نیا AstConsArray{flp, triggersp->dtypep(),
                                                     نیا AstConst{flp, AstConst::Unsized64{}, 0}};
        برائے (ثابت خودکار& لفظ_نقاب : لفظ_Masks) {
            consp->valuep(لفظ_نقاب.پہلا,
                          نیا AstConst{flp, AstConst::Unsized64{}, لفظ_نقاب.دوسرا});
        }
        execGraphp->جوڑو_Maskps(V3ConstPool::ڈھونڈو(consp));
        VL_DO_DANGLING(consp->delete_درخت(), consp);
    }
}

خالی جوڑو_Trigger_Costs(Ast_Exec_گراف* ثابت execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_سطر* ثابت flp = execGraphp->fileline();
    ثابت uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::سمتیہ<uint32_t> costs(nTriggers, 0);
    برائے (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            اگر (ثابت AstTriggered* ثابت trigp = VN_CAST(ifp->condp(), Triggered)) {
                ثابت uint32_t لاگت = V3_Instr_گنتی::گنتی(ifp, جھوٹ);
                برائے (ثابت uint32_t اشاریہ : trigp->indices()) costs.at(اشاریہ) += لاگت;
            }
        });
    }
    AstNetlist* ثابت netlistp = v3Global.rootp();
    AstUnpackArrayDType* ثابت dtypep
        = نیا AstUnpackArrayDType{flp, netlistp->ڈھونڈو_ثنائی_ہندسہ_DType(32, 32, VSigning::UNSIGNED),
                                  نیا AstRange{flp, static_cast<صحیح>(nTriggers - 1), 0}};
    netlistp->typeTablep()->جوڑو_Typesp(dtypep);
    AstConsArray* ثابت consp = نیا AstConsArray{flp, dtypep, نیا AstConst{flp, 0}};
    برائے (uint32_t اشاریہ = 0; اشاریہ < nTriggers; ++اشاریہ) {
        اگر (costs[اشاریہ]) consp->valuep(اشاریہ, نیا AstConst{flp, costs[اشاریہ]});
    }
    execGraphp->triggerCostsp(V3ConstPool::ڈھونڈو(consp));
    VL_DO_DANGLING(consp->delete_درخت(), consp);
}

// Called by Verilator top stage
خالی implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::سمتیہ<Ast_Exec_گراف*> execGraphps;
    netlistp->چوٹی_Modulep()->foreach([&](Ast_Exec_گراف* egp) { execGraphps.emplace_عقب(egp); });

    // Process each
    برائے (Ast_Exec_گراف* ثابت execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_خالی_خانے_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        اگر (execGraphp->depGraphp()->خالی_خانے()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_عقب()->delete_درخت(), execGraphp);
            واپس;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        اگر (dump_گراف_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_تسلسل_Function(execGraphp);
        جوڑو_Thread_آغاز_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ثابت std::سمتیہ<Thread_جدول_وقت> packed = Pack_دھاگے::apply(*execGraphp->depGraphp());
        V3Stats::جوڑو_Stat_مجموع("Optimizations, Thread schedule count",
                            static_cast<دوہرا>(packed.سائز()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        برائے (ثابت Thread_جدول_وقت& جدول_وقت : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_گراف(execGraphp, جدول_وقت);
        }

        جوڑو_Thread_انتہا_Wrapper(execGraphp);

        جوڑو_Trigger_Masks(execGraphp);
        جوڑو_Trigger_Costs(execGraphp);
    }
}

خالی selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پہلا, 1000);
        UASSERT_SELFTEST(costs[1].دوسرا, 1000);
        UASSERT_SELFTEST(costs[2].پہلا, 2000);
        UASSERT_SELFTEST(costs[2].دوسرا, 0);
        UASSERT_SELFTEST(costs[3].پہلا, 3000);
        UASSERT_SELFTEST(costs[3].دوسرا, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].پہلا, 2500000);
        UASSERT_SELFTEST(costs[1].دوسرا, 5000000);
        UASSERT_SELFTEST(costs[2].پہلا, 5000000);
        UASSERT_SELFTEST(costs[2].دوسرا, 10000000);
        UASSERT_SELFTEST(costs[3].پہلا, 7500000);
        UASSERT_SELFTEST(costs[3].دوسرا, 1);
    }

    Pack_دھاگے::selfTest();
}

}  // namespace V3ExecGraph
