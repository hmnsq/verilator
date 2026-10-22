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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"
#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

AstCFunc* ExecMTask::createCFunc(Ast_Exec_ನಕ್ಷೆ* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp,
                                 uint32_t id) {
    ಸ್ಥಿರಾಂಕ std::string newName = execGraphp->name() + "_mtask" + std::ಗಮ್ಯ_string(id);
    AstCFunc* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCFunc{execGraphp->fileline(), newName, scopep};
    newp->isLoose(ನಿಜ);
    newp->voidSelf(ನಿಜ);  // Called through VlExecGraph::Fnp
    newp->dontCombine(ನಿಜ);
    newp->ಕೂಡಿಸು_Stmtsp(stmtsp);
    ಆದರೆ (scopep) scopep->ಕೂಡಿಸು_Blocksp(newp);
    ಹಿಂತಿರುಗಿ newp;
}

ExecMTask::ExecMTask(Ast_Exec_ನಕ್ಷೆ* execGraphp, AstScope* scopep,
                     Ast_ಗಂಟು_Stmt* stmtsp) VL_MT_DISABLED  //
    : V3_ನಕ್ಷೆ_ಶೃಂಗ{execGraphp->depGraphp()},
      m_id{s_ಮುಂದಿನ_Id++},
      m_funcp{createCFunc(execGraphp, scopep, stmtsp, m_id)},
      m_hashName{V3Hasher::uncachedHash(m_funcp).ಗಮ್ಯ_String()} {}

ಶೂನ್ಯ ExecMTask::dump(std::ostream& str) ಸ್ಥಿರಾಂಕ {
    str << name() << "." << cvt_ಗಮ್ಯ_Hex(ಇದು);
    ಆದರೆ (ಆದ್ಯತೆ_2() || ವೆಚ್ಚ()) str << " [pr=" << ಆದ್ಯತೆ_2() << " c=" << cvt_ಗಮ್ಯ_Str(ವೆಚ್ಚ()) << "]";
}

std::atomic<uint32_t> ExecMTask::s_ಮುಂದಿನ_Id{0};

