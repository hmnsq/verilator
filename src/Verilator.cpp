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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_CONTROL_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Active.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ActiveTop.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Assert.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AssertNfa.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AssertPre.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstPatterns.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Begin.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3BitBlast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Branch.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Broken.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3CCtors.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3CUse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Case.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Class.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Clean.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Clock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Combine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Common.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Coverage.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3CoverageJoin.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Covergroup.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dead.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Decompose.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Delayed.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Depth.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DepthBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Descope.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgOptimizer.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DiagSarif.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCMain.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitMk.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitMkJson.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Expand.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Force.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Fork.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FsmDetect.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FuncOpt.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Gate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inline.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3InlineCFuncs.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inst.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Interface.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LibMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Life.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LifePost.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LiftExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDot.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDotIfaceCapture.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkInc.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkJump.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLValue.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLevel.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkParse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkResolve.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkWith.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Localize.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MergeCond.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Name.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Param.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreShell.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Premit.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ProtectLib.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3RandSequence.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Randomize.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Reloop.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Reorder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sampled.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Scope.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Slice.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Split.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Subst.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Table.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ThreadPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Timing.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Trace.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3TraceDecl.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Tristate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Udp.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Undriven.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unknown.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unroll.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3VariableOrder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Waiver.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3WidthCommit.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <ctime>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

ਸਥਿਰ ਖਾਲੀ reportStatsIfEnabled() {
    ਜੇ (v3Global.opt.stats()) {
        File_ਸਤਰ::stats();
        V3Stats::statsReport();
    }
}

