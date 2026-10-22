// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Parse module/signal name references
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
// LinkParse TRANSFORMATIONS:
//      Top-down traversal
//          Move some attributes around
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkParse.h"

#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

తరగతి LinkParseVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // TYPES
    ఉపయోగించి ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_సమితి<File_పంక్తి*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = శూన్యసూచిక;  // Variable we're under
    Ast_కణుపు_Module* m_విలువ_Modp = శూన్యసూచిక;
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    తార్కిక m_లోపల_Interface = అబద్ధం;  // True when inside interface declaration
    Ast_కణుపు_Procedure* m_procedurep = శూన్యసూచిక;  // Current procedure
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current task
    AstRSProd* m_rsProdp = శూన్యసూచిక;  // Current randsequence production
    Ast_కణుపు_ఖండం* m_blockp = శూన్యసూచిక;  // Current AstNodeBlock
    Ast_కణుపు_Stmt* m_ఖండం_కూడు_Automatic_Stmtp = శూన్యసూచిక;  // Initial statements to add to block
    Ast_కణుపు_Stmt* m_ఖండం_కూడు_Static_Stmtp = శూన్యసూచిక;  // Initial statements to add to block
    Ast_కణుపు_DType* m_dtypep = శూన్యసూచిక;  // Current data type
    Ast_కణుపు_సమాసం* m_default_లోపల_Skewp = శూన్యసూచిక;  // Current default input skew
    Ast_కణుపు_సమాసం* m_default_బహిర్గతం_Skewp = శూన్యసూచిక;  // Current default output skew
    AstCoverpoint* m_coverpointp = శూన్యసూచిక;  // Current coverpoint
    AstCoverCross* m_crossp = శూన్యసూచిక;  // Current cross
    std::map<string, స్థిరరాశి AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    పూర్ణాంకం m_anonUdpId = 0;  // Counter for anonymous UDP instances
    పూర్ణాంకం m_coverpoint_అంకె = 0;  // Counter for unnamed coverpoints within current covergroup
    పూర్ణాంకం m_genblkAbove = 0;  // Begin block number of if/case/for above
    పూర్ణాంకం m_genblk_అంకె = 0;  // Begin block number, 0=none seen
    పూర్ణాంకం m_begin_లోతు = 0;  // How many begin blocks above current node within current AstNodeModule
    పూర్ణాంకం m_rand_Sequence_అంకె = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    తార్కిక m_lifetimeAllowed = అబద్ధం;  // True to allow lifetime settings
    తార్కిక m_hasTimingControl = అబద్ధం;  // If current task has timing control
    తార్కిక m_moduleWithGenericIface = అబద్ధం;  // If current module contains generic interface
    std::సమితి<AstVar*> m_పోర్టు_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    శూన్య cleanFileline(Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->user2_సమితి_Once()) తిరిగి;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        ఒకవేళ (m_filelines.కనుగొను(nodep->fileline()) != m_filelines.చివర()) {
            nodep->fileline(కొత్త File_పంక్తి{nodep->fileline()});
        }
        m_filelines.చొప్పించు(nodep->fileline());
    }

    string nameFromTypedef(Ast_కణుపు* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        ఒకవేళ (స్థిరరాశి AstTypedef* స్థిరరాశి typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            ఒకవేళ (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } లేకపోతే ఒకవేళ (m_modp) {
                above = m_modp->name() + ".";
            }
            తిరిగి above + typedefp->name();
        }
        తిరిగి "";
    }

    శూన్య సందర్శించు_Iterate_కణుపు_DType(Ast_కణుపు_DType* nodep) {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    తార్కిక nestedIfBegin(Ast_Gen_ఖండం* nodep) {  // Point at begin inside the GenIf/GenCaseItem
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        //    1: GENIF
        // -> 1:3: GENBLOCK [IMPLIED]  // nodep passed to this function
        //    1:3:1: GENIF
        //    1:3:1:2: GENBLOCK genblk1 [IMPLIED]
        // Likewise for a generate case item holding only a generate if
        //    1: GENCASEITEM
        // -> 1:2: GENBLOCK [IMPLIED]  // nodep passed to this function
        //    1:2:1: GENIF
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        తిరిగి (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, Gen_Case_అంశం))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    శూన్య checkIndent(Ast_కణుపు* nodep, Ast_కణుపు* childp) {
        // Try very hard to avoid false positives
        Ast_కణుపు* nextp = nodep->nextp();
        ఒకవేళ (!childp) తిరిగి;
        ఒకవేళ (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        ఒకవేళ (!nextp) తిరిగి;
        ఒకవేళ (VN_IS(childp, Begin) || VN_IS(childp, Gen_ఖండం)) తిరిగి;
        File_పంక్తి* స్థిరరాశి కణుపు_Flp = nodep->fileline();
        File_పంక్తి* స్థిరరాశి బిడ్డ_Flp = childp->fileline();
        File_పంక్తి* స్థిరరాశి తదుపరి_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        ఒకవేళ (కణుపు_Flp->filenameno() != బిడ్డ_Flp->filenameno()) తిరిగి;
        ఒకవేళ (కణుపు_Flp->filenameno() != తదుపరి_Flp->filenameno()) తిరిగి;
        ఒకవేళ (కణుపు_Flp->చివరి_Lineno() >= బిడ్డ_Flp->మొదటి_Lineno()) తిరిగి;
        ఒకవేళ (బిడ్డ_Flp->చివరి_Lineno() >= తదుపరి_Flp->మొదటి_Lineno()) తిరిగి;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        ఒకవేళ (!(కణుపు_Flp->మొదటి_స్తంభం() < బిడ్డ_Flp->మొదటి_స్తంభం()
              && తదుపరి_Flp->మొదటి_స్తంభం() >= బిడ్డ_Flp->మొదటి_స్తంభం()))
            తిరిగి;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        స్థిరరాశి std::string కణుపు_ఉపసర్గ = కణుపు_Flp->ఆధారం_ఉపసర్గ(కణుపు_Flp->మొదటి_స్తంభం());
        స్థిరరాశి std::string బిడ్డ_ఉపసర్గ = బిడ్డ_Flp->ఆధారం_ఉపసర్గ(కణుపు_Flp->మొదటి_స్తంభం());
        స్థిరరాశి std::string తదుపరి_ఉపసర్గ = తదుపరి_Flp->ఆధారం_ఉపసర్గ(కణుపు_Flp->మొదటి_స్తంభం());
        ఒకవేళ (బిడ్డ_ఉపసర్గ != కణుపు_ఉపసర్గ) తిరిగి;
        ఒకవేళ (తదుపరి_ఉపసర్గ != బిడ్డ_ఉపసర్గ) తిరిగి;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        స్థిరరాశి std::string కణుపు_ఆధారం = కణుపు_Flp->ఆధారం();
        స్థిరరాశి std::string బిడ్డ_ఆధారం = బిడ్డ_Flp->ఆధారం();
        స్థిరరాశి std::string తదుపరి_ఆధారం = తదుపరి_Flp->ఆధారం();
        ఒకవేళ (!(VString::leading_Whitespace_లెక్క(కణుపు_ఆధారం)
                  < VString::leading_Whitespace_లెక్క(బిడ్డ_ఆధారం)
              && VString::leading_Whitespace_లెక్క(తదుపరి_ఆధారం)
                     >= VString::leading_Whitespace_లెక్క(బిడ్డ_ఆధారం)))
            తిరిగి;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    శూన్య collectPorts(Ast_కణుపు* కణుపు_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> పోర్టు_పేర్లు;
        కొరకు (Ast_కణుపు* nodep = కణుపు_Listp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isIO()) పోర్టు_పేర్లు.emplace(varp->name(), varp);
            }
        }
        కొరకు (Ast_కణుపు* nodep = కణుపు_Listp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isIO()) కొనసాగు;
                స్థిరరాశి స్వయంచాలక it = పోర్టు_పేర్లు.కనుగొను(varp->name());
                ఒకవేళ (it == పోర్టు_పేర్లు.చివర()) కొనసాగు;
                AstVar* స్థిరరాశి iop = it->రెండవ;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                ఒకవేళ (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_వెనుక_With_తదుపరి());
                m_పోర్టు_Dups.emplace(varp);
            }
        }
    }

    Ast_కణుపు* getVarsAndUnlink(Ast_కణుపు* stmtsp) {
        Ast_కణుపు* varsp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ఒకవేళ (VN_IS(itemp, Var)) varsp = Ast_కణుపు::కూడు_తదుపరి(varsp, itemp->unlink_Fr_వెనుక());
        }
        తిరిగి varsp;
    }
    Ast_కణుపు_Stmt* get_ఖండం_Adds() {
        Ast_కణుపు_Stmt* addsp = శూన్యసూచిక;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        ఒకవేళ (m_ఖండం_కూడు_Static_Stmtp)
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp,
                                     కొత్త AstInitialStaticStmt{m_ఖండం_కూడు_Static_Stmtp->fileline(),
                                                              m_ఖండం_కూడు_Static_Stmtp});
        ఒకవేళ (m_ఖండం_కూడు_Automatic_Stmtp)
            addsp = Ast_కణుపు::కూడు_తదుపరి(
                addsp, కొత్త AstInitialAutomaticStmt{m_ఖండం_కూడు_Automatic_Stmtp->fileline(),
                                                   m_ఖండం_కూడు_Automatic_Stmtp});
        m_ఖండం_కూడు_Static_Stmtp = శూన్యసూచిక;
        m_ఖండం_కూడు_Automatic_Stmtp = శూన్యసూచిక;
        తిరిగి addsp;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process only once.
        // Mark class methods
        ఒకవేళ (VN_IS(m_modp, Class)) nodep->class_పద్ధతి(నిజం);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_ఖండం_కూడు_Automatic_Stmtp);
        m_ఖండం_కూడు_Automatic_Stmtp = శూన్యసూచిక;
        VL_RESTORER(m_ఖండం_కూడు_Static_Stmtp);
        m_ఖండం_కూడు_Static_Stmtp = శూన్యసూచిక;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = నిజం;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) { తిరిగి nodep->isTimingControl(); });
        ఒకవేళ (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } లేకపోతే {
            ఒకవేళ (nodep->class_పద్ధతి()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } లేకపోతే ఒకవేళ (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        ఒకవేళ (nodep->class_పద్ధతి() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_మరిన్ని() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_పోర్టు_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_కణుపు* stmtsp = శూన్యసూచిక;
        ఒకవేళ (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_వెనుక();
        // Move all Vars to front of function
        stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, get_ఖండం_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    శూన్య సందర్శించు(Ast_కణుపు_DType* nodep) override { సందర్శించు_Iterate_కణుపు_DType(nodep); }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(నిజం);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConstraintForeach* nodep) override {
        ఒకవేళ (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_సమాసం* exprp) { exprp->isSoft(నిజం); });
        }
    }
    శూన్య సందర్శించు(AstEnumDType* nodep) override {
        ఒకవేళ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        సందర్శించు_Iterate_కణుపు_DType(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        ఒకవేళ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        సందర్శించు_Iterate_కణుపు_DType(nodep);
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (nodep->rangep()) {
            ఒకవేళ (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t స్థానభ్రంశం_from_init = 0;
            Ast_Enum_అంశం* addp = శూన్యసూచిక;
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            కొరకు (స్థిరరాశి పూర్ణాంకం i : nodep->rangep()->seq_ఎడమ_గమ్యం_కుడి()) {
                స్థిరరాశి string name = nodep->name() + cvt_గమ్యం_Str(i);
                Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;
                ఒకవేళ (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = కొత్త Ast_కూడు{flp, nodep->valuep()->clone_వృక్షం(నిజం),
                                     కొత్త AstConst{flp, AstConst::Unsized32{}, స్థానభ్రంశం_from_init}};
                }
                addp = Ast_కణుపు::కూడు_తదుపరి(addp, కొత్త Ast_Enum_అంశం{flp, name, శూన్యసూచిక, valuep});
                ++స్థానభ్రంశం_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class);
        ఒకవేళ (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(నిజం);
        }
        ఒకవేళ (nodep->embeddedCovergroup()) {
            AstRefDType* స్థిరరాశి refp = VN_AS(nodep->బిడ్డ_DTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        ఒకవేళ (nodep->valuep()) nodep->hasUserInit(నిజం);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        ఒకవేళ (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            ఒకవేళ (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_మరిన్ని() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_మరిన్ని() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } లేకపోతే {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_మరిన్ని() << "... The initializer value will only be set once\n"
                        << nodep->warn_మరిన్ని()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        ఒకవేళ (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        ఒకవేళ (!nodep->direction().isAny()) {  // Not a port
            ఒకవేళ (nodep->lifetime().isNone()) {
                ఒకవేళ (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } లేకపోతే {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } లేకపోతే ఒకవేళ (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            ఒకవేళ (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } లేకపోతే {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } లేకపోతే ఒకవేళ (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        ఒకవేళ (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            ఒకవేళ (m_begin_లోతు > 0
                || (m_begin_లోతు == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        ఒకవేళ (nodep->isGParam() && m_modp) m_modp->hasGParam(నిజం);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(nodep->బిడ్డ_DTypep(), BasicDType);
        ఒకవేళ (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(నిజం);

        ఒకవేళ (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_సంకేతం::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        ఒకవేళ (nodep->isParam() && m_లోపల_Interface) nodep->isIfaceParam(నిజం);
        ఒకవేళ (AstParseTypeDType* స్థిరరాశి ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_కణుపు* dtypep = nodep->valuep();
            ఒకవేళ (dtypep) {
                dtypep->unlink_Fr_వెనుక();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                ఒకవేళ (Ast_కణుపు_సమాసం* exprp = VN_CAST(dtypep, కణుపు_సమాసం)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    అయితే (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        ఒకవేళ (!rhsDotp) ఆపు;
                        File_పంక్తి* స్థిరరాశి fl = dotp->fileline();
                        స్థిరరాశి తార్కిక colon = dotp->colon();
                        Ast_కణుపు_సమాసం* స్థిరరాశి lhs = VN_AS(dotp->lhsp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                        Ast_కణుపు_సమాసం* స్థిరరాశి rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                        Ast_కణుపు_సమాసం* స్థిరరాశి rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                        File_పంక్తి* స్థిరరాశి rhsFl = rhsDotp->fileline();
                        స్థిరరాశి తార్కిక rhsColon = rhsDotp->colon();
                        AstDot* స్థిరరాశి newLhs = కొత్త AstDot{fl, colon, lhs, rhsLhs};
                        exprp = కొత్త AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_వృక్షం(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } లేకపోతే {
                dtypep = కొత్త AstVoidDType{nodep->fileline()};
            }

            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlag_బిడ్డ_DType{}, కొత్త AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->బిడ్డ_DTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(అబద్ధం), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        ఒకవేళ (v3Global.opt.any_ప్రజా_Flat() && nodep->varType().isVPIAccessible()) {
            ఒకవేళ (v3Global.opt.ప్రజా_Flat_RW()) {
                nodep->sigUserRWPublic(నిజం);
            } లేకపోతే ఒకవేళ (v3Global.opt.ప్రజా_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(నిజం);
            } లేకపోతే ఒకవేళ (m_modp && v3Global.opt.ప్రజా_లోతు()) {
                ఒకవేళ ((m_modp->లోతు() - 1) <= v3Global.opt.ప్రజా_లోతు()) {
                    nodep->sigUserRWPublic(నిజం);
                } లేకపోతే ఒకవేళ (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(నిజం);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        ఒకవేళ (v3Global.opt.trace_లోతు() && m_modp
            && (m_modp->లోతు() - 1) > v3Global.opt.trace_లోతు()) {
            m_modp->modTrace(అబద్ధం);
            nodep->trace(అబద్ధం);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = శూన్యసూచిక;
        // temporaries under an always aren't expected to be blocking
        ఒకవేళ (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::BLKSEQ, నిజం);
        // Compute initial value
        ఒకవేళ (nodep->valuep() || nodep->needsCReset()) {
            File_పంక్తి* స్థిరరాశి fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            స్వయంచాలక createValuep = [&]() -> Ast_కణుపు_సమాసం* {
                ఒకవేళ (nodep->valuep()) {
                    తిరిగి VN_AS(nodep->valuep()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                } లేకపోతే {
                    తిరిగి కొత్త AstCReset{fl, nodep, అబద్ధం};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            ఒకవేళ (m_పోర్టు_Dups.కనుగొను(nodep) != m_పోర్టు_Dups.చివర()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } లేకపోతే ఒకవేళ (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } లేకపోతే ఒకవేళ (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ఆదానం()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                ఒకవేళ (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                }
            } లేకపోతే ఒకవేళ (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(నిజం);
                File_పంక్తి* స్థిరరాశి newfl = కొత్త File_పంక్తి{fl};
                newfl->warnOff(V3_Error_సంకేతం::E_CONSTWRITTEN, నిజం);
                Ast_కేటాయించు* స్థిరరాశి assp
                    = కొత్త Ast_కేటాయించు{newfl, కొత్త AstParseRef{newfl, nodep->name()}, createValuep()};
                ఒకవేళ (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_ఖండం_కూడు_Automatic_Stmtp = Ast_కణుపు::కూడు_తదుపరి(m_ఖండం_కూడు_Automatic_Stmtp, assp);
                } లేకపోతే {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_ఖండం_కూడు_Static_Stmtp = Ast_కణుపు::కూడు_తదుపరి(m_ఖండం_కూడు_Static_Stmtp, assp);
                }
            } లేకపోతే ఒకవేళ (m_విలువ_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                ఒకవేళ (nodep->valuep()) {
                    nodep->noCReset(నిజం);
                    File_పంక్తి* స్థిరరాశి newfl = కొత్త File_పంక్తి{fl};
                    newfl->warnOff(V3_Error_సంకేతం::PROCASSWIRE, నిజం);
                    newfl->warnOff(V3_Error_సంకేతం::E_CONSTWRITTEN, నిజం);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_కేటాయించు* స్థిరరాశి assp = కొత్త Ast_కేటాయించు{
                        newfl, కొత్త AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_కణుపు* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    ఒకవేళ (nodep->lifetime().isAutomatic()) {
                        newInitp = కొత్త AstInitialAutomatic{newfl, assp};
                    } లేకపోతే {
                        newInitp = కొత్త AstInitialStatic{newfl, assp};
                    }
                    nodep->కూడు_తదుపరి_ఇక్కడ(newInitp);
                }
            } లేకపోతే ఒకవేళ (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    స్థిర తార్కిక isNegative(స్థిరరాశి AstConst* constp) {
        తిరిగి constp->isSigned() && constp->అంకె().బిట్టు_Is1(constp->వెడల్పు() - 1);
    }
    శూన్య సందర్శించు(AstRange* nodep) override {
        స్థిర constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        స్థిరరాశి AstConst* స్థిరరాశి leftp = VN_CAST(nodep->leftp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి rightp = VN_CAST(nodep->rightp(), Const);
        ఒకవేళ (leftp && rightp && leftp->వెడల్పు() <= 64 && rightp->వెడల్పు() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            స్థిరరాశి uint64_t ఎడమ = leftp->గమ్యం_UQuad();
            స్థిరరాశి uint64_t కుడి = rightp->గమ్యం_UQuad();
            స్థిరరాశి uint64_t పరిధి = std::max(ఎడమ, కుడి) - std::min(ఎడమ, కుడి);
            ఒకవేళ (పరిధి >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << పరిధి + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        ఒకవేళ (nodep->అంకె().autoExtend() && nodep->fileline()->language() < V3_Lang_సంకేతం::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (nodep->attrType() == VAttrType::DT_ప్రజా) {
            AstTypedef* స్థిరరాశి typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_ప్రజా(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->సమితి_Forceable();
            v3Global.సమితి_Has_Forceable_సంకేతాలు();
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_ప్రజా) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(నిజం);
            m_varp->sig_Mod_ప్రజా(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_ప్రజా_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_ప్రజా_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_ప్రజా(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_ప్రజా_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_విభజించు_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            ఒకవేళ (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } లేకపోతే {
                m_varp->attr_విభజించు_Var(నిజం);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_నిండిన) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::నిండిన);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_FSM_STATE_నిండిన) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(నిజం);
            m_varp->attrFsmStateExpand(VFsmExpandType::నిండిన);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
    }

    శూన్య సందర్శించు(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = శూన్యసూచిక;
        స్థిరరాశి ImplTypedefMap::iterator it = m_implTypedef.కనుగొను(nodep->name());
        ఒకవేళ (it != m_implTypedef.చివర()) {
            defp = it->రెండవ;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } లేకపోతే {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_కణుపు* backp = nodep->backp();
            కొరకు (; backp; backp = backp->backp()) {
                ఒకవేళ (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, కణుపు_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ఆపు;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->బిడ్డ_DTypep();
            dtypep->unlink_Fr_వెనుక();
            ఒకవేళ (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                తిరిగి;
            } లేకపోతే {
                defp = కొత్త AstTypedef{nodep->fileline(), nodep->name(), శూన్యసూచిక, VFlag_బిడ్డ_DType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_గమ్యం_Str(m_implTypedef.కొలత()));
                UINFO(9, "New impltypedef " << defp);
                backp->కూడు_తదుపరి_ఇక్కడ(defp);
            }
        }
        nodep->replaceWith(కొత్త AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }

    శూన్య సందర్శించు(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {
        // FOREACH(array, loopvars, body)
        UINFO(9, "FOREACH " << nodep);
        cleanFileline(nodep);
        // Separate iteration vars from base from variable
        // Input:
        //      v--- arrayp
        //   1. DOT(DOT(first, second), ASTSELLOOPVARS(third, var0..var1))
        // Separated:
        //   bracketp = ASTSELLOOPVARS(...)
        //   arrayp = DOT(DOT(first, second), third)
        //   firstVarp = var0..var1
        // Other examples
        //   2. ASTSELBIT(first, var0))
        //   3. ASTSELLOOPVARS(first, var0..var1))
        //   4. DOT(DOT(first, second), ASTSELBIT(third, var0))
        Ast_Foreach_శీర్షిక* స్థిరరాశి headerp = nodep->headerp();
        ఒకవేళ (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        cleanFileline(nodep);
        ఒకవేళ (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } లేకపోతే {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_పంక్తి* స్థిరరాశి newfl = nodep->fileline();
            newfl->warnOff(V3_Error_సంకేతం::WAITCONST, నిజం);
            nodep->fileline(newfl);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        ఒకవేళ (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_లోపల_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_అంకె);
        VL_RESTORER(m_begin_లోతు);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_అంకె);
        VL_RESTORER(m_విలువ_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        ఒకవేళ (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        ఒకవేళ (VN_IS(nodep, Iface)) m_లోపల_Interface = నిజం;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_అంకె = 0;
        m_begin_లోతు = 0;
        m_విలువ_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = అబద్ధం;
        m_rand_Sequence_అంకె = 0;

        ఒకవేళ (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        ఒకవేళ (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_పోర్టు_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        ఒకవేళ (AstModule* స్థిరరాశి modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    శూన్య సందర్శించు_Iterate_No_విలువ_Mod(Ast_కణుపు* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_విలువ_Modp);
        m_విలువ_Modp = శూన్యసూచిక;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = నిజం;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        సందర్శించు_Iterate_No_విలువ_Mod(nodep);
    }
    శూన్య సందర్శించు(AstCover* nodep) override { సందర్శించు_Iterate_No_విలువ_Mod(nodep); }
    శూన్య సందర్శించు(AstRestrict* nodep) override { సందర్శించు_Iterate_No_విలువ_Mod(nodep); }

    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        V3Control::apply_Coverage_ఖండం(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_లోతు);
        ++m_begin_లోతు;
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        స్థిరరాశి తార్కిక nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        పూర్ణాంకం కేటాయించు_Gen_Blk_అంకె = -1;
        ఒకవేళ (nodep->genforp()) {
            ++m_genblk_అంకె;
            ఒకవేళ (nodep->name() == "") కేటాయించు_Gen_Blk_అంకె = m_genblk_అంకె;
        } లేకపోతే ఒకవేళ (nodep->name() == "" && (VN_IS(backp, Gen_Case_అంశం) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            కేటాయించు_Gen_Blk_అంకె = m_genblkAbove;
        }
        ఒకవేళ (కేటాయించు_Gen_Blk_అంకె != -1) {
            nodep->name("genblk" + cvt_గమ్యం_Str(కేటాయించు_Gen_Blk_అంకె));
            ఒకవేళ (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_మరిన్ని()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        ఒకవేళ (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_అంకె);
            m_genblkAbove = 0;
            m_genblk_అంకె = 0;
            iterateChildren(nodep);
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstGenCase* nodep) override {
        ++m_genblk_అంకె;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_అంకె);
        m_genblkAbove = m_genblk_అంకె;
        m_genblk_అంకె = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        స్థిరరాశి తార్కిక nestedIf = (VN_IS(nodep->backp(), Gen_ఖండం)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_ఖండం)));
        ఒకవేళ (nestedIf) {
            iterateChildren(nodep);
        } లేకపోతే {
            ++m_genblk_అంకె;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_అంకె);
            m_genblkAbove = m_genblk_అంకె;
            m_genblk_అంకె = 0;
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        ఒకవేళ (nodep->origName().ఖాళీలు()) {
            ఒకవేళ (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            స్థిరరాశి string newName = "$unnamedudp" + cvt_గమ్యం_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_ఖండం* nodep) override {
        VL_RESTORER(m_ఖండం_కూడు_Automatic_Stmtp);
        m_ఖండం_కూడు_Automatic_Stmtp = శూన్యసూచిక;
        VL_RESTORER(m_ఖండం_కూడు_Static_Stmtp);
        m_ఖండం_కూడు_Static_Stmtp = శూన్యసూచిక;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_తదుపరి_Null(nodep->declsp());
        }

        ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_ఖండం(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        ఒకవేళ (AstFork* స్థిరరాశి forkp = VN_CAST(nodep, Fork)) iterate_And_తదుపరి_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_కణుపు* stmtsp = శూన్యసూచిక;
        // Move all Vars to front of function
        stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, get_ఖండం_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    శూన్య సందర్శించు(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        ఒకవేళ (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        ఒకవేళ (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(నిజం);
            Ast_కణుపు* స్థిరరాశి nextp = nodep->nextp();
            nodep->unlink_Fr_వెనుక();
            nextp->కూడు_తదుపరి(nodep);
        }
    }
    శూన్య సందర్శించు(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_కణుపు* scanp = nodep;
            // Skip over the New's statement
            కొరకు (; scanp && !VN_IS(scanp, Stmt_సమాసం); scanp = scanp->backp()) {}
            ఒకవేళ (VN_IS(scanp, Stmt_సమాసం)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                కొరకు (; scanp; scanp = scanp->backp()) {
                    ఒకవేళ (VN_IS(scanp, కణుపు_Stmt) || VN_IS(scanp, కణుపు_Module)
                        || VN_IS(scanp, కణుపు_FTask))
                        ఆపు;
                }
                ఒకవేళ (!VN_IS(scanp, కణుపు_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    శూన్య సందర్శించు(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Print_సమయం_Scale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::గమ్యం_string(m_rand_Sequence_అంకె++));
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(Ast_సమయం* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(Ast_సమయం_D* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(Ast_సమయం_Import* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(Ast_సమయం_Unit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    శూన్య సందర్శించు(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* స్థిరరాశి alwaysp = VN_CAST(nodep->backp(), Always);
        ఒకవేళ (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_మరిన్ని() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            ఒకవేళ (Ast_Sen_వృక్షం* స్థిరరాశి sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_వెనుక_With_తదుపరి();
                alwaysp->sentreep(sentreep);
            }
            ఒకవేళ (nodep->stmtsp()) alwaysp->కూడు_Stmtsp(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
    }
    శూన్య సందర్శించు(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_లోపల_Skewp);
        VL_RESTORER(m_default_బహిర్గతం_Skewp);
        // Find default input and output skews
        కొరకు (Ast_కణుపు *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ఒకవేళ (Ast_Clocking_అంశం* citemp = VN_CAST(itemp, Clocking_అంశం)) {
                ఒకవేళ (citemp->exprp() || citemp->assignp()) కొనసాగు;
                ఒకవేళ (citemp->skewp()) {
                    ఒకవేళ (citemp->direction() == VDirection::ఆదానం) {
                        // Disallow default redefinition; note some simulators allow this
                        ఒకవేళ (m_default_లోపల_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_లోపల_Skewp = citemp->skewp();
                    } లేకపోతే ఒకవేళ (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        ఒకవేళ (m_default_బహిర్గతం_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_బహిర్గతం_Skewp = citemp->skewp();
                    } లేకపోతే {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(నెట్టు_Deletep(citemp->unlink_Fr_వెనుక()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Clocking_అంశం* nodep) override {
        cleanFileline(nodep);
        ఒకవేళ (nodep->direction() == VDirection::OUTPUT) {
            ఒకవేళ (!nodep->skewp()) {
                ఒకవేళ (m_default_బహిర్గతం_Skewp) {
                    nodep->skewp(m_default_బహిర్గతం_Skewp->clone_వృక్షం(అబద్ధం));
                } లేకపోతే {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(కొత్త AstConst{nodep->fileline(), 0});
                }
            }
        } లేకపోతే ఒకవేళ (nodep->direction() == VDirection::ఆదానం) {
            ఒకవేళ (!nodep->skewp()) {
                ఒకవేళ (m_default_లోపల_Skewp) {
                    nodep->skewp(m_default_లోపల_Skewp->clone_వృక్షం(అబద్ధం));
                } లేకపోతే {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(కొత్త AstConst{nodep->fileline(), AstConst::One_అడుగు{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        ఒకవేళ (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    శూన్య సందర్శించు(Ast_Tagged_సమాసం* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Tagged_నమూనా* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_నమూనా_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_నమూనా_నక్షత్రం* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        ఒకవేళ (nodep->keyword() == VBasicDTypeKwd::CVOID) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: void (for tagged unions)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    // Return, for each arg in argsp, a parameter plus a "this.<member> = <param>"
    // assignment.  The parameter is a clone of the covergroup member and so shares its
    // name; 'this.' on the LHS targets the member, otherwise the same-named local parameter
    // shadows it and the assignment self-assigns the parameter, leaving the member unwritten.
    // argsp may be null (returns nullptr).
    స్థిర Ast_కణుపు* makeArgMemberCopies(Ast_కణుపు* argsp, తార్కిక readOnlyRefs) {
        Ast_కణుపు* resultp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* స్థిరరాశి origVarp = VN_AS(argp, Var);
            AstVar* స్థిరరాశి paramp = origVarp->clone_వృక్షం(అబద్ధం);
            File_పంక్తి* స్థిరరాశి fl = కొత్త File_పంక్తి{paramp->fileline()};
            fl->warnOff(V3_Error_సంకేతం::VARHIDDEN, నిజం);
            paramp->fileline(fl);
            paramp->funcLocal(నిజం);
            paramp->direction(origVarp->direction());
            ఒకవేళ (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_కణుపు::కూడు_తదుపరి_Null(resultp, paramp);
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstDot{
                origVarp->fileline(), అబద్ధం, కొత్త AstParseRef{origVarp->fileline(), "this"},
                కొత్త AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstParseRef{paramp->fileline(), paramp->name()};
            resultp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{origVarp->fileline(), lhsp, rhsp});
        }
        తిరిగి resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    స్థిర శూన్య createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_కణుపు* argsp,
                                        Ast_కణుపు* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* స్థిరరాశి defaultVarp
            = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->కనుగొను_Int_DType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->కూడు_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        ఒకవేళ (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, నిజం));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* స్థిరరాశి varp
                = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlag_బిడ్డ_DType{},
                             కొత్త AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             కొత్త AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      శూన్యసూచిక, శూన్యసూచిక},
                                             శూన్యసూచిక}};
            nodep->కూడు_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* స్థిరరాశి varp
                = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlag_బిడ్డ_DType{},
                             కొత్త AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             కొత్త AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      శూన్యసూచిక, శూన్యసూచిక},
                                             శూన్యసూచిక}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->కూడు_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* స్థిరరాశి funcp = కొత్త AstFunc{nodep->fileline(), "sample", శూన్యసూచిక, శూన్యసూచిక};
            funcp->కూడు_Stmtsp(makeArgMemberCopies(sampleArgsp, అబద్ధం));
            funcp->class_పద్ధతి(నిజం);
            funcp->dtypep(funcp->కనుగొను_Void_DType());
            funcp->keepAlive(నిజం);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->కూడు_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        కొరకు (స్థిరరాశి string& name : {"start"s, "stop"s}) {
            AstFunc* స్థిరరాశి funcp = కొత్త AstFunc{nodep->fileline(), name, శూన్యసూచిక, శూన్యసూచిక};
            funcp->class_పద్ధతి(నిజం);
            funcp->dtypep(funcp->కనుగొను_Void_DType());
            nodep->కూడు_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        కొరకు (స్థిరరాశి string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* స్థిరరాశి funcp = కొత్త AstFunc{nodep->fileline(), name, శూన్యసూచిక, శూన్యసూచిక};
            funcp->fileline()->warnOff(V3_Error_సంకేతం::NORETURN, నిజం);
            funcp->isStatic(name == "get_coverage");
            funcp->class_పద్ధతి(నిజం);
            funcp->dtypep(funcp->కనుగొను_Void_DType());
            nodep->కూడు_Membersp(funcp);
            {
                AstVar* స్థిరరాశి varp = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->కనుగొను_Double_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(నిజం);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(నిజం);
                funcp->fvarp(varp);
            }
            కొరకు (స్థిరరాశి string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* స్థిరరాశి varp = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->కనుగొను_String_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(నిజం);
                varp->direction(VDirection::ఆదానం);
                varp->valuep(కొత్త AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->కూడు_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* స్థిరరాశి funcp
                = కొత్త AstFunc{nodep->fileline(), "set_inst_name", శూన్యసూచిక, శూన్యసూచిక};
            funcp->class_పద్ధతి(నిజం);
            funcp->dtypep(funcp->కనుగొను_Void_DType());
            nodep->కూడు_Membersp(funcp);
            AstVar* స్థిరరాశి varp = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->కనుగొను_String_DType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(నిజం);
            varp->direction(VDirection::ఆదానం);
            funcp->కూడు_Stmtsp(varp);
        }
    }

    తార్కిక dropDeprecatedCoverageOption(Ast_Cg_Option_కేటాయించు* స్థిరరాశి nodep) {
        ఒకవేళ (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) తిరిగి అబద్ధం;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(Ast_Cg_Option_కేటాయించు* nodep) override {
        ఒకవేళ (dropDeprecatedCoverageOption(nodep)) తిరిగి;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        ఒకవేళ (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) తిరిగి;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        ఒకవేళ (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        స్థిరరాశి string libname = m_modp->libname();
        AstClass* స్థిరరాశి cgClassp = కొత్త AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
        cgClassp->fileline()->warnOff(V3_Error_సంకేతం::UNDRIVEN, నిజం);
        cgClassp->isCovergroup(నిజం);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(నిజం);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        ఒకవేళ (Ast_Sen_వృక్షం* స్థిరరాశి eventp = nodep->eventp()) {
            eventp->unlink_Fr_వెనుక();
            AstCovergroup* స్థిరరాశి cgNodep = కొత్త AstCovergroup{
                nodep->fileline(), nodep->name(), శూన్యసూచిక, శూన్యసూచిక, శూన్యసూచిక, eventp};
            cgClassp->కూడు_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        కొరకు (Ast_కణుపు* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* స్థిరరాశి origVarp = VN_AS(argp, Var);
            ఒకవేళ (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ఆదానం);
            }
            ఒకవేళ ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* స్థిరరాశి memberp = origVarp->clone_వృక్షం(అబద్ధం);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(అబద్ధం);
            memberp->direction(VDirection::NONE);
            ఒకవేళ (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(నిజం);
            cgClassp->కూడు_Membersp(memberp);
        }

        // Convert sample args to member variables
        కొరకు (Ast_కణుపు* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* స్థిరరాశి origVarp = VN_AS(argp, Var);
            ఒకవేళ (!origVarp->is_ఆదానం()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ఆదానం);
            }
            AstVar* స్థిరరాశి memberp = origVarp->clone_వృక్షం(అబద్ధం);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(అబద్ధం);
            memberp->direction(VDirection::NONE);
            cgClassp->కూడు_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* స్థిరరాశి newFuncp = కొత్త AstFunc{nodep->fileline(), "new", శూన్యసూచిక, శూన్యసూచిక};
        newFuncp->fileline()->warnOff(V3_Error_సంకేతం::NORETURN, నిజం);
        newFuncp->class_పద్ధతి(నిజం);
        newFuncp->isConstructor(నిజం);
        newFuncp->dtypep(cgClassp->dtypep());
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_వెనుక_With_తదుపరి();
            newFuncp->కూడు_Stmtsp(bodyp);
        }
        cgClassp->కూడు_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_అంకె);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_అంకె = 0;
        కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCoverpoint* స్థిరరాశి pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    స్థిర string coverpointName(స్థిరరాశి AstCoverpoint* nodep) {
        ఒకవేళ (!nodep->name().ఖాళీలు()) తిరిగి nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        స్థిరరాశి AstParseRef* స్థిరరాశి refp = VN_CAST(nodep->exprp(), ParseRef);
        తిరిగి refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    స్థిరరాశి AstCoverpoint* కనుగొను_Coverpoint(స్థిరరాశి AstCoverpointRef* refp) స్థిరరాశి {
        ఒకవేళ (refp->exprp()) తిరిగి శూన్యసూచిక;  // A hierarchical reference, unsupported
        స్థిరరాశి స్వయంచాలక it = m_coverpoints.కనుగొను(refp->name());
        తిరిగి it == m_coverpoints.చివర() ? శూన్యసూచిక : it->రెండవ;
    }

    శూన్య సందర్శించు(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        ఒకవేళ (nodep->name().ఖాళీలు()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            స్థిరరాశి string name = coverpointName(nodep);
            nodep->name(name.ఖాళీలు() ? "__Vcoverpoint" + cvt_గమ్యం_Str(m_coverpoint_అంకె++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        కొరకు (Ast_కణుపు *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ఒకవేళ (Ast_Cg_Option_కేటాయించు* స్థిరరాశి optp = VN_CAST(itemp, Cg_Option_కేటాయించు)) {
                ఒకవేళ (dropDeprecatedCoverageOption(optp)) కొనసాగు;
                optp->unlink_Fr_వెనుక();
                ఒకవేళ (optp->optType() == VCoverOptionType::AT_కనిష్ఠ
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::బరువు) {
                    nodep->కూడు_Optionsp(కొత్త AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_వృక్షం(అబద్ధం)});
                } లేకపోతే {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_వృక్షం(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        ఒకవేళ (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        ఒకవేళ (స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            ఒకవేళ (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        File_పంక్తి* స్థిరరాశి flNoWarn = కొత్త File_పంక్తి{fl};
        flNoWarn->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
        flNoWarn->modifyWarnOff(V3_Error_సంకేతం::VARHIDDEN, నిజం);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{flNoWarn, VVarType::VAR, "item", VFlag_బిడ్డ_DType{},
                                        కొత్త AstRefDType{fl, AstRefDType::Flag_Type_యొక్క_సమాసం{},
                                                        m_coverpointp->exprp()->clone_వృక్షం(అబద్ధం)}};
        varp->funcLocal(నిజం);
        varp->noReset(నిజం);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* స్థిరరాశి refp = nodep->pointp();
        స్థిరరాశి AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        స్థిరరాశి AstParseRef* binp = శూన్యసూచిక;
        స్థిరరాశి AstDot* స్థిరరాశి dotp = VN_CAST(refp->exprp(), Dot);
        ఒకవేళ (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        ఒకవేళ (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        ఒకవేళ (binp) nodep->name(binp->name());
        refp->replaceWith(కొత్త AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        }
    }

    శూన్య సందర్శించు(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ఒకవేళ (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        }
    }

    శూన్య సందర్శించు(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        కొరకు (Ast_కణుపు *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ఒకవేళ (VN_IS(itemp, CoverCrossBin)) కొనసాగు;
            Ast_Cg_Option_కేటాయించు* స్థిరరాశి optp = VN_AS(itemp, Cg_Option_కేటాయించు);
            ఒకవేళ (dropDeprecatedCoverageOption(optp)) కొనసాగు;
            itemp->unlink_Fr_వెనుక();
            స్థిరరాశి VCoverOptionType optType = optp->optType();
            ఒకవేళ (!(optType == VCoverOptionType::AT_కనిష్ఠ || optType == VCoverOptionType::బరువు)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->కూడు_Optionsp(కొత్త AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_వృక్షం(అబద్ధం)});
            VL_DO_DANGLING(optp->delete_వృక్షం(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        ఒకవేళ (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            File_పంక్తి* స్థిరరాశి flNoWarn = కొత్త File_పంక్తి{fl};
            flNoWarn->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
            flNoWarn->modifyWarnOff(V3_Error_సంకేతం::VARHIDDEN, నిజం);
            std::సమితి<string> పేర్లు;
            కొరకు (స్థిరరాశి Ast_కణుపు* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                స్థిరరాశి AstCoverpointRef* స్థిరరాశి refp = VN_AS(itemp, CoverpointRef);
                స్థిరరాశి AstCoverpoint* స్థిరరాశి pointp = కనుగొను_Coverpoint(refp);
                ఒకవేళ (!pointp || !పేర్లు.చొప్పించు(refp->name()).రెండవ) కొనసాగు;
                AstVar* స్థిరరాశి varp
                    = కొత్త AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlag_బిడ్డ_DType{},
                                 కొత్త AstRefDType{fl, AstRefDType::Flag_Type_యొక్క_సమాసం{},
                                                 pointp->exprp()->clone_వృక్షం(అబద్ధం)}};
                varp->funcLocal(నిజం);
                varp->noReset(నిజం);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->కూడు_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        ఒకవేళ (!nodep->selectp()) VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(AstSystemCSection* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process only once.
        cleanFileline(nodep);
        m_modp->కూడు_Sc_Sectionsp(nodep->unlink_Fr_వెనుక());
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::కూడు_Stat_కూడిక(V3Stats::STAT_ఆధారం_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

శూన్య V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linkparse", 0, dump_వృక్షం_Either_Level() >= 6);
}
