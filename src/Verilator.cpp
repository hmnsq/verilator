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

#അന്തര്ഭാവ "V3Active.h"
#അന്തര്ഭാവ "V3ActiveTop.h"
#അന്തര്ഭാവ "V3Assert.h"
#അന്തര്ഭാവ "V3AssertNfa.h"
#അന്തര്ഭാവ "V3AssertPre.h"
#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3AstPatterns.h"
#അന്തര്ഭാവ "V3Begin.h"
#അന്തര്ഭാവ "V3BitBlast.h"
#അന്തര്ഭാവ "V3Branch.h"
#അന്തര്ഭാവ "V3Broken.h"
#അന്തര്ഭാവ "V3CCtors.h"
#അന്തര്ഭാവ "V3CUse.h"
#അന്തര്ഭാവ "V3Case.h"
#അന്തര്ഭാവ "V3Cast.h"
#അന്തര്ഭാവ "V3Class.h"
#അന്തര്ഭാവ "V3Clean.h"
#അന്തര്ഭാവ "V3Clock.h"
#അന്തര്ഭാവ "V3Combine.h"
#അന്തര്ഭാവ "V3Common.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Coverage.h"
#അന്തര്ഭാവ "V3CoverageJoin.h"
#അന്തര്ഭാവ "V3Covergroup.h"
#അന്തര്ഭാവ "V3Dead.h"
#അന്തര്ഭാവ "V3Decompose.h"
#അന്തര്ഭാവ "V3Delayed.h"
#അന്തര്ഭാവ "V3Depth.h"
#അന്തര്ഭാവ "V3DepthBlock.h"
#അന്തര്ഭാവ "V3Descope.h"
#അന്തര്ഭാവ "V3DfgOptimizer.h"
#അന്തര്ഭാവ "V3DiagSarif.h"
#അന്തര്ഭാവ "V3EmitC.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3EmitCMain.h"
#അന്തര്ഭാവ "V3EmitMk.h"
#അന്തര്ഭാവ "V3EmitMkJson.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3Expand.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Force.h"
#അന്തര്ഭാവ "V3Fork.h"
#അന്തര്ഭാവ "V3FsmDetect.h"
#അന്തര്ഭാവ "V3FuncOpt.h"
#അന്തര്ഭാവ "V3Gate.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3HashTable.h"
#അന്തര്ഭാവ "V3HierBlock.h"
#അന്തര്ഭാവ "V3Inline.h"
#അന്തര്ഭാവ "V3InlineCFuncs.h"
#അന്തര്ഭാവ "V3Inst.h"
#അന്തര്ഭാവ "V3Interface.h"
#അന്തര്ഭാവ "V3LibMap.h"
#അന്തര്ഭാവ "V3Life.h"
#അന്തര്ഭാവ "V3LifePost.h"
#അന്തര്ഭാവ "V3LiftExpr.h"
#അന്തര്ഭാവ "V3LinkDot.h"
#അന്തര്ഭാവ "V3LinkDotIfaceCapture.h"
#അന്തര്ഭാവ "V3LinkInc.h"
#അന്തര്ഭാവ "V3LinkJump.h"
#അന്തര്ഭാവ "V3LinkLValue.h"
#അന്തര്ഭാവ "V3LinkLevel.h"
#അന്തര്ഭാവ "V3LinkParse.h"
#അന്തര്ഭാവ "V3LinkResolve.h"
#അന്തര്ഭാവ "V3LinkWith.h"
#അന്തര്ഭാവ "V3Localize.h"
#അന്തര്ഭാവ "V3MergeCond.h"
#അന്തര്ഭാവ "V3Name.h"
#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Param.h"
#അന്തര്ഭാവ "V3PreShell.h"
#അന്തര്ഭാവ "V3Premit.h"
#അന്തര്ഭാവ "V3ProtectLib.h"
#അന്തര്ഭാവ "V3RandSequence.h"
#അന്തര്ഭാവ "V3Randomize.h"
#അന്തര്ഭാവ "V3Reloop.h"
#അന്തര്ഭാവ "V3Reorder.h"
#അന്തര്ഭാവ "V3Sampled.h"
#അന്തര്ഭാവ "V3Sched.h"
#അന്തര്ഭാവ "V3Scope.h"
#അന്തര്ഭാവ "V3Slice.h"
#അന്തര്ഭാവ "V3Split.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3Subst.h"
#അന്തര്ഭാവ "V3Table.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3ThreadPool.h"
#അന്തര്ഭാവ "V3Timing.h"
#അന്തര്ഭാവ "V3Trace.h"
#അന്തര്ഭാവ "V3TraceDecl.h"
#അന്തര്ഭാവ "V3Tristate.h"
#അന്തര്ഭാവ "V3Udp.h"
#അന്തര്ഭാവ "V3Undriven.h"
#അന്തര്ഭാവ "V3Unknown.h"
#അന്തര്ഭാവ "V3Unroll.h"
#അന്തര്ഭാവ "V3VariableOrder.h"
#അന്തര്ഭാവ "V3Waiver.h"
#അന്തര്ഭാവ "V3Width.h"
#അന്തര്ഭാവ "V3WidthCommit.h"

