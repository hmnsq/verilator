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

#define VL_MT_CONTROL_CODE_UNIT 1

#অন্তর্ভাৱ "V3Active.h"
#অন্তর্ভাৱ "V3ActiveTop.h"
#অন্তর্ভাৱ "V3Assert.h"
#অন্তর্ভাৱ "V3AssertNfa.h"
#অন্তর্ভাৱ "V3AssertPre.h"
#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3AstPatterns.h"
#অন্তর্ভাৱ "V3Begin.h"
#অন্তর্ভাৱ "V3BitBlast.h"
#অন্তর্ভাৱ "V3Branch.h"
#অন্তর্ভাৱ "V3Broken.h"
#অন্তর্ভাৱ "V3CCtors.h"
#অন্তর্ভাৱ "V3CUse.h"
#অন্তর্ভাৱ "V3Case.h"
#অন্তর্ভাৱ "V3Cast.h"
#অন্তর্ভাৱ "V3Class.h"
#অন্তর্ভাৱ "V3Clean.h"
#অন্তর্ভাৱ "V3Clock.h"
#অন্তর্ভাৱ "V3Combine.h"
#অন্তর্ভাৱ "V3Common.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3Coverage.h"
#অন্তর্ভাৱ "V3CoverageJoin.h"
#অন্তর্ভাৱ "V3Covergroup.h"
#অন্তর্ভাৱ "V3Dead.h"
#অন্তর্ভাৱ "V3Decompose.h"
#অন্তর্ভাৱ "V3Delayed.h"
#অন্তর্ভাৱ "V3Depth.h"
#অন্তর্ভাৱ "V3DepthBlock.h"
#অন্তর্ভাৱ "V3Descope.h"
#অন্তর্ভাৱ "V3DfgOptimizer.h"
#অন্তর্ভাৱ "V3DiagSarif.h"
#অন্তর্ভাৱ "V3EmitC.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3EmitCMain.h"
#অন্তর্ভাৱ "V3EmitMk.h"
#অন্তর্ভাৱ "V3EmitMkJson.h"
#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3ExecGraph.h"
#অন্তর্ভাৱ "V3Expand.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Force.h"
#অন্তর্ভাৱ "V3Fork.h"
#অন্তর্ভাৱ "V3FsmDetect.h"
#অন্তর্ভাৱ "V3FuncOpt.h"
#অন্তর্ভাৱ "V3Gate.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3HashTable.h"
#অন্তর্ভাৱ "V3HierBlock.h"
#অন্তর্ভাৱ "V3Inline.h"
#অন্তর্ভাৱ "V3InlineCFuncs.h"
#অন্তর্ভাৱ "V3Inst.h"
#অন্তর্ভাৱ "V3Interface.h"
#অন্তর্ভাৱ "V3LibMap.h"
#অন্তর্ভাৱ "V3Life.h"
#অন্তর্ভাৱ "V3LifePost.h"
#অন্তর্ভাৱ "V3LiftExpr.h"
#অন্তর্ভাৱ "V3LinkDot.h"
#অন্তর্ভাৱ "V3LinkDotIfaceCapture.h"
#অন্তর্ভাৱ "V3LinkInc.h"
#অন্তর্ভাৱ "V3LinkJump.h"
#অন্তর্ভাৱ "V3LinkLValue.h"
#অন্তর্ভাৱ "V3LinkLevel.h"
#অন্তর্ভাৱ "V3LinkParse.h"
#অন্তর্ভাৱ "V3LinkResolve.h"
#অন্তর্ভাৱ "V3LinkWith.h"
#অন্তর্ভাৱ "V3Localize.h"
#অন্তর্ভাৱ "V3MergeCond.h"
#অন্তর্ভাৱ "V3Name.h"
#অন্তর্ভাৱ "V3Order.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3Param.h"
#অন্তর্ভাৱ "V3PreShell.h"
#অন্তর্ভাৱ "V3Premit.h"
#অন্তর্ভাৱ "V3ProtectLib.h"
#অন্তর্ভাৱ "V3RandSequence.h"
#অন্তর্ভাৱ "V3Randomize.h"
#অন্তর্ভাৱ "V3Reloop.h"
#অন্তর্ভাৱ "V3Reorder.h"
#অন্তর্ভাৱ "V3Sampled.h"
#অন্তর্ভাৱ "V3Sched.h"
#অন্তর্ভাৱ "V3Scope.h"
#অন্তর্ভাৱ "V3Slice.h"
#অন্তর্ভাৱ "V3Split.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3String.h"
#অন্তর্ভাৱ "V3Subst.h"
#অন্তর্ভাৱ "V3Table.h"
#অন্তর্ভাৱ "V3Task.h"
#অন্তর্ভাৱ "V3ThreadPool.h"
#অন্তর্ভাৱ "V3Timing.h"
#অন্তর্ভাৱ "V3Trace.h"
#অন্তর্ভাৱ "V3TraceDecl.h"
#অন্তর্ভাৱ "V3Tristate.h"
#অন্তর্ভাৱ "V3Udp.h"
#অন্তর্ভাৱ "V3Undriven.h"
#অন্তর্ভাৱ "V3Unknown.h"
#অন্তর্ভাৱ "V3Unroll.h"
#অন্তর্ভাৱ "V3VariableOrder.h"
#অন্তর্ভাৱ "V3Waiver.h"
#অন্তর্ভাৱ "V3Width.h"
#অন্তর্ভাৱ "V3WidthCommit.h"

