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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkParse.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ವರ್ಗ LinkParseVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_ಸಾಲು*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_ಗಂಟು_Module* m_ಮೌಲ್ಯ_Modp = nullptr;
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ತಾರ್ಕಿಕ m_inInterface = ಅಸತ್ಯ;  // True when inside interface declaration
    Ast_ಗಂಟು_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_ಗಂಟು_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_ಗಂಟು_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ಗಂಟು_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ಗಂಟು_DType* m_dtypep = nullptr;  // Current data type
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, ಧ್ರುವ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    ಪೂರ್ಣಾಙ್ಕ m_anonUdpId = 0;  // Counter for anonymous UDP instances
    ಪೂರ್ಣಾಙ್ಕ m_coverpoint_ಅಂಕೆ = 0;  // Counter for unnamed coverpoints within current covergroup
    ಪೂರ್ಣಾಙ್ಕ m_genblkAbove = 0;  // Begin block number of if/case/for above
    ಪೂರ್ಣಾಙ್ಕ m_genblk_ಅಂಕೆ = 0;  // Begin block number, 0=none seen
    ಪೂರ್ಣಾಙ್ಕ m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    ಪೂರ್ಣಾಙ್ಕ m_rand_Sequence_ಅಂಕೆ = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    ತಾರ್ಕಿಕ m_lifetimeAllowed = ಅಸತ್ಯ;  // True to allow lifetime settings
    ತಾರ್ಕಿಕ m_hasTimingControl = ಅಸತ್ಯ;  // If current task has timing control
    ತಾರ್ಕಿಕ m_moduleWithGenericIface = ಅಸತ್ಯ;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    ಶೂನ್ಯ cleanFileline(Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->user2SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        ಯದಿ (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(ನೂತನ File_ಸಾಲು{nodep->fileline()});
        }
        m_filelines.ಸೇರಿಸು(nodep->fileline());
    }

    string nameFromTypedef(Ast_ಗಂಟು* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        ಯದಿ (ಧ್ರುವ AstTypedef* ಧ್ರುವ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            ಯದಿ (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } ಅನ್ಯಥಾ ಯದಿ (m_modp) {
                above = m_modp->name() + ".";
            }
            ಪ್ರತ್ಯಾಗಮ above + typedefp->name();
        }
        ಪ್ರತ್ಯಾಗಮ "";
    }

    ಶೂನ್ಯ visit_Iterate_ಗಂಟು_DType(Ast_ಗಂಟು_DType* nodep) {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    ತಾರ್ಕಿಕ nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        ಪ್ರತ್ಯಾಗಮ (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    ಶೂನ್ಯ checkIndent(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* childp) {
        // Try very hard to avoid false positives
        Ast_ಗಂಟು* nextp = nodep->nextp();
        ಯದಿ (!childp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        ಯದಿ (!nextp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ ಗಂಟು_Flp = nodep->fileline();
        File_ಸಾಲು* ಧ್ರುವ childFlp = childp->fileline();
        File_ಸಾಲು* ಧ್ರುವ nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        ಯದಿ (ಗಂಟು_Flp->filenameno() != childFlp->filenameno()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (ಗಂಟು_Flp->filenameno() != nextFlp->filenameno()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (ಗಂಟು_Flp->lastLineno() >= childFlp->firstLineno()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (childFlp->lastLineno() >= nextFlp->firstLineno()) ಪ್ರತ್ಯಾಗಮ;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        ಯದಿ (!(ಗಂಟು_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            ಪ್ರತ್ಯಾಗಮ;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ಧ್ರುವ std::string ಗಂಟು_Prefix = ಗಂಟು_Flp->sourcePrefix(ಗಂಟು_Flp->firstColumn());
        ಧ್ರುವ std::string childPrefix = childFlp->sourcePrefix(ಗಂಟು_Flp->firstColumn());
        ಧ್ರುವ std::string nextPrefix = nextFlp->sourcePrefix(ಗಂಟು_Flp->firstColumn());
        ಯದಿ (childPrefix != ಗಂಟು_Prefix) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nextPrefix != childPrefix) ಪ್ರತ್ಯಾಗಮ;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ಧ್ರುವ std::string ಗಂಟು_Source = ಗಂಟು_Flp->source();
        ಧ್ರುವ std::string childSource = childFlp->source();
        ಧ್ರುವ std::string nextSource = nextFlp->source();
        ಯದಿ (!(VString::leading_Whitespace_ಎಣಿಕೆ(ಗಂಟು_Source)
                  < VString::leading_Whitespace_ಎಣಿಕೆ(childSource)
              && VString::leading_Whitespace_ಎಣಿಕೆ(nextSource)
                     >= VString::leading_Whitespace_ಎಣಿಕೆ(childSource)))
            ಪ್ರತ್ಯಾಗಮ;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    ಶೂನ್ಯ collectPorts(Ast_ಗಂಟು* ಗಂಟು_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = ಗಂಟು_Listp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                ಯದಿ (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = ಗಂಟು_Listp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                ಯದಿ (varp->isIO()) ಅನುವರ್ತ;
                ಧ್ರುವ auto it = portNames.find(varp->name());
                ಯದಿ (it == portNames.end()) ಅನುವರ್ತ;
                AstVar* ಧ್ರುವ iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                ಯದಿ (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_ಗಂಟು* getVarsAndUnlink(Ast_ಗಂಟು* stmtsp) {
        Ast_ಗಂಟು* varsp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಯದಿ (VN_IS(itemp, Var)) varsp = Ast_ಗಂಟು::addNext(varsp, itemp->unlinkFrBack());
        }
        ಪ್ರತ್ಯಾಗಮ varsp;
    }
    Ast_ಗಂಟು_Stmt* getBlockAdds() {
        Ast_ಗಂಟು_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        ಯದಿ (m_blockAddStaticStmtp)
            addsp = Ast_ಗಂಟು::addNext(addsp,
                                     ನೂತನ AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        ಯದಿ (m_blockAddAutomaticStmtp)
            addsp = Ast_ಗಂಟು::addNext(
                addsp, ನೂತನ AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        ಪ್ರತ್ಯಾಗಮ addsp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process only once.
        // Mark class methods
        ಯದಿ (VN_IS(m_modp, Class)) nodep->classMethod(ಸತ್ಯ);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ಸತ್ಯ;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) { ಪ್ರತ್ಯಾಗಮ nodep->isTimingControl(); });
        ಯದಿ (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        ಯದಿ (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ಗಂಟು* stmtsp = nullptr;
        ಯದಿ (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType* nodep) override { visit_Iterate_ಗಂಟು_DType(nodep); }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(ಸತ್ಯ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintForeach* nodep) override {
        ಯದಿ (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* exprp) { exprp->isSoft(ಸತ್ಯ); });
        }
    }
    ಶೂನ್ಯ visit(AstEnumDType* nodep) override {
        ಯದಿ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಯದಿ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (nodep->rangep()) {
            ಯದಿ (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i : nodep->rangep()->seq_ಎಡ_To_ಬಲ()) {
                ಧ್ರುವ string name = nodep->name() + cvtToStr(i);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
                ಯದಿ (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = ನೂತನ AstAdd{flp, nodep->valuep()->clone_ವೃಕ್ಷ(ಸತ್ಯ),
                                     ನೂತನ AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_ಗಂಟು::addNext(addp, ನೂತನ AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class);
        ಯದಿ (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(ಸತ್ಯ);
        }
        ಯದಿ (nodep->embeddedCovergroup()) {
            AstRefDType* ಧ್ರುವ refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        ಯದಿ (nodep->valuep()) nodep->hasUserInit(ಸತ್ಯ);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        ಯದಿ (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            ಯದಿ (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warnMore() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } ಅನ್ಯಥಾ {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        ಯದಿ (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        ಯದಿ (!nodep->direction().isAny()) {  // Not a port
            ಯದಿ (nodep->lifetime().isNone()) {
                ಯದಿ (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } ಅನ್ಯಥಾ {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            ಯದಿ (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } ಅನ್ಯಥಾ {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        ಯದಿ (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            ಯದಿ (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        ಯದಿ (nodep->isGParam() && m_modp) m_modp->hasGParam(ಸತ್ಯ);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        ಯದಿ (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(ಸತ್ಯ);

        ಯದಿ (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        ಯದಿ (nodep->isParam() && m_inInterface) nodep->isIfaceParam(ಸತ್ಯ);
        ಯದಿ (AstParseTypeDType* ಧ್ರುವ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ಗಂಟು* dtypep = nodep->valuep();
            ಯದಿ (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_CAST(dtypep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    ಯಾವತ್ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        ಯದಿ (!rhsDotp) ಭಙ್ಗ;
                        File_ಸಾಲು* ಧ್ರುವ fl = dotp->fileline();
                        ಧ್ರುವ ತಾರ್ಕಿಕ colon = dotp->colon();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        File_ಸಾಲು* ಧ್ರುವ rhsFl = rhsDotp->fileline();
                        ಧ್ರುವ ತಾರ್ಕಿಕ rhsColon = rhsDotp->colon();
                        AstDot* ಧ್ರುವ newLhs = ನೂತನ AstDot{fl, colon, lhs, rhsLhs};
                        exprp = ನೂತನ AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } ಅನ್ಯಥಾ {
                dtypep = ನೂತನ AstVoidDType{nodep->fileline()};
            }

            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, ನೂತನ AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(ಅಸತ್ಯ), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        ಯದಿ (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            ಯದಿ (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (m_modp && v3Global.opt.publicDepth()) {
                ಯದಿ ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(ಸತ್ಯ);
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(ಸತ್ಯ);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        ಯದಿ (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(ಅಸತ್ಯ);
            nodep->trace(ಅಸತ್ಯ);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        ಯದಿ (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ಸತ್ಯ);
        // Compute initial value
        ಯದಿ (nodep->valuep() || nodep->needsCReset()) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಯದಿ (nodep->valuep()) {
                    ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->valuep()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                } ಅನ್ಯಥಾ {
                    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCReset{fl, nodep, ಅಸತ್ಯ};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            ಯದಿ (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } ಅನ್ಯಥಾ ಯದಿ (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ಒಳಹರಿವು()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                ಯದಿ (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                }
            } ಅನ್ಯಥಾ ಯದಿ (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(ಸತ್ಯ);
                File_ಸಾಲು* ಧ್ರುವ newfl = ನೂತನ File_ಸಾಲು{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, ಸತ್ಯ);
                AstAssign* ಧ್ರುವ assp
                    = ನೂತನ AstAssign{newfl, ನೂತನ AstParseRef{newfl, nodep->name()}, createValuep()};
                ಯದಿ (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_ಗಂಟು::addNext(m_blockAddAutomaticStmtp, assp);
                } ಅನ್ಯಥಾ {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_ಗಂಟು::addNext(m_blockAddStaticStmtp, assp);
                }
            } ಅನ್ಯಥಾ ಯದಿ (m_ಮೌಲ್ಯ_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                ಯದಿ (nodep->valuep()) {
                    nodep->noCReset(ಸತ್ಯ);
                    File_ಸಾಲು* ಧ್ರುವ newfl = ನೂತನ File_ಸಾಲು{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, ಸತ್ಯ);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, ಸತ್ಯ);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* ಧ್ರುವ assp = ನೂತನ AstAssign{
                        newfl, ನೂತನ AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ಗಂಟು* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    ಯದಿ (nodep->lifetime().isAutomatic()) {
                        newInitp = ನೂತನ AstInitialAutomatic{newfl, assp};
                    } ಅನ್ಯಥಾ {
                        newInitp = ನೂತನ AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isNegative(ಧ್ರುವ AstConst* constp) {
        ಪ್ರತ್ಯಾಗಮ constp->isSigned() && constp->ಅಂಕೆ().bitIs1(constp->width() - 1);
    }
    ಶೂನ್ಯ visit(AstRange* nodep) override {
        ಸ್ಥಿರ constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ಧ್ರುವ AstConst* ಧ್ರುವ leftp = VN_CAST(nodep->leftp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ rightp = VN_CAST(nodep->rightp(), Const);
        ಯದಿ (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ಧ್ರುವ uint64_t ಎಡ = leftp->toUQuad();
            ಧ್ರುವ uint64_t ಬಲ = rightp->toUQuad();
            ಧ್ರುವ uint64_t span = std::max(ಎಡ, ಬಲ) - std::min(ಎಡ, ಬಲ);
            ಯದಿ (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        ಯದಿ (nodep->ಅಂಕೆ().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* ಧ್ರುವ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ಸತ್ಯ);
            m_varp->sigModPublic(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            ಯದಿ (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } ಅನ್ಯಥಾ {
                m_varp->attrSplitVar(ಸತ್ಯ);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ಸತ್ಯ);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        }
    }

    ಶೂನ್ಯ visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        ಧ್ರುವ ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        ಯದಿ (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } ಅನ್ಯಥಾ {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ಗಂಟು* backp = nodep->backp();
            ಪ್ರತಿ (; backp; backp = backp->backp()) {
                ಯದಿ (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ಗಂಟು_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ಭಙ್ಗ;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            ಯದಿ (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                defp = ನೂತನ AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(ನೂತನ AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {
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
        AstForeachHeader* ಧ್ರುವ headerp = nodep->headerp();
        ಯದಿ (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        ಯದಿ (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } ಅನ್ಯಥಾ {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_ಸಾಲು* ಧ್ರುವ newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, ಸತ್ಯ);
            nodep->fileline(newfl);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        ಯದಿ (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ಅಂಕೆ);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_ಅಂಕೆ);
        VL_RESTORER(m_ಮೌಲ್ಯ_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        ಯದಿ (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        ಯದಿ (VN_IS(nodep, Iface)) m_inInterface = ಸತ್ಯ;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_ಅಂಕೆ = 0;
        m_beginDepth = 0;
        m_ಮೌಲ್ಯ_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = ಅಸತ್ಯ;
        m_rand_Sequence_ಅಂಕೆ = 0;

        ಯದಿ (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        ಯದಿ (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        ಯದಿ (AstModule* ಧ್ರುವ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    ಶೂನ್ಯ visit_Iterate_No_ಮೌಲ್ಯ_Mod(Ast_ಗಂಟು* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_ಮೌಲ್ಯ_Modp);
        m_ಮೌಲ್ಯ_Modp = nullptr;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ಸತ್ಯ;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_ಮೌಲ್ಯ_Mod(nodep);
    }
    ಶೂನ್ಯ visit(AstCover* nodep) override { visit_Iterate_No_ಮೌಲ್ಯ_Mod(nodep); }
    ಶೂನ್ಯ visit(AstRestrict* nodep) override { visit_Iterate_No_ಮೌಲ್ಯ_Mod(nodep); }

    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ಧ್ರುವ ತಾರ್ಕಿಕ nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        ಪೂರ್ಣಾಙ್ಕ assign_Gen_Blk_ಅಂಕೆ = -1;
        ಯದಿ (nodep->genforp()) {
            ++m_genblk_ಅಂಕೆ;
            ಯದಿ (nodep->name() == "") assign_Gen_Blk_ಅಂಕೆ = m_genblk_ಅಂಕೆ;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_ಅಂಕೆ = m_genblkAbove;
        }
        ಯದಿ (assign_Gen_Blk_ಅಂಕೆ != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_ಅಂಕೆ));
            ಯದಿ (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        ಯದಿ (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ಅಂಕೆ);
            m_genblkAbove = 0;
            m_genblk_ಅಂಕೆ = 0;
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstGenCase* nodep) override {
        ++m_genblk_ಅಂಕೆ;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ಅಂಕೆ);
        m_genblkAbove = m_genblk_ಅಂಕೆ;
        m_genblk_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ಧ್ರುವ ತಾರ್ಕಿಕ nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        ಯದಿ (nestedIf) {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            ++m_genblk_ಅಂಕೆ;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ಅಂಕೆ);
            m_genblkAbove = m_genblk_ಅಂಕೆ;
            m_genblk_ಅಂಕೆ = 0;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        ಯದಿ (nodep->origName().empty()) {
            ಯದಿ (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ಧ್ರುವ string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        ಯದಿ (AstFork* ಧ್ರುವ forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ಗಂಟು* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ಶೂನ್ಯ visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        ಯದಿ (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        ಯದಿ (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(ಸತ್ಯ);
            Ast_ಗಂಟು* ಧ್ರುವ nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ಗಂಟು* scanp = nodep;
            // Skip over the New's statement
            ಪ್ರತಿ (; scanp && !VN_IS(scanp, Stmt_ಅಭಿವ್ಯಕ್ತಿ); scanp = scanp->backp()) {}
            ಯದಿ (VN_IS(scanp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                ಪ್ರತಿ (; scanp; scanp = scanp->backp()) {
                    ಯದಿ (VN_IS(scanp, ಗಂಟು_Stmt) || VN_IS(scanp, ಗಂಟು_Module)
                        || VN_IS(scanp, ಗಂಟು_FTask))
                        ಭಙ್ಗ;
                }
                ಯದಿ (!VN_IS(scanp, ಗಂಟು_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_ಅಂಕೆ++));
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ಧ್ರುವ alwaysp = VN_CAST(nodep->backp(), Always);
        ಯದಿ (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            ಯದಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            ಯದಿ (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಯದಿ (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                ಯದಿ (citemp->exprp() || citemp->assignp()) ಅನುವರ್ತ;
                ಯದಿ (citemp->skewp()) {
                    ಯದಿ (citemp->direction() == VDirection::ಒಳಹರಿವು) {
                        // Disallow default redefinition; note some simulators allow this
                        ಯದಿ (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } ಅನ್ಯಥಾ ಯದಿ (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        ಯದಿ (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } ಅನ್ಯಥಾ {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        ಯದಿ (nodep->direction() == VDirection::OUTPUT) {
            ಯದಿ (!nodep->skewp()) {
                ಯದಿ (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                } ಅನ್ಯಥಾ {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(ನೂತನ AstConst{nodep->fileline(), 0});
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            ಯದಿ (!nodep->skewp()) {
                ಯದಿ (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                } ಅನ್ಯಥಾ {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(ನೂತನ AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        ಯದಿ (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    ಶೂನ್ಯ visit(Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        ಯದಿ (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ಸ್ಥಿರ Ast_ಗಂಟು* makeArgMemberCopies(Ast_ಗಂಟು* argsp, ತಾರ್ಕಿಕ readOnlyRefs) {
        Ast_ಗಂಟು* resultp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ಧ್ರುವ origVarp = VN_AS(argp, Var);
            AstVar* ಧ್ರುವ paramp = origVarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            File_ಸಾಲು* ಧ್ರುವ fl = ನೂತನ File_ಸಾಲು{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, ಸತ್ಯ);
            paramp->fileline(fl);
            paramp->funcLocal(ಸತ್ಯ);
            paramp->direction(origVarp->direction());
            ಯದಿ (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ಗಂಟು::addNextNull(resultp, paramp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstDot{
                origVarp->fileline(), ಅಸತ್ಯ, ನೂತನ AstParseRef{origVarp->fileline(), "this"},
                ನೂತನ AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(ನೂತನ AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ಸ್ಥಿರ ಶೂನ್ಯ createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ಗಂಟು* argsp,
                                        Ast_ಗಂಟು* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ಧ್ರುವ defaultVarp
            = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        ಯದಿ (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, ಸತ್ಯ));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             ನೂತನ AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             ನೂತನ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             ನೂತನ AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             ನೂತನ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ಧ್ರುವ funcp = ನೂತನ AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, ಅಸತ್ಯ));
            funcp->classMethod(ಸತ್ಯ);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(ಸತ್ಯ);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        ಪ್ರತಿ (ಧ್ರುವ string& name : {"start"s, "stop"s}) {
            AstFunc* ಧ್ರುವ funcp = ನೂತನ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(ಸತ್ಯ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        ಪ್ರತಿ (ಧ್ರುವ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ಧ್ರುವ funcp = ನೂತನ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, ಸತ್ಯ);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(ಸತ್ಯ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ಸತ್ಯ);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(ಸತ್ಯ);
                funcp->fvarp(varp);
            }
            ಪ್ರತಿ (ಧ್ರುವ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ಸತ್ಯ);
                varp->direction(VDirection::ಒಳಹರಿವು);
                varp->valuep(ನೂತನ AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ಧ್ರುವ funcp
                = ನೂತನ AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(ಸತ್ಯ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ಸತ್ಯ);
            varp->direction(VDirection::ಒಳಹರಿವು);
            funcp->addStmtsp(varp);
        }
    }

    ತಾರ್ಕಿಕ dropDeprecatedCoverageOption(AstCgOptionAssign* ಧ್ರುವ nodep) {
        ಯದಿ (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstCgOptionAssign* nodep) override {
        ಯದಿ (dropDeprecatedCoverageOption(nodep)) ಪ್ರತ್ಯಾಗಮ;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        ಯದಿ (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ಪ್ರತ್ಯಾಗಮ;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        ಯದಿ (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ಧ್ರುವ string libname = m_modp->libname();
        AstClass* ಧ್ರುವ cgClassp = ನೂತನ AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, ಸತ್ಯ);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, ಸತ್ಯ);
        cgClassp->isCovergroup(ಸತ್ಯ);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(ಸತ್ಯ);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        ಯದಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* ಧ್ರುವ cgNodep = ನೂತನ AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        ಪ್ರತಿ (Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ಧ್ರುವ origVarp = VN_AS(argp, Var);
            ಯದಿ (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ಒಳಹರಿವು);
            }
            ಯದಿ ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ಧ್ರುವ memberp = origVarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ಅಸತ್ಯ);
            memberp->direction(VDirection::NONE);
            ಯದಿ (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(ಸತ್ಯ);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        ಪ್ರತಿ (Ast_ಗಂಟು* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ಧ್ರುವ origVarp = VN_AS(argp, Var);
            ಯದಿ (!origVarp->is_ಒಳಹರಿವು()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ಒಳಹರಿವು);
            }
            AstVar* ಧ್ರುವ memberp = origVarp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ಅಸತ್ಯ);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ಧ್ರುವ newFuncp = ನೂತನ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, ಸತ್ಯ);
        newFuncp->classMethod(ಸತ್ಯ);
        newFuncp->isConstructor(ಸತ್ಯ);
        newFuncp->dtypep(cgClassp->dtypep());
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_ಅಂಕೆ);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_ಅಂಕೆ = 0;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstCoverpoint* ಧ್ರುವ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ಸ್ಥಿರ string coverpointName(ಧ್ರುವ AstCoverpoint* nodep) {
        ಯದಿ (!nodep->name().empty()) ಪ್ರತ್ಯಾಗಮ nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ಧ್ರುವ AstParseRef* ಧ್ರುವ refp = VN_CAST(nodep->exprp(), ParseRef);
        ಪ್ರತ್ಯಾಗಮ refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ಧ್ರುವ AstCoverpoint* findCoverpoint(ಧ್ರುವ AstCoverpointRef* refp) ಧ್ರುವ {
        ಯದಿ (refp->exprp()) ಪ್ರತ್ಯಾಗಮ nullptr;  // A hierarchical reference, unsupported
        ಧ್ರುವ auto it = m_coverpoints.find(refp->name());
        ಪ್ರತ್ಯಾಗಮ it == m_coverpoints.end() ? nullptr : it->second;
    }

    ಶೂನ್ಯ visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        ಯದಿ (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ಧ್ರುವ string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_ಅಂಕೆ++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        ಪ್ರತಿ (Ast_ಗಂಟು *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಯದಿ (AstCgOptionAssign* ಧ್ರುವ optp = VN_CAST(itemp, CgOptionAssign)) {
                ಯದಿ (dropDeprecatedCoverageOption(optp)) ಅನುವರ್ತ;
                optp->unlinkFrBack();
                ಯದಿ (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(ನೂತನ AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
                } ಅನ್ಯಥಾ {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_ವೃಕ್ಷ(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        ಯದಿ (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        ಯದಿ (ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            ಯದಿ (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        File_ಸಾಲು* ಧ್ರುವ flNoWarn = ನೂತನ File_ಸಾಲು{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ಸತ್ಯ);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, ಸತ್ಯ);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        ನೂತನ AstRefDType{fl, AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{},
                                                        m_coverpointp->exprp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}};
        varp->funcLocal(ಸತ್ಯ);
        varp->noReset(ಸತ್ಯ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ಧ್ರುವ refp = nodep->pointp();
        ಧ್ರುವ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ಧ್ರುವ AstParseRef* binp = nullptr;
        ಧ್ರುವ AstDot* ಧ್ರುವ dotp = VN_CAST(refp->exprp(), Dot);
        ಯದಿ (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        ಯದಿ (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        ಯದಿ (binp) nodep->name(binp->name());
        refp->replaceWith(ನೂತನ AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ಶೂನ್ಯ visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಯದಿ (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ಶೂನ್ಯ visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        ಪ್ರತಿ (Ast_ಗಂಟು *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಯದಿ (VN_IS(itemp, CoverCrossBin)) ಅನುವರ್ತ;
            AstCgOptionAssign* ಧ್ರುವ optp = VN_AS(itemp, CgOptionAssign);
            ಯದಿ (dropDeprecatedCoverageOption(optp)) ಅನುವರ್ತ;
            itemp->unlinkFrBack();
            ಧ್ರುವ VCoverOptionType optType = optp->optType();
            ಯದಿ (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(ನೂತನ AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
            VL_DO_DANGLING(optp->delete_ವೃಕ್ಷ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        ಯದಿ (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            File_ಸಾಲು* ಧ್ರುವ flNoWarn = ನೂತನ File_ಸಾಲು{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ಸತ್ಯ);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, ಸತ್ಯ);
            std::set<string> names;
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಧ್ರುವ AstCoverpointRef* ಧ್ರುವ refp = VN_AS(itemp, CoverpointRef);
                ಧ್ರುವ AstCoverpoint* ಧ್ರುವ pointp = findCoverpoint(refp);
                ಯದಿ (!pointp || !names.ಸೇರಿಸು(refp->name()).second) ಅನುವರ್ತ;
                AstVar* ಧ್ರುವ varp
                    = ನೂತನ AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 ನೂತನ AstRefDType{fl, AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{},
                                                 pointp->exprp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}};
                varp->funcLocal(ಸತ್ಯ);
                varp->noReset(ಸತ್ಯ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        ಯದಿ (!nodep->selectp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(AstSystemCSection* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkparse", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