#അന്തര്ഭാവ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

സ്ഥിതം ശൂന്യം reportStatsIfEnabled() {
    എങ്കിൽ (v3Global.opt.stats()) {
        File_വരി::stats();
        V3Stats::statsReport();
    }
}

സ്ഥിതം ശൂന്യം emitJson() VL_MT_DISABLED {
    സ്ഥിരം string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_വൃക്ഷം_Json_File(filename);
}

സ്ഥിതം ശൂന്യം emitSerialized() VL_MT_DISABLED {
    എങ്കിൽ (v3Global.opt.jsonOnly()) emitJson();
}

സ്ഥിതം ശൂന്യം process() {
    {
        സ്ഥിരം VlOs::DeltaWallTime elabWallTime{സത്യം};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        എങ്കിൽ (v3Global.opt.debugExitParse()) {
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_വൃക്ഷം();  // Force a check, as link is most likely place for problems
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

        എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_വൃക്ഷം();
        എങ്കിൽ (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            എങ്കിൽ (v3Global.hierGraphp()) തിരികെ;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(സത്യം);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        സ്ഥിരം VlOs::DeltaWallTime cvtWallTime{സത്യം};
        എങ്കിൽ (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            എങ്കിൽ (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_മുകൾ(v3Global.rootp());
        } അല്ലെങ്കിൽ {
            V3LinkLevel::non_Wrap_മുകൾ(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            എങ്കിൽ (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        എങ്കിൽ (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        എങ്കിൽ (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        എങ്കിൽ (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

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
        എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        എങ്കിൽ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(സത്യം);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_വ്യഞ്ജകം::lift_വ്യഞ്ജകം_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(സത്യം);
        }

        // Interface references feed trace file aliases and VPI name resolution
        എങ്കിൽ (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_വൃക്ഷം();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_വൃക്ഷം();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        എങ്കിൽ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            എങ്കിൽ (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            എങ്കിൽ (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        എങ്കിൽ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_വൃക്ഷം();
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        എങ്കിൽ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            എങ്കിൽ (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            എങ്കിൽ (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            എങ്കിൽ (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            എങ്കിൽ (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            എങ്കിൽ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_വൃക്ഷം();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            എങ്കിൽ (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            എങ്കിൽ (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            എങ്കിൽ (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            എങ്കിൽ (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } അല്ലെങ്കിൽ {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            എങ്കിൽ (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            എങ്കിൽ (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
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
            V3_Active_മുകൾ::active_മുകൾ_All(v3Global.rootp());

            // Remove SAMPLED
            എങ്കിൽ (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            എങ്കിൽ (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            എങ്കിൽ (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            എങ്കിൽ (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            എങ്കിൽ (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(അസത്യം);

            // Move variables from modules to function local variables where possible
            എങ്കിൽ (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            എങ്കിൽ (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        എങ്കിൽ (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            എങ്കിൽ (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            എങ്കിൽ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

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
        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        എങ്കിൽ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            എങ്കിൽ (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            എങ്കിൽ (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            എങ്കിൽ (v3Global.opt.fInlineCFuncs()) {
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
        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        എങ്കിൽ (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        എങ്കിൽ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            സ്ഥിരം EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(സത്യം);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    എങ്കിൽ (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    എങ്കിൽ (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
        എങ്കിൽ (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    എങ്കിൽ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        എങ്കിൽ (v3Global.opt.പ്രധാന()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        size_t src_f_cnt = 0;
        വേണ്ടി (Ast_കണ്ണി* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        എങ്കിൽ (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(സത്യം);
        എങ്കിൽ (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        എങ്കിൽ (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

സ്ഥിതം തർക്കം verilate(സ്ഥിരം string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    എങ്കിൽ (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        തിരികെ അസത്യം;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    എങ്കിൽ (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(പുതിയ V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_മുകൾ_Data_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_മുകൾ_Data_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_മുകൾ_Data_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    എങ്കിൽ (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_സംഖ്യ::selfTest();
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
    എങ്കിൽ (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_വൃക്ഷം("final", 990, dump_വൃക്ഷം_Either_Level() >= 3);
    എങ്കിൽ (v3Global.opt.jsonOnly() || dump_വൃക്ഷം_Json_Level()) {
        സ്ഥിരം string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    എങ്കിൽ (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::ലിഖ(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    എങ്കിൽ (സ്ഥിരം V3HierGraph* സ്ഥിരം hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        എങ്കിൽ (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(അസത്യം);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        എങ്കിൽ (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(സത്യം);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    എങ്കിൽ (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_മുകൾ() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    എങ്കിൽ (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    എങ്കിൽ ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    എങ്കിൽ (v3Global.opt.hier_മുകൾ()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_മുകൾ_Data_Dir());
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", സത്യം);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    തിരികെ സത്യം;
}

സ്ഥിതം string buildMakeCmd(സ്ഥിരം string& makefile, സ്ഥിരം string& target) {
    സ്ഥിരം VStringList& makeFlags = v3Global.opt.makeFlags();
    സ്ഥിരം പൂർണ്ണസംഖ്യ jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    എങ്കിൽ (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    എങ്കിൽ (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        എങ്കിൽ (jobs > 0) cmd << " -j " << jobs;
    }
    വേണ്ടി (സ്ഥിരം string& flag : makeFlags) cmd << ' ' << flag;
    എങ്കിൽ (!target.empty()) cmd << ' ' << target;

    തിരികെ cmd.str();
}

സ്ഥിതം ശൂന്യം releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("released");
}

സ്ഥിതം ശൂന്യം execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    സ്ഥിരം VlOs::DeltaWallTime buildWallTime{സത്യം};
    UINFO(1, "Start Build");

    സ്ഥിരം string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_മുകൾ_Data_Dir());
    സ്ഥിരം പൂർണ്ണസംഖ്യ exit_code = V3Os::system(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    എങ്കിൽ (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

സ്ഥിതം ശൂന്യം execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    സ്ഥിരം string makefile = v3Global.opt.prefix() + "_hier.mk ";
    സ്ഥിരം string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    സ്ഥിരം string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_മുകൾ_Data_Dir());
    സ്ഥിരം പൂർണ്ണസംഖ്യ exit_code = V3Os::system(cmdStr);
    എങ്കിൽ (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

പൂർണ്ണസംഖ്യ പ്രധാന(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    സ്ഥിരം VlOs::DeltaWallTime wallTimeTotal{സത്യം};
    സ്ഥിരം VlOs::DeltaCpuTime cpuTimeTotal{സത്യം};

    time_t randseed;
    time(&randseed);
    srand(static_cast<പൂർണ്ണസംഖ്യ>(randseed));

    സ്ഥിരം string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(പുതിയ File_വരി{File_വരി::command_വരി_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    തർക്കം didVerilate = അസത്യം;
    എങ്കിൽ (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } അല്ലെങ്കിൽ {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    എങ്കിൽ (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.build()) {
        execBuildJob();
    }

    എങ്കിൽ (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(സത്യം);

    // Explicitly release resources
    v3Global.shutdown();

    എങ്കിൽ (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
