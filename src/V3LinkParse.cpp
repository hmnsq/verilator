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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkParse.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ਵਰਗ LinkParseVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // TYPES
    ਵਰਤੋਂ ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_ਸੈੱਟ<File_ਸਤਰ*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = ਖਾਲੀ_ਸੂਚਕ;  // Variable we're under
    Ast_ਗੰਢ_Module* m_ਮੁੱਲ_Modp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Interface = ਝੂਠ;  // True when inside interface declaration
    Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current procedure
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current task
    AstRSProd* m_rsProdp = ਖਾਲੀ_ਸੂਚਕ;  // Current randsequence production
    Ast_ਗੰਢ_ਖੰਡ* m_blockp = ਖਾਲੀ_ਸੂਚਕ;  // Current AstNodeBlock
    Ast_ਗੰਢ_Stmt* m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Initial statements to add to block
    Ast_ਗੰਢ_Stmt* m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Initial statements to add to block
    Ast_ਗੰਢ_DType* m_dtypep = ਖਾਲੀ_ਸੂਚਕ;  // Current data type
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_default_ਵਿੱਚ_Skewp = ਖਾਲੀ_ਸੂਚਕ;  // Current default input skew
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_default_ਬਾਹਰਲਾ_Skewp = ਖਾਲੀ_ਸੂਚਕ;  // Current default output skew
    AstCoverpoint* m_coverpointp = ਖਾਲੀ_ਸੂਚਕ;  // Current coverpoint
    AstCoverCross* m_crossp = ਖਾਲੀ_ਸੂਚਕ;  // Current cross
    std::map<string, ਅਚਲ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    ਪੂਰਨ m_anonUdpId = 0;  // Counter for anonymous UDP instances
    ਪੂਰਨ m_coverpoint_ਅੰਕ = 0;  // Counter for unnamed coverpoints within current covergroup
    ਪੂਰਨ m_genblkAbove = 0;  // Begin block number of if/case/for above
    ਪੂਰਨ m_genblk_ਅੰਕ = 0;  // Begin block number, 0=none seen
    ਪੂਰਨ m_begin_ਡੂੰਘਾਈ = 0;  // How many begin blocks above current node within current AstNodeModule
    ਪੂਰਨ m_rand_Sequence_ਅੰਕ = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    ਤਾਰਕਿਕ m_lifetimeAllowed = ਝੂਠ;  // True to allow lifetime settings
    ਤਾਰਕਿਕ m_hasTimingControl = ਝੂਠ;  // If current task has timing control
    ਤਾਰਕਿਕ m_moduleWithGenericIface = ਝੂਠ;  // If current module contains generic interface
    std::ਸੈੱਟ<AstVar*> m_ਪੋਰਟ_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    ਖਾਲੀ cleanFileline(Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->user2_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        ਜੇ (m_filelines.ਲੱਭੋ(nodep->fileline()) != m_filelines.ਸਿਰਾ()) {
            nodep->fileline(ਨਵਾਂ File_ਸਤਰ{nodep->fileline()});
        }
        m_filelines.ਪਾਓ(nodep->fileline());
    }

    string nameFromTypedef(Ast_ਗੰਢ* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        ਜੇ (ਅਚਲ AstTypedef* ਅਚਲ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            ਜੇ (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } ਨਹੀਂਤਾਂ ਜੇ (m_modp) {
                above = m_modp->name() + ".";
            }
            ਵਾਪਸ above + typedefp->name();
        }
        ਵਾਪਸ "";
    }

    ਖਾਲੀ ਦੇਖੋ_Iterate_ਗੰਢ_DType(Ast_ਗੰਢ_DType* nodep) {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    ਤਾਰਕਿਕ nestedIfBegin(Ast_Gen_ਖੰਡ* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        ਵਾਪਸ (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, Gen_Case_ਵਸਤੂ))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    ਖਾਲੀ checkIndent(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* childp) {
        // Try very hard to avoid false positives
        Ast_ਗੰਢ* nextp = nodep->nextp();
        ਜੇ (!childp) ਵਾਪਸ;
        ਜੇ (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        ਜੇ (!nextp) ਵਾਪਸ;
        ਜੇ (VN_IS(childp, Begin) || VN_IS(childp, Gen_ਖੰਡ)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ ਗੰਢ_Flp = nodep->fileline();
        File_ਸਤਰ* ਅਚਲ ਬੱਚਾ_Flp = childp->fileline();
        File_ਸਤਰ* ਅਚਲ ਅਗਲਾ_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        ਜੇ (ਗੰਢ_Flp->filenameno() != ਬੱਚਾ_Flp->filenameno()) ਵਾਪਸ;
        ਜੇ (ਗੰਢ_Flp->filenameno() != ਅਗਲਾ_Flp->filenameno()) ਵਾਪਸ;
        ਜੇ (ਗੰਢ_Flp->ਆਖਰੀ_Lineno() >= ਬੱਚਾ_Flp->ਪਹਿਲਾ_Lineno()) ਵਾਪਸ;
        ਜੇ (ਬੱਚਾ_Flp->ਆਖਰੀ_Lineno() >= ਅਗਲਾ_Flp->ਪਹਿਲਾ_Lineno()) ਵਾਪਸ;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        ਜੇ (!(ਗੰਢ_Flp->ਪਹਿਲਾ_ਕਾਲਮ() < ਬੱਚਾ_Flp->ਪਹਿਲਾ_ਕਾਲਮ()
              && ਅਗਲਾ_Flp->ਪਹਿਲਾ_ਕਾਲਮ() >= ਬੱਚਾ_Flp->ਪਹਿਲਾ_ਕਾਲਮ()))
            ਵਾਪਸ;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ਅਚਲ std::string ਗੰਢ_ਅਗੇਤਰ = ਗੰਢ_Flp->ਸਰੋਤ_ਅਗੇਤਰ(ਗੰਢ_Flp->ਪਹਿਲਾ_ਕਾਲਮ());
        ਅਚਲ std::string ਬੱਚਾ_ਅਗੇਤਰ = ਬੱਚਾ_Flp->ਸਰੋਤ_ਅਗੇਤਰ(ਗੰਢ_Flp->ਪਹਿਲਾ_ਕਾਲਮ());
        ਅਚਲ std::string ਅਗਲਾ_ਅਗੇਤਰ = ਅਗਲਾ_Flp->ਸਰੋਤ_ਅਗੇਤਰ(ਗੰਢ_Flp->ਪਹਿਲਾ_ਕਾਲਮ());
        ਜੇ (ਬੱਚਾ_ਅਗੇਤਰ != ਗੰਢ_ਅਗੇਤਰ) ਵਾਪਸ;
        ਜੇ (ਅਗਲਾ_ਅਗੇਤਰ != ਬੱਚਾ_ਅਗੇਤਰ) ਵਾਪਸ;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ਅਚਲ std::string ਗੰਢ_ਸਰੋਤ = ਗੰਢ_Flp->ਸਰੋਤ();
        ਅਚਲ std::string ਬੱਚਾ_ਸਰੋਤ = ਬੱਚਾ_Flp->ਸਰੋਤ();
        ਅਚਲ std::string ਅਗਲਾ_ਸਰੋਤ = ਅਗਲਾ_Flp->ਸਰੋਤ();
        ਜੇ (!(VString::leading_Whitespace_ਗਿਣਤੀ(ਗੰਢ_ਸਰੋਤ)
                  < VString::leading_Whitespace_ਗਿਣਤੀ(ਬੱਚਾ_ਸਰੋਤ)
              && VString::leading_Whitespace_ਗਿਣਤੀ(ਅਗਲਾ_ਸਰੋਤ)
                     >= VString::leading_Whitespace_ਗਿਣਤੀ(ਬੱਚਾ_ਸਰੋਤ)))
            ਵਾਪਸ;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    ਖਾਲੀ collectPorts(Ast_ਗੰਢ* ਗੰਢ_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> ਪੋਰਟ_ਨਾਂ;
        ਲਈ (Ast_ਗੰਢ* nodep = ਗੰਢ_Listp; nodep; nodep = nodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isIO()) ਪੋਰਟ_ਨਾਂ.emplace(varp->name(), varp);
            }
        }
        ਲਈ (Ast_ਗੰਢ* nodep = ਗੰਢ_Listp; nodep; nodep = nodep->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isIO()) ਜਾਰੀ;
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = ਪੋਰਟ_ਨਾਂ.ਲੱਭੋ(varp->name());
                ਜੇ (it == ਪੋਰਟ_ਨਾਂ.ਸਿਰਾ()) ਜਾਰੀ;
                AstVar* ਅਚਲ iop = it->ਦੂਜਾ;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                ਜੇ (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                m_ਪੋਰਟ_Dups.emplace(varp);
            }
        }
    }

    Ast_ਗੰਢ* getVarsAndUnlink(Ast_ਗੰਢ* stmtsp) {
        Ast_ਗੰਢ* varsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ਜੇ (VN_IS(itemp, Var)) varsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(varsp, itemp->unlink_Fr_ਪਿੱਠ());
        }
        ਵਾਪਸ varsp;
    }
    Ast_ਗੰਢ_Stmt* get_ਖੰਡ_Adds() {
        Ast_ਗੰਢ_Stmt* addsp = ਖਾਲੀ_ਸੂਚਕ;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        ਜੇ (m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp)
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp,
                                     ਨਵਾਂ AstInitialStaticStmt{m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp->fileline(),
                                                              m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp});
        ਜੇ (m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp)
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                addsp, ਨਵਾਂ AstInitialAutomaticStmt{m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp->fileline(),
                                                   m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp});
        m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ addsp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process only once.
        // Mark class methods
        ਜੇ (VN_IS(m_modp, Class)) nodep->class_ਵਿਧੀ(ਸੱਚ);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp);
        m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        VL_RESTORER(m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp);
        m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ਸੱਚ;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) { ਵਾਪਸ nodep->isTimingControl(); });
        ਜੇ (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->class_ਵਿਧੀ()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        ਜੇ (nodep->class_ਵਿਧੀ() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_ਹੋਰ() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_ਪੋਰਟ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_ਪਿੱਠ();
        // Move all Vars to front of function
        stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, get_ਖੰਡ_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType* nodep) override { ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(ਸੱਚ);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintForeach* nodep) override {
        ਜੇ (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_ਵਿਅੰਜਕ* exprp) { exprp->isSoft(ਸੱਚ); });
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override {
        ਜੇ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        ਜੇ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (nodep->rangep()) {
            ਜੇ (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t ਵਿੱਥ_from_init = 0;
            Ast_Enum_ਵਸਤੂ* addp = ਖਾਲੀ_ਸੂਚਕ;
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            ਲਈ (ਅਚਲ ਪੂਰਨ i : nodep->rangep()->seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ()) {
                ਅਚਲ string name = nodep->name() + cvt_ਮੰਜ਼ਿਲ_Str(i);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = ਨਵਾਂ Ast_ਜੋੜੋ{flp, nodep->valuep()->clone_ਰੁੱਖ(ਸੱਚ),
                                     ਨਵਾਂ AstConst{flp, AstConst::Unsized32{}, ਵਿੱਥ_from_init}};
                }
                addp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addp, ਨਵਾਂ Ast_Enum_ਵਸਤੂ{flp, name, ਖਾਲੀ_ਸੂਚਕ, valuep});
                ++ਵਿੱਥ_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class);
        ਜੇ (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(ਸੱਚ);
        }
        ਜੇ (nodep->embeddedCovergroup()) {
            AstRefDType* ਅਚਲ refp = VN_AS(nodep->ਬੱਚਾ_DTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        ਜੇ (nodep->valuep()) nodep->hasUserInit(ਸੱਚ);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        ਜੇ (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            ਜੇ (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_ਹੋਰ() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_ਹੋਰ() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_ਹੋਰ() << "... The initializer value will only be set once\n"
                        << nodep->warn_ਹੋਰ()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        ਜੇ (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        ਜੇ (!nodep->direction().isAny()) {  // Not a port
            ਜੇ (nodep->lifetime().isNone()) {
                ਜੇ (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } ਨਹੀਂਤਾਂ {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            ਜੇ (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } ਨਹੀਂਤਾਂ {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        ਜੇ (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            ਜੇ (m_begin_ਡੂੰਘਾਈ > 0
                || (m_begin_ਡੂੰਘਾਈ == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        ਜੇ (nodep->isGParam() && m_modp) m_modp->hasGParam(ਸੱਚ);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(nodep->ਬੱਚਾ_DTypep(), BasicDType);
        ਜੇ (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(ਸੱਚ);

        ਜੇ (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_ਕੋਡ::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        ਜੇ (nodep->isParam() && m_ਵਿੱਚ_Interface) nodep->isIfaceParam(ਸੱਚ);
        ਜੇ (AstParseTypeDType* ਅਚਲ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ਗੰਢ* dtypep = nodep->valuep();
            ਜੇ (dtypep) {
                dtypep->unlink_Fr_ਪਿੱਠ();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = VN_CAST(dtypep, ਗੰਢ_ਵਿਅੰਜਕ)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    ਜਦੋਂ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        ਜੇ (!rhsDotp) ਰੋਕੋ;
                        File_ਸਤਰ* ਅਚਲ fl = dotp->fileline();
                        ਅਚਲ ਤਾਰਕਿਕ colon = dotp->colon();
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhs = VN_AS(dotp->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                        File_ਸਤਰ* ਅਚਲ rhsFl = rhsDotp->fileline();
                        ਅਚਲ ਤਾਰਕਿਕ rhsColon = rhsDotp->colon();
                        AstDot* ਅਚਲ newLhs = ਨਵਾਂ AstDot{fl, colon, lhs, rhsLhs};
                        exprp = ਨਵਾਂ AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_ਰੁੱਖ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } ਨਹੀਂਤਾਂ {
                dtypep = ਨਵਾਂ AstVoidDType{nodep->fileline()};
            }

            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlag_ਬੱਚਾ_DType{}, ਨਵਾਂ AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->ਬੱਚਾ_DTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(ਝੂਠ), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        ਜੇ (v3Global.opt.any_ਜਨਤਕ_Flat() && nodep->varType().isVPIAccessible()) {
            ਜੇ (v3Global.opt.ਜਨਤਕ_Flat_RW()) {
                nodep->sigUserRWPublic(ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.ਜਨਤਕ_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (m_modp && v3Global.opt.ਜਨਤਕ_ਡੂੰਘਾਈ()) {
                ਜੇ ((m_modp->ਡੂੰਘਾਈ() - 1) <= v3Global.opt.ਜਨਤਕ_ਡੂੰਘਾਈ()) {
                    nodep->sigUserRWPublic(ਸੱਚ);
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(ਸੱਚ);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        ਜੇ (v3Global.opt.trace_ਡੂੰਘਾਈ() && m_modp
            && (m_modp->ਡੂੰਘਾਈ() - 1) > v3Global.opt.trace_ਡੂੰਘਾਈ()) {
            m_modp->modTrace(ਝੂਠ);
            nodep->trace(ਝੂਠ);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = ਖਾਲੀ_ਸੂਚਕ;
        // temporaries under an always aren't expected to be blocking
        ਜੇ (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::BLKSEQ, ਸੱਚ);
        // Compute initial value
        ਜੇ (nodep->valuep() || nodep->needsCReset()) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            ਸਵੈਚਾਲਿਤ createValuep = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                ਜੇ (nodep->valuep()) {
                    ਵਾਪਸ VN_AS(nodep->valuep()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                } ਨਹੀਂਤਾਂ {
                    ਵਾਪਸ ਨਵਾਂ AstCReset{fl, nodep, ਝੂਠ};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            ਜੇ (m_ਪੋਰਟ_Dups.ਲੱਭੋ(nodep) != m_ਪੋਰਟ_Dups.ਸਿਰਾ()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ਨਿਵੇਸ਼()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                ਜੇ (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                }
            } ਨਹੀਂਤਾਂ ਜੇ (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(ਸੱਚ);
                File_ਸਤਰ* ਅਚਲ newfl = ਨਵਾਂ File_ਸਤਰ{fl};
                newfl->warnOff(V3_Error_ਕੋਡ::E_CONSTWRITTEN, ਸੱਚ);
                Ast_ਸੌਂਪੋ* ਅਚਲ assp
                    = ਨਵਾਂ Ast_ਸੌਂਪੋ{newfl, ਨਵਾਂ AstParseRef{newfl, nodep->name()}, createValuep()};
                ਜੇ (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp, assp);
                } ਨਹੀਂਤਾਂ {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp, assp);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (m_ਮੁੱਲ_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                ਜੇ (nodep->valuep()) {
                    nodep->noCReset(ਸੱਚ);
                    File_ਸਤਰ* ਅਚਲ newfl = ਨਵਾਂ File_ਸਤਰ{fl};
                    newfl->warnOff(V3_Error_ਕੋਡ::PROCASSWIRE, ਸੱਚ);
                    newfl->warnOff(V3_Error_ਕੋਡ::E_CONSTWRITTEN, ਸੱਚ);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_ਸੌਂਪੋ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        newfl, ਨਵਾਂ AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ਗੰਢ* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    ਜੇ (nodep->lifetime().isAutomatic()) {
                        newInitp = ਨਵਾਂ AstInitialAutomatic{newfl, assp};
                    } ਨਹੀਂਤਾਂ {
                        newInitp = ਨਵਾਂ AstInitialStatic{newfl, assp};
                    }
                    nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newInitp);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isNegative(ਅਚਲ AstConst* constp) {
        ਵਾਪਸ constp->isSigned() && constp->ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(constp->ਚੌੜਾਈ() - 1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRange* nodep) override {
        ਸਥਿਰ constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ਅਚਲ AstConst* ਅਚਲ leftp = VN_CAST(nodep->leftp(), Const);
        ਅਚਲ AstConst* ਅਚਲ rightp = VN_CAST(nodep->rightp(), Const);
        ਜੇ (leftp && rightp && leftp->ਚੌੜਾਈ() <= 64 && rightp->ਚੌੜਾਈ() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ਅਚਲ uint64_t ਖੱਬੇ = leftp->ਮੰਜ਼ਿਲ_UQuad();
            ਅਚਲ uint64_t ਸੱਜੇ = rightp->ਮੰਜ਼ਿਲ_UQuad();
            ਅਚਲ uint64_t ਵਿਸਤਾਰ = std::max(ਖੱਬੇ, ਸੱਜੇ) - std::min(ਖੱਬੇ, ਸੱਜੇ);
            ਜੇ (ਵਿਸਤਾਰ >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << ਵਿਸਤਾਰ + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        ਜੇ (nodep->ਅੰਕ().autoExtend() && nodep->fileline()->language() < V3_Lang_ਕੋਡ::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (nodep->attrType() == VAttrType::DT_ਜਨਤਕ) {
            AstTypedef* ਅਚਲ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_ਜਨਤਕ(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->ਸੈੱਟ_Forceable();
            v3Global.ਸੈੱਟ_Has_Forceable_ਸੰਕੇਤ();
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_ਜਨਤਕ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ਸੱਚ);
            m_varp->sig_Mod_ਜਨਤਕ(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_ਜਨਤਕ_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_ਜਨਤਕ_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_ਜਨਤਕ(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_ਜਨਤਕ_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_ਵੰਡੋ_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            ਜੇ (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } ਨਹੀਂਤਾਂ {
                m_varp->attr_ਵੰਡੋ_Var(ਸੱਚ);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_ਭਰਿਆ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::ਭਰਿਆ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_FSM_STATE_ਭਰਿਆ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ਸੱਚ);
            m_varp->attrFsmStateExpand(VFsmExpandType::ਭਰਿਆ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ImplTypedefMap::iterator it = m_implTypedef.ਲੱਭੋ(nodep->name());
        ਜੇ (it != m_implTypedef.ਸਿਰਾ()) {
            defp = it->ਦੂਜਾ;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } ਨਹੀਂਤਾਂ {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ਗੰਢ* backp = nodep->backp();
            ਲਈ (; backp; backp = backp->backp()) {
                ਜੇ (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ਗੰਢ_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ਰੋਕੋ;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->ਬੱਚਾ_DTypep();
            dtypep->unlink_Fr_ਪਿੱਠ();
            ਜੇ (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                defp = ਨਵਾਂ AstTypedef{nodep->fileline(), nodep->name(), ਖਾਲੀ_ਸੂਚਕ, VFlag_ਬੱਚਾ_DType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_ਮੰਜ਼ਿਲ_Str(m_implTypedef.ਮਾਪ()));
                UINFO(9, "New impltypedef " << defp);
                backp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(defp);
            }
        }
        nodep->replaceWith(ਨਵਾਂ AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {
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
        Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp = nodep->headerp();
        ਜੇ (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        cleanFileline(nodep);
        ਜੇ (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } ਨਹੀਂਤਾਂ {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_ਸਤਰ* ਅਚਲ newfl = nodep->fileline();
            newfl->warnOff(V3_Error_ਕੋਡ::WAITCONST, ਸੱਚ);
            nodep->fileline(newfl);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        ਜੇ (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_ਵਿੱਚ_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ਅੰਕ);
        VL_RESTORER(m_begin_ਡੂੰਘਾਈ);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_ਅੰਕ);
        VL_RESTORER(m_ਮੁੱਲ_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        ਜੇ (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        ਜੇ (VN_IS(nodep, Iface)) m_ਵਿੱਚ_Interface = ਸੱਚ;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_ਅੰਕ = 0;
        m_begin_ਡੂੰਘਾਈ = 0;
        m_ਮੁੱਲ_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = ਝੂਠ;
        m_rand_Sequence_ਅੰਕ = 0;

        ਜੇ (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        ਜੇ (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_ਪੋਰਟ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        ਜੇ (AstModule* ਅਚਲ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Iterate_No_ਮੁੱਲ_Mod(Ast_ਗੰਢ* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_ਮੁੱਲ_Modp);
        m_ਮੁੱਲ_Modp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ਸੱਚ;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        ਦੇਖੋ_Iterate_No_ਮੁੱਲ_Mod(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCover* nodep) override { ਦੇਖੋ_Iterate_No_ਮੁੱਲ_Mod(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRestrict* nodep) override { ਦੇਖੋ_Iterate_No_ਮੁੱਲ_Mod(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        V3Control::apply_Coverage_ਖੰਡ(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_ਡੂੰਘਾਈ);
        ++m_begin_ਡੂੰਘਾਈ;
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ਅਚਲ ਤਾਰਕਿਕ nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        ਪੂਰਨ ਸੌਂਪੋ_Gen_Blk_ਅੰਕ = -1;
        ਜੇ (nodep->genforp()) {
            ++m_genblk_ਅੰਕ;
            ਜੇ (nodep->name() == "") ਸੌਂਪੋ_Gen_Blk_ਅੰਕ = m_genblk_ਅੰਕ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "" && (VN_IS(backp, Gen_Case_ਵਸਤੂ) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            ਸੌਂਪੋ_Gen_Blk_ਅੰਕ = m_genblkAbove;
        }
        ਜੇ (ਸੌਂਪੋ_Gen_Blk_ਅੰਕ != -1) {
            nodep->name("genblk" + cvt_ਮੰਜ਼ਿਲ_Str(ਸੌਂਪੋ_Gen_Blk_ਅੰਕ));
            ਜੇ (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_ਹੋਰ()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        ਜੇ (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ਅੰਕ);
            m_genblkAbove = 0;
            m_genblk_ਅੰਕ = 0;
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenCase* nodep) override {
        ++m_genblk_ਅੰਕ;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ਅੰਕ);
        m_genblkAbove = m_genblk_ਅੰਕ;
        m_genblk_ਅੰਕ = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ਅਚਲ ਤਾਰਕਿਕ nestedIf = (VN_IS(nodep->backp(), Gen_ਖੰਡ)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_ਖੰਡ)));
        ਜੇ (nestedIf) {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            ++m_genblk_ਅੰਕ;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ਅੰਕ);
            m_genblkAbove = m_genblk_ਅੰਕ;
            m_genblk_ਅੰਕ = 0;
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        ਜੇ (nodep->origName().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜੇ (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ਅਚਲ string newName = "$unnamedudp" + cvt_ਮੰਜ਼ਿਲ_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਖੰਡ* nodep) override {
        VL_RESTORER(m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp);
        m_ਖੰਡ_ਜੋੜੋ_Automatic_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        VL_RESTORER(m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp);
        m_ਖੰਡ_ਜੋੜੋ_Static_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_ਅਗਲਾ_Null(nodep->declsp());
        }

        ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_ਖੰਡ(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        ਜੇ (AstFork* ਅਚਲ forkp = VN_CAST(nodep, Fork)) iterate_And_ਅਗਲਾ_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        // Move all Vars to front of function
        stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, get_ਖੰਡ_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        ਜੇ (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        ਜੇ (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(ਸੱਚ);
            Ast_ਗੰਢ* ਅਚਲ nextp = nodep->nextp();
            nodep->unlink_Fr_ਪਿੱਠ();
            nextp->ਜੋੜੋ_ਅਗਲਾ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ਗੰਢ* scanp = nodep;
            // Skip over the New's statement
            ਲਈ (; scanp && !VN_IS(scanp, Stmt_ਵਿਅੰਜਕ); scanp = scanp->backp()) {}
            ਜੇ (VN_IS(scanp, Stmt_ਵਿਅੰਜਕ)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                ਲਈ (; scanp; scanp = scanp->backp()) {
                    ਜੇ (VN_IS(scanp, ਗੰਢ_Stmt) || VN_IS(scanp, ਗੰਢ_Module)
                        || VN_IS(scanp, ਗੰਢ_FTask))
                        ਰੋਕੋ;
                }
                ਜੇ (!VN_IS(scanp, ਗੰਢ_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Print_ਸਮਾਂ_Scale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::ਮੰਜ਼ਿਲ_string(m_rand_Sequence_ਅੰਕ++));
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_D* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Import* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Unit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ਖਾਲੀ ਦੇਖੋ(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ਅਚਲ alwaysp = VN_CAST(nodep->backp(), Always);
        ਜੇ (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_ਹੋਰ() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            ਜੇ (Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                alwaysp->sentreep(sentreep);
            }
            ਜੇ (nodep->stmtsp()) alwaysp->ਜੋੜੋ_Stmtsp(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_ਵਿੱਚ_Skewp);
        VL_RESTORER(m_default_ਬਾਹਰਲਾ_Skewp);
        // Find default input and output skews
        ਲਈ (Ast_ਗੰਢ *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ਜੇ (Ast_Clocking_ਵਸਤੂ* citemp = VN_CAST(itemp, Clocking_ਵਸਤੂ)) {
                ਜੇ (citemp->exprp() || citemp->assignp()) ਜਾਰੀ;
                ਜੇ (citemp->skewp()) {
                    ਜੇ (citemp->direction() == VDirection::ਨਿਵੇਸ਼) {
                        // Disallow default redefinition; note some simulators allow this
                        ਜੇ (m_default_ਵਿੱਚ_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_ਵਿੱਚ_Skewp = citemp->skewp();
                    } ਨਹੀਂਤਾਂ ਜੇ (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        ਜੇ (m_default_ਬਾਹਰਲਾ_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_ਬਾਹਰਲਾ_Skewp = citemp->skewp();
                    } ਨਹੀਂਤਾਂ {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ਧੱਕੋ_Deletep(citemp->unlink_Fr_ਪਿੱਠ()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Clocking_ਵਸਤੂ* nodep) override {
        cleanFileline(nodep);
        ਜੇ (nodep->direction() == VDirection::OUTPUT) {
            ਜੇ (!nodep->skewp()) {
                ਜੇ (m_default_ਬਾਹਰਲਾ_Skewp) {
                    nodep->skewp(m_default_ਬਾਹਰਲਾ_Skewp->clone_ਰੁੱਖ(ਝੂਠ));
                } ਨਹੀਂਤਾਂ {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(ਨਵਾਂ AstConst{nodep->fileline(), 0});
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->direction() == VDirection::ਨਿਵੇਸ਼) {
            ਜੇ (!nodep->skewp()) {
                ਜੇ (m_default_ਵਿੱਚ_Skewp) {
                    nodep->skewp(m_default_ਵਿੱਚ_Skewp->clone_ਰੁੱਖ(ਝੂਠ));
                } ਨਹੀਂਤਾਂ {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::One_ਕਦਮ{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        ਜੇ (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    ਖਾਲੀ ਦੇਖੋ(Ast_Tagged_ਵਿਅੰਜਕ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Tagged_ਨਮੂਨਾ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਨਮੂਨਾ_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਨਮੂਨਾ_ਤਾਰਾ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        ਜੇ (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ਸਥਿਰ Ast_ਗੰਢ* makeArgMemberCopies(Ast_ਗੰਢ* argsp, ਤਾਰਕਿਕ readOnlyRefs) {
        Ast_ਗੰਢ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ਅਚਲ origVarp = VN_AS(argp, Var);
            AstVar* ਅਚਲ paramp = origVarp->clone_ਰੁੱਖ(ਝੂਠ);
            File_ਸਤਰ* ਅਚਲ fl = ਨਵਾਂ File_ਸਤਰ{paramp->fileline()};
            fl->warnOff(V3_Error_ਕੋਡ::VARHIDDEN, ਸੱਚ);
            paramp->fileline(fl);
            paramp->funcLocal(ਸੱਚ);
            paramp->direction(origVarp->direction());
            ਜੇ (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(resultp, paramp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstDot{
                origVarp->fileline(), ਝੂਠ, ਨਵਾਂ AstParseRef{origVarp->fileline(), "this"},
                ਨਵਾਂ AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstParseRef{paramp->fileline(), paramp->name()};
            resultp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{origVarp->fileline(), lhsp, rhsp});
        }
        ਵਾਪਸ resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ਸਥਿਰ ਖਾਲੀ createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ਗੰਢ* argsp,
                                        Ast_ਗੰਢ* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ਅਚਲ defaultVarp
            = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->ਲੱਭੋ_Int_DType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->ਜੋੜੋ_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        ਜੇ (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, ਸੱਚ));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlag_ਬੱਚਾ_DType{},
                             ਨਵਾਂ AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             ਨਵਾਂ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ},
                                             ਖਾਲੀ_ਸੂਚਕ}};
            nodep->ਜੋੜੋ_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlag_ਬੱਚਾ_DType{},
                             ਨਵਾਂ AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             ਨਵਾਂ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ},
                                             ਖਾਲੀ_ਸੂਚਕ}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->ਜੋੜੋ_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ਅਚਲ funcp = ਨਵਾਂ AstFunc{nodep->fileline(), "sample", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            funcp->ਜੋੜੋ_Stmtsp(makeArgMemberCopies(sampleArgsp, ਝੂਠ));
            funcp->class_ਵਿਧੀ(ਸੱਚ);
            funcp->dtypep(funcp->ਲੱਭੋ_Void_DType());
            funcp->keepAlive(ਸੱਚ);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->ਜੋੜੋ_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        ਲਈ (ਅਚਲ string& name : {"start"s, "stop"s}) {
            AstFunc* ਅਚਲ funcp = ਨਵਾਂ AstFunc{nodep->fileline(), name, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            funcp->class_ਵਿਧੀ(ਸੱਚ);
            funcp->dtypep(funcp->ਲੱਭੋ_Void_DType());
            nodep->ਜੋੜੋ_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        ਲਈ (ਅਚਲ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ਅਚਲ funcp = ਨਵਾਂ AstFunc{nodep->fileline(), name, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            funcp->fileline()->warnOff(V3_Error_ਕੋਡ::NORETURN, ਸੱਚ);
            funcp->isStatic(name == "get_coverage");
            funcp->class_ਵਿਧੀ(ਸੱਚ);
            funcp->dtypep(funcp->ਲੱਭੋ_Void_DType());
            nodep->ਜੋੜੋ_Membersp(funcp);
            {
                AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->ਲੱਭੋ_Double_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ਸੱਚ);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(ਸੱਚ);
                funcp->fvarp(varp);
            }
            ਲਈ (ਅਚਲ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->ਲੱਭੋ_String_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ਸੱਚ);
                varp->direction(VDirection::ਨਿਵੇਸ਼);
                varp->valuep(ਨਵਾਂ AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->ਜੋੜੋ_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ਅਚਲ funcp
                = ਨਵਾਂ AstFunc{nodep->fileline(), "set_inst_name", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            funcp->class_ਵਿਧੀ(ਸੱਚ);
            funcp->dtypep(funcp->ਲੱਭੋ_Void_DType());
            nodep->ਜੋੜੋ_Membersp(funcp);
            AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->ਲੱਭੋ_String_DType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ਸੱਚ);
            varp->direction(VDirection::ਨਿਵੇਸ਼);
            funcp->ਜੋੜੋ_Stmtsp(varp);
        }
    }

    ਤਾਰਕਿਕ dropDeprecatedCoverageOption(Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ nodep) {
        ਜੇ (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ਵਾਪਸ ਝੂਠ;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Cg_Option_ਸੌਂਪੋ* nodep) override {
        ਜੇ (dropDeprecatedCoverageOption(nodep)) ਵਾਪਸ;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        ਜੇ (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ਵਾਪਸ;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        ਜੇ (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ਅਚਲ string libname = m_modp->libname();
        AstClass* ਅਚਲ cgClassp = ਨਵਾਂ AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
        cgClassp->fileline()->warnOff(V3_Error_ਕੋਡ::UNDRIVEN, ਸੱਚ);
        cgClassp->isCovergroup(ਸੱਚ);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(ਸੱਚ);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        ਜੇ (Ast_Sen_ਰੁੱਖ* ਅਚਲ eventp = nodep->eventp()) {
            eventp->unlink_Fr_ਪਿੱਠ();
            AstCovergroup* ਅਚਲ cgNodep = ਨਵਾਂ AstCovergroup{
                nodep->fileline(), nodep->name(), ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, eventp};
            cgClassp->ਜੋੜੋ_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        ਲਈ (Ast_ਗੰਢ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ਅਚਲ origVarp = VN_AS(argp, Var);
            ਜੇ (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ਨਿਵੇਸ਼);
            }
            ਜੇ ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ਅਚਲ memberp = origVarp->clone_ਰੁੱਖ(ਝੂਠ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ਝੂਠ);
            memberp->direction(VDirection::NONE);
            ਜੇ (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(ਸੱਚ);
            cgClassp->ਜੋੜੋ_Membersp(memberp);
        }

        // Convert sample args to member variables
        ਲਈ (Ast_ਗੰਢ* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ਅਚਲ origVarp = VN_AS(argp, Var);
            ਜੇ (!origVarp->is_ਨਿਵੇਸ਼()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ਨਿਵੇਸ਼);
            }
            AstVar* ਅਚਲ memberp = origVarp->clone_ਰੁੱਖ(ਝੂਠ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ਝੂਠ);
            memberp->direction(VDirection::NONE);
            cgClassp->ਜੋੜੋ_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ਅਚਲ newFuncp = ਨਵਾਂ AstFunc{nodep->fileline(), "new", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        newFuncp->fileline()->warnOff(V3_Error_ਕੋਡ::NORETURN, ਸੱਚ);
        newFuncp->class_ਵਿਧੀ(ਸੱਚ);
        newFuncp->isConstructor(ਸੱਚ);
        newFuncp->dtypep(cgClassp->dtypep());
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            newFuncp->ਜੋੜੋ_Stmtsp(bodyp);
        }
        cgClassp->ਜੋੜੋ_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_ਅੰਕ);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_ਅੰਕ = 0;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstCoverpoint* ਅਚਲ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ਸਥਿਰ string coverpointName(ਅਚਲ AstCoverpoint* nodep) {
        ਜੇ (!nodep->name().ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ਅਚਲ AstParseRef* ਅਚਲ refp = VN_CAST(nodep->exprp(), ParseRef);
        ਵਾਪਸ refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ਅਚਲ AstCoverpoint* ਲੱਭੋ_Coverpoint(ਅਚਲ AstCoverpointRef* refp) ਅਚਲ {
        ਜੇ (refp->exprp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // A hierarchical reference, unsupported
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_coverpoints.ਲੱਭੋ(refp->name());
        ਵਾਪਸ it == m_coverpoints.ਸਿਰਾ() ? ਖਾਲੀ_ਸੂਚਕ : it->ਦੂਜਾ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        ਜੇ (nodep->name().ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ਅਚਲ string name = coverpointName(nodep);
            nodep->name(name.ਖਾਲੀ_ਥਾਵਾਂ() ? "__Vcoverpoint" + cvt_ਮੰਜ਼ਿਲ_Str(m_coverpoint_ਅੰਕ++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        ਲਈ (Ast_ਗੰਢ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ਜੇ (Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ optp = VN_CAST(itemp, Cg_Option_ਸੌਂਪੋ)) {
                ਜੇ (dropDeprecatedCoverageOption(optp)) ਜਾਰੀ;
                optp->unlink_Fr_ਪਿੱਠ();
                ਜੇ (optp->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::ਭਾਰ) {
                    nodep->ਜੋੜੋ_Optionsp(ਨਵਾਂ AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_ਰੁੱਖ(ਝੂਠ)});
                } ਨਹੀਂਤਾਂ {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_ਰੁੱਖ(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        ਜੇ (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        ਜੇ (ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            ਜੇ (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        File_ਸਤਰ* ਅਚਲ flNoWarn = ਨਵਾਂ File_ਸਤਰ{fl};
        flNoWarn->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
        flNoWarn->modifyWarnOff(V3_Error_ਕੋਡ::VARHIDDEN, ਸੱਚ);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flNoWarn, VVarType::VAR, "item", VFlag_ਬੱਚਾ_DType{},
                                        ਨਵਾਂ AstRefDType{fl, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{},
                                                        m_coverpointp->exprp()->clone_ਰੁੱਖ(ਝੂਠ)}};
        varp->funcLocal(ਸੱਚ);
        varp->noReset(ਸੱਚ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ਅਚਲ refp = nodep->pointp();
        ਅਚਲ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ਅਚਲ AstParseRef* binp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstDot* ਅਚਲ dotp = VN_CAST(refp->exprp(), Dot);
        ਜੇ (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        ਜੇ (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        ਜੇ (binp) nodep->name(binp->name());
        refp->replaceWith(ਨਵਾਂ AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ਜੇ (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        ਲਈ (Ast_ਗੰਢ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ਜੇ (VN_IS(itemp, CoverCrossBin)) ਜਾਰੀ;
            Ast_Cg_Option_ਸੌਂਪੋ* ਅਚਲ optp = VN_AS(itemp, Cg_Option_ਸੌਂਪੋ);
            ਜੇ (dropDeprecatedCoverageOption(optp)) ਜਾਰੀ;
            itemp->unlink_Fr_ਪਿੱਠ();
            ਅਚਲ VCoverOptionType optType = optp->optType();
            ਜੇ (!(optType == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ || optType == VCoverOptionType::ਭਾਰ)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->ਜੋੜੋ_Optionsp(ਨਵਾਂ AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_ਰੁੱਖ(ਝੂਠ)});
            VL_DO_DANGLING(optp->delete_ਰੁੱਖ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        ਜੇ (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            File_ਸਤਰ* ਅਚਲ flNoWarn = ਨਵਾਂ File_ਸਤਰ{fl};
            flNoWarn->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
            flNoWarn->modifyWarnOff(V3_Error_ਕੋਡ::VARHIDDEN, ਸੱਚ);
            std::ਸੈੱਟ<string> ਨਾਂ;
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ਅਚਲ AstCoverpointRef* ਅਚਲ refp = VN_AS(itemp, CoverpointRef);
                ਅਚਲ AstCoverpoint* ਅਚਲ pointp = ਲੱਭੋ_Coverpoint(refp);
                ਜੇ (!pointp || !ਨਾਂ.ਪਾਓ(refp->name()).ਦੂਜਾ) ਜਾਰੀ;
                AstVar* ਅਚਲ varp
                    = ਨਵਾਂ AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlag_ਬੱਚਾ_DType{},
                                 ਨਵਾਂ AstRefDType{fl, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{},
                                                 pointp->exprp()->clone_ਰੁੱਖ(ਝੂਠ)}};
                varp->funcLocal(ਸੱਚ);
                varp->noReset(ਸੱਚ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->ਜੋੜੋ_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        ਜੇ (!nodep->selectp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstSystemCSection* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process only once.
        cleanFileline(nodep);
        m_modp->ਜੋੜੋ_Sc_Sectionsp(nodep->unlink_Fr_ਪਿੱਠ());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ(V3Stats::STAT_ਸਰੋਤ_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

ਖਾਲੀ V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linkparse", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
