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

#घोषणा VL_MT_CONTROL_CODE_UNIT 1

#समावेश "V3Active.h"
#समावेश "V3ActiveTop.h"
#समावेश "V3Assert.h"
#समावेश "V3AssertNfa.h"
#समावेश "V3AssertPre.h"
#समावेश "V3Ast.h"
#समावेश "V3AstPatterns.h"
#समावेश "V3Begin.h"
#समावेश "V3BitBlast.h"
#समावेश "V3Branch.h"
#समावेश "V3Broken.h"
#समावेश "V3CCtors.h"
#समावेश "V3CUse.h"
#समावेश "V3Case.h"
#समावेश "V3Cast.h"
#समावेश "V3Class.h"
#समावेश "V3Clean.h"
#समावेश "V3Clock.h"
#समावेश "V3Combine.h"
#समावेश "V3Common.h"
#समावेश "V3Const.h"
#समावेश "V3Control.h"
#समावेश "V3Coverage.h"
#समावेश "V3CoverageJoin.h"
#समावेश "V3Covergroup.h"
#समावेश "V3Dead.h"
#समावेश "V3Decompose.h"
#समावेश "V3Delayed.h"
#समावेश "V3Depth.h"
#समावेश "V3DepthBlock.h"
#समावेश "V3Descope.h"
#समावेश "V3DfgOptimizer.h"
#समावेश "V3DiagSarif.h"
#समावेश "V3EmitC.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3EmitCMain.h"
#समावेश "V3EmitMk.h"
#समावेश "V3EmitMkJson.h"
#समावेश "V3EmitV.h"
#समावेश "V3ExecGraph.h"
#समावेश "V3Expand.h"
#समावेश "V3File.h"
#समावेश "V3Force.h"
#समावेश "V3Fork.h"
#समावेश "V3FsmDetect.h"
#समावेश "V3FuncOpt.h"
#समावेश "V3Gate.h"
#समावेश "V3Global.h"
#समावेश "V3Graph.h"
#समावेश "V3HashTable.h"
#समावेश "V3HierBlock.h"
#समावेश "V3Inline.h"
#समावेश "V3InlineCFuncs.h"
#समावेश "V3Inst.h"
#समावेश "V3Interface.h"
#समावेश "V3LibMap.h"
#समावेश "V3Life.h"
#समावेश "V3LifePost.h"
#समावेश "V3LiftExpr.h"
#समावेश "V3LinkDot.h"
#समावेश "V3LinkDotIfaceCapture.h"
#समावेश "V3LinkInc.h"
#समावेश "V3LinkJump.h"
#समावेश "V3LinkLValue.h"
#समावेश "V3LinkLevel.h"
#समावेश "V3LinkParse.h"
#समावेश "V3LinkResolve.h"
#समावेश "V3LinkWith.h"
#समावेश "V3Localize.h"
#समावेश "V3MergeCond.h"
#समावेश "V3Name.h"
#समावेश "V3Order.h"
#समावेश "V3Os.h"
#समावेश "V3Param.h"
#समावेश "V3PreShell.h"
#समावेश "V3Premit.h"
#समावेश "V3ProtectLib.h"
#समावेश "V3RandSequence.h"
#समावेश "V3Randomize.h"
#समावेश "V3Reloop.h"
#समावेश "V3Reorder.h"
#समावेश "V3Sampled.h"
#समावेश "V3Sched.h"
#समावेश "V3Scope.h"
#समावेश "V3Slice.h"
#समावेश "V3Split.h"
#समावेश "V3Stats.h"
#समावेश "V3String.h"
#समावेश "V3Subst.h"
#समावेश "V3Table.h"
#समावेश "V3Task.h"
#समावेश "V3ThreadPool.h"
#समावेश "V3Timing.h"
#समावेश "V3Trace.h"
#समावेश "V3TraceDecl.h"
#समावेश "V3Tristate.h"
#समावेश "V3Udp.h"
#समावेश "V3Undriven.h"
#समावेश "V3Unknown.h"
#समावेश "V3Unroll.h"
#समावेश "V3VariableOrder.h"
#समावेश "V3Waiver.h"
#समावेश "V3Width.h"
#समावेश "V3WidthCommit.h"

#समावेश <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

अटल व्योम reportStatsIfEnabled() {
    अगर (v3Global.opt.stats()) {
        File_पंक्ति::stats();
        V3Stats::statsReport();
    }
}

अटल व्योम emitJson() VL_MT_DISABLED {
    स्थिर string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_वृक्ष_Json_File(filename);
}

अटल व्योम emitSerialized() VL_MT_DISABLED {
    अगर (v3Global.opt.jsonOnly()) emitJson();
}

