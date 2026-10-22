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

#నిర్వచించు VL_MT_CONTROL_సంకేతం_UNIT 1

#అన్తర్భావ "V3Active.h"
#అన్తర్భావ "V3ActiveTop.h"
#అన్తర్భావ "V3Assert.h"
#అన్తర్భావ "V3AssertNfa.h"
#అన్తర్భావ "V3AssertPre.h"
#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3AstPatterns.h"
#అన్తర్భావ "V3Begin.h"
#అన్తర్భావ "V3BitBlast.h"
#అన్తర్భావ "V3Branch.h"
#అన్తర్భావ "V3Broken.h"
#అన్తర్భావ "V3CCtors.h"
#అన్తర్భావ "V3CUse.h"
#అన్తర్భావ "V3Case.h"
#అన్తర్భావ "V3Cast.h"
#అన్తర్భావ "V3Class.h"
#అన్తర్భావ "V3Clean.h"
#అన్తర్భావ "V3Clock.h"
#అన్తర్భావ "V3Combine.h"
#అన్తర్భావ "V3Common.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Coverage.h"
#అన్తర్భావ "V3CoverageJoin.h"
#అన్తర్భావ "V3Covergroup.h"
#అన్తర్భావ "V3Dead.h"
#అన్తర్భావ "V3Decompose.h"
#అన్తర్భావ "V3Delayed.h"
#అన్తర్భావ "V3Depth.h"
#అన్తర్భావ "V3DepthBlock.h"
#అన్తర్భావ "V3Descope.h"
#అన్తర్భావ "V3DfgOptimizer.h"
#అన్తర్భావ "V3DiagSarif.h"
#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3EmitCMain.h"
#అన్తర్భావ "V3EmitMk.h"
#అన్తర్భావ "V3EmitMkJson.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3Expand.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Force.h"
#అన్తర్భావ "V3Fork.h"
#అన్తర్భావ "V3FsmDetect.h"
#అన్తర్భావ "V3FuncOpt.h"
#అన్తర్భావ "V3Gate.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3HashTable.h"
#అన్తర్భావ "V3HierBlock.h"
#అన్తర్భావ "V3Inline.h"
#అన్తర్భావ "V3InlineCFuncs.h"
#అన్తర్భావ "V3Inst.h"
#అన్తర్భావ "V3Interface.h"
#అన్తర్భావ "V3LibMap.h"
#అన్తర్భావ "V3Life.h"
#అన్తర్భావ "V3LifePost.h"
#అన్తర్భావ "V3LiftExpr.h"
#అన్తర్భావ "V3LinkDot.h"
#అన్తర్భావ "V3LinkDotIfaceCapture.h"
#అన్తర్భావ "V3LinkInc.h"
#అన్తర్భావ "V3LinkJump.h"
#అన్తర్భావ "V3LinkLValue.h"
#అన్తర్భావ "V3LinkLevel.h"
#అన్తర్భావ "V3LinkParse.h"
#అన్తర్భావ "V3LinkResolve.h"
#అన్తర్భావ "V3LinkWith.h"
#అన్తర్భావ "V3Localize.h"
#అన్తర్భావ "V3MergeCond.h"
#అన్తర్భావ "V3Name.h"
#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Param.h"
#అన్తర్భావ "V3PreShell.h"
#అన్తర్భావ "V3Premit.h"
#అన్తర్భావ "V3ProtectLib.h"
#అన్తర్భావ "V3RandSequence.h"
#అన్తర్భావ "V3Randomize.h"
#అన్తర్భావ "V3Reloop.h"
#అన్తర్భావ "V3Reorder.h"
#అన్తర్భావ "V3Sampled.h"
#అన్తర్భావ "V3Sched.h"
#అన్తర్భావ "V3Scope.h"
#అన్తర్భావ "V3Slice.h"
#అన్తర్భావ "V3Split.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3Subst.h"
#అన్తర్భావ "V3Table.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3ThreadPool.h"
#అన్తర్భావ "V3Timing.h"
#అన్తర్భావ "V3Trace.h"
#అన్తర్భావ "V3TraceDecl.h"
#అన్తర్భావ "V3Tristate.h"
#అన్తర్భావ "V3Udp.h"
#అన్తర్భావ "V3Undriven.h"
#అన్తర్భావ "V3Unknown.h"
#అన్తర్భావ "V3Unroll.h"
#అన్తర్భావ "V3VariableOrder.h"
#అన్తర్భావ "V3Waiver.h"
#అన్తర్భావ "V3Width.h"
#అన్తర్భావ "V3WidthCommit.h"

#అన్తర్భావ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

