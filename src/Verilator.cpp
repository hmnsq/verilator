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

#அந்தர்பாவ "V3Active.h"
#அந்தர்பாவ "V3ActiveTop.h"
#அந்தர்பாவ "V3Assert.h"
#அந்தர்பாவ "V3AssertNfa.h"
#அந்தர்பாவ "V3AssertPre.h"
#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3AstPatterns.h"
#அந்தர்பாவ "V3Begin.h"
#அந்தர்பாவ "V3BitBlast.h"
#அந்தர்பாவ "V3Branch.h"
#அந்தர்பாவ "V3Broken.h"
#அந்தர்பாவ "V3CCtors.h"
#அந்தர்பாவ "V3CUse.h"
#அந்தர்பாவ "V3Case.h"
#அந்தர்பாவ "V3Cast.h"
#அந்தர்பாவ "V3Class.h"
#அந்தர்பாவ "V3Clean.h"
#அந்தர்பாவ "V3Clock.h"
#அந்தர்பாவ "V3Combine.h"
#அந்தர்பாவ "V3Common.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Coverage.h"
#அந்தர்பாவ "V3CoverageJoin.h"
#அந்தர்பாவ "V3Covergroup.h"
#அந்தர்பாவ "V3Dead.h"
#அந்தர்பாவ "V3Decompose.h"
#அந்தர்பாவ "V3Delayed.h"
#அந்தர்பாவ "V3Depth.h"
#அந்தர்பாவ "V3DepthBlock.h"
#அந்தர்பாவ "V3Descope.h"
#அந்தர்பாவ "V3DfgOptimizer.h"
#அந்தர்பாவ "V3DiagSarif.h"
#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3EmitCMain.h"
#அந்தர்பாவ "V3EmitMk.h"
#அந்தர்பாவ "V3EmitMkJson.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3Expand.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Force.h"
#அந்தர்பாவ "V3Fork.h"
#அந்தர்பாவ "V3FsmDetect.h"
#அந்தர்பாவ "V3FuncOpt.h"
#அந்தர்பாவ "V3Gate.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3HashTable.h"
#அந்தர்பாவ "V3HierBlock.h"
#அந்தர்பாவ "V3Inline.h"
#அந்தர்பாவ "V3InlineCFuncs.h"
#அந்தர்பாவ "V3Inst.h"
#அந்தர்பாவ "V3Interface.h"
#அந்தர்பாவ "V3LibMap.h"
#அந்தர்பாவ "V3Life.h"
#அந்தர்பாவ "V3LifePost.h"
#அந்தர்பாவ "V3LiftExpr.h"
#அந்தர்பாவ "V3LinkDot.h"
#அந்தர்பாவ "V3LinkDotIfaceCapture.h"
#அந்தர்பாவ "V3LinkInc.h"
#அந்தர்பாவ "V3LinkJump.h"
#அந்தர்பாவ "V3LinkLValue.h"
#அந்தர்பாவ "V3LinkLevel.h"
#அந்தர்பாவ "V3LinkParse.h"
#அந்தர்பாவ "V3LinkResolve.h"
#அந்தர்பாவ "V3LinkWith.h"
#அந்தர்பாவ "V3Localize.h"
#அந்தர்பாவ "V3MergeCond.h"
#அந்தர்பாவ "V3Name.h"
#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Param.h"
#அந்தர்பாவ "V3PreShell.h"
#அந்தர்பாவ "V3Premit.h"
#அந்தர்பாவ "V3ProtectLib.h"
#அந்தர்பாவ "V3RandSequence.h"
#அந்தர்பாவ "V3Randomize.h"
#அந்தர்பாவ "V3Reloop.h"
#அந்தர்பாவ "V3Reorder.h"
#அந்தர்பாவ "V3Sampled.h"
#அந்தர்பாவ "V3Sched.h"
#அந்தர்பாவ "V3Scope.h"
#அந்தர்பாவ "V3Slice.h"
#அந்தர்பாவ "V3Split.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3Subst.h"
#அந்தர்பாவ "V3Table.h"
#அந்தர்பாவ "V3Task.h"
#அந்தர்பாவ "V3ThreadPool.h"
#அந்தர்பாவ "V3Timing.h"
#அந்தர்பாவ "V3Trace.h"
#அந்தர்பாவ "V3TraceDecl.h"
#அந்தர்பாவ "V3Tristate.h"
#அந்தர்பாவ "V3Udp.h"
#அந்தர்பாவ "V3Undriven.h"
#அந்தர்பாவ "V3Unknown.h"
#அந்தர்பாவ "V3Unroll.h"
#அந்தர்பாவ "V3VariableOrder.h"
#அந்தர்பாவ "V3Waiver.h"
#அந்தர்பாவ "V3Width.h"
#அந்தர்பாவ "V3WidthCommit.h"

#அந்தர்பாவ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

நிலையான வெறுமை reportStatsIfEnabled() {
    எனில் (v3Global.opt.stats()) {
        File_வரி::stats();
        V3Stats::statsReport();
    }
}

நிலையான வெறுமை emitJson() VL_MT_DISABLED {
    மாறிலி string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_மரம்_Json_File(filename);
}

நிலையான வெறுமை emitSerialized() VL_MT_DISABLED {
    எனில் (v3Global.opt.jsonOnly()) emitJson();
}