अटल व्योम process() {
    {
        स्थिर VlOs::DeltaWallTime elabWallTime{true};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        अगर (v3Global.opt.debugExitParse()) {
            अ_बाहर << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_वृक्ष();  // Force a check, as link is most likely place for problems
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

        अगर (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_वृक्ष();
        अगर (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            अगर (v3Global.hierGraphp()) वापस;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(true);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        स्थिर VlOs::DeltaWallTime cvtWallTime{true};
        अगर (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            अगर (v3Global.opt.serializeOnly()) emitSerialized();
            अ_बाहर << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        अगर (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_शीर्ष(v3Global.rootp());
        } अन्यथा {
            V3LinkLevel::non_Wrap_शीर्ष(v3Global.rootp());
        }

        अगर (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            अगर (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        अगर (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        अगर (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        अगर (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

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
        अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        अगर (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            अगर (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(true);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        अगर (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_व्यंजक::lift_व्यंजक_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(true);
        }

        // Interface references feed trace file aliases and VPI name resolution
        अगर (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_वृक्ष();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_वृक्ष();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        अगर (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            अगर (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            अगर (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        अगर (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_वृक्ष();
        }

        अगर (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        अगर (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        अगर (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            अगर (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            अगर (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            अगर (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            अगर (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            अगर (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_वृक्ष();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            अगर (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            अगर (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            अगर (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            अगर (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } अन्यथा {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            अगर (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            अगर (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            अगर (v3Global.opt.timing().isSetTrue()) {
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
            V3_Active_शीर्ष::active_शीर्ष_All(v3Global.rootp());

            // Remove SAMPLED
            अगर (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            अगर (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            अगर (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            अगर (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            अगर (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            अगर (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        अगर (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            अगर (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(false);

            // Move variables from modules to function local variables where possible
            अगर (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            अगर (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        अगर (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            अगर (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            अगर (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

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
        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        अगर (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        अगर (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            अगर (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            अगर (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            अगर (v3Global.opt.fInlineCFuncs()) {
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
        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        अगर (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            अगर (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        अगर (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            स्थिर EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } अन्यथा अगर (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(true);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    अगर (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    अगर (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } अन्यथा अगर (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    अगर (!v3Global.opt.libCreate().empty()) {
        अगर (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    अगर (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        अगर (v3Global.opt.मुख्य()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        माप_प्रकार src_f_cnt = 0;
        क्रम (Ast_गाँठ* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        अगर (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(true);
        अगर (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        अगर (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    अगर (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

अटल bool verilate(स्थिर string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    अगर (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        वापस false;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    अगर (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(नया V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_तथ्य_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_तथ्य_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_तथ्य_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    अगर (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_संख्या::selfTest();
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
    अगर (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_वृक्ष("final", 990, dump_वृक्ष_Either_Level() >= 3);
    अगर (v3Global.opt.jsonOnly() || dump_वृक्ष_Json_Level()) {
        स्थिर string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    अगर (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::ग_लिखो(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    अगर (स्थिर V3HierGraph* स्थिर hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        अगर (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(false);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        अगर (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(true);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    अगर (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_शीर्ष() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    अगर (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    अगर ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    अगर (v3Global.opt.hier_शीर्ष()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_तथ्य_Dir());
    अगर (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    अगर (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", true);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    वापस true;
}

अटल string buildMakeCmd(स्थिर string& makefile, स्थिर string& target) {
    स्थिर VStringList& makeFlags = v3Global.opt.makeFlags();
    स्थिर पूर्णांक jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    अगर (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    अगर (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        अगर (jobs > 0) cmd << " -j " << jobs;
    }
    क्रम (स्थिर string& flag : makeFlags) cmd << ' ' << flag;
    अगर (!target.empty()) cmd << ' ' << target;

    वापस cmd.str();
}

अटल व्योम releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    अगर (v3Global.opt.stats()) V3Stats::statsStage("released");
}

अटल व्योम execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    स्थिर VlOs::DeltaWallTime buildWallTime{true};
    UINFO(1, "Start Build");

    स्थिर string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_तथ्य_Dir());
    स्थिर पूर्णांक exit_code = V3Os::प्रणाली(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    अगर (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::प_पूर्ण);
        v3Global.vlExit(exit_code);
    }
}

अटल व्योम execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    स्थिर string makefile = v3Global.opt.prefix() + "_hier.mk ";
    स्थिर string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    स्थिर string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_तथ्य_Dir());
    स्थिर पूर्णांक exit_code = V3Os::प्रणाली(cmdStr);
    अगर (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::प_पूर्ण);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

पूर्णांक मुख्य(पूर्णांक argc, अक्षर** argv) {
    // General initialization
    std::पनप::sync_with_stdio();
    स्थिर VlOs::DeltaWallTime wall_Time_योग{true};
    स्थिर VlOs::DeltaCpuTime cpu_Time_योग{true};

    समय_प्रकार randseed;
    समय(&randseed);
    बेक्रम(static_cast<पूर्णांक>(randseed));

    स्थिर string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    bool didVerilate = false;
    अगर (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } अन्यथा {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    अगर (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } अन्यथा अगर (v3Global.opt.build()) {
        execBuildJob();
    }

    अगर (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(true);

    // Explicitly release resources
    v3Global.shutdown();

    अगर (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpu_Time_योग.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wall_Time_योग.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