ನಾಮಾಕಾಶ V3_Exec_ನಕ್ಷೆ {

//######################################################################
// ThreadSchedule

// The thread schedule, containing all information needed later. Note that this is a simple
// aggregate data type and the only way to get hold of an instance of it is via
// PackThreads::pack, which is moved from there and is const, which means we can only acquire a
// const reference to is so no further modifications are allowed, so all members are public
// (attributes).
ವರ್ಗ Thread_ವೇಳಾಪಟ್ಟಿ final {
    ಮಿತ್ರ ವರ್ಗ Pack_ಎಳೆಗಳು;

    uint32_t m_id;  // Unique ID of a schedule
    ಸ್ಥಿರ uint32_t s_ಮುಂದಿನ_Id;  // Next ID number to use
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ ExecMTask*> mtasks;  // Mtasks in this schedule
    uint32_t m_ಕೊನೆ_ಸಮಯ = 0;  // Latest task end time in this schedule

ಸಾರ್ವಜನಿಕ:
    // CONSTANTS
    ಸ್ಥಿರ constexpr uint32_t UNASSIGNED = 0xffffffff;

    // TYPES
    ರಚನೆ MTaskState final {
        uint32_t completion_ಸಮಯ = 0;  // Estimated time this mtask will complete
        uint32_t threadId = UNASSIGNED;  // Thread id this MTask is assigned to
        ಸ್ಥಿರಾಂಕ ExecMTask* nextp = ಶೂನ್ಯಸೂಚಕ;  // Next MTask on same thread after this
    };

    // MEMBERS
    // Allocation of sequence of MTasks to threads. Can be considered a map from thread ID to
    // the sequence of MTasks to be executed by that thread.
    std::ಸದಿಶ<std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>> m_ಎಳೆಗಳು;

    // Global state for each mtask.
    ಸ್ಥಿರ std::unordered_map<ಸ್ಥಿರಾಂಕ ExecMTask*, MTaskState> s_mtaskState;

    ಸ್ಪಷ್ಟ Thread_ವೇಳಾಪಟ್ಟಿ(uint32_t n_ಎಳೆಗಳು)
        : m_id{s_ಮುಂದಿನ_Id++}
        , m_ಎಳೆಗಳು{n_ಎಳೆಗಳು} {}
    Thread_ವೇಳಾಪಟ್ಟಿ(Thread_ವೇಳಾಪಟ್ಟಿ&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Thread_ವೇಳಾಪಟ್ಟಿ& ಕ್ರಿಯಾಕಾರಕ=(Thread_ವೇಳಾಪಟ್ಟಿ&&) = ಪೂರ್ವನಿಯೋಜಿತ;

ಖಾಸಗಿ:
    VL_UNCOPYABLE(Thread_ವೇಳಾಪಟ್ಟಿ);

    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_thread_Box_ಅಗಲ = 2.5;
    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_thread_Box_ಎತ್ತರ = 1.5;
    ಸ್ಥಿರ constexpr ದ್ವಿಗುಣ s_horizontalGap = s_thread_Box_ಅಗಲ / 2;

    // Debugging
    // Variant of dumpDotFilePrefixed without --dump option check
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFilePrefixedAlways(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ>& schedules,
                                          ಸ್ಥಿರಾಂಕ string& nameComment, uint32_t n_ಎಳೆಗಳು) {
        dumpDotFile(schedules, v3Global.debugFilename(nameComment) + ".dot", n_ಎಳೆಗಳು);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFile(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ>& schedules, ಸ್ಥಿರಾಂಕ string& filename,
                            uint32_t n_ಎಳೆಗಳು) {
        // This generates a file used by graphviz, https://www.graphviz.org
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
        ಆದರೆ (logp->fail()) v3fatal("Can't write file: " << filename);

        //ಶೀರ್ಷಿಕೆ
        *logp << "digraph v3graph {\n";
        *logp << "  graph[layout=\"neato\" labelloc=t labeljust=l label=\"" << filename << "\"]\n";
        *logp << "  node[shape=\"rect\" ratio=\"fill\" fixedsize=true]\n";

        // Thread labels
        *logp << "\n  // Threads\n";

        ಗಾಗಿ (uint32_t i = 0; i < n_ಎಳೆಗಳು; ++i) {
            ಸ್ಥಿರಾಂಕ string name = "t" + std::ಗಮ್ಯ_string(i);
            ಸ್ಥಿರಾಂಕ string label = "Thread " + std::ಗಮ್ಯ_string(i);
            constexpr ದ್ವಿಗುಣ posX = -s_horizontalGap;
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ posY = -static_cast<ದ್ವಿಗುಣ>(i) * s_thread_Box_ಎತ್ತರ;
            dump_Dot_File_Emit_ಖಂಡ(logp, name, label, s_thread_Box_ಅಗಲ, s_thread_Box_ಎತ್ತರ, posX,
                                 posY, "grey");
        }

        // MTask nodes
        *logp << "\n  // MTasks\n";

        uint32_t max_ವೆಚ್ಚ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& state : Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = state.ಮೊದಲ;
            max_ವೆಚ್ಚ = std::max(max_ವೆಚ್ಚ, mtaskp->ವೆಚ್ಚ());
        }
        // To avoid segments getting too large, limit maximal mtask length.
        // Based on the mtask cost, normalize it using segment cost
        constexpr uint32_t segments_ಪ್ರತಿ_Longest_Mtask = 4;
        ಸ್ಥಿರಾಂಕ uint32_t segment_ವೆಚ್ಚ = max_ವೆಚ್ಚ / segments_ಪ್ರತಿ_Longest_Mtask;

        // Create columns of tasks whose execution intervals overlaps.
        // Keep offset for each column for correctly aligned tasks.
        std::ಸದಿಶ<ದ್ವಿಗುಣ> offsets(n_ಎಳೆಗಳು, 0.0);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ : schedules) {
            ಆದರೆ (ವೇಳಾಪಟ್ಟಿ.mtasks.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಬಳಸಿ ಸ್ತಂಭ = std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>;
            std::ಸದಿಶ<ಸ್ತಂಭ> columns = {{}};

            // Order tasks based on their start time
            ರಚನೆ Cmp final {
                ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ a, ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ b) ಸ್ಥಿರಾಂಕ {
                    ಆದರೆ (ಆರಂಭ_ಸಮಯ(a) == ಆರಂಭ_ಸಮಯ(b)) ಹಿಂತಿರುಗಿ threadId(a) < threadId(b);
                    ಹಿಂತಿರುಗಿ ಆರಂಭ_ಸಮಯ(a) < ಆರಂಭ_ಸಮಯ(b);
                }
            };
            ಸ್ಥಿರಾಂಕ std::multiset<ಸ್ಥಿರಾಂಕ ExecMTask*, Cmp> tasks(ವೇಳಾಪಟ್ಟಿ.mtasks.begin(),
                                                             ವೇಳಾಪಟ್ಟಿ.mtasks.ಕೊನೆ());

            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : tasks) {
                ಸ್ತಂಭ& ಸ್ತಂಭ_2 = columns.ಹಿಂಭಾಗ();
                UASSERT(ಸ್ತಂಭ_2.ಗಾತ್ರ_2() <= n_ಎಳೆಗಳು, "Invalid partitioning");

                ತಾರ್ಕಿಕ intersects = ನಿಜ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ earlierMtask : ಸ್ತಂಭ_2) {
                    ಆದರೆ (ಕೊನೆ_ಸಮಯ(mtaskp) <= ಆರಂಭ_ಸಮಯ(earlierMtask)
                        || ಆರಂಭ_ಸಮಯ(mtaskp) >= ಕೊನೆ_ಸಮಯ(earlierMtask)) {
                        intersects = ಸುಳ್ಳು;
                        ನಿಲ್ಲಿಸು;
                    }
                }
                ಆದರೆ (intersects) {
                    ಸ್ತಂಭ_2.emplace_ಹಿಂಭಾಗ(mtaskp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    columns.emplace_ಹಿಂಭಾಗ(ಸ್ತಂಭ{mtaskp});
                }
            }

            UASSERT(!columns.front().ಖಾಲಿಗಳು(), "Should be populated by mtasks");

            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ತಂಭ& ಸ್ತಂಭ_2 : columns) {
                ದ್ವಿಗುಣ ಕೊನೆಯ_ಸ್ತಂಭ_ಸ್ಥಾನಾಂತರ = 0;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : ಸ್ತಂಭ_2) {
                    dumpDotFileEmitMTask(logp, mtaskp, ವೇಳಾಪಟ್ಟಿ, segment_ವೆಚ್ಚ, offsets);
                    ಕೊನೆಯ_ಸ್ತಂಭ_ಸ್ಥಾನಾಂತರ = std::max(ಕೊನೆಯ_ಸ್ತಂಭ_ಸ್ಥಾನಾಂತರ, offsets[threadId(mtaskp)]);
                }
                // Even out column offset
                std::fill(offsets.begin(), offsets.ಕೊನೆ(), ಕೊನೆಯ_ಸ್ತಂಭ_ಸ್ಥಾನಾಂತರ);
            }

            dumpDotFileEmitFork(logp, offsets.front(), n_ಎಳೆಗಳು);

            // Emit MTask dependency edges
            *logp << "\n  // MTask dependencies\n";

            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>& thread : ವೇಳಾಪಟ್ಟಿ.m_ಎಳೆಗಳು) {
                ಆದರೆ (thread.ಖಾಲಿಗಳು()) ನಿಲ್ಲಿಸು;  // No more threads

                // Show that schedule ends when all tasks are finished
                *logp << "  " << thread.ಹಿಂಭಾಗ()->name() << " -> fork_"
                      << static_cast<ಪೂರ್ಣಾಂಕ>(offsets.front()) << "\n";

                // Show that tasks from the same thread are executed in a sequence
                ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < thread.ಗಾತ್ರ_2(); ++i)
                    *logp << "  " << thread[i - 1]->name() << " -> " << thread[i]->name() << "\n";

                // Emit cross-task dependencies
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : thread) {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                        ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ ತುದಿ_MTaskp = ಅಂಚು.ತುದಿ()->cast<ಸ್ಥಿರಾಂಕ ExecMTask>();
                        ಆದರೆ (ತುದಿ_MTaskp && ವೇಳಾಪಟ್ಟಿ.ಒಳಗೊಂಡಿದೆ(ತುದಿ_MTaskp)
                            && threadId(ತುದಿ_MTaskp) != threadId(mtaskp))
                            *logp << "  " << mtaskp->name() << " -> " << ತುದಿ_MTaskp->name() << "\n";
                    }
                }
            }
        }

        // Trailer
        *logp << "}\n";
        logp->close();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dump_Dot_File_Emit_ಖಂಡ(ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream>& logp,
                                     ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& label, ದ್ವಿಗುಣ ಅಗಲ,
                                     ದ್ವಿಗುಣ ಎತ್ತರ, ದ್ವಿಗುಣ xPos, ದ್ವಿಗುಣ yPos,
                                     ಸ್ಥಿರಾಂಕ string& fillColor) {
        *logp << "  " << name << " [label=\"" << label << "\" width=" << ಅಗಲ
              << " height=" << ಎತ್ತರ << " pos=\"" << xPos << "," << yPos
              << "!\" style=\"filled\" fillcolor=\"" << fillColor << "\"]\n";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFileEmitMTask(ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream>& logp,
                                     ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp, ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ,
                                     uint32_t segment_ವೆಚ್ಚ, std::ಸದಿಶ<ದ್ವಿಗುಣ>& offsets) {
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < mtaskp->ಎಳೆಗಳು(); ++i) {
            // Keep original name for the original thread of hierarchical task to keep
            // dependency tracking, add '_' for the rest to differentiate them.
            ಸ್ಥಿರಾಂಕ string name = i == 0 ? mtaskp->name() : mtaskp->name() + '_' + std::ಗಮ್ಯ_string(i);
            ಸ್ಥಿರಾಂಕ string label = mtaskp->name() + " (" + std::ಗಮ್ಯ_string(ಆರಂಭ_ಸಮಯ(mtaskp)) + ':'
                                 + std::ಗಮ್ಯ_string(ಕೊನೆ_ಸಮಯ(mtaskp)) + ')'
                                 + "\\ncost=" + std::ಗಮ್ಯ_string(mtaskp->ವೆಚ್ಚ())
                                 + "\\npriority=" + std::ಗಮ್ಯ_string(mtaskp->ಆದ್ಯತೆ_2());
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಅಗಲ
                = std::max(s_thread_Box_ಅಗಲ,
                           s_thread_Box_ಅಗಲ * static_cast<ದ್ವಿಗುಣ>(mtaskp->ವೆಚ್ಚ()) / segment_ವೆಚ್ಚ);
            ಸ್ಥಿರಾಂಕ uint32_t mtaskThreadId = threadId(mtaskp) + i * ವೇಳಾಪಟ್ಟಿ.m_ಎಳೆಗಳು.ಗಾತ್ರ_2();
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ xPos = ಅಗಲ / 2 + offsets[mtaskThreadId];
            offsets[mtaskThreadId] += ಅಗಲ + s_horizontalGap;
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ yPos = -s_thread_Box_ಎತ್ತರ * static_cast<ದ್ವಿಗುಣ>(mtaskThreadId);
            ಸ್ಥಿರಾಂಕ string fillColor = i == 0 ? "white" : "lightgreen";
            dump_Dot_File_Emit_ಖಂಡ(logp, name, label, ಅಗಲ, s_thread_Box_ಎತ್ತರ, xPos, yPos,
                                 fillColor);
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ dumpDotFileEmitFork(ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream>& logp, ದ್ವಿಗುಣ ಸ್ಥಾನಾಂತರ,
                                    uint32_t n_ಎಳೆಗಳು) {
        ಸ್ಥಿರಾಂಕ string& name = "fork_" + std::ಗಮ್ಯ_string(static_cast<ಪೂರ್ಣಾಂಕ>(ಸ್ಥಾನಾಂತರ));
        constexpr ದ್ವಿಗುಣ ಅಗಲ = s_thread_Box_ಅಗಲ / 8;
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಎತ್ತರ = s_thread_Box_ಎತ್ತರ * n_ಎಳೆಗಳು;
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ xPos = ಸ್ಥಾನಾಂತರ - s_horizontalGap / 2;
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ yPos
            = -static_cast<ದ್ವಿಗುಣ>(n_ಎಳೆಗಳು) / 2 * s_thread_Box_ಎತ್ತರ + s_thread_Box_ಎತ್ತರ / 2;
        dump_Dot_File_Emit_ಖಂಡ(logp, name, "", ಅಗಲ, ಎತ್ತರ, xPos, yPos, "black");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ uint32_t threadId(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it = s_mtaskState.ಕಂಡುಹಿಡಿ(mtaskp);
        ಹಿಂತಿರುಗಿ it != s_mtaskState.ಕೊನೆ() ? it->ಎರಡನೇ.threadId : UNASSIGNED;
    }
    ಸ್ಥಿರ uint32_t ಆರಂಭ_ಸಮಯ(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) {
        ಹಿಂತಿರುಗಿ s_mtaskState.at(mtaskp).completion_ಸಮಯ - mtaskp->ವೆಚ್ಚ();
    }
    ಸ್ಥಿರ uint32_t ಕೊನೆ_ಸಮಯ(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) {
        ಹಿಂತಿರುಗಿ s_mtaskState.at(mtaskp).completion_ಸಮಯ;
    }

    // Returns the number of cross-thread dependencies of the given MTask. If > 0, the MTask must
    // test whether its dependencies are ready before starting, and therefore may need to block.
    uint32_t crossThreadDependencies(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ uint32_t thisThreadId = threadId(mtaskp);
        uint32_t ಫಲಿತಾಂಶ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಒಳ_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ prevp = ಅಂಚು.fromp()->as<ExecMTask>();
            ಆದರೆ (threadId(prevp) != thisThreadId && ಒಳಗೊಂಡಿದೆ(prevp)) ++ಫಲಿತಾಂಶ;
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    uint32_t id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_id; }
    uint32_t ವೇಳಾಪಟ್ಟಿ_On(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp, uint32_t ಉತ್ತಮ_Thread_Id) {
        mtasks.emplace(mtaskp);
        ಸ್ಥಿರಾಂಕ uint32_t ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ = mtaskp->predict_ಆರಂಭ() + mtaskp->ವೆಚ್ಚ();
        m_ಕೊನೆ_ಸಮಯ = std::max(m_ಕೊನೆ_ಸಮಯ, ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ);
        s_mtaskState[mtaskp].completion_ಸಮಯ = ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ;
        s_mtaskState[mtaskp].threadId = ಉತ್ತಮ_Thread_Id;

        // Reference to thread in schedule we are assigning this MTask to.
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>& ಉತ್ತಮ_Thread = m_ಎಳೆಗಳು[ಉತ್ತಮ_Thread_Id];
        ಆದರೆ (!ಉತ್ತಮ_Thread.ಖಾಲಿಗಳು()) s_mtaskState[ಉತ್ತಮ_Thread.ಹಿಂಭಾಗ()].nextp = mtaskp;

        // Add the MTask to the schedule
        ಉತ್ತಮ_Thread.ತಳ್ಳು_ಹಿಂಭಾಗ(mtaskp);
        ಹಿಂತಿರುಗಿ ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ;
    }
    ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ(ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ mtasks.ಎಣಿಕೆ_2(mtaskp); }
    uint32_t ಕೊನೆ_ಸಮಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೊನೆ_ಸಮಯ; }
};

uint32_t Thread_ವೇಳಾಪಟ್ಟಿ::s_ಮುಂದಿನ_Id = 0;
std::unordered_map<ಸ್ಥಿರಾಂಕ ExecMTask*, Thread_ವೇಳಾಪಟ್ಟಿ::MTaskState> Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState{};
constexpr ದ್ವಿಗುಣ V3_Exec_ನಕ್ಷೆ::Thread_ವೇಳಾಪಟ್ಟಿ::s_thread_Box_ಅಗಲ;

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
ವರ್ಗ Pack_ಎಳೆಗಳು final {
    // TYPES
    ರಚನೆ MTaskCmp final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ExecMTask* ap, ಸ್ಥಿರಾಂಕ ExecMTask* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ap->id() < bp->id();
        }
    };

    // MEMBERS
    ಸ್ಥಿರಾಂಕ uint32_t m_n_ಎಳೆಗಳು;  // Number of threads
    ಸ್ಥಿರಾಂಕ uint32_t m_n_Hier_ಎಳೆಗಳು;  // Number of threads used for hierarchical tasks
    ಸ್ಥಿರಾಂಕ uint32_t m_sandbagNumerator;  // Numerator padding for est runtime
    ಸ್ಥಿರಾಂಕ uint32_t m_sandbagDenom;  // Denominator padding for est runtime

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Pack_ಎಳೆಗಳು(uint32_t n_ಎಳೆಗಳು = v3Global.opt.ಎಳೆಗಳು(),
                         uint32_t n_Hier_ಎಳೆಗಳು = v3Global.opt.hier_ಎಳೆಗಳು(),
                         ಚಿಹ್ನೆರಹಿತ sandbagNumerator = 30, ಚಿಹ್ನೆರಹಿತ sandbagDenom = 100)
        : m_n_ಎಳೆಗಳು{n_ಎಳೆಗಳು}
        , m_n_Hier_ಎಳೆಗಳು{n_Hier_ಎಳೆಗಳು}
        , m_sandbagNumerator{sandbagNumerator}
        , m_sandbagDenom{sandbagDenom} {}
    ~Pack_ಎಳೆಗಳು() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(Pack_ಎಳೆಗಳು);

    // METHODS
    uint32_t completion_ಸಮಯ(ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ, ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp,
                            uint32_t threadId) {
        // Ignore tasks that were scheduled on a different schedule
        ಆದರೆ (!ವೇಳಾಪಟ್ಟಿ.ಒಳಗೊಂಡಿದೆ(mtaskp)) ಹಿಂತಿರುಗಿ 0;
        ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ::MTaskState& state = ವೇಳಾಪಟ್ಟಿ.s_mtaskState.at(mtaskp);
        UASSERT(state.threadId != Thread_ವೇಳಾಪಟ್ಟಿ::UNASSIGNED, "Mtask should have assigned thread");
        ಆದರೆ (threadId == state.threadId) {
            // No overhead on same thread
            ಹಿಂತಿರುಗಿ state.completion_ಸಮಯ;
        }

        // Add some padding to the estimated runtime when looking from
        // another thread
        uint32_t sandbagged_ಕೊನೆ_ಸಮಯ
            = state.completion_ಸಮಯ + (m_sandbagNumerator * mtaskp->ವೆಚ್ಚ()) / m_sandbagDenom;

        // If task B is packed after task A on thread 0, don't let thread 1
        // think that A finishes earlier than thread 0 thinks that B
        // finishes, otherwise we get priority inversions and fail the self
        // test.
        ಆದರೆ (state.nextp) {
            ಸ್ಥಿರಾಂಕ uint32_t successor_ಕೊನೆ_ಸಮಯ
                = completion_ಸಮಯ(ವೇಳಾಪಟ್ಟಿ, state.nextp, state.threadId);
            ಆದರೆ ((sandbagged_ಕೊನೆ_ಸಮಯ >= successor_ಕೊನೆ_ಸಮಯ) && (successor_ಕೊನೆ_ಸಮಯ > 1)) {
                sandbagged_ಕೊನೆ_ಸಮಯ = successor_ಕೊನೆ_ಸಮಯ - 1;
            }
        }

        UINFO(6, "Sandbagged end time for " << mtaskp->name() << " on th " << threadId << " = "
                                            << sandbagged_ಕೊನೆ_ಸಮಯ);
        ಹಿಂತಿರುಗಿ sandbagged_ಕೊನೆ_ಸಮಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isReady(Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ, ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು_ಒಳ : mtaskp->ಒಳ_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ prevp = ಅಂಚು_ಒಳ.fromp()->as<ಸ್ಥಿರಾಂಕ ExecMTask>();
            ಆದರೆ (ವೇಳಾಪಟ್ಟಿ.threadId(prevp) == Thread_ವೇಳಾಪಟ್ಟಿ::UNASSIGNED) {
                // This predecessor is not assigned yet
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Pack an MTasks from given graph into m_nThreads threads, return the schedule.
    std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> pack(V3_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ) {
        std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> ಫಲಿತಾಂಶ;
        ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(Thread_ವೇಳಾಪಟ್ಟಿ{m_n_ಎಳೆಗಳು});

        // To support scheduling tasks that utilize more than one thread, we introduce a wide
        // task (ExecMTask with threads() > 1). Those tasks are scheduled on a separate thread
        // schedule to ensure that indexes for simulation-time thread pool workers are not shadowed
        // by another tasks.
        // For retaining control over thread schedules, we distinguish SchedulingModes:
        ಎಣಿಕೆ ವರ್ಗ Scheduling_ಬಹುಲಕ : uint8_t {
            SCHEDULING,  // Schedule normal tasks
            WIDE_TASK_DISCOVERED,  // Found a wide task, if this is the only one available,
                                   // switch to WIDE_TASK_SCHEDULING
            WIDE_TASK_SCHEDULING  // Schedule wide tasks
        };
        Scheduling_ಬಹುಲಕ ಬಹುಲಕ = Scheduling_ಬಹುಲಕ::SCHEDULING;

        // Time each thread is occupied until
        std::ಸದಿಶ<uint32_t> busyUntil(std::max(m_n_ಎಳೆಗಳು, m_n_Hier_ಎಳೆಗಳು), 0);

        // MTasks ready to be assigned next. All their dependencies are already assigned.
        std::ಗಣ<ExecMTask*, MTaskCmp> readyMTasks;
        ಪೂರ್ಣಾಂಕ maxThreadWorkers = 1;

        // Build initial ready list
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : mtask_ನಕ್ಷೆ.vertices()) {
            ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = vtx.as<ExecMTask>();
            ಆದರೆ (isReady(ಫಲಿತಾಂಶ.ಹಿಂಭಾಗ(), mtaskp)) readyMTasks.ಸೇರಿಸು(mtaskp);
            // TODO right now we schedule tasks assuming they take the same number of threads for
            // simplification.
            maxThreadWorkers = std::max(maxThreadWorkers, mtaskp->ಎಳೆಗಳು());
        }

        ಆಗ (!readyMTasks.ಖಾಲಿಗಳು()) {
            // For each task in the ready set, compute when it might start
            // on each thread (in that thread's local time frame.)
            uint32_t ಉತ್ತಮ_ಸಮಯ = 0xffffffff;
            uint32_t ಉತ್ತಮ_Thread_Id = 0;
            ExecMTask* ಉತ್ತಮ_Mtaskp = ಶೂನ್ಯಸೂಚಕ;  // Todo: const ExecMTask*
            Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ = ಫಲಿತಾಂಶ.ಹಿಂಭಾಗ();
            ಗಾಗಿ (uint32_t threadId = 0; threadId < ವೇಳಾಪಟ್ಟಿ.m_ಎಳೆಗಳು.ಗಾತ್ರ_2(); ++threadId) {
                ಗಾಗಿ (ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : readyMTasks) {
                    ಆದರೆ (ಬಹುಲಕ != Scheduling_ಬಹುಲಕ::WIDE_TASK_SCHEDULING && mtaskp->ಎಳೆಗಳು() > 1) {
                        ಬಹುಲಕ = Scheduling_ಬಹುಲಕ::WIDE_TASK_DISCOVERED;
                        ಮುಂದುವರಿ;
                    }
                    ಆದರೆ (ಬಹುಲಕ == Scheduling_ಬಹುಲಕ::WIDE_TASK_SCHEDULING && mtaskp->ಎಳೆಗಳು() <= 1)
                        ಮುಂದುವರಿ;

                    uint32_t ಸಮಯ_Begin = busyUntil[threadId];
                    ಆದರೆ (ಸಮಯ_Begin > ಉತ್ತಮ_ಸಮಯ) {
                        UINFO(6, "th " << threadId << " busy until " << ಸಮಯ_Begin
                                       << ", later than bestTime " << ಉತ್ತಮ_ಸಮಯ
                                       << ", skipping thread.");
                        ನಿಲ್ಲಿಸು;
                    }
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಒಳ_ಅಂಚುಗಳು()) {
                        ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ priorp = ಅಂಚು.fromp()->as<ExecMTask>();
                        ಸ್ಥಿರಾಂಕ uint32_t ಪೂರ್ವ_ಕೊನೆ_ಸಮಯ = completion_ಸಮಯ(ವೇಳಾಪಟ್ಟಿ, priorp, threadId);
                        ಆದರೆ (ಪೂರ್ವ_ಕೊನೆ_ಸಮಯ > ಸಮಯ_Begin) ಸಮಯ_Begin = ಪೂರ್ವ_ಕೊನೆ_ಸಮಯ;
                    }
                    UINFO(6, "Task " << mtaskp->name() << " start at " << ಸಮಯ_Begin
                                     << " on thread " << threadId);
                    ಆದರೆ ((ಸಮಯ_Begin < ಉತ್ತಮ_ಸಮಯ)
                        || ((ಸಮಯ_Begin == ಉತ್ತಮ_ಸಮಯ)
                            && ಉತ್ತಮ_Mtaskp  // Redundant, but appeases static analysis tools
                            && (mtaskp->ಆದ್ಯತೆ_2() > ಉತ್ತಮ_Mtaskp->ಆದ್ಯತೆ_2()))) {
                        ಉತ್ತಮ_ಸಮಯ = ಸಮಯ_Begin;
                        ಉತ್ತಮ_Thread_Id = threadId;
                        ಉತ್ತಮ_Mtaskp = mtaskp;
                    }
                }
            }

            ಸ್ಥಿರಾಂಕ uint32_t ಕೊನೆ_ಸಮಯ = ವೇಳಾಪಟ್ಟಿ.ಕೊನೆ_ಸಮಯ();

            ಆದರೆ (!ಉತ್ತಮ_Mtaskp && ಬಹುಲಕ == Scheduling_ಬಹುಲಕ::WIDE_TASK_DISCOVERED) {
                ಬಹುಲಕ = Scheduling_ಬಹುಲಕ::WIDE_TASK_SCHEDULING;
                ಸ್ಥಿರಾಂಕ uint32_t ಗಾತ್ರ_2 = m_n_Hier_ಎಳೆಗಳು / maxThreadWorkers;
                UASSERT(ಗಾತ್ರ_2, "Thread pool size should be bigger than 0");
                // If no tasks were added to the normal thread schedule, clear it.
                ಆದರೆ (ವೇಳಾಪಟ್ಟಿ.s_mtaskState.ಖಾಲಿಗಳು()) ಫಲಿತಾಂಶ.clear();
                ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(Thread_ವೇಳಾಪಟ್ಟಿ{ಗಾತ್ರ_2});
                std::fill(busyUntil.begin(), busyUntil.ಕೊನೆ(), ಕೊನೆ_ಸಮಯ);
                ಮುಂದುವರಿ;
            }

            ಆದರೆ (!ಉತ್ತಮ_Mtaskp && ಬಹುಲಕ == Scheduling_ಬಹುಲಕ::WIDE_TASK_SCHEDULING) {
                ಬಹುಲಕ = Scheduling_ಬಹುಲಕ::SCHEDULING;
                UASSERT(!ವೇಳಾಪಟ್ಟಿ.s_mtaskState.ಖಾಲಿಗಳು(), "Mtask should be added");
                ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(Thread_ವೇಳಾಪಟ್ಟಿ{m_n_ಎಳೆಗಳು});
                std::fill(busyUntil.begin(), busyUntil.ಕೊನೆ(), ಕೊನೆ_ಸಮಯ);
                ಮುಂದುವರಿ;
            }

            UASSERT(ಉತ್ತಮ_Mtaskp, "Should have found some task");

            ಉತ್ತಮ_Mtaskp->predict_ಆರಂಭ(ಉತ್ತಮ_ಸಮಯ);
            ಸ್ಥಿರಾಂಕ uint32_t ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ = ವೇಳಾಪಟ್ಟಿ.ವೇಳಾಪಟ್ಟಿ_On(ಉತ್ತಮ_Mtaskp, ಉತ್ತಮ_Thread_Id);
            busyUntil[ಉತ್ತಮ_Thread_Id] = ಉತ್ತಮ_ಕೊನೆ_ಸಮಯ;

            // Update the ready list
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t erased = readyMTasks.erase(ಉತ್ತಮ_Mtaskp);
            UASSERT_OBJ(erased > 0, ಉತ್ತಮ_Mtaskp, "Should have erased something?");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು_ಹೊರಹೋಗು : ಉತ್ತಮ_Mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ExecMTask* ಸ್ಥಿರಾಂಕ nextp = ಅಂಚು_ಹೊರಹೋಗು.ತುದಿ()->as<ExecMTask>();
                // Dependent MTask should not yet be assigned to a thread
                UASSERT(ವೇಳಾಪಟ್ಟಿ.threadId(nextp) == Thread_ವೇಳಾಪಟ್ಟಿ::UNASSIGNED,
                        "Tasks after one being assigned should not be assigned yet");
                // Dependent MTask should not be ready yet, since dependency is just being assigned
                UASSERT_OBJ(readyMTasks.ಕಂಡುಹಿಡಿ(nextp) == readyMTasks.ಕೊನೆ(), nextp,
                            "Tasks after one being assigned should not be ready");
                ಆದರೆ (isReady(ವೇಳಾಪಟ್ಟಿ, nextp)) {
                    readyMTasks.ಸೇರಿಸು(nextp);
                    UINFO(6, "Inserted " << nextp->name() << " into ready");
                }
            }
        }

        // All schedules are combined on a single graph
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 4)
            Thread_ವೇಳಾಪಟ್ಟಿ::dumpDotFilePrefixedAlways(ಫಲಿತಾಂಶ, "schedule", m_n_ಎಳೆಗಳು);

        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

