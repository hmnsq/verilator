// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break case statements up and add Unknown assigns
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
// V3Case's Transformations:
//
// Each module:
//      TBD: Eliminate tristates by adding __in, __inen, __en wires in parallel
//          Need __en in changed list if a signal is on the LHS of a assign
//      Cases:
//          CASE(v) CASEITEM(items,body) ->  IF (OR (EQ (AND v mask)
//                                                      (AND item1 mask))
//                                                  (other items))
//                                              body
//              Or, converts to a if/else tree.
//      FUTURES:
//          Large 16+ bit tables with constants and no masking (address muxes)
//              Enter all into std::multimap, sort by value and use a tree of < and == compares.
//          "Diagonal" find of {rightmost,leftmost} bit {set,clear}
//              Ignoring mask, check each value is unique (using std::multimap as above?)
//              Each branch is then mask-and-compare operation (IE
//              <000000001_000000000 at midpoint.)
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Case.h"

#ضمن "V3ConstPool.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف CaseLintVisitor final : عام VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ثابت Ast_غوټه* m_casep = تش_اشاره;

    // METHODS
    کينډۍ <ډول_نوم Case_توکی>
    ساكن فراغ detect_مضرب_Defaults(Case_توکی* itemsp) {
        منطقي موندنه_Default = خطأ;
        لكل (Case_توکی* itemp = itemsp; itemp; itemp = Ast_غوټه::as<Case_توکی>(itemp->nextp())) {
            إذا (!itemp->isDefault()) استمر;
            إذا (موندنه_Default) itemp->v3error("Multiple default statements in case statement.");
            موندنه_Default = ریښتیا;
        }
    }

    کينډۍ <ډول_نوم Case_توکی>
    فراغ checkXZinNonCaseX(Ast_غوټه* casep, Ast_غوټه_بیان* exprp, Case_توکی* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        لكل (Case_توکی* itemp = itemsp; itemp; itemp = Ast_غوټه::as<Case_توکی>(itemp->nextp())) {
            iterate_And_بل_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    فراغ لیدنه(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    فراغ لیدنه(AstCase* nodep) override {
        إذا (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ لیدنه(AstConst* nodep) override {
        إذا (!nodep->عدد().isFourState()) أرجع;

        // Error if generate case
        إذا (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            أرجع;
        }

        // Otherwise must be a case statement
        ثابت AstCase* ثابت casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        إذا (casep->casex()) أرجع;

        إذا (casep->casez() || casep->caseInside()) {
            إذا (nodep->عدد().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            أرجع;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    څرګند CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    څرګند CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = افتراضي;

عام:
    ساكن فراغ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ساكن فراغ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

صنف CaseVisitor final : عام VNVisitor {
    // Maximum width for detailed analysis
    constexpr ساكن صحيح CASE_DETAILS_MAX_پلنوالی = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساكن صحيح CASE_ENCODER_ډله_ژوروالی = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساكن اندازه_t CASE_جدول_TINY_بټونه = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساكن اندازه_t CASE_جدول_MAX_بټونه = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساكن اندازه_t CASE_جدول_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    بنية Case_ریکارډ final {
        Ast_Case_توکی* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_غوټه* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    بنية Lhs_ریکارډ final {
        Ast_غوټه_بیان* lhsp = تش_اشاره;  // LHS of the assignment
        Ast_غوټه_ټاکل* preDefaultp = تش_اشاره;  // Default assignment *before the case statement*
        اندازه_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        اندازه_t n_Case_ټاکل_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        اندازه_t دننه_واټن = 0;  // Offset in the table for this LHS

        ساكن اندازه_t s_بل_Id;  // Static unique Id counter
        اندازه_t id = ++s_بل_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    بنية Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_جدول_نورمال;  // Cases using table method with normal table
        VDouble0 case_جدول_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت Ast_غوټه* m_alwaysp = تش_اشاره;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_جدول_بهر_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_بهر_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = تش_اشاره;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقي m_caseOpaque = خطأ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقي m_caseHasDefault = خطأ;  // Indicates the case statement has a default case
    اندازه_t m_case_NCase_توکي = 0;  // Number of AstCaseItems in the case statement
    اندازه_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_غوټه_بیان>, Lhs_ریکارډ> m_case_Lhs_ریکارډونه;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::ویکټور<Lhs_ریکارډ> m_case_Decoder_ریکارډونه;
    اندازه_t m_case_Decoder_ثبت_پلنوالی = 0;  // Width of each entry in the decoder table
    اندازه_t m_case_جدول_پلنوالی = 0;  // Total width of the case table - 0 means can't optimize
    منطقي m_caseDetailsValid = خطأ;  // Indicates m_caseDetails is valid
    بنية final {
        منطقي exhaustive = خطأ;  // Proven exhaustive
        منطقي exhaustiveOverEnumOnly = خطأ;  // Exhaustive over enum values only
        منطقي noOverlaps = خطأ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_ریکارډ, 1U << CASE_DETAILS_MAX_پلنوالی> ریکارډونه;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساكن منطقي never_توکی(ثابت AstCase* casep, ثابت Ast_غوټه_بیان* توکی_Exprp) {
        ثابت AstConst* ثابت constp = VN_CAST(توکی_Exprp, Const);
        إذا (!constp) أرجع خطأ;
        إذا (casep->casex() || casep->caseInside()) أرجع خطأ;
        إذا (casep->casez()) أرجع constp->عدد().isAnyX();
        أرجع constp->عدد().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساكن std::pair<V3_شمېره, V3_شمېره> سمون_نمونه(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_شمېره, V3_شمېره> pair_ماسک_بټونه{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->پلنوالی(), 0),
            std::forward_as_tuple(condp->fileline(), condp->پلنوالی(), 0)};
        pair_ماسک_بټونه.لومړی.op_بټونه_Non_XZ(condp->عدد());
        pair_ماسک_بټونه.دوهم.op_بټونه_One(condp->عدد());
        أرجع pair_ماسک_بټونه;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساكن Ast_غوټه_ټاکل* check_Decoder_ټاکل(Ast_غوټه* stmtp) {
        // Only Assign and AssignDly are supported
        إذا (!VN_IS(stmtp, ټاکل) && !VN_IS(stmtp, ټاکل_Dly)) أرجع تش_اشاره;
        Ast_غوټه_ټاکل* ثابت assp = VN_AS(stmtp, غوټه_ټاکل);
        // Only if no timing control
        إذا (assp->timingControlp()) أرجع تش_اشاره;
        // Only if assigning a constant
        إذا (!VN_IS(assp->rhsp(), Const)) أرجع تش_اشاره;
        // Only if it's a packed value
        Ast_غوټه_DType* ثابت dtypep = assp->rhsp()->dtypep();
        إذا (dtypep->isString() || dtypep->isDouble()) أرجع تش_اشاره;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        إذا (assp->lhsp()->exists([](AstVarRef* refp) { أرجع refp->access().isReadOrRW(); })) {
            أرجع تش_اشاره;
        }
        // This is an assignment that fits the decoder pattern
        أرجع assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقي analyze_Decoder_Case_توکی(Ast_Case_توکی* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        لكل (Ast_غوټه* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_غوټه_ټاکل* ثابت assp = check_Decoder_ټاکل(stmtp);
            إذا (!assp) أرجع خطأ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                أرجع refp->varScopep()->user1_سېټ_Once();
            });
            إذا (مضرب_Assignments) أرجع خطأ;
            // Update LhsRecord
            Lhs_ریکارډ& lhs_ریکارډ = m_case_Lhs_ریکارډونه[*assp->lhsp()];
            إذا (!lhs_ریکارډ.lhsp) lhs_ریکارډ.lhsp = assp->lhsp();
            lhs_ریکارډ.nCaseAssigns += VN_IS(assp, ټاکل);
            lhs_ریکارډ.n_Case_ټاکل_Dlys += VN_IS(assp, ټاکل_Dly);
        }
        أرجع ریښتیا;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساكن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        إذا (!nodep->uniquePragma() && !nodep->unique0Pragma()) أرجع تش_اشاره;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enumDtp) أرجع تش_اشاره;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        إذا (!basicp) أرجع تش_اشاره;  // Not simple type (perhaps IEEE illegal)
        أرجع enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقي checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->پلنوالی();
        منطقي fullyCovered = ریښتیا;
        string missing_توکي;
        لكل (Ast_Enum_توکی* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_توکی)) {
            ثابت خپلکاری& سمون = سمون_نمونه(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t ماسک = سمون.لومړی.منزل_UInt();
            ثابت uint32_t بټونه = سمون.دوهم.منزل_UInt();

            // Check all cases to see if they cover this enum value/pattern
            لكل (uint32_t i = 0; i < عدد_Cases; ++i) {
                إذا ((i & ماسک) != بټونه) استمر;  // This case is not for this enum value
                إذا (m_caseDetails.ریکارډونه[i].itemp) استمر;  // Covered case
                إذا (!missing_توکي.تشه()) missing_توکي += ", ";
                missing_توکي += eip->prettyNameQ();
                fullyCovered = خطأ;
                اكسر;
            }
        }
        إذا (!missing_توکي.تشه() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_توکي);
        }
        أرجع fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقي checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->پلنوالی();
        لكل (uint32_t i = 0; i < عدد_Cases; ++i) {
            إذا (m_caseDetails.ریکارډونه[i].itemp) استمر;  // Covered case
            إذا (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            أرجع خطأ;
        }

        // It's an exhaustive case statement
        أرجع ریښتیا;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    فراغ analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t عدد_ارزښتونه = 1UL << nodep->exprp()->پلنوالی();
        // Clear case records
        لكل (uint32_t i = 0; i < عدد_ارزښتونه; ++i) {
            m_caseDetails.ریکارډونه[i].itemp = تش_اشاره;
            m_caseDetails.ریکارډونه[i].constp = تش_اشاره;
            m_caseDetails.ریکارډونه[i].stmtsp = تش_اشاره;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقي reportedOverlap = خطأ;
        منطقي hasDefault = خطأ;
        m_caseDetails.noOverlaps = ریښتیا;
        لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {

            // Default case
            إذا (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                لكل (uint32_t i = 0; i < عدد_ارزښتونه; ++i) {
                    Case_ریکارډ& case_ریکارډ = m_caseDetails.ریکارډونه[i];
                    إذا (!case_ریکارډ.itemp) {
                        case_ریکارډ.itemp = itemp;
                        case_ریکارډ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = ریښتیا;
                استمر;
            }

            لكل (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                إذا (never_توکی(nodep, iconstp)) استمر;

                ثابت خپلکاری& سمون = سمون_نمونه(nodep, iconstp);
                ثابت uint32_t ماسک = سمون.لومړی.منزل_UInt();
                ثابت uint32_t بټونه = سمون.دوهم.منزل_UInt();

                منطقي وموندل_New_Case = خطأ;
                ثابت AstConst* لومړی_Overlap_Constp = تش_اشاره;
                uint32_t لومړی_Overlap_ارزښت = 0;
                لكل (uint32_t i = 0; i < عدد_ارزښتونه; ++i) {
                    إذا ((i & ماسک) != بټونه) استمر;

                    Case_ریکارډ& case_ریکارډ = m_caseDetails.ریکارډونه[i];

                    // If this is the first case that covers this value, record it
                    إذا (!case_ریکارډ.itemp) {
                        case_ریکارډ.itemp = itemp;
                        case_ریکارډ.constp = iconstp;
                        case_ریکارډ.stmtsp = itemp->stmtsp();
                        وموندل_New_Case = ریښتیا;
                        استمر;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    إذا (case_ریکارډ.itemp == itemp) استمر;

                    // Otherwise record the first overlapping case
                    إذا (!لومړی_Overlap_Constp) {
                        لومړی_Overlap_Constp = case_ریکارډ.constp;
                        لومړی_Overlap_ارزښت = i;
                        m_caseDetails.noOverlaps = خطأ;
                    }
                }

                // Only report first overlap
                إذا (reportedOverlap || !لومړی_Overlap_Constp) استمر;

                // Report first overlap
                إذا (nodep->لومړیتوب_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    إذا (!وموندل_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << لومړی_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << لومړی_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = ریښتیا;
                    }
                } وإلا {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_نمونه;
                    إذا (iconstp->عدد().isAnyXZ()) {
                        example_نمونه << " (example pattern 0x" << std::hex << لومړی_Overlap_ارزښت
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_نمونه.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << لومړی_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << لومړی_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = ریښتیا;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = خطأ;
        m_caseDetails.exhaustive = hasDefault;
        إذا (!hasDefault) {
            إذا (ثابت AstEnumDType* ثابت enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ثابت منطقي exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } وإلا {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = ریښتیا;
    }

    فراغ analyze_Decoder_نمونه(AstCase* nodep) {
        // Check each LHS record
        لكل (خپلکاری it = m_case_Lhs_ریکارډونه.cbegin(); it != m_case_Lhs_ریکارډونه.cend();) {
            ثابت Lhs_ریکارډ& lhs_ریکارډ = it->دوهم;

            // Delete records that have no assignments in any case item (only pre-defaults)
            إذا (!lhs_ریکارډ.nCaseAssigns && !lhs_ریکارډ.n_Case_ټاکل_Dlys) {
                it = m_case_Lhs_ریکارډونه.erase(it);
                استمر;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            إذا (lhs_ریکارډ.nCaseAssigns && lhs_ریکارډ.n_Case_ټاکل_Dlys) أرجع;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            إذا (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                إذا (lhs_ریکارډ.nCaseAssigns == m_case_NCase_توکي) استمر;
                إذا (lhs_ریکارډ.n_Case_ټاکل_Dlys == m_case_NCase_توکي) استمر;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_غوټه* ثابت preDefaultp = lhs_ریکارډ.preDefaultp;
            إذا (!preDefaultp) أرجع;
            // And the pre-default needs to be the same type
            إذا (lhs_ریکارډ.nCaseAssigns && !VN_IS(preDefaultp, ټاکل)) أرجع;
            إذا (lhs_ریکارډ.n_Case_ټاکل_Dlys && !VN_IS(preDefaultp, ټاکل_Dly)) أرجع;
        }
        // All cases check out, can optimize if there are some entries left
        إذا (m_case_Lhs_ریکارډونه.تشه()) أرجع;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_ریکارډونه.reserve(m_case_Lhs_ریکارډونه.اندازه());
        لكل (ثابت خپلکاری& توکی_2 : m_case_Lhs_ریکارډونه) m_case_Decoder_ریکارډونه.emplace_شا(توکی_2.دوهم);
        std::ترتیبول(m_case_Decoder_ریکارډونه.begin(), m_case_Decoder_ریکارډونه.پای_ټکی(),
                  [](ثابت Lhs_ریکارډ& a, ثابت Lhs_ریکارډ& b) {
                      // Sort by width, then id
                      ثابت صحيح a_پلنوالی = a.lhsp->پلنوالی();
                      ثابت صحيح b_پلنوالی = b.lhsp->پلنوالی();
                      إذا (a_پلنوالی != b_پلنوالی) أرجع a_پلنوالی < b_پلنوالی;
                      أرجع a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحيح case_پلنوالی = nodep->exprp()->پلنوالی();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت خپلکاری ځاییږي_حد = [&](اندازه_t ثبت_پلنوالی, اندازه_t max_پلنوالی) -> منطقي {
            اندازه_t ټول_پلنوالی = ثبت_پلنوالی;
            // Multiply cases - iterative to avoid overflow
            لكل (صحيح i = 0; i < case_پلنوالی; ++i) {
                ټول_پلنوالی <<= 1;
                إذا (ټول_پلنوالی > max_پلنوالی) أرجع خطأ;
            }
            أرجع ریښتیا;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_ثبت_پلنوالی = 0;
        لكل (Lhs_ریکارډ& lhs_ریکارډ : m_case_Decoder_ریکارډونه) {
            lhs_ریکارډ.دننه_واټن = m_case_Decoder_ثبت_پلنوالی;
            m_case_Decoder_ثبت_پلنوالی += lhs_ریکارډ.lhsp->پلنوالی();
        }
        // If it fits, we will pack it tightly
        إذا (ځاییږي_حد(m_case_Decoder_ثبت_پلنوالی, CASE_جدول_TINY_بټونه)) {
            m_case_جدول_پلنوالی = m_case_Decoder_ثبت_پلنوالی << case_پلنوالی;  // Can optimize
            أرجع;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_ثبت_پلنوالی = 0;
        لكل (Lhs_ریکارډ& lhs_ریکارډ : m_case_Decoder_ریکارډونه) {
            ثابت اندازه_t پلنوالی = lhs_ریکارډ.lhsp->پلنوالی();
            ثابت اندازه_t lsb_کلمه = VL_BITWORD_E(m_case_Decoder_ثبت_پلنوالی);
            ثابت اندازه_t msb_کلمه = VL_BITWORD_E(m_case_Decoder_ثبت_پلنوالی + پلنوالی - 1);
            إذا (lsb_کلمه != msb_کلمه) {
                m_case_Decoder_ثبت_پلنوالی = VL_ټکي_I(m_case_Decoder_ثبت_پلنوالی) * VL_EDATASIZE;
            }
            lhs_ریکارډ.دننه_واټن = m_case_Decoder_ثبت_پلنوالی;
            m_case_Decoder_ثبت_پلنوالی += پلنوالی;
        }
        // Check the table fits max size
        ثابت اندازه_t aligned_ثبت_پلنوالی = VL_ټکي_I(m_case_Decoder_ثبت_پلنوالی) * VL_EDATASIZE;
        إذا (ځاییږي_حد(aligned_ثبت_پلنوالی, CASE_جدول_MAX_بټونه)) {
            m_case_جدول_پلنوالی = aligned_ثبت_پلنوالی << case_پلنوالی;  // Can optimize
            أرجع;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    فراغ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = خطأ;
        m_caseHasDefault = خطأ;
        m_case_NCase_توکي = 0;
        m_caseNConditions = 0;
        m_case_Decoder_ریکارډونه.clear();
        m_case_Decoder_ثبت_پلنوالی = 0;
        m_case_جدول_پلنوالی = 0;
        m_case_Lhs_ریکارډونه.clear();
        m_caseDetailsValid = خطأ;

        Ast_غوټه* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        إذا (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = ریښتیا;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1_دننه_Use user1_دننه_Use;
            لكل (Ast_غوټه* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_غوټه_ټاکل* ثابت assp = check_Decoder_ټاکل(prevp);
                إذا (!assp) اكسر;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    أرجع refp->varScopep()->user1_سېټ_Once();
                });
                إذا (مضرب_Assignments) اكسر;
                // Store pre-default assignment
                Lhs_ریکارډ& lhs_ریکارډ = m_case_Lhs_ریکارډونه[*assp->lhsp()];
                lhs_ریکارډ.lhsp = assp->lhsp();
                lhs_ریکارډ.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقي canBeDecoder = ریښتیا;
        لكل (Ast_Case_توکی* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_توکی)) {
            // Check conditions
            لكل (Ast_غوټه* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                إذا (!never_توکی(nodep, VN_AS(condp, غوټه_بیان))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                إذا (!VN_IS(condp, Const)) {
                    m_caseOpaque = ریښتیا;
                    canBeDecoder = خطأ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            إذا (cip->isDefault()) m_caseHasDefault = ریښتیا;
            // Count case items
            ++m_case_NCase_توکي;
            // Check if it fits the decoder pattern, if still possible
            إذا (canBeDecoder) canBeDecoder = analyze_Decoder_Case_توکی(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        إذا (m_caseOpaque) أرجع;

        // If small enough, analyse details
        إذا (caseExprp->پلنوالی() <= CASE_DETAILS_MAX_پلنوالی) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        إذا (canBeDecoder) analyze_Decoder_نمونه(nodep);
    }

    Ast_غوټه_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_غوټه_بیان* exprp, V3SharedTmps& tmps) {
        File_کرښه* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        إذا (m_case_Decoder_ریکارډونه.اندازه() == 1) {
            ثابت Lhs_ریکارډ& lhs_ریکارډ = m_case_Decoder_ریکارډونه[0];
            ثابت صحيح پلنوالی = lhs_ریکارډ.lhsp->پلنوالی();
            Ast_غوټه_بیان* ثابت rhsp
                = exprp->پلنوالی() == پلنوالی ? exprp : جديد AstSel{flp, exprp, 0, پلنوالی};
            Ast_غوټه_بیان* ثابت lhsp = lhs_ریکارډ.lhsp->clone_ونه_خالص(خطأ);
            إذا (lhs_ریکارډ.nCaseAssigns) {
                أرجع جديد Ast_ټاکل{flp, lhsp, rhsp};
            } وإلا إذا (lhs_ریکارډ.n_Case_ټاکل_Dlys) {
                أرجع جديد Ast_ټاکل_Dly{flp, lhsp, rhsp};
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت لنډمهاله_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_ثبت_پلنوالی);
        Ast_غوټه_بیان* ثابت لنډمهاله_Writep = جديد AstVarRef{flp, لنډمهاله_Vscp, VAccess::WRITE};
        Ast_غوټه_Stmt* ثابت resultp = جديد Ast_ټاکل{flp, لنډمهاله_Writep, exprp};

        // For each LHS, select out the result
        لكل (ثابت Lhs_ریکارډ& lhs_ریکارډ : m_case_Decoder_ریکارډونه) {
            ثابت صحيح پلنوالی = lhs_ریکارډ.lhsp->پلنوالی();
            ثابت صحيح lsb = lhs_ریکارډ.دننه_واټن;
            Ast_غوټه_بیان* ثابت لنډمهاله_Readp = جديد AstVarRef{flp, لنډمهاله_Vscp, VAccess::READ};
            Ast_غوټه_بیان* ثابت rhsp = جديد AstSel{flp, لنډمهاله_Readp, lsb, پلنوالی};
            Ast_غوټه_بیان* ثابت lhsp = lhs_ریکارډ.lhsp->clone_ونه_خالص(خطأ);
            إذا (lhs_ریکارډ.nCaseAssigns) {
                resultp->جمع_کړه_بل(جديد Ast_ټاکل{flp, lhsp, rhsp});
            } وإلا إذا (lhs_ریکارډ.n_Case_ټاکل_Dlys) {
                resultp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{flp, lhsp, rhsp});
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        أرجع resultp;
    }

    Ast_غوټه_Stmt* convert_Case_جدول(AstCase* nodep) {
        // Create the table constant
        File_کرښه* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, static_cast<صحيح>(m_case_جدول_پلنوالی), 0};
        ثابت uint32_t جدول_Entries = 1U << nodep->exprp()->پلنوالی();

        ثابت منطقي is_Tiny_جدول = m_case_جدول_پلنوالی <= CASE_جدول_TINY_بټونه;
        إذا (is_Tiny_جدول) {
            ++m_stats.case_جدول_Tiny;
        } وإلا {
            ++m_stats.case_جدول_نورمال;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t ثبت_پلنوالی = is_Tiny_جدول
                                        ? m_case_Decoder_ثبت_پلنوالی
                                        : VL_ټکي_I(m_case_Decoder_ثبت_پلنوالی) * VL_EDATASIZE;
        لكل (ثابت Lhs_ریکارډ& lhs_ریکارډ : m_case_Decoder_ریکارډونه) {
            ثابت صحيح lhs_پلنوالی = lhs_ریکارډ.lhsp->پلنوالی();
            ثابت صحيح lhs_دننه_واټن = lhs_ریکارډ.دننه_واټن;

            // Broadcast the pre-default assignment
            إذا (lhs_ریکارډ.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhs_ریکارډ.preDefaultp->rhsp(), Const);
                لكل (uint32_t شاخص = 0; شاخص < جدول_Entries; ++شاخص) {
                    ثابت uint32_t جدول_دننه_واټن = شاخص * ثبت_پلنوالی + lhs_دننه_واټن;
                    tablep->عدد().opSelInto(rhsp->عدد(), جدول_دننه_واټن, lhs_پلنوالی);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            لكل (Ast_Case_توکی* cip = VN_AS(nodep->itemsp()->lastp(), Case_توکی); cip;
                 cip = VN_AS(cip->prevp(), Case_توکی)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    لكل (Ast_غوټه* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_غوټه_ټاکل* ثابت ap = VN_AS(stmtp, غوټه_ټاکل);
                        إذا (lhs_ریکارډ.lhsp->same_ونه(ap->lhsp())) أرجع VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    أرجع VN_AS(lhs_ریکارډ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                إذا (cip->isDefault()) {
                    لكل (uint32_t شاخص = 0; شاخص < جدول_Entries; ++شاخص) {
                        ثابت uint32_t جدول_دننه_واټن = شاخص * ثبت_پلنوالی + lhs_دننه_واټن;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_دننه_واټن, lhs_پلنوالی);
                    }
                    استمر;
                }

                // Iterate case conditions in reverse order
                لكل (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    إذا (never_توکی(nodep, condp)) استمر;  // If item never matches, ignore it
                    ثابت خپلکاری& سمون = سمون_نمونه(nodep, condp);
                    ثابت uint32_t سمون_ماسک = سمون.لومړی.منزل_UInt();
                    ثابت uint32_t سمون_بټونه = سمون.دوهم.منزل_UInt();
                    ثابت uint32_t inverse_ماسک = ~سمون_ماسک & ((1U << condp->پلنوالی()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    لكل (uint32_t i = inverse_ماسک; ریښتیا; i = (i - 1) & inverse_ماسک) {
                        ثابت uint32_t شاخص = i | سمون_بټونه;
                        ثابت uint32_t جدول_دننه_واټن = شاخص * ثبت_پلنوالی + lhs_دننه_واټن;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_دننه_واټن, lhs_پلنوالی);
                        إذا (!i) اكسر;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_غوټه_بیان* ثابت جدول_Refp = [&]() -> Ast_غوټه_بیان* {
            إذا (is_Tiny_جدول) أرجع tablep;
            AstVarRef* ثابت refp = V3ConstPool::پیدا_کړه(tablep);
            VL_DO_DANGLING(tablep->delete_ونه(), tablep);  // V3ConstPool::find clones
            أرجع refp;
        }();

        // Create the lookup index
        Ast_غوټه_بیان* ثابت caseExprp
            = جديد AstExtend{flp, nodep->exprp()->clone_ونه_خالص(خطأ), 32};
        Ast_غوټه_بیان* ثابت scalep = جديد AstConst{flp, ثبت_پلنوالی};
        Ast_غوټه_بیان* ثابت جدول_Lsbp = جديد AstMul{flp, scalep, caseExprp};
        Ast_غوټه_بیان* ثابت جدول_Selp
            = جديد AstSel{flp, جدول_Refp, جدول_Lsbp, static_cast<صحيح>(m_case_Decoder_ثبت_پلنوالی)};

        // Connect outputs
        أرجع connectDecoderOutputs(nodep, جدول_Selp, m_جدول_بهر_Tmps);
    }

    Ast_غوټه_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_کرښه* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::ویکټور<std::pair<AstConst*, Ast_غوټه*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        لكل (Ast_Case_توکی* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_توکی)) {
            إذا (cip->isDefault()) {
                clauses.emplace_شا(تش_اشاره, cip->stmtsp());
                استمر;
            }
            لكل (Ast_غوټه* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                إذا (never_توکی(nodep, iconstp)) استمر;
                clauses.emplace_شا(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقي provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        إذا (clauses.شا().لومړی && !provenExhaustive) clauses.emplace_شا(تش_اشاره, تش_اشاره);

        // Number of entries in decoder table
        ثابت صحيح decoderEnries = clauses.اندازه();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحيح cond_پلنوالی = nodep->exprp()->پلنوالی();
        ثابت صحيح سمون_پلنوالی = 2 * VL_ټکي_I(cond_پلنوالی) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جديد AstConst{flp, AstConst::Widthed_ارزښت{}, decoderEnries * سمون_پلنوالی, 0};
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            ثابت صحيح ثبت_Lsb = i * سمون_پلنوالی;

            // If the entry has a condition, use it's match bits and mask
            إذا (AstConst* ثابت condp = clauses[i].لومړی) {
                ثابت خپلکاری& سمون = سمون_نمونه(nodep, condp);
                matchp->عدد().opSelInto(سمون.لومړی, ثبت_Lsb, cond_پلنوالی);
                matchp->عدد().opSelInto(سمون.دوهم, ثبت_Lsb + سمون_پلنوالی / 2, cond_پلنوالی);
                استمر;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_شمېره عدد_Zero{flp, cond_پلنوالی, 0};
            matchp->عدد().opSelInto(عدد_Zero, ثبت_Lsb, cond_پلنوالی);
            matchp->عدد().opSelInto(عدد_Zero, ثبت_Lsb + سمون_پلنوالی / 2, cond_پلنوالی);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جديد AstRange{flp, decoderEnries - 1, 0};
        Ast_غوټه_DType* ثابت ثبت_Dtypep = nodep->پیدا_کړه_بټ_DType(
            m_case_Decoder_ثبت_پلنوالی, m_case_Decoder_ثبت_پلنوالی, VSigning::UNSIGNED);
        Ast_غوټه_DType* ثابت جدول_Dtypep = جديد AstUnpackArrayDType{flp, ثبت_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(جدول_Dtypep);
        AstConsArray* ثابت tablep = جديد AstConsArray{flp, جدول_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            Ast_غوټه* ثابت stmtsp = clauses[i].دوهم;
            AstConst* ثابت entryp = جديد AstConst{flp, AstConst::Widthed_ارزښت{},
                                                  static_cast<صحيح>(m_case_Decoder_ثبت_پلنوالی), 0};
            لكل (ثابت Lhs_ریکارډ& lhs_ریکارډ : m_case_Decoder_ریکارډونه) {
                Ast_غوټه_بیان* ثابت lhsp = lhs_ریکارډ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = تش_اشاره;
                لكل (Ast_غوټه* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_غوټه_ټاکل* ثابت assignp = VN_AS(stmtp, غوټه_ټاکل);
                    إذا (!lhsp->same_ونه(assignp->lhsp())) استمر;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    اكسر;
                }
                // Not assigned in this clause, so use the pre-assigned default
                إذا (!valConstp) {
                    UASSERT_OBJ(lhs_ریکارډ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_ریکارډ.preDefaultp->rhsp(), Const);
                }
                entryp->عدد().opSelInto(valConstp->عدد(), lhs_ریکارډ.دننه_واټن, lhsp->پلنوالی());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت سمون_Refp = V3ConstPool::پیدا_کړه(matchp);
        AstVarRef* ثابت جدول_Refp = V3ConstPool::پیدا_کړه(tablep);
        VL_DO_DANGLING(matchp->delete_ونه(), matchp);
        VL_DO_DANGLING(tablep->delete_ونه(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_غوټه_بیان* ثابت caseExprp = nodep->exprp()->clone_ونه_خالص(خطأ);
        Ast_سمون_Masked* ثابت indexp = جديد Ast_سمون_Masked{flp, caseExprp, سمون_Refp};
        Ast_غوټه_بیان* ثابت entryp = جديد AstArraySel{flp, جدول_Refp, indexp};

        أرجع connectDecoderOutputs(nodep, entryp, m_decoder_بهر_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_غوټه* convertCaseFastRecurse(Ast_غوټه_بیان* cexprp, صحيح msb, uint32_t پاسنی_ارزښت) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        إذا (msb < 0) أرجع m_caseDetails.ریکارډونه[پاسنی_ارزښت].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t پاسنی_Value0 = پاسنی_ارزښت;
        ثابت uint32_t پاسنی_Value1 = پاسنی_ارزښت | (1UL << msb);
        Ast_غوټه* tree0p = convertCaseFastRecurse(cexprp, msb - 1, پاسنی_Value0);
        Ast_غوټه* tree1p = convertCaseFastRecurse(cexprp, msb - 1, پاسنی_Value1);

        // If same logic on both sides, we can just return one of them
        إذا (tree0p == tree1p) أرجع tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقي same = ریښتیا;
            لكل (uint32_t a = پاسنی_Value0, b = پاسنی_Value1; a < پاسنی_Value1; ++a, ++b) {
                إذا (m_caseDetails.ریکارډونه[a].stmtsp != m_caseDetails.ریکارډونه[b].stmtsp) {
                    same = خطأ;
                    اكسر;
                }
            }
            إذا (same) {
                VL_DO_DANGLING(tree1p->delete_ونه(), tree1p);
                أرجع tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        إذا (tree0p && tree0p->backp()) tree0p = tree0p->clone_ونه(ریښتیا);
        إذا (tree1p && tree1p->backp()) tree1p = tree1p->clone_ونه(ریښتیا);
        // Create the If statement
        File_کرښه* ثابت flp = cexprp->fileline();
        Ast_غوټه_بیان* ثابت condp = جديد AstSel{flp, cexprp->clone_ونه_خالص(خطأ), msb, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp, tree1p, tree0p};
        أرجع ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_غوټه* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحيح case_پلنوالی = nodep->exprp()->پلنوالی();
        Ast_غوټه* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), case_پلنوالی - 1, 0UL);
        أرجع ifrootp && ifrootp->backp() ? ifrootp->clone_ونه(ریښتیا) : ifrootp;
    }

    Ast_غوټه_بیان* newCaseCompare(ثابت AstCase* casep, Ast_غوټه_بیان* توکی_Exprp) {
        File_کرښه* ثابت flp = توکی_Exprp->fileline();
        إذا (never_توکی(casep, توکی_Exprp)) {
            VL_DO_DANGLING(ټېل_Deletep(توکی_Exprp), توکی_Exprp);
            أرجع جديد AstConst{flp, AstConst::بټ_False{}};
        }

        إذا (AstCond* ثابت condp = VN_CAST(توکی_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_غوټه_بیان* ثابت testp = condp->condp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_شا());
            Ast_غوټه_بیان* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_شا());
            VL_DO_DANGLING2(ټېل_Deletep(توکی_Exprp), توکی_Exprp, condp);
            أرجع جديد AstCond{flp, testp, thenp, elsep};
        }

        Ast_غوټه_بیان* ثابت caseExprp = casep->exprp()->clone_ونه_خالص(خطأ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        إذا (AstInsideRange* ثابت توکی_Rangep = VN_CAST(توکی_Exprp, InsideRange)) {
            Ast_غوټه_بیان* ثابت resultp = توکی_Rangep->newAndFromInside(
                caseExprp, توکی_Rangep->lhsp()->unlink_Fr_شا(), توکی_Rangep->rhsp()->unlink_Fr_شا());
            VL_DO_DANGLING2(ټېل_Deletep(توکی_Exprp), توکی_Exprp, توکی_Rangep);
            أرجع resultp;
        }

        إذا (AstConst* ثابت توکی_Constp = VN_CAST(توکی_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            إذا (توکی_Constp->عدد().isFourState()) {
                ثابت خپلکاری& سمون = سمون_نمونه(casep, توکی_Constp);
                ثابت V3_شمېره& سمون_ماسک = سمون.لومړی;
                ثابت V3_شمېره& سمون_بټونه = سمون.دوهم;
                VL_DO_DANGLING2(ټېل_Deletep(توکی_Exprp), توکی_Exprp, توکی_Constp);
                أرجع AstEq::newTyped(flp, جديد AstConst{flp, سمون_بټونه},
                                       جديد AstAnd{flp, caseExprp, جديد AstConst{flp, سمون_ماسک}});
            }
        }

        أرجع AstEq::newTyped(flp, caseExprp, توکی_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_غوټه* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        منطقي hasDefault = خطأ;
        لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {

            File_کرښه* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            إذا (itemp->isDefault()) {
                itemp->جمع_کړه_Condsp(جديد AstConst{flp, AstConst::بټ_True{}});
                hasDefault = ریښتیا;
                استمر;
            }

            // Regular clause. Construct the condition expression.
            Ast_غوټه_بیان* newCondp = تش_اشاره;
            لكل (Ast_غوټه_بیان *توکی_Exprp = itemp->condsp(), *nextp; توکی_Exprp; توکی_Exprp = nextp) {
                nextp = VN_AS(توکی_Exprp->nextp(), غوټه_بیان);
                توکی_Exprp->unlink_Fr_شا();

                // If case never matches, ignore it
                إذا (never_توکی(nodep, توکی_Exprp)) {
                    VL_DO_DANGLING(ټېل_Deletep(توکی_Exprp), توکی_Exprp);
                    استمر;
                }

                // Compute the term to add to the condition expression
                Ast_غوټه_بیان* ثابت termp = newCaseCompare(nodep, توکی_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جديد AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            إذا (!newCondp) newCondp = جديد AstConst{flp, AstConst::بټ_False{}};
            itemp->جمع_کړه_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        إذا (!hasDefault) {
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_توکی* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), Case_توکی);
                ټېل_Deletep(lastp->condsp()->unlink_Fr_شا_With_بل());
                lastp->جمع_کړه_Condsp(جديد AstConst{lastp->fileline(), AstConst::بټ_True{}});
            } وإلا {
                // Otherwise needs an explicit empty default
                nodep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                    nodep->fileline(), جديد AstConst{nodep->fileline(), AstConst::بټ_True{}},
                    تش_اشاره});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحيح ژوروالی = 0;
        AstIf* grouprootp = تش_اشاره;
        AstIf* groupnextp = تش_اشاره;
        AstIf* itemnextp = تش_اشاره;
        لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {

            // Grab the statements from this item. May be empty.
            Ast_غوټه* ثابت istmtsp = itemp->stmtsp();
            إذا (istmtsp) istmtsp->unlink_Fr_شا_With_بل();

            // Expressioned clause
            Ast_غوټه_بیان* ثابت ifexprp = itemp->condsp()->unlink_Fr_شا();
            {  // Prepare for next group
                إذا (++ژوروالی > CASE_ENCODER_ډله_ژوروالی) ژوروالی = 1;
                إذا (ژوروالی == 1) {  // First group or starting new group
                    itemnextp = تش_اشاره;
                    AstIf* ثابت newp = جديد AstIf{itemp->fileline(), ifexprp->clone_ونه_خالص(ریښتیا)};
                    إذا (groupnextp) {
                        groupnextp->جمع_کړه_Elsesp(newp);
                    } وإلا {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وإلا {  // Continue group, modify if condition to OR in this new condition
                    Ast_غوټه_بیان* ثابت condp = groupnextp->condp()->unlink_Fr_شا();
                    groupnextp->condp(
                        جديد AstOr{ifexprp->fileline(), condp, ifexprp->clone_ونه_خالص(ریښتیا)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_غوټه_بیان* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                إذا (ژوروالی == CASE_ENCODER_ډله_ژوروالی) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_ونه(), itemexprp);
                    itemexprp = جديد AstConst{itemp->fileline(), AstConst::بټ_True{}};
                }
                AstIf* ثابت newp = جديد AstIf{itemp->fileline(), itemexprp, istmtsp};
                إذا (itemnextp) {
                    itemnextp->جمع_کړه_Elsesp(newp);
                } وإلا {
                    groupnextp->جمع_کړه_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        أرجع grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_غوټه* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقي use_جدول = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_جدول()) أرجع خطأ;
            // Not if analysis tells us we can't
            إذا (!m_case_جدول_پلنوالی) أرجع خطأ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            إذا (m_case_جدول_پلنوالی <= CASE_جدول_TINY_بټونه) أرجع ریښتیا;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت اندازه_t branches = std::min<اندازه_t>(nodep->exprp()->پلنوالی(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع ریښتیا;
        }();
        إذا (use_جدول) أرجع convert_Case_جدول(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقي useDecoder = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.fCaseDecoder()) أرجع خطأ;
            // Not if not a decoder pattern
            إذا (m_case_Decoder_ریکارډونه.تشه()) أرجع خطأ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت اندازه_t branches = std::min<اندازه_t>(nodep->exprp()->پلنوالی(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع ریښتیا;
        }();
        إذا (useDecoder) أرجع convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقي use_Fast_بټ_ونه = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_ونه()) أرجع خطأ;
            // Can't do it without the detailed analysis
            إذا (!m_caseDetailsValid) أرجع خطأ;
            // Can't do it if not exhaustive
            إذا (!m_caseDetails.exhaustive) أرجع خطأ;
            // Not worth doing if there are few conditions
            إذا (m_caseNConditions <= 3) أرجع خطأ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت اندازه_t case_پلنوالی = nodep->exprp()->پلنوالی();
            إذا (case_پلنوالی >= 8 && m_caseNConditions <= (case_پلنوالی + 1)) أرجع خطأ;
            // Otherwise use the bit tree
            أرجع ریښتیا;
        }();
        إذا (use_Fast_بټ_ونه) أرجع convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        إذا (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_کوډ::LATCH, ریښتیا);
        }
        أرجع convertCaseGeneric(nodep);
    }

    // VISITORS
    فراغ لیدنه(AstCase* nodep) override {
        UASSERT_OBJ(nodep->exprp()->is_خالص(), nodep,
                    "Impure case expression should have been removed by V3LiftExpr");

        CaseLintVisitor::apply(nodep);

        // Convert any children first
        iterateChildren(nodep);

        // Analyze this case statement
        analyzeCase(nodep);

        // Take the 'notParallelp' statements under the case statement created by V3Assert.
        // If the statement was proven to have no overlaps and all cases covered,
        // it can be removed. Otherwise insert the assertion after the case statement.
        إذا (Ast_غوټه* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_شا_With_بل();
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_ونه(), stmtp);
            } وإلا {
                nodep->جمع_کړه_بل_دلته(stmtp);
            }
        }

        // Convert the case statement and replace the original
        إذا (Ast_غوټه* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }

    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::جمع_کړه_Stat("Optimizations, Cases table normal", m_stats.case_جدول_نورمال);
        V3Stats::جمع_کړه_Stat("Optimizations, Cases table tiny", m_stats.case_جدول_Tiny);
        V3Stats::جمع_کړه_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::جمع_کړه_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::جمع_کړه_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

اندازه_t CaseVisitor::Lhs_ریکارډ::s_بل_Id = 0;

//######################################################################
// Case class functions

فراغ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("case", 0, dump_ونه_Either_Level() >= 3);
}
فراغ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
