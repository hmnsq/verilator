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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkParse.h"

#ضمن "V3Control.h"
#ضمن "V3Stats.h"

#ضمن <set>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

صنف LinkParseVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;

    // TYPES
    کارول ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_سېټ<File_کرښه*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = تش_اشاره;  // Variable we're under
    Ast_غوټه_Module* m_ارزښت_Modp = تش_اشاره;
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    منطقي m_دننه_Interface = خطأ;  // True when inside interface declaration
    Ast_غوټه_Procedure* m_procedurep = تش_اشاره;  // Current procedure
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current task
    AstRSProd* m_rsProdp = تش_اشاره;  // Current randsequence production
    Ast_غوټه_بلاک* m_blockp = تش_اشاره;  // Current AstNodeBlock
    Ast_غوټه_Stmt* m_بلاک_جمع_کړه_Automatic_Stmtp = تش_اشاره;  // Initial statements to add to block
    Ast_غوټه_Stmt* m_بلاک_جمع_کړه_Static_Stmtp = تش_اشاره;  // Initial statements to add to block
    Ast_غوټه_DType* m_dtypep = تش_اشاره;  // Current data type
    Ast_غوټه_بیان* m_default_دننه_Skewp = تش_اشاره;  // Current default input skew
    Ast_غوټه_بیان* m_default_بهر_Skewp = تش_اشاره;  // Current default output skew
    AstCoverpoint* m_coverpointp = تش_اشاره;  // Current coverpoint
    AstCoverCross* m_crossp = تش_اشاره;  // Current cross
    std::map<string, ثابت AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    صحيح m_anonUdpId = 0;  // Counter for anonymous UDP instances
    صحيح m_coverpoint_عدد = 0;  // Counter for unnamed coverpoints within current covergroup
    صحيح m_genblkAbove = 0;  // Begin block number of if/case/for above
    صحيح m_genblk_عدد = 0;  // Begin block number, 0=none seen
    صحيح m_begin_ژوروالی = 0;  // How many begin blocks above current node within current AstNodeModule
    صحيح m_rand_Sequence_عدد = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    منطقي m_lifetimeAllowed = خطأ;  // True to allow lifetime settings
    منطقي m_hasTimingControl = خطأ;  // If current task has timing control
    منطقي m_moduleWithGenericIface = خطأ;  // If current module contains generic interface
    std::سېټ<AstVar*> m_پورټ_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    فراغ cleanFileline(Ast_غوټه* nodep) {
        إذا (nodep->user2_سېټ_Once()) أرجع;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        إذا (m_filelines.پیدا_کړه(nodep->fileline()) != m_filelines.پای_ټکی()) {
            nodep->fileline(جديد File_کرښه{nodep->fileline()});
        }
        m_filelines.ورننباسه(nodep->fileline());
    }

    string nameFromTypedef(Ast_غوټه* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        إذا (ثابت AstTypedef* ثابت typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            إذا (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } وإلا إذا (m_modp) {
                above = m_modp->name() + ".";
            }
            أرجع above + typedefp->name();
        }
        أرجع "";
    }

    فراغ لیدنه_Iterate_غوټه_DType(Ast_غوټه_DType* nodep) {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    منطقي nestedIfBegin(Ast_Gen_بلاک* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        أرجع (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, Gen_Case_توکی))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    فراغ checkIndent(Ast_غوټه* nodep, Ast_غوټه* childp) {
        // Try very hard to avoid false positives
        Ast_غوټه* nextp = nodep->nextp();
        إذا (!childp) أرجع;
        إذا (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        إذا (!nextp) أرجع;
        إذا (VN_IS(childp, Begin) || VN_IS(childp, Gen_بلاک)) أرجع;
        File_کرښه* ثابت غوټه_Flp = nodep->fileline();
        File_کرښه* ثابت ماشوم_Flp = childp->fileline();
        File_کرښه* ثابت بل_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        إذا (غوټه_Flp->filenameno() != ماشوم_Flp->filenameno()) أرجع;
        إذا (غوټه_Flp->filenameno() != بل_Flp->filenameno()) أرجع;
        إذا (غوټه_Flp->وروستی_Lineno() >= ماشوم_Flp->لومړی_Lineno()) أرجع;
        إذا (ماشوم_Flp->وروستی_Lineno() >= بل_Flp->لومړی_Lineno()) أرجع;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        إذا (!(غوټه_Flp->لومړی_ستنه() < ماشوم_Flp->لومړی_ستنه()
              && بل_Flp->لومړی_ستنه() >= ماشوم_Flp->لومړی_ستنه()))
            أرجع;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ثابت std::string غوټه_مختاړی = غوټه_Flp->سرچینه_مختاړی(غوټه_Flp->لومړی_ستنه());
        ثابت std::string ماشوم_مختاړی = ماشوم_Flp->سرچینه_مختاړی(غوټه_Flp->لومړی_ستنه());
        ثابت std::string بل_مختاړی = بل_Flp->سرچینه_مختاړی(غوټه_Flp->لومړی_ستنه());
        إذا (ماشوم_مختاړی != غوټه_مختاړی) أرجع;
        إذا (بل_مختاړی != ماشوم_مختاړی) أرجع;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ثابت std::string غوټه_سرچینه = غوټه_Flp->سرچینه();
        ثابت std::string ماشوم_سرچینه = ماشوم_Flp->سرچینه();
        ثابت std::string بل_سرچینه = بل_Flp->سرچینه();
        إذا (!(VString::leading_Whitespace_شمېر(غوټه_سرچینه)
                  < VString::leading_Whitespace_شمېر(ماشوم_سرچینه)
              && VString::leading_Whitespace_شمېر(بل_سرچینه)
                     >= VString::leading_Whitespace_شمېر(ماشوم_سرچینه)))
            أرجع;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    فراغ collectPorts(Ast_غوټه* غوټه_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> پورټ_نومونه;
        لكل (Ast_غوټه* nodep = غوټه_Listp; nodep; nodep = nodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isIO()) پورټ_نومونه.emplace(varp->name(), varp);
            }
        }
        لكل (Ast_غوټه* nodep = غوټه_Listp; nodep; nodep = nodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isIO()) استمر;
                ثابت خپلکاری it = پورټ_نومونه.پیدا_کړه(varp->name());
                إذا (it == پورټ_نومونه.پای_ټکی()) استمر;
                AstVar* ثابت iop = it->دوهم;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                إذا (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_شا_With_بل());
                m_پورټ_Dups.emplace(varp);
            }
        }
    }

    Ast_غوټه* getVarsAndUnlink(Ast_غوټه* stmtsp) {
        Ast_غوټه* varsp = تش_اشاره;
        لكل (Ast_غوټه *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (VN_IS(itemp, Var)) varsp = Ast_غوټه::جمع_کړه_بل(varsp, itemp->unlink_Fr_شا());
        }
        أرجع varsp;
    }
    Ast_غوټه_Stmt* get_بلاک_Adds() {
        Ast_غوټه_Stmt* addsp = تش_اشاره;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        إذا (m_بلاک_جمع_کړه_Static_Stmtp)
            addsp = Ast_غوټه::جمع_کړه_بل(addsp,
                                     جديد AstInitialStaticStmt{m_بلاک_جمع_کړه_Static_Stmtp->fileline(),
                                                              m_بلاک_جمع_کړه_Static_Stmtp});
        إذا (m_بلاک_جمع_کړه_Automatic_Stmtp)
            addsp = Ast_غوټه::جمع_کړه_بل(
                addsp, جديد AstInitialAutomaticStmt{m_بلاک_جمع_کړه_Automatic_Stmtp->fileline(),
                                                   m_بلاک_جمع_کړه_Automatic_Stmtp});
        m_بلاک_جمع_کړه_Static_Stmtp = تش_اشاره;
        m_بلاک_جمع_کړه_Automatic_Stmtp = تش_اشاره;
        أرجع addsp;
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process only once.
        // Mark class methods
        إذا (VN_IS(m_modp, Class)) nodep->class_لاره(ریښتیا);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_بلاک_جمع_کړه_Automatic_Stmtp);
        m_بلاک_جمع_کړه_Automatic_Stmtp = تش_اشاره;
        VL_RESTORER(m_بلاک_جمع_کړه_Static_Stmtp);
        m_بلاک_جمع_کړه_Static_Stmtp = تش_اشاره;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ریښتیا;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ثابت Ast_غوټه* ثابت nodep) { أرجع nodep->isTimingControl(); });
        إذا (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } وإلا {
            إذا (nodep->class_لاره()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } وإلا إذا (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        إذا (nodep->class_لاره() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_نور() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_پورټ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_غوټه* stmtsp = تش_اشاره;
        إذا (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_شا();
        // Move all Vars to front of function
        stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, get_بلاک_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    فراغ لیدنه(Ast_غوټه_DType* nodep) override { لیدنه_Iterate_غوټه_DType(nodep); }
    فراغ لیدنه(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(ریښتیا);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstConstraintForeach* nodep) override {
        إذا (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_بیان* exprp) { exprp->isSoft(ریښتیا); });
        }
    }
    فراغ لیدنه(AstEnumDType* nodep) override {
        إذا (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        لیدنه_Iterate_غوټه_DType(nodep);
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        إذا (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        لیدنه_Iterate_غوټه_DType(nodep);
    }
    فراغ لیدنه(Ast_Enum_توکی* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (nodep->rangep()) {
            إذا (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t دننه_واټن_from_init = 0;
            Ast_Enum_توکی* addp = تش_اشاره;
            File_کرښه* ثابت flp = nodep->fileline();
            لكل (ثابت صحيح i : nodep->rangep()->seq_کیڼ_منزل_ښی()) {
                ثابت string name = nodep->name() + cvt_منزل_Str(i);
                Ast_غوټه_بیان* valuep = تش_اشاره;
                إذا (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = جديد Ast_جمع_کړه{flp, nodep->valuep()->clone_ونه(ریښتیا),
                                     جديد AstConst{flp, AstConst::Unsized32{}, دننه_واټن_from_init}};
                }
                addp = Ast_غوټه::جمع_کړه_بل(addp, جديد Ast_Enum_توکی{flp, name, تش_اشاره, valuep});
                ++دننه_واټن_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }

    فراغ لیدنه(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
        إذا (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(ریښتیا);
        }
        إذا (nodep->embeddedCovergroup()) {
            AstRefDType* ثابت refp = VN_AS(nodep->ماشوم_DTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        إذا (nodep->valuep()) nodep->hasUserInit(ریښتیا);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        إذا (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            إذا (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_نور() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_نور() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } وإلا {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_نور() << "... The initializer value will only be set once\n"
                        << nodep->warn_نور()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        إذا (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        إذا (!nodep->direction().isAny()) {  // Not a port
            إذا (nodep->lifetime().isNone()) {
                إذا (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } وإلا {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } وإلا إذا (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            إذا (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } وإلا {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } وإلا إذا (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        إذا (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            إذا (m_begin_ژوروالی > 0
                || (m_begin_ژوروالی == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        إذا (nodep->isGParam() && m_modp) m_modp->hasGParam(ریښتیا);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ثابت AstBasicDType* ثابت basicp = VN_CAST(nodep->ماشوم_DTypep(), BasicDType);
        إذا (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(ریښتیا);

        إذا (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_کوډ::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        إذا (nodep->isParam() && m_دننه_Interface) nodep->isIfaceParam(ریښتیا);
        إذا (AstParseTypeDType* ثابت ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_غوټه* dtypep = nodep->valuep();
            إذا (dtypep) {
                dtypep->unlink_Fr_شا();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                إذا (Ast_غوټه_بیان* exprp = VN_CAST(dtypep, غوټه_بیان)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    طالما (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        إذا (!rhsDotp) اكسر;
                        File_کرښه* ثابت fl = dotp->fileline();
                        ثابت منطقي colon = dotp->colon();
                        Ast_غوټه_بیان* ثابت lhs = VN_AS(dotp->lhsp()->unlink_Fr_شا(), غوټه_بیان);
                        Ast_غوټه_بیان* ثابت rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_شا(), غوټه_بیان);
                        Ast_غوټه_بیان* ثابت rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_شا(), غوټه_بیان);
                        File_کرښه* ثابت rhsFl = rhsDotp->fileline();
                        ثابت منطقي rhsColon = rhsDotp->colon();
                        AstDot* ثابت newLhs = جديد AstDot{fl, colon, lhs, rhsLhs};
                        exprp = جديد AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_ونه(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } وإلا {
                dtypep = جديد AstVoidDType{nodep->fileline()};
            }

            Ast_غوټه* ثابت newp = جديد AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlag_ماشوم_DType{}, جديد AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        ثابت Ast_غوټه_DType* ثابت dtypep = nodep->ماشوم_DTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(خطأ), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        إذا (v3Global.opt.any_عامه_Flat() && nodep->varType().isVPIAccessible()) {
            إذا (v3Global.opt.عامه_Flat_RW()) {
                nodep->sigUserRWPublic(ریښتیا);
            } وإلا إذا (v3Global.opt.عامه_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(ریښتیا);
            } وإلا إذا (m_modp && v3Global.opt.عامه_ژوروالی()) {
                إذا ((m_modp->ژوروالی() - 1) <= v3Global.opt.عامه_ژوروالی()) {
                    nodep->sigUserRWPublic(ریښتیا);
                } وإلا إذا (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(ریښتیا);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        إذا (v3Global.opt.trace_ژوروالی() && m_modp
            && (m_modp->ژوروالی() - 1) > v3Global.opt.trace_ژوروالی()) {
            m_modp->modTrace(خطأ);
            nodep->trace(خطأ);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = تش_اشاره;
        // temporaries under an always aren't expected to be blocking
        إذا (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_کوډ::BLKSEQ, ریښتیا);
        // Compute initial value
        إذا (nodep->valuep() || nodep->needsCReset()) {
            File_کرښه* ثابت fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            خپلکاری createValuep = [&]() -> Ast_غوټه_بیان* {
                إذا (nodep->valuep()) {
                    أرجع VN_AS(nodep->valuep()->unlink_Fr_شا(), غوټه_بیان);
                } وإلا {
                    أرجع جديد AstCReset{fl, nodep, خطأ};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            إذا (m_پورټ_Dups.پیدا_کړه(nodep) != m_پورټ_Dups.پای_ټکی()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } وإلا إذا (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } وإلا إذا (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ننوت()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                إذا (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_شا()->delete_ونه();
                }
            } وإلا إذا (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(ریښتیا);
                File_کرښه* ثابت newfl = جديد File_کرښه{fl};
                newfl->warnOff(V3_Error_کوډ::E_CONSTWRITTEN, ریښتیا);
                Ast_ټاکل* ثابت assp
                    = جديد Ast_ټاکل{newfl, جديد AstParseRef{newfl, nodep->name()}, createValuep()};
                إذا (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_بلاک_جمع_کړه_Automatic_Stmtp = Ast_غوټه::جمع_کړه_بل(m_بلاک_جمع_کړه_Automatic_Stmtp, assp);
                } وإلا {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_بلاک_جمع_کړه_Static_Stmtp = Ast_غوټه::جمع_کړه_بل(m_بلاک_جمع_کړه_Static_Stmtp, assp);
                }
            } وإلا إذا (m_ارزښت_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                إذا (nodep->valuep()) {
                    nodep->noCReset(ریښتیا);
                    File_کرښه* ثابت newfl = جديد File_کرښه{fl};
                    newfl->warnOff(V3_Error_کوډ::PROCASSWIRE, ریښتیا);
                    newfl->warnOff(V3_Error_کوډ::E_CONSTWRITTEN, ریښتیا);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_ټاکل* ثابت assp = جديد Ast_ټاکل{
                        newfl, جديد AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_غوټه* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    إذا (nodep->lifetime().isAutomatic()) {
                        newInitp = جديد AstInitialAutomatic{newfl, assp};
                    } وإلا {
                        newInitp = جديد AstInitialStatic{newfl, assp};
                    }
                    nodep->جمع_کړه_بل_دلته(newInitp);
                }
            } وإلا إذا (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ساكن منطقي isNegative(ثابت AstConst* constp) {
        أرجع constp->isSigned() && constp->عدد().بټ_Is1(constp->پلنوالی() - 1);
    }
    فراغ لیدنه(AstRange* nodep) override {
        ساكن constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ثابت AstConst* ثابت leftp = VN_CAST(nodep->leftp(), Const);
        ثابت AstConst* ثابت rightp = VN_CAST(nodep->rightp(), Const);
        إذا (leftp && rightp && leftp->پلنوالی() <= 64 && rightp->پلنوالی() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ثابت uint64_t کیڼ = leftp->منزل_UQuad();
            ثابت uint64_t ښی = rightp->منزل_UQuad();
            ثابت uint64_t پراختیا = std::max(کیڼ, ښی) - std::min(کیڼ, ښی);
            إذا (پراختیا >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << پراختیا + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstConst* nodep) override {
        إذا (nodep->عدد().autoExtend() && nodep->fileline()->language() < V3_Lang_کوډ::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    فراغ لیدنه(Ast_Attr_د* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (nodep->attrType() == VAttrType::DT_عامه) {
            AstTypedef* ثابت typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_عامه(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->سېټ_Forceable();
            v3Global.سېټ_Has_Forceable_سیګنالونه();
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_عامه) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ریښتیا);
            m_varp->sig_Mod_عامه(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_عامه_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_عامه_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_عامه(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_عامه_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_وېشل_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            إذا (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } وإلا {
                m_varp->attr_وېشل_Var(ریښتیا);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_ډک) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::ډک);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_FSM_STATE_ډک) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ریښتیا);
            m_varp->attrFsmStateExpand(VFsmExpandType::ډک);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        }
    }

    فراغ لیدنه(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = تش_اشاره;
        ثابت ImplTypedefMap::iterator it = m_implTypedef.پیدا_کړه(nodep->name());
        إذا (it != m_implTypedef.پای_ټکی()) {
            defp = it->دوهم;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } وإلا {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_غوټه* backp = nodep->backp();
            لكل (; backp; backp = backp->backp()) {
                إذا (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, غوټه_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    اكسر;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_غوټه_DType* ثابت dtypep = nodep->ماشوم_DTypep();
            dtypep->unlink_Fr_شا();
            إذا (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                أرجع;
            } وإلا {
                defp = جديد AstTypedef{nodep->fileline(), nodep->name(), تش_اشاره, VFlag_ماشوم_DType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_منزل_Str(m_implTypedef.اندازه()));
                UINFO(9, "New impltypedef " << defp);
                backp->جمع_کړه_بل_دلته(defp);
            }
        }
        nodep->replaceWith(جديد AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }

    فراغ لیدنه(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {
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
        Ast_Foreach_سرلیک* ثابت headerp = nodep->headerp();
        إذا (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstLoop* nodep) override {
        cleanFileline(nodep);
        إذا (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } وإلا {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_کرښه* ثابت newfl = nodep->fileline();
            newfl->warnOff(V3_Error_کوډ::WAITCONST, ریښتیا);
            nodep->fileline(newfl);
        }
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        إذا (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_دننه_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_عدد);
        VL_RESTORER(m_begin_ژوروالی);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_عدد);
        VL_RESTORER(m_ارزښت_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        إذا (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        إذا (VN_IS(nodep, Iface)) m_دننه_Interface = ریښتیا;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_عدد = 0;
        m_begin_ژوروالی = 0;
        m_ارزښت_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = خطأ;
        m_rand_Sequence_عدد = 0;

        إذا (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        إذا (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_پورټ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        إذا (AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    فراغ لیدنه_Iterate_No_ارزښت_Mod(Ast_غوټه* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_ارزښت_Modp);
        m_ارزښت_Modp = تش_اشاره;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ریښتیا;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        لیدنه_Iterate_No_ارزښت_Mod(nodep);
    }
    فراغ لیدنه(AstCover* nodep) override { لیدنه_Iterate_No_ارزښت_Mod(nodep); }
    فراغ لیدنه(AstRestrict* nodep) override { لیدنه_Iterate_No_ارزښت_Mod(nodep); }

    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        V3Control::apply_Coverage_بلاک(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_ژوروالی);
        ++m_begin_ژوروالی;
        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ثابت منطقي nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        صحيح ټاکل_Gen_Blk_عدد = -1;
        إذا (nodep->genforp()) {
            ++m_genblk_عدد;
            إذا (nodep->name() == "") ټاکل_Gen_Blk_عدد = m_genblk_عدد;
        } وإلا إذا (nodep->name() == "" && (VN_IS(backp, Gen_Case_توکی) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            ټاکل_Gen_Blk_عدد = m_genblkAbove;
        }
        إذا (ټاکل_Gen_Blk_عدد != -1) {
            nodep->name("genblk" + cvt_منزل_Str(ټاکل_Gen_Blk_عدد));
            إذا (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_نور()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        إذا (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_عدد);
            m_genblkAbove = 0;
            m_genblk_عدد = 0;
            iterateChildren(nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstGenCase* nodep) override {
        ++m_genblk_عدد;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_عدد);
        m_genblkAbove = m_genblk_عدد;
        m_genblk_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ثابت منطقي nestedIf = (VN_IS(nodep->backp(), Gen_بلاک)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_بلاک)));
        إذا (nestedIf) {
            iterateChildren(nodep);
        } وإلا {
            ++m_genblk_عدد;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_عدد);
            m_genblkAbove = m_genblk_عدد;
            m_genblk_عدد = 0;
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstCell* nodep) override {
        إذا (nodep->origName().تشه()) {
            إذا (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ثابت string newName = "$unnamedudp" + cvt_منزل_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_بلاک* nodep) override {
        VL_RESTORER(m_بلاک_جمع_کړه_Automatic_Stmtp);
        m_بلاک_جمع_کړه_Automatic_Stmtp = تش_اشاره;
        VL_RESTORER(m_بلاک_جمع_کړه_Static_Stmtp);
        m_بلاک_جمع_کړه_Static_Stmtp = تش_اشاره;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_بل_Null(nodep->declsp());
        }

        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_بلاک(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_بل_Null(nodep->stmtsp());
        إذا (AstFork* ثابت forkp = VN_CAST(nodep, Fork)) iterate_And_بل_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_غوټه* stmtsp = تش_اشاره;
        // Move all Vars to front of function
        stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, get_بلاک_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    فراغ لیدنه(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        إذا (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        إذا (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(ریښتیا);
            Ast_غوټه* ثابت nextp = nodep->nextp();
            nodep->unlink_Fr_شا();
            nextp->جمع_کړه_بل(nodep);
        }
    }
    فراغ لیدنه(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_غوټه* scanp = nodep;
            // Skip over the New's statement
            لكل (; scanp && !VN_IS(scanp, Stmt_بیان); scanp = scanp->backp()) {}
            إذا (VN_IS(scanp, Stmt_بیان)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                لكل (; scanp; scanp = scanp->backp()) {
                    إذا (VN_IS(scanp, غوټه_Stmt) || VN_IS(scanp, غوټه_Module)
                        || VN_IS(scanp, غوټه_FTask))
                        اكسر;
                }
                إذا (!VN_IS(scanp, غوټه_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    فراغ لیدنه(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Print_وخت_Scale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::منزل_string(m_rand_Sequence_عدد++));
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(Ast_وخت* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(Ast_وخت_D* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(Ast_وخت_Import* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(Ast_وخت_Unit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    فراغ لیدنه(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ثابت alwaysp = VN_CAST(nodep->backp(), Always);
        إذا (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_نور() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا إذا (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            إذا (Ast_Sen_ونه* ثابت sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_شا_With_بل();
                alwaysp->sentreep(sentreep);
            }
            إذا (nodep->stmtsp()) alwaysp->جمع_کړه_Stmtsp(nodep->stmtsp()->unlink_Fr_شا_With_بل());
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        }
    }
    فراغ لیدنه(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_دننه_Skewp);
        VL_RESTORER(m_default_بهر_Skewp);
        // Find default input and output skews
        لكل (Ast_غوټه *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (Ast_Clocking_توکی* citemp = VN_CAST(itemp, Clocking_توکی)) {
                إذا (citemp->exprp() || citemp->assignp()) استمر;
                إذا (citemp->skewp()) {
                    إذا (citemp->direction() == VDirection::ننوت) {
                        // Disallow default redefinition; note some simulators allow this
                        إذا (m_default_دننه_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_دننه_Skewp = citemp->skewp();
                    } وإلا إذا (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        إذا (m_default_بهر_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_بهر_Skewp = citemp->skewp();
                    } وإلا {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ټېل_Deletep(citemp->unlink_Fr_شا()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Clocking_توکی* nodep) override {
        cleanFileline(nodep);
        إذا (nodep->direction() == VDirection::OUTPUT) {
            إذا (!nodep->skewp()) {
                إذا (m_default_بهر_Skewp) {
                    nodep->skewp(m_default_بهر_Skewp->clone_ونه(خطأ));
                } وإلا {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(جديد AstConst{nodep->fileline(), 0});
                }
            }
        } وإلا إذا (nodep->direction() == VDirection::ننوت) {
            إذا (!nodep->skewp()) {
                إذا (m_default_دننه_Skewp) {
                    nodep->skewp(m_default_دننه_Skewp->clone_ونه(خطأ));
                } وإلا {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(جديد AstConst{nodep->fileline(), AstConst::One_ګام{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        إذا (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    فراغ لیدنه(Ast_Tagged_بیان* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Tagged_نمونه* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_نمونه_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_نمونه_ستوری* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        إذا (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ساكن Ast_غوټه* makeArgMemberCopies(Ast_غوټه* argsp, منطقي readOnlyRefs) {
        Ast_غوټه* resultp = تش_اشاره;
        لكل (Ast_غوټه* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            AstVar* ثابت paramp = origVarp->clone_ونه(خطأ);
            File_کرښه* ثابت fl = جديد File_کرښه{paramp->fileline()};
            fl->warnOff(V3_Error_کوډ::VARHIDDEN, ریښتیا);
            paramp->fileline(fl);
            paramp->funcLocal(ریښتیا);
            paramp->direction(origVarp->direction());
            إذا (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_غوټه::جمع_کړه_بل_Null(resultp, paramp);
            Ast_غوټه_بیان* ثابت lhsp = جديد AstDot{
                origVarp->fileline(), خطأ, جديد AstParseRef{origVarp->fileline(), "this"},
                جديد AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_غوټه_بیان* ثابت rhsp = جديد AstParseRef{paramp->fileline(), paramp->name()};
            resultp->جمع_کړه_بل(جديد Ast_ټاکل{origVarp->fileline(), lhsp, rhsp});
        }
        أرجع resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ساكن فراغ createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_غوټه* argsp,
                                        Ast_غوټه* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ثابت defaultVarp
            = جديد AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->پیدا_کړه_Int_DType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->جمع_کړه_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        إذا (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, ریښتیا));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ثابت varp
                = جديد AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlag_ماشوم_DType{},
                             جديد AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             جديد AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      تش_اشاره, تش_اشاره},
                                             تش_اشاره}};
            nodep->جمع_کړه_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ثابت varp
                = جديد AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlag_ماشوم_DType{},
                             جديد AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             جديد AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      تش_اشاره, تش_اشاره},
                                             تش_اشاره}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->جمع_کړه_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ثابت funcp = جديد AstFunc{nodep->fileline(), "sample", تش_اشاره, تش_اشاره};
            funcp->جمع_کړه_Stmtsp(makeArgMemberCopies(sampleArgsp, خطأ));
            funcp->class_لاره(ریښتیا);
            funcp->dtypep(funcp->پیدا_کړه_Void_DType());
            funcp->keepAlive(ریښتیا);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->جمع_کړه_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        لكل (ثابت string& name : {"start"s, "stop"s}) {
            AstFunc* ثابت funcp = جديد AstFunc{nodep->fileline(), name, تش_اشاره, تش_اشاره};
            funcp->class_لاره(ریښتیا);
            funcp->dtypep(funcp->پیدا_کړه_Void_DType());
            nodep->جمع_کړه_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        لكل (ثابت string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ثابت funcp = جديد AstFunc{nodep->fileline(), name, تش_اشاره, تش_اشاره};
            funcp->fileline()->warnOff(V3_Error_کوډ::NORETURN, ریښتیا);
            funcp->isStatic(name == "get_coverage");
            funcp->class_لاره(ریښتیا);
            funcp->dtypep(funcp->پیدا_کړه_Void_DType());
            nodep->جمع_کړه_Membersp(funcp);
            {
                AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->پیدا_کړه_Double_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ریښتیا);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(ریښتیا);
                funcp->fvarp(varp);
            }
            لكل (ثابت string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->پیدا_کړه_String_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ریښتیا);
                varp->direction(VDirection::ننوت);
                varp->valuep(جديد AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->جمع_کړه_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ثابت funcp
                = جديد AstFunc{nodep->fileline(), "set_inst_name", تش_اشاره, تش_اشاره};
            funcp->class_لاره(ریښتیا);
            funcp->dtypep(funcp->پیدا_کړه_Void_DType());
            nodep->جمع_کړه_Membersp(funcp);
            AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->پیدا_کړه_String_DType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ریښتیا);
            varp->direction(VDirection::ننوت);
            funcp->جمع_کړه_Stmtsp(varp);
        }
    }

    منطقي dropDeprecatedCoverageOption(Ast_Cg_Option_ټاکل* ثابت nodep) {
        إذا (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) أرجع خطأ;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        أرجع ریښتیا;
    }

    فراغ لیدنه(Ast_Cg_Option_ټاکل* nodep) override {
        إذا (dropDeprecatedCoverageOption(nodep)) أرجع;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        إذا (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) أرجع;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        إذا (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ثابت string libname = m_modp->libname();
        AstClass* ثابت cgClassp = جديد AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
        cgClassp->fileline()->warnOff(V3_Error_کوډ::UNDRIVEN, ریښتیا);
        cgClassp->isCovergroup(ریښتیا);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(ریښتیا);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        إذا (Ast_Sen_ونه* ثابت eventp = nodep->eventp()) {
            eventp->unlink_Fr_شا();
            AstCovergroup* ثابت cgNodep = جديد AstCovergroup{
                nodep->fileline(), nodep->name(), تش_اشاره, تش_اشاره, تش_اشاره, eventp};
            cgClassp->جمع_کړه_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        لكل (Ast_غوټه* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            إذا (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ننوت);
            }
            إذا ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ثابت memberp = origVarp->clone_ونه(خطأ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(خطأ);
            memberp->direction(VDirection::NONE);
            إذا (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(ریښتیا);
            cgClassp->جمع_کړه_Membersp(memberp);
        }

        // Convert sample args to member variables
        لكل (Ast_غوټه* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            إذا (!origVarp->is_ننوت()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ننوت);
            }
            AstVar* ثابت memberp = origVarp->clone_ونه(خطأ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(خطأ);
            memberp->direction(VDirection::NONE);
            cgClassp->جمع_کړه_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ثابت newFuncp = جديد AstFunc{nodep->fileline(), "new", تش_اشاره, تش_اشاره};
        newFuncp->fileline()->warnOff(V3_Error_کوډ::NORETURN, ریښتیا);
        newFuncp->class_لاره(ریښتیا);
        newFuncp->isConstructor(ریښتیا);
        newFuncp->dtypep(cgClassp->dtypep());
        إذا (Ast_غوټه* ثابت bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_شا_With_بل();
            newFuncp->جمع_کړه_Stmtsp(bodyp);
        }
        cgClassp->جمع_کړه_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_عدد);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_عدد = 0;
        لكل (ثابت Ast_غوټه* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstCoverpoint* ثابت pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ساكن string coverpointName(ثابت AstCoverpoint* nodep) {
        إذا (!nodep->name().تشه()) أرجع nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ثابت AstParseRef* ثابت refp = VN_CAST(nodep->exprp(), ParseRef);
        أرجع refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ثابت AstCoverpoint* پیدا_کړه_Coverpoint(ثابت AstCoverpointRef* refp) ثابت {
        إذا (refp->exprp()) أرجع تش_اشاره;  // A hierarchical reference, unsupported
        ثابت خپلکاری it = m_coverpoints.پیدا_کړه(refp->name());
        أرجع it == m_coverpoints.پای_ټکی() ? تش_اشاره : it->دوهم;
    }

    فراغ لیدنه(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        إذا (nodep->name().تشه()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ثابت string name = coverpointName(nodep);
            nodep->name(name.تشه() ? "__Vcoverpoint" + cvt_منزل_Str(m_coverpoint_عدد++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        لكل (Ast_غوټه *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (Ast_Cg_Option_ټاکل* ثابت optp = VN_CAST(itemp, Cg_Option_ټاکل)) {
                إذا (dropDeprecatedCoverageOption(optp)) استمر;
                optp->unlink_Fr_شا();
                إذا (optp->optType() == VCoverOptionType::AT_لږترلږه
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::وزن) {
                    nodep->جمع_کړه_Optionsp(جديد AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_ونه(خطأ)});
                } وإلا {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_ونه(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        إذا (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        إذا (ثابت AstCoverpointRef* ثابت refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            إذا (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_کرښه* ثابت fl = nodep->fileline();
        File_کرښه* ثابت flNoWarn = جديد File_کرښه{fl};
        flNoWarn->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
        flNoWarn->modifyWarnOff(V3_Error_کوډ::VARHIDDEN, ریښتیا);
        AstVar* ثابت varp = جديد AstVar{flNoWarn, VVarType::VAR, "item", VFlag_ماشوم_DType{},
                                        جديد AstRefDType{fl, AstRefDType::Flag_Type_د_بیان{},
                                                        m_coverpointp->exprp()->clone_ونه(خطأ)}};
        varp->funcLocal(ریښتیا);
        varp->noReset(ریښتیا);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ثابت refp = nodep->pointp();
        ثابت AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ثابت AstParseRef* binp = تش_اشاره;
        ثابت AstDot* ثابت dotp = VN_CAST(refp->exprp(), Dot);
        إذا (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        إذا (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        إذا (binp) nodep->name(binp->name());
        refp->replaceWith(جديد AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ټېل_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        }
    }

    فراغ لیدنه(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        إذا (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        }
    }

    فراغ لیدنه(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        لكل (Ast_غوټه *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (VN_IS(itemp, CoverCrossBin)) استمر;
            Ast_Cg_Option_ټاکل* ثابت optp = VN_AS(itemp, Cg_Option_ټاکل);
            إذا (dropDeprecatedCoverageOption(optp)) استمر;
            itemp->unlink_Fr_شا();
            ثابت VCoverOptionType optType = optp->optType();
            إذا (!(optType == VCoverOptionType::AT_لږترلږه || optType == VCoverOptionType::وزن)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->جمع_کړه_Optionsp(جديد AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_ونه(خطأ)});
            VL_DO_DANGLING(optp->delete_ونه(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        إذا (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_کرښه* ثابت fl = nodep->fileline();
            File_کرښه* ثابت flNoWarn = جديد File_کرښه{fl};
            flNoWarn->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
            flNoWarn->modifyWarnOff(V3_Error_کوډ::VARHIDDEN, ریښتیا);
            std::سېټ<string> نومونه;
            لكل (ثابت Ast_غوټه* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                ثابت AstCoverpoint* ثابت pointp = پیدا_کړه_Coverpoint(refp);
                إذا (!pointp || !نومونه.ورننباسه(refp->name()).دوهم) استمر;
                AstVar* ثابت varp
                    = جديد AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlag_ماشوم_DType{},
                                 جديد AstRefDType{fl, AstRefDType::Flag_Type_د_بیان{},
                                                 pointp->exprp()->clone_ونه(خطأ)}};
                varp->funcLocal(ریښتیا);
                varp->noReset(ریښتیا);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->جمع_کړه_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        إذا (!nodep->selectp()) VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(AstSystemCSection* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process only once.
        cleanFileline(nodep);
        m_modp->جمع_کړه_Sc_Sectionsp(nodep->unlink_Fr_شا());
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::جمع_کړه_Stat_مجموعه(V3Stats::STAT_سرچینه_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

فراغ V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("linkparse", 0, dump_ونه_Either_Level() >= 6);
}