#অন্তর্ভাৱ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

স্থির শূন্য reportStatsIfEnabled() {
    যদি (v3Global.opt.stats()) {
        File_সাৰি::stats();
        V3Stats::statsReport();
    }
}

স্থির শূন্য emitJson() VL_MT_DISABLED {
    ধ্রুৱ string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_বৃক্ষ_Json_File(filename);
}

স্থির শূন্য emitSerialized() VL_MT_DISABLED {
    যদি (v3Global.opt.jsonOnly()) emitJson();
}

স্থির শূন্য process() {
    {
        ধ্রুৱ VlOs::DeltaWallTime elabWallTime{সঁচা};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        যদি (v3Global.opt.debugExitParse()) {
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_বৃক্ষ();  // Force a check, as link is most likely place for problems
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

        যদি (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_বৃক্ষ();
        যদি (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            যদি (v3Global.hierGraphp()) ঘূৰাই;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(সঁচা);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        ধ্রুৱ VlOs::DeltaWallTime cvtWallTime{সঁচা};
        যদি (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            যদি (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        যদি (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_শীৰ্ষ(v3Global.rootp());
        } অন্যথা {
            V3LinkLevel::non_Wrap_শীৰ্ষ(v3Global.rootp());
        }

        যদি (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            যদি (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        যদি (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        যদি (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        যদি (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

        // Signal based lint checks, no change to structures
        // Must be before first constification pass drops dead code
        V3Undriven::undrivenAll(v3Global.rootp());

        // Assertion insertion
        //    After we've added block coverage, but before other nasty transforms
        V3AssertCommon::collectDefaultDisable(v3Global.rootp());
        V3AssertCommon::lowerSequenceEvents(v3Global.rootp());
        V3AssertNfa::assertNfaAll(v3Global.rootp());
        // V3AssertProp removed: NFA subsumes multi-cycle property lowering.
        // Unsupported constructs fall through to V3AssertPre.
        V3AssertPre::assertPreAll(v3Global.rootp());
        //
        V3Assert::assertAll(v3Global.rootp());

        // Propagate constants into expressions
        যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        যদি (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(সঁচা);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        যদি (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_ৰাশিমালা::lift_ৰাশিমালা_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(সঁচা);
        }

        // Interface references feed trace file aliases and VPI name resolution
        যদি (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_বৃক্ষ();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_বৃক্ষ();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        যদি (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            যদি (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            যদি (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        যদি (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_বৃক্ষ();
        }

        যদি (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        যদি (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        যদি (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            যদি (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            যদি (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            যদি (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            যদি (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            যদি (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_বৃক্ষ();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            যদি (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            যদি (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            যদি (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            যদি (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } অন্যথা {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            যদি (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            যদি (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            যদি (v3Global.opt.timing().isSetTrue()) {
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
            V3_Active_শীৰ্ষ::active_শীৰ্ষ_All(v3Global.rootp());

            // Remove SAMPLED
            যদি (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            যদি (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            যদি (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            যদি (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            যদি (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            যদি (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        যদি (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            যদি (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(মিছা);

            // Move variables from modules to function local variables where possible
            যদি (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            যদি (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        যদি (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            যদি (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            যদি (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

            // Here down, widthMin() is the Verilog width, and width() is the C++ width
            // Bits between widthMin() and width() are irrelevant, but may be non-zero.
            v3Global.widthMinUsage(VWidthMinUsage::VERILOG_WIDTH);

            // Make all expressions 32, 64, or 32*N bits
            // Variables and selects-of-variables remain verilog-width
            V3Clean::cleanAll(v3Global.rootp());

            // Move wide constants to BLOCK temps / ConstPool.
            V3Premit::premitAll(v3Global.rootp());
        }

        // Expand macros and wide operators into C++ primitives
        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        যদি (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        যদি (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            যদি (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            যদি (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            যদি (v3Global.opt.fInlineCFuncs()) {
                // Inline small CFuncs to reduce function call overhead
                V3InlineCFuncs::inlineAll(v3Global.rootp());
            }

            // Fix very deep expressions
            // Mark evaluation functions as member functions, if needed.
            V3Depth::depthAll(v3Global.rootp());

            // Branch prediction
            V3Branch::branchAll(v3Global.rootp());

            // Add C casts when longs need to become long-long and vice-versa
            // Note depth may insert something needing a cast, so this must be last.
            V3Cast::castAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();
        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        যদি (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            যদি (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        যদি (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            ধ্রুৱ EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } অন্যথা যদি (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(সঁচা);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    যদি (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    যদি (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } অন্যথা যদি (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    যদি (!v3Global.opt.libCreate().empty()) {
        যদি (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    যদি (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        যদি (v3Global.opt.main()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        size_t src_f_cnt = 0;
        বাবে (Ast_গ্ৰন্থি* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুৱ AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        যদি (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(সঁচা);
        যদি (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        যদি (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    যদি (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

স্থির তার্কিক verilate(ধ্রুৱ string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    যদি (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        ঘূৰাই মিছা;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    যদি (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(নূতন V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_শীৰ্ষ_Data_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_শীৰ্ষ_Data_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_শীৰ্ষ_Data_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    যদি (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_সংখ্যা::selfTest();
        VString::selfTest();
        VHashSha512::selfTest();
        VSpellCheck::selfTest();
        V3Graph::selfTest();
        V3ExecGraph::selfTest();
        V3PreShell::selfTest();
        V3Broken::selfTest();
        V3Control::selfTest();
        V3HashTableInternals::selfTest();
        V3ThreadPool::selfTest();
        UINFO(2, "selfTest done");
    }

    // Read first filename
    v3Global.readFiles();
    v3Global.removeStd();

    // Link, etc, if needed
    যদি (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_বৃক্ষ("final", 990, dump_বৃক্ষ_Either_Level() >= 3);
    যদি (v3Global.opt.jsonOnly() || dump_বৃক্ষ_Json_Level()) {
        ধ্রুৱ string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    যদি (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::লিখ(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    যদি (ধ্রুৱ V3HierGraph* ধ্রুৱ hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        যদি (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(মিছা);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        যদি (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(সঁচা);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    যদি (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_শীৰ্ষ() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    যদি (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    যদি ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    যদি (v3Global.opt.hier_শীৰ্ষ()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_শীৰ্ষ_Data_Dir());
    যদি (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    যদি (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", সঁচা);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    ঘূৰাই সঁচা;
}

স্থির string buildMakeCmd(ধ্রুৱ string& makefile, ধ্রুৱ string& target) {
    ধ্রুৱ VStringList& makeFlags = v3Global.opt.makeFlags();
    ধ্রুৱ পূর্ণাঙ্ক jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    যদি (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    যদি (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        যদি (jobs > 0) cmd << " -j " << jobs;
    }
    বাবে (ধ্রুৱ string& flag : makeFlags) cmd << ' ' << flag;
    যদি (!target.empty()) cmd << ' ' << target;

    ঘূৰাই cmd.str();
}

স্থির শূন্য releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    যদি (v3Global.opt.stats()) V3Stats::statsStage("released");
}

স্থির শূন্য execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    ধ্রুৱ VlOs::DeltaWallTime buildWallTime{সঁচা};
    UINFO(1, "Start Build");

    ধ্রুৱ string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_শীৰ্ষ_Data_Dir());
    ধ্রুৱ পূর্ণাঙ্ক exit_code = V3Os::system(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    যদি (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

স্থির শূন্য execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    ধ্রুৱ string makefile = v3Global.opt.prefix() + "_hier.mk ";
    ধ্রুৱ string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    ধ্রুৱ string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_শীৰ্ষ_Data_Dir());
    ধ্রুৱ পূর্ণাঙ্ক exit_code = V3Os::system(cmdStr);
    যদি (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

পূর্ণাঙ্ক main(পূর্ণাঙ্ক argc, ৱর্ণ** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    ধ্রুৱ VlOs::DeltaWallTime wallTimeTotal{সঁচা};
    ধ্রুৱ VlOs::DeltaCpuTime cpuTimeTotal{সঁচা};

    time_t randseed;
    time(&randseed);
    srand(static_cast<পূর্ণাঙ্ক>(randseed));

    ধ্রুৱ string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    তার্কিক didVerilate = মিছা;
    যদি (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } অন্যথা {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    যদি (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } অন্যথা যদি (v3Global.opt.build()) {
        execBuildJob();
    }

    যদি (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(সঁচা);

    // Explicitly release resources
    v3Global.shutdown();

    যদি (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
