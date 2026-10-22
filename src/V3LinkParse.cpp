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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkParse.h"

#شامل "V3Control.h"
#شامل "V3Stats.h"

#شامل <set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

جماعت LinkParseVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // TYPES
    استعمال ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_سیٹ<File_سطر*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = خالی_اشاریہ;  // Variable we're under
    Ast_گرہ_Module* m_قدر_Modp = خالی_اشاریہ;
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    منطقی m_اندر_Interface = جھوٹ;  // True when inside interface declaration
    Ast_گرہ_Procedure* m_procedurep = خالی_اشاریہ;  // Current procedure
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current task
    AstRSProd* m_rsProdp = خالی_اشاریہ;  // Current randsequence production
    Ast_گرہ_بلاک* m_blockp = خالی_اشاریہ;  // Current AstNodeBlock
    Ast_گرہ_Stmt* m_بلاک_جوڑو_Automatic_Stmtp = خالی_اشاریہ;  // Initial statements to add to block
    Ast_گرہ_Stmt* m_بلاک_جوڑو_Static_Stmtp = خالی_اشاریہ;  // Initial statements to add to block
    Ast_گرہ_DType* m_dtypep = خالی_اشاریہ;  // Current data type
    Ast_گرہ_اظہاریہ* m_default_اندر_Skewp = خالی_اشاریہ;  // Current default input skew
    Ast_گرہ_اظہاریہ* m_default_برآمد_Skewp = خالی_اشاریہ;  // Current default output skew
    AstCoverpoint* m_coverpointp = خالی_اشاریہ;  // Current coverpoint
    AstCoverCross* m_crossp = خالی_اشاریہ;  // Current cross
    std::map<string, ثابت AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    صحیح m_anonUdpId = 0;  // Counter for anonymous UDP instances
    صحیح m_coverpoint_عدد = 0;  // Counter for unnamed coverpoints within current covergroup
    صحیح m_genblkAbove = 0;  // Begin block number of if/case/for above
    صحیح m_genblk_عدد = 0;  // Begin block number, 0=none seen
    صحیح m_begin_گہرائی = 0;  // How many begin blocks above current node within current AstNodeModule
    صحیح m_rand_Sequence_عدد = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    منطقی m_lifetimeAllowed = جھوٹ;  // True to allow lifetime settings
    منطقی m_hasTimingControl = جھوٹ;  // If current task has timing control
    منطقی m_moduleWithGenericIface = جھوٹ;  // If current module contains generic interface
    std::سیٹ<AstVar*> m_پورٹ_Dups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    خالی cleanFileline(Ast_گرہ* nodep) {
        اگر (nodep->user2_سیٹ_Once()) واپس;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        اگر (m_filelines.ڈھونڈو(nodep->fileline()) != m_filelines.انتہا()) {
            nodep->fileline(نیا File_سطر{nodep->fileline()});
        }
        m_filelines.داخل_کرو(nodep->fileline());
    }

    string nameFromTypedef(Ast_گرہ* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        اگر (ثابت AstTypedef* ثابت typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            اگر (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } ورنہ اگر (m_modp) {
                above = m_modp->name() + ".";
            }
            واپس above + typedefp->name();
        }
        واپس "";
    }

    خالی دورہ_Iterate_گرہ_DType(Ast_گرہ_DType* nodep) {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    منطقی nestedIfBegin(Ast_Gen_بلاک* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        واپس (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, Gen_Case_شے))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    خالی checkIndent(Ast_گرہ* nodep, Ast_گرہ* childp) {
        // Try very hard to avoid false positives
        Ast_گرہ* nextp = nodep->nextp();
        اگر (!childp) واپس;
        اگر (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        اگر (!nextp) واپس;
        اگر (VN_IS(childp, Begin) || VN_IS(childp, Gen_بلاک)) واپس;
        File_سطر* ثابت گرہ_Flp = nodep->fileline();
        File_سطر* ثابت بچہ_Flp = childp->fileline();
        File_سطر* ثابت اگلا_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        اگر (گرہ_Flp->filenameno() != بچہ_Flp->filenameno()) واپس;
        اگر (گرہ_Flp->filenameno() != اگلا_Flp->filenameno()) واپس;
        اگر (گرہ_Flp->آخری_Lineno() >= بچہ_Flp->پہلا_Lineno()) واپس;
        اگر (بچہ_Flp->آخری_Lineno() >= اگلا_Flp->پہلا_Lineno()) واپس;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        اگر (!(گرہ_Flp->پہلا_عمود() < بچہ_Flp->پہلا_عمود()
              && اگلا_Flp->پہلا_عمود() >= بچہ_Flp->پہلا_عمود()))
            واپس;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ثابت std::string گرہ_سابقہ = گرہ_Flp->ماخذ_سابقہ(گرہ_Flp->پہلا_عمود());
        ثابت std::string بچہ_سابقہ = بچہ_Flp->ماخذ_سابقہ(گرہ_Flp->پہلا_عمود());
        ثابت std::string اگلا_سابقہ = اگلا_Flp->ماخذ_سابقہ(گرہ_Flp->پہلا_عمود());
        اگر (بچہ_سابقہ != گرہ_سابقہ) واپس;
        اگر (اگلا_سابقہ != بچہ_سابقہ) واپس;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ثابت std::string گرہ_ماخذ = گرہ_Flp->ماخذ();
        ثابت std::string بچہ_ماخذ = بچہ_Flp->ماخذ();
        ثابت std::string اگلا_ماخذ = اگلا_Flp->ماخذ();
        اگر (!(VString::leading_Whitespace_گنتی(گرہ_ماخذ)
                  < VString::leading_Whitespace_گنتی(بچہ_ماخذ)
              && VString::leading_Whitespace_گنتی(اگلا_ماخذ)
                     >= VString::leading_Whitespace_گنتی(بچہ_ماخذ)))
            واپس;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    خالی collectPorts(Ast_گرہ* گرہ_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> پورٹ_نام;
        برائے (Ast_گرہ* nodep = گرہ_Listp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isIO()) پورٹ_نام.emplace(varp->name(), varp);
            }
        }
        برائے (Ast_گرہ* nodep = گرہ_Listp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isIO()) جاری;
                ثابت خودکار it = پورٹ_نام.ڈھونڈو(varp->name());
                اگر (it == پورٹ_نام.انتہا()) جاری;
                AstVar* ثابت iop = it->دوسرا;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                اگر (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_عقب_With_اگلا());
                m_پورٹ_Dups.emplace(varp);
            }
        }
    }

    Ast_گرہ* getVarsAndUnlink(Ast_گرہ* stmtsp) {
        Ast_گرہ* varsp = خالی_اشاریہ;
        برائے (Ast_گرہ *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (VN_IS(itemp, Var)) varsp = Ast_گرہ::جوڑو_اگلا(varsp, itemp->unlink_Fr_عقب());
        }
        واپس varsp;
    }
    Ast_گرہ_Stmt* get_بلاک_Adds() {
        Ast_گرہ_Stmt* addsp = خالی_اشاریہ;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        اگر (m_بلاک_جوڑو_Static_Stmtp)
            addsp = Ast_گرہ::جوڑو_اگلا(addsp,
                                     نیا AstInitialStaticStmt{m_بلاک_جوڑو_Static_Stmtp->fileline(),
                                                              m_بلاک_جوڑو_Static_Stmtp});
        اگر (m_بلاک_جوڑو_Automatic_Stmtp)
            addsp = Ast_گرہ::جوڑو_اگلا(
                addsp, نیا AstInitialAutomaticStmt{m_بلاک_جوڑو_Automatic_Stmtp->fileline(),
                                                   m_بلاک_جوڑو_Automatic_Stmtp});
        m_بلاک_جوڑو_Static_Stmtp = خالی_اشاریہ;
        m_بلاک_جوڑو_Automatic_Stmtp = خالی_اشاریہ;
        واپس addsp;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process only once.
        // Mark class methods
        اگر (VN_IS(m_modp, Class)) nodep->class_طریقہ(سچ);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_بلاک_جوڑو_Automatic_Stmtp);
        m_بلاک_جوڑو_Automatic_Stmtp = خالی_اشاریہ;
        VL_RESTORER(m_بلاک_جوڑو_Static_Stmtp);
        m_بلاک_جوڑو_Static_Stmtp = خالی_اشاریہ;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = سچ;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ثابت Ast_گرہ* ثابت nodep) { واپس nodep->isTimingControl(); });
        اگر (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } ورنہ {
            اگر (nodep->class_طریقہ()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } ورنہ اگر (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        اگر (nodep->class_طریقہ() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warn_مزید() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_پورٹ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_گرہ* stmtsp = خالی_اشاریہ;
        اگر (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlink_Fr_عقب();
        // Move all Vars to front of function
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, get_بلاک_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    خالی دورہ(Ast_گرہ_DType* nodep) override { دورہ_Iterate_گرہ_DType(nodep); }
    خالی دورہ(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(سچ);
        iterateChildren(nodep);
    }
    خالی دورہ(AstConstraintForeach* nodep) override {
        اگر (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_اظہاریہ* exprp) { exprp->isSoft(سچ); });
        }
    }
    خالی دورہ(AstEnumDType* nodep) override {
        اگر (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        دورہ_Iterate_گرہ_DType(nodep);
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        اگر (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        دورہ_Iterate_گرہ_DType(nodep);
    }
    خالی دورہ(Ast_Enum_شے* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->rangep()) {
            اگر (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t انحراف_from_init = 0;
            Ast_Enum_شے* addp = خالی_اشاریہ;
            File_سطر* ثابت flp = nodep->fileline();
            برائے (ثابت صحیح i : nodep->rangep()->seq_بایاں_مقصد_دایاں()) {
                ثابت string name = nodep->name() + cvt_مقصد_Str(i);
                Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;
                اگر (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = نیا Ast_جوڑو{flp, nodep->valuep()->clone_درخت(سچ),
                                     نیا AstConst{flp, AstConst::Unsized32{}, انحراف_from_init}};
                }
                addp = Ast_گرہ::جوڑو_اگلا(addp, نیا Ast_Enum_شے{flp, name, خالی_اشاریہ, valuep});
                ++انحراف_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }

    خالی دورہ(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
        اگر (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(سچ);
        }
        اگر (nodep->embeddedCovergroup()) {
            AstRefDType* ثابت refp = VN_AS(nodep->بچہ_DTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        اگر (nodep->valuep()) nodep->hasUserInit(سچ);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        اگر (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            اگر (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warn_مزید() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warn_مزید() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } ورنہ {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warn_مزید() << "... The initializer value will only be set once\n"
                        << nodep->warn_مزید()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        اگر (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        اگر (!nodep->direction().isAny()) {  // Not a port
            اگر (nodep->lifetime().isNone()) {
                اگر (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } ورنہ {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } ورنہ اگر (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            اگر (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } ورنہ {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } ورنہ اگر (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        اگر (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            اگر (m_begin_گہرائی > 0
                || (m_begin_گہرائی == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        اگر (nodep->isGParam() && m_modp) m_modp->hasGParam(سچ);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ثابت AstBasicDType* ثابت basicp = VN_CAST(nodep->بچہ_DTypep(), BasicDType);
        اگر (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(سچ);

        اگر (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3_Lang_رمز::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        اگر (nodep->isParam() && m_اندر_Interface) nodep->isIfaceParam(سچ);
        اگر (AstParseTypeDType* ثابت ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_گرہ* dtypep = nodep->valuep();
            اگر (dtypep) {
                dtypep->unlink_Fr_عقب();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                اگر (Ast_گرہ_اظہاریہ* exprp = VN_CAST(dtypep, گرہ_اظہاریہ)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    جب (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        اگر (!rhsDotp) توڑو;
                        File_سطر* ثابت fl = dotp->fileline();
                        ثابت منطقی colon = dotp->colon();
                        Ast_گرہ_اظہاریہ* ثابت lhs = VN_AS(dotp->lhsp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                        Ast_گرہ_اظہاریہ* ثابت rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                        Ast_گرہ_اظہاریہ* ثابت rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                        File_سطر* ثابت rhsFl = rhsDotp->fileline();
                        ثابت منطقی rhsColon = rhsDotp->colon();
                        AstDot* ثابت newLhs = نیا AstDot{fl, colon, lhs, rhsLhs};
                        exprp = نیا AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_درخت(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } ورنہ {
                dtypep = نیا AstVoidDType{nodep->fileline()};
            }

            Ast_گرہ* ثابت newp = نیا AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlag_بچہ_DType{}, نیا AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            واپس;
        }
        ثابت Ast_گرہ_DType* ثابت dtypep = nodep->بچہ_DTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(جھوٹ), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        اگر (v3Global.opt.any_عوامی_Flat() && nodep->varType().isVPIAccessible()) {
            اگر (v3Global.opt.عوامی_Flat_RW()) {
                nodep->sigUserRWPublic(سچ);
            } ورنہ اگر (v3Global.opt.عوامی_Params() && nodep->isParam()) {
                nodep->sigUserRWPublic(سچ);
            } ورنہ اگر (m_modp && v3Global.opt.عوامی_گہرائی()) {
                اگر ((m_modp->گہرائی() - 1) <= v3Global.opt.عوامی_گہرائی()) {
                    nodep->sigUserRWPublic(سچ);
                } ورنہ اگر (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(سچ);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        اگر (v3Global.opt.trace_گہرائی() && m_modp
            && (m_modp->گہرائی() - 1) > v3Global.opt.trace_گہرائی()) {
            m_modp->modTrace(جھوٹ);
            nodep->trace(جھوٹ);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = خالی_اشاریہ;
        // temporaries under an always aren't expected to be blocking
        اگر (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3_Error_رمز::BLKSEQ, سچ);
        // Compute initial value
        اگر (nodep->valuep() || nodep->needsCReset()) {
            File_سطر* ثابت fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            خودکار createValuep = [&]() -> Ast_گرہ_اظہاریہ* {
                اگر (nodep->valuep()) {
                    واپس VN_AS(nodep->valuep()->unlink_Fr_عقب(), گرہ_اظہاریہ);
                } ورنہ {
                    واپس نیا AstCReset{fl, nodep, جھوٹ};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            اگر (m_پورٹ_Dups.ڈھونڈو(nodep) != m_پورٹ_Dups.انتہا()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } ورنہ اگر (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } ورنہ اگر (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_داخلہ()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                اگر (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlink_Fr_عقب()->delete_درخت();
                }
            } ورنہ اگر (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(سچ);
                File_سطر* ثابت newfl = نیا File_سطر{fl};
                newfl->warnOff(V3_Error_رمز::E_CONSTWRITTEN, سچ);
                Ast_تفویض* ثابت assp
                    = نیا Ast_تفویض{newfl, نیا AstParseRef{newfl, nodep->name()}, createValuep()};
                اگر (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_بلاک_جوڑو_Automatic_Stmtp = Ast_گرہ::جوڑو_اگلا(m_بلاک_جوڑو_Automatic_Stmtp, assp);
                } ورنہ {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_بلاک_جوڑو_Static_Stmtp = Ast_گرہ::جوڑو_اگلا(m_بلاک_جوڑو_Static_Stmtp, assp);
                }
            } ورنہ اگر (m_قدر_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                اگر (nodep->valuep()) {
                    nodep->noCReset(سچ);
                    File_سطر* ثابت newfl = نیا File_سطر{fl};
                    newfl->warnOff(V3_Error_رمز::PROCASSWIRE, سچ);
                    newfl->warnOff(V3_Error_رمز::E_CONSTWRITTEN, سچ);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    Ast_تفویض* ثابت assp = نیا Ast_تفویض{
                        newfl, نیا AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_گرہ* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    اگر (nodep->lifetime().isAutomatic()) {
                        newInitp = نیا AstInitialAutomatic{newfl, assp};
                    } ورنہ {
                        newInitp = نیا AstInitialStatic{newfl, assp};
                    }
                    nodep->جوڑو_اگلا_یہاں(newInitp);
                }
            } ورنہ اگر (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ساکن منطقی isNegative(ثابت AstConst* constp) {
        واپس constp->isSigned() && constp->عدد().ثنائی_ہندسہ_Is1(constp->چوڑائی() - 1);
    }
    خالی دورہ(AstRange* nodep) override {
        ساکن constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ثابت AstConst* ثابت leftp = VN_CAST(nodep->leftp(), Const);
        ثابت AstConst* ثابت rightp = VN_CAST(nodep->rightp(), Const);
        اگر (leftp && rightp && leftp->چوڑائی() <= 64 && rightp->چوڑائی() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ثابت uint64_t بائیں = leftp->مقصد_UQuad();
            ثابت uint64_t دائیں = rightp->مقصد_UQuad();
            ثابت uint64_t وسعت = std::max(بائیں, دائیں) - std::min(بائیں, دائیں);
            اگر (وسعت >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << وسعت + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstConst* nodep) override {
        اگر (nodep->عدد().autoExtend() && nodep->fileline()->language() < V3_Lang_رمز::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    خالی دورہ(Ast_Attr_کا* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->attrType() == VAttrType::DT_عوامی) {
            AstTypedef* ثابت typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attr_عوامی(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->سیٹ_Forceable();
            v3Global.سیٹ_Has_Forceable_اشارات();
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_عوامی) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(سچ);
            m_varp->sig_Mod_عوامی(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_عوامی_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_عوامی_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sig_User_Rd_عوامی(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_عوامی_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_بانٹو_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            اگر (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } ورنہ {
                m_varp->attr_بانٹو_Var(سچ);
            }
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_بھرا) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::بھرا);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_بھرا) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(سچ);
            m_varp->attrFsmStateExpand(VFsmExpandType::بھرا);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
    }

    خالی دورہ(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = خالی_اشاریہ;
        ثابت ImplTypedefMap::iterator it = m_implTypedef.ڈھونڈو(nodep->name());
        اگر (it != m_implTypedef.انتہا()) {
            defp = it->دوسرا;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } ورنہ {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_گرہ* backp = nodep->backp();
            برائے (; backp; backp = backp->backp()) {
                اگر (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, گرہ_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    توڑو;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_گرہ_DType* ثابت dtypep = nodep->بچہ_DTypep();
            dtypep->unlink_Fr_عقب();
            اگر (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                واپس;
            } ورنہ {
                defp = نیا AstTypedef{nodep->fileline(), nodep->name(), خالی_اشاریہ, VFlag_بچہ_DType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvt_مقصد_Str(m_implTypedef.سائز()));
                UINFO(9, "New impltypedef " << defp);
                backp->جوڑو_اگلا_یہاں(defp);
            }
        }
        nodep->replaceWith(نیا AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    خالی دورہ(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_Foreach* nodep) override {
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
        Ast_Foreach_سرخی* ثابت headerp = nodep->headerp();
        اگر (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    خالی دورہ(AstLoop* nodep) override {
        cleanFileline(nodep);
        اگر (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } ورنہ {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_سطر* ثابت newfl = nodep->fileline();
            newfl->warnOff(V3_Error_رمز::WAITCONST, سچ);
            nodep->fileline(newfl);
        }
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        اگر (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_اندر_Interface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_عدد);
        VL_RESTORER(m_begin_گہرائی);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_عدد);
        VL_RESTORER(m_قدر_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        اگر (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        اگر (VN_IS(nodep, Iface)) m_اندر_Interface = سچ;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_عدد = 0;
        m_begin_گہرائی = 0;
        m_قدر_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = جھوٹ;
        m_rand_Sequence_عدد = 0;

        اگر (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        اگر (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_پورٹ_Dups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        اگر (AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    خالی دورہ_Iterate_No_قدر_Mod(Ast_گرہ* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_قدر_Modp);
        m_قدر_Modp = خالی_اشاریہ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = سچ;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        دورہ_Iterate_No_قدر_Mod(nodep);
    }
    خالی دورہ(AstCover* nodep) override { دورہ_Iterate_No_قدر_Mod(nodep); }
    خالی دورہ(AstRestrict* nodep) override { دورہ_Iterate_No_قدر_Mod(nodep); }

    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        V3Control::apply_Coverage_بلاک(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_begin_گہرائی);
        ++m_begin_گہرائی;
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ثابت منطقی nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        صحیح تفویض_Gen_Blk_عدد = -1;
        اگر (nodep->genforp()) {
            ++m_genblk_عدد;
            اگر (nodep->name() == "") تفویض_Gen_Blk_عدد = m_genblk_عدد;
        } ورنہ اگر (nodep->name() == "" && (VN_IS(backp, Gen_Case_شے) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            تفویض_Gen_Blk_عدد = m_genblkAbove;
        }
        اگر (تفویض_Gen_Blk_عدد != -1) {
            nodep->name("genblk" + cvt_مقصد_Str(تفویض_Gen_Blk_عدد));
            اگر (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warn_مزید()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        اگر (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_عدد);
            m_genblkAbove = 0;
            m_genblk_عدد = 0;
            iterateChildren(nodep);
        } ورنہ {
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstGenCase* nodep) override {
        ++m_genblk_عدد;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_عدد);
        m_genblkAbove = m_genblk_عدد;
        m_genblk_عدد = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ثابت منطقی nestedIf = (VN_IS(nodep->backp(), Gen_بلاک)
                               && nestedIfBegin(VN_CAST(nodep->backp(), Gen_بلاک)));
        اگر (nestedIf) {
            iterateChildren(nodep);
        } ورنہ {
            ++m_genblk_عدد;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_عدد);
            m_genblkAbove = m_genblk_عدد;
            m_genblk_عدد = 0;
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstCell* nodep) override {
        اگر (nodep->origName().خالی_خانے()) {
            اگر (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ثابت string newName = "$unnamedudp" + cvt_مقصد_Str(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_بلاک* nodep) override {
        VL_RESTORER(m_بلاک_جوڑو_Automatic_Stmtp);
        m_بلاک_جوڑو_Automatic_Stmtp = خالی_اشاریہ;
        VL_RESTORER(m_بلاک_جوڑو_Static_Stmtp);
        m_بلاک_جوڑو_Static_Stmtp = خالی_اشاریہ;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_اگلا_Null(nodep->declsp());
        }

        اگر (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            V3Control::apply_Coverage_بلاک(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_اگلا_Null(nodep->stmtsp());
        اگر (AstFork* ثابت forkp = VN_CAST(nodep, Fork)) iterate_And_اگلا_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_گرہ* stmtsp = خالی_اشاریہ;
        // Move all Vars to front of function
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, get_بلاک_Adds());
        nodep->prependStmtsp(stmtsp);
    }
    خالی دورہ(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        اگر (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        اگر (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(سچ);
            Ast_گرہ* ثابت nextp = nodep->nextp();
            nodep->unlink_Fr_عقب();
            nextp->جوڑو_اگلا(nodep);
        }
    }
    خالی دورہ(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_گرہ* scanp = nodep;
            // Skip over the New's statement
            برائے (; scanp && !VN_IS(scanp, Stmt_اظہاریہ); scanp = scanp->backp()) {}
            اگر (VN_IS(scanp, Stmt_اظہاریہ)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                برائے (; scanp; scanp = scanp->backp()) {
                    اگر (VN_IS(scanp, گرہ_Stmt) || VN_IS(scanp, گرہ_Module)
                        || VN_IS(scanp, گرہ_FTask))
                        توڑو;
                }
                اگر (!VN_IS(scanp, گرہ_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    خالی دورہ(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Print_وقت_Scale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::مقصد_string(m_rand_Sequence_عدد++));
        iterateChildren(nodep);
    }
    خالی دورہ(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(Ast_وقت* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(Ast_وقت_D* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(Ast_وقت_Import* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(Ast_وقت_Unit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    خالی دورہ(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ثابت alwaysp = VN_CAST(nodep->backp(), Always);
        اگر (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warn_مزید() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ اگر (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            اگر (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_عقب_With_اگلا();
                alwaysp->sentreep(sentreep);
            }
            اگر (nodep->stmtsp()) alwaysp->جوڑو_Stmtsp(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
    }
    خالی دورہ(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_default_اندر_Skewp);
        VL_RESTORER(m_default_برآمد_Skewp);
        // Find default input and output skews
        برائے (Ast_گرہ *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (Ast_Clocking_شے* citemp = VN_CAST(itemp, Clocking_شے)) {
                اگر (citemp->exprp() || citemp->assignp()) جاری;
                اگر (citemp->skewp()) {
                    اگر (citemp->direction() == VDirection::داخلہ) {
                        // Disallow default redefinition; note some simulators allow this
                        اگر (m_default_اندر_Skewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_default_اندر_Skewp = citemp->skewp();
                    } ورنہ اگر (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        اگر (m_default_برآمد_Skewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_default_برآمد_Skewp = citemp->skewp();
                    } ورنہ {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ڈالو_Deletep(citemp->unlink_Fr_عقب()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Clocking_شے* nodep) override {
        cleanFileline(nodep);
        اگر (nodep->direction() == VDirection::OUTPUT) {
            اگر (!nodep->skewp()) {
                اگر (m_default_برآمد_Skewp) {
                    nodep->skewp(m_default_برآمد_Skewp->clone_درخت(جھوٹ));
                } ورنہ {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(نیا AstConst{nodep->fileline(), 0});
                }
            }
        } ورنہ اگر (nodep->direction() == VDirection::داخلہ) {
            اگر (!nodep->skewp()) {
                اگر (m_default_اندر_Skewp) {
                    nodep->skewp(m_default_اندر_Skewp->clone_درخت(جھوٹ));
                } ورنہ {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(نیا AstConst{nodep->fileline(), AstConst::One_قدم{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        اگر (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    خالی دورہ(Ast_Tagged_اظہاریہ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Tagged_نمونہ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_نمونہ_Var* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_نمونہ_ستارہ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        اگر (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ساکن Ast_گرہ* makeArgMemberCopies(Ast_گرہ* argsp, منطقی readOnlyRefs) {
        Ast_گرہ* resultp = خالی_اشاریہ;
        برائے (Ast_گرہ* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            AstVar* ثابت paramp = origVarp->clone_درخت(جھوٹ);
            File_سطر* ثابت fl = نیا File_سطر{paramp->fileline()};
            fl->warnOff(V3_Error_رمز::VARHIDDEN, سچ);
            paramp->fileline(fl);
            paramp->funcLocal(سچ);
            paramp->direction(origVarp->direction());
            اگر (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_گرہ::جوڑو_اگلا_Null(resultp, paramp);
            Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstDot{
                origVarp->fileline(), جھوٹ, نیا AstParseRef{origVarp->fileline(), "this"},
                نیا AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstParseRef{paramp->fileline(), paramp->name()};
            resultp->جوڑو_اگلا(نیا Ast_تفویض{origVarp->fileline(), lhsp, rhsp});
        }
        واپس resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ساکن خالی createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_گرہ* argsp,
                                        Ast_گرہ* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ثابت defaultVarp
            = نیا AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->ڈھونڈو_Int_DType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->جوڑو_Stmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        اگر (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, سچ));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ثابت varp
                = نیا AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlag_بچہ_DType{},
                             نیا AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             نیا AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      خالی_اشاریہ, خالی_اشاریہ},
                                             خالی_اشاریہ}};
            nodep->جوڑو_Membersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ثابت varp
                = نیا AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlag_بچہ_DType{},
                             نیا AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             نیا AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      خالی_اشاریہ, خالی_اشاریہ},
                                             خالی_اشاریہ}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->جوڑو_Membersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ثابت funcp = نیا AstFunc{nodep->fileline(), "sample", خالی_اشاریہ, خالی_اشاریہ};
            funcp->جوڑو_Stmtsp(makeArgMemberCopies(sampleArgsp, جھوٹ));
            funcp->class_طریقہ(سچ);
            funcp->dtypep(funcp->ڈھونڈو_Void_DType());
            funcp->keepAlive(سچ);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->جوڑو_Membersp(funcp);
        }

        // IEEE: function void start(), void stop()
        برائے (ثابت string& name : {"start"s, "stop"s}) {
            AstFunc* ثابت funcp = نیا AstFunc{nodep->fileline(), name, خالی_اشاریہ, خالی_اشاریہ};
            funcp->class_طریقہ(سچ);
            funcp->dtypep(funcp->ڈھونڈو_Void_DType());
            nodep->جوڑو_Membersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        برائے (ثابت string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ثابت funcp = نیا AstFunc{nodep->fileline(), name, خالی_اشاریہ, خالی_اشاریہ};
            funcp->fileline()->warnOff(V3_Error_رمز::NORETURN, سچ);
            funcp->isStatic(name == "get_coverage");
            funcp->class_طریقہ(سچ);
            funcp->dtypep(funcp->ڈھونڈو_Void_DType());
            nodep->جوڑو_Membersp(funcp);
            {
                AstVar* ثابت varp = نیا AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->ڈھونڈو_Double_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(سچ);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(سچ);
                funcp->fvarp(varp);
            }
            برائے (ثابت string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ثابت varp = نیا AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->ڈھونڈو_String_DType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(سچ);
                varp->direction(VDirection::داخلہ);
                varp->valuep(نیا AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->جوڑو_Stmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ثابت funcp
                = نیا AstFunc{nodep->fileline(), "set_inst_name", خالی_اشاریہ, خالی_اشاریہ};
            funcp->class_طریقہ(سچ);
            funcp->dtypep(funcp->ڈھونڈو_Void_DType());
            nodep->جوڑو_Membersp(funcp);
            AstVar* ثابت varp = نیا AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->ڈھونڈو_String_DType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(سچ);
            varp->direction(VDirection::داخلہ);
            funcp->جوڑو_Stmtsp(varp);
        }
    }

    منطقی dropDeprecatedCoverageOption(Ast_Cg_Option_تفویض* ثابت nodep) {
        اگر (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) واپس جھوٹ;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        واپس سچ;
    }

    خالی دورہ(Ast_Cg_Option_تفویض* nodep) override {
        اگر (dropDeprecatedCoverageOption(nodep)) واپس;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    خالی دورہ(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        اگر (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) واپس;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        اگر (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ثابت string libname = m_modp->libname();
        AstClass* ثابت cgClassp = نیا AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
        cgClassp->fileline()->warnOff(V3_Error_رمز::UNDRIVEN, سچ);
        cgClassp->isCovergroup(سچ);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(سچ);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        اگر (Ast_Sen_درخت* ثابت eventp = nodep->eventp()) {
            eventp->unlink_Fr_عقب();
            AstCovergroup* ثابت cgNodep = نیا AstCovergroup{
                nodep->fileline(), nodep->name(), خالی_اشاریہ, خالی_اشاریہ, خالی_اشاریہ, eventp};
            cgClassp->جوڑو_Membersp(cgNodep);
        }

        // Convert constructor args to member variables
        برائے (Ast_گرہ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            اگر (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::داخلہ);
            }
            اگر ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ثابت memberp = origVarp->clone_درخت(جھوٹ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(جھوٹ);
            memberp->direction(VDirection::NONE);
            اگر (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(سچ);
            cgClassp->جوڑو_Membersp(memberp);
        }

        // Convert sample args to member variables
        برائے (Ast_گرہ* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            اگر (!origVarp->is_داخلہ()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::داخلہ);
            }
            AstVar* ثابت memberp = origVarp->clone_درخت(جھوٹ);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(جھوٹ);
            memberp->direction(VDirection::NONE);
            cgClassp->جوڑو_Membersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ثابت newFuncp = نیا AstFunc{nodep->fileline(), "new", خالی_اشاریہ, خالی_اشاریہ};
        newFuncp->fileline()->warnOff(V3_Error_رمز::NORETURN, سچ);
        newFuncp->class_طریقہ(سچ);
        newFuncp->isConstructor(سچ);
        newFuncp->dtypep(cgClassp->dtypep());
        اگر (Ast_گرہ* ثابت bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_عقب_With_اگلا();
            newFuncp->جوڑو_Stmtsp(bodyp);
        }
        cgClassp->جوڑو_Membersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_عدد);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_عدد = 0;
        برائے (ثابت Ast_گرہ* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstCoverpoint* ثابت pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ساکن string coverpointName(ثابت AstCoverpoint* nodep) {
        اگر (!nodep->name().خالی_خانے()) واپس nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ثابت AstParseRef* ثابت refp = VN_CAST(nodep->exprp(), ParseRef);
        واپس refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ثابت AstCoverpoint* ڈھونڈو_Coverpoint(ثابت AstCoverpointRef* refp) ثابت {
        اگر (refp->exprp()) واپس خالی_اشاریہ;  // A hierarchical reference, unsupported
        ثابت خودکار it = m_coverpoints.ڈھونڈو(refp->name());
        واپس it == m_coverpoints.انتہا() ? خالی_اشاریہ : it->دوسرا;
    }

    خالی دورہ(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        اگر (nodep->name().خالی_خانے()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ثابت string name = coverpointName(nodep);
            nodep->name(name.خالی_خانے() ? "__Vcoverpoint" + cvt_مقصد_Str(m_coverpoint_عدد++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        برائے (Ast_گرہ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (Ast_Cg_Option_تفویض* ثابت optp = VN_CAST(itemp, Cg_Option_تفویض)) {
                اگر (dropDeprecatedCoverageOption(optp)) جاری;
                optp->unlink_Fr_عقب();
                اگر (optp->optType() == VCoverOptionType::AT_کم_سے_کم
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::وزن) {
                    nodep->جوڑو_Optionsp(نیا AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_درخت(جھوٹ)});
                } ورنہ {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_درخت(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        اگر (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        اگر (ثابت AstCoverpointRef* ثابت refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            اگر (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_سطر* ثابت fl = nodep->fileline();
        File_سطر* ثابت flNoWarn = نیا File_سطر{fl};
        flNoWarn->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
        flNoWarn->modifyWarnOff(V3_Error_رمز::VARHIDDEN, سچ);
        AstVar* ثابت varp = نیا AstVar{flNoWarn, VVarType::VAR, "item", VFlag_بچہ_DType{},
                                        نیا AstRefDType{fl, AstRefDType::Flag_Type_کا_اظہاریہ{},
                                                        m_coverpointp->exprp()->clone_درخت(جھوٹ)}};
        varp->funcLocal(سچ);
        varp->noReset(سچ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ثابت refp = nodep->pointp();
        ثابت AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ثابت AstParseRef* binp = خالی_اشاریہ;
        ثابت AstDot* ثابت dotp = VN_CAST(refp->exprp(), Dot);
        اگر (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        اگر (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        اگر (binp) nodep->name(binp->name());
        refp->replaceWith(نیا AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        }
    }

    خالی دورہ(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        }
    }

    خالی دورہ(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        برائے (Ast_گرہ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (VN_IS(itemp, CoverCrossBin)) جاری;
            Ast_Cg_Option_تفویض* ثابت optp = VN_AS(itemp, Cg_Option_تفویض);
            اگر (dropDeprecatedCoverageOption(optp)) جاری;
            itemp->unlink_Fr_عقب();
            ثابت VCoverOptionType optType = optp->optType();
            اگر (!(optType == VCoverOptionType::AT_کم_سے_کم || optType == VCoverOptionType::وزن)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->جوڑو_Optionsp(نیا AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_درخت(جھوٹ)});
            VL_DO_DANGLING(optp->delete_درخت(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        اگر (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_سطر* ثابت fl = nodep->fileline();
            File_سطر* ثابت flNoWarn = نیا File_سطر{fl};
            flNoWarn->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
            flNoWarn->modifyWarnOff(V3_Error_رمز::VARHIDDEN, سچ);
            std::سیٹ<string> نام;
            برائے (ثابت Ast_گرہ* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                ثابت AstCoverpoint* ثابت pointp = ڈھونڈو_Coverpoint(refp);
                اگر (!pointp || !نام.داخل_کرو(refp->name()).دوسرا) جاری;
                AstVar* ثابت varp
                    = نیا AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlag_بچہ_DType{},
                                 نیا AstRefDType{fl, AstRefDType::Flag_Type_کا_اظہاریہ{},
                                                 pointp->exprp()->clone_درخت(جھوٹ)}};
                varp->funcLocal(سچ);
                varp->noReset(سچ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->جوڑو_Varsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        اگر (!nodep->selectp()) VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    خالی دورہ(AstSystemCSection* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process only once.
        cleanFileline(nodep);
        m_modp->جوڑو_Sc_Sectionsp(nodep->unlink_Fr_عقب());
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::جوڑو_Stat_مجموع(V3Stats::STAT_ماخذ_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

خالی V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkparse", 0, dump_درخت_Either_Level() >= 6);
}
