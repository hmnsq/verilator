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

#अन्तर्भाव "V3Active.h"
#अन्तर्भाव "V3ActiveTop.h"
#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3AssertNfa.h"
#अन्तर्भाव "V3AssertPre.h"
#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3AstPatterns.h"
#अन्तर्भाव "V3Begin.h"
#अन्तर्भाव "V3BitBlast.h"
#अन्तर्भाव "V3Branch.h"
#अन्तर्भाव "V3Broken.h"
#अन्तर्भाव "V3CCtors.h"
#अन्तर्भाव "V3CUse.h"
#अन्तर्भाव "V3Case.h"
#अन्तर्भाव "V3Cast.h"
#अन्तर्भाव "V3Class.h"
#अन्तर्भाव "V3Clean.h"
#अन्तर्भाव "V3Clock.h"
#अन्तर्भाव "V3Combine.h"
#अन्तर्भाव "V3Common.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Coverage.h"
#अन्तर्भाव "V3CoverageJoin.h"
#अन्तर्भाव "V3Covergroup.h"
#अन्तर्भाव "V3Dead.h"
#अन्तर्भाव "V3Decompose.h"
#अन्तर्भाव "V3Delayed.h"
#अन्तर्भाव "V3Depth.h"
#अन्तर्भाव "V3DepthBlock.h"
#अन्तर्भाव "V3Descope.h"
#अन्तर्भाव "V3DfgOptimizer.h"
#अन्तर्भाव "V3DiagSarif.h"
#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3EmitCMain.h"
#अन्तर्भाव "V3EmitMk.h"
#अन्तर्भाव "V3EmitMkJson.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3Expand.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Force.h"
#अन्तर्भाव "V3Fork.h"
#अन्तर्भाव "V3FsmDetect.h"
#अन्तर्भाव "V3FuncOpt.h"
#अन्तर्भाव "V3Gate.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3HashTable.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3Inline.h"
#अन्तर्भाव "V3InlineCFuncs.h"
#अन्तर्भाव "V3Inst.h"
#अन्तर्भाव "V3Interface.h"
#अन्तर्भाव "V3LibMap.h"
#अन्तर्भाव "V3Life.h"
#अन्तर्भाव "V3LifePost.h"
#अन्तर्भाव "V3LiftExpr.h"
#अन्तर्भाव "V3LinkDot.h"
#अन्तर्भाव "V3LinkDotIfaceCapture.h"
#अन्तर्भाव "V3LinkInc.h"
#अन्तर्भाव "V3LinkJump.h"
#अन्तर्भाव "V3LinkLValue.h"
#अन्तर्भाव "V3LinkLevel.h"
#अन्तर्भाव "V3LinkParse.h"
#अन्तर्भाव "V3LinkResolve.h"
#अन्तर्भाव "V3LinkWith.h"
#अन्तर्भाव "V3Localize.h"
#अन्तर्भाव "V3MergeCond.h"
#अन्तर्भाव "V3Name.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Param.h"
#अन्तर्भाव "V3PreShell.h"
#अन्तर्भाव "V3Premit.h"
#अन्तर्भाव "V3ProtectLib.h"
#अन्तर्भाव "V3RandSequence.h"
#अन्तर्भाव "V3Randomize.h"
#अन्तर्भाव "V3Reloop.h"
#अन्तर्भाव "V3Reorder.h"
#अन्तर्भाव "V3Sampled.h"
#अन्तर्भाव "V3Sched.h"
#अन्तर्भाव "V3Scope.h"
#अन्तर्भाव "V3Slice.h"
#अन्तर्भाव "V3Split.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3Subst.h"
#अन्तर्भाव "V3Table.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3ThreadPool.h"
#अन्तर्भाव "V3Timing.h"
#अन्तर्भाव "V3Trace.h"
#अन्तर्भाव "V3TraceDecl.h"
#अन्तर्भाव "V3Tristate.h"
#अन्तर्भाव "V3Udp.h"
#अन्तर्भाव "V3Undriven.h"
#अन्तर्भाव "V3Unknown.h"
#अन्तर्भाव "V3Unroll.h"
#अन्तर्भाव "V3VariableOrder.h"
#अन्तर्भाव "V3Waiver.h"
#अन्तर्भाव "V3Width.h"
#अन्तर्भाव "V3WidthCommit.h"

