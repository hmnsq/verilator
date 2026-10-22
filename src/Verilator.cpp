// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: main()
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

#تعریف_کړه VL_MT_CONTROL_کوډ_UNIT 1

#ضمن "V3Active.h"
#ضمن "V3ActiveTop.h"
#ضمن "V3Assert.h"
#ضمن "V3AssertNfa.h"
#ضمن "V3AssertPre.h"
#ضمن "V3Ast.h"
#ضمن "V3AstPatterns.h"
#ضمن "V3Begin.h"
#ضمن "V3BitBlast.h"
#ضمن "V3Branch.h"
#ضمن "V3Broken.h"
#ضمن "V3CCtors.h"
#ضمن "V3CUse.h"
#ضمن "V3Case.h"
#ضمن "V3Cast.h"
#ضمن "V3Class.h"
#ضمن "V3Clean.h"
#ضمن "V3Clock.h"
#ضمن "V3Combine.h"
#ضمن "V3Common.h"
#ضمن "V3Const.h"
#ضمن "V3Control.h"
#ضمن "V3Coverage.h"
#ضمن "V3CoverageJoin.h"
#ضمن "V3Covergroup.h"
#ضمن "V3Dead.h"
#ضمن "V3Decompose.h"
#ضمن "V3Delayed.h"
#ضمن "V3Depth.h"
#ضمن "V3DepthBlock.h"
#ضمن "V3Descope.h"
#ضمن "V3DfgOptimizer.h"
#ضمن "V3DiagSarif.h"
#ضمن "V3EmitC.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3EmitCMain.h"
#ضمن "V3EmitMk.h"
#ضمن "V3EmitMkJson.h"
#ضمن "V3EmitV.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Expand.h"
#ضمن "V3File.h"
#ضمن "V3Force.h"
#ضمن "V3Fork.h"
#ضمن "V3FsmDetect.h"
#ضمن "V3FuncOpt.h"
#ضمن "V3Gate.h"
#ضمن "V3Global.h"
#ضمن "V3Graph.h"
#ضمن "V3HashTable.h"
#ضمن "V3HierBlock.h"
#ضمن "V3Inline.h"
#ضمن "V3InlineCFuncs.h"
#ضمن "V3Inst.h"
#ضمن "V3Interface.h"
#ضمن "V3LibMap.h"
#ضمن "V3Life.h"
#ضمن "V3LifePost.h"
#ضمن "V3LiftExpr.h"
#ضمن "V3LinkDot.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3LinkInc.h"
#ضمن "V3LinkJump.h"
#ضمن "V3LinkLValue.h"
#ضمن "V3LinkLevel.h"
#ضمن "V3LinkParse.h"
#ضمن "V3LinkResolve.h"
#ضمن "V3LinkWith.h"
#ضمن "V3Localize.h"
#ضمن "V3MergeCond.h"
#ضمن "V3Name.h"
#ضمن "V3Order.h"
#ضمن "V3Os.h"
#ضمن "V3Param.h"
#ضمن "V3PreShell.h"
#ضمن "V3Premit.h"
#ضمن "V3ProtectLib.h"
#ضمن "V3RandSequence.h"
#ضمن "V3Randomize.h"
#ضمن "V3Reloop.h"
#ضمن "V3Reorder.h"
#ضمن "V3Sampled.h"
#ضمن "V3Sched.h"
#ضمن "V3Scope.h"
#ضمن "V3Slice.h"
#ضمن "V3Split.h"
#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3Subst.h"
#ضمن "V3Table.h"
#ضمن "V3Task.h"
#ضمن "V3ThreadPool.h"
#ضمن "V3Timing.h"
#ضمن "V3Trace.h"
#ضمن "V3TraceDecl.h"
#ضمن "V3Tristate.h"
#ضمن "V3Udp.h"
#ضمن "V3Undriven.h"
#ضمن "V3Unknown.h"
#ضمن "V3Unroll.h"
#ضمن "V3VariableOrder.h"
#ضمن "V3Waiver.h"
#ضمن "V3Width.h"
#ضمن "V3WidthCommit.h"

#ضمن <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

ساكن فراغ reportStatsIfEnabled() {
    إذا (v3Global.opt.stats()) {
        File_کرښه::stats();
        V3Stats::statsReport();
    }
}