ಸಾರ್ವಜನಿಕ:
    // SELF TEST
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        // Dummy trigger vector for the test graphs, which are never emitted
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, "__VtestTriggered",
                                        v3Global.rootp()->ಕಂಡುಹಿಡಿ_UInt64_DType()};
        AstScope* ಸ್ಥಿರಾಂಕ scopep = ಹೊಸ AstScope{flp, ಶೂನ್ಯಸೂಚಕ, "test", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        AstVarScope* ಸ್ಥಿರಾಂಕ trigVscp = ಹೊಸ AstVarScope{flp, scopep, varp};
        self_Test_Hier_ಮೊದಲ(trigVscp);
        self_Test_ಸಾಮಾನ್ಯ_ಮೊದಲ(trigVscp);
        VL_DO_DANGLING(trigVscp->delete_ವೃಕ್ಷ(), trigVscp);
        VL_DO_DANGLING(scopep->delete_ವೃಕ್ಷ(), scopep);
        VL_DO_DANGLING(varp->delete_ವೃಕ್ಷ(), varp);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ self_Test_ಸಾಮಾನ್ಯ_ಮೊದಲ(AstVarScope* trigVscp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
        Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp = ಹೊಸ Ast_Exec_ನಕ್ಷೆ{flp, "test", trigVscp};
        V3_ನಕ್ಷೆ& ನಕ್ಷೆ = *execGraphp->depGraphp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ makeBody = [&]() -> Ast_ಗಂಟು_Stmt* { ಹಿಂತಿರುಗಿ ಹೊಸ AstComment{flp, ""}; };
        ExecMTask* ಸ್ಥಿರಾಂಕ t0 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t0->ವೆಚ್ಚ(1000);
        t0->ಆದ್ಯತೆ_2(1100);
        ExecMTask* ಸ್ಥಿರಾಂಕ t1 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t1->ವೆಚ್ಚ(100);
        t1->ಆದ್ಯತೆ_2(100);
        ExecMTask* ಸ್ಥಿರಾಂಕ t2 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t2->ವೆಚ್ಚ(100);
        t2->ಆದ್ಯತೆ_2(100);
        t2->ಎಳೆಗಳು(2);
        ExecMTask* ಸ್ಥಿರಾಂಕ t3 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t3->ವೆಚ್ಚ(100);
        t3->ಆದ್ಯತೆ_2(100);
        t3->ಎಳೆಗಳು(3);
        ExecMTask* ಸ್ಥಿರಾಂಕ t4 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t4->ವೆಚ್ಚ(100);
        t4->ಆದ್ಯತೆ_2(100);
        t4->ಎಳೆಗಳು(3);
        ExecMTask* ಸ್ಥಿರಾಂಕ t5 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t5->ವೆಚ್ಚ(100);
        t5->ಆದ್ಯತೆ_2(100);
        ExecMTask* ಸ್ಥಿರಾಂಕ t6 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t6->ವೆಚ್ಚ(100);
        t6->ಆದ್ಯತೆ_2(100);

        /*
                          0
                         / \
                        1   2
                           / \
                          3   4
                         /    \
                        5      6
        */
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t0, t1, 1};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t0, t2, 1};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t2, t3, 1};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t2, t4, 1};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t3, t5, 1};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t4, t6, 1};

        constexpr uint32_t ಎಳೆಗಳು = 2;
        constexpr uint32_t hier_ಎಳೆಗಳು = 6;
        Pack_ಎಳೆಗಳು packer{ಎಳೆಗಳು, hier_ಎಳೆಗಳು,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> scheduled = packer.pack(ನಕ್ಷೆ);
        UASSERT_SELFTEST(scheduled.ಗಾತ್ರ_2(), 3);
        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(), ಎಳೆಗಳು);
        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[0].ಗಾತ್ರ_2(), 2);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < scheduled[0].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[i].ಗಾತ್ರ_2(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[0][0], t0);
        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[0][1], t1);

        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(), hier_ಎಳೆಗಳು / 3);
        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[0][0], t2);
        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[0][1], t3);
        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[1][0], t4);

        UASSERT_SELFTEST(scheduled[2].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(), ಎಳೆಗಳು);
        UASSERT_SELFTEST(scheduled[2].m_ಎಳೆಗಳು[0][0], t5);
        UASSERT_SELFTEST(scheduled[2].m_ಎಳೆಗಳು[1][0], t6);

        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState.ಗಾತ್ರ_2(), 7);

        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t1), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t2), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t3), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t4), 1);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t5), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t6), 1);

        // On its native thread, we see the actual end time for t0:
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t0, 0), 1000);
        // On the other thread, we see a sandbagged end time which does not
        // exceed the t1 end time:
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t0, 1), 1099);

        // Actual end time on native thread:
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t1, 0), 1100);
        // Sandbagged end time seen on thread 1.  Note it does not compound
        // with t0's sandbagged time; compounding caused trouble in
        // practice.
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t1, 1), 1130);

        // Wide task scheduling

        // Task does not depend on previous or future schedules
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t2, 0), 0);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[2], t2, 0), 0);

        // We allow sandbagging for hierarchical children tasks, this does not affect
        // wide task scheduling. When the next schedule is created it doesn't matter
        // anyway.
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 0), 1200);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 1), 1230);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 2), 1230);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 3), 1230);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 4), 1230);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t2, 5), 1230);

        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 0), 1300);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 2), 1330);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 3), 1330);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 4), 1330);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t3, 5), 1330);

        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 0), 1360);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 1), 1330);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 2), 1360);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 3), 1360);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 4), 1360);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t4, 5), 1360);

        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ವೃಕ್ಷ();
        VL_DO_DANGLING(execGraphp->delete_ವೃಕ್ಷ(), execGraphp);
        Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState.clear();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ self_Test_Hier_ಮೊದಲ(AstVarScope* trigVscp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
        Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp = ಹೊಸ Ast_Exec_ನಕ್ಷೆ{flp, "test", trigVscp};
        V3_ನಕ್ಷೆ& ನಕ್ಷೆ = *execGraphp->depGraphp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ makeBody = [&]() -> Ast_ಗಂಟು_Stmt* { ಹಿಂತಿರುಗಿ ಹೊಸ AstComment{flp, ""}; };
        ExecMTask* ಸ್ಥಿರಾಂಕ t0 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t0->ವೆಚ್ಚ(1000);
        t0->ಆದ್ಯತೆ_2(1100);
        t0->ಎಳೆಗಳು(2);
        ExecMTask* ಸ್ಥಿರಾಂಕ t1 = ಹೊಸ ExecMTask{execGraphp, ಶೂನ್ಯಸೂಚಕ, makeBody()};
        t1->ವೆಚ್ಚ(100);
        t1->ಆದ್ಯತೆ_2(100);

        /*
                          0
                          |
                          1
        */
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&ನಕ್ಷೆ, t0, t1, 1};

        constexpr uint32_t ಎಳೆಗಳು = 1;
        constexpr uint32_t hier_ಎಳೆಗಳು = 2;
        Pack_ಎಳೆಗಳು packer{ಎಳೆಗಳು, hier_ಎಳೆಗಳು,
                           3,  // Sandbag numerator
                           10};  // Sandbag denom

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> scheduled = packer.pack(ನಕ್ಷೆ);
        UASSERT_SELFTEST(scheduled.ಗಾತ್ರ_2(), 2);
        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(), hier_ಎಳೆಗಳು / 2);
        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[0].ಗಾತ್ರ_2(), 1);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < scheduled[0].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(); ++i)
            UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[i].ಗಾತ್ರ_2(), 0);

        UASSERT_SELFTEST(scheduled[0].m_ಎಳೆಗಳು[0][0], t0);

        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(), ಎಳೆಗಳು);
        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[0].ಗಾತ್ರ_2(), 1);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < scheduled[1].m_ಎಳೆಗಳು.ಗಾತ್ರ_2(); ++i)
            UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[i].ಗಾತ್ರ_2(), 0);
        UASSERT_SELFTEST(scheduled[1].m_ಎಳೆಗಳು[0][0], t1);

        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState.ಗಾತ್ರ_2(), 2);

        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t0), 0);
        UASSERT_SELFTEST(Thread_ವೇಳಾಪಟ್ಟಿ::threadId(t1), 0);

        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[0], t0, 0), 1000);

        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t1, 0), 1100);
        UASSERT_SELFTEST(packer.completion_ಸಮಯ(scheduled[1], t1, 1), 1130);

        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) vtx.as<ExecMTask>()->funcp()->delete_ವೃಕ್ಷ();
        VL_DO_DANGLING(execGraphp->delete_ವೃಕ್ಷ(), execGraphp);
        Thread_ವೇಳಾಪಟ್ಟಿ::s_mtaskState.clear();
    }

    ಸ್ಥಿರ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> apply(V3_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ) {
        ಹಿಂತಿರುಗಿ Pack_ಎಳೆಗಳು{}.pack(mtask_ನಕ್ಷೆ);
    }
};