#अन्तर्भाव <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

स्थिर शून्य reportStatsIfEnabled() {
    यदि (v3Global.opt.stats()) {
        File_पङ्क्ति::stats();
        V3Stats::statsReport();
    }
}

स्थिर शून्य emitJson() VL_MT_DISABLED {
    अचर string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_रूख_Json_File(filename);
}

स्थिर शून्य emitSerialized() VL_MT_DISABLED {
    यदि (v3Global.opt.jsonOnly()) emitJson();
}

स्थिर शून्य process() {
    {
        अचर VlOs::DeltaWallTime elabWallTime{सत्य};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfErrors();
        यदि (v3Global.opt.debugExitParse()) {
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_रूख();  // Force a check, as link is most likely place for problems
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

        यदि (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_रूख();
        यदि (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.hierarchical() && !v3Global.opt.hierChild()) {
            V3Hierarchical::createGraph(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            यदि (v3Global.hierGraphp()) फर्काऊ;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(सत्य);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        अचर VlOs::DeltaWallTime cvtWallTime{सत्य};
        यदि (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            यदि (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        यदि (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_शीर्ष(v3Global.rootp());
        } अन्यथा {
            V3LinkLevel::non_Wrap_शीर्ष(v3Global.rootp());
        }

        यदि (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            यदि (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        यदि (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        यदि (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        यदि (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

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
        यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        यदि (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            यदि (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(सत्य);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        यदि (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_अभिव्यक्ति::lift_अभिव्यक्ति_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(सत्य);
        }

        // Interface references feed trace file aliases and VPI name resolution
        यदि (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_रूख();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_रूख();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        यदि (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            यदि (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            यदि (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        यदि (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_रूख();
        }

        यदि (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        यदि (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        यदि (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            यदि (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            यदि (v3Global.opt.fBitblast()) V3BitBlast::bitBlastAll(v3Global.rootp());

            यदि (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            यदि (!v3Global.opt.lintOnly() && v3Global.opt.fTable()) {
                V3Table::tableAll(v3Global.rootp());
            }

            // Cleanup
            यदि (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_रूख();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            यदि (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            यदि (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            यदि (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            यदि (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } अन्यथा {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            यदि (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            यदि (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            यदि (v3Global.opt.timing().isSetTrue()) {
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
            यदि (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            यदि (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::schedule(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            यदि (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            यदि (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            यदि (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            यदि (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        यदि (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            यदि (!v3Global.opt.lintOnly() && v3Global.opt.compLimitBlocks()) {
                V3DepthBlock::depthBlockAll(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(असत्य);

            // Move variables from modules to function local variables where possible
            यदि (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            यदि (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        यदि (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            यदि (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            यदि (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

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
        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        यदि (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        यदि (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            यदि (v3Global.opt.fMergeCond()) {
                // Merge conditionals
                V3MergeCond::mergeAll(v3Global.rootp());
            }

            यदि (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            यदि (v3Global.opt.fInlineCFuncs()) {
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
        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        यदि (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3ExecGraph::implement(v3Global.rootp());
        }

        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3VariableOrder::orderAll(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            यदि (!v3Global.opt.libCreate().empty()) {
                v3Global.currentHierBlockCost(V3Control::getCurrentHierBlockCost());
            }
        }

        यदि (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            अचर EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitcExecGraph();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitcHeaders();
        } अन्यथा यदि (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(सत्य);
        }

        // End of conversion
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_CVT, cvtWallTime.deltaTime());
    }
    यदि (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    यदि (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } अन्यथा यदि (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    यदि (!v3Global.opt.libCreate().empty()) {
        यदि (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    यदि (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        यदि (v3Global.opt.main()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        माप_प्रकार src_f_cnt = 0;
        लागि (Ast_गाँठो* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->source() ? 1 : 0;
        }
        यदि (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_THRESHOLD) v3Global.useParallelBuild(सत्य);
        यदि (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        यदि (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    यदि (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

स्थिर तार्किक verilate(अचर string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    यदि (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        फर्काऊ असत्य;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    यदि (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(नूतन V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_Data_Dir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_Data_Dir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_शीर्ष_Data_Dir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    यदि (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_सङ्ख्या::selfTest();
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
    यदि (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_रूख("final", 990, dump_रूख_Either_Level() >= 3);
    यदि (v3Global.opt.jsonOnly() || dump_रूख_Json_Level()) {
        अचर string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    यदि (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::लिख(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    यदि (अचर V3HierGraph* अचर hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.hierarchical(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hierChild(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hierBlocks().empty(), "hierarchical-block must not be set");
        यदि (v3Global.opt.gmake()) {
            hierGraphp->writeCommandArgsFiles(असत्य);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        यदि (v3Global.opt.makeJson()) {
            hierGraphp->writeCommandArgsFiles(सत्य);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    यदि (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.prefix();
        filename += v3Global.opt.hier_शीर्ष() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    यदि (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__idmap.xml");
    }

    यदि ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    यदि (v3Global.opt.hier_शीर्ष()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_Data_Dir());
    यदि (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    यदि (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", सत्य);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    फर्काऊ सत्य;
}

स्थिर string buildMakeCmd(अचर string& makefile, अचर string& target) {
    अचर VStringList& makeFlags = v3Global.opt.makeFlags();
    अचर पूर्णाङ्क jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    यदि (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    यदि (v3Global.opt.getenvMAKEFLAGS().find("-jobserver-auth") == string::npos) {
        यदि (jobs > 0) cmd << " -j " << jobs;
    }
    लागि (अचर string& flag : makeFlags) cmd << ' ' << flag;
    यदि (!target.empty()) cmd << ' ' << target;

    फर्काऊ cmd.str();
}

स्थिर शून्य releaseNetlistMemory() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::releaseMemory();
    यदि (v3Global.opt.stats()) V3Stats::statsStage("released");
}

स्थिर शून्य execBuildJob() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    releaseNetlistMemory();
    अचर VlOs::DeltaWallTime buildWallTime{सत्य};
    UINFO(1, "Start Build");

    अचर string cmdStr = buildMakeCmd(v3Global.opt.prefix() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_Data_Dir());
    अचर पूर्णाङ्क exit_code = V3Os::प्रणाली(cmdStr);
    V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_BUILD, buildWallTime.deltaTime());

    यदि (exit_code != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

स्थिर शून्य execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    releaseNetlistMemory();
    अचर string makefile = v3Global.opt.prefix() + "_hier.mk ";
    अचर string target = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    अचर string cmdStr = buildMakeCmd(makefile, target);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_शीर्ष_Data_Dir());
    अचर पूर्णाङ्क exit_code = V3Os::प्रणाली(cmdStr);
    यदि (exit_code != 0) {
        v3error(cmdStr << " exited with " << exit_code << std::endl);
        v3Global.vlExit(exit_code);
    }
}

//######################################################################

पूर्णाङ्क main(पूर्णाङ्क argc, वर्ण** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    अचर VlOs::DeltaWallTime wallTimeTotal{सत्य};
    अचर VlOs::DeltaCpuTime cpuTimeTotal{सत्य};

    समय_प्रकार randseed;
    समय(&randseed);
    बेक्रम(static_cast<पूर्णाङ्क>(randseed));

    अचर string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(नूतन File_पङ्क्ति{File_पङ्क्ति::command_पङ्क्ति_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    तार्किक didVerilate = असत्य;
    यदि (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } अन्यथा {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    यदि (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } अन्यथा यदि (v3Global.opt.build()) {
        execBuildJob();
    }

    यदि (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(सत्य);

    // Explicitly release resources
    v3Global.shutdown();

    यदि (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
