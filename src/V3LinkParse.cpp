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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkParse.h"

#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

वर्ग LinkParseVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // TYPES
    उपयुज्य ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_समुच्चयः<File_पङ्क्तिः*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = शून्यसूचकः;  // Variable we're under
    Ast_ग्रन्थिः_Module* m_मानम्_Modp = शून्यसूचकः;
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    तार्किक m_अन्तर्_Interface = असत्य;  // True when inside interface declaration
    Ast_ग्रन्थिः_Procedure* m_procedurep = शून्यसूचकः;  // Current procedure
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current task
    AstRSProd* m_rsProdp = शून्यसूचकः;  // Current randsequence production
    Ast_ग्रन्थिः_खण्डम्* m_blockp = शून्यसूचकः;  // Current AstNodeBlock
    Ast_ग्रन्थिः_Stmt* m_खण्डम्_सङ्कलय_Automatic_Stmtp = शून्यसूचकः;  // Initial statements to add to block
    Ast_ग्रन्थिः_Stmt* m_खण्डम्_सङ्कलय_Static_Stmtp = शून्यसूचकः;  // Initial statements to add to block
    Ast_ग्रन्थिः_DType* m_dtypep = शून्यसूचकः;  // Current data type
    Ast_ग्रन्थिः_व्यञ्जकम्* m_default_अन्तर्_Skewp = शून्यसूचकः;  // Current default input skew
    Ast_ग्रन्थिः_व्यञ्जकम्* m_default_बहिर्गमनम्_Skewp = शून्यसूचकः;  // Current default output skew
    AstCoverpoint* m_coverpointp = शून्यसूचकः;  // Current coverpoint
    AstCoverCross* m_crossp = शून्यसूचकः;  // Current cross
    std::map<string, ध्रुव AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    पूर्णाङ्क m_anonUdpId = 0;  // Counter for anonymous UDP instances
    पूर्णाङ्क m_coverpoint_अङ्कः = 0;  // Counter for unnamed coverpoints within current covergroup
    पूर्णाङ्क m_genblkAbove = 0;  // Begin block number of if/case/for above
    पूर्णाङ्क m_genblk_अङ्कः = 0;  // Begin block number, 0=none seen
    पूर्णाङ्क m_begin_गभीरता = 0;  // How many begin blocks above current node within current AstNodeModule
    पूर्णाङ्क m_rand_Sequence_अङ्कः = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    तार्किक m_lifetimeAllowed = असत्य;  // True to allow lifetime settings
    तार्किक m_hasTimingControl = असत्य;  // If current task has timing control
    तार्किक m_moduleWithGenericIface = असत्य;  // If current module contains generic interface
    std::समुच्चयः<AstVar*> m_पत्तनम्_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    शून्य cleanFileline(Ast_ग्रन्थिः* nodep) {
        यदि (nodep->user2_समुच्चयः_Once()) प्रत्यागम;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        यदि (m_filelines.find(nodep->fileline()) != m_filelines.अन्तः_2()) {
            nodep->fileline(नूतन File_पङ्क्तिः{nodep->fileline()});
        }
        m_filelines.निवेशय(nodep->fileline());
    }

    string nameFromTypedef(Ast_ग्रन्थिः* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        यदि (ध्रुव AstTypedef* ध्रुव typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            यदि (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } अन्यथा यदि (m_modp) {
                above = m_modp->name() + ".";
            }
            प्रत्यागम above + typedefp->name();
        }
        प्रत्यागम "";
    }

    शून्य दर्शय_Iterate_ग्रन्थिः_DType(Ast_ग्रन्थिः_DType* nodep) {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    तार्किक nestedIfBegin(Ast_Gen_खण्डम्* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        प्रत्यागम (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    शून्य checkIndent(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* childp) {
        // Try very hard to avoid false positives
        Ast_ग्रन्थिः* nextp = nodep->nextp();
        यदि (!childp) प्रत्यागम;
        यदि (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        यदि (!nextp) प्रत्यागम;
        यदि (VN_IS(childp, Begin) || VN_IS(childp, Gen_खण्डम्)) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव ग्रन्थिः_Flp = nodep->fileline();
        File_पङ्क्तिः* ध्रुव childFlp = childp->fileline();
        File_पङ्क्तिः* ध्रुव अग्र_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        यदि (ग्रन्थिः_Flp->filenameno() != childFlp->filenameno()) प्रत्यागम;
        यदि (ग्रन्थिः_Flp->filenameno() != अग्र_Flp->filenameno()) प्रत्यागम;
        यदि (ग्रन्थिः_Flp->चरमः_Lineno() >= childFlp->firstLineno()) प्रत्यागम;
        यदि (childFlp->चरमः_Lineno() >= अग्र_Flp->firstLineno()) प्रत्यागम;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        यदि (!(ग्रन्थिः_Flp->firstColumn() < childFlp->firstColumn()
              && अग्र_Flp->firstColumn() >= childFlp->firstColumn()))
            प्रत्यागम;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ध्रुव std::string ग्रन्थिः_उपसर्गः = ग्रन्थिः_Flp->source_उपसर्गः(ग्रन्थिः_Flp->firstColumn());
        ध्रुव std::string child_उपसर्गः = childFlp->source_उपसर्गः(ग्रन्थिः_Flp->firstColumn());
        ध्रुव std::string अग्र_उपसर्गः = अग्र_Flp->source_उपसर्गः(ग्रन्थिः_Flp->firstColumn());
        यदि (child_उपसर्गः != ग्रन्थिः_उपसर्गः) प्रत्यागम;
        यदि (अग्र_उपसर्गः != child_उपसर्गः) प्रत्यागम;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ध्रुव std::string ग्रन्थिः_Source = ग्रन्थिः_Flp->source();
        ध्रुव std::string childSource = childFlp->source();
        ध्रुव std::string अग्र_Source = अग्र_Flp->source();
        यदि (!(VString::leading_Whitespace_गणनम्(ग्रन्थिः_Source)
                  < VString::leading_Whitespace_गणनम्(childSource)
              && VString::leading_Whitespace_गणनम्(अग्र_Source)
                     >= VString::leading_Whitespace_गणनम्(childSource)))
            प्रत्यागम;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    शून्य collectPorts(Ast_ग्रन्थिः* ग्रन्थिः_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> पत्तनम्_नामानि;
        प्रति (Ast_ग्रन्थिः* nodep = ग्रन्थिः_Listp; nodep; nodep = nodep->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isIO()) पत्तनम्_नामानि.emplace(varp->name(), varp);
            }
        }
        प्रति (Ast_ग्रन्थिः* nodep = ग्रन्थिः_Listp; nodep; nodep = nodep->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isIO()) अनुवर्त;
                ध्रुव स्वचालित it = पत्तनम्_नामानि.find(varp->name());
                यदि (it == पत्तनम्_नामानि.अन्तः_2()) अनुवर्त;
                AstVar* ध्रुव iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                यदि (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_पृष्ठभागः_With_अग्र());
                m_पत्तनम्_Dups.emplace(varp);
            }
        }
    }

    Ast_ग्रन्थिः* getVarsAndUnlink(Ast_ग्रन्थिः* stmtsp) {
        Ast_ग्रन्थिः* varsp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            यदि (VN_IS(itemp, Var)) varsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(varsp, itemp->unlink_Fr_पृष्ठभागः());
        }
        प्रत्यागम varsp;
    }
    Ast_ग्रन्थिः_Stmt* get_खण्डम्_Adds() {
        Ast_ग्रन्थिः_Stmt* addsp = शून्यसूचकः;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        यदि (m_खण्डम्_सङ्कलय_Static_Stmtp)
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp,
                                     नूतन AstInitialStaticStmt{m_खण्डम्_सङ्कलय_Static_Stmtp->fileline(),
                                                              m_खण्डम्_सङ्कलय_Static_Stmtp});
        यदि (m_खण्डम्_सङ्कलय_Automatic_Stmtp)
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(
                addsp, नूतन AstInitialAutomaticStmt{m_खण्डम्_सङ्कलय_Automatic_Stmtp->fileline(),
                                                   m_खण्डम्_सङ्कलय_Automatic_Stmtp});
        m_खण्डम्_सङ्कलय_Static_Stmtp = शून्यसूचकः;
        m_खण्डम्_सङ्कलय_Automatic_Stmtp = शून्यसूचकः;
        प्रत्यागम addsp;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process only once.
        // Mark class methods
        यदि (VN_IS(m_modp, Class)) nodep->class_विधानम्(सत्य);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_खण्डम्_सङ्कलय_Automatic_Stmtp);
        m_खण्डम्_सङ्कलय_Automatic_Stmtp = शून्यसूचकः;
        VL_RESTORER(m_खण्डम्_सङ्कलय_Static_Stmtp);
        m_खण्डम्_सङ्कलय_Static_Stmtp = शून्यसूचकः;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = सत्य;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) { प्रत्यागम nodep->isTimingControl(); });
        यदि (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } अन्यथा {
            यदि (nodep->class_विधानम्()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } अन्यथा यदि (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        यदि (nodep->class_विधानम्() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_अधिकम्() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_पत्तनम्_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;
        यदि (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_पृष्ठभागः();
        // Move all Vars to front of function
        stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(stmtsp, get_खण्डम्_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_DType* nodep) override { दर्शय_Iterate_ग्रन्थिः_DType(nodep); }
    शून्य दर्शय(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(सत्य);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstConstraintForeach* nodep) override {
        यदि (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_व्यञ्जकम्* exprp) { exprp->isSoft(सत्य); });
        }
    }
    शून्य दर्शय(AstEnumDType* nodep) override {
        यदि (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        दर्शय_Iterate_ग्रन्थिः_DType(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        यदि (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        दर्शय_Iterate_ग्रन्थिः_DType(nodep);
    }
    शून्य दर्शय(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (nodep->rangep()) {
            यदि (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t विस्थापनम्_from_init = 0;
            AstEnumItem* addp = शून्यसूचकः;
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            प्रति (ध्रुव पूर्णाङ्क i : nodep->rangep()->seq_वामः_गन्तव्यम्_दक्षिणः()) {
                ध्रुव string name = nodep->name() + cvt_गन्तव्यम्_Str(i);
                Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;
                यदि (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = नूतन Ast_सङ्कलय{flp, nodep->valuep()->clone_वृक्षः(सत्य),
                                     नूतन AstConst{flp, AstConst::Unsized32{}, विस्थापनम्_from_init}};
                }
                addp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addp, नूतन AstEnumItem{flp, name, शून्यसूचकः, valuep});
                ++विस्थापनम्_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }

    शून्य दर्शय(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class);
        यदि (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(सत्य);
        }
        यदि (nodep->embeddedCovergroup()) {
            AstRefDType* ध्रुव refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        यदि (nodep->valuep()) nodep->hasUserInit(सत्य);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        यदि (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            यदि (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_अधिकम्() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_अधिकम्() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } अन्यथा {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_अधिकम्() << "... The initializer value will only be set once\n"
                        << nodep->warn_अधिकम्()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        यदि (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        यदि (!nodep->direction().isAny()) {  // Not a port
            यदि (nodep->lifetime().isNone()) {
                यदि (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } अन्यथा {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } अन्यथा यदि (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            यदि (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } अन्यथा {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } अन्यथा यदि (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        यदि (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            यदि (m_begin_गभीरता > 0
                || (m_begin_गभीरता == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        यदि (nodep->isGParam() && m_modp) m_modp->hasGParam(सत्य);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        यदि (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(सत्य);

        यदि (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_संहिता::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        यदि (nodep->isParam() && m_अन्तर्_Interface) nodep->isIfaceParam(सत्य);
        यदि (AstParseTypeDType* ध्रुव ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ग्रन्थिः* dtypep = nodep->valuep();
            यदि (dtypep) {
                dtypep->unlink_Fr_पृष्ठभागः();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* exprp = VN_CAST(dtypep, ग्रन्थिः_व्यञ्जकम्)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    यावत् (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        यदि (!rhsDotp) भङ्ग;
                        File_पङ्क्तिः* ध्रुव fl = dotp->fileline();
                        ध्रुव तार्किक colon = dotp->colon();
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhs = VN_AS(dotp->lhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                        File_पङ्क्तिः* ध्रुव rhsFl = rhsDotp->fileline();
                        ध्रुव तार्किक rhsColon = rhsDotp->colon();
                        AstDot* ध्रुव newLhs = नूतन AstDot{fl, colon, lhs, rhsLhs};
                        exprp = नूतन AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_वृक्षः(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } अन्यथा {
                dtypep = नूतन AstVoidDType{nodep->fileline()};
            }

            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, नूतन AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(असत्य), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        यदि (v3Global.opt.any_लौकिकम्_Flat() && nodep->varType().isVPIAccessible()) {
            यदि (v3Global.opt.लौकिकम्_Flat_RW()) {
                nodep->sigUserRWPublic(सत्य);
            } अन्यथा यदि (v3Global.opt.लौकिकम्_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(सत्य);
            } अन्यथा यदि (m_modp && v3Global.opt.लौकिकम्_गभीरता()) {
                यदि ((m_modp->गभीरता() - 1) <= v3Global.opt.लौकिकम्_गभीरता()) {
                    nodep->sigUserRWPublic(सत्य);
                } अन्यथा यदि (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(सत्य);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        यदि (v3Global.opt.trace_गभीरता() && m_modp
            && (m_modp->गभीरता() - 1) > v3Global.opt.trace_गभीरता()) {
            m_modp->modTrace(असत्य);
            nodep->trace(असत्य);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = शून्यसूचकः;
        // temporaries under an always aren't expected to be blocking
        यदि (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_संहिता::BLKSEQ, सत्य);
        // Compute initial value
        यदि (nodep->valuep() || nodep->needsCReset()) {
            File_पङ्क्तिः* ध्रुव fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            स्वचालित createValuep = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                यदि (nodep->valuep()) {
                    प्रत्यागम VN_AS(nodep->valuep()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                } अन्यथा {
                    प्रत्यागम नूतन AstCReset{fl, nodep, असत्य};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            यदि (m_पत्तनम्_Dups.find(nodep) != m_पत्तनम्_Dups.अन्तः_2()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } अन्यथा यदि (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } अन्यथा यदि (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_निवेशः()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                यदि (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                }
            } अन्यथा यदि (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(सत्य);
                File_पङ्क्तिः* ध्रुव newfl = नूतन File_पङ्क्तिः{fl};
                newfl->warnOff(V3_Error_संहिता::E_CONSTWRITTEN, सत्य);
                Ast_नियोजय* ध्रुव assp
                    = नूतन Ast_नियोजय{newfl, नूतन AstParseRef{newfl, nodep->name()}, createValuep()};
                यदि (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_खण्डम्_सङ्कलय_Automatic_Stmtp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_खण्डम्_सङ्कलय_Automatic_Stmtp, assp);
                } अन्यथा {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_खण्डम्_सङ्कलय_Static_Stmtp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_खण्डम्_सङ्कलय_Static_Stmtp, assp);
                }
            } अन्यथा यदि (m_मानम्_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                यदि (nodep->valuep()) {
                    nodep->noCReset(सत्य);
                    File_पङ्क्तिः* ध्रुव newfl = नूतन File_पङ्क्तिः{fl};
                    newfl->warnOff(V3_Error_संहिता::PROCASSWIRE, सत्य);
                    newfl->warnOff(V3_Error_संहिता::E_CONSTWRITTEN, सत्य);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_नियोजय* ध्रुव assp = नूतन Ast_नियोजय{
                        newfl, नूतन AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ग्रन्थिः* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    यदि (nodep->lifetime().isAutomatic()) {
                        newInitp = नूतन AstInitialAutomatic{newfl, assp};
                    } अन्यथा {
                        newInitp = नूतन AstInitialStatic{newfl, assp};
                    }
                    nodep->सङ्कलय_अग्र_अत्र(newInitp);
                }
            } अन्यथा यदि (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    स्थिर तार्किक isNegative(ध्रुव AstConst* constp) {
        प्रत्यागम constp->isSigned() && constp->अङ्कः().bitIs1(constp->विस्तारः() - 1);
    }
    शून्य दर्शय(AstRange* nodep) override {
        स्थिर constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ध्रुव AstConst* ध्रुव leftp = VN_CAST(nodep->leftp(), Const);
        ध्रुव AstConst* ध्रुव rightp = VN_CAST(nodep->rightp(), Const);
        यदि (leftp && rightp && leftp->विस्तारः() <= 64 && rightp->विस्तारः() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ध्रुव uint64_t वामतः = leftp->गन्तव्यम्_UQuad();
            ध्रुव uint64_t दक्षिणतः = rightp->गन्तव्यम्_UQuad();
            ध्रुव uint64_t विस्तृतिः = std::max(वामतः, दक्षिणतः) - std::min(वामतः, दक्षिणतः);
            यदि (विस्तृतिः >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << विस्तृतिः + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstConst* nodep) override {
        यदि (nodep->अङ्कः().autoExtend() && nodep->fileline()->language() < V3_Lang_संहिता::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    शून्य दर्शय(Ast_Attr_सम्बन्धि* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (nodep->attrType() == VAttrType::DT_लौकिकम्) {
            AstTypedef* ध्रुव typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_लौकिकम्(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->समुच्चयः_Forceable();
            v3Global.समुच्चयः_Has_Forceable_सङ्केताः();
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_लौकिकम्) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(सत्य);
            m_varp->sig_Mod_लौकिकम्(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_लौकिकम्_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_लौकिकम्_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_लौकिकम्(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_लौकिकम्_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_विभज_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            यदि (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } अन्यथा {
                m_varp->attr_विभज_Var(सत्य);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_पूर्णम्) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::पूर्णम्);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_FSM_STATE_पूर्णम्) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(सत्य);
            m_varp->attrFsmStateExpand(VFsmExpandType::पूर्णम्);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        }
    }

    शून्य दर्शय(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = शून्यसूचकः;
        ध्रुव ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        यदि (it != m_implTypedef.अन्तः_2()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } अन्यथा {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ग्रन्थिः* backp = nodep->backp();
            प्रति (; backp; backp = backp->backp()) {
                यदि (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ग्रन्थिः_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    भङ्ग;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->childDTypep();
            dtypep->unlink_Fr_पृष्ठभागः();
            यदि (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                प्रत्यागम;
            } अन्यथा {
                defp = नूतन AstTypedef{nodep->fileline(), nodep->name(), शून्यसूचकः, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_गन्तव्यम्_Str(m_implTypedef.आकार()));
                UINFO(9, "New impltypedef " << defp);
                backp->सङ्कलय_अग्र_अत्र(defp);
            }
        }
        nodep->replaceWith(नूतन AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    शून्य दर्शय(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Foreach* nodep) override {
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
        Ast_Foreach_शीर्षकम्* ध्रुव headerp = nodep->headerp();
        यदि (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstLoop* nodep) override {
        cleanFileline(nodep);
        यदि (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } अन्यथा {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_पङ्क्तिः* ध्रुव newfl = nodep->fileline();
            newfl->warnOff(V3_Error_संहिता::WAITCONST, सत्य);
            nodep->fileline(newfl);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        यदि (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_अन्तर्_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_अङ्कः);
        VL_RESTORER(m_begin_गभीरता);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_अङ्कः);
        VL_RESTORER(m_मानम्_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        यदि (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        यदि (VN_IS(nodep, Iface)) m_अन्तर्_Interface = सत्य;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_अङ्कः = 0;
        m_begin_गभीरता = 0;
        m_मानम्_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = असत्य;
        m_rand_Sequence_अङ्कः = 0;

        यदि (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        यदि (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_पत्तनम्_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        यदि (AstModule* ध्रुव modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    शून्य दर्शय_Iterate_No_मानम्_Mod(Ast_ग्रन्थिः* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_मानम्_Modp);
        m_मानम्_Modp = शून्यसूचकः;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = सत्य;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        दर्शय_Iterate_No_मानम्_Mod(nodep);
    }
    शून्य दर्शय(AstCover* nodep) override { दर्शय_Iterate_No_मानम्_Mod(nodep); }
    शून्य दर्शय(AstRestrict* nodep) override { दर्शय_Iterate_No_मानम्_Mod(nodep); }

    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        V3Control::apply_Coverage_खण्डम्(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_गभीरता);
        ++m_begin_गभीरता;
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ध्रुव तार्किक nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        पूर्णाङ्क नियोजय_Gen_Blk_अङ्कः = -1;
        यदि (nodep->genforp()) {
            ++m_genblk_अङ्कः;
            यदि (nodep->name() == "") नियोजय_Gen_Blk_अङ्कः = m_genblk_अङ्कः;
        } अन्यथा यदि (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            नियोजय_Gen_Blk_अङ्कः = m_genblkAbove;
        }
        यदि (नियोजय_Gen_Blk_अङ्कः != -1) {
            nodep->name("genblk" + cvt_गन्तव्यम्_Str(नियोजय_Gen_Blk_अङ्कः));
            यदि (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_अधिकम्()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        यदि (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_अङ्कः);
            m_genblkAbove = 0;
            m_genblk_अङ्कः = 0;
            iterateChildren(nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstGenCase* nodep) override {
        ++m_genblk_अङ्कः;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_अङ्कः);
        m_genblkAbove = m_genblk_अङ्कः;
        m_genblk_अङ्कः = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ध्रुव तार्किक nestedIf = (VN_IS(nodep->backp(), Gen_खण्डम्)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_खण्डम्)));
        यदि (nestedIf) {
            iterateChildren(nodep);
        } अन्यथा {
            ++m_genblk_अङ्कः;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_अङ्कः);
            m_genblkAbove = m_genblk_अङ्कः;
            m_genblk_अङ्कः = 0;
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstCell* nodep) override {
        यदि (nodep->origName().रिक्तानि()) {
            यदि (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ध्रुव string newName = "$unnamedudp" + cvt_गन्तव्यम्_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_खण्डम्* nodep) override {
        VL_RESTORER(m_खण्डम्_सङ्कलय_Automatic_Stmtp);
        m_खण्डम्_सङ्कलय_Automatic_Stmtp = शून्यसूचकः;
        VL_RESTORER(m_खण्डम्_सङ्कलय_Static_Stmtp);
        m_खण्डम्_सङ्कलय_Static_Stmtp = शून्यसूचकः;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_अग्र_Null(nodep->declsp());
        }

        यदि (AstBegin* ध्रुव beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_खण्डम्(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_अग्र_Null(nodep->stmtsp());
        यदि (AstFork* ध्रुव forkp = VN_CAST(nodep, Fork)) iterate_And_अग्र_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;
        // Move all Vars to front of function
        stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(stmtsp, get_खण्डम्_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    शून्य दर्शय(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        यदि (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        यदि (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(सत्य);
            Ast_ग्रन्थिः* ध्रुव nextp = nodep->nextp();
            nodep->unlink_Fr_पृष्ठभागः();
            nextp->सङ्कलय_अग्र(nodep);
        }
    }
    शून्य दर्शय(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ग्रन्थिः* scanp = nodep;
            // Skip over the New's statement
            प्रति (; scanp && !VN_IS(scanp, Stmt_व्यञ्जकम्); scanp = scanp->backp()) {}
            यदि (VN_IS(scanp, Stmt_व्यञ्जकम्)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                प्रति (; scanp; scanp = scanp->backp()) {
                    यदि (VN_IS(scanp, ग्रन्थिः_Stmt) || VN_IS(scanp, ग्रन्थिः_Module)
                        || VN_IS(scanp, ग्रन्थिः_FTask))
                        भङ्ग;
                }
                यदि (!VN_IS(scanp, ग्रन्थिः_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    शून्य दर्शय(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::गन्तव्यम्_string(m_rand_Sequence_अङ्कः++));
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    शून्य दर्शय(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ध्रुव alwaysp = VN_CAST(nodep->backp(), Always);
        यदि (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_अधिकम्() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा यदि (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            यदि (Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_पृष्ठभागः_With_अग्र();
                alwaysp->sentreep(sentreep);
            }
            यदि (nodep->stmtsp()) alwaysp->सङ्कलय_Stmtsp(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        }
    }
    शून्य दर्शय(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_अन्तर्_Skewp);
        VL_RESTORER(m_default_बहिर्गमनम्_Skewp);
        // Find default input and output skews
        प्रति (Ast_ग्रन्थिः *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            यदि (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                यदि (citemp->exprp() || citemp->assignp()) अनुवर्त;
                यदि (citemp->skewp()) {
                    यदि (citemp->direction() == VDirection::निवेशः) {
                        // Disallow default redefinition; note some simulators allow this
                        यदि (m_default_अन्तर्_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_अन्तर्_Skewp = citemp->skewp();
                    } अन्यथा यदि (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        यदि (m_default_बहिर्गमनम्_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_बहिर्गमनम्_Skewp = citemp->skewp();
                    } अन्यथा {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(प्रक्षिप_Deletep(citemp->unlink_Fr_पृष्ठभागः()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        यदि (nodep->direction() == VDirection::OUTPUT) {
            यदि (!nodep->skewp()) {
                यदि (m_default_बहिर्गमनम्_Skewp) {
                    nodep->skewp(m_default_बहिर्गमनम्_Skewp->clone_वृक्षः(असत्य));
                } अन्यथा {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(नूतन AstConst{nodep->fileline(), 0});
                }
            }
        } अन्यथा यदि (nodep->direction() == VDirection::निवेशः) {
            यदि (!nodep->skewp()) {
                यदि (m_default_अन्तर्_Skewp) {
                    nodep->skewp(m_default_अन्तर्_Skewp->clone_वृक्षः(असत्य));
                } अन्यथा {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(नूतन AstConst{nodep->fileline(), AstConst::One_चरणः{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        यदि (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    शून्य दर्शय(Ast_Tagged_व्यञ्जकम्* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Tagged_आदर्शः* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_आदर्शः_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_आदर्शः_तारकम्* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        यदि (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    स्थिर Ast_ग्रन्थिः* makeArgMemberCopies(Ast_ग्रन्थिः* argsp, तार्किक readOnlyRefs) {
        Ast_ग्रन्थिः* resultp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ध्रुव origVarp = VN_AS(argp, Var);
            AstVar* ध्रुव paramp = origVarp->clone_वृक्षः(असत्य);
            File_पङ्क्तिः* ध्रुव fl = नूतन File_पङ्क्तिः{paramp->fileline()};
            fl->warnOff(V3_Error_संहिता::VARHIDDEN, सत्य);
            paramp->fileline(fl);
            paramp->funcLocal(सत्य);
            paramp->direction(origVarp->direction());
            यदि (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(resultp, paramp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstDot{
                origVarp->fileline(), असत्य, नूतन AstParseRef{origVarp->fileline(), "this"},
                नूतन AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstParseRef{paramp->fileline(), paramp->name()};
            resultp->सङ्कलय_अग्र(नूतन Ast_नियोजय{origVarp->fileline(), lhsp, rhsp});
        }
        प्रत्यागम resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    स्थिर शून्य createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ग्रन्थिः* argsp,
                                        Ast_ग्रन्थिः* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ध्रुव defaultVarp
            = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->सङ्कलय_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        यदि (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, सत्य));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ध्रुव varp
                = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             नूतन AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             नूतन AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      शून्यसूचकः, शून्यसूचकः},
                                             शून्यसूचकः}};
            nodep->सङ्कलय_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ध्रुव varp
                = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             नूतन AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             नूतन AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      शून्यसूचकः, शून्यसूचकः},
                                             शून्यसूचकः}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->सङ्कलय_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ध्रुव funcp = नूतन AstFunc{nodep->fileline(), "sample", शून्यसूचकः, शून्यसूचकः};
            funcp->सङ्कलय_Stmtsp(makeArgMemberCopies(sampleArgsp, असत्य));
            funcp->class_विधानम्(सत्य);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(सत्य);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->सङ्कलय_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        प्रति (ध्रुव string& name : {"start"s, "stop"s}) {
            AstFunc* ध्रुव funcp = नूतन AstFunc{nodep->fileline(), name, शून्यसूचकः, शून्यसूचकः};
            funcp->class_विधानम्(सत्य);
            funcp->dtypep(funcp->findVoidDType());
            nodep->सङ्कलय_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        प्रति (ध्रुव string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ध्रुव funcp = नूतन AstFunc{nodep->fileline(), name, शून्यसूचकः, शून्यसूचकः};
            funcp->fileline()->warnOff(V3_Error_संहिता::NORETURN, सत्य);
            funcp->isStatic(name == "get_coverage");
            funcp->class_विधानम्(सत्य);
            funcp->dtypep(funcp->findVoidDType());
            nodep->सङ्कलय_Membersp(funcp);
            {
                AstVar* ध्रुव varp = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(सत्य);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(सत्य);
                funcp->fvarp(varp);
            }
            प्रति (ध्रुव string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ध्रुव varp = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(सत्य);
                varp->direction(VDirection::निवेशः);
                varp->valuep(नूतन AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->सङ्कलय_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ध्रुव funcp
                = नूतन AstFunc{nodep->fileline(), "set_inst_name", शून्यसूचकः, शून्यसूचकः};
            funcp->class_विधानम्(सत्य);
            funcp->dtypep(funcp->findVoidDType());
            nodep->सङ्कलय_Membersp(funcp);
            AstVar* ध्रुव varp = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(सत्य);
            varp->direction(VDirection::निवेशः);
            funcp->सङ्कलय_Stmtsp(varp);
        }
    }

    तार्किक dropDeprecatedCoverageOption(Ast_Cg_Option_नियोजय* ध्रुव nodep) {
        यदि (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) प्रत्यागम असत्य;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        प्रत्यागम सत्य;
    }

    शून्य दर्शय(Ast_Cg_Option_नियोजय* nodep) override {
        यदि (dropDeprecatedCoverageOption(nodep)) प्रत्यागम;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        यदि (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) प्रत्यागम;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        यदि (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ध्रुव string libname = m_modp->libname();
        AstClass* ध्रुव cgClassp = नूतन AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
        cgClassp->fileline()->warnOff(V3_Error_संहिता::UNDRIVEN, सत्य);
        cgClassp->isCovergroup(सत्य);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(सत्य);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        यदि (Ast_Sen_वृक्षः* ध्रुव eventp = nodep->eventp()) {
            eventp->unlink_Fr_पृष्ठभागः();
            AstCovergroup* ध्रुव cgNodep = नूतन AstCovergroup{
                nodep->fileline(), nodep->name(), शून्यसूचकः, शून्यसूचकः, शून्यसूचकः, eventp};
            cgClassp->सङ्कलय_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        प्रति (Ast_ग्रन्थिः* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ध्रुव origVarp = VN_AS(argp, Var);
            यदि (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::निवेशः);
            }
            यदि ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ध्रुव memberp = origVarp->clone_वृक्षः(असत्य);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(असत्य);
            memberp->direction(VDirection::NONE);
            यदि (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(सत्य);
            cgClassp->सङ्कलय_Membersp(memberp);
        }

        // Convert sample args to member variables
        प्रति (Ast_ग्रन्थिः* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ध्रुव origVarp = VN_AS(argp, Var);
            यदि (!origVarp->is_निवेशः()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::निवेशः);
            }
            AstVar* ध्रुव memberp = origVarp->clone_वृक्षः(असत्य);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(असत्य);
            memberp->direction(VDirection::NONE);
            cgClassp->सङ्कलय_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ध्रुव newFuncp = नूतन AstFunc{nodep->fileline(), "new", शून्यसूचकः, शून्यसूचकः};
        newFuncp->fileline()->warnOff(V3_Error_संहिता::NORETURN, सत्य);
        newFuncp->class_विधानम्(सत्य);
        newFuncp->isConstructor(सत्य);
        newFuncp->dtypep(cgClassp->dtypep());
        यदि (Ast_ग्रन्थिः* ध्रुव bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_पृष्ठभागः_With_अग्र();
            newFuncp->सङ्कलय_Stmtsp(bodyp);
        }
        cgClassp->सङ्कलय_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_अङ्कः);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_अङ्कः = 0;
        प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstCoverpoint* ध्रुव pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    स्थिर string coverpointName(ध्रुव AstCoverpoint* nodep) {
        यदि (!nodep->name().रिक्तानि()) प्रत्यागम nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ध्रुव AstParseRef* ध्रुव refp = VN_CAST(nodep->exprp(), ParseRef);
        प्रत्यागम refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ध्रुव AstCoverpoint* findCoverpoint(ध्रुव AstCoverpointRef* refp) ध्रुव {
        यदि (refp->exprp()) प्रत्यागम शून्यसूचकः;  // A hierarchical reference, unsupported
        ध्रुव स्वचालित it = m_coverpoints.find(refp->name());
        प्रत्यागम it == m_coverpoints.अन्तः_2() ? शून्यसूचकः : it->second;
    }

    शून्य दर्शय(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        यदि (nodep->name().रिक्तानि()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ध्रुव string name = coverpointName(nodep);
            nodep->name(name.रिक्तानि() ? "__Vcoverpoint" + cvt_गन्तव्यम्_Str(m_coverpoint_अङ्कः++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        प्रति (Ast_ग्रन्थिः *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            यदि (Ast_Cg_Option_नियोजय* ध्रुव optp = VN_CAST(itemp, Cg_Option_नियोजय)) {
                यदि (dropDeprecatedCoverageOption(optp)) अनुवर्त;
                optp->unlink_Fr_पृष्ठभागः();
                यदि (optp->optType() == VCoverOptionType::AT_न्यूनतमम्
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->सङ्कलय_Optionsp(नूतन AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_वृक्षः(असत्य)});
                } अन्यथा {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_वृक्षः(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        यदि (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        यदि (ध्रुव AstCoverpointRef* ध्रुव refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            यदि (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        File_पङ्क्तिः* ध्रुव flNoWarn = नूतन File_पङ्क्तिः{fl};
        flNoWarn->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
        flNoWarn->modifyWarnOff(V3_Error_संहिता::VARHIDDEN, सत्य);
        AstVar* ध्रुव varp = नूतन AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        नूतन AstRefDType{fl, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{},
                                                        m_coverpointp->exprp()->clone_वृक्षः(असत्य)}};
        varp->funcLocal(सत्य);
        varp->noReset(सत्य);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ध्रुव refp = nodep->pointp();
        ध्रुव AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ध्रुव AstParseRef* binp = शून्यसूचकः;
        ध्रुव AstDot* ध्रुव dotp = VN_CAST(refp->exprp(), Dot);
        यदि (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        यदि (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        यदि (binp) nodep->name(binp->name());
        refp->replaceWith(नूतन AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        }
    }

    शून्य दर्शय(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        यदि (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        }
    }

    शून्य दर्शय(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        प्रति (Ast_ग्रन्थिः *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            यदि (VN_IS(itemp, CoverCrossBin)) अनुवर्त;
            Ast_Cg_Option_नियोजय* ध्रुव optp = VN_AS(itemp, Cg_Option_नियोजय);
            यदि (dropDeprecatedCoverageOption(optp)) अनुवर्त;
            itemp->unlink_Fr_पृष्ठभागः();
            ध्रुव VCoverOptionType optType = optp->optType();
            यदि (!(optType == VCoverOptionType::AT_न्यूनतमम् || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->सङ्कलय_Optionsp(नूतन AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_वृक्षः(असत्य)});
            VL_DO_DANGLING(optp->delete_वृक्षः(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        यदि (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            File_पङ्क्तिः* ध्रुव flNoWarn = नूतन File_पङ्क्तिः{fl};
            flNoWarn->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
            flNoWarn->modifyWarnOff(V3_Error_संहिता::VARHIDDEN, सत्य);
            std::समुच्चयः<string> नामानि;
            प्रति (ध्रुव Ast_ग्रन्थिः* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ध्रुव AstCoverpointRef* ध्रुव refp = VN_AS(itemp, CoverpointRef);
                ध्रुव AstCoverpoint* ध्रुव pointp = findCoverpoint(refp);
                यदि (!pointp || !नामानि.निवेशय(refp->name()).second) अनुवर्त;
                AstVar* ध्रुव varp
                    = नूतन AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 नूतन AstRefDType{fl, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{},
                                                 pointp->exprp()->clone_वृक्षः(असत्य)}};
                varp->funcLocal(सत्य);
                varp->noReset(सत्य);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->सङ्कलय_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        यदि (!nodep->selectp()) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    शून्य दर्शय(AstSystemCSection* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process only once.
        cleanFileline(nodep);
        m_modp->सङ्कलय_Sc_Sectionsp(nodep->unlink_Fr_पृष्ठभागः());
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::सङ्कलय_Stat_योग(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

शून्य V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linkparse", 0, dump_वृक्षः_Either_Level() >= 6);
}