நிலையான வெறுமை process() {
    {
        மாறிலி VlOs::DeltaWallTime elabWallTime{உண்மை};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        எனில் (v3Global.opt.debugExitParse()) {
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_மரம்();  // Force a check, as link is most likely place for problems
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

        எனில் (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_மரம்();
        எனில் (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            எனில் (v3Global.hierGraphp()) திரும்பு;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(உண்மை);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        மாறிலி VlOs::DeltaWallTime cvtWallTime{உண்மை};
        எனில் (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            எனில் (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        எனில் (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_உச்சி(v3Global.rootp());
        } இல்லையெனில் {
            V3LinkLevel::non_Wrap_உச்சி(v3Global.rootp());
        }

        எனில் (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            எனில் (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        எனில் (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        எனில் (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        எனில் (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

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
        எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        எனில் (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            எனில் (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(உண்மை);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        எனில் (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_கோவை::lift_கோவை_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(உண்மை);
        }

        // Interface references feed trace file aliases and VPI name resolution
        எனில் (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_மரம்();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_மரம்();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        எனில் (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            எனில் (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            எனில் (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        எனில் (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_மரம்();
        }

        எனில் (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        எனில் (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        எனில் (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            எனில் (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            எனில் (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            எனில் (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            எனில் (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            எனில் (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_மரம்();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            எனில் (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            எனில் (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            எனில் (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            எனில் (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } இல்லையெனில் {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            எனில் (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            எனில் (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            எனில் (v3Global.opt.timing().isSetTrue()) {
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
            V3_Active_உச்சி::active_உச்சி_All(v3Global.rootp());

            // Remove SAMPLED
            எனில் (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            எனில் (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            எனில் (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            எனில் (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            எனில் (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            எனில் (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        எனில் (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            எனில் (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(பொய்);

            // Move variables from modules to function local variables where possible
            எனில் (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            எனில் (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        எனில் (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            எனில் (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            எனில் (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

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
        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        எனில் (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        எனில் (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            எனில் (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            எனில் (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            எனில் (v3Global.opt.fInlineCFuncs()) {
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
        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        எனில் (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            எனில் (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        எனில் (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            மாறிலி EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } இல்லையெனில் எனில் (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(உண்மை);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    எனில் (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    எனில் (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } இல்லையெனில் எனில் (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    எனில் (!v3Global.opt.libCreate().empty()) {
        எனில் (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    எனில் (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        எனில் (v3Global.opt.முதன்மை()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        size_t src_f_cnt = 0;
        க்கு (Ast_முனை* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        எனில் (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(உண்மை);
        எனில் (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        எனில் (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

நிலையான தருக்க verilate(மாறிலி string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    எனில் (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        திரும்பு பொய்;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    எனில் (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(புதிய V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_உச்சி_Data_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_உச்சி_Data_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_உச்சி_Data_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    எனில் (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_எண்::selfTest();
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
    எனில் (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_மரம்("final", 990, dump_மரம்_Either_Level() >= 3);
    எனில் (v3Global.opt.jsonOnly() || dump_மரம்_Json_Level()) {
        மாறிலி string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    எனில் (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::லிக(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    எனில் (மாறிலி V3HierGraph* மாறிலி hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        எனில் (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(பொய்);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        எனில் (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(உண்மை);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    எனில் (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_உச்சி() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    எனில் (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    எனில் ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    எனில் (v3Global.opt.hier_உச்சி()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_உச்சி_Data_Dir());
    எனில் (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    எனில் (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", உண்மை);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    திரும்பு உண்மை;
}

நிலையான string buildMakeCmd(மாறிலி string& makefile, மாறிலி string& target) {
    மாறிலி VStringList& makeFlags = v3Global.opt.makeFlags();
    மாறிலி முழு jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    எனில் (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    எனில் (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        எனில் (jobs > 0) cmd << " -j " << jobs;
    }
    க்கு (மாறிலி string& flag : makeFlags) cmd << ' ' << flag;
    எனில் (!target.empty()) cmd << ' ' << target;

    திரும்பு cmd.str();
}

நிலையான வெறுமை releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("released");
}

நிலையான வெறுமை execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    மாறிலி VlOs::DeltaWallTime buildWallTime{உண்மை};
    UINFO(1, "Start Build");

    மாறிலி string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_உச்சி_Data_Dir());
    மாறிலி முழு exit_code = V3Os::system(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    எனில் (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

நிலையான வெறுமை execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    மாறிலி string makefile = v3Global.opt.prefix() + "_hier.mk ";
    மாறிலி string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    மாறிலி string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_உச்சி_Data_Dir());
    மாறிலி முழு exit_code = V3Os::system(cmdStr);
    எனில் (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

முழு முதன்மை(முழு argc, எழுத்து** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    மாறிலி VlOs::DeltaWallTime wallTimeTotal{உண்மை};
    மாறிலி VlOs::DeltaCpuTime cpuTimeTotal{உண்மை};

    time_t randseed;
    time(&randseed);
    srand(static_cast<முழு>(randseed));

    மாறிலி string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(புதிய File_வரி{File_வரி::command_வரி_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    தருக்க didVerilate = பொய்;
    எனில் (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } இல்லையெனில் {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    எனில் (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } இல்லையெனில் எனில் (v3Global.opt.build()) {
        execBuildJob();
    }

    எனில் (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(உண்மை);

    // Explicitly release resources
    v3Global.shutdown();

    எனில் (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
