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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3ExecGraph.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3GraphStream.h"
#అన్తర్భావ "V3Hasher.h"
#అన్తర్భావ "V3InstrCount.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(Ast_Exec_లేఖాచిత్రం* execGraphp, AstScope* scopep, Ast_కణుపు_Stmt* stmtsp,
                                 uint32_t id) {
    స్థిరరాశి std::string newName = execGraphp->name() + "_mtask" + std::గమ్యం_string(id);
    AstCFunc* స్థిరరాశి newp = కొత్త AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(నిజం);
    newp->voidSelf(నిజం);  // Called through VlExecGraph::Fnp
    newp->dontCombine(నిజం);
    newp->కూడు_Stmtsp(stmtsp);
    ఒకవేళ (scopep) scopep->కూడు_Blocksp(newp);
    తిరిగి newp;
}

ExecMTask::ExecMTask(Ast_Exec_లేఖాచిత్రం* execGraphp, AstScope* scopep,
                     Ast_కణుపు_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_లేఖాచిత్రం_శీర్షం{execGraphp->depGraphp()},
      m_id{s_తదుపరి_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).గమ్యం_String()} {}

శూన్య ExecMTask::dump(std::ostream& str) స్థిరరాశి {
    str << name() << "." << cvt_గమ్యం_Hex(ఇది);
    ఒకవేళ (ప్రాధాన్యత_2() || ఖర్చు()) str << " [pr=" << ప్రాధాన్యత_2() << " c=" << cvt_గమ్యం_Str(ఖర్చు()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_తదుపరి_Id{0};

నామాకాశ V3_Exec_లేఖాచిత్రం {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
తరగతి Thread_కాలపట్టిక final {
    మిత్ర తరగతి Pack_దారాలు;

    uint32_t m_id;  // Unique ID of a schedule
    స్థిర uint32_t s_తదుపరి_Id;  // Next ID number to use
    std::unordered_సమితి<స్థిరరాశి ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_చివర_సమయం = 0;  // Latest task end time in this schedule

బహిరంగ:
    // CONSTANTS
    స్థిర constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    నిర్మాణం MTaskState final {
        uint32_t completion_సమయం = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        స్థిరరాశి ExecMTask* nextp = శూన్యసూచిక;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::సదిశ<std::సదిశ<స్థిరరాశి ExecMTask*>> m_దారాలు;

    // Global state for each mtask.
    స్థిర std::unordered_map<స్థిరరాశి ExecMTask*, MTaskState> s_mtaskState;

    స్పష్టమైన Thread_కాలపట్టిక(uint32_t n_దారాలు)
        : m_id{s_తదుపరి_Id++}
        , m_దారాలు{n_దారాలు} {}
    Thread_కాలపట్టిక(Thread_కాలపట్టిక&&) = అప్రమేయ;
    Thread_కాలపట్టిక& క్రియాకారకం=(Thread_కాలపట్టిక&&) = అప్రమేయ;

అంతరంగ:
    VL_UNCOPYABLE(Thread_కాలపట్టిక);

    స్థిర constexpr ద్విగుణ s_thread_Box_వెడల్పు = 2.5;
    స్థిర constexpr ద్విగుణ s_thread_Box_ఎత్తు = 1.5;
    స్థిర constexpr ద్విగుణ s_horizontalGap = s_thread_Box_వెడల్పు / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    స్థిర శూన్య dumpDotFilePrefixedAlways(స్థిరరాశి std::సదిశ<Thread_కాలపట్టిక>& schedules,
                                          స్థిరరాశి string& nameComment, uint32_t n_దారాలు) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_దారాలు);
    }
    స్థిర శూన్య dumpDotFile(స్థిరరాశి std::సదిశ<Thread_కాలపట్టిక>& schedules, స్థిరరాశి string& filename,
                            uint32_t n_దారాలు) {
        // This generates a file used by graphviz, https://www.graphviz.org
        స్థిరరాశి std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        ఒకవేళ (logp->fail()) v3fatal("Can't write file: " << filename);

        //శీర్షిక
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        కొరకు (uint32_t i = 0; i < n_దారాలు; ++i) {
            స్థిరరాశి string name = "t" + std::గమ్యం_string(i);
            స్థిరరాశి string label = "Thread " + std::గమ్యం_string(i);
            constexpr ద్విగుణ posX = -s_horizontalGap;
            స్థిరరాశి ద్విగుణ posY = -static_cast<ద్విగుణ>(i) * s_thread_Box_ఎత్తు;
            dump_Dot_File_Emit_ఖండం(logp, name, label, s_thread_Box_వెడల్పు, s_thread_Box_ఎత్తు, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_ఖర్చు = 0;
        కొరకు (స్థిరరాశి స్వయంచాలక& state : Thread_కాలపట్టిక::s_mtaskState) {
            స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp = state.మొదటి;
            max_ఖర్చు = std::max(max_ఖర్చు, mtaskp->ఖర్చు());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_చొప్పున_Longest_Mtask = 4;
        స్థిరరాశి uint32_t segment_ఖర్చు = max_ఖర్చు / segments_చొప్పున_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::సదిశ<ద్విగుణ> offsets(n_దారాలు, 0.0);
        కొరకు (స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక : schedules) {
            ఒకవేళ (కాలపట్టిక.mtasks.ఖాళీలు()) కొనసాగు;
            ఉపయోగించి స్తంభం = std::సదిశ<స్థిరరాశి ExecMTask*>;
            std::సదిశ<స్తంభం> columns = {{}};

            // Order tasks based on their start time
            నిర్మాణం Cmp final {
                తార్కిక క్రియాకారకం()(స్థిరరాశి ExecMTask* స్థిరరాశి a, స్థిరరాశి ExecMTask* స్థిరరాశి b) స్థిరరాశి {
                    ఒకవేళ (ప్రారంభం_సమయం(a) == ప్రారంభం_సమయం(b)) తిరిగి threadId(a) < threadId(b);
                    తిరిగి ప్రారంభం_సమయం(a) < ప్రారంభం_సమయం(b);
                }
            };
            స్థిరరాశి std::multiset<స్థిరరాశి ExecMTask*, Cmp> tasks(కాలపట్టిక.mtasks.begin(),
                                                             కాలపట్టిక.mtasks.చివర());

            కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp : tasks) {
                స్తంభం& స్తంభం_2 = columns.వెనుక();
                UASSERT(స్తంభం_2.కొలత() <= n_దారాలు, "Invalid partitioning");

                తార్కిక intersects = నిజం;
                కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి earlierMtask : స్తంభం_2) {
                    ఒకవేళ (చివర_సమయం(mtaskp) <= ప్రారంభం_సమయం(earlierMtask)
                        || ప్రారంభం_సమయం(mtaskp) >= చివర_సమయం(earlierMtask)) {
                        intersects = అబద్ధం;
                        ఆపు;
                    }
                }
                ఒకవేళ (intersects) {
                    స్తంభం_2.emplace_వెనుక(mtaskp);
                } లేకపోతే {
                    columns.emplace_వెనుక(స్తంభం{mtaskp});
                }
            }

            UASSERT(!columns.front().ఖాళీలు(), "Should be populated by mtasks");

            కొరకు (స్థిరరాశి స్తంభం& స్తంభం_2 : columns) {
                ద్విగుణ చివరి_స్తంభం_స్థానభ్రంశం = 0;
                కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp : స్తంభం_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, కాలపట్టిక, segment_ఖర్చు, offsets);
                    చివరి_స్తంభం_స్థానభ్రంశం = std::max(చివరి_స్తంభం_స్థానభ్రంశం, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.చివర(), చివరి_స్తంభం_స్థానభ్రంశం);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_దారాలు);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            కొరకు (స్థిరరాశి std::సదిశ<స్థిరరాశి ExecMTask*>& thread : కాలపట్టిక.m_దారాలు) {
                ఒకవేళ (thread.ఖాళీలు()) ఆపు;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.వెనుక()->name() << " -> fork_"
                      << static_cast<పూర్ణాంకం>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                కొరకు (కొలత_t i = 1; i < thread.కొలత(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp : thread) {
                    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->బహిర్గతం_అంచులు()) {
                        స్థిరరాశి ExecMTask* స్థిరరాశి శిఖరం_MTaskp = అంచు.శిఖరం()->cast<స్థిరరాశి ExecMTask>();
                        ఒకవేళ (శిఖరం_MTaskp && కాలపట్టిక.కలిగియుంది(శిఖరం_MTaskp)
                            && threadId(శిఖరం_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << శిఖరం_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    స్థిర శూన్య dump_Dot_File_Emit_ఖండం(స్థిరరాశి std::unique_ptr<std::ofstream>& logp,
                                     స్థిరరాశి string& name, స్థిరరాశి string& label, ద్విగుణ వెడల్పు,
                                     ద్విగుణ ఎత్తు, ద్విగుణ xPos, ద్విగుణ yPos,
                                     స్థిరరాశి string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << వెడల్పు
              << " height=" << ఎత్తు << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    స్థిర శూన్య dumpDotFileEmitMTask(స్థిరరాశి std::unique_ptr<std::ofstream>& logp,
                                     స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp, స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక,
                                     uint32_t segment_ఖర్చు, std::సదిశ<ద్విగుణ>& offsets) {
        కొరకు (పూర్ణాంకం i = 0; i < mtaskp->దారాలు(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            స్థిరరాశి string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::గమ్యం_string(i);
            స్థిరరాశి string label = mtaskp->name() + " (" + std::గమ్యం_string(ప్రారంభం_సమయం(mtaskp)) + ':'
                                 + std::గమ్యం_string(చివర_సమయం(mtaskp)) + ')'
                                 + "\\ncost=" + std::గమ్యం_string(mtaskp->ఖర్చు())
                                 + "\\npriority=" + std::గమ్యం_string(mtaskp->ప్రాధాన్యత_2());
            స్థిరరాశి ద్విగుణ వెడల్పు
                = std::max(s_thread_Box_వెడల్పు,
                           s_thread_Box_వెడల్పు * static_cast<ద్విగుణ>(mtaskp->ఖర్చు()) / segment_ఖర్చు);
            స్థిరరాశి uint32_t mtaskThreadId = threadId(mtaskp) + i * కాలపట్టిక.m_దారాలు.కొలత();
            స్థిరరాశి ద్విగుణ xPos = వెడల్పు / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += వెడల్పు + s_horizontalGap;
            స్థిరరాశి ద్విగుణ yPos = -s_thread_Box_ఎత్తు * static_cast<ద్విగుణ>(mtaskThreadId);
            స్థిరరాశి string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_ఖండం(logp, name, label, వెడల్పు, s_thread_Box_ఎత్తు, xPos, yPos,
                                 fillColor);
        }
    }

    స్థిర శూన్య dumpDotFileEmitFork(స్థిరరాశి std::unique_ptr<std::ofstream>& logp, ద్విగుణ స్థానభ్రంశం,
                                    uint32_t n_దారాలు) {
        స్థిరరాశి string& name = "fork_" + std::గమ్యం_string(static_cast<పూర్ణాంకం>(స్థానభ్రంశం));
        constexpr ద్విగుణ వెడల్పు = s_thread_Box_వెడల్పు / 8;
        స్థిరరాశి ద్విగుణ ఎత్తు = s_thread_Box_ఎత్తు * n_దారాలు;
        స్థిరరాశి ద్విగుణ xPos = స్థానభ్రంశం - s_horizontalGap / 2;
        స్థిరరాశి ద్విగుణ yPos
            = -static_cast<ద్విగుణ>(n_దారాలు) / 2 * s_thread_Box_ఎత్తు + s_thread_Box_ఎత్తు / 2;
        dump_Dot_File_Emit_ఖండం(logp, name, "", వెడల్పు, ఎత్తు, xPos, yPos, "black");
    }

బహిరంగ:
    స్థిర uint32_t threadId(స్థిరరాశి ExecMTask* mtaskp) {
        స్థిరరాశి స్వయంచాలక& it = s_mtaskState.కనుగొను(mtaskp);
        తిరిగి it != s_mtaskState.చివర() ? it->రెండవ.threadId : UNASSIGNED;
    }
    స్థిర uint32_t ప్రారంభం_సమయం(స్థిరరాశి ExecMTask* mtaskp) {
        తిరిగి s_mtaskState.at(mtaskp).completion_సమయం - mtaskp->ఖర్చు();
    }
    స్థిర uint32_t చివర_సమయం(స్థిరరాశి ExecMTask* mtaskp) {
        తిరిగి s_mtaskState.at(mtaskp).completion_సమయం;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(స్థిరరాశి ExecMTask* mtaskp) స్థిరరాశి {
        స్థిరరాశి uint32_t thisThreadId = threadId(mtaskp);
        uint32_t ఫలితం = 0;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->లోపల_అంచులు()) {
            స్థిరరాశి ExecMTask* స్థిరరాశి prevp = అంచు.fromp()->as<ExecMTask>();
            ఒకవేళ (threadId(prevp) != thisThreadId && కలిగియుంది(prevp)) ++ఫలితం;
        }
        తిరిగి ఫలితం;
    }

    uint32_t id() స్థిరరాశి { తిరిగి m_id; }
    uint32_t కాలపట్టిక_On(స్థిరరాశి ExecMTask* mtaskp, uint32_t ఉత్తమ_Thread_Id) {
        mtasks.emplace(mtaskp);
        స్థిరరాశి uint32_t ఉత్తమ_చివర_సమయం = mtaskp->predict_ప్రారంభం() + mtaskp->ఖర్చు();
        m_చివర_సమయం = std::max(m_చివర_సమయం, ఉత్తమ_చివర_సమయం);
        s_mtaskState[mtaskp].completion_సమయం = ఉత్తమ_చివర_సమయం;
        s_mtaskState[mtaskp].threadId = ఉత్తమ_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::సదిశ<స్థిరరాశి ExecMTask*>& ఉత్తమ_Thread = m_దారాలు[ఉత్తమ_Thread_Id];
        ఒకవేళ (!ఉత్తమ_Thread.ఖాళీలు()) s_mtaskState[ఉత్తమ_Thread.వెనుక()].nextp = mtaskp;

        // Add the MTask to the schedule
        ఉత్తమ_Thread.నెట్టు_వెనుక(mtaskp);
        తిరిగి ఉత్తమ_చివర_సమయం;
    }
    తార్కిక కలిగియుంది(స్థిరరాశి ExecMTask* mtaskp) స్థిరరాశి { తిరిగి mtasks.లెక్క(mtaskp); }
    uint32_t చివర_సమయం() స్థిరరాశి { తిరిగి m_చివర_సమయం; }
};

uint32_t Thread_కాలపట్టిక::s_తదుపరి_Id = 0;
std::unordered_map<స్థిరరాశి ExecMTask*, Thread_కాలపట్టిక::MTaskState> Thread_కాలపట్టిక::s_mtaskState{};
constexpr ద్విగుణ V3_Exec_లేఖాచిత్రం::Thread_కాలపట్టిక::s_thread_Box_వెడల్పు;

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
తరగతి Pack_దారాలు final {
    // TYPES
    నిర్మాణం MTaskCmp final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి ExecMTask* ap, స్థిరరాశి ExecMTask* bp) స్థిరరాశి {
            తిరిగి ap->id() < bp->id();
        }
    };

    // MEMBERS
    స్థిరరాశి uint32_t m_n_దారాలు;  // Number of threads
    స్థిరరాశి uint32_t m_n_Hier_దారాలు;  // Number of threads used for hierarchical tasks
    స్థిరరాశి uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    స్థిరరాశి uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    స్పష్టమైన Pack_దారాలు(uint32_t n_దారాలు = v3Global.opt.దారాలు(),
                         uint32_t n_Hier_దారాలు = v3Global.opt.hier_దారాలు(),
                         గుర్తులేని sandbagNumerator = 30, గుర్తులేని sandbagDenom = 100)
        : m_n_దారాలు{n_దారాలు}
        , m_n_Hier_దారాలు{n_Hier_దారాలు}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_దారాలు() = అప్రమేయ;
    VL_UNCOPYABLE(Pack_దారాలు);

    // METHODS
    uint32_t completion_సమయం(స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక, స్థిరరాశి ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        ఒకవేళ (!కాలపట్టిక.కలిగియుంది(mtaskp)) తిరిగి 0;
        స్థిరరాశి Thread_కాలపట్టిక::MTaskState& state = కాలపట్టిక.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_కాలపట్టిక::UNASSIGNED, "Mtask should have assigned thread");
        ఒకవేళ (threadId == state.threadId) {
            // No overhead on same thread
            తిరిగి state.completion_సమయం;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_చివర_సమయం
            = state.completion_సమయం + (m_sandbagNumerator * mtaskp->ఖర్చు()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        ఒకవేళ (state.nextp) {
            స్థిరరాశి uint32_t successor_చివర_సమయం
                = completion_సమయం(కాలపట్టిక, state.nextp, state.threadId);
            ఒకవేళ ((sandbagged_చివర_సమయం >= successor_చివర_సమయం) && (successor_చివర_సమయం > 1)) {
                sandbagged_చివర_సమయం = successor_చివర_సమయం - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_చివర_సమయం);
        తిరిగి sandbagged_చివర_సమయం;
    }

    స్థిర తార్కిక isReady(Thread_కాలపట్టిక& కాలపట్టిక, స్థిరరాశి ExecMTask* mtaskp) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు_లోపల : mtaskp->లోపల_అంచులు()) {
            స్థిరరాశి ExecMTask* స్థిరరాశి prevp = అంచు_లోపల.fromp()->as<స్థిరరాశి ExecMTask>();
            ఒకవేళ (కాలపట్టిక.threadId(prevp) == Thread_కాలపట్టిక::UNASSIGNED) {
                // This predecessor is not assigned yet
                తిరిగి అబద్ధం;
            }
        }
        తిరిగి నిజం;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::సదిశ<Thread_కాలపట్టిక> pack(V3_లేఖాచిత్రం& mtask_లేఖాచిత్రం) {
        std::సదిశ<Thread_కాలపట్టిక> ఫలితం;
        ఫలితం.emplace_వెనుక(Thread_కాలపట్టిక{m_n_దారాలు});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        గణన తరగతి Scheduling_బహుళకం : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_బహుళకం బహుళకం = Scheduling_బహుళకం::SCHEDULING;

        // Time each thread is occupied until
        std::సదిశ<uint32_t> busyUntil(std::max(m_n_దారాలు, m_n_Hier_దారాలు), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::సమితి<ExecMTask*, MTaskCmp> readyMTasks;
        పూర్ణాంకం maxThreadWorkers = 1;

        // Build initial ready list
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : mtask_లేఖాచిత్రం.vertices()) {
            ExecMTask* స్థిరరాశి mtaskp = vtx.as<ExecMTask>();
            ఒకవేళ (isReady(ఫలితం.వెనుక(), mtaskp)) readyMTasks.చొప్పించు(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->దారాలు());
        }

        అయితే (!readyMTasks.ఖాళీలు()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t ఉత్తమ_సమయం = 0xffffffff;
            uint32_t ఉత్తమ_Thread_Id = 0;
            ExecMTask* ఉత్తమ_Mtaskp = శూన్యసూచిక;  // Todo: const ExecMTask*
            Thread_కాలపట్టిక& కాలపట్టిక = ఫలితం.వెనుక();
            కొరకు (uint32_t threadId = 0; threadId < కాలపట్టిక.m_దారాలు.కొలత(); ++threadId) {
                కొరకు (ExecMTask* స్థిరరాశి mtaskp : readyMTasks) {
                    ఒకవేళ (బహుళకం != Scheduling_బహుళకం::WIDE_TASK_SCHEDULING && mtaskp->దారాలు() > 1) {
                        బహుళకం = Scheduling_బహుళకం::WIDE_TASK_DISCOVERED;
                        కొనసాగు;
                    }
                    ఒకవేళ (బహుళకం == Scheduling_బహుళకం::WIDE_TASK_SCHEDULING && mtaskp->దారాలు() <= 1)
                        కొనసాగు;

                    uint32_t సమయం_Begin = busyUntil[threadId];
                    ఒకవేళ (సమయం_Begin > ఉత్తమ_సమయం) {
                        UINFO(6, "th " << threadId << " busy until " << సమయం_Begin
                                       << ", later than bestTime " << ఉత్తమ_సమయం
                                       << ", skipping thread.");
                        ఆపు;
                    }
                    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->లోపల_అంచులు()) {
                        స్థిరరాశి ExecMTask* స్థిరరాశి priorp = అంచు.fromp()->as<ExecMTask>();
                        స్థిరరాశి uint32_t పూర్వ_చివర_సమయం = completion_సమయం(కాలపట్టిక, priorp, threadId);
                        ఒకవేళ (పూర్వ_చివర_సమయం > సమయం_Begin) సమయం_Begin = పూర్వ_చివర_సమయం;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << సమయం_Begin
                                     << " on thread " << threadId);
                    ఒకవేళ ((సమయం_Begin < ఉత్తమ_సమయం)
                        || ((సమయం_Begin == ఉత్తమ_సమయం)
                            && ఉత్తమ_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ప్రాధాన్యత_2() > ఉత్తమ_Mtaskp->ప్రాధాన్యత_2()))) {
                        ఉత్తమ_సమయం = సమయం_Begin;
                        ఉత్తమ_Thread_Id = threadId;
                        ఉత్తమ_Mtaskp = mtaskp;
                    }
                }
            }

            స్థిరరాశి uint32_t చివర_సమయం = కాలపట్టిక.చివర_సమయం();

            ఒకవేళ (!ఉత్తమ_Mtaskp && బహుళకం == Scheduling_బహుళకం::WIDE_TASK_DISCOVERED) {
                బహుళకం = Scheduling_బహుళకం::WIDE_TASK_SCHEDULING;
                స్థిరరాశి uint32_t కొలత = m_n_Hier_దారాలు / maxThreadWorkers;
                UASSERT(కొలత, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                ఒకవేళ (కాలపట్టిక.s_mtaskState.ఖాళీలు()) ఫలితం.clear();
                ఫలితం.emplace_వెనుక(Thread_కాలపట్టిక{కొలత});
                std::fill(busyUntil.begin(), busyUntil.చివర(), చివర_సమయం);
                కొనసాగు;
            }

            ఒకవేళ (!ఉత్తమ_Mtaskp && బహుళకం == Scheduling_బహుళకం::WIDE_TASK_SCHEDULING) {
                బహుళకం = Scheduling_బహుళకం::SCHEDULING;
                UASSERT(!కాలపట్టిక.s_mtaskState.ఖాళీలు(), "Mtask should be added");
                ఫలితం.emplace_వెనుక(Thread_కాలపట్టిక{m_n_దారాలు});
                std::fill(busyUntil.begin(), busyUntil.చివర(), చివర_సమయం);
                కొనసాగు;
            }

            UASSERT(ఉత్తమ_Mtaskp, "Should have found some task");

            ఉత్తమ_Mtaskp->predict_ప్రారంభం(ఉత్తమ_సమయం);
            స్థిరరాశి uint32_t ఉత్తమ_చివర_సమయం = కాలపట్టిక.కాలపట్టిక_On(ఉత్తమ_Mtaskp, ఉత్తమ_Thread_Id);
            busyUntil[ఉత్తమ_Thread_Id] = ఉత్తమ_చివర_సమయం;

            // Update the ready list
            స్థిరరాశి కొలత_t erased = readyMTasks.erase(ఉత్తమ_Mtaskp);
            UASSERT_OBJ(erased > 0, ఉత్తమ_Mtaskp, "Should have erased something?");
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు_బహిర్గతం : ఉత్తమ_Mtaskp->బహిర్గతం_అంచులు()) {
                ExecMTask* స్థిరరాశి nextp = అంచు_బహిర్గతం.శిఖరం()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(కాలపట్టిక.threadId(nextp) == Thread_కాలపట్టిక::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.కనుగొను(nextp) == readyMTasks.చివర(), nextp,
                            "Tasks after one being assigned should not be ready");
                ఒకవేళ (isReady(కాలపట్టిక, nextp)) {
                    readyMTasks.చొప్పించు(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 4)
            Thread_కాలపట్టిక::dumpDotFilePrefixedAlways(ఫలితం, "schedule", m_n_దారాలు);

        తిరిగి ఫలితం;
    }

బహిరంగ:
    // SELF TEST
    స్థిర శూన్య selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
        AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->కనుగొను_UInt64_DType()};
        AstScope* స్థిరరాశి scopep = కొత్త AstScope{flp, శూన్యసూచిక, "test", శూన్యసూచిక, శూన్యసూచిక};
        AstVarScope* స్థిరరాశి trigVscp = కొత్త AstVarScope{flp, scopep, varp};
        self_Test_Hier_మొదటి(trigVscp);
        self_Test_సాధారణ_మొదటి(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_వృక్షం(), trigVscp);
        VL_DO_DANGLING(scopep->delete_వృక్షం(), scopep);
        VL_DO_DANGLING(varp->delete_వృక్షం(), varp);
    }
    స్థిర శూన్య self_Test_సాధారణ_మొదటి(AstVarScope* trigVscp) {
        File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
        Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp = కొత్త Ast_Exec_లేఖాచిత్రం{flp, "test", trigVscp};
        V3_లేఖాచిత్రం& లేఖాచిత్రం = *execGraphp->depGraphp();
        స్థిరరాశి స్వయంచాలక makeBody = [&]() -> Ast_కణుపు_Stmt* { తిరిగి కొత్త AstComment{flp, ""}; };
        ExecMTask* స్థిరరాశి t0 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t0->ఖర్చు(1000);
        t0->ప్రాధాన్యత_2(1100);
        ExecMTask* స్థిరరాశి t1 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t1->ఖర్చు(100);
        t1->ప్రాధాన్యత_2(100);
        ExecMTask* స్థిరరాశి t2 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t2->ఖర్చు(100);
        t2->ప్రాధాన్యత_2(100);
        t2->దారాలు(2);
        ExecMTask* స్థిరరాశి t3 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t3->ఖర్చు(100);
        t3->ప్రాధాన్యత_2(100);
        t3->దారాలు(3);
        ExecMTask* స్థిరరాశి t4 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t4->ఖర్చు(100);
        t4->ప్రాధాన్యత_2(100);
        t4->దారాలు(3);
        ExecMTask* స్థిరరాశి t5 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t5->ఖర్చు(100);
        t5->ప్రాధాన్యత_2(100);
        ExecMTask* స్థిరరాశి t6 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t6->ఖర్చు(100);
        t6->ప్రాధాన్యత_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t0, t1, 1};
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t0, t2, 1};
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t2, t3, 1};
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t2, t4, 1};
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t3, t5, 1};
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t4, t6, 1};

        constexpr uint32_t దారాలు = 2;
        constexpr uint32_t hier_దారాలు = 6;
        Pack_దారాలు packer{దారాలు, hier_దారాలు,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        స్థిరరాశి std::సదిశ<Thread_కాలపట్టిక> scheduled = packer.pack(లేఖాచిత్రం);
        UASSERT_SELFTEST(scheduled.కొలత(), 3);
        UASSERT_SELFTEST(scheduled[0].m_దారాలు.కొలత(), దారాలు);
        UASSERT_SELFTEST(scheduled[0].m_దారాలు[0].కొలత(), 2);
        కొరకు (కొలత_t i = 1; i < scheduled[0].m_దారాలు.కొలత(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_దారాలు[i].కొలత(), 0);

        UASSERT_SELFTEST(scheduled[0].m_దారాలు[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_దారాలు[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_దారాలు.కొలత(), hier_దారాలు / 3);
        UASSERT_SELFTEST(scheduled[1].m_దారాలు[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_దారాలు[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_దారాలు[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_దారాలు.కొలత(), దారాలు);
        UASSERT_SELFTEST(scheduled[2].m_దారాలు[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_దారాలు[1][0], t6);

        UASSERT_SELFTEST(Thread_కాలపట్టిక::s_mtaskState.కొలత(), 7);

        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t4, 5), 1360);

        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) vtx.as<ExecMTask>()->funcp()->delete_వృక్షం();
        VL_DO_DANGLING(execGraphp->delete_వృక్షం(), execGraphp);
        Thread_కాలపట్టిక::s_mtaskState.clear();
    }
    స్థిర శూన్య self_Test_Hier_మొదటి(AstVarScope* trigVscp) {
        File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
        Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp = కొత్త Ast_Exec_లేఖాచిత్రం{flp, "test", trigVscp};
        V3_లేఖాచిత్రం& లేఖాచిత్రం = *execGraphp->depGraphp();
        స్థిరరాశి స్వయంచాలక makeBody = [&]() -> Ast_కణుపు_Stmt* { తిరిగి కొత్త AstComment{flp, ""}; };
        ExecMTask* స్థిరరాశి t0 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t0->ఖర్చు(1000);
        t0->ప్రాధాన్యత_2(1100);
        t0->దారాలు(2);
        ExecMTask* స్థిరరాశి t1 = కొత్త ExecMTask{execGraphp, శూన్యసూచిక, makeBody()};
        t1->ఖర్చు(100);
        t1->ప్రాధాన్యత_2(100);

        /*
                          0
                          |
                          1
        */
        కొత్త V3_లేఖాచిత్రం_అంచు{&లేఖాచిత్రం, t0, t1, 1};

        constexpr uint32_t దారాలు = 1;
        constexpr uint32_t hier_దారాలు = 2;
        Pack_దారాలు packer{దారాలు, hier_దారాలు,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        స్థిరరాశి std::సదిశ<Thread_కాలపట్టిక> scheduled = packer.pack(లేఖాచిత్రం);
        UASSERT_SELFTEST(scheduled.కొలత(), 2);
        UASSERT_SELFTEST(scheduled[0].m_దారాలు.కొలత(), hier_దారాలు / 2);
        UASSERT_SELFTEST(scheduled[0].m_దారాలు[0].కొలత(), 1);
        కొరకు (కొలత_t i = 1; i < scheduled[0].m_దారాలు.కొలత(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_దారాలు[i].కొలత(), 0);

        UASSERT_SELFTEST(scheduled[0].m_దారాలు[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_దారాలు.కొలత(), దారాలు);
        UASSERT_SELFTEST(scheduled[1].m_దారాలు[0].కొలత(), 1);
        కొరకు (కొలత_t i = 1; i < scheduled[1].m_దారాలు.కొలత(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_దారాలు[i].కొలత(), 0);
        UASSERT_SELFTEST(scheduled[1].m_దారాలు[0][0], t1);

        UASSERT_SELFTEST(Thread_కాలపట్టిక::s_mtaskState.కొలత(), 2);

        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_కాలపట్టిక::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_సమయం(scheduled[1], t1, 1), 1130);

        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) vtx.as<ExecMTask>()->funcp()->delete_వృక్షం();
        VL_DO_DANGLING(execGraphp->delete_వృక్షం(), execGraphp);
        Thread_కాలపట్టిక::s_mtaskState.clear();
    }

    స్థిర std::సదిశ<Thread_కాలపట్టిక> apply(V3_లేఖాచిత్రం& mtask_లేఖాచిత్రం) {
        తిరిగి Pack_దారాలు{}.pack(mtask_లేఖాచిత్రం);
    }
};

ఉపయోగించి EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
ఉపయోగించి Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

శూన్య normalizeCosts(Costs& costs) {
    స్థిరరాశి స్వయంచాలక scale_ఖర్చు = [](uint64_t విలువ, ద్విగుణ multiplier) {
        ద్విగుణ scaled = static_cast<ద్విగుణ>(విలువ) * multiplier;
        ఒకవేళ (విలువ && scaled < 1) scaled = 1;
        తిరిగి static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t కూడిక_ఖర్చు_Profiled = 0;  // For data with estimate and profile
    uint64_t కూడిక_ఖర్చు_Estimate = 0;  // For data with estimate and profile
    కొరకు (స్థిరరాశి స్వయంచాలక& est : costs) {
        ఒకవేళ (est.రెండవ.రెండవ) {
            కూడిక_ఖర్చు_Estimate += est.రెండవ.మొదటి;
            కూడిక_ఖర్చు_Profiled += est.రెండవ.రెండవ;
        }
    }

    ఒకవేళ (కూడిక_ఖర్చు_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        స్థిరరాశి ద్విగుణ est_గమ్యం_Profile
            = static_cast<ద్విగుణ>(కూడిక_ఖర్చు_Profiled) / static_cast<ద్విగుణ>(కూడిక_ఖర్చు_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_గమ్యం_Profile
                                                    << ", sumCostProfiled=" << కూడిక_ఖర్చు_Profiled
                                                    << " sumCostEstimate=" << కూడిక_ఖర్చు_Estimate);
        కొరకు (స్వయంచాలక& est : costs) {
            uint64_t& ఖర్చు_Estimate = est.రెండవ.మొదటి;
            ఖర్చు_Estimate = scale_ఖర్చు(ఖర్చు_Estimate, est_గమ్యం_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_ఖర్చు = 0;
    కొరకు (స్వయంచాలక& est : costs) {
        స్థిరరాశి uint64_t& ఖర్చు_Estimate = est.రెండవ.మొదటి;
        స్థిరరాశి uint64_t& ఖర్చు_Profiled = est.రెండవ.రెండవ;
        ఒకవేళ (max_ఖర్చు < ఖర్చు_Estimate) max_ఖర్చు = ఖర్చు_Estimate;
        ఒకవేళ (max_ఖర్చు < ఖర్చు_Profiled) max_ఖర్చు = ఖర్చు_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.రెండవ.మొదటి << " cp=" << est.రెండవ.రెండవ);
    }
    constexpr uint64_t scale_కిందికి_గమ్యం = 10000000;  // Extra room for future algorithms to add costs
    ఒకవేళ (max_ఖర్చు > scale_కిందికి_గమ్యం) {
        స్థిరరాశి ద్విగుణ scaleup = static_cast<ద్విగుణ>(scale_కిందికి_గమ్యం) / static_cast<ద్విగుణ>(max_ఖర్చు);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_ఖర్చు);
        కొరకు (స్వయంచాలక& est : costs) {
            est.రెండవ.మొదటి = scale_ఖర్చు(est.రెండవ.మొదటి, scaleup);
            est.రెండవ.రెండవ = scale_ఖర్చు(est.రెండవ.రెండవ, scaleup);
        }
    }
}

శూన్య remove_ఖాళీలు_MTasks(Ast_Exec_లేఖాచిత్రం* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    కొరకు (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        ఒకవేళ (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_వెనుక()->delete_వృక్షం(), callp);
    }
    V3_లేఖాచిత్రం* స్థిరరాశి execMTaskGraphp = execGraphp->depGraphp();
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* స్థిరరాశి mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* స్థిరరాశి funcp = mtaskp->funcp();
        ఒకవేళ (funcp->stmtsp()) కొనసాగు;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_అంచులు(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_వెనుక()->delete_వృక్షం(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
}

శూన్య fillinCosts(V3_లేఖాచిత్రం* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* స్థిరరాశి mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        స్థిరరాశి uint64_t ఖర్చు_Estimate = V3_Instr_లెక్క::లెక్క(mtp->funcp(), అబద్ధం);
        స్థిరరాశి uint64_t ఖర్చు_Profiled
            = V3Control::get_Profile_దత్తాంశం(v3Global.opt.ఉపసర్గ(), mtp->hashName());
        ఒకవేళ (ఖర్చు_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << ఖర్చు_Profiled);
        }
        costs[mtp->id()] = std::make_pair(ఖర్చు_Estimate, ఖర్చు_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    పూర్ణాంకం మొత్తం_Estimates = 0;
    పూర్ణాంకం missingProfiles = 0;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* స్థిరరాశి mtp = vtx.as<ExecMTask>();
        స్థిరరాశి uint32_t ఖర్చు_Estimate = costs[mtp->id()].మొదటి;
        స్థిరరాశి uint64_t ఖర్చు_Profiled = costs[mtp->id()].రెండవ;
        UINFO(9, "ce = " << ఖర్చు_Estimate << " cp=" << ఖర్చు_Profiled);
        UASSERT(ఖర్చు_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(ఖర్చు_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        స్థిరరాశి uint64_t ఖర్చు_Profiled32 = static_cast<uint32_t>(ఖర్చు_Profiled);
        uint32_t ఖర్చు_గమ్యం_Use = ఖర్చు_Profiled32;
        ఒకవేళ (!ఖర్చు_Profiled32) {
            ఖర్చు_గమ్యం_Use = ఖర్చు_Estimate;
            ఒకవేళ (ఖర్చు_Estimate != 0) ++missingProfiles;
        }
        ఒకవేళ (ఖర్చు_Estimate != 0) ++మొత్తం_Estimates;
        mtp->ఖర్చు(ఖర్చు_గమ్యం_Use);
        mtp->ప్రాధాన్యత_2(ఖర్చు_గమ్యం_Use);
    }

    ఒకవేళ (missingProfiles) {
        ఒకవేళ (File_పంక్తి* స్థిరరాశి fl = V3Control::get_Profile_దత్తాంశం_File_పంక్తి()) {
            ఒకవేళ (V3Control::కలిగియుంది_MTask_Profile_దత్తాంశం()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << మొత్తం_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

శూన్య finalizeCosts(V3_లేఖాచిత్రం* execMTaskGraphp) {
    లేఖాచిత్రం_ప్రవాహం_Unordered ser(execMTaskGraphp, లేఖాచిత్రం_Way::తిరగేయి_2);
    అయితే (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vxp = ser.nextp()) {
        ExecMTask* స్థిరరాశి mtp = const_cast<V3_లేఖాచిత్రం_శీర్షం*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mtp->బహిర్గతం_అంచులు()) {
            స్థిరరాశి ExecMTask* స్థిరరాశి followp = అంచు.శిఖరం()->as<ExecMTask>();
            ఒకవేళ ((followp->ప్రాధాన్యత_2() + mtp->ఖర్చు()) > mtp->ప్రాధాన్యత_2()) {
                mtp->ప్రాధాన్యత_2(followp->ప్రాధాన్యత_2() + mtp->ఖర్చు());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_సంక్రమణ_అంచులు();

    // Record summary stats for final m_tasks graph.
    స్థిరరాశి స్వయంచాలక report = execMTaskGraphp->parallelismReport([](స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* vtxp) {  //
        తిరిగి vtxp->as<స్థిరరాశి ExecMTask>()->ఖర్చు();
    });
    V3Stats::కూడు_Stat("MTask graph, final, critical path cost", report.critical_మార్గం_ఖర్చు());
    V3Stats::కూడు_Stat("MTask graph, final, total graph cost", report.మొత్తం_లేఖాచిత్రం_ఖర్చు());
    V3Stats::కూడు_Stat("MTask graph, final, mtask count", report.శీర్షం_లెక్క());
    V3Stats::కూడు_Stat("MTask graph, final, edge count", report.అంచు_లెక్క());
    V3Stats::కూడు_Stat("MTask graph, final, parallelism factor", report.parallelism_గుణకం());
    ఒకవేళ (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_మార్గం_ఖర్చు());
        UINFO(0, "    Total graph cost = " << report.మొత్తం_లేఖాచిత్రం_ఖర్చు());
        UINFO(0, "    MTask vertex count = " << report.శీర్షం_లెక్క());
        UINFO(0, "    Edge count = " << report.అంచు_లెక్క());
        UINFO(0, "    Parallelism factor = " << report.parallelism_గుణకం());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(స్థిరరాశి std::string& name, uint32_t nDependencies) {
    Ast_కణుపు_Module* స్థిరరాశి modp = v3Global.rootp()->శిఖరం_Modulep();
    File_పంక్తి* స్థిరరాశి fl = modp->fileline();
    AstBasicDType* స్థిరరాశి dtypep
        = v3Global.rootp()->typeTablep()->కనుగొను_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* స్థిరరాశి varp = కొత్త AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(నిజం);
    varp->valuep(కొత్త AstConst{fl, nDependencies});
    modp->కూడు_Stmtsp(varp);
    తిరిగి varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, స్థిరరాశి std::string& పద్ధతి) {
    File_పంక్తి* స్థిరరాశి fl = varp->fileline();
    AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    స్థిరరాశి AstScope* స్థిరరాశి శిఖరం_Scopep = v3Global.rootp()->శిఖరం_Scopep()->scopep();
    refp->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::VlSyms{}, శిఖరం_Scopep->nameDotless()});
    AstCStmt* స్థిరరాశి stmtp = కొత్త AstCStmt{fl};
    stmtp->కూడు(refp);
    stmtp->కూడు("." + పద్ధతి + ";");
    తిరిగి stmtp;
}

శూన్య కూడు_MTask_గమ్యం_Function(స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక, స్థిరరాశి uint32_t threadId, AstCFunc* funcp,
                        స్థిరరాశి ExecMTask* mtaskp,
                        స్థిరరాశి std::unordered_map<స్థిరరాశి ExecMTask*, AstVar*>& stateVars) {
    AstScope* స్థిరరాశి scopep = v3Global.rootp()->శిఖరం_Scopep()->scopep();
    Ast_కణుపు_Module* స్థిరరాశి modp = v3Global.rootp()->శిఖరం_Modulep();
    File_పంక్తి* స్థిరరాశి fl = modp->fileline();

    // Helper function to make the code a bit more legible
    స్థిరరాశి స్వయంచాలక కూడు_CStmt = [=](స్థిరరాశి string& stmt) -> శూన్య {  //
        funcp->కూడు_Stmtsp(కొత్త AstCStmt{fl, stmt});
    };

    ఒకవేళ (కాలపట్టిక.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        ఒకవేళ (v3Global.opt.profExec()) {
            కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->కూడు_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        ఒకవేళ (v3Global.opt.profExec()) {
            కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    ఒకవేళ (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        కూడు_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::గమ్యం_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{fl, mtaskp->funcp()};
    callp->self_సూచిక(VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::VlSyms{}, scopep->nameDotless()});
    callp->dtype_సమితి_Void();
    funcp->కూడు_Stmtsp(callp->makeStmt());

    ఒకవేళ (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        కూడు_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::గమ్యం_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->బహిర్గతం_అంచులు()) {
        స్థిరరాశి ExecMTask* స్థిరరాశి nextp = అంచు.శిఖరం()->as<ExecMTask>();
        ఒకవేళ (కాలపట్టిక.threadId(nextp) != threadId && కాలపట్టిక.కలిగియుంది(nextp)) {
            funcp->కూడు_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

స్థిరరాశి std::సదిశ<AstCFunc*> createThreadFunctions(స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక,
                                                   స్థిరరాశి string& ట్యాగ్) {
    Ast_కణుపు_Module* స్థిరరాశి modp = v3Global.rootp()->శిఖరం_Modulep();
    File_పంక్తి* స్థిరరాశి fl = modp->fileline();

    std::సదిశ<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<స్థిరరాశి ExecMTask*, AstVar*> stateVars;
    కొరకు (స్థిరరాశి std::సదిశ<స్థిరరాశి ExecMTask*>& thread : కాలపట్టిక.m_దారాలు) {
        కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp : thread) {
            ఒకవేళ (స్థిరరాశి uint32_t nDependencies = కాలపట్టిక.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_గమ్యం_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    కొరకు (స్థిరరాశి std::సదిశ<స్థిరరాశి ExecMTask*>& thread : కాలపట్టిక.m_దారాలు) {
        ఒకవేళ (thread.ఖాళీలు()) కొనసాగు;
        స్థిరరాశి uint32_t threadId = కాలపట్టిక.threadId(thread.front());
        స్థిరరాశి string name{"__Vthread__" + ట్యాగ్ + "__s" + cvt_గమ్యం_Str(కాలపట్టిక.id()) + "__t"
                          + cvt_గమ్యం_Str(threadId)};
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{fl, name, శూన్యసూచిక, "void"};
        modp->కూడు_Stmtsp(funcp);
        funcps.నెట్టు_వెనుక(funcp);
        funcp->isStatic(నిజం);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(నిజం);
        funcp->నమోదు_బిందువు(నిజం);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->కూడు_Stmtsp(కొత్త AstCStmt{fl, EmitCUtil::void_Self_కేటాయించు(modp)});
        funcp->కూడు_Stmtsp(కొత్త AstCStmt{fl, EmitCUtil::sym_Class_కేటాయించు()});

        // Invoke each mtask scheduled to this thread from the thread function
        కొరకు (స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp : thread) {
            కూడు_MTask_గమ్యం_Function(కాలపట్టిక, threadId, funcp, mtaskp, stateVars);
        }
    }

    తిరిగి funcps;
}

శూన్య కూడు_Thread_ప్రారంభం_Wrapper(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    // FileLine used for constructing nodes below
    File_పంక్తి* స్థిరరాశి fl = v3Global.rootp()->fileline();
    స్థిరరాశి string& ట్యాగ్ = execGraphp->name();

    // Add thread function invocations to execGraph
    స్థిరరాశి స్వయంచాలక కూడు_CStmt = [=](స్థిరరాశి string& stmt) -> శూన్య {  //
        execGraphp->runfuncp()->కూడు_Stmtsp(కొత్త AstCStmt{fl, stmt});
    };

    ఒకవేళ (v3Global.opt.profExec()) {
        కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    కూడు_CStmt("vlSymsp->__Vm_even_cycle__" + ట్యాగ్ + " = !vlSymsp->__Vm_even_cycle__" + ట్యాగ్ + ";");

    ఒకవేళ (!v3Global.opt.hier_ఖండాలు().ఖాళీలు()) కూడు_CStmt("std::vector<size_t> indexes;");
}

శూన్య కూడు_Thread_చివర_Wrapper(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    // Add thread function invocations to execGraph
    స్థిరరాశి స్వయంచాలక కూడు_CStmt = [=](స్థిరరాశి string& stmt) -> శూన్య {  //
        File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->కూడు_Stmtsp(కొత్త AstCStmt{flp, stmt});
    };

    కూడు_CStmt("Verilated::mtaskId(0);");
    ఒకవేళ (v3Global.opt.profExec()) { కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
శూన్య కూడు_Thread_ప్రారంభం_గమ్యం_Exec_లేఖాచిత్రం(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp,
                               స్థిరరాశి std::సదిశ<AstCFunc*>& funcps, uint32_t కాలపట్టిక_Id) {
    // FileLine used for constructing nodes below
    File_పంక్తి* స్థిరరాశి fl = v3Global.rootp()->fileline();
    స్థిరరాశి string& ట్యాగ్ = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* స్థిరరాశి finalVarp
        = newMTaskStateVar("final__" + cvt_గమ్యం_Str(కాలపట్టిక_Id) + "__" + ట్యాగ్, funcps.కొలత());
    కొరకు (AstCFunc* స్థిరరాశి funcp : funcps) {
        funcp->కూడు_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    స్థిరరాశి స్వయంచాలక కూడు_CStmt = [=](స్థిరరాశి string& stmt) -> శూన్య {  //
        execGraphp->runfuncp()->కూడు_Stmtsp(కొత్త AstCStmt{fl, stmt});
    };

    స్థిరరాశి uint32_t చివరి = funcps.కొలత() - 1;
    ఒకవేళ (!v3Global.opt.hier_ఖండాలు().ఖాళీలు() && చివరి > 0) {
        కూడు_CStmt("for (size_t i = 0; i < " + std::గమ్యం_string(చివరి) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    కొరకు (AstCFunc* స్థిరరాశి funcp : funcps) {
        ఒకవేళ (i != చివరి) {
            // The first N-1 will run on the thread pool.
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{fl};
            execGraphp->runfuncp()->కూడు_Stmtsp(cstmtp);
            cstmtp->కూడు("vlSymsp->__Vm_threadPoolp->workerp(");
            ఒకవేళ (v3Global.opt.hier_బిడ్డ() || !v3Global.opt.hier_ఖండాలు().ఖాళీలు()) {
                cstmtp->కూడు("indexes[" + std::గమ్యం_string(i) + "]");
            } లేకపోతే {
                cstmtp->కూడు(std::గమ్యం_string(i));
            }
            cstmtp->కూడు(")->addTask(");
            cstmtp->కూడు(కొత్త Ast_Addr_యొక్క_CFunc{fl, funcp});
            cstmtp->కూడు(", vlSelf, vlSymsp->__Vm_even_cycle__" + ట్యాగ్ + ");");
        } లేకపోతే {
            // The last will run on the main thread.
            AstCCall* స్థిరరాశి callp = కొత్త AstCCall{fl, funcp};
            callp->dtype_సమితి_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + ట్యాగ్);
            execGraphp->runfuncp()->కూడు_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::కూడు_Stat_కూడిక("Optimizations, Thread schedule total tasks", i);

    ఒకవేళ (v3Global.opt.profExec()) {
        కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->కూడు_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + ట్యాగ్ + ")"));
    ఒకవేళ (v3Global.opt.profExec()) {
        కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    ఒకవేళ (!v3Global.opt.hier_ఖండాలు().ఖాళీలు() && చివరి > 0) {
        కూడు_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

శూన్య processMTaskBodies(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* స్థిరరాశి mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* స్థిరరాశి funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_కణుపు* stmtsp = funcp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();

        // Helper function to make the code a bit more legible
        స్థిరరాశి స్వయంచాలక కూడు_CStmt = [=](స్థిరరాశి string& stmt) -> శూన్య {  //
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        ఒకవేళ (v3Global.opt.profExec()) {
            std::string వాదనలు = std::గమ్యం_string(mtaskp->id());
            వాదనలు += ", " + std::గమ్యం_string(mtaskp->predict_ప్రారంభం());
            వాదనలు += ", \"";
            ఒకవేళ (v3Global.opt.hier_బిడ్డ()) వాదనలు += v3Global.opt.శిఖరం_Module();
            వాదనలు += "\"";
            కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + వాదనలు + ");");
        }
        // Set mtask ID in the run-time system
        కూడు_CStmt("Verilated::mtaskId(" + std::గమ్యం_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->కూడు_Stmtsp(stmtsp);
        // Flush message queue
        కూడు_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        ఒకవేళ (v3Global.opt.profExec()) {
            స్థిరరాశి std::string& వాదనలు = std::గమ్యం_string(mtaskp->ఖర్చు());
            కూడు_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + వాదనలు + ");");
        }
    }
}

శూన్య implement_Exec_లేఖాచిత్రం(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp, స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక) {
    // Nothing to be done if there are no MTasks in the graph at all.
    ఒకవేళ (execGraphp->depGraphp()->ఖాళీలు()) తిరిగి;

    // Create a function to be run by each thread.
    స్థిరరాశి std::సదిశ<AstCFunc*>& funcps = createThreadFunctions(కాలపట్టిక, execGraphp->name());
    UASSERT(!funcps.ఖాళీలు(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    కూడు_Thread_ప్రారంభం_గమ్యం_Exec_లేఖాచిత్రం(execGraphp, funcps, కాలపట్టిక.id());
}

శూన్య create_నిరంతరం_Function(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_పంక్తి* స్థిరరాశి flp = execGraphp->fileline();
    Ast_కణుపు_Module* స్థిరరాశి modp = v3Global.rootp()->శిఖరం_Modulep();
    AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{flp, "runExecGraph_" + execGraphp->name(), శూన్యసూచిక};
    funcp->isLoose(నిజం);
    funcp->voidSelf(నిజం);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(నిజం);
    modp->కూడు_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

శూన్య కూడు_Trigger_Masks(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_పంక్తి* స్థిరరాశి flp = execGraphp->fileline();
    AstVar* స్థిరరాశి triggersp = execGraphp->triggerp()->varp();
    కొరకు (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> మాట_Masks;
        callp->funcp()->foreach([&](స్థిరరాశి AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            కొరకు (స్థిరరాశి uint32_t సూచీ : trigp->indices()) {
                మాట_Masks[VL_BITWORD_Q(సూచీ)] |= VL_బిట్టు_Q(సూచీ);
            }
        });
        AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{flp, triggersp->dtypep(),
                                                     కొత్త AstConst{flp, AstConst::Unsized64{}, 0}};
        కొరకు (స్థిరరాశి స్వయంచాలక& మాట_ముసుగు : మాట_Masks) {
            consp->valuep(మాట_ముసుగు.మొదటి,
                          కొత్త AstConst{flp, AstConst::Unsized64{}, మాట_ముసుగు.రెండవ});
        }
        execGraphp->కూడు_Maskps(V3ConstPool::కనుగొను(consp));
        VL_DO_DANGLING(consp->delete_వృక్షం(), consp);
    }
}

శూన్య కూడు_Trigger_Costs(Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_పంక్తి* స్థిరరాశి flp = execGraphp->fileline();
    స్థిరరాశి uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::సదిశ<uint32_t> costs(nTriggers, 0);
    కొరకు (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            ఒకవేళ (స్థిరరాశి AstTriggered* స్థిరరాశి trigp = VN_CAST(ifp->condp(), Triggered)) {
                స్థిరరాశి uint32_t ఖర్చు = V3_Instr_లెక్క::లెక్క(ifp, అబద్ధం);
                కొరకు (స్థిరరాశి uint32_t సూచీ : trigp->indices()) costs.at(సూచీ) += ఖర్చు;
            }
        });
    }
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    AstUnpackArrayDType* స్థిరరాశి dtypep
        = కొత్త AstUnpackArrayDType{flp, netlistp->కనుగొను_బిట్టు_DType(32, 32, VSigning::UNSIGNED),
                                  కొత్త AstRange{flp, static_cast<పూర్ణాంకం>(nTriggers - 1), 0}};
    netlistp->typeTablep()->కూడు_Typesp(dtypep);
    AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{flp, dtypep, కొత్త AstConst{flp, 0}};
    కొరకు (uint32_t సూచీ = 0; సూచీ < nTriggers; ++సూచీ) {
        ఒకవేళ (costs[సూచీ]) consp->valuep(సూచీ, కొత్త AstConst{flp, costs[సూచీ]});
    }
    execGraphp->triggerCostsp(V3ConstPool::కనుగొను(consp));
    VL_DO_DANGLING(consp->delete_వృక్షం(), consp);
}

// Called by Verilator top stage
శూన్య implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::సదిశ<Ast_Exec_లేఖాచిత్రం*> execGraphps;
    netlistp->శిఖరం_Modulep()->foreach([&](Ast_Exec_లేఖాచిత్రం* egp) { execGraphps.emplace_వెనుక(egp); });

    // Process each
    కొరకు (Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_ఖాళీలు_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        ఒకవేళ (execGraphp->depGraphp()->ఖాళీలు()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_వెనుక()->delete_వృక్షం(), execGraphp);
            తిరిగి;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_నిరంతరం_Function(execGraphp);
        కూడు_Thread_ప్రారంభం_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        స్థిరరాశి std::సదిశ<Thread_కాలపట్టిక> packed = Pack_దారాలు::apply(*execGraphp->depGraphp());
        V3Stats::కూడు_Stat_కూడిక("Optimizations, Thread schedule count",
                            static_cast<ద్విగుణ>(packed.కొలత()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        కొరకు (స్థిరరాశి Thread_కాలపట్టిక& కాలపట్టిక : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_లేఖాచిత్రం(execGraphp, కాలపట్టిక);
        }

        కూడు_Thread_చివర_Wrapper(execGraphp);

        కూడు_Trigger_Masks(execGraphp);
        కూడు_Trigger_Costs(execGraphp);
    }
}

శూన్య selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].మొదటి, 1000);
        UASSERT_SELFTEST(costs[1].రెండవ, 1000);
        UASSERT_SELFTEST(costs[2].మొదటి, 2000);
        UASSERT_SELFTEST(costs[2].రెండవ, 0);
        UASSERT_SELFTEST(costs[3].మొదటి, 3000);
        UASSERT_SELFTEST(costs[3].రెండవ, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].మొదటి, 2500000);
        UASSERT_SELFTEST(costs[1].రెండవ, 5000000);
        UASSERT_SELFTEST(costs[2].మొదటి, 5000000);
        UASSERT_SELFTEST(costs[2].రెండవ, 10000000);
        UASSERT_SELFTEST(costs[3].మొదటి, 7500000);
        UASSERT_SELFTEST(costs[3].రెండవ, 1);
    }

    Pack_దారాలు::selfTest();
}

}  // namespace V3ExecGraph