స్థిర శూన్య reportStatsIfEnabled() {
    ఒకవేళ (v3Global.opt.stats()) {
        File_పంక్తి::stats();
        V3Stats::statsReport();
    }
}

స్థిర శూన్య emitJson() VL_MT_DISABLED {
    స్థిరరాశి string filename
        = (v3Global.opt.jsonOnlyOutput().ఖాళీలు()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_వృక్షం_Json_File(filename);
}

స్థిర శూన్య emitSerialized() VL_MT_DISABLED {
    ఒకవేళ (v3Global.opt.jsonOnly()) emitJson();
}

స్థిర శూన్య process() {
    {
        స్థిరరాశి VlOs::Delta_Wall_సమయం elab_Wall_సమయం{నిజం};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::mod_క్రమీకరించు_By_Level();
        V3Error::abortIfErrors();
        ఒకవేళ (v3Global.opt.debugExitParse()) {
            బయటకు << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_వృక్షం();  // Force a check, as link is most likely place for problems
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

        ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_వృక్షం();
        ఒకవేళ (v3Global.has_పట్టిక()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.క్రమానుగత() && !v3Global.opt.hier_బిడ్డ()) {
            V3_క్రమానుగత::create_లేఖాచిత్రం(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            ఒకవేళ (v3Global.hierGraphp()) తిరిగి;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3_వెడల్పు::వెడల్పు(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3_వెడల్పు_Commit::వెడల్పు_Commit(v3Global.rootp());
        v3Global.assertDTypesResolved(నిజం);
        v3Global.వెడల్పు_Min_Usage(VWidthMinUsage::MATCHES_వెడల్పు);

        // End of elaboration
        V3Stats::కూడు_Stat_Perf(V3Stats::STAT_WALLTIME_ELAB, elab_Wall_సమయం.delta_సమయం());
        స్థిరరాశి VlOs::Delta_Wall_సమయం cvt_Wall_సమయం{నిజం};
        ఒకవేళ (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            ఒకవేళ (v3Global.opt.serializeOnly()) emitSerialized();
            బయటకు << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        ఒకవేళ (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_శిఖరం(v3Global.rootp());
        } లేకపోతే {
            V3LinkLevel::non_Wrap_శిఖరం(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            ఒకవేళ (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        ఒకవేళ (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        ఒకవేళ (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        ఒకవేళ (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constify_All_సజీవ(v3Global.rootp());

        // Signal based lint checks, no change to structures
        // Must be before first constification pass drops dead code
        V3Undriven::undrivenAll(v3Global.rootp());

        // Assertion insertion
        //    After we've added block coverage, but before other nasty transforms
        V3AssertCommon::collectDefaultDisable(v3Global.rootp());
        V3AssertCommon::క్రింది_Sequence_Events(v3Global.rootp());
        V3AssertNfa::assertNfaAll(v3Global.rootp());
        // V3AssertProp removed: NFA subsumes multi-cycle property lowering.
        // Unsupported constructs fall through to V3AssertPre.
        V3AssertPre::assertPreAll(v3Global.rootp());
        //
        V3Assert::assertAll(v3Global.rootp());

        // Propagate constants into expressions
        ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        ఒకవేళ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(నిజం);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_సమాసం::lift_సమాసం_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(నిజం);
        }

        // Interface references feed trace file aliases and VPI name resolution
        ఒకవేళ (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_వృక్షం();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_వృక్షం();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        ఒకవేళ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            ఒకవేళ (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            ఒకవేళ (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        ఒకవేళ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_వృక్షం();
        }

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        ఒకవేళ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3_ముక్క::ముక్క_All(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3_చురుకైన::చురుకైన_All(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            ఒకవేళ (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            ఒకవేళ (v3Global.opt.fBitblast()) V3_బిట్టు_Blast::బిట్టు_Blast_All(v3Global.rootp());

            ఒకవేళ (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            ఒకవేళ (!v3Global.opt.lintOnly() && v3Global.opt.f_పట్టిక()) {
                V3_పట్టిక::పట్టిక_All(v3Global.rootp());
            }

            // Cleanup
            ఒకవేళ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_వృక్షం();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            ఒకవేళ (v3Global.opt.f_విభజించు()) V3_విభజించు::విభజించు_All(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            ఒకవేళ (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::force_And_కేటాయించు_All(v3Global.rootp());

            // DFG optimization
            ఒకవేళ (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            ఒకవేళ (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } లేకపోతే {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            ఒకవేళ (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            ఒకవేళ (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
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
            V3_చురుకైన_శిఖరం::చురుకైన_శిఖరం_All(v3Global.rootp());

            // Remove SAMPLED
            ఒకవేళ (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::కాలపట్టిక(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            ఒకవేళ (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            ఒకవేళ (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            ఒకవేళ (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            ఒకవేళ (!v3Global.opt.lintOnly() && v3Global.opt.comp_హద్దు_ఖండాలు()) {
                V3_లోతు_ఖండం::లోతు_ఖండం_All(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(అబద్ధం);

            // Move variables from modules to function local variables where possible
            ఒకవేళ (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            ఒకవేళ (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        ఒకవేళ (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            ఒకవేళ (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            ఒకవేళ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

            // Here down, widthMin() is the Verilog width, and width() is the C++ width
            // Bits between widthMin() and width() are irrelevant, but may be non-zero.
            v3Global.వెడల్పు_Min_Usage(VWidthMinUsage::VERILOG_వెడల్పు);

            // Make all expressions 32, 64, or 32*N bits
            // Variables and selects-of-variables remain verilog-width
            V3Clean::cleanAll(v3Global.rootp());

            // Move wide constants to BLOCK temps / ConstPool.
            V3Premit::premitAll(v3Global.rootp());
        }

        // Expand macros and wide operators into C++ primitives
        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        ఒకవేళ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ఒకవేళ (v3Global.opt.f_విలీనం_Cond()) {
                // Merge conditionals
                V3_విలీనం_Cond::విలీనం_All(v3Global.rootp());
            }

            ఒకవేళ (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ఒకవేళ (v3Global.opt.fInlineCFuncs()) {
                // Inline small CFuncs to reduce function call overhead
                V3InlineCFuncs::inlineAll(v3Global.rootp());
            }

            // Fix very deep expressions
            // Mark evaluation functions as member functions, if needed.
            V3_లోతు::లోతు_All(v3Global.rootp());

            // Branch prediction
            V3Branch::branchAll(v3Global.rootp());

            // Add C casts when longs need to become long-long and vice-versa
            // Note depth may insert something needing a cast, so this must be last.
            V3Cast::castAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();
        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        ఒకవేళ (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3_Exec_లేఖాచిత్రం::implement(v3Global.rootp());
        }

        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3_Variable_క్రమం::క్రమం_All(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
                v3Global.current_Hier_ఖండం_ఖర్చు(V3Control::get_Current_Hier_ఖండం_ఖర్చు());
            }
        }

        ఒకవేళ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            స్థిరరాశి EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitc_Exec_లేఖాచిత్రం();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitc_శీర్షికలు();
        } లేకపోతే ఒకవేళ (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(నిజం);
        }

        // End of conversion
        V3Stats::కూడు_Stat_Perf(V3Stats::STAT_WALLTIME_CVT, cvt_Wall_సమయం.delta_సమయం());
    }
    ఒకవేళ (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    ఒకవేళ (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } లేకపోతే ఒకవేళ (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
        ఒకవేళ (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    ఒకవేళ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        ఒకవేళ (v3Global.opt.ప్రధాన()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        కొలత_t src_f_cnt = 0;
        కొరకు (Ast_కణుపు* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->ఆధారం() ? 1 : 0;
        }
        ఒకవేళ (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_పరిమితి) v3Global.useParallelBuild(నిజం);
        ఒకవేళ (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        ఒకవేళ (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

స్థిర తార్కిక verilate(స్థిరరాశి string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::కూడు_Src_Depend(v3Global.opt.buildDepBin());
    ఒకవేళ (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        తిరిగి అబద్ధం;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    ఒకవేళ (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").ఖాళీలు()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(కొత్త V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir(), v3Global.opt.ఉపసర్గ() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir(), v3Global.opt.ఉపసర్గ() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir(), v3Global.opt.ఉపసర్గ() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    ఒకవేళ (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_సంఖ్య::selfTest();
        VString::selfTest();
        VHashSha512::selfTest();
        VSpellCheck::selfTest();
        V3_లేఖాచిత్రం::selfTest();
        V3_Exec_లేఖాచిత్రం::selfTest();
        V3PreShell::selfTest();
        V3Broken::selfTest();
        V3Control::selfTest();
        V3_Hash_పట్టిక_Internals::selfTest();
        V3ThreadPool::selfTest();
        UINFO(2, "selfTest done");
    }

    // Read first filename
    v3Global.readFiles();
    v3Global.removeStd();

    // Link, etc, if needed
    ఒకవేళ (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_వృక్షం("final", 990, dump_వృక్షం_Either_Level() >= 3);
    ఒకవేళ (v3Global.opt.jsonOnly() || dump_వృక్షం_Json_Level()) {
        స్థిరరాశి string filename
            = (v3Global.opt.jsonOnlyMetaOutput().ఖాళీలు()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    ఒకవేళ (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::లిఖ(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    ఒకవేళ (స్థిరరాశి V3_Hier_లేఖాచిత్రం* స్థిరరాశి hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.క్రమానుగత(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hier_బిడ్డ(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hier_ఖండాలు().ఖాళీలు(), "hierarchical-block must not be set");
        ఒకవేళ (v3Global.opt.gmake()) {
            hierGraphp->write_Command_వాదనలు_Files(అబద్ధం);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        ఒకవేళ (v3Global.opt.makeJson()) {
            hierGraphp->write_Command_వాదనలు_Files(నిజం);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    ఒకవేళ (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ();
        filename += v3Global.opt.hier_శిఖరం() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    ఒకవేళ (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ()
                                 + "__idmap.xml");
    }

    ఒకవేళ ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    ఒకవేళ (v3Global.opt.hier_శిఖరం()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir());
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", నిజం);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    తిరిగి నిజం;
}

స్థిర string buildMakeCmd(స్థిరరాశి string& makefile, స్థిరరాశి string& లక్ష్యం) {
    స్థిరరాశి VStringList& makeFlags = v3Global.opt.makeFlags();
    స్థిరరాశి పూర్ణాంకం jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    ఒకవేళ (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    ఒకవేళ (v3Global.opt.getenvMAKEFLAGS().కనుగొను("-jobserver-auth") == string::npos) {
        ఒకవేళ (jobs > 0) cmd << " -j " << jobs;
    }
    కొరకు (స్థిరరాశి string& flag : makeFlags) cmd << ' ' << flag;
    ఒకవేళ (!లక్ష్యం.ఖాళీలు()) cmd << ' ' << లక్ష్యం;

    తిరిగి cmd.str();
}

స్థిర శూన్య release_Netlist_జ్ఞాపకం() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::release_జ్ఞాపకం();
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("released");
}

స్థిర శూన్య exec_Build_పని() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    release_Netlist_జ్ఞాపకం();
    స్థిరరాశి VlOs::Delta_Wall_సమయం build_Wall_సమయం{నిజం};
    UINFO(1, "Start Build");

    స్థిరరాశి string cmdStr = buildMakeCmd(v3Global.opt.ఉపసర్గ() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir());
    స్థిరరాశి పూర్ణాంకం exit_సంకేతం = V3Os::system(cmdStr);
    V3Stats::కూడు_Stat_Perf(V3Stats::STAT_WALLTIME_BUILD, build_Wall_సమయం.delta_సమయం());

    ఒకవేళ (exit_సంకేతం != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_సంకేతం << std::పంక్తి_ముగింపు);
        v3Global.vlExit(exit_సంకేతం);
    }
}

స్థిర శూన్య execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    release_Netlist_జ్ఞాపకం();
    స్థిరరాశి string makefile = v3Global.opt.ఉపసర్గ() + "_hier.mk ";
    స్థిరరాశి string లక్ష్యం = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    స్థిరరాశి string cmdStr = buildMakeCmd(makefile, లక్ష్యం);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir());
    స్థిరరాశి పూర్ణాంకం exit_సంకేతం = V3Os::system(cmdStr);
    ఒకవేళ (exit_సంకేతం != 0) {
        v3error(cmdStr << " exited with " << exit_సంకేతం << std::పంక్తి_ముగింపు);
        v3Global.vlExit(exit_సంకేతం);
    }
}

//######################################################################

పూర్ణాంకం ప్రధాన(పూర్ణాంకం argc, అక్షరం** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    స్థిరరాశి VlOs::Delta_Wall_సమయం wall_సమయం_మొత్తం{నిజం};
    స్థిరరాశి VlOs::Delta_Cpu_సమయం cpu_సమయం_మొత్తం{నిజం};

    సమయం_t randseed;
    సమయం_2(&randseed);
    srand(static_cast<పూర్ణాంకం>(randseed));

    స్థిరరాశి string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->సమయం_Init();

    V3Error::abortIfErrors();

    తార్కిక didVerilate = అబద్ధం;
    ఒకవేళ (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } లేకపోతే {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    ఒకవేళ (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } లేకపోతే ఒకవేళ (v3Global.opt.build()) {
        exec_Build_పని();
    }

    ఒకవేళ (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(నిజం);

    // Explicitly release resources
    v3Global.shutdown();

    ఒకవేళ (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::కూడు_Stat_Perf(V3Stats::STAT_CPUTIME, cpu_సమయం_మొత్తం.delta_సమయం());
        V3Stats::కూడు_Stat_Perf(V3Stats::STAT_WALLTIME, wall_సమయం_మొత్తం.delta_సమయం());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
