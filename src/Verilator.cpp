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

#ଅନ୍ତର୍ଭାଵ "V3Active.h"
#ଅନ୍ତର୍ଭାଵ "V3ActiveTop.h"
#ଅନ୍ତର୍ଭାଵ "V3Assert.h"
#ଅନ୍ତର୍ଭାଵ "V3AssertNfa.h"
#ଅନ୍ତର୍ଭାଵ "V3AssertPre.h"
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3AstPatterns.h"
#ଅନ୍ତର୍ଭାଵ "V3Begin.h"
#ଅନ୍ତର୍ଭାଵ "V3BitBlast.h"
#ଅନ୍ତର୍ଭାଵ "V3Branch.h"
#ଅନ୍ତର୍ଭାଵ "V3Broken.h"
#ଅନ୍ତର୍ଭାଵ "V3CCtors.h"
#ଅନ୍ତର୍ଭାଵ "V3CUse.h"
#ଅନ୍ତର୍ଭାଵ "V3Case.h"
#ଅନ୍ତର୍ଭାଵ "V3Cast.h"
#ଅନ୍ତର୍ଭାଵ "V3Class.h"
#ଅନ୍ତର୍ଭାଵ "V3Clean.h"
#ଅନ୍ତର୍ଭାଵ "V3Clock.h"
#ଅନ୍ତର୍ଭାଵ "V3Combine.h"
#ଅନ୍ତର୍ଭାଵ "V3Common.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Coverage.h"
#ଅନ୍ତର୍ଭାଵ "V3CoverageJoin.h"
#ଅନ୍ତର୍ଭାଵ "V3Covergroup.h"
#ଅନ୍ତର୍ଭାଵ "V3Dead.h"
#ଅନ୍ତର୍ଭାଵ "V3Decompose.h"
#ଅନ୍ତର୍ଭାଵ "V3Delayed.h"
#ଅନ୍ତର୍ଭାଵ "V3Depth.h"
#ଅନ୍ତର୍ଭାଵ "V3DepthBlock.h"
#ଅନ୍ତର୍ଭାଵ "V3Descope.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgOptimizer.h"
#ଅନ୍ତର୍ଭାଵ "V3DiagSarif.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCMain.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitMk.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitMkJson.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Expand.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Force.h"
#ଅନ୍ତର୍ଭାଵ "V3Fork.h"
#ଅନ୍ତର୍ଭାଵ "V3FsmDetect.h"
#ଅନ୍ତର୍ଭାଵ "V3FuncOpt.h"
#ଅନ୍ତର୍ଭାଵ "V3Gate.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3HashTable.h"
#ଅନ୍ତର୍ଭାଵ "V3HierBlock.h"
#ଅନ୍ତର୍ଭାଵ "V3Inline.h"
#ଅନ୍ତର୍ଭାଵ "V3InlineCFuncs.h"
#ଅନ୍ତର୍ଭାଵ "V3Inst.h"
#ଅନ୍ତର୍ଭାଵ "V3Interface.h"
#ଅନ୍ତର୍ଭାଵ "V3LibMap.h"
#ଅନ୍ତର୍ଭାଵ "V3Life.h"
#ଅନ୍ତର୍ଭାଵ "V3LifePost.h"
#ଅନ୍ତର୍ଭାଵ "V3LiftExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkDot.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkDotIfaceCapture.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkInc.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkJump.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkLValue.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkLevel.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkParse.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkResolve.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkWith.h"
#ଅନ୍ତର୍ଭାଵ "V3Localize.h"
#ଅନ୍ତର୍ଭାଵ "V3MergeCond.h"
#ଅନ୍ତର୍ଭାଵ "V3Name.h"
#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Param.h"
#ଅନ୍ତର୍ଭାଵ "V3PreShell.h"
#ଅନ୍ତର୍ଭାଵ "V3Premit.h"
#ଅନ୍ତର୍ଭାଵ "V3ProtectLib.h"
#ଅନ୍ତର୍ଭାଵ "V3RandSequence.h"
#ଅନ୍ତର୍ଭାଵ "V3Randomize.h"
#ଅନ୍ତର୍ଭାଵ "V3Reloop.h"
#ଅନ୍ତର୍ଭାଵ "V3Reorder.h"
#ଅନ୍ତର୍ଭାଵ "V3Sampled.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"
#ଅନ୍ତର୍ଭାଵ "V3Scope.h"
#ଅନ୍ତର୍ଭାଵ "V3Slice.h"
#ଅନ୍ତର୍ଭାଵ "V3Split.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3Subst.h"
#ଅନ୍ତର୍ଭାଵ "V3Table.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3ThreadPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Timing.h"
#ଅନ୍ତର୍ଭାଵ "V3Trace.h"
#ଅନ୍ତର୍ଭାଵ "V3TraceDecl.h"
#ଅନ୍ତର୍ଭାଵ "V3Tristate.h"
#ଅନ୍ତର୍ଭାଵ "V3Udp.h"
#ଅନ୍ତର୍ଭାଵ "V3Undriven.h"
#ଅନ୍ତର୍ଭାଵ "V3Unknown.h"
#ଅନ୍ତର୍ଭାଵ "V3Unroll.h"
#ଅନ୍ତର୍ଭାଵ "V3VariableOrder.h"
#ଅନ୍ତର୍ଭାଵ "V3Waiver.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"
#ଅନ୍ତର୍ଭାଵ "V3WidthCommit.h"