ಬಳಸಿ EstimateAndProfiled = std::pair<uint64_t, uint64_t>;  // cost est, cost profiled
ಬಳಸಿ Costs = std::unordered_map<uint32_t, EstimateAndProfiled>;

ಶೂನ್ಯ normalizeCosts(Costs& costs) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ scale_ವೆಚ್ಚ = [](uint64_t ಮೌಲ್ಯ, ದ್ವಿಗುಣ multiplier) {
        ದ್ವಿಗುಣ scaled = static_cast<ದ್ವಿಗುಣ>(ಮೌಲ್ಯ) * multiplier;
        ಆದರೆ (ಮೌಲ್ಯ && scaled < 1) scaled = 1;
        ಹಿಂತಿರುಗಿ static_cast<uint64_t>(scaled);
    };

    // For all costs with a profile, compute sum
    uint64_t ಮೊತ್ತ_ವೆಚ್ಚ_Profiled = 0;  // For data with estimate and profile
    uint64_t ಮೊತ್ತ_ವೆಚ್ಚ_Estimate = 0;  // For data with estimate and profile
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& est : costs) {
        ಆದರೆ (est.ಎರಡನೇ.ಎರಡನೇ) {
            ಮೊತ್ತ_ವೆಚ್ಚ_Estimate += est.ಎರಡನೇ.ಮೊದಲ;
            ಮೊತ್ತ_ವೆಚ್ಚ_Profiled += est.ಎರಡನೇ.ಎರಡನೇ;
        }
    }

    ಆದರೆ (ಮೊತ್ತ_ವೆಚ್ಚ_Estimate) {
        // For data where we don't have profiled data, compute how much to
        // scale up/down the estimate to make on same relative scale as
        // profiled data.  (Improves results if only a few profiles missing.)
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ est_ಗಮ್ಯ_Profile
            = static_cast<ದ್ವಿಗುಣ>(ಮೊತ್ತ_ವೆಚ್ಚ_Profiled) / static_cast<ದ್ವಿಗುಣ>(ಮೊತ್ತ_ವೆಚ್ಚ_Estimate);
        UINFO(5, "Estimated data needs scaling by " << est_ಗಮ್ಯ_Profile
                                                    << ", sumCostProfiled=" << ಮೊತ್ತ_ವೆಚ್ಚ_Profiled
                                                    << " sumCostEstimate=" << ಮೊತ್ತ_ವೆಚ್ಚ_Estimate);
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& est : costs) {
            uint64_t& ವೆಚ್ಚ_Estimate = est.ಎರಡನೇ.ಮೊದಲ;
            ವೆಚ್ಚ_Estimate = scale_ವೆಚ್ಚ(ವೆಚ್ಚ_Estimate, est_ಗಮ್ಯ_Profile);
        }
    }

    // COSTS can overflow a uint32.  Using maximum value of costs, scale all down
    uint64_t max_ವೆಚ್ಚ = 0;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& est : costs) {
        ಸ್ಥಿರಾಂಕ uint64_t& ವೆಚ್ಚ_Estimate = est.ಎರಡನೇ.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ uint64_t& ವೆಚ್ಚ_Profiled = est.ಎರಡನೇ.ಎರಡನೇ;
        ಆದರೆ (max_ವೆಚ್ಚ < ವೆಚ್ಚ_Estimate) max_ವೆಚ್ಚ = ವೆಚ್ಚ_Estimate;
        ಆದರೆ (max_ವೆಚ್ಚ < ವೆಚ್ಚ_Profiled) max_ವೆಚ್ಚ = ವೆಚ್ಚ_Profiled;
        UINFO(9, "Post uint scale: ce = " << est.ಎರಡನೇ.ಮೊದಲ << " cp=" << est.ಎರಡನೇ.ಎರಡನೇ);
    }
    constexpr uint64_t scale_ಕೆಳಕ್ಕೆ_ಗಮ್ಯ = 10000000;  // Extra room for future algorithms to add costs
    ಆದರೆ (max_ವೆಚ್ಚ > scale_ಕೆಳಕ್ಕೆ_ಗಮ್ಯ) {
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ scaleup = static_cast<ದ್ವಿಗುಣ>(scale_ಕೆಳಕ್ಕೆ_ಗಮ್ಯ) / static_cast<ದ್ವಿಗುಣ>(max_ವೆಚ್ಚ);
        UINFO(5, "Scaling data to within 32-bits by multiply by=" << scaleup
                                                                  << ", maxCost=" << max_ವೆಚ್ಚ);
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& est : costs) {
            est.ಎರಡನೇ.ಮೊದಲ = scale_ವೆಚ್ಚ(est.ಎರಡನೇ.ಮೊದಲ, scaleup);
            est.ಎರಡನೇ.ಎರಡನೇ = scale_ವೆಚ್ಚ(est.ಎರಡನೇ.ಎರಡನೇ, scaleup);
        }
    }
}