ਸਥਿਰ ਖਾਲੀ emitJson() VL_MT_DISABLED {
    ਅਚਲ string filename
        = (v3Global.opt.jsonOnlyOutput().ਖਾਲੀ_ਥਾਵਾਂ()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dump_ਰੁੱਖ_Json_File(filename);
}

ਸਥਿਰ ਖਾਲੀ emitSerialized() VL_MT_DISABLED {
    ਜੇ (v3Global.opt.jsonOnly()) emitJson();
}

ਸਥਿਰ ਖਾਲੀ process() {
    {
        ਅਚਲ VlOs::Delta_Wall_ਸਮਾਂ elab_Wall_ਸਮਾਂ{ਸੱਚ};

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::mod_ਛਾਂਟੋ_By_Level();
        V3Error::abortIfErrors();
        ਜੇ (v3Global.opt.debugExitParse()) {
            ਬਾਹਰ << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.check_ਰੁੱਖ();  // Force a check, as link is most likely place for problems
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

        ਜੇ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

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

        v3Global.check_ਰੁੱਖ();
        ਜੇ (v3Global.has_ਸਾਰਣੀ()) V3Udp::udpResolve(v3Global.rootp());

        // Create a hierarchical Verilation plan
        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && v3Global.opt.ਦਰਜਾਵਾਰ() && !v3Global.opt.hier_ਬੱਚਾ()) {
            V3_ਦਰਜਾਵਾਰ::create_ਗ੍ਰਾਫ(v3Global.rootp());
            // If a plan is created, further analysis is not necessary.
            // The actual Verilation will be done based on this plan.
            ਜੇ (v3Global.hierGraphp()) ਵਾਪਸ;
        }

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3_ਚੌੜਾਈ::ਚੌੜਾਈ(v3Global.rootp());

        V3Error::abortIfErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3_ਚੌੜਾਈ_Commit::ਚੌੜਾਈ_Commit(v3Global.rootp());
        v3Global.assertDTypesResolved(ਸੱਚ);
        v3Global.ਚੌੜਾਈ_Min_Usage(VWidthMinUsage::MATCHES_ਚੌੜਾਈ);

        // End of elaboration
        V3Stats::ਜੋੜੋ_Stat_Perf(V3Stats::STAT_WALLTIME_ELAB, elab_Wall_ਸਮਾਂ.delta_ਸਮਾਂ());
        ਅਚਲ VlOs::Delta_Wall_ਸਮਾਂ cvt_Wall_ਸਮਾਂ{ਸੱਚ};
        ਜੇ (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            ਜੇ (v3Global.opt.serializeOnly()) emitSerialized();
            ਬਾਹਰ << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        ਜੇ (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrap_ਸਿਖਰ(v3Global.rootp());
        } ਨਹੀਂਤਾਂ {
            V3LinkLevel::non_Wrap_ਸਿਖਰ(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            ਜੇ (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        ਜੇ (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        ਜੇ (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        ਜੇ (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constify_All_ਜੀਵਤ(v3Global.rootp());

        // Signal based lint checks, no change to structures
        // Must be before first constification pass drops dead code
        V3Undriven::undrivenAll(v3Global.rootp());

        // Assertion insertion
        //    After we've added block coverage, but before other nasty transforms
        V3AssertCommon::collectDefaultDisable(v3Global.rootp());
        V3AssertCommon::ਹੇਠਲਾ_Sequence_Events(v3Global.rootp());
        V3AssertNfa::assertNfaAll(v3Global.rootp());
        // V3AssertProp removed: NFA subsumes multi-cycle property lowering.
        // Unsupported constructs fall through to V3AssertPre.
        V3AssertPre::assertPreAll(v3Global.rootp());
        //
        V3Assert::assertAll(v3Global.rootp());

        // Propagate constants into expressions
        ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        ਜੇ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner
            v3Global.staticsLifted(ਸੱਚ);  // All static variables now live under a NodeModule

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3_Lift_ਵਿਅੰਜਕ::lift_ਵਿਅੰਜਕ_All(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(ਸੱਚ);
        }

        // Interface references feed trace file aliases and VPI name resolution
        ਜੇ (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.check_ਰੁੱਖ();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.check_ਰੁੱਖ();

        V3Error::abortIfErrors();

        // --FLATTENING---------------

        ਜੇ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            ਜੇ (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            ਜੇ (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        ਜੇ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ਰੁੱਖ();
        }

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        ਜੇ (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3_ਟੁਕੜਾ::ਟੁਕੜਾ_All(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3_ਸਰਗਰਮ::ਸਰਗਰਮ_All(v3Global.rootp());

            // Decompose arrays and structs into their elements and members
            ਜੇ (v3Global.opt.fDecompose()) V3Decompose::decomposeAll(v3Global.rootp());

            // Split packed variables into bit ranges
            ਜੇ (v3Global.opt.fBitblast()) V3_ਦੋਆਧਾਰੀ_ਅੰਕ_Blast::ਦੋਆਧਾਰੀ_ਅੰਕ_Blast_All(v3Global.rootp());

            ਜੇ (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Make large low-fanin logic blocks into lookup tables
            // This should probably be done much later, once we have common logic elimination.
            ਜੇ (!v3Global.opt.lintOnly() && v3Global.opt.f_ਸਾਰਣੀ()) {
                V3_ਸਾਰਣੀ::ਸਾਰਣੀ_All(v3Global.rootp());
            }

            // Cleanup
            ਜੇ (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.check_ਰੁੱਖ();

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            ਜੇ (v3Global.opt.f_ਵੰਡੋ()) V3_ਵੰਡੋ::ਵੰਡੋ_All(v3Global.rootp());

            // Create tracing sample points, before we start eliminating signals
            ਜੇ (v3Global.opt.trace()) V3TraceDecl::traceDeclAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // After V3TraceDecl so we don't trace additional signals inserted to implement
            // forcing.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::force_And_ਸੌਂਪੋ_All(v3Global.rootp());

            // DFG optimization
            ਜੇ (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            ਜੇ (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } ਨਹੀਂਤਾਂ {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            ਜੇ (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            ਜੇ (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
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
            V3_ਸਰਗਰਮ_ਸਿਖਰ::ਸਰਗਰਮ_ਸਿਖਰ_All(v3Global.rootp());

            // Remove SAMPLED
            ਜੇ (v3Global.hasSampled()) V3Sampled::sampledAll(v3Global.rootp());

            ਜੇ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "PreOrder");

            // Schedule the logic
            V3Sched::ਸਮਾਂ_ਸਾਰਣੀ(v3Global.rootp());
            V3Sched::transformForks(v3Global.rootp());

            // Post scheduling transformations - TODO: this should at least be renamed
            V3Clock::clockAll(v3Global.rootp());

            // Cleanup any dly vars or other temps that are simple assignments
            // Life must be done before Subst, as it assumes each CFunc under
            // _eval is called only once.
            ਜੇ (v3Global.opt.fLife()) {
                V3Const::constifyAll(v3Global.rootp());
                V3Life::lifeAll(v3Global.rootp());
            }

            ਜੇ (v3Global.opt.fLifePost()) V3LifePost::lifepostAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Create tracing logic, since we ripped out some signals the user might want to trace
            // Note past this point, we presume traced variables won't move between CFuncs
            // (It's OK if untraced temporaries move around, or vars
            // "effectively" activate the same way.)
            ਜੇ (v3Global.opt.trace()) V3Trace::traceAll(v3Global.rootp());

            ਜੇ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Scoped");
        }

        // --MODULE OPTIMIZATIONS--------------

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Split deep blocks to appease MSVC++.  Must be before Localize.
            ਜੇ (!v3Global.opt.lintOnly() && v3Global.opt.comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ()) {
                V3_ਡੂੰਘਾਈ_ਖੰਡ::ਡੂੰਘਾਈ_ਖੰਡ_All(v3Global.rootp());
            }

            // Up until this point, all references must be scoped
            v3Global.assertScoped(ਝੂਠ);

            // Move variables from modules to function local variables where possible
            ਜੇ (v3Global.opt.fLocalize()) V3Localize::localizeAll(v3Global.rootp());

            // Remove remaining scopes; make varrefs/funccalls relative to current module
            V3Descope::descopeAll(v3Global.rootp());

            // Icache packing; combine common code in each module's functions into subroutines
            ਜੇ (v3Global.opt.fCombine()) V3Combine::combineAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();

        // --GENERATION------------------

        ਜੇ (!v3Global.opt.serializeOnly()) {
            // Generic optimizations on a per-function basis
            ਜੇ (v3Global.opt.fFunc()) V3FuncOpt::funcOptAll(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());

            ਜੇ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "prec");

            // Here down, widthMin() is the Verilog width, and width() is the C++ width
            // Bits between widthMin() and width() are irrelevant, but may be non-zero.
            v3Global.ਚੌੜਾਈ_Min_Usage(VWidthMinUsage::VERILOG_ਚੌੜਾਈ);

            // Make all expressions 32, 64, or 32*N bits
            // Variables and selects-of-variables remain verilog-width
            V3Clean::cleanAll(v3Global.rootp());

            // Move wide constants to BLOCK temps / ConstPool.
            V3Premit::premitAll(v3Global.rootp());
        }

        // Expand macros and wide operators into C++ primitives
        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && v3Global.opt.fExpand()) {
            V3Expand::expandAll(v3Global.rootp());
        }

        // Propagate constants across WORDSEL arrayed temporaries
        ਜੇ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubst()) {
            // Constant folding of expanded stuff
            V3Const::constifyCpp(v3Global.rootp());
            V3Subst::substituteAll(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.serializeOnly() && v3Global.opt.fSubstConst()) {
            // Constant folding of substitutions
            V3Const::constifyCpp(v3Global.rootp());
            V3Dead::deadifyAll(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ਜੇ (v3Global.opt.f_ਮਿਲਾਓ_Cond()) {
                // Merge conditionals
                V3_ਮਿਲਾਓ_Cond::ਮਿਲਾਓ_All(v3Global.rootp());
            }

            ਜੇ (v3Global.opt.fReloop()) {
                // Reform loops to reduce code size
                // Must be after all Sel/array index based optimizations
                V3Reloop::reloopAll(v3Global.rootp());
            }
        }

        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {
            ਜੇ (v3Global.opt.fInlineCFuncs()) {
                // Inline small CFuncs to reduce function call overhead
                V3InlineCFuncs::inlineAll(v3Global.rootp());
            }

            // Fix very deep expressions
            // Mark evaluation functions as member functions, if needed.
            V3_ਡੂੰਘਾਈ::ਡੂੰਘਾਈ_All(v3Global.rootp());

            // Branch prediction
            V3Branch::branchAll(v3Global.rootp());

            // Add C casts when longs need to become long-long and vice-versa
            // Note depth may insert something needing a cast, so this must be last.
            V3Cast::castAll(v3Global.rootp());
        }

        V3Error::abortIfErrors();
        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()) {  //
            V3CCtors::cctorsAll();
        }

        ਜੇ (!v3Global.opt.serializeOnly() && v3Global.opt.mtasks()) {
            // Implement the ExecGraphs by packing mtasks to thread.
            // This should happen as late as possible (after all optimizations)
            // as it relies on cost estimates.
            V3_Exec_ਗ੍ਰਾਫ::implement(v3Global.rootp());
        }

        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Add common methods/etc to modules
            V3Common::commonAll();

            // Order variables
            V3_Variable_ਕ੍ਰਮ::ਕ੍ਰਮ_All(v3Global.rootp());

            // Create AstCUse to determine what class forward declarations/#includes needed in C
            V3CUse::cUseAll();

            // Evaluate cost of a current hierarchical block
            ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
                v3Global.current_Hier_ਖੰਡ_ਲਾਗਤ(V3Control::get_Current_Hier_ਖੰਡ_ਲਾਗਤ());
            }
        }

        ਜੇ (v3Global.opt.dumpAstPatterns()) V3AstPatterns::dumpAll(v3Global.rootp(), "emit");

        // Output the text
        ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly()
            && !v3Global.opt.dpiHdrOnly()) {
            // Make parent module pointers available for all Emit stages below
            ਅਚਲ EmitCParentModule emitCParentModule;
            // emitcInlines is first, as it may set needHInlines which other emitters read
            V3EmitC::emitcInlines();
            V3EmitC::emitcSyms();
            V3EmitC::emitcConstPool();
            V3EmitC::emitc_Exec_ਗ੍ਰਾਫ();
            V3EmitC::emitcModel();
            V3EmitC::emitcPch();
            V3EmitC::emitc_ਸਿਰਲੇਖਾਂ();
        } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.dpiHdrOnly()) {
            V3EmitC::emitcSyms(ਸੱਚ);
        }

        // End of conversion
        V3Stats::ਜੋੜੋ_Stat_Perf(V3Stats::STAT_WALLTIME_CVT, cvt_Wall_ਸਮਾਂ.delta_ਸਮਾਂ());
    }
    ਜੇ (!v3Global.opt.serializeOnly()
        && !v3Global.opt.dpiHdrOnly()) {  // Unfortunately we have some lint checks in emitcImp.
        V3EmitC::emitcImp();
    }
    ਜੇ (v3Global.opt.serializeOnly()) {
        emitSerialized();
    } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.debugCheck() && !v3Global.opt.lintOnly()
               && !v3Global.opt.dpiHdrOnly()) {
        // Check JSON when debugging to make sure no missing node types
        emitJson();
    }

    // Output DPI protected library files
    ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (v3Global.rootp()->delaySchedulerp()) {
            v3warn(E_UNSUPPORTED, "Unsupported: --lib-create with --timing and delays");
        }
        V3ProtectLib::protect();
        V3EmitV::emitvFiles();
        V3EmitC::emitcFiles();
    }

    ਜੇ (!v3Global.opt.lintOnly() && !v3Global.opt.serializeOnly() && !v3Global.opt.dpiHdrOnly()) {
        ਜੇ (v3Global.opt.ਮੁੱਖ()) V3EmitCMain::emit();

        // V3EmitMk/V3EmitMkJson must be after all other emitters,
        // as they and below code visits AstCFiles added earlier
        ਮਾਪ_t src_f_cnt = 0;
        ਲਈ (Ast_ਗੰਢ* nodep = v3Global.rootp()->filesp(); nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ AstCFile* cfilep = VN_CAST(nodep, CFile))
                src_f_cnt += cfilep->ਸਰੋਤ() ? 1 : 0;
        }
        ਜੇ (src_f_cnt >= V3EmitMk::PARALLEL_FILE_CNT_ਹੱਦ) v3Global.useParallelBuild(ਸੱਚ);
        ਜੇ (v3Global.opt.makeJson()) V3EmitMkJson::emit();
        ਜੇ (v3Global.opt.gmake()) V3EmitMk::emitmk();
    }

    // Final statistics
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("emit");
}

ਸਥਿਰ ਤਾਰਕਿਕ verilate(ਅਚਲ string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::ਜੋੜੋ_Src_Depend(v3Global.opt.buildDepBin());
    ਜੇ (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        ਵਾਪਸ ਝੂਠ;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    ਜੇ (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").ਖਾਲੀ_ਥਾਵਾਂ()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(ਨਵਾਂ V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir(), v3Global.opt.ਅਗੇਤਰ() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir(), v3Global.opt.ਅਗੇਤਰ() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir(), v3Global.opt.ਅਗੇਤਰ() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    ਜੇ (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3_ਸੰਖਿਆ::selfTest();
        VString::selfTest();
        VHashSha512::selfTest();
        VSpellCheck::selfTest();
        V3_ਗ੍ਰਾਫ::selfTest();
        V3_Exec_ਗ੍ਰਾਫ::selfTest();
        V3PreShell::selfTest();
        V3Broken::selfTest();
        V3Control::selfTest();
        V3_Hash_ਸਾਰਣੀ_Internals::selfTest();
        V3ThreadPool::selfTest();
        UINFO(2, "selfTest done");
    }

    // Read first filename
    v3Global.readFiles();
    v3Global.removeStd();

    // Link, etc, if needed
    ਜੇ (!v3Global.opt.preprocOnly()) {  //
        process();
    }

    // Final steps
    V3Global::dump_Check_Global_ਰੁੱਖ("final", 990, dump_ਰੁੱਖ_Either_Level() >= 3);
    ਜੇ (v3Global.opt.jsonOnly() || dump_ਰੁੱਖ_Json_Level()) {
        ਅਚਲ string filename
            = (v3Global.opt.jsonOnlyMetaOutput().ਖਾਲੀ_ਥਾਵਾਂ()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    ਜੇ (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::ਲਿਖ(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    ਜੇ (ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* ਅਚਲ hierGraphp
        = v3Global.hierGraphp()) {  // This run is for just write a makefile
        UASSERT(v3Global.opt.ਦਰਜਾਵਾਰ(), "hierarchical must be set");
        UASSERT(!v3Global.opt.hier_ਬੱਚਾ(), "This must not be a hierarchical-child run");
        UASSERT(v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ(), "hierarchical-block must not be set");
        ਜੇ (v3Global.opt.gmake()) {
            hierGraphp->write_Command_ਦਲੀਲਾਂ_Files(ਝੂਠ);
            V3EmitMk::emitHierVerilation(hierGraphp);
        }
        ਜੇ (v3Global.opt.makeJson()) {
            hierGraphp->write_Command_ਦਲੀਲਾਂ_Files(ਸੱਚ);
            V3EmitMkJson::emit();
        }
        hierGraphp->writeParametersFiles();
    }
    ਜੇ (v3Global.opt.makeDepend().isTrue()) {
        string filename = v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ();
        filename += v3Global.opt.hier_ਸਿਖਰ() ? "__hierVer.d" : "__ver.d";
        V3File::writeDepend(filename);
    }
    ਜੇ (v3Global.opt.protectIds()) {
        VIdProtect::writeMapFile(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                                 + "__idmap.xml");
    }

    ਜੇ ((v3Global.opt.skipIdentical().isTrue() || v3Global.opt.makeDepend().isTrue())
        && !V3Error::isErrorOrWarn()) {
        V3File::writeTimes(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                               + "__verFiles.dat",
                           argString);
    }

    V3Os::filesystemFlushBuildDir(v3Global.opt.makeDir());
    ਜੇ (v3Global.opt.hier_ਸਿਖਰ()) V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir());
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteAll");
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "WroteFast", ਸੱਚ);

    // Final writing shouldn't throw warnings, but...
    V3Error::abortIfWarnings();

    ਵਾਪਸ ਸੱਚ;
}

ਸਥਿਰ string buildMakeCmd(ਅਚਲ string& makefile, ਅਚਲ string& ਨਿਸ਼ਾਨਾ) {
    ਅਚਲ VStringList& makeFlags = v3Global.opt.makeFlags();
    ਅਚਲ ਪੂਰਨ jobs = v3Global.opt.buildJobs();
    UASSERT(jobs >= 0, "-j option parser in V3Options.cpp filters out negative value");

    std::ostringstream cmd;
    cmd << v3Global.opt.getenvMAKE();
    cmd << " -C " << v3Global.opt.makeDir();
    cmd << " -f " << makefile;
    // Unless using make's jobserver, do a -j
    ਜੇ (v3Global.opt.quietBuild()) cmd << " -s --no-print-directory";
    ਜੇ (v3Global.opt.getenvMAKEFLAGS().ਲੱਭੋ("-jobserver-auth") == string::npos) {
        ਜੇ (jobs > 0) cmd << " -j " << jobs;
    }
    ਲਈ (ਅਚਲ string& flag : makeFlags) cmd << ' ' << flag;
    ਜੇ (!ਨਿਸ਼ਾਨਾ.ਖਾਲੀ_ਥਾਵਾਂ()) cmd << ' ' << ਨਿਸ਼ਾਨਾ;

    ਵਾਪਸ cmd.str();
}

ਸਥਿਰ ਖਾਲੀ release_Netlist_ਸਮ੍ਰਿਤੀ() {
    UINFO(1, "Releasing netlist memory");
    v3Global.rootp()->deleteContents();
    V3Os::release_ਸਮ੍ਰਿਤੀ();
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("released");
}

ਸਥਿਰ ਖਾਲੀ exec_Build_ਕੰਮ() {
    UASSERT(v3Global.opt.build(), "--build is not specified.");
    UASSERT(v3Global.opt.gmake(), "--build requires GNU Make.");
    UASSERT(!v3Global.opt.makeJson(), "--build cannot use json build.");
    release_Netlist_ਸਮ੍ਰਿਤੀ();
    ਅਚਲ VlOs::Delta_Wall_ਸਮਾਂ build_Wall_ਸਮਾਂ{ਸੱਚ};
    UINFO(1, "Start Build");

    ਅਚਲ string cmdStr = buildMakeCmd(v3Global.opt.ਅਗੇਤਰ() + ".mk", "");
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir());
    ਅਚਲ ਪੂਰਨ exit_ਕੋਡ = V3Os::system(cmdStr);
    V3Stats::ਜੋੜੋ_Stat_Perf(V3Stats::STAT_WALLTIME_BUILD, build_Wall_ਸਮਾਂ.delta_ਸਮਾਂ());

    ਜੇ (exit_ਕੋਡ != 0) {
        v3error("'" << cmdStr << "' exited with " << exit_ਕੋਡ << std::ਸਤਰ_ਅੰਤ);
        v3Global.vlExit(exit_ਕੋਡ);
    }
}

ਸਥਿਰ ਖਾਲੀ execHierVerilation() {
    UASSERT(v3Global.hierGraphp(), "must be called only when plan exists");
    release_Netlist_ਸਮ੍ਰਿਤੀ();
    ਅਚਲ string makefile = v3Global.opt.ਅਗੇਤਰ() + "_hier.mk ";
    ਅਚਲ string ਨਿਸ਼ਾਨਾ = v3Global.opt.build() ? " hier_build" : " hier_verilation";
    ਅਚਲ string cmdStr = buildMakeCmd(makefile, ਨਿਸ਼ਾਨਾ);
    V3Os::filesystemFlushBuildDir(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir());
    ਅਚਲ ਪੂਰਨ exit_ਕੋਡ = V3Os::system(cmdStr);
    ਜੇ (exit_ਕੋਡ != 0) {
        v3error(cmdStr << " exited with " << exit_ਕੋਡ << std::ਸਤਰ_ਅੰਤ);
        v3Global.vlExit(exit_ਕੋਡ);
    }
}

//######################################################################

ਪੂਰਨ ਮੁੱਖ(ਪੂਰਨ argc, ਅੱਖਰ** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    ਅਚਲ VlOs::Delta_Wall_ਸਮਾਂ wall_ਸਮਾਂ_ਕੁੱਲ{ਸੱਚ};
    ਅਚਲ VlOs::Delta_Cpu_ਸਮਾਂ cpu_ਸਮਾਂ_ਕੁੱਲ{ਸੱਚ};

    ਸਮਾਂ_t randseed;
    ਸਮਾਂ_2(&randseed);
    srand(static_cast<ਪੂਰਨ>(randseed));

    ਅਚਲ string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->ਸਮਾਂ_Init();

    V3Error::abortIfErrors();

    ਤਾਰਕਿਕ didVerilate = ਝੂਠ;
    ਜੇ (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } ਨਹੀਂਤਾਂ {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    ਜੇ (v3Global.hierGraphp() && v3Global.opt.gmake()) {
        execHierVerilation();  // execHierVerilation() takes care of --build too
    } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.build()) {
        exec_Build_ਕੰਮ();
    }

    ਜੇ (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(ਸੱਚ);

    // Explicitly release resources
    v3Global.shutdown();

    ਜੇ (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::ਜੋੜੋ_Stat_Perf(V3Stats::STAT_CPUTIME, cpu_ਸਮਾਂ_ਕੁੱਲ.delta_ਸਮਾਂ());
        V3Stats::ਜੋੜੋ_Stat_Perf(V3Stats::STAT_WALLTIME, wall_ਸਮਾਂ_ਕੁੱਲ.delta_ਸਮਾਂ());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