#ଅନ୍ତର୍ଭାଵ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

ସ୍ଥିର ଶୂନ୍ୟ reportStatsIfEnabled() {
    ଯଦି (v3Global.opt.stats()) {
        File_ଧାଡ଼ି::stats();
        V3Stats::statsReport();
    }
}

ସ୍ଥିର ଶୂନ୍ୟ emitJson() VL_MT_DISABLED {
    ଅଚଳ string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_ବୃକ୍ଷ_Json_File(filename);
}

ସ୍ଥିର ଶୂନ୍ୟ emitSerialized() VL_MT_DISABLED {
    ଯଦି (v3Global.opt.jsonOnly()) emitJson();
}

ସ୍ଥିର ଶୂନ୍ୟ process() {
    {
        ଅଚଳ VlOs::DeltaWallTime elabWallTime{ସତ୍ୟ};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        ଯଦି (v3Global.opt.debugExitParse()) {
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_ବୃକ୍ଷ();  // Force a check, as link is most likely place for problems
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

        ଯଦି (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_ବୃକ୍ଷ();
        ଯଦି (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            ଯଦି (v3Global.hierGraphp()) ଫେରନ୍ତୁ;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(ସତ୍ୟ);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        ଅଚଳ VlOs::DeltaWallTime cvtWallTime{ସତ୍ୟ};
        ଯଦି (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            ଯଦି (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        ଯଦି (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_ଶୀର୍ଷ(v3Global.rootp());
        } ଅନ୍ୟଥା {
            V3LinkLevel::non_Wrap_ଶୀର୍ଷ(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            ଯଦି (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        ଯଦି (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        ଯଦି (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        ଯଦି (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

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
        ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        ଯଦି (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            ଯଦି (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(ସତ୍ୟ);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_ପଦସମଷ୍ଟି::lift_ପଦସମଷ୍ଟି_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(ସତ୍ୟ);
        }

        // Interface references feed trace file aliases and VPI name resolution
        ଯଦି (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_ବୃକ୍ଷ();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_ବୃକ୍ଷ();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        ଯଦି (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            ଯଦି (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            ଯଦି (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        ଯଦି (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ବୃକ୍ଷ();
        }

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        ଯଦି (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            ଯଦି (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            ଯଦି (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            ଯଦି (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            ଯଦି (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            ଯଦି (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ବୃକ୍ଷ();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            ଯଦି (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            ଯଦି (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            ଯଦି (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            ଯଦି (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } ଅନ୍ୟଥା {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            ଯଦି (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            ଯଦି (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            ଯଦି (v3Global.opt.timing().isSetTrue()) {
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
            V3_Active_ଶୀର୍ଷ::active_ଶୀର୍ଷ_All(v3Global.rootp());

            // Remove SAMPLED
            ଯଦି (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            ଯଦି (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            ଯଦି (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            ଯଦି (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            ଯଦି (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            ଯଦି (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            ଯଦି (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(ମିଥ୍ୟା);

            // Move variables from modules to function local variables where possible
            ଯଦି (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            ଯଦି (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        ଯଦି (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            ଯଦି (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            ଯଦି (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

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
        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        ଯଦି (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ଯଦି (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            ଯଦି (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ଯଦି (v3Global.opt.fInlineCFuncs()) {
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
        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        ଯଦି (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            ଯଦି (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        ଯଦି (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            ଅଚଳ EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } ଅନ୍ୟଥା ଯଦି (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(ସତ୍ୟ);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    ଯଦି (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    ଯଦି (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } ଅନ୍ୟଥା ଯଦି (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    ଯଦି (!v3Global.opt.libCreate().empty()) {
        ଯଦି (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    ଯଦି (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        ଯଦି (v3Global.opt.ମୁଖ୍ୟ()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        size_t src_f_cnt = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        ଯଦି (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(ସତ୍ୟ);
        ଯଦି (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        ଯଦି (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

ସ୍ଥିର ତାର୍କିକ verilate(ଅଚଳ string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    ଯଦି (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    ଯଦି (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(ନୂତନ V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    ଯଦି (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_ସଂଖ୍ୟା::selfTest();
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
    ଯଦି (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_ବୃକ୍ଷ("final", 990, dump_ବୃକ୍ଷ_Either_Level() >= 3);
    ଯଦି (v3Global.opt.jsonOnly() || dump_ବୃକ୍ଷ_Json_Level()) {
        ଅଚଳ string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    ଯଦି (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::ଲିଖ(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    ଯଦି (ଅଚଳ V3HierGraph* ଅଚଳ hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        ଯଦି (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(ମିଥ୍ୟା);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        ଯଦି (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(ସତ୍ୟ);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    ଯଦି (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_ଶୀର୍ଷ() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    ଯଦି (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    ଯଦି ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    ଯଦି (v3Global.opt.hier_ଶୀର୍ଷ()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir());
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", ସତ୍ୟ);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ସ୍ଥିର string buildMakeCmd(ଅଚଳ string& makefile, ଅଚଳ string& target) {
    ଅଚଳ VStringList& makeFlags = v3Global.opt.makeFlags();
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    ଯଦି (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    ଯଦି (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        ଯଦି (jobs > 0) cmd << " -j " << jobs;
    }
    ପାଇଁ (ଅଚଳ string& flag : makeFlags) cmd << ' ' << flag;
    ଯଦି (!target.empty()) cmd << ' ' << target;

    ଫେରନ୍ତୁ cmd.str();
}

ସ୍ଥିର ଶୂନ୍ୟ releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("released");
}

ସ୍ଥିର ଶୂନ୍ୟ execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    ଅଚଳ VlOs::DeltaWallTime buildWallTime{ସତ୍ୟ};
    UINFO(1, "Start Build");

    ଅଚଳ string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir());
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା exit_code = V3Os::system(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    ଯଦି (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

ସ୍ଥିର ଶୂନ୍ୟ execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    ଅଚଳ string makefile = v3Global.opt.prefix() + "_hier.mk ";
    ଅଚଳ string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    ଅଚଳ string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir());
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା exit_code = V3Os::system(cmdStr);
    ଯଦି (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୁଖ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    ଅଚଳ VlOs::DeltaWallTime wallTimeTotal{ସତ୍ୟ};
    ଅଚଳ VlOs::DeltaCpuTime cpuTimeTotal{ସତ୍ୟ};

    time_t randseed;
    time(&randseed);
    srand(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(randseed));

    ଅଚଳ string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    ତାର୍କିକ didVerilate = ମିଥ୍ୟା;
    ଯଦି (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } ଅନ୍ୟଥା {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    ଯଦି (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } ଅନ୍ୟଥା ଯଦି (v3Global.opt.build()) {
        execBuildJob();
    }

    ଯଦି (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(ସତ୍ୟ);

    // Explicitly release resources
    v3Global.shutdown();

    ଯଦି (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