ಶೂನ್ಯ remove_ಖಾಲಿಗಳು_MTasks(Ast_Exec_ನಕ್ಷೆ* execGraphp) {
    // Remove the calls to empty MTask functions, then the MTasks
    ಗಾಗಿ (AstCCall *callp = execGraphp->callsp(), *nextp; callp; callp = nextp) {
        nextp = VN_AS(callp->nextp(), CCall);
        ಆದರೆ (!callp->funcp()->stmtsp()) VL_DO_DANGLING(callp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), callp);
    }
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execMTaskGraphp = execGraphp->depGraphp();
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : execMTaskGraphp->vertices().unlinkable()) {
        ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = mtaskp->funcp();
        ಆದರೆ (funcp->stmtsp()) ಮುಂದುವರಿ;

        UINFO(6, "Removing empty MTask " << mtaskp->name());
        // Redirect edges
        mtaskp->reroute_ಅಂಚುಗಳು(execMTaskGraphp);
        // Delete the MTask function
        VL_DO_DANGLING(funcp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), funcp);
        // Delete the MTask vertex
        VL_DO_DANGLING(mtaskp->unlinkDelete(execMTaskGraphp), mtaskp);
    }
    // Remove redundant dependencies
    execMTaskGraphp->remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
}

ಶೂನ್ಯ fillinCosts(V3_ನಕ್ಷೆ* execMTaskGraphp) {
    // Pass 1: See what profiling data applies
    Costs costs;  // For each mtask, costs

    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ಸ್ಥಿರಾಂಕ mtp = vtx.as<ExecMTask>();
        // This estimate is 64 bits, but the final mtask graph algorithm needs 32 bits
        ಸ್ಥಿರಾಂಕ uint64_t ವೆಚ್ಚ_Estimate = V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ_2(mtp->funcp(), ಸುಳ್ಳು);
        ಸ್ಥಿರಾಂಕ uint64_t ವೆಚ್ಚ_Profiled
            = V3Control::get_Profile_ದತ್ತಾಂಶ(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ(), mtp->hashName());
        ಆದರೆ (ವೆಚ್ಚ_Profiled) {
            UINFO(5, "Profile data for mtask " << mtp->id() << " " << mtp->hashName()
                                               << " cost override " << ವೆಚ್ಚ_Profiled);
        }
        costs[mtp->id()] = std::make_pair(ವೆಚ್ಚ_Estimate, ವೆಚ್ಚ_Profiled);
    }

    normalizeCosts(costs /*ref*/);

    ಪೂರ್ಣಾಂಕ ಒಟ್ಟು_Estimates = 0;
    ಪೂರ್ಣಾಂಕ missingProfiles = 0;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : execMTaskGraphp->vertices()) {
        ExecMTask* ಸ್ಥಿರಾಂಕ mtp = vtx.as<ExecMTask>();
        ಸ್ಥಿರಾಂಕ uint32_t ವೆಚ್ಚ_Estimate = costs[mtp->id()].ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ uint64_t ವೆಚ್ಚ_Profiled = costs[mtp->id()].ಎರಡನೇ;
        UINFO(9, "ce = " << ವೆಚ್ಚ_Estimate << " cp=" << ವೆಚ್ಚ_Profiled);
        UASSERT(ವೆಚ್ಚ_Estimate <= (1UL << 31), "cost scaling math would overflow uint32");
        UASSERT(ವೆಚ್ಚ_Profiled <= (1UL << 31), "cost scaling math would overflow uint32");
        ಸ್ಥಿರಾಂಕ uint64_t ವೆಚ್ಚ_Profiled32 = static_cast<uint32_t>(ವೆಚ್ಚ_Profiled);
        uint32_t ವೆಚ್ಚ_ಗಮ್ಯ_Use = ವೆಚ್ಚ_Profiled32;
        ಆದರೆ (!ವೆಚ್ಚ_Profiled32) {
            ವೆಚ್ಚ_ಗಮ್ಯ_Use = ವೆಚ್ಚ_Estimate;
            ಆದರೆ (ವೆಚ್ಚ_Estimate != 0) ++missingProfiles;
        }
        ಆದರೆ (ವೆಚ್ಚ_Estimate != 0) ++ಒಟ್ಟು_Estimates;
        mtp->ವೆಚ್ಚ(ವೆಚ್ಚ_ಗಮ್ಯ_Use);
        mtp->ಆದ್ಯತೆ_2(ವೆಚ್ಚ_ಗಮ್ಯ_Use);
    }

    ಆದರೆ (missingProfiles) {
        ಆದರೆ (File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = V3Control::get_Profile_ದತ್ತಾಂಶ_File_ಸಾಲು()) {
            ಆದರೆ (V3Control::ಒಳಗೊಂಡಿದೆ_MTask_Profile_ದತ್ತಾಂಶ()) {
                fl->v3warn(PROFOUTOFDATE, "Profile data for mtasks may be out of date. "
                                              << missingProfiles << " of " << ಒಟ್ಟು_Estimates
                                              << " mtasks had no data");
            }
        }
    }
}

