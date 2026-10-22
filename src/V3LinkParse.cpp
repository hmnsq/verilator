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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // TYPES
    ಬಳಸಿ ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_ಗಣ<File_ಸಾಲು*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = ಶೂನ್ಯಸೂಚಕ;  // Variable we're under
    Ast_ಗಂಟು_Module* m_ಮೌಲ್ಯ_Modp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ತಾರ್ಕಿಕ m_ಒಳ_Interface = ಸುಳ್ಳು;  // True when inside interface declaration
    Ast_ಗಂಟು_Procedure* m_procedurep = ಶೂನ್ಯಸೂಚಕ;  // Current procedure
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current task
    AstRSProd* m_rsProdp = ಶೂನ್ಯಸೂಚಕ;  // Current randsequence production
    Ast_ಗಂಟು_ಖಂಡ* m_blockp = ಶೂನ್ಯಸೂಚಕ;  // Current AstNodeBlock
    Ast_ಗಂಟು_Stmt* m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp = ಶೂನ್ಯಸೂಚಕ;  // Initial statements to add to block
    Ast_ಗಂಟು_Stmt* m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp = ಶೂನ್ಯಸೂಚಕ;  // Initial statements to add to block
    Ast_ಗಂಟು_DType* m_dtypep = ಶೂನ್ಯಸೂಚಕ;  // Current data type
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_default_ಒಳ_Skewp = ಶೂನ್ಯಸೂಚಕ;  // Current default input skew
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_default_ಹೊರಹೋಗು_Skewp = ಶೂನ್ಯಸೂಚಕ;  // Current default output skew
    AstCoverpoint* m_coverpointp = ಶೂನ್ಯಸೂಚಕ;  // Current coverpoint
    AstCoverCross* m_crossp = ಶೂನ್ಯಸೂಚಕ;  // Current cross
    std::map<string, ಸ್ಥಿರಾಂಕ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    ಪೂರ್ಣಾಂಕ m_anonUdpId = 0;  // Counter for anonymous UDP instances
    ಪೂರ್ಣಾಂಕ m_coverpoint_ಅಂಕೆ = 0;  // Counter for unnamed coverpoints within current covergroup
    ಪೂರ್ಣಾಂಕ m_genblkAbove = 0;  // Begin block number of if/case/for above
    ಪೂರ್ಣಾಂಕ m_genblk_ಅಂಕೆ = 0;  // Begin block number, 0=none seen
    ಪೂರ್ಣಾಂಕ m_begin_ಆಳ = 0;  // How many begin blocks above current node within current AstNodeModule
    ಪೂರ್ಣಾಂಕ m_rand_Sequence_ಅಂಕೆ = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    ತಾರ್ಕಿಕ m_lifetimeAllowed = ಸುಳ್ಳು;  // True to allow lifetime settings
    ತಾರ್ಕಿಕ m_hasTimingControl = ಸುಳ್ಳು;  // If current task has timing control
    ತಾರ್ಕಿಕ m_moduleWithGenericIface = ಸುಳ್ಳು;  // If current module contains generic interface
    std::ಗಣ<AstVar*> m_ಪೋರ್ಟ್_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    ಶೂನ್ಯ cleanFileline(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep->user2_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        ಆದರೆ (m_filelines.ಕಂಡುಹಿಡಿ(nodep->fileline()) != m_filelines.ಕೊನೆ()) {
            nodep->fileline(ಹೊಸ File_ಸಾಲು{nodep->fileline()});
        }
        m_filelines.ಸೇರಿಸು(nodep->fileline());
    }

    string nameFromTypedef(Ast_ಗಂಟು* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            ಆದರೆ (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_modp) {
                above = m_modp->name() + ".";
            }
            ಹಿಂತಿರುಗಿ above + typedefp->name();
        }
        ಹಿಂತಿರುಗಿ "";
    }

    ಶೂನ್ಯ ಭೇಟಿ_Iterate_ಗಂಟು_DType(Ast_ಗಂಟು_DType* nodep) {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    ತಾರ್ಕಿಕ nestedIfBegin(Ast_Gen_ಖಂಡ* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        ಹಿಂತಿರುಗಿ (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, Gen_Case_ಅಂಶ))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    ಶೂನ್ಯ checkIndent(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* childp) {
        // Try very hard to avoid false positives
        Ast_ಗಂಟು* nextp = nodep->nextp();
        ಆದರೆ (!childp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        ಆದರೆ (!nextp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (VN_IS(childp, Begin) || VN_IS(childp, Gen_ಖಂಡ)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ ಗಂಟು_Flp = nodep->fileline();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ ಮಗು_Flp = childp->fileline();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        ಆದರೆ (ಗಂಟು_Flp->filenameno() != ಮಗು_Flp->filenameno()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಗಂಟು_Flp->filenameno() != ಮುಂದಿನ_Flp->filenameno()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಗಂಟು_Flp->ಕೊನೆಯ_Lineno() >= ಮಗು_Flp->ಮೊದಲ_Lineno()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಮಗು_Flp->ಕೊನೆಯ_Lineno() >= ಮುಂದಿನ_Flp->ಮೊದಲ_Lineno()) ಹಿಂತಿರುಗಿ;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        ಆದರೆ (!(ಗಂಟು_Flp->ಮೊದಲ_ಸ್ತಂಭ() < ಮಗು_Flp->ಮೊದಲ_ಸ್ತಂಭ()
              && ಮುಂದಿನ_Flp->ಮೊದಲ_ಸ್ತಂಭ() >= ಮಗು_Flp->ಮೊದಲ_ಸ್ತಂಭ()))
            ಹಿಂತಿರುಗಿ;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ಸ್ಥಿರಾಂಕ std::string ಗಂಟು_ಪೂರ್ವಪ್ರತ್ಯಯ = ಗಂಟು_Flp->ಮೂಲ_ಪೂರ್ವಪ್ರತ್ಯಯ(ಗಂಟು_Flp->ಮೊದಲ_ಸ್ತಂಭ());
        ಸ್ಥಿರಾಂಕ std::string ಮಗು_ಪೂರ್ವಪ್ರತ್ಯಯ = ಮಗು_Flp->ಮೂಲ_ಪೂರ್ವಪ್ರತ್ಯಯ(ಗಂಟು_Flp->ಮೊದಲ_ಸ್ತಂಭ());
        ಸ್ಥಿರಾಂಕ std::string ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ = ಮುಂದಿನ_Flp->ಮೂಲ_ಪೂರ್ವಪ್ರತ್ಯಯ(ಗಂಟು_Flp->ಮೊದಲ_ಸ್ತಂಭ());
        ಆದರೆ (ಮಗು_ಪೂರ್ವಪ್ರತ್ಯಯ != ಗಂಟು_ಪೂರ್ವಪ್ರತ್ಯಯ) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ != ಮಗು_ಪೂರ್ವಪ್ರತ್ಯಯ) ಹಿಂತಿರುಗಿ;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ಸ್ಥಿರಾಂಕ std::string ಗಂಟು_ಮೂಲ = ಗಂಟು_Flp->ಮೂಲ();
        ಸ್ಥಿರಾಂಕ std::string ಮಗು_ಮೂಲ = ಮಗು_Flp->ಮೂಲ();
        ಸ್ಥಿರಾಂಕ std::string ಮುಂದಿನ_ಮೂಲ = ಮುಂದಿನ_Flp->ಮೂಲ();
        ಆದರೆ (!(VString::leading_Whitespace_ಎಣಿಕೆ(ಗಂಟು_ಮೂಲ)
                  < VString::leading_Whitespace_ಎಣಿಕೆ(ಮಗು_ಮೂಲ)
              && VString::leading_Whitespace_ಎಣಿಕೆ(ಮುಂದಿನ_ಮೂಲ)
                     >= VString::leading_Whitespace_ಎಣಿಕೆ(ಮಗು_ಮೂಲ)))
            ಹಿಂತಿರುಗಿ;
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
        std::unordered_map<std::string, AstVar*> ಪೋರ್ಟ್_ಹೆಸರುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = ಗಂಟು_Listp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isIO()) ಪೋರ್ಟ್_ಹೆಸರುಗಳು.emplace(varp->name(), varp);
            }
        }
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = ಗಂಟು_Listp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isIO()) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ಪೋರ್ಟ್_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(varp->name());
                ಆದರೆ (it == ಪೋರ್ಟ್_ಹೆಸರುಗಳು.ಕೊನೆ()) ಮುಂದುವರಿ;
                AstVar* ಸ್ಥಿರಾಂಕ iop = it->ಎರಡನೇ;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                ಆದರೆ (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                m_ಪೋರ್ಟ್_Dups.emplace(varp);
            }
        }
    }

    Ast_ಗಂಟು* getVarsAndUnlink(Ast_ಗಂಟು* stmtsp) {
        Ast_ಗಂಟು* varsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಆದರೆ (VN_IS(itemp, Var)) varsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(varsp, itemp->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಹಿಂತಿರುಗಿ varsp;
    }
    Ast_ಗಂಟು_Stmt* get_ಖಂಡ_Adds() {
        Ast_ಗಂಟು_Stmt* addsp = ಶೂನ್ಯಸೂಚಕ;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        ಆದರೆ (m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp)
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp,
                                     ಹೊಸ AstInitialStaticStmt{m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp->fileline(),
                                                              m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp});
        ಆದರೆ (m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp)
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                addsp, ಹೊಸ AstInitialAutomaticStmt{m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp->fileline(),
                                                   m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp});
        m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ addsp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process only once.
        // Mark class methods
        ಆದರೆ (VN_IS(m_modp, Class)) nodep->class_ವಿಧಾನ(ನಿಜ);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp);
        m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        VL_RESTORER(m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp);
        m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ನಿಜ;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) { ಹಿಂತಿರುಗಿ nodep->isTimingControl(); });
        ಆದರೆ (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->class_ವಿಧಾನ()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        ಆದರೆ (nodep->class_ವಿಧಾನ() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_ಇನ್ನಷ್ಟು() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_ಪೋರ್ಟ್_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Move all Vars to front of function
        stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, get_ಖಂಡ_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType* nodep) override { ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(ನಿಜ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintForeach* nodep) override {
        ಆದರೆ (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* exprp) { exprp->isSoft(ನಿಜ); });
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override {
        ಆದರೆ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಆದರೆ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (nodep->rangep()) {
            ಆದರೆ (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t ಸ್ಥಾನಾಂತರ_from_init = 0;
            Ast_Enum_ಅಂಶ* addp = ಶೂನ್ಯಸೂಚಕ;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i : nodep->rangep()->seq_ಎಡ_ಗಮ್ಯ_ಬಲ()) {
                ಸ್ಥಿರಾಂಕ string name = nodep->name() + cvt_ಗಮ್ಯ_Str(i);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = ಹೊಸ Ast_ಕೂಡಿಸು{flp, nodep->valuep()->clone_ವೃಕ್ಷ(ನಿಜ),
                                     ಹೊಸ AstConst{flp, AstConst::Unsized32{}, ಸ್ಥಾನಾಂತರ_from_init}};
                }
                addp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addp, ಹೊಸ Ast_Enum_ಅಂಶ{flp, name, ಶೂನ್ಯಸೂಚಕ, valuep});
                ++ಸ್ಥಾನಾಂತರ_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class);
        ಆದರೆ (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(ನಿಜ);
        }
        ಆದರೆ (nodep->embeddedCovergroup()) {
            AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_AS(nodep->ಮಗು_DTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        ಆದರೆ (nodep->valuep()) nodep->hasUserInit(ನಿಜ);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        ಆದರೆ (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            ಆದರೆ (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_ಇನ್ನಷ್ಟು() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_ಇನ್ನಷ್ಟು() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_ಇನ್ನಷ್ಟು() << "... The initializer value will only be set once\n"
                        << nodep->warn_ಇನ್ನಷ್ಟು()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        ಆದರೆ (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        ಆದರೆ (!nodep->direction().isAny()) {  // Not a port
            ಆದರೆ (nodep->lifetime().isNone()) {
                ಆದರೆ (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } ಇಲ್ಲದಿದ್ದರೆ {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            ಆದರೆ (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        ಆದರೆ (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            ಆದರೆ (m_begin_ಆಳ > 0
                || (m_begin_ಆಳ == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        ಆದರೆ (nodep->isGParam() && m_modp) m_modp->hasGParam(ನಿಜ);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(nodep->ಮಗು_DTypep(), BasicDType);
        ಆದರೆ (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(ನಿಜ);

        ಆದರೆ (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_ಸಂಕೇತ::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        ಆದರೆ (nodep->isParam() && m_ಒಳ_Interface) nodep->isIfaceParam(ನಿಜ);
        ಆದರೆ (AstParseTypeDType* ಸ್ಥಿರಾಂಕ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ಗಂಟು* dtypep = nodep->valuep();
            ಆದರೆ (dtypep) {
                dtypep->unlink_Fr_ಹಿಂಭಾಗ();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_CAST(dtypep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    ಆಗ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        ಆದರೆ (!rhsDotp) ನಿಲ್ಲಿಸು;
                        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = dotp->fileline();
                        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ colon = dotp->colon();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhs = VN_AS(dotp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ rhsFl = rhsDotp->fileline();
                        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsColon = rhsDotp->colon();
                        AstDot* ಸ್ಥಿರಾಂಕ newLhs = ಹೊಸ AstDot{fl, colon, lhs, rhsLhs};
                        exprp = ಹೊಸ AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                dtypep = ಹೊಸ AstVoidDType{nodep->fileline()};
            }

            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlag_ಮಗು_DType{}, ಹೊಸ AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->ಮಗು_DTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(ಸುಳ್ಳು), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        ಆದರೆ (v3Global.opt.any_ಬಹಿರಂಗ_Flat() && nodep->varType().isVPIAccessible()) {
            ಆದರೆ (v3Global.opt.ಬಹಿರಂಗ_Flat_RW()) {
                nodep->sigUserRWPublic(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.ಬಹಿರಂಗ_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_modp && v3Global.opt.ಬಹಿರಂಗ_ಆಳ()) {
                ಆದರೆ ((m_modp->ಆಳ() - 1) <= v3Global.opt.ಬಹಿರಂಗ_ಆಳ()) {
                    nodep->sigUserRWPublic(ನಿಜ);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(ನಿಜ);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        ಆದರೆ (v3Global.opt.trace_ಆಳ() && m_modp
            && (m_modp->ಆಳ() - 1) > v3Global.opt.trace_ಆಳ()) {
            m_modp->modTrace(ಸುಳ್ಳು);
            nodep->trace(ಸುಳ್ಳು);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = ಶೂನ್ಯಸೂಚಕ;
        // temporaries under an always aren't expected to be blocking
        ಆದರೆ (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::BLKSEQ, ನಿಜ);
        // Compute initial value
        ಆದರೆ (nodep->valuep() || nodep->needsCReset()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            ಸ್ವಯಂಚಾಲಿತ createValuep = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಆದರೆ (nodep->valuep()) {
                    ಹಿಂತಿರುಗಿ VN_AS(nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಹಿಂತಿರುಗಿ ಹೊಸ AstCReset{fl, nodep, ಸುಳ್ಳು};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            ಆದರೆ (m_ಪೋರ್ಟ್_Dups.ಕಂಡುಹಿಡಿ(nodep) != m_ಪೋರ್ಟ್_Dups.ಕೊನೆ()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ಒಳಹರಿವು()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                ಆದರೆ (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(ನಿಜ);
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newfl = ಹೊಸ File_ಸಾಲು{fl};
                newfl->warnOff(V3_Error_ಸಂಕೇತ::E_CONSTWRITTEN, ನಿಜ);
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp
                    = ಹೊಸ Ast_ನಿಯೋಜಿಸು{newfl, ಹೊಸ AstParseRef{newfl, nodep->name()}, createValuep()};
                ಆದರೆ (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp, assp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp, assp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಮೌಲ್ಯ_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                ಆದರೆ (nodep->valuep()) {
                    nodep->noCReset(ನಿಜ);
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newfl = ಹೊಸ File_ಸಾಲು{fl};
                    newfl->warnOff(V3_Error_ಸಂಕೇತ::PROCASSWIRE, ನಿಜ);
                    newfl->warnOff(V3_Error_ಸಂಕೇತ::E_CONSTWRITTEN, ನಿಜ);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        newfl, ಹೊಸ AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ಗಂಟು* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    ಆದರೆ (nodep->lifetime().isAutomatic()) {
                        newInitp = ಹೊಸ AstInitialAutomatic{newfl, assp};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newInitp = ಹೊಸ AstInitialStatic{newfl, assp};
                    }
                    nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newInitp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isNegative(ಸ್ಥಿರಾಂಕ AstConst* constp) {
        ಹಿಂತಿರುಗಿ constp->isSigned() && constp->ಅಂಕೆ().ಬಿಟ್_Is1(constp->ಅಗಲ() - 1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRange* nodep) override {
        ಸ್ಥಿರ constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ leftp = VN_CAST(nodep->leftp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ rightp = VN_CAST(nodep->rightp(), Const);
        ಆದರೆ (leftp && rightp && leftp->ಅಗಲ() <= 64 && rightp->ಅಗಲ() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ಸ್ಥಿರಾಂಕ uint64_t ಎಡ = leftp->ಗಮ್ಯ_UQuad();
            ಸ್ಥಿರಾಂಕ uint64_t ಬಲ = rightp->ಗಮ್ಯ_UQuad();
            ಸ್ಥಿರಾಂಕ uint64_t ಹರವು = std::max(ಎಡ, ಬಲ) - std::min(ಎಡ, ಬಲ);
            ಆದರೆ (ಹರವು >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << ಹರವು + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        ಆದರೆ (nodep->ಅಂಕೆ().autoExtend() && nodep->fileline()->language() < V3_Lang_ಸಂಕೇತ::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (nodep->attrType() == VAttrType::DT_ಬಹಿರಂಗ) {
            AstTypedef* ಸ್ಥಿರಾಂಕ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_ಬಹಿರಂಗ(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->ಗಣ_Forceable();
            v3Global.ಗಣ_Has_Forceable_ಸಂಕೇತಗಳು();
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_ಬಹಿರಂಗ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ನಿಜ);
            m_varp->sig_Mod_ಬಹಿರಂಗ(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_ಬಹಿರಂಗ_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_ಬಹಿರಂಗ_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_ಬಹಿರಂಗ(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_ಬಹಿರಂಗ_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_ವಿಭಜಿಸು_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            ಆದರೆ (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_varp->attr_ವಿಭಜಿಸು_Var(ನಿಜ);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_ತುಂಬಿದ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::ತುಂಬಿದ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_FSM_STATE_ತುಂಬಿದ) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ನಿಜ);
            m_varp->attrFsmStateExpand(VFsmExpandType::ತುಂಬಿದ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ImplTypedefMap::iterator it = m_implTypedef.ಕಂಡುಹಿಡಿ(nodep->name());
        ಆದರೆ (it != m_implTypedef.ಕೊನೆ()) {
            defp = it->ಎರಡನೇ;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ಗಂಟು* backp = nodep->backp();
            ಗಾಗಿ (; backp; backp = backp->backp()) {
                ಆದರೆ (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ಗಂಟು_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ನಿಲ್ಲಿಸು;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->ಮಗು_DTypep();
            dtypep->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                defp = ಹೊಸ AstTypedef{nodep->fileline(), nodep->name(), ಶೂನ್ಯಸೂಚಕ, VFlag_ಮಗು_DType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_ಗಮ್ಯ_Str(m_implTypedef.ಗಾತ್ರ_2()));
                UINFO(9, "New impltypedef " << defp);
                backp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(defp);
            }
        }
        nodep->replaceWith(ಹೊಸ AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {
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
        Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp = nodep->headerp();
        ಆದರೆ (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        cleanFileline(nodep);
        ಆದರೆ (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newfl = nodep->fileline();
            newfl->warnOff(V3_Error_ಸಂಕೇತ::WAITCONST, ನಿಜ);
            nodep->fileline(newfl);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        ಆದರೆ (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_ಒಳ_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ಅಂಕೆ);
        VL_RESTORER(m_begin_ಆಳ);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_ಅಂಕೆ);
        VL_RESTORER(m_ಮೌಲ್ಯ_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        ಆದರೆ (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        ಆದರೆ (VN_IS(nodep, Iface)) m_ಒಳ_Interface = ನಿಜ;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_ಅಂಕೆ = 0;
        m_begin_ಆಳ = 0;
        m_ಮೌಲ್ಯ_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = ಸುಳ್ಳು;
        m_rand_Sequence_ಅಂಕೆ = 0;

        ಆದರೆ (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        ಆದರೆ (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_ಪೋರ್ಟ್_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        ಆದರೆ (AstModule* ಸ್ಥಿರಾಂಕ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Iterate_No_ಮೌಲ್ಯ_Mod(Ast_ಗಂಟು* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_ಮೌಲ್ಯ_Modp);
        m_ಮೌಲ್ಯ_Modp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ನಿಜ;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        ಭೇಟಿ_Iterate_No_ಮೌಲ್ಯ_Mod(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCover* nodep) override { ಭೇಟಿ_Iterate_No_ಮೌಲ್ಯ_Mod(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRestrict* nodep) override { ಭೇಟಿ_Iterate_No_ಮೌಲ್ಯ_Mod(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        V3Control::apply_Coverage_ಖಂಡ(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_ಆಳ);
        ++m_begin_ಆಳ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        ಪೂರ್ಣಾಂಕ ನಿಯೋಜಿಸು_Gen_Blk_ಅಂಕೆ = -1;
        ಆದರೆ (nodep->genforp()) {
            ++m_genblk_ಅಂಕೆ;
            ಆದರೆ (nodep->name() == "") ನಿಯೋಜಿಸು_Gen_Blk_ಅಂಕೆ = m_genblk_ಅಂಕೆ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "" && (VN_IS(backp, Gen_Case_ಅಂಶ) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            ನಿಯೋಜಿಸು_Gen_Blk_ಅಂಕೆ = m_genblkAbove;
        }
        ಆದರೆ (ನಿಯೋಜಿಸು_Gen_Blk_ಅಂಕೆ != -1) {
            nodep->name("genblk" + cvt_ಗಮ್ಯ_Str(ನಿಯೋಜಿಸು_Gen_Blk_ಅಂಕೆ));
            ಆದರೆ (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_ಇನ್ನಷ್ಟು()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        ಆದರೆ (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ಅಂಕೆ);
            m_genblkAbove = 0;
            m_genblk_ಅಂಕೆ = 0;
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenCase* nodep) override {
        ++m_genblk_ಅಂಕೆ;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ಅಂಕೆ);
        m_genblkAbove = m_genblk_ಅಂಕೆ;
        m_genblk_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ nestedIf = (VN_IS(nodep->backp(), Gen_ಖಂಡ)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_ಖಂಡ)));
        ಆದರೆ (nestedIf) {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ++m_genblk_ಅಂಕೆ;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ಅಂಕೆ);
            m_genblkAbove = m_genblk_ಅಂಕೆ;
            m_genblk_ಅಂಕೆ = 0;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        ಆದರೆ (nodep->origName().ಖಾಲಿಗಳು()) {
            ಆದರೆ (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ಸ್ಥಿರಾಂಕ string newName = "$unnamedudp" + cvt_ಗಮ್ಯ_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಖಂಡ* nodep) override {
        VL_RESTORER(m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp);
        m_ಖಂಡ_ಕೂಡಿಸು_Automatic_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        VL_RESTORER(m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp);
        m_ಖಂಡ_ಕೂಡಿಸು_Static_Stmtp = ಶೂನ್ಯಸೂಚಕ;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_ಮುಂದಿನ_Null(nodep->declsp());
        }

        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_ಖಂಡ(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        ಆದರೆ (AstFork* ಸ್ಥಿರಾಂಕ forkp = VN_CAST(nodep, Fork)) iterate_And_ಮುಂದಿನ_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        // Move all Vars to front of function
        stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, get_ಖಂಡ_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        ಆದರೆ (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        ಆದರೆ (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(ನಿಜ);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = nodep->nextp();
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            nextp->ಕೂಡಿಸು_ಮುಂದಿನ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ಗಂಟು* scanp = nodep;
            // Skip over the New's statement
            ಗಾಗಿ (; scanp && !VN_IS(scanp, Stmt_ಅಭಿವ್ಯಕ್ತಿ); scanp = scanp->backp()) {}
            ಆದರೆ (VN_IS(scanp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                ಗಾಗಿ (; scanp; scanp = scanp->backp()) {
                    ಆದರೆ (VN_IS(scanp, ಗಂಟು_Stmt) || VN_IS(scanp, ಗಂಟು_Module)
                        || VN_IS(scanp, ಗಂಟು_FTask))
                        ನಿಲ್ಲಿಸು;
                }
                ಆದರೆ (!VN_IS(scanp, ಗಂಟು_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Print_ಸಮಯ_Scale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::ಗಮ್ಯ_string(m_rand_Sequence_ಅಂಕೆ++));
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_D* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Import* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಮಯ_Unit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_CAST(nodep->backp(), Always);
        ಆದರೆ (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_ಇನ್ನಷ್ಟು() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            ಆದರೆ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                alwaysp->sentreep(sentreep);
            }
            ಆದರೆ (nodep->stmtsp()) alwaysp->ಕೂಡಿಸು_Stmtsp(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_ಒಳ_Skewp);
        VL_RESTORER(m_default_ಹೊರಹೋಗು_Skewp);
        // Find default input and output skews
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಆದರೆ (Ast_Clocking_ಅಂಶ* citemp = VN_CAST(itemp, Clocking_ಅಂಶ)) {
                ಆದರೆ (citemp->exprp() || citemp->assignp()) ಮುಂದುವರಿ;
                ಆದರೆ (citemp->skewp()) {
                    ಆದರೆ (citemp->direction() == VDirection::ಒಳಹರಿವು) {
                        // Disallow default redefinition; note some simulators allow this
                        ಆದರೆ (m_default_ಒಳ_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_ಒಳ_Skewp = citemp->skewp();
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        ಆದರೆ (m_default_ಹೊರಹೋಗು_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_ಹೊರಹೋಗು_Skewp = citemp->skewp();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(citemp->unlink_Fr_ಹಿಂಭಾಗ()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Clocking_ಅಂಶ* nodep) override {
        cleanFileline(nodep);
        ಆದರೆ (nodep->direction() == VDirection::OUTPUT) {
            ಆದರೆ (!nodep->skewp()) {
                ಆದರೆ (m_default_ಹೊರಹೋಗು_Skewp) {
                    nodep->skewp(m_default_ಹೊರಹೋಗು_Skewp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(ಹೊಸ AstConst{nodep->fileline(), 0});
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            ಆದರೆ (!nodep->skewp()) {
                ಆದರೆ (m_default_ಒಳ_Skewp) {
                    nodep->skewp(m_default_ಒಳ_Skewp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(ಹೊಸ AstConst{nodep->fileline(), AstConst::One_ಹೆಜ್ಜೆ{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        ಆದರೆ (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Tagged_ನಮೂನೆ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಮೂನೆ_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಮೂನೆ_ನಕ್ಷತ್ರ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        ಆದರೆ (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
        Ast_ಗಂಟು* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ origVarp = VN_AS(argp, Var);
            AstVar* ಸ್ಥಿರಾಂಕ paramp = origVarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಹೊಸ File_ಸಾಲು{paramp->fileline()};
            fl->warnOff(V3_Error_ಸಂಕೇತ::VARHIDDEN, ನಿಜ);
            paramp->fileline(fl);
            paramp->funcLocal(ನಿಜ);
            paramp->direction(origVarp->direction());
            ಆದರೆ (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(resultp, paramp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstDot{
                origVarp->fileline(), ಸುಳ್ಳು, ಹೊಸ AstParseRef{origVarp->fileline(), "this"},
                ಹೊಸ AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstParseRef{paramp->fileline(), paramp->name()};
            resultp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{origVarp->fileline(), lhsp, rhsp});
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ಸ್ಥಿರ ಶೂನ್ಯ createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ಗಂಟು* argsp,
                                        Ast_ಗಂಟು* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ಸ್ಥಿರಾಂಕ defaultVarp
            = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->ಕಂಡುಹಿಡಿ_Int_DType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->ಕೂಡಿಸು_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        ಆದರೆ (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, ನಿಜ));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlag_ಮಗು_DType{},
                             ಹೊಸ AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             ಹೊಸ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ},
                                             ಶೂನ್ಯಸೂಚಕ}};
            nodep->ಕೂಡಿಸು_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlag_ಮಗು_DType{},
                             ಹೊಸ AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             ಹೊಸ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ},
                                             ಶೂನ್ಯಸೂಚಕ}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->ಕೂಡಿಸು_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstFunc{nodep->fileline(), "sample", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            funcp->ಕೂಡಿಸು_Stmtsp(makeArgMemberCopies(sampleArgsp, ಸುಳ್ಳು));
            funcp->class_ವಿಧಾನ(ನಿಜ);
            funcp->dtypep(funcp->ಕಂಡುಹಿಡಿ_Void_DType());
            funcp->keepAlive(ನಿಜ);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->ಕೂಡಿಸು_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& name : {"start"s, "stop"s}) {
            AstFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstFunc{nodep->fileline(), name, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            funcp->class_ವಿಧಾನ(ನಿಜ);
            funcp->dtypep(funcp->ಕಂಡುಹಿಡಿ_Void_DType());
            nodep->ಕೂಡಿಸು_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstFunc{nodep->fileline(), name, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            funcp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::NORETURN, ನಿಜ);
            funcp->isStatic(name == "get_coverage");
            funcp->class_ವಿಧಾನ(ನಿಜ);
            funcp->dtypep(funcp->ಕಂಡುಹಿಡಿ_Void_DType());
            nodep->ಕೂಡಿಸು_Membersp(funcp);
            {
                AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->ಕಂಡುಹಿಡಿ_Double_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ನಿಜ);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(ನಿಜ);
                funcp->fvarp(varp);
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->ಕಂಡುಹಿಡಿ_String_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ನಿಜ);
                varp->direction(VDirection::ಒಳಹರಿವು);
                varp->valuep(ಹೊಸ AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->ಕೂಡಿಸು_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ಸ್ಥಿರಾಂಕ funcp
                = ಹೊಸ AstFunc{nodep->fileline(), "set_inst_name", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            funcp->class_ವಿಧಾನ(ನಿಜ);
            funcp->dtypep(funcp->ಕಂಡುಹಿಡಿ_Void_DType());
            nodep->ಕೂಡಿಸು_Membersp(funcp);
            AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->ಕಂಡುಹಿಡಿ_String_DType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ನಿಜ);
            varp->direction(VDirection::ಒಳಹರಿವು);
            funcp->ಕೂಡಿಸು_Stmtsp(varp);
        }
    }

    ತಾರ್ಕಿಕ dropDeprecatedCoverageOption(Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cg_Option_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (dropDeprecatedCoverageOption(nodep)) ಹಿಂತಿರುಗಿ;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        ಆದರೆ (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ಹಿಂತಿರುಗಿ;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        ಆದರೆ (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ಸ್ಥಿರಾಂಕ string libname = m_modp->libname();
        AstClass* ಸ್ಥಿರಾಂಕ cgClassp = ಹೊಸ AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
        cgClassp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::UNDRIVEN, ನಿಜ);
        cgClassp->isCovergroup(ನಿಜ);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(ನಿಜ);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        ಆದರೆ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ eventp = nodep->eventp()) {
            eventp->unlink_Fr_ಹಿಂಭಾಗ();
            AstCovergroup* ಸ್ಥಿರಾಂಕ cgNodep = ಹೊಸ AstCovergroup{
                nodep->fileline(), nodep->name(), ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, eventp};
            cgClassp->ಕೂಡಿಸು_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        ಗಾಗಿ (Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ origVarp = VN_AS(argp, Var);
            ಆದರೆ (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ಒಳಹರಿವು);
            }
            ಆದರೆ ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ಸ್ಥಿರಾಂಕ memberp = origVarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ಸುಳ್ಳು);
            memberp->direction(VDirection::NONE);
            ಆದರೆ (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(ನಿಜ);
            cgClassp->ಕೂಡಿಸು_Membersp(memberp);
        }

        // Convert sample args to member variables
        ಗಾಗಿ (Ast_ಗಂಟು* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ origVarp = VN_AS(argp, Var);
            ಆದರೆ (!origVarp->is_ಒಳಹರಿವು()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ಒಳಹರಿವು);
            }
            AstVar* ಸ್ಥಿರಾಂಕ memberp = origVarp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ಸುಳ್ಳು);
            memberp->direction(VDirection::NONE);
            cgClassp->ಕೂಡಿಸು_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ಸ್ಥಿರಾಂಕ newFuncp = ಹೊಸ AstFunc{nodep->fileline(), "new", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        newFuncp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::NORETURN, ನಿಜ);
        newFuncp->class_ವಿಧಾನ(ನಿಜ);
        newFuncp->isConstructor(ನಿಜ);
        newFuncp->dtypep(cgClassp->dtypep());
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            newFuncp->ಕೂಡಿಸು_Stmtsp(bodyp);
        }
        cgClassp->ಕೂಡಿಸು_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_ಅಂಕೆ);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_ಅಂಕೆ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverpoint* ಸ್ಥಿರಾಂಕ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ಸ್ಥಿರ string coverpointName(ಸ್ಥಿರಾಂಕ AstCoverpoint* nodep) {
        ಆದರೆ (!nodep->name().ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ಸ್ಥಿರಾಂಕ AstParseRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->exprp(), ParseRef);
        ಹಿಂತಿರುಗಿ refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ಸ್ಥಿರಾಂಕ AstCoverpoint* ಕಂಡುಹಿಡಿ_Coverpoint(ಸ್ಥಿರಾಂಕ AstCoverpointRef* refp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (refp->exprp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // A hierarchical reference, unsupported
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_coverpoints.ಕಂಡುಹಿಡಿ(refp->name());
        ಹಿಂತಿರುಗಿ it == m_coverpoints.ಕೊನೆ() ? ಶೂನ್ಯಸೂಚಕ : it->ಎರಡನೇ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        ಆದರೆ (nodep->name().ಖಾಲಿಗಳು()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ಸ್ಥಿರಾಂಕ string name = coverpointName(nodep);
            nodep->name(name.ಖಾಲಿಗಳು() ? "__Vcoverpoint" + cvt_ಗಮ್ಯ_Str(m_coverpoint_ಅಂಕೆ++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        ಗಾಗಿ (Ast_ಗಂಟು *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಆದರೆ (Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ optp = VN_CAST(itemp, Cg_Option_ನಿಯೋಜಿಸು)) {
                ಆದರೆ (dropDeprecatedCoverageOption(optp)) ಮುಂದುವರಿ;
                optp->unlink_Fr_ಹಿಂಭಾಗ();
                ಆದರೆ (optp->optType() == VCoverOptionType::AT_ಕನಿಷ್ಠ
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::ತೂಕ) {
                    nodep->ಕೂಡಿಸು_Optionsp(ಹೊಸ AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
                } ಇಲ್ಲದಿದ್ದರೆ {
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

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        ಆದರೆ (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            ಆದರೆ (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flNoWarn = ಹೊಸ File_ಸಾಲು{fl};
        flNoWarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
        flNoWarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::VARHIDDEN, ನಿಜ);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flNoWarn, VVarType::VAR, "item", VFlag_ಮಗು_DType{},
                                        ಹೊಸ AstRefDType{fl, AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{},
                                                        m_coverpointp->exprp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}};
        varp->funcLocal(ನಿಜ);
        varp->noReset(ನಿಜ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = nodep->pointp();
        ಸ್ಥಿರಾಂಕ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ಸ್ಥಿರಾಂಕ AstParseRef* binp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(refp->exprp(), Dot);
        ಆದರೆ (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        ಆದರೆ (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        ಆದರೆ (binp) nodep->name(binp->name());
        refp->replaceWith(ಹೊಸ AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ಆದರೆ (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        ಗಾಗಿ (Ast_ಗಂಟು *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಆದರೆ (VN_IS(itemp, CoverCrossBin)) ಮುಂದುವರಿ;
            Ast_Cg_Option_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ optp = VN_AS(itemp, Cg_Option_ನಿಯೋಜಿಸು);
            ಆದರೆ (dropDeprecatedCoverageOption(optp)) ಮುಂದುವರಿ;
            itemp->unlink_Fr_ಹಿಂಭಾಗ();
            ಸ್ಥಿರಾಂಕ VCoverOptionType optType = optp->optType();
            ಆದರೆ (!(optType == VCoverOptionType::AT_ಕನಿಷ್ಠ || optType == VCoverOptionType::ತೂಕ)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->ಕೂಡಿಸು_Optionsp(ಹೊಸ AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
            VL_DO_DANGLING(optp->delete_ವೃಕ್ಷ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        ಆದರೆ (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flNoWarn = ಹೊಸ File_ಸಾಲು{fl};
            flNoWarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
            flNoWarn->modifyWarnOff(V3_Error_ಸಂಕೇತ::VARHIDDEN, ನಿಜ);
            std::ಗಣ<string> ಹೆಸರುಗಳು;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCoverpointRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(itemp, CoverpointRef);
                ಸ್ಥಿರಾಂಕ AstCoverpoint* ಸ್ಥಿರಾಂಕ pointp = ಕಂಡುಹಿಡಿ_Coverpoint(refp);
                ಆದರೆ (!pointp || !ಹೆಸರುಗಳು.ಸೇರಿಸು(refp->name()).ಎರಡನೇ) ಮುಂದುವರಿ;
                AstVar* ಸ್ಥಿರಾಂಕ varp
                    = ಹೊಸ AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlag_ಮಗು_DType{},
                                 ಹೊಸ AstRefDType{fl, AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{},
                                                 pointp->exprp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}};
                varp->funcLocal(ನಿಜ);
                varp->noReset(ನಿಜ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->ಕೂಡಿಸು_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        ಆದರೆ (!nodep->selectp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSystemCSection* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process only once.
        cleanFileline(nodep);
        m_modp->ಕೂಡಿಸು_Sc_Sectionsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ(V3Stats::STAT_ಮೂಲ_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkparse", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