ساكن فراغ emitJson() VL_MT_DISABLED {
    ثابت string filename
        = (v3Global.opt.jsonOnlyOutput().تشه()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_ونه_Json_File(filename);
}

ساكن فراغ emitSerialized() VL_MT_DISABLED {
    إذا (v3Global.opt.jsonOnly()) emitJson();
}

ساكن فراغ process() {
    {
        ثابت VlOs::Delta_Wall_وخت elab_Wall_وخت{ریښتیا};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::mod_ترتیبول_By_Level();
        V3Error::abortIfErrors();
        إذا (v3Global.opt.debugExitParse()) {
            وتنه << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_ونه();  // Force a check, as link is most likely place for problems
        // Check if all parameters have been found
        v3Global.opt.checkParameters();
        // Correct state we couldn't know at parse time, repair SEL's
        V3LinkResolve::linkResolve(v3Global.rootp());
        // Set Lvalue's in variable refs
        V3LinkLValue::linkLValue(v3Global.rootp());
        // Convert return/continue/disable to jumps
        V3LinkJump::linkJump(v3Global.rootp());
        // Convert --/++ to normal operations. Must be after LinkJump.
        V3LinkInc::linkIncrements(v3Global.rootp());
        V3Error::abortIfErrors();

        إذا (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

        // Remove parameters by cloning modules to de-parameterized versions
        //   This requires some width calculations and constant propagation
        // No more AstGenCase/AstGenFor/AstGenIf after this
        V3Param::param(v3Global.rootp());

        V3LinkDot::linkDotParamed(v3Global.rootp());  // Cleanup as made new modules
        V3Param::finalizeDeferredParams(v3Global.rootp());
        V3LinkLValue::linkLValue(v3Global.rootp());  // Resolve new VarRefs

        // Link cleanup of 'with' as final link phase before V3Width
        // (called only once, not when width a single params)
        V3LinkWith::linkWith(v3Global.rootp());

        V3Error::abortIfErrors();

        // Fix any remaining cross-interface refs created during V3Width::widthParamsEdit
        // that weren't captured earlier. Must run before V3Dead deletes template modules.
        V3LinkDotIfaceCapture::finalizeIfaceCapture();

        // Remove any modules that were parameterized and are no longer referenced.
        V3Dead::deadifyModules(v3Global.rootp());

        v3Global.check_ونه();
        إذا (v3Global.has_جدول()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.پوړیز() && !v3Global.opt.hier_ماشوم()) {
            V3_پوړیز::create_ګراف(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            إذا (v3Global.hierGraphp()) أرجع;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3_پلنوالی::پلنوالی(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3_پلنوالی_Commit::پلنوالی_Commit(v3Global.rootp());
        v3Global.assertDTypesResolved(ریښتیا);
        v3Global.پلنوالی_Min_Usage(VWidthMinUsage::MATCHES_پلنوالی);

        // End of elaboration
        V3Stats::جمع_کړه_Stat_Perf(V3Stats::STAT_WALLTIME_ELAB, elab_Wall_وخت.delta_وخت());
        ثابت VlOs::Delta_Wall_وخت cvt_Wall_وخت{ریښتیا};
        إذا (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            إذا (v3Global.opt.serializeOnly()) emitSerialized();
            وتنه << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        إذا (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_سر(v3Global.rootp());
        } وإلا {
            V3LinkLevel::non_Wrap_سر(v3Global.rootp());
        }

        إذا (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            إذا (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        إذا (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        إذا (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        إذا (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constify_All_ژوندی(v3Global.rootp());

        // Signal based lint checks, no change to structures
        // Must be before first constification pass drops dead code
        V3Undriven::undrivenAll(v3Global.rootp());

        // Assertion insertion
        //    After we've added block coverage, but before other nasty transforms
        V3AssertCommon::collectDefaultDisable(v3Global.rootp());
        V3AssertCommon::ښکتنی_Sequence_Events(v3Global.rootp());
        V3AssertNfa::assertNfaAll(v3Global.rootp());
        // V3AssertProp removed: NFA subsumes multi-cycle property lowering.
        // Unsupported constructs fall through to V3AssertPre.
        V3AssertPre::assertPreAll(v3Global.rootp());
        //
        V3Assert::assertAll(v3Global.rootp());

        // Propagate constants into expressions
        إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        إذا (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            إذا (v3Global.opt.timing().is_سېټ_True()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(ریښتیا);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        إذا (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_بیان::lift_بیان_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(ریښتیا);
        }

        // Interface references feed trace file aliases and VPI name resolution
        إذا (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_ونه();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_ونه();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        إذا (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            إذا (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            إذا (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        إذا (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ونه();
        }

        إذا (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        إذا (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        إذا (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3_ټوټه::ټوټه_All(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3_فعال::فعال_All(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            إذا (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            إذا (v3Global.opt.fBitblast()) V3_بټ_Blast::بټ_Blast_All(v3Global.rootp());

            إذا (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            إذا (!v3Global.opt.lintOnly() && v3Global.opt.f_جدول()) {
                V3_جدول::جدول_All(v3Global.rootp());
            }

            // Cleanup
            إذا (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ونه();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            إذا (v3Global.opt.f_وېشل()) V3_وېشل::وېشل_All(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            إذا (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::force_And_ټاکل_All(v3Global.rootp());

            // DFG optimization
            إذا (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            إذا (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } وإلا {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            إذا (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            إذا (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            إذا (v3Global.opt.timing().is_سېټ_True()) {
                // Convert AST for timing if requested
                // Needs to be after V3Gate, as that step modifies sentrees
                // Needs to be before V3Delayed, as delayed assignments are handled differently in
                // suspendable processes
                V3Timing::timingAll(v3Global.rootp());
            }

            // Create delayed assignments
            // This creates lots of duplicate ACTIVES so ActiveTop needs to be after this step.
            V3Delayed::delayedAll(v3Global.rootp());

            // Make Active's on the top level.
            // Differs from V3Active, because identical clocks may be pushed
            // down to a module and now be identical
            V3_فعال_سر::فعال_سر_All(v3Global.rootp());

            // Remove SAMPLED
            إذا (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            إذا (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::مهالوېش(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            إذا (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            إذا (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            إذا (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            إذا (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        إذا (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            إذا (!v3Global.opt.lintOnly() && v3Global.opt.comp_حد_بلاکونه()) {
                V3_ژوروالی_بلاک::ژوروالی_بلاک_All(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(خطأ);

            // Move variables from modules to function local variables where possible
            إذا (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            إذا (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        إذا (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            إذا (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            إذا (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

            // Here down, widthMin() is the Verilog width, and width() is the C++ width
            // Bits between widthMin() and width() are irrelevant, but may be non-zero.
            v3Global.پلنوالی_Min_Usage(VWidthMinUsage::VERILOG_پلنوالی);

            // Make all expressions 32, 64, or 32*N bits
            // Variables and selects-of-variables remain verilog-width
            V3Clean::cleanAll(v3Global.rootp());

            // Move wide constants to BLOCK temps / ConstPool.
            V3Premit::premitAll(v3Global.rootp());
        }

        // Expand macros and wide operators into C++ primitives
        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        إذا (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        إذا (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            إذا (v3Global.opt.f_یوځای_Cond()) {
                // Merge conditionals
                V3_یوځای_Cond::یوځای_All(v3Global.rootp());
            }

            إذا (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            إذا (v3Global.opt.fInlineCFuncs()) {
                // Inline small CFuncs to reduce function call overhead
                V3InlineCFuncs::inlineAll(v3Global.rootp());
            }

            // Fix very deep expressions
            // Mark evaluation functions as member functions, if needed.
            V3_ژوروالی::ژوروالی_All(v3Global.rootp());

            // Branch prediction
            V3Branch::branchAll(v3Global.rootp());

            // Add C casts when longs need to become long-long and vice-versa
            // Note depth may insert something needing a cast, so this must be last.
            V3Cast::castAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();
        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        إذا (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3_Exec_ګراف::implement(v3Global.rootp());
        }

        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3_Variable_ترتیب::ترتیب_All(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            إذا (!v3Global.opt.libCreate().تشه()) {
                v3Global.current_Hier_بلاک_لګښت(V3Control::get_Current_Hier_بلاک_لګښت());
            }
        }

        إذا (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            ثابت EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitc_Exec_ګراف();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitc_سرلیکونه();
        } وإلا إذا (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(ریښتیا);
        }

        // End of conversion
        V3Stats::جمع_کړه_Stat_Perf(V3Stats::STAT_WALLTIME_CVT, cvt_Wall_وخت.delta_وخت());
    }
    إذا (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    إذا (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } وإلا إذا (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    إذا (!v3Global.opt.libCreate().تشه()) {
        إذا (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    إذا (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        إذا (v3Global.opt.اصلي()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        اندازه_t src_f_cnt = 0;
        لكل (Ast_غوټه* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->سرچینه() ? 1 : 0;
        }
        إذا (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_پوله) v3Global.useParallelBuild(ریښتیا);
        إذا (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        إذا (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    إذا (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

ساكن منطقي verilate(ثابت string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::جمع_کړه_Src_Depend(v3Global.opt.buildDepBin());
    إذا (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        أرجع خطأ;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    إذا (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").تشه()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(جديد V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_سر_معلومات_Dir(), v3Global.opt.مختاړی() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_سر_معلومات_Dir(), v3Global.opt.مختاړی() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_سر_معلومات_Dir(), v3Global.opt.مختاړی() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    إذا (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_شمېره::selfTest();
        VString::selfTest();
        VHashSha512::selfTest();
        VSpellCheck::selfTest();
        V3_ګراف::selfTest();
        V3_Exec_ګراف::selfTest();
        V3PreShell::selfTest();
        V3Broken::selfTest();
        V3Control::selfTest();
        V3_Hash_جدول_Internals::selfTest();
        V3ThreadPool::selfTest();
        UINFO(2, "selfTest done");
    }

    // Read first filename
    v3Global.readFiles();
    v3Global.removeStd();

    // Link, etc, if needed
    إذا (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_ونه("final", 990, dump_ونه_Either_Level() >= 3);
    إذا (v3Global.opt.jsonOnly() || dump_ونه_Json_Level()) {
        ثابت string filename
            = (v3Global.opt.jsonOnlyMetaOutput().تشه()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    إذا (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::اكتب(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    إذا (ثابت V3_Hier_ګراف* ثابت hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.پوړیز(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hier_ماشوم(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hier_بلاکونه().تشه(), "hierarchical-block must not be set");
        إذا (v3Global.opt.gmake()) {
            hierGraphp->write_Command_دلیلونه_Files(خطأ);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        إذا (v3Global.opt.makeJson()) {
            hierGraphp->write_Command_دلیلونه_Files(ریښتیا);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    إذا (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی();
        filename += v3Global.opt.hier_سر() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    إذا (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی()
                                 + "__idmap.xml");
    }

    إذا ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    إذا (v3Global.opt.hier_سر()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_سر_معلومات_Dir());
    إذا (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    إذا (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", ریښتیا);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    أرجع ریښتیا;
}

ساكن string buildMakeCmd(ثابت string& makefile, ثابت string& موخه) {
    ثابت VStringList& makeFlags = v3Global.opt.makeFlags();
    ثابت صحيح jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    إذا (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    إذا (v3Global.opt.getenvMAKEFLAGS().پیدا_کړه("-jobserver-auth") == string::npos) {
        إذا (jobs > 0) cmd << " -j " << jobs;
    }
    لكل (ثابت string& flag : makeFlags) cmd << ' ' << flag;
    إذا (!موخه.تشه()) cmd << ' ' << موخه;

    أرجع cmd.str();
}

ساكن فراغ release_Netlist_حافظه() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::release_حافظه();
    إذا (v3Global.opt.stats()) V3Stats::statsStage("released");
}

ساكن فراغ exec_Build_دنده() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    release_Netlist_حافظه();
    ثابت VlOs::Delta_Wall_وخت build_Wall_وخت{ریښتیا};
    UINFO(1, "Start Build");

    ثابت string cmdStr = buildMakeCmd(v3Global.opt.مختاړی() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_سر_معلومات_Dir());
    ثابت صحيح exit_کوډ = V3Os::system(cmdStr);
    V3Stats::جمع_کړه_Stat_Perf(V3Stats::STAT_WALLTIME_BUILD, build_Wall_وخت.delta_وخت());

    إذا (exit_کوډ != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_کوډ << std::کرښه_پای);
        v3Global.vlExit(exit_کوډ);
    }
}

ساكن فراغ execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    release_Netlist_حافظه();
    ثابت string makefile = v3Global.opt.مختاړی() + "_hier.mk ";
    ثابت string موخه = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    ثابت string cmdStr = buildMakeCmd(makefile, موخه);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_سر_معلومات_Dir());
    ثابت صحيح exit_کوډ = V3Os::system(cmdStr);
    إذا (exit_کوډ != 0) {
        v3error(cmdStr << " exited with " << exit_کوډ << std::کرښه_پای);
        v3Global.vlExit(exit_کوډ);
    }
}

//######################################################################

صحيح اصلي(صحيح argc, حرف** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    ثابت VlOs::Delta_Wall_وخت wall_وخت_ټول{ریښتیا};
    ثابت VlOs::Delta_Cpu_وخت cpu_وخت_ټول{ریښتیا};

    وخت_t randseed;
    وخت_2(&randseed);
    srand(static_cast<صحيح>(randseed));

    ثابت string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(جديد File_کرښه{File_کرښه::command_کرښه_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->وخت_Init();

    V3Error::abortIfErrors();

    منطقي didVerilate = خطأ;
    إذا (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } وإلا {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    إذا (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } وإلا إذا (v3Global.opt.build()) {
        exec_Build_دنده();
    }

    إذا (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(ریښتیا);

    // Explicitly release resources
    v3Global.shutdown();

    إذا (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::جمع_کړه_Stat_Perf(V3Stats::STAT_CPUTIME, cpu_وخت_ټول.delta_وخت());
        V3Stats::جمع_کړه_Stat_Perf(V3Stats::STAT_WALLTIME, wall_وخت_ټول.delta_وخت());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