ಶೂನ್ಯ finalizeCosts(V3_ನಕ್ಷೆ* execMTaskGraphp) {
    ನಕ್ಷೆ_ಹರಿವು_Unordered ser(execMTaskGraphp, ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2);
    ಆಗ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vxp = ser.nextp()) {
        ExecMTask* ಸ್ಥಿರಾಂಕ mtp = const_cast<V3_ನಕ್ಷೆ_ಶೃಂಗ*>(vxp)->as<ExecMTask>();
        // "Priority" is the critical path from the start of the mtask, to
        // the end of the graph reachable from this mtask.  Given the
        // choice among several ready mtasks, we'll want to start the
        // highest priority one first, so we're always working on the "long
        // pole"
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ followp = ಅಂಚು.ತುದಿ()->as<ExecMTask>();
            ಆದರೆ ((followp->ಆದ್ಯತೆ_2() + mtp->ವೆಚ್ಚ()) > mtp->ಆದ್ಯತೆ_2()) {
                mtp->ಆದ್ಯತೆ_2(followp->ಆದ್ಯತೆ_2() + mtp->ವೆಚ್ಚ());
            }
        }
    }

    // Removing tasks may cause edges that were formerly non-transitive to
    // become transitive. Also we just created new edges around the removed
    // tasks, which could be transitive. Prune out all transitive edges.
    execMTaskGraphp->remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು();

    // Record summary stats for final m_tasks graph.
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ report = execMTaskGraphp->parallelismReport([](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp) {  //
        ಹಿಂತಿರುಗಿ vtxp->as<ಸ್ಥಿರಾಂಕ ExecMTask>()->ವೆಚ್ಚ();
    });
    V3Stats::ಕೂಡಿಸು_Stat("MTask graph, final, critical path cost", report.critical_ಮಾರ್ಗ_ವೆಚ್ಚ());
    V3Stats::ಕೂಡಿಸು_Stat("MTask graph, final, total graph cost", report.ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ());
    V3Stats::ಕೂಡಿಸು_Stat("MTask graph, final, mtask count", report.ಶೃಂಗ_ಎಣಿಕೆ());
    V3Stats::ಕೂಡಿಸು_Stat("MTask graph, final, edge count", report.ಅಂಚು_ಎಣಿಕೆ());
    V3Stats::ಕೂಡಿಸು_Stat("MTask graph, final, parallelism factor", report.parallelism_ಗುಣಕ());
    ಆದರೆ (debug() >= 3) {
        UINFO(0, "\n");
        UINFO(0, "    Final mtask parallelism report:");
        UINFO(0, "    Critical path cost = " << report.critical_ಮಾರ್ಗ_ವೆಚ್ಚ());
        UINFO(0, "    Total graph cost = " << report.ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ());
        UINFO(0, "    MTask vertex count = " << report.ಶೃಂಗ_ಎಣಿಕೆ());
        UINFO(0, "    Edge count = " << report.ಅಂಚು_ಎಣಿಕೆ());
        UINFO(0, "    Parallelism factor = " << report.parallelism_ಗುಣಕ());
    }
}

// Create an mtask state variable in the top module, waiting for 'nDependencies'
AstVar* newMTaskStateVar(ಸ್ಥಿರಾಂಕ std::string& name, uint32_t nDependencies) {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = modp->fileline();
    AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep
        = v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Basic_DType(fl, VBasicDTypeKwd::MTASKSTATE);
    AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, "__Vm_mtaskstate_" + name, dtypep};
    varp->isConst(ನಿಜ);
    varp->valuep(ಹೊಸ AstConst{fl, nDependencies});
    modp->ಕೂಡಿಸು_Stmtsp(varp);
    ಹಿಂತಿರುಗಿ varp;
}

// Statement calling 'method' on the given mtask state variable
AstCStmt* newMTaskStateCall(AstVar* varp, ಸ್ಥಿರಾಂಕ std::string& ವಿಧಾನ) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = varp->fileline();
    AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{fl, varp, VAccess::READWRITE};
    // Not 'this', the thread functions are static
    ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    refp->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::VlSyms{}, ತುದಿ_Scopep->nameDotless()});
    AstCStmt* ಸ್ಥಿರಾಂಕ stmtp = ಹೊಸ AstCStmt{fl};
    stmtp->ಕೂಡಿಸು(refp);
    stmtp->ಕೂಡಿಸು("." + ವಿಧಾನ + ";");
    ಹಿಂತಿರುಗಿ stmtp;
}

