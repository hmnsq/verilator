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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphStream.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3InstrCount.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(Ast_Exec_ਗ੍ਰਾਫ* execGraphp, AstScope* scopep, Ast_ਗੰਢ_Stmt* stmtsp,
                                 uint32_t id) {
    ਅਚਲ std::string newName = execGraphp->name() + "_mtask" + std::ਮੰਜ਼ਿਲ_string(id);
    AstCFunc* ਅਚਲ newp = ਨਵਾਂ AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(ਸੱਚ);
    newp->voidSelf(ਸੱਚ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(ਸੱਚ);
    newp->ਜੋੜੋ_Stmtsp(stmtsp);
    ਜੇ (scopep) scopep->ਜੋੜੋ_Blocksp(newp);
    ਵਾਪਸ newp;
}

ExecMTask::ExecMTask(Ast_Exec_ਗ੍ਰਾਫ* execGraphp, AstScope* scopep,
                     Ast_ਗੰਢ_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{execGraphp->depGraphp()},
      m_id{s_ਅਗਲਾ_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).ਮੰਜ਼ਿਲ_String()} {}

ਖਾਲੀ ExecMTask::dump(std::ostream& str) ਅਚਲ {
    str << name() << "." << cvt_ਮੰਜ਼ਿਲ_Hex(ਇਹ);
    ਜੇ (ਤਰਜੀਹ_2() || ਲਾਗਤ()) str << " [pr=" << ਤਰਜੀਹ_2() << " c=" << cvt_ਮੰਜ਼ਿਲ_Str(ਲਾਗਤ()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_ਅਗਲਾ_Id{0};

ਨਾਮਾਕਾਸ਼ V3_Exec_ਗ੍ਰਾਫ {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
ਵਰਗ Thread_ਸਮਾਂ_ਸਾਰਣੀ final {
    ਮਿੱਤਰ ਵਰਗ Pack_ਧਾਗੇ;

    uint32_t m_id;  // Unique ID of a schedule
    ਸਥਿਰ uint32_t s_ਅਗਲਾ_Id;  // Next ID number to use
    std::unordered_ਸੈੱਟ<ਅਚਲ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_ਸਿਰਾ_ਸਮਾਂ = 0;  // Latest task end time in this schedule

ਜਨਤਕ:
    // CONSTANTS
    ਸਥਿਰ constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ਬਣਤਰ MTaskState final {
        uint32_t completion_ਸਮਾਂ = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ਅਚਲ ExecMTask* nextp = ਖਾਲੀ_ਸੂਚਕ;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>> m_ਧਾਗੇ;

    // Global state for each mtask.
    ਸਥਿਰ std::unordered_map<ਅਚਲ ExecMTask*, MTaskState> s_mtaskState;

    ਸਪਸ਼ਟ Thread_ਸਮਾਂ_ਸਾਰਣੀ(uint32_t n_ਧਾਗੇ)
        : m_id{s_ਅਗਲਾ_Id++}
        , m_ਧਾਗੇ{n_ਧਾਗੇ} {}
    Thread_ਸਮਾਂ_ਸਾਰਣੀ(Thread_ਸਮਾਂ_ਸਾਰਣੀ&&) = ਮੂਲ;
    Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸੰਚਾਲਕ=(Thread_ਸਮਾਂ_ਸਾਰਣੀ&&) = ਮੂਲ;

ਨਿਜੀ:
    VL_UNCOPYABLE(Thread_ਸਮਾਂ_ਸਾਰਣੀ);

    ਸਥਿਰ constexpr ਦੁੱਗਣਾ s_thread_Box_ਚੌੜਾਈ = 2.5;
    ਸਥਿਰ constexpr ਦੁੱਗਣਾ s_thread_Box_ਉਚਾਈ = 1.5;
    ਸਥਿਰ constexpr ਦੁੱਗਣਾ s_horizontalGap = s_thread_Box_ਚੌੜਾਈ / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ਸਥਿਰ ਖਾਲੀ dumpDotFilePrefixedAlways(ਅਚਲ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ>& schedules,
                                          ਅਚਲ string& nameComment, uint32_t n_ਧਾਗੇ) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_ਧਾਗੇ);
    }
    ਸਥਿਰ ਖਾਲੀ dumpDotFile(ਅਚਲ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ>& schedules, ਅਚਲ string& filename,
                            uint32_t n_ਧਾਗੇ) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ਅਚਲ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        ਜੇ (logp->fail()) v3fatal("Can't write file: " << filename);

        //ਸਿਰਲੇਖ
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        ਲਈ (uint32_t i = 0; i < n_ਧਾਗੇ; ++i) {
            ਅਚਲ string name = "t" + std::ਮੰਜ਼ਿਲ_string(i);
            ਅਚਲ string label = "Thread " + std::ਮੰਜ਼ਿਲ_string(i);
            constexpr ਦੁੱਗਣਾ posX = -s_horizontalGap;
            ਅਚਲ ਦੁੱਗਣਾ posY = -static_cast<ਦੁੱਗਣਾ>(i) * s_thread_Box_ਉਚਾਈ;
            dump_Dot_File_Emit_ਖੰਡ(logp, name, label, s_thread_Box_ਚੌੜਾਈ, s_thread_Box_ਉਚਾਈ, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_ਲਾਗਤ = 0;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& state : Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState) {
            ਅਚਲ ExecMTask* ਅਚਲ mtaskp = state.ਪਹਿਲਾ;
            max_ਲਾਗਤ = std::max(max_ਲਾਗਤ, mtaskp->ਲਾਗਤ());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_ਪ੍ਰਤੀ_Longest_Mtask = 4;
        ਅਚਲ uint32_t segment_ਲਾਗਤ = max_ਲਾਗਤ / segments_ਪ੍ਰਤੀ_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::ਸਦਿਸ਼<ਦੁੱਗਣਾ> offsets(n_ਧਾਗੇ, 0.0);
        ਲਈ (ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ : schedules) {
            ਜੇ (ਸਮਾਂ_ਸਾਰਣੀ.mtasks.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਵਰਤੋਂ ਕਾਲਮ = std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>;
            std::ਸਦਿਸ਼<ਕਾਲਮ> columns = {{}};

            // Order tasks based on their start time
            ਬਣਤਰ Cmp final {
                ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ExecMTask* ਅਚਲ a, ਅਚਲ ExecMTask* ਅਚਲ b) ਅਚਲ {
                    ਜੇ (ਸ਼ੁਰੂ_ਸਮਾਂ(a) == ਸ਼ੁਰੂ_ਸਮਾਂ(b)) ਵਾਪਸ threadId(a) < threadId(b);
                    ਵਾਪਸ ਸ਼ੁਰੂ_ਸਮਾਂ(a) < ਸ਼ੁਰੂ_ਸਮਾਂ(b);
                }
            };
            ਅਚਲ std::multiset<ਅਚਲ ExecMTask*, Cmp> tasks(ਸਮਾਂ_ਸਾਰਣੀ.mtasks.begin(),
                                                             ਸਮਾਂ_ਸਾਰਣੀ.mtasks.ਸਿਰਾ());

            ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ mtaskp : tasks) {
                ਕਾਲਮ& ਕਾਲਮ_2 = columns.ਪਿੱਠ();
                UASSERT(ਕਾਲਮ_2.ਮਾਪ() <= n_ਧਾਗੇ, "Invalid partitioning");

                ਤਾਰਕਿਕ intersects = ਸੱਚ;
                ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ earlierMtask : ਕਾਲਮ_2) {
                    ਜੇ (ਸਿਰਾ_ਸਮਾਂ(mtaskp) <= ਸ਼ੁਰੂ_ਸਮਾਂ(earlierMtask)
                        || ਸ਼ੁਰੂ_ਸਮਾਂ(mtaskp) >= ਸਿਰਾ_ਸਮਾਂ(earlierMtask)) {
                        intersects = ਝੂਠ;
                        ਰੋਕੋ;
                    }
                }
                ਜੇ (intersects) {
                    ਕਾਲਮ_2.emplace_ਪਿੱਠ(mtaskp);
                } ਨਹੀਂਤਾਂ {
                    columns.emplace_ਪਿੱਠ(ਕਾਲਮ{mtaskp});
                }
            }

            UASSERT(!columns.front().ਖਾਲੀ_ਥਾਵਾਂ(), "Should be populated by mtasks");

            ਲਈ (ਅਚਲ ਕਾਲਮ& ਕਾਲਮ_2 : columns) {
                ਦੁੱਗਣਾ ਆਖਰੀ_ਕਾਲਮ_ਵਿੱਥ = 0;
                ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ mtaskp : ਕਾਲਮ_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, ਸਮਾਂ_ਸਾਰਣੀ, segment_ਲਾਗਤ, offsets);
                    ਆਖਰੀ_ਕਾਲਮ_ਵਿੱਥ = std::max(ਆਖਰੀ_ਕਾਲਮ_ਵਿੱਥ, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.ਸਿਰਾ(), ਆਖਰੀ_ਕਾਲਮ_ਵਿੱਥ);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_ਧਾਗੇ);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>& thread : ਸਮਾਂ_ਸਾਰਣੀ.m_ਧਾਗੇ) {
                ਜੇ (thread.ਖਾਲੀ_ਥਾਵਾਂ()) ਰੋਕੋ;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.ਪਿੱਠ()->name() << " -> fork_"
                      << static_cast<ਪੂਰਨ>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                ਲਈ (ਮਾਪ_t i = 1; i < thread.ਮਾਪ(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ mtaskp : thread) {
                    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                        ਅਚਲ ExecMTask* ਅਚਲ ਸਿਖਰ_MTaskp = ਕਿਨਾਰਾ.ਸਿਖਰ()->cast<ਅਚਲ ExecMTask>();
                        ਜੇ (ਸਿਖਰ_MTaskp && ਸਮਾਂ_ਸਾਰਣੀ.ਸ਼ਾਮਲ_ਹੈ(ਸਿਖਰ_MTaskp)
                            && threadId(ਸਿਖਰ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << ਸਿਖਰ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ਸਥਿਰ ਖਾਲੀ dump_Dot_File_Emit_ਖੰਡ(ਅਚਲ std::unique_ptr<std::ofstream>& logp,
                                     ਅਚਲ string& name, ਅਚਲ string& label, ਦੁੱਗਣਾ ਚੌੜਾਈ,
                                     ਦੁੱਗਣਾ ਉਚਾਈ, ਦੁੱਗਣਾ xPos, ਦੁੱਗਣਾ yPos,
                                     ਅਚਲ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << ਚੌੜਾਈ
              << " height=" << ਉਚਾਈ << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ਸਥਿਰ ਖਾਲੀ dumpDotFileEmitMTask(ਅਚਲ std::unique_ptr<std::ofstream>& logp,
                                     ਅਚਲ ExecMTask* ਅਚਲ mtaskp, ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ,
                                     uint32_t segment_ਲਾਗਤ, std::ਸਦਿਸ਼<ਦੁੱਗਣਾ>& offsets) {
        ਲਈ (ਪੂਰਨ i = 0; i < mtaskp->ਧਾਗੇ(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ਅਚਲ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::ਮੰਜ਼ਿਲ_string(i);
            ਅਚਲ string label = mtaskp->name() + " (" + std::ਮੰਜ਼ਿਲ_string(ਸ਼ੁਰੂ_ਸਮਾਂ(mtaskp)) + ':'
                                 + std::ਮੰਜ਼ਿਲ_string(ਸਿਰਾ_ਸਮਾਂ(mtaskp)) + ')'
                                 + "\\ncost=" + std::ਮੰਜ਼ਿਲ_string(mtaskp->ਲਾਗਤ())
                                 + "\\npriority=" + std::ਮੰਜ਼ਿਲ_string(mtaskp->ਤਰਜੀਹ_2());
            ਅਚਲ ਦੁੱਗਣਾ ਚੌੜਾਈ
                = std::max(s_thread_Box_ਚੌੜਾਈ,
                           s_thread_Box_ਚੌੜਾਈ * static_cast<ਦੁੱਗਣਾ>(mtaskp->ਲਾਗਤ()) / segment_ਲਾਗਤ);
            ਅਚਲ uint32_t mtaskThreadId = threadId(mtaskp) + i * ਸਮਾਂ_ਸਾਰਣੀ.m_ਧਾਗੇ.ਮਾਪ();
            ਅਚਲ ਦੁੱਗਣਾ xPos = ਚੌੜਾਈ / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += ਚੌੜਾਈ + s_horizontalGap;
            ਅਚਲ ਦੁੱਗਣਾ yPos = -s_thread_Box_ਉਚਾਈ * static_cast<ਦੁੱਗਣਾ>(mtaskThreadId);
            ਅਚਲ string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_ਖੰਡ(logp, name, label, ਚੌੜਾਈ, s_thread_Box_ਉਚਾਈ, xPos, yPos,
                                 fillColor);
        }
    }

    ਸਥਿਰ ਖਾਲੀ dumpDotFileEmitFork(ਅਚਲ std::unique_ptr<std::ofstream>& logp, ਦੁੱਗਣਾ ਵਿੱਥ,
                                    uint32_t n_ਧਾਗੇ) {
        ਅਚਲ string& name = "fork_" + std::ਮੰਜ਼ਿਲ_string(static_cast<ਪੂਰਨ>(ਵਿੱਥ));
        constexpr ਦੁੱਗਣਾ ਚੌੜਾਈ = s_thread_Box_ਚੌੜਾਈ / 8;
        ਅਚਲ ਦੁੱਗਣਾ ਉਚਾਈ = s_thread_Box_ਉਚਾਈ * n_ਧਾਗੇ;
        ਅਚਲ ਦੁੱਗਣਾ xPos = ਵਿੱਥ - s_horizontalGap / 2;
        ਅਚਲ ਦੁੱਗਣਾ yPos
            = -static_cast<ਦੁੱਗਣਾ>(n_ਧਾਗੇ) / 2 * s_thread_Box_ਉਚਾਈ + s_thread_Box_ਉਚਾਈ / 2;
        dump_Dot_File_Emit_ਖੰਡ(logp, name, "", ਚੌੜਾਈ, ਉਚਾਈ, xPos, yPos, "black");
    }

ਜਨਤਕ:
    ਸਥਿਰ uint32_t threadId(ਅਚਲ ExecMTask* mtaskp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ& it = s_mtaskState.ਲੱਭੋ(mtaskp);
        ਵਾਪਸ it != s_mtaskState.ਸਿਰਾ() ? it->ਦੂਜਾ.threadId : UNASSIGNED;
    }
    ਸਥਿਰ uint32_t ਸ਼ੁਰੂ_ਸਮਾਂ(ਅਚਲ ExecMTask* mtaskp) {
        ਵਾਪਸ s_mtaskState.at(mtaskp).completion_ਸਮਾਂ - mtaskp->ਲਾਗਤ();
    }
    ਸਥਿਰ uint32_t ਸਿਰਾ_ਸਮਾਂ(ਅਚਲ ExecMTask* mtaskp) {
        ਵਾਪਸ s_mtaskState.at(mtaskp).completion_ਸਮਾਂ;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ਅਚਲ ExecMTask* mtaskp) ਅਚਲ {
        ਅਚਲ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t ਨਤੀਜਾ = 0;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਅਚਲ ExecMTask* ਅਚਲ prevp = ਕਿਨਾਰਾ.fromp()->as<ExecMTask>();
            ਜੇ (threadId(prevp) != thisThreadId && ਸ਼ਾਮਲ_ਹੈ(prevp)) ++ਨਤੀਜਾ;
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    uint32_t id() ਅਚਲ { ਵਾਪਸ m_id; }
    uint32_t ਸਮਾਂ_ਸਾਰਣੀ_On(ਅਚਲ ExecMTask* mtaskp, uint32_t ਉੱਤਮ_Thread_Id) {
        mtasks.emplace(mtaskp);
        ਅਚਲ uint32_t ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ = mtaskp->predict_ਸ਼ੁਰੂ() + mtaskp->ਲਾਗਤ();
        m_ਸਿਰਾ_ਸਮਾਂ = std::max(m_ਸਿਰਾ_ਸਮਾਂ, ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ);
        s_mtaskState[mtaskp].completion_ਸਮਾਂ = ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ;
        s_mtaskState[mtaskp].threadId = ਉੱਤਮ_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>& ਉੱਤਮ_Thread = m_ਧਾਗੇ[ਉੱਤਮ_Thread_Id];
        ਜੇ (!ਉੱਤਮ_Thread.ਖਾਲੀ_ਥਾਵਾਂ()) s_mtaskState[ਉੱਤਮ_Thread.ਪਿੱਠ()].nextp = mtaskp;

        // Add the MTask to the schedule
        ਉੱਤਮ_Thread.ਧੱਕੋ_ਪਿੱਠ(mtaskp);
        ਵਾਪਸ ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ;
    }
    ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ(ਅਚਲ ExecMTask* mtaskp) ਅਚਲ { ਵਾਪਸ mtasks.ਗਿਣਤੀ_2(mtaskp); }
    uint32_t ਸਿਰਾ_ਸਮਾਂ() ਅਚਲ { ਵਾਪਸ m_ਸਿਰਾ_ਸਮਾਂ; }
};

uint32_t Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_ਅਗਲਾ_Id = 0;
std::unordered_map<ਅਚਲ ExecMTask*, Thread_ਸਮਾਂ_ਸਾਰਣੀ::MTaskState> Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState{};
constexpr ਦੁੱਗਣਾ V3_Exec_ਗ੍ਰਾਫ::Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_thread_Box_ਚੌੜਾਈ;

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
ਵਰਗ Pack_ਧਾਗੇ final {
    // TYPES
    ਬਣਤਰ MTaskCmp final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ ExecMTask* ap, ਅਚਲ ExecMTask* bp) ਅਚਲ {
            ਵਾਪਸ ap->id() < bp->id();
        }
    };

    // MEMBERS
    ਅਚਲ uint32_t m_n_ਧਾਗੇ;  // Number of threads
    ਅਚਲ uint32_t m_n_Hier_ਧਾਗੇ;  // Number of threads used for hierarchical tasks
    ਅਚਲ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ਅਚਲ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Pack_ਧਾਗੇ(uint32_t n_ਧਾਗੇ = v3Global.opt.ਧਾਗੇ(),
                         uint32_t n_Hier_ਧਾਗੇ = v3Global.opt.hier_ਧਾਗੇ(),
                         ਬਿਨਾਨਿਸ਼ਾਨੀ sandbagNumerator = 30, ਬਿਨਾਨਿਸ਼ਾਨੀ sandbagDenom = 100)
        : m_n_ਧਾਗੇ{n_ਧਾਗੇ}
        , m_n_Hier_ਧਾਗੇ{n_Hier_ਧਾਗੇ}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_ਧਾਗੇ() = ਮੂਲ;
    VL_UNCOPYABLE(Pack_ਧਾਗੇ);

    // METHODS
    uint32_t completion_ਸਮਾਂ(ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ, ਅਚਲ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        ਜੇ (!ਸਮਾਂ_ਸਾਰਣੀ.ਸ਼ਾਮਲ_ਹੈ(mtaskp)) ਵਾਪਸ 0;
        ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ::MTaskState& state = ਸਮਾਂ_ਸਾਰਣੀ.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_ਸਮਾਂ_ਸਾਰਣੀ::UNASSIGNED, "Mtask should have assigned thread");
        ਜੇ (threadId == state.threadId) {
            // No overhead on same thread
            ਵਾਪਸ state.completion_ਸਮਾਂ;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_ਸਿਰਾ_ਸਮਾਂ
            = state.completion_ਸਮਾਂ + (m_sandbagNumerator * mtaskp->ਲਾਗਤ()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        ਜੇ (state.nextp) {
            ਅਚਲ uint32_t successor_ਸਿਰਾ_ਸਮਾਂ
                = completion_ਸਮਾਂ(ਸਮਾਂ_ਸਾਰਣੀ, state.nextp, state.threadId);
            ਜੇ ((sandbagged_ਸਿਰਾ_ਸਮਾਂ >= successor_ਸਿਰਾ_ਸਮਾਂ) && (successor_ਸਿਰਾ_ਸਮਾਂ > 1)) {
                sandbagged_ਸਿਰਾ_ਸਮਾਂ = successor_ਸਿਰਾ_ਸਮਾਂ - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_ਸਿਰਾ_ਸਮਾਂ);
        ਵਾਪਸ sandbagged_ਸਿਰਾ_ਸਮਾਂ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isReady(Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ, ਅਚਲ ExecMTask* mtaskp) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ_ਵਿੱਚ : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਅਚਲ ExecMTask* ਅਚਲ prevp = ਕਿਨਾਰਾ_ਵਿੱਚ.fromp()->as<ਅਚਲ ExecMTask>();
            ਜੇ (ਸਮਾਂ_ਸਾਰਣੀ.threadId(prevp) == Thread_ਸਮਾਂ_ਸਾਰਣੀ::UNASSIGNED) {
                // This predecessor is not assigned yet
                ਵਾਪਸ ਝੂਠ;
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> pack(V3_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ) {
        std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> ਨਤੀਜਾ;
        ਨਤੀਜਾ.emplace_ਪਿੱਠ(Thread_ਸਮਾਂ_ਸਾਰਣੀ{m_n_ਧਾਗੇ});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        ਗਿਣਤੀ ਵਰਗ Scheduling_ਬਹੁਲਕ : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_ਬਹੁਲਕ ਬਹੁਲਕ = Scheduling_ਬਹੁਲਕ::SCHEDULING;

        // Time each thread is occupied until
        std::ਸਦਿਸ਼<uint32_t> busyUntil(std::max(m_n_ਧਾਗੇ, m_n_Hier_ਧਾਗੇ), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::ਸੈੱਟ<ExecMTask*, MTaskCmp> readyMTasks;
        ਪੂਰਨ maxThreadWorkers = 1;

        // Build initial ready list
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : mtask_ਗ੍ਰਾਫ.vertices()) {
            ExecMTask* ਅਚਲ mtaskp = vtx.as<ExecMTask>();
            ਜੇ (isReady(ਨਤੀਜਾ.ਪਿੱਠ(), mtaskp)) readyMTasks.ਪਾਓ(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->ਧਾਗੇ());
        }

        ਜਦੋਂ (!readyMTasks.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t ਉੱਤਮ_ਸਮਾਂ = 0xffffffff;
            uint32_t ਉੱਤਮ_Thread_Id = 0;
            ExecMTask* ਉੱਤਮ_Mtaskp = ਖਾਲੀ_ਸੂਚਕ;  // Todo: const ExecMTask*
            Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ = ਨਤੀਜਾ.ਪਿੱਠ();
            ਲਈ (uint32_t threadId = 0; threadId < ਸਮਾਂ_ਸਾਰਣੀ.m_ਧਾਗੇ.ਮਾਪ(); ++threadId) {
                ਲਈ (ExecMTask* ਅਚਲ mtaskp : readyMTasks) {
                    ਜੇ (ਬਹੁਲਕ != Scheduling_ਬਹੁਲਕ::WIDE_TASK_SCHEDULING && mtaskp->ਧਾਗੇ() > 1) {
                        ਬਹੁਲਕ = Scheduling_ਬਹੁਲਕ::WIDE_TASK_DISCOVERED;
                        ਜਾਰੀ;
                    }
                    ਜੇ (ਬਹੁਲਕ == Scheduling_ਬਹੁਲਕ::WIDE_TASK_SCHEDULING && mtaskp->ਧਾਗੇ() <= 1)
                        ਜਾਰੀ;

                    uint32_t ਸਮਾਂ_Begin = busyUntil[threadId];
                    ਜੇ (ਸਮਾਂ_Begin > ਉੱਤਮ_ਸਮਾਂ) {
                        UINFO(6, "th " << threadId << " busy until " << ਸਮਾਂ_Begin
                                       << ", later than bestTime " << ਉੱਤਮ_ਸਮਾਂ
                                       << ", skipping thread.");
                        ਰੋਕੋ;
                    }
                    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                        ਅਚਲ ExecMTask* ਅਚਲ priorp = ਕਿਨਾਰਾ.fromp()->as<ExecMTask>();
                        ਅਚਲ uint32_t ਪੂਰਵ_ਸਿਰਾ_ਸਮਾਂ = completion_ਸਮਾਂ(ਸਮਾਂ_ਸਾਰਣੀ, priorp, threadId);
                        ਜੇ (ਪੂਰਵ_ਸਿਰਾ_ਸਮਾਂ > ਸਮਾਂ_Begin) ਸਮਾਂ_Begin = ਪੂਰਵ_ਸਿਰਾ_ਸਮਾਂ;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << ਸਮਾਂ_Begin
                                     << " on thread " << threadId);
                    ਜੇ ((ਸਮਾਂ_Begin < ਉੱਤਮ_ਸਮਾਂ)
                        || ((ਸਮਾਂ_Begin == ਉੱਤਮ_ਸਮਾਂ)
                            && ਉੱਤਮ_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ਤਰਜੀਹ_2() > ਉੱਤਮ_Mtaskp->ਤਰਜੀਹ_2()))) {
                        ਉੱਤਮ_ਸਮਾਂ = ਸਮਾਂ_Begin;
                        ਉੱਤਮ_Thread_Id = threadId;
                        ਉੱਤਮ_Mtaskp = mtaskp;
                    }
                }
            }

            ਅਚਲ uint32_t ਸਿਰਾ_ਸਮਾਂ = ਸਮਾਂ_ਸਾਰਣੀ.ਸਿਰਾ_ਸਮਾਂ();

            ਜੇ (!ਉੱਤਮ_Mtaskp && ਬਹੁਲਕ == Scheduling_ਬਹੁਲਕ::WIDE_TASK_DISCOVERED) {
                ਬਹੁਲਕ = Scheduling_ਬਹੁਲਕ::WIDE_TASK_SCHEDULING;
                ਅਚਲ uint32_t ਮਾਪ = m_n_Hier_ਧਾਗੇ / maxThreadWorkers;
                UASSERT(ਮਾਪ, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                ਜੇ (ਸਮਾਂ_ਸਾਰਣੀ.s_mtaskState.ਖਾਲੀ_ਥਾਵਾਂ()) ਨਤੀਜਾ.clear();
                ਨਤੀਜਾ.emplace_ਪਿੱਠ(Thread_ਸਮਾਂ_ਸਾਰਣੀ{ਮਾਪ});
                std::fill(busyUntil.begin(), busyUntil.ਸਿਰਾ(), ਸਿਰਾ_ਸਮਾਂ);
                ਜਾਰੀ;
            }

            ਜੇ (!ਉੱਤਮ_Mtaskp && ਬਹੁਲਕ == Scheduling_ਬਹੁਲਕ::WIDE_TASK_SCHEDULING) {
                ਬਹੁਲਕ = Scheduling_ਬਹੁਲਕ::SCHEDULING;
                UASSERT(!ਸਮਾਂ_ਸਾਰਣੀ.s_mtaskState.ਖਾਲੀ_ਥਾਵਾਂ(), "Mtask should be added");
                ਨਤੀਜਾ.emplace_ਪਿੱਠ(Thread_ਸਮਾਂ_ਸਾਰਣੀ{m_n_ਧਾਗੇ});
                std::fill(busyUntil.begin(), busyUntil.ਸਿਰਾ(), ਸਿਰਾ_ਸਮਾਂ);
                ਜਾਰੀ;
            }

            UASSERT(ਉੱਤਮ_Mtaskp, "Should have found some task");

            ਉੱਤਮ_Mtaskp->predict_ਸ਼ੁਰੂ(ਉੱਤਮ_ਸਮਾਂ);
            ਅਚਲ uint32_t ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ = ਸਮਾਂ_ਸਾਰਣੀ.ਸਮਾਂ_ਸਾਰਣੀ_On(ਉੱਤਮ_Mtaskp, ਉੱਤਮ_Thread_Id);
            busyUntil[ਉੱਤਮ_Thread_Id] = ਉੱਤਮ_ਸਿਰਾ_ਸਮਾਂ;

            // Update the ready list
            ਅਚਲ ਮਾਪ_t erased = readyMTasks.erase(ਉੱਤਮ_Mtaskp);
            UASSERT_OBJ(erased > 0, ਉੱਤਮ_Mtaskp, "Should have erased something?");
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ_ਬਾਹਰਲਾ : ਉੱਤਮ_Mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ExecMTask* ਅਚਲ nextp = ਕਿਨਾਰਾ_ਬਾਹਰਲਾ.ਸਿਖਰ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(ਸਮਾਂ_ਸਾਰਣੀ.threadId(nextp) == Thread_ਸਮਾਂ_ਸਾਰਣੀ::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.ਲੱਭੋ(nextp) == readyMTasks.ਸਿਰਾ(), nextp,
                            "Tasks after one being assigned should not be ready");
                ਜੇ (isReady(ਸਮਾਂ_ਸਾਰਣੀ, nextp)) {
                    readyMTasks.ਪਾਓ(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 4)
            Thread_ਸਮਾਂ_ਸਾਰਣੀ::dumpDotFilePrefixedAlways(ਨਤੀਜਾ, "schedule", m_n_ਧਾਗੇ);

        ਵਾਪਸ ਨਤੀਜਾ;
    }

ਜਨਤਕ:
    // SELF TEST
    ਸਥਿਰ ਖਾਲੀ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->ਲੱਭੋ_UInt64_DType()};
        AstScope* ਅਚਲ scopep = ਨਵਾਂ AstScope{flp, ਖਾਲੀ_ਸੂਚਕ, "test", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        AstVarScope* ਅਚਲ trigVscp = ਨਵਾਂ AstVarScope{flp, scopep, varp};
        self_Test_Hier_ਪਹਿਲਾ(trigVscp);
        self_Test_ਸਾਧਾਰਨ_ਪਹਿਲਾ(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_ਰੁੱਖ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_ਰੁੱਖ(), scopep);
        VL_DO_DANGLING(varp->delete_ਰੁੱਖ(), varp);
    }
    ਸਥਿਰ ਖਾਲੀ self_Test_ਸਾਧਾਰਨ_ਪਹਿਲਾ(AstVarScope* trigVscp) {
        File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
        Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp = ਨਵਾਂ Ast_Exec_ਗ੍ਰਾਫ{flp, "test", trigVscp};
        V3_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ = *execGraphp->depGraphp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ makeBody = [&]() -> Ast_ਗੰਢ_Stmt* { ਵਾਪਸ ਨਵਾਂ AstComment{flp, ""}; };
        ExecMTask* ਅਚਲ t0 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t0->ਲਾਗਤ(1000);
        t0->ਤਰਜੀਹ_2(1100);
        ExecMTask* ਅਚਲ t1 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t1->ਲਾਗਤ(100);
        t1->ਤਰਜੀਹ_2(100);
        ExecMTask* ਅਚਲ t2 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t2->ਲਾਗਤ(100);
        t2->ਤਰਜੀਹ_2(100);
        t2->ਧਾਗੇ(2);
        ExecMTask* ਅਚਲ t3 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t3->ਲਾਗਤ(100);
        t3->ਤਰਜੀਹ_2(100);
        t3->ਧਾਗੇ(3);
        ExecMTask* ਅਚਲ t4 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t4->ਲਾਗਤ(100);
        t4->ਤਰਜੀਹ_2(100);
        t4->ਧਾਗੇ(3);
        ExecMTask* ਅਚਲ t5 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t5->ਲਾਗਤ(100);
        t5->ਤਰਜੀਹ_2(100);
        ExecMTask* ਅਚਲ t6 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t6->ਲਾਗਤ(100);
        t6->ਤਰਜੀਹ_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t0, t1, 1};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t0, t2, 1};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t2, t3, 1};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t2, t4, 1};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t3, t5, 1};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t4, t6, 1};

        constexpr uint32_t ਧਾਗੇ = 2;
        constexpr uint32_t hier_ਧਾਗੇ = 6;
        Pack_ਧਾਗੇ packer{ਧਾਗੇ, hier_ਧਾਗੇ,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ਅਚਲ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> scheduled = packer.pack(ਗ੍ਰਾਫ);
        UASSERT_SELFTEST(scheduled.ਮਾਪ(), 3);
        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ.ਮਾਪ(), ਧਾਗੇ);
        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[0].ਮਾਪ(), 2);
        ਲਈ (ਮਾਪ_t i = 1; i < scheduled[0].m_ਧਾਗੇ.ਮਾਪ(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[i].ਮਾਪ(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ.ਮਾਪ(), hier_ਧਾਗੇ / 3);
        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_ਧਾਗੇ.ਮਾਪ(), ਧਾਗੇ);
        UASSERT_SELFTEST(scheduled[2].m_ਧਾਗੇ[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_ਧਾਗੇ[1][0], t6);

        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState.ਮਾਪ(), 7);

        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t4, 5), 1360);

        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ਰੁੱਖ();
        VL_DO_DANGLING(execGraphp->delete_ਰੁੱਖ(), execGraphp);
        Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState.clear();
    }
    ਸਥਿਰ ਖਾਲੀ self_Test_Hier_ਪਹਿਲਾ(AstVarScope* trigVscp) {
        File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
        Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp = ਨਵਾਂ Ast_Exec_ਗ੍ਰਾਫ{flp, "test", trigVscp};
        V3_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ = *execGraphp->depGraphp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ makeBody = [&]() -> Ast_ਗੰਢ_Stmt* { ਵਾਪਸ ਨਵਾਂ AstComment{flp, ""}; };
        ExecMTask* ਅਚਲ t0 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t0->ਲਾਗਤ(1000);
        t0->ਤਰਜੀਹ_2(1100);
        t0->ਧਾਗੇ(2);
        ExecMTask* ਅਚਲ t1 = ਨਵਾਂ ExecMTask{execGraphp, ਖਾਲੀ_ਸੂਚਕ, makeBody()};
        t1->ਲਾਗਤ(100);
        t1->ਤਰਜੀਹ_2(100);

        /*
                          0
                          |
                          1
        */
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&ਗ੍ਰਾਫ, t0, t1, 1};

        constexpr uint32_t ਧਾਗੇ = 1;
        constexpr uint32_t hier_ਧਾਗੇ = 2;
        Pack_ਧਾਗੇ packer{ਧਾਗੇ, hier_ਧਾਗੇ,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ਅਚਲ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> scheduled = packer.pack(ਗ੍ਰਾਫ);
        UASSERT_SELFTEST(scheduled.ਮਾਪ(), 2);
        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ.ਮਾਪ(), hier_ਧਾਗੇ / 2);
        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[0].ਮਾਪ(), 1);
        ਲਈ (ਮਾਪ_t i = 1; i < scheduled[0].m_ਧਾਗੇ.ਮਾਪ(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[i].ਮਾਪ(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ਧਾਗੇ[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ.ਮਾਪ(), ਧਾਗੇ);
        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[0].ਮਾਪ(), 1);
        ਲਈ (ਮਾਪ_t i = 1; i < scheduled[1].m_ਧਾਗੇ.ਮਾਪ(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[i].ਮਾਪ(), 0);
        UASSERT_SELFTEST(scheduled[1].m_ਧਾਗੇ[0][0], t1);

        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState.ਮਾਪ(), 2);

        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ਸਮਾਂ_ਸਾਰਣੀ::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_ਸਮਾਂ(scheduled[1], t1, 1), 1130);

        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ਰੁੱਖ();
        VL_DO_DANGLING(execGraphp->delete_ਰੁੱਖ(), execGraphp);
        Thread_ਸਮਾਂ_ਸਾਰਣੀ::s_mtaskState.clear();
    }

    ਸਥਿਰ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> apply(V3_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ) {
        ਵਾਪਸ Pack_ਧਾਗੇ{}.pack(mtask_ਗ੍ਰਾਫ);
    }
};

ਵਰਤੋਂ EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
ਵਰਤੋਂ Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

ਖਾਲੀ normalizeCosts(Costs& costs) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ scale_ਲਾਗਤ = [](uint64_t ਮੁੱਲ, ਦੁੱਗਣਾ multiplier) {
        ਦੁੱਗਣਾ scaled = static_cast<ਦੁੱਗਣਾ>(ਮੁੱਲ) * multiplier;
        ਜੇ (ਮੁੱਲ && scaled < 1) scaled = 1;
        ਵਾਪਸ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t ਜੋੜ_ਲਾਗਤ_Profiled = 0;  // For data with estimate and profile
    uint64_t ਜੋੜ_ਲਾਗਤ_Estimate = 0;  // For data with estimate and profile
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& est : costs) {
        ਜੇ (est.ਦੂਜਾ.ਦੂਜਾ) {
            ਜੋੜ_ਲਾਗਤ_Estimate += est.ਦੂਜਾ.ਪਹਿਲਾ;
            ਜੋੜ_ਲਾਗਤ_Profiled += est.ਦੂਜਾ.ਦੂਜਾ;
        }
    }

    ਜੇ (ਜੋੜ_ਲਾਗਤ_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ਅਚਲ ਦੁੱਗਣਾ est_ਮੰਜ਼ਿਲ_Profile
            = static_cast<ਦੁੱਗਣਾ>(ਜੋੜ_ਲਾਗਤ_Profiled) / static_cast<ਦੁੱਗਣਾ>(ਜੋੜ_ਲਾਗਤ_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_ਮੰਜ਼ਿਲ_Profile
                                                    << ", sumCostProfiled=" << ਜੋੜ_ਲਾਗਤ_Profiled
                                                    << " sumCostEstimate=" << ਜੋੜ_ਲਾਗਤ_Estimate);
        ਲਈ (ਸਵੈਚਾਲਿਤ& est : costs) {
            uint64_t& ਲਾਗਤ_Estimate = est.ਦੂਜਾ.ਪਹਿਲਾ;
            ਲਾਗਤ_Estimate = scale_ਲਾਗਤ(ਲਾਗਤ_Estimate, est_ਮੰਜ਼ਿਲ_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_ਲਾਗਤ = 0;
    ਲਈ (ਸਵੈਚਾਲਿਤ& est : costs) {
        ਅਚਲ uint64_t& ਲਾਗਤ_Estimate = est.ਦੂਜਾ.ਪਹਿਲਾ;
        ਅਚਲ uint64_t& ਲਾਗਤ_Profiled = est.ਦੂਜਾ.ਦੂਜਾ;
        ਜੇ (max_ਲਾਗਤ < ਲਾਗਤ_Estimate) max_ਲਾਗਤ = ਲਾਗਤ_Estimate;
        ਜੇ (max_ਲਾਗਤ < ਲਾਗਤ_Profiled) max_ਲਾਗਤ = ਲਾਗਤ_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.ਦੂਜਾ.ਪਹਿਲਾ << " cp=" << est.ਦੂਜਾ.ਦੂਜਾ);
    }
    constexpr uint64_t scale_ਥੱਲੇ_ਮੰਜ਼ਿਲ = 10000000;  // Extra room for future algorithms to add costs
    ਜੇ (max_ਲਾਗਤ > scale_ਥੱਲੇ_ਮੰਜ਼ਿਲ) {
        ਅਚਲ ਦੁੱਗਣਾ scaleup = static_cast<ਦੁੱਗਣਾ>(scale_ਥੱਲੇ_ਮੰਜ਼ਿਲ) / static_cast<ਦੁੱਗਣਾ>(max_ਲਾਗਤ);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_ਲਾਗਤ);
        ਲਈ (ਸਵੈਚਾਲਿਤ& est : costs) {
            est.ਦੂਜਾ.ਪਹਿਲਾ = scale_ਲਾਗਤ(est.ਦੂਜਾ.ਪਹਿਲਾ, scaleup);
            est.ਦੂਜਾ.ਦੂਜਾ = scale_ਲਾਗਤ(est.ਦੂਜਾ.ਦੂਜਾ, scaleup);
        }
    }
}

ਖਾਲੀ remove_ਖਾਲੀ_ਥਾਵਾਂ_MTasks(Ast_Exec_ਗ੍ਰਾਫ* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    ਲਈ (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        ਜੇ (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), callp);
    }
    V3_ਗ੍ਰਾਫ* ਅਚਲ execMTaskGraphp = execGraphp->depGraphp();
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ਅਚਲ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ਅਚਲ funcp = mtaskp->funcp();
        ਜੇ (funcp->stmtsp()) ਜਾਰੀ;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_ਕਿਨਾਰੇ(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
}

ਖਾਲੀ fillinCosts(V3_ਗ੍ਰਾਫ* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ਅਚਲ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ਅਚਲ uint64_t ਲਾਗਤ_Estimate = V3_Instr_ਗਿਣਤੀ::ਗਿਣਤੀ_2(mtp->funcp(), ਝੂਠ);
        ਅਚਲ uint64_t ਲਾਗਤ_Profiled
            = V3Control::get_Profile_ਅੰਕੜੇ(v3Global.opt.ਅਗੇਤਰ(), mtp->hashName());
        ਜੇ (ਲਾਗਤ_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << ਲਾਗਤ_Profiled);
        }
        costs[mtp->id()] = std::make_pair(ਲਾਗਤ_Estimate, ਲਾਗਤ_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    ਪੂਰਨ ਕੁੱਲ_Estimates = 0;
    ਪੂਰਨ missingProfiles = 0;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ਅਚਲ mtp = vtx.as<ExecMTask>();
        ਅਚਲ uint32_t ਲਾਗਤ_Estimate = costs[mtp->id()].ਪਹਿਲਾ;
        ਅਚਲ uint64_t ਲਾਗਤ_Profiled = costs[mtp->id()].ਦੂਜਾ;
        UINFO(9, "ce = " << ਲਾਗਤ_Estimate << " cp=" << ਲਾਗਤ_Profiled);
        UASSERT(ਲਾਗਤ_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(ਲਾਗਤ_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ਅਚਲ uint64_t ਲਾਗਤ_Profiled32 = static_cast<uint32_t>(ਲਾਗਤ_Profiled);
        uint32_t ਲਾਗਤ_ਮੰਜ਼ਿਲ_Use = ਲਾਗਤ_Profiled32;
        ਜੇ (!ਲਾਗਤ_Profiled32) {
            ਲਾਗਤ_ਮੰਜ਼ਿਲ_Use = ਲਾਗਤ_Estimate;
            ਜੇ (ਲਾਗਤ_Estimate != 0) ++missingProfiles;
        }
        ਜੇ (ਲਾਗਤ_Estimate != 0) ++ਕੁੱਲ_Estimates;
        mtp->ਲਾਗਤ(ਲਾਗਤ_ਮੰਜ਼ਿਲ_Use);
        mtp->ਤਰਜੀਹ_2(ਲਾਗਤ_ਮੰਜ਼ਿਲ_Use);
    }

    ਜੇ (missingProfiles) {
        ਜੇ (File_ਸਤਰ* ਅਚਲ fl = V3Control::get_Profile_ਅੰਕੜੇ_File_ਸਤਰ()) {
            ਜੇ (V3Control::ਸ਼ਾਮਲ_ਹੈ_MTask_Profile_ਅੰਕੜੇ()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << ਕੁੱਲ_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

ਖਾਲੀ finalizeCosts(V3_ਗ੍ਰਾਫ* execMTaskGraphp) {
    ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered ser(execMTaskGraphp, ਗ੍ਰਾਫ_Way::ਉਲਟ_2);
    ਜਦੋਂ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vxp = ser.nextp()) {
        ExecMTask* ਅਚਲ mtp = const_cast<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਅਚਲ ExecMTask* ਅਚਲ followp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ExecMTask>();
            ਜੇ ((followp->ਤਰਜੀਹ_2() + mtp->ਲਾਗਤ()) > mtp->ਤਰਜੀਹ_2()) {
                mtp->ਤਰਜੀਹ_2(followp->ਤਰਜੀਹ_2() + mtp->ਲਾਗਤ());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ();

    // Record summary stats for final m_tasks graph.
    ਅਚਲ ਸਵੈਚਾਲਿਤ report = execMTaskGraphp->parallelismReport([](ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp) {  //
        ਵਾਪਸ vtxp->as<ਅਚਲ ExecMTask>()->ਲਾਗਤ();
    });
    V3Stats::ਜੋੜੋ_Stat("MTask graph, final, critical path cost", report.critical_ਰਸਤਾ_ਲਾਗਤ());
    V3Stats::ਜੋੜੋ_Stat("MTask graph, final, total graph cost", report.ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ());
    V3Stats::ਜੋੜੋ_Stat("MTask graph, final, mtask count", report.ਸ਼ੀਰਸ਼_ਗਿਣਤੀ());
    V3Stats::ਜੋੜੋ_Stat("MTask graph, final, edge count", report.ਕਿਨਾਰਾ_ਗਿਣਤੀ());
    V3Stats::ਜੋੜੋ_Stat("MTask graph, final, parallelism factor", report.parallelism_ਗੁਣਕ());
    ਜੇ (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_ਰਸਤਾ_ਲਾਗਤ());
        UINFO(0, "    Total graph cost = " << report.ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ());
        UINFO(0, "    MTask vertex count = " << report.ਸ਼ੀਰਸ਼_ਗਿਣਤੀ());
        UINFO(0, "    Edge count = " << report.ਕਿਨਾਰਾ_ਗਿਣਤੀ());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ਗੁਣਕ());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ਅਚਲ std::string& name, uint32_t nDependencies) {
    Ast_ਗੰਢ_Module* ਅਚਲ modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
    File_ਸਤਰ* ਅਚਲ fl = modp->fileline();
    AstBasicDType* ਅਚਲ dtypep
        = v3Global.rootp()->typeTablep()->ਲੱਭੋ_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(ਸੱਚ);
    varp->valuep(ਨਵਾਂ AstConst{fl, nDependencies});
    modp->ਜੋੜੋ_Stmtsp(varp);
    ਵਾਪਸ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ਅਚਲ std::string& ਵਿਧੀ) {
    File_ਸਤਰ* ਅਚਲ fl = varp->fileline();
    AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ਅਚਲ AstScope* ਅਚਲ ਸਿਖਰ_Scopep = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();
    refp->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::VlSyms{}, ਸਿਖਰ_Scopep->nameDotless()});
    AstCStmt* ਅਚਲ stmtp = ਨਵਾਂ AstCStmt{fl};
    stmtp->ਜੋੜੋ(refp);
    stmtp->ਜੋੜੋ("." + ਵਿਧੀ + ";");
    ਵਾਪਸ stmtp;
}

ਖਾਲੀ ਜੋੜੋ_MTask_ਮੰਜ਼ਿਲ_Function(ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ, ਅਚਲ uint32_t threadId, AstCFunc* funcp,
                        ਅਚਲ ExecMTask* mtaskp,
                        ਅਚਲ std::unordered_map<ਅਚਲ ExecMTask*, AstVar*>& stateVars) {
    AstScope* ਅਚਲ scopep = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();
    Ast_ਗੰਢ_Module* ਅਚਲ modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
    File_ਸਤਰ* ਅਚਲ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_CStmt = [=](ਅਚਲ string& stmt) -> ਖਾਲੀ {  //
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, stmt});
    };

    ਜੇ (ਸਮਾਂ_ਸਾਰਣੀ.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        ਜੇ (v3Global.opt.profExec()) {
            ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->ਜੋੜੋ_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        ਜੇ (v3Global.opt.profExec()) {
            ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    ਜੇ (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        ਜੋੜੋ_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::ਮੰਜ਼ਿਲ_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{fl, mtaskp->funcp()};
    callp->self_ਸੂਚਕ(VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::VlSyms{}, scopep->nameDotless()});
    callp->dtype_ਸੈੱਟ_Void();
    funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());

    ਜੇ (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        ਜੋੜੋ_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::ਮੰਜ਼ਿਲ_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਅਚਲ ExecMTask* ਅਚਲ nextp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ExecMTask>();
        ਜੇ (ਸਮਾਂ_ਸਾਰਣੀ.threadId(nextp) != threadId && ਸਮਾਂ_ਸਾਰਣੀ.ਸ਼ਾਮਲ_ਹੈ(nextp)) {
            funcp->ਜੋੜੋ_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ਅਚਲ std::ਸਦਿਸ਼<AstCFunc*> createThreadFunctions(ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ,
                                                   ਅਚਲ string& ਟੈਗ) {
    Ast_ਗੰਢ_Module* ਅਚਲ modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
    File_ਸਤਰ* ਅਚਲ fl = modp->fileline();

    std::ਸਦਿਸ਼<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ਅਚਲ ExecMTask*, AstVar*> stateVars;
    ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>& thread : ਸਮਾਂ_ਸਾਰਣੀ.m_ਧਾਗੇ) {
        ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ mtaskp : thread) {
            ਜੇ (ਅਚਲ uint32_t nDependencies = ਸਮਾਂ_ਸਾਰਣੀ.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_ਮੰਜ਼ਿਲ_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ ExecMTask*>& thread : ਸਮਾਂ_ਸਾਰਣੀ.m_ਧਾਗੇ) {
        ਜੇ (thread.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
        ਅਚਲ uint32_t threadId = ਸਮਾਂ_ਸਾਰਣੀ.threadId(thread.front());
        ਅਚਲ string name{"__Vthread__" + ਟੈਗ + "__s" + cvt_ਮੰਜ਼ਿਲ_Str(ਸਮਾਂ_ਸਾਰਣੀ.id()) + "__t"
                          + cvt_ਮੰਜ਼ਿਲ_Str(threadId)};
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{fl, name, ਖਾਲੀ_ਸੂਚਕ, "void"};
        modp->ਜੋੜੋ_Stmtsp(funcp);
        funcps.ਧੱਕੋ_ਪਿੱਠ(funcp);
        funcp->isStatic(ਸੱਚ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(ਸੱਚ);
        funcp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਸੱਚ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, EmitCUtil::void_Self_ਸੌਂਪੋ(modp)});
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, EmitCUtil::sym_Class_ਸੌਂਪੋ()});

        // Invoke each mtask scheduled to this thread from the thread function
        ਲਈ (ਅਚਲ ExecMTask* ਅਚਲ mtaskp : thread) {
            ਜੋੜੋ_MTask_ਮੰਜ਼ਿਲ_Function(ਸਮਾਂ_ਸਾਰਣੀ, threadId, funcp, mtaskp, stateVars);
        }
    }

    ਵਾਪਸ funcps;
}

ਖਾਲੀ ਜੋੜੋ_Thread_ਸ਼ੁਰੂ_Wrapper(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    // FileLine used for constructing nodes below
    File_ਸਤਰ* ਅਚਲ fl = v3Global.rootp()->fileline();
    ਅਚਲ string& ਟੈਗ = execGraphp->name();

    // Add thread function invocations to execGraph
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_CStmt = [=](ਅਚਲ string& stmt) -> ਖਾਲੀ {  //
        execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, stmt});
    };

    ਜੇ (v3Global.opt.profExec()) {
        ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    ਜੋੜੋ_CStmt("vlSymsp->__Vm_even_cycle__" + ਟੈਗ + " = !vlSymsp->__Vm_even_cycle__" + ਟੈਗ + ";");

    ਜੇ (!v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ_CStmt("std::vector<size_t> indexes;");
}

ਖਾਲੀ ਜੋੜੋ_Thread_ਸਿਰਾ_Wrapper(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    // Add thread function invocations to execGraph
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_CStmt = [=](ਅਚਲ string& stmt) -> ਖਾਲੀ {  //
        File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, stmt});
    };

    ਜੋੜੋ_CStmt("Verilated::mtaskId(0);");
    ਜੇ (v3Global.opt.profExec()) { ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
ਖਾਲੀ ਜੋੜੋ_Thread_ਸ਼ੁਰੂ_ਮੰਜ਼ਿਲ_Exec_ਗ੍ਰਾਫ(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp,
                               ਅਚਲ std::ਸਦਿਸ਼<AstCFunc*>& funcps, uint32_t ਸਮਾਂ_ਸਾਰਣੀ_Id) {
    // FileLine used for constructing nodes below
    File_ਸਤਰ* ਅਚਲ fl = v3Global.rootp()->fileline();
    ਅਚਲ string& ਟੈਗ = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ਅਚਲ finalVarp
        = newMTaskStateVar("final__" + cvt_ਮੰਜ਼ਿਲ_Str(ਸਮਾਂ_ਸਾਰਣੀ_Id) + "__" + ਟੈਗ, funcps.ਮਾਪ());
    ਲਈ (AstCFunc* ਅਚਲ funcp : funcps) {
        funcp->ਜੋੜੋ_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_CStmt = [=](ਅਚਲ string& stmt) -> ਖਾਲੀ {  //
        execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, stmt});
    };

    ਅਚਲ uint32_t ਆਖਰੀ = funcps.ਮਾਪ() - 1;
    ਜੇ (!v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ() && ਆਖਰੀ > 0) {
        ਜੋੜੋ_CStmt("for (size_t i = 0; i < " + std::ਮੰਜ਼ਿਲ_string(ਆਖਰੀ) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    ਲਈ (AstCFunc* ਅਚਲ funcp : funcps) {
        ਜੇ (i != ਆਖਰੀ) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{fl};
            execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(cstmtp);
            cstmtp->ਜੋੜੋ("vlSymsp->__Vm_threadPoolp->workerp(");
            ਜੇ (v3Global.opt.hier_ਬੱਚਾ() || !v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ()) {
                cstmtp->ਜੋੜੋ("indexes[" + std::ਮੰਜ਼ਿਲ_string(i) + "]");
            } ਨਹੀਂਤਾਂ {
                cstmtp->ਜੋੜੋ(std::ਮੰਜ਼ਿਲ_string(i));
            }
            cstmtp->ਜੋੜੋ(")->addTask(");
            cstmtp->ਜੋੜੋ(ਨਵਾਂ Ast_Addr_ਦਾ_CFunc{fl, funcp});
            cstmtp->ਜੋੜੋ(", vlSelf, vlSymsp->__Vm_even_cycle__" + ਟੈਗ + ");");
        } ਨਹੀਂਤਾਂ {
            // The last will run on the main thread.
            AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{fl, funcp};
            callp->dtype_ਸੈੱਟ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + ਟੈਗ);
            execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Thread schedule total tasks", i);

    ਜੇ (v3Global.opt.profExec()) {
        ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->ਜੋੜੋ_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + ਟੈਗ + ")"));
    ਜੇ (v3Global.opt.profExec()) {
        ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    ਜੇ (!v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ() && ਆਖਰੀ > 0) {
        ਜੋੜੋ_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

ਖਾਲੀ processMTaskBodies(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ਅਚਲ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ਅਚਲ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ਗੰਢ* stmtsp = funcp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();

        // Helper function to make the code a bit more legible
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_CStmt = [=](ਅਚਲ string& stmt) -> ਖਾਲੀ {  //
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        ਜੇ (v3Global.opt.profExec()) {
            std::string ਦਲੀਲਾਂ = std::ਮੰਜ਼ਿਲ_string(mtaskp->id());
            ਦਲੀਲਾਂ += ", " + std::ਮੰਜ਼ਿਲ_string(mtaskp->predict_ਸ਼ੁਰੂ());
            ਦਲੀਲਾਂ += ", \"";
            ਜੇ (v3Global.opt.hier_ਬੱਚਾ()) ਦਲੀਲਾਂ += v3Global.opt.ਸਿਖਰ_Module();
            ਦਲੀਲਾਂ += "\"";
            ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + ਦਲੀਲਾਂ + ");");
        }
        // Set mtask ID in the run-time system
        ਜੋੜੋ_CStmt("Verilated::mtaskId(" + std::ਮੰਜ਼ਿਲ_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->ਜੋੜੋ_Stmtsp(stmtsp);
        // Flush message queue
        ਜੋੜੋ_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        ਜੇ (v3Global.opt.profExec()) {
            ਅਚਲ std::string& ਦਲੀਲਾਂ = std::ਮੰਜ਼ਿਲ_string(mtaskp->ਲਾਗਤ());
            ਜੋੜੋ_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + ਦਲੀਲਾਂ + ");");
        }
    }
}

ਖਾਲੀ implement_Exec_ਗ੍ਰਾਫ(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp, ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ) {
    // Nothing to be done if there are no MTasks in the graph at all.
    ਜੇ (execGraphp->depGraphp()->ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

    // Create a function to be run by each thread.
    ਅਚਲ std::ਸਦਿਸ਼<AstCFunc*>& funcps = createThreadFunctions(ਸਮਾਂ_ਸਾਰਣੀ, execGraphp->name());
    UASSERT(!funcps.ਖਾਲੀ_ਥਾਵਾਂ(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    ਜੋੜੋ_Thread_ਸ਼ੁਰੂ_ਮੰਜ਼ਿਲ_Exec_ਗ੍ਰਾਫ(execGraphp, funcps, ਸਮਾਂ_ਸਾਰਣੀ.id());
}

ਖਾਲੀ create_ਲੜੀਵਾਰ_Function(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_ਸਤਰ* ਅਚਲ flp = execGraphp->fileline();
    Ast_ਗੰਢ_Module* ਅਚਲ modp = v3Global.rootp()->ਸਿਖਰ_Modulep();
    AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{flp, "runExecGraph_" + execGraphp->name(), ਖਾਲੀ_ਸੂਚਕ};
    funcp->isLoose(ਸੱਚ);
    funcp->voidSelf(ਸੱਚ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(ਸੱਚ);
    modp->ਜੋੜੋ_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

ਖਾਲੀ ਜੋੜੋ_Trigger_Masks(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_ਸਤਰ* ਅਚਲ flp = execGraphp->fileline();
    AstVar* ਅਚਲ triggersp = execGraphp->triggerp()->varp();
    ਲਈ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> ਸ਼ਬਦ_Masks;
        callp->funcp()->foreach([&](ਅਚਲ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            ਲਈ (ਅਚਲ uint32_t ਸੂਚਕਾਂਕ : trigp->indices()) {
                ਸ਼ਬਦ_Masks[VL_BITWORD_Q(ਸੂਚਕਾਂਕ)] |= VL_ਦੋਆਧਾਰੀ_ਅੰਕ_Q(ਸੂਚਕਾਂਕ);
            }
        });
        AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{flp, triggersp->dtypep(),
                                                     ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, 0}};
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਸ਼ਬਦ_ਨਕਾਬ : ਸ਼ਬਦ_Masks) {
            consp->valuep(ਸ਼ਬਦ_ਨਕਾਬ.ਪਹਿਲਾ,
                          ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, ਸ਼ਬਦ_ਨਕਾਬ.ਦੂਜਾ});
        }
        execGraphp->ਜੋੜੋ_Maskps(V3ConstPool::ਲੱਭੋ(consp));
        VL_DO_DANGLING(consp->delete_ਰੁੱਖ(), consp);
    }
}

ਖਾਲੀ ਜੋੜੋ_Trigger_Costs(Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_ਸਤਰ* ਅਚਲ flp = execGraphp->fileline();
    ਅਚਲ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::ਸਦਿਸ਼<uint32_t> costs(nTriggers, 0);
    ਲਈ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            ਜੇ (ਅਚਲ AstTriggered* ਅਚਲ trigp = VN_CAST(ifp->condp(), Triggered)) {
                ਅਚਲ uint32_t ਲਾਗਤ = V3_Instr_ਗਿਣਤੀ::ਗਿਣਤੀ_2(ifp, ਝੂਠ);
                ਲਈ (ਅਚਲ uint32_t ਸੂਚਕਾਂਕ : trigp->indices()) costs.at(ਸੂਚਕਾਂਕ) += ਲਾਗਤ;
            }
        });
    }
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    AstUnpackArrayDType* ਅਚਲ dtypep
        = ਨਵਾਂ AstUnpackArrayDType{flp, netlistp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(32, 32, VSigning::UNSIGNED),
                                  ਨਵਾਂ AstRange{flp, static_cast<ਪੂਰਨ>(nTriggers - 1), 0}};
    netlistp->typeTablep()->ਜੋੜੋ_Typesp(dtypep);
    AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{flp, dtypep, ਨਵਾਂ AstConst{flp, 0}};
    ਲਈ (uint32_t ਸੂਚਕਾਂਕ = 0; ਸੂਚਕਾਂਕ < nTriggers; ++ਸੂਚਕਾਂਕ) {
        ਜੇ (costs[ਸੂਚਕਾਂਕ]) consp->valuep(ਸੂਚਕਾਂਕ, ਨਵਾਂ AstConst{flp, costs[ਸੂਚਕਾਂਕ]});
    }
    execGraphp->triggerCostsp(V3ConstPool::ਲੱਭੋ(consp));
    VL_DO_DANGLING(consp->delete_ਰੁੱਖ(), consp);
}

// Called by Verilator top stage
ਖਾਲੀ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::ਸਦਿਸ਼<Ast_Exec_ਗ੍ਰਾਫ*> execGraphps;
    netlistp->ਸਿਖਰ_Modulep()->foreach([&](Ast_Exec_ਗ੍ਰਾਫ* egp) { execGraphps.emplace_ਪਿੱਠ(egp); });

    // Process each
    ਲਈ (Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_ਖਾਲੀ_ਥਾਵਾਂ_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        ਜੇ (execGraphp->depGraphp()->ਖਾਲੀ_ਥਾਵਾਂ()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), execGraphp);
            ਵਾਪਸ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_ਲੜੀਵਾਰ_Function(execGraphp);
        ਜੋੜੋ_Thread_ਸ਼ੁਰੂ_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ਅਚਲ std::ਸਦਿਸ਼<Thread_ਸਮਾਂ_ਸਾਰਣੀ> packed = Pack_ਧਾਗੇ::apply(*execGraphp->depGraphp());
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Optimizations, Thread schedule count",
                            static_cast<ਦੁੱਗਣਾ>(packed.ਮਾਪ()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        ਲਈ (ਅਚਲ Thread_ਸਮਾਂ_ਸਾਰਣੀ& ਸਮਾਂ_ਸਾਰਣੀ : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_ਗ੍ਰਾਫ(execGraphp, ਸਮਾਂ_ਸਾਰਣੀ);
        }

        ਜੋੜੋ_Thread_ਸਿਰਾ_Wrapper(execGraphp);

        ਜੋੜੋ_Trigger_Masks(execGraphp);
        ਜੋੜੋ_Trigger_Costs(execGraphp);
    }
}

ਖਾਲੀ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].ਪਹਿਲਾ, 1000);
        UASSERT_SELFTEST(costs[1].ਦੂਜਾ, 1000);
        UASSERT_SELFTEST(costs[2].ਪਹਿਲਾ, 2000);
        UASSERT_SELFTEST(costs[2].ਦੂਜਾ, 0);
        UASSERT_SELFTEST(costs[3].ਪਹਿਲਾ, 3000);
        UASSERT_SELFTEST(costs[3].ਦੂਜਾ, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].ਪਹਿਲਾ, 2500000);
        UASSERT_SELFTEST(costs[1].ਦੂਜਾ, 5000000);
        UASSERT_SELFTEST(costs[2].ਪਹਿਲਾ, 5000000);
        UASSERT_SELFTEST(costs[2].ਦੂਜਾ, 10000000);
        UASSERT_SELFTEST(costs[3].ਪਹਿਲਾ, 7500000);
        UASSERT_SELFTEST(costs[3].ਦੂਜਾ, 1);
    }

    Pack_ਧਾਗੇ::selfTest();
}

}  // namespace V3ExecGraph