ಶೂನ್ಯ ಕೂಡಿಸು_MTask_ಗಮ್ಯ_Function(ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ, ಸ್ಥಿರಾಂಕ uint32_t threadId, AstCFunc* funcp,
                        ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp,
                        ಸ್ಥಿರಾಂಕ std::unordered_map<ಸ್ಥಿರಾಂಕ ExecMTask*, AstVar*>& stateVars) {
    AstScope* ಸ್ಥಿರಾಂಕ scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = modp->fileline();

    // Helper function to make the code a bit more legible
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_CStmt = [=](ಸ್ಥಿರಾಂಕ string& stmt) -> ಶೂನ್ಯ {  //
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, stmt});
    };

    ಆದರೆ (ವೇಳಾಪಟ್ಟಿ.crossThreadDependencies(mtaskp)) {
        // This mtask has dependencies executed on another thread, so it may block. Wait on the
        // task state variable to be notified.
        ಆದರೆ (v3Global.opt.profExec()) {
            ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
        }
        funcp->ಕೂಡಿಸು_Stmtsp(
            newMTaskStateCall(stateVars.at(mtaskp), "waitUntilUpstreamDone(even_cycle)"));
        ಆದರೆ (v3Global.opt.profExec()) {
            ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
        }
    }

    ಆದರೆ (v3Global.opt.profPgo()) {
        // No lock around startCounter, as counter numbers are unique per thread
        ಕೂಡಿಸು_CStmt("vlSymsp->_vm_pgoProfiler.startCounter(" + std::ಗಮ್ಯ_string(mtaskp->id()) + ");");
    }

    // Call the MTask function
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{fl, mtaskp->funcp()};
    callp->self_ಸೂಚಕ(VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::VlSyms{}, scopep->nameDotless()});
    callp->dtype_ಗಣ_Void();
    funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());

    ಆದರೆ (v3Global.opt.profPgo()) {
        // No lock around stopCounter, as counter numbers are unique per thread
        ಕೂಡಿಸು_CStmt("vlSymsp->_vm_pgoProfiler.stopCounter(" + std::ಗಮ್ಯ_string(mtaskp->id()) + ");");
    }

    // For any dependent mtask that's on another thread, signal one dependency completion.
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ nextp = ಅಂಚು.ತುದಿ()->as<ExecMTask>();
        ಆದರೆ (ವೇಳಾಪಟ್ಟಿ.threadId(nextp) != threadId && ವೇಳಾಪಟ್ಟಿ.ಒಳಗೊಂಡಿದೆ(nextp)) {
            funcp->ಕೂಡಿಸು_Stmtsp(
                newMTaskStateCall(stateVars.at(nextp), "signalUpstreamDone(even_cycle)"));
        }
    }
}

ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCFunc*> createThreadFunctions(ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ,
                                                   ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್) {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = v3Global.rootp()->ತುದಿ_Modulep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = modp->fileline();

    std::ಸದಿಶ<AstCFunc*> funcps;

    // Create the task state variables up front, as threads signal mtasks on other threads
    std::unordered_map<ಸ್ಥಿರಾಂಕ ExecMTask*, AstVar*> stateVars;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>& thread : ವೇಳಾಪಟ್ಟಿ.m_ಎಳೆಗಳು) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : thread) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint32_t nDependencies = ವೇಳಾಪಟ್ಟಿ.crossThreadDependencies(mtaskp)) {
                stateVars.emplace(mtaskp, newMTaskStateVar(cvt_ಗಮ್ಯ_Str(mtaskp->id()), nDependencies));
            }
        }
    }

    // For each thread, create a function representing its entry point
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ ExecMTask*>& thread : ವೇಳಾಪಟ್ಟಿ.m_ಎಳೆಗಳು) {
        ಆದರೆ (thread.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ uint32_t threadId = ವೇಳಾಪಟ್ಟಿ.threadId(thread.front());
        ಸ್ಥಿರಾಂಕ string name{"__Vthread__" + ಟ್ಯಾಗ್ + "__s" + cvt_ಗಮ್ಯ_Str(ವೇಳಾಪಟ್ಟಿ.id()) + "__t"
                          + cvt_ಗಮ್ಯ_Str(threadId)};
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{fl, name, ಶೂನ್ಯಸೂಚಕ, "void"};
        modp->ಕೂಡಿಸು_Stmtsp(funcp);
        funcps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
        funcp->isStatic(ನಿಜ);  // Uses void self pointer, so static and hand rolled
        funcp->isLoose(ನಿಜ);
        funcp->ನಮೂದು_ಬಿಂದು(ನಿಜ);
        funcp->argTypes("void* voidSelf, bool even_cycle");

        // Setup vlSelf and vlSyms
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, EmitCUtil::void_Self_ನಿಯೋಜಿಸು(modp)});
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, EmitCUtil::sym_Class_ನಿಯೋಜಿಸು()});

        // Invoke each mtask scheduled to this thread from the thread function
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp : thread) {
            ಕೂಡಿಸು_MTask_ಗಮ್ಯ_Function(ವೇಳಾಪಟ್ಟಿ, threadId, funcp, mtaskp, stateVars);
        }
    }

    ಹಿಂತಿರುಗಿ funcps;
}

ಶೂನ್ಯ ಕೂಡಿಸು_Thread_ಆರಂಭ_Wrapper(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    // FileLine used for constructing nodes below
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = v3Global.rootp()->fileline();
    ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್ = execGraphp->name();

    // Add thread function invocations to execGraph
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_CStmt = [=](ಸ್ಥಿರಾಂಕ string& stmt) -> ಶೂನ್ಯ {  //
        execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, stmt});
    };

    ಆದರೆ (v3Global.opt.profExec()) {
        ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphBegin();");
    }

    ಕೂಡಿಸು_CStmt("vlSymsp->__Vm_even_cycle__" + ಟ್ಯಾಗ್ + " = !vlSymsp->__Vm_even_cycle__" + ಟ್ಯಾಗ್ + ";");

    ಆದರೆ (!v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು()) ಕೂಡಿಸು_CStmt("std::vector<size_t> indexes;");
}

ಶೂನ್ಯ ಕೂಡಿಸು_Thread_ಕೊನೆ_Wrapper(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    // Add thread function invocations to execGraph
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_CStmt = [=](ಸ್ಥಿರಾಂಕ string& stmt) -> ಶೂನ್ಯ {  //
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
        execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, stmt});
    };

    ಕೂಡಿಸು_CStmt("Verilated::mtaskId(0);");
    ಆದರೆ (v3Global.opt.profExec()) { ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).execGraphEnd();"); }
}
ಶೂನ್ಯ ಕೂಡಿಸು_Thread_ಆರಂಭ_ಗಮ್ಯ_Exec_ನಕ್ಷೆ(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp,
                               ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCFunc*>& funcps, uint32_t ವೇಳಾಪಟ್ಟಿ_Id) {
    // FileLine used for constructing nodes below
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = v3Global.rootp()->fileline();
    ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್ = execGraphp->name();

    // The fake "final" mtask state variable, signaled by each thread when finished
    AstVar* ಸ್ಥಿರಾಂಕ finalVarp
        = newMTaskStateVar("final__" + cvt_ಗಮ್ಯ_Str(ವೇಳಾಪಟ್ಟಿ_Id) + "__" + ಟ್ಯಾಗ್, funcps.ಗಾತ್ರ_2());
    ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcps) {
        funcp->ಕೂಡಿಸು_Stmtsp(newMTaskStateCall(finalVarp, "signalUpstreamDone(even_cycle)"));
    }

    // Add thread function invocations to execGraph
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_CStmt = [=](ಸ್ಥಿರಾಂಕ string& stmt) -> ಶೂನ್ಯ {  //
        execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, stmt});
    };

    ಸ್ಥಿರಾಂಕ uint32_t ಕೊನೆಯ = funcps.ಗಾತ್ರ_2() - 1;
    ಆದರೆ (!v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು() && ಕೊನೆಯ > 0) {
        ಕೂಡಿಸು_CStmt("for (size_t i = 0; i < " + std::ಗಮ್ಯ_string(ಕೊನೆಯ) + "; ++i) {\n"  //
                 + "indexes.push_back(vlSymsp->__Vm_threadPoolp->assignWorkerIndex());\n"  //
                 + "}");
    }
    uint32_t i = 0;
    ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : funcps) {
        ಆದರೆ (i != ಕೊನೆಯ) {
            // The first N-1 will run on the thread pool.
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{fl};
            execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(cstmtp);
            cstmtp->ಕೂಡಿಸು("vlSymsp->__Vm_threadPoolp->workerp(");
            ಆದರೆ (v3Global.opt.hier_ಮಗು() || !v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು()) {
                cstmtp->ಕೂಡಿಸು("indexes[" + std::ಗಮ್ಯ_string(i) + "]");
            } ಇಲ್ಲದಿದ್ದರೆ {
                cstmtp->ಕೂಡಿಸು(std::ಗಮ್ಯ_string(i));
            }
            cstmtp->ಕೂಡಿಸು(")->addTask(");
            cstmtp->ಕೂಡಿಸು(ಹೊಸ Ast_Addr_ದ_CFunc{fl, funcp});
            cstmtp->ಕೂಡಿಸು(", vlSelf, vlSymsp->__Vm_even_cycle__" + ಟ್ಯಾಗ್ + ");");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // The last will run on the main thread.
            AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{fl, funcp};
            callp->dtype_ಗಣ_Void();
            callp->argTypes("vlSelf, vlSymsp->__Vm_even_cycle__" + ಟ್ಯಾಗ್);
            execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        }
        ++i;
    }
    V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Thread schedule total tasks", i);

    ಆದರೆ (v3Global.opt.profExec()) {
        ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitBegin();");
    }
    execGraphp->runfuncp()->ಕೂಡಿಸು_Stmtsp(newMTaskStateCall(
        finalVarp, "waitUntilUpstreamDone(vlSymsp->__Vm_even_cycle__" + ಟ್ಯಾಗ್ + ")"));
    ಆದರೆ (v3Global.opt.profExec()) {
        ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).threadScheduleWaitEnd();");
    }
    // Free all assigned worker indices in this section
    ಆದರೆ (!v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು() && ಕೊನೆಯ > 0) {
        ಕೂಡಿಸು_CStmt("vlSymsp->__Vm_threadPoolp->freeWorkerIndexes(indexes);");
    }
}

ಶೂನ್ಯ processMTaskBodies(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : execGraphp->depGraphp()->vertices().unlinkable()) {
        ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = vtxp->as<ExecMTask>();
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = mtaskp->funcp();
        // Temporarily unlink function body so we can add more statemetns
        Ast_ಗಂಟು* stmtsp = funcp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();

        // Helper function to make the code a bit more legible
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_CStmt = [=](ಸ್ಥಿರಾಂಕ string& stmt) -> ಶೂನ್ಯ {  //
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{execGraphp->fileline(), stmt});
        };

        // Profiling mtaskStart
        ಆದರೆ (v3Global.opt.profExec()) {
            std::string ವಾದಗಳು = std::ಗಮ್ಯ_string(mtaskp->id());
            ವಾದಗಳು += ", " + std::ಗಮ್ಯ_string(mtaskp->predict_ಆರಂಭ());
            ವಾದಗಳು += ", \"";
            ಆದರೆ (v3Global.opt.hier_ಮಗು()) ವಾದಗಳು += v3Global.opt.ತುದಿ_Module();
            ವಾದಗಳು += "\"";
            ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskBegin(" + ವಾದಗಳು + ");");
        }
        // Set mtask ID in the run-time system
        ಕೂಡಿಸು_CStmt("Verilated::mtaskId(" + std::ಗಮ್ಯ_string(mtaskp->id()) + ");");
        // Add back the body
        funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);
        // Flush message queue
        ಕೂಡಿಸು_CStmt("Verilated::endOfThreadMTask(vlSymsp->__Vm_evalMsgQp);");
        // Profiling mtaskEnd
        ಆದರೆ (v3Global.opt.profExec()) {
            ಸ್ಥಿರಾಂಕ std::string& ವಾದಗಳು = std::ಗಮ್ಯ_string(mtaskp->ವೆಚ್ಚ());
            ಕೂಡಿಸು_CStmt("VL_EXEC_TRACE_ADD_RECORD(vlSymsp).mtaskEnd(" + ವಾದಗಳು + ");");
        }
    }
}

ಶೂನ್ಯ implement_Exec_ನಕ್ಷೆ(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp, ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ) {
    // Nothing to be done if there are no MTasks in the graph at all.
    ಆದರೆ (execGraphp->depGraphp()->ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

    // Create a function to be run by each thread.
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCFunc*>& funcps = createThreadFunctions(ವೇಳಾಪಟ್ಟಿ, execGraphp->name());
    UASSERT(!funcps.ಖಾಲಿಗಳು(), "Non-empty ExecGraph yields no threads?");

    // Start the thread functions from the function that dispatches the graph.
    ಕೂಡಿಸು_Thread_ಆರಂಭ_ಗಮ್ಯ_Exec_ನಕ್ಷೆ(execGraphp, funcps, ವೇಳಾಪಟ್ಟಿ.id());
}

ಶೂನ್ಯ create_ಸತತ_Function(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    // Create the function that dispatches the graph to the thread pool, as scheduled statically
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = execGraphp->fileline();
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = v3Global.rootp()->ತುದಿ_Modulep();
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{flp, "runExecGraph_" + execGraphp->name(), ಶೂನ್ಯಸೂಚಕ};
    funcp->isLoose(ನಿಜ);
    funcp->voidSelf(ನಿಜ);  // Called through VlExecGraph::Fnp
    funcp->dontCombine(ನಿಜ);
    modp->ಕೂಡಿಸು_Stmtsp(funcp);
    execGraphp->runfuncp(funcp);
}

ಶೂನ್ಯ ಕೂಡಿಸು_Trigger_Masks(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    // Add to 'maskps' a constant table for each MTask in 'callsp', with the trigger bits its
    // code tests
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = execGraphp->fileline();
    AstVar* ಸ್ಥಿರಾಂಕ triggersp = execGraphp->triggerp()->varp();
    ಗಾಗಿ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        std::map<uint32_t, uint64_t> ಶಬ್ದ_Masks;
        callp->funcp()->foreach([&](ಸ್ಥಿರಾಂಕ AstTriggered* trigp) {
            UASSERT_OBJ(trigp->trigRefp()->varp() == triggersp, trigp,
                        "MTask tests another trigger vector");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ : trigp->indices()) {
                ಶಬ್ದ_Masks[VL_BITWORD_Q(ಸೂಚ್ಯಂಕ)] |= VL_ಬಿಟ್_Q(ಸೂಚ್ಯಂಕ);
            }
        });
        AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{flp, triggersp->dtypep(),
                                                     ಹೊಸ AstConst{flp, AstConst::Unsized64{}, 0}};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಶಬ್ದ_ಮುಖವಾಡ : ಶಬ್ದ_Masks) {
            consp->valuep(ಶಬ್ದ_ಮುಖವಾಡ.ಮೊದಲ,
                          ಹೊಸ AstConst{flp, AstConst::Unsized64{}, ಶಬ್ದ_ಮುಖವಾಡ.ಎರಡನೇ});
        }
        execGraphp->ಕೂಡಿಸು_Maskps(V3ConstPool::ಕಂಡುಹಿಡಿ(consp));
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);
    }
}

ಶೂನ್ಯ ಕೂಡಿಸು_Trigger_Costs(Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp) {
    // Set 'triggerCostsp' to a constant table with the static cost of the logic each trigger
    // guards in the MTasks
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = execGraphp->fileline();
    ಸ್ಥಿರಾಂಕ uint32_t nTriggers
        = VN_AS(execGraphp->triggerp()->varp()->dtypep(), UnpackArrayDType)->elementsConst()
          * VL_QUADSIZE;
    std::ಸದಿಶ<uint32_t> costs(nTriggers, 0);
    ಗಾಗಿ (AstCCall* callp = execGraphp->callsp(); callp; callp = VN_AS(callp->nextp(), CCall)) {
        callp->funcp()->foreach([&](AstIf* ifp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTriggered* ಸ್ಥಿರಾಂಕ trigp = VN_CAST(ifp->condp(), Triggered)) {
                ಸ್ಥಿರಾಂಕ uint32_t ವೆಚ್ಚ = V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ_2(ifp, ಸುಳ್ಳು);
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ : trigp->indices()) costs.at(ಸೂಚ್ಯಂಕ) += ವೆಚ್ಚ;
            }
        });
    }
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep
        = ಹೊಸ AstUnpackArrayDType{flp, netlistp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(32, 32, VSigning::UNSIGNED),
                                  ಹೊಸ AstRange{flp, static_cast<ಪೂರ್ಣಾಂಕ>(nTriggers - 1), 0}};
    netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(dtypep);
    AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{flp, dtypep, ಹೊಸ AstConst{flp, 0}};
    ಗಾಗಿ (uint32_t ಸೂಚ್ಯಂಕ = 0; ಸೂಚ್ಯಂಕ < nTriggers; ++ಸೂಚ್ಯಂಕ) {
        ಆದರೆ (costs[ಸೂಚ್ಯಂಕ]) consp->valuep(ಸೂಚ್ಯಂಕ, ಹೊಸ AstConst{flp, costs[ಸೂಚ್ಯಂಕ]});
    }
    execGraphp->triggerCostsp(V3ConstPool::ಕಂಡುಹಿಡಿ(consp));
    VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);
}

// Called by Verilator top stage
ಶೂನ್ಯ implement(AstNetlist* netlistp) {
    // Gather all ExecGraphs
    std::ಸದಿಶ<Ast_Exec_ನಕ್ಷೆ*> execGraphps;
    netlistp->ತುದಿ_Modulep()->foreach([&](Ast_Exec_ನಕ್ಷೆ* egp) { execGraphps.emplace_ಹಿಂಭಾಗ(egp); });

    // Process each
    ಗಾಗಿ (Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp : execGraphps) {
        // Some MTasks may have become empty after scheduling due to
        // optimizations after scheduling. Remove those.
        remove_ಖಾಲಿಗಳು_MTasks(execGraphp);

        // In some very small test cases, we might end up with a completely
        // empty ExecGraph, if so just delete it.
        ಆದರೆ (execGraphp->depGraphp()->ಖಾಲಿಗಳು()) {
            VL_DO_DANGLING(execGraphp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), execGraphp);
            ಹಿಂತಿರುಗಿ;
        }

        // Back in V3Order, we partitioned mtasks using provisional cost
        // estimates. However, V3Order precedes some optimizations (notably
        // V3LifePost) that can change the cost of logic within each mtask.
        // Now that logic is final, recompute the cost and priority of each
        // ExecMTask.
        fillinCosts(execGraphp->depGraphp());
        finalizeCosts(execGraphp->depGraphp());

        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 4) execGraphp->depGraphp()->dumpDotFilePrefixedAlways("pack");

        create_ಸತತ_Function(execGraphp);
        ಕೂಡಿಸು_Thread_ಆರಂಭ_Wrapper(execGraphp);

        // Schedule the mtasks: statically associate each mtask with a thread,
        // and determine the order in which each thread will run its mtasks.
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Thread_ವೇಳಾಪಟ್ಟಿ> packed = Pack_ಎಳೆಗಳು::apply(*execGraphp->depGraphp());
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, Thread schedule count",
                            static_cast<ದ್ವಿಗುಣ>(packed.ಗಾತ್ರ_2()));

        // Process MTask function bodies to add additional code
        processMTaskBodies(execGraphp);

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Thread_ವೇಳಾಪಟ್ಟಿ& ವೇಳಾಪಟ್ಟಿ : packed) {
            // Replace the graph body with its multi-threaded implementation.
            implement_Exec_ನಕ್ಷೆ(execGraphp, ವೇಳಾಪಟ್ಟಿ);
        }

        ಕೂಡಿಸು_Thread_ಕೊನೆ_Wrapper(execGraphp);

        ಕೂಡಿಸು_Trigger_Masks(execGraphp);
        ಕೂಡಿಸು_Trigger_Costs(execGraphp);
    }
}

ಶೂನ್ಯ selfTest() {
    {  // Test that omitted profile data correctly scales estimates
        Costs costs({// id  est  prof
                     {1, {10, 1000}},
                     {2, {20, 0}},  // Note no profile
                     {3, {30, 3000}}});
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].ಮೊದಲ, 1000);
        UASSERT_SELFTEST(costs[1].ಎರಡನೇ, 1000);
        UASSERT_SELFTEST(costs[2].ಮೊದಲ, 2000);
        UASSERT_SELFTEST(costs[2].ಎರಡನೇ, 0);
        UASSERT_SELFTEST(costs[3].ಮೊದಲ, 3000);
        UASSERT_SELFTEST(costs[3].ಎರಡನೇ, 3000);
    }
    {  // Test that very large profile data properly scales
        Costs costs({// id  est  prof
                     {1, {10, 100000000000}},
                     {2, {20, 200000000000}},
                     {3, {30, 1}}});  // Make sure doesn't underflow
        normalizeCosts(costs);
        UASSERT_SELFTEST(costs[1].ಮೊದಲ, 2500000);
        UASSERT_SELFTEST(costs[1].ಎರಡನೇ, 5000000);
        UASSERT_SELFTEST(costs[2].ಮೊದಲ, 5000000);
        UASSERT_SELFTEST(costs[2].ಎರಡನೇ, 10000000);
        UASSERT_SELFTEST(costs[3].ಮೊದಲ, 7500000);
        UASSERT_SELFTEST(costs[3].ಎರಡನೇ, 1);
    }

    Pack_ಎಳೆಗಳು::selfTest();
}

}  // namespace V3ExecGraph
