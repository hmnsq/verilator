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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Case.h"

#شامل "V3ConstPool.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت CaseLintVisitor final : عوامی VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ثابت Ast_گرہ* m_casep = خالی_اشاریہ;

    // METHODS
    سانچہ <قسم_نام Case_شے>
    ساکن خالی detect_مضرب_Defaults(Case_شے* itemsp) {
        منطقی کامیابی_Default = جھوٹ;
        برائے (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            اگر (!itemp->isDefault()) جاری;
            اگر (کامیابی_Default) itemp->v3error("Multiple default statements in case statement.");
            کامیابی_Default = سچ;
        }
    }

    سانچہ <قسم_نام Case_شے>
    خالی checkXZinNonCaseX(Ast_گرہ* casep, Ast_گرہ_اظہاریہ* exprp, Case_شے* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        برائے (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            iterate_And_اگلا_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    خالی دورہ(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    خالی دورہ(AstCase* nodep) override {
        اگر (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    خالی دورہ(AstConst* nodep) override {
        اگر (!nodep->عدد().isFourState()) واپس;

        // Error if generate case
        اگر (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            واپس;
        }

        // Otherwise must be a case statement
        ثابت AstCase* ثابت casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        اگر (casep->casex()) واپس;

        اگر (casep->casez() || casep->caseInside()) {
            اگر (nodep->عدد().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            واپس;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    صریح CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    صریح CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = طےشدہ;

عوامی:
    ساکن خالی apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ساکن خالی apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

جماعت CaseVisitor final : عوامی VNVisitor {
    // Maximum width for detailed analysis
    constexpr ساکن صحیح CASE_DETAILS_MAX_چوڑائی = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساکن صحیح CASE_ENCODER_گروہ_گہرائی = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساکن سائز_t CASE_جدول_TINY_بٹ = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساکن سائز_t CASE_جدول_MAX_بٹ = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساکن سائز_t CASE_جدول_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ساخت Case_ریکارڈ final {
        Ast_Case_شے* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_گرہ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ساخت Lhs_ریکارڈ final {
        Ast_گرہ_اظہاریہ* lhsp = خالی_اشاریہ;  // LHS of the assignment
        Ast_گرہ_تفویض* preDefaultp = خالی_اشاریہ;  // Default assignment *before the case statement*
        سائز_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        سائز_t n_Case_تفویض_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        سائز_t انحراف = 0;  // Offset in the table for this LHS

        ساکن سائز_t s_اگلا_Id;  // Static unique Id counter
        سائز_t id = ++s_اگلا_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ساخت Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_جدول_معمول;  // Cases using table method with normal table
        VDouble0 case_جدول_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت Ast_گرہ* m_alwaysp = خالی_اشاریہ;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_جدول_برآمد_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_برآمد_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقی m_caseOpaque = جھوٹ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقی m_caseHasDefault = جھوٹ;  // Indicates the case statement has a default case
    سائز_t m_case_NCase_اشیاء = 0;  // Number of AstCaseItems in the case statement
    سائز_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_گرہ_اظہاریہ>, Lhs_ریکارڈ> m_case_Lhs_اندراجات;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::سمتیہ<Lhs_ریکارڈ> m_case_Decoder_اندراجات;
    سائز_t m_case_Decoder_اندراج_چوڑائی = 0;  // Width of each entry in the decoder table
    سائز_t m_case_جدول_چوڑائی = 0;  // Total width of the case table - 0 means can't optimize
    منطقی m_caseDetailsValid = جھوٹ;  // Indicates m_caseDetails is valid
    ساخت final {
        منطقی exhaustive = جھوٹ;  // Proven exhaustive
        منطقی exhaustiveOverEnumOnly = جھوٹ;  // Exhaustive over enum values only
        منطقی noOverlaps = جھوٹ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_ریکارڈ, 1U << CASE_DETAILS_MAX_چوڑائی> اندراجات;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساکن منطقی never_شے(ثابت AstCase* casep, ثابت Ast_گرہ_اظہاریہ* شے_Exprp) {
        ثابت AstConst* ثابت constp = VN_CAST(شے_Exprp, Const);
        اگر (!constp) واپس جھوٹ;
        اگر (casep->casex() || casep->caseInside()) واپس جھوٹ;
        اگر (casep->casez()) واپس constp->عدد().isAnyX();
        واپس constp->عدد().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساکن std::pair<V3_نمبر, V3_نمبر> مطابقت_نمونہ(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_نمبر, V3_نمبر> pair_نقاب_بٹ{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->چوڑائی(), 0),
            std::forward_as_tuple(condp->fileline(), condp->چوڑائی(), 0)};
        pair_نقاب_بٹ.پہلا.op_بٹ_Non_XZ(condp->عدد());
        pair_نقاب_بٹ.دوسرا.op_بٹ_One(condp->عدد());
        واپس pair_نقاب_بٹ;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساکن Ast_گرہ_تفویض* check_Decoder_تفویض(Ast_گرہ* stmtp) {
        // Only Assign and AssignDly are supported
        اگر (!VN_IS(stmtp, تفویض) && !VN_IS(stmtp, تفویض_Dly)) واپس خالی_اشاریہ;
        Ast_گرہ_تفویض* ثابت assp = VN_AS(stmtp, گرہ_تفویض);
        // Only if no timing control
        اگر (assp->timingControlp()) واپس خالی_اشاریہ;
        // Only if assigning a constant
        اگر (!VN_IS(assp->rhsp(), Const)) واپس خالی_اشاریہ;
        // Only if it's a packed value
        Ast_گرہ_DType* ثابت dtypep = assp->rhsp()->dtypep();
        اگر (dtypep->isString() || dtypep->isDouble()) واپس خالی_اشاریہ;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        اگر (assp->lhsp()->exists([](AstVarRef* refp) { واپس refp->access().isReadOrRW(); })) {
            واپس خالی_اشاریہ;
        }
        // This is an assignment that fits the decoder pattern
        واپس assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقی analyze_Decoder_Case_شے(Ast_Case_شے* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        برائے (Ast_گرہ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_گرہ_تفویض* ثابت assp = check_Decoder_تفویض(stmtp);
            اگر (!assp) واپس جھوٹ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقی مضرب_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                واپس refp->varScopep()->user1_سیٹ_Once();
            });
            اگر (مضرب_Assignments) واپس جھوٹ;
            // Update LhsRecord
            Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Lhs_اندراجات[*assp->lhsp()];
            اگر (!lhs_ریکارڈ.lhsp) lhs_ریکارڈ.lhsp = assp->lhsp();
            lhs_ریکارڈ.nCaseAssigns += VN_IS(assp, تفویض);
            lhs_ریکارڈ.n_Case_تفویض_Dlys += VN_IS(assp, تفویض_Dly);
        }
        واپس سچ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساکن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        اگر (!nodep->uniquePragma() && !nodep->unique0Pragma()) واپس خالی_اشاریہ;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        اگر (!enumDtp) واپس خالی_اشاریہ;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        اگر (!basicp) واپس خالی_اشاریہ;  // Not simple type (perhaps IEEE illegal)
        واپس enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقی checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->چوڑائی();
        منطقی fullyCovered = سچ;
        string missing_اشیاء;
        برائے (Ast_Enum_شے* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_شے)) {
            ثابت خودکار& مطابقت = مطابقت_نمونہ(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t نقاب = مطابقت.پہلا.مقصد_UInt();
            ثابت uint32_t بٹ = مطابقت.دوسرا.مقصد_UInt();

            // Check all cases to see if they cover this enum value/pattern
            برائے (uint32_t i = 0; i < عدد_Cases; ++i) {
                اگر ((i & نقاب) != بٹ) جاری;  // This case is not for this enum value
                اگر (m_caseDetails.اندراجات[i].itemp) جاری;  // Covered case
                اگر (!missing_اشیاء.خالی_خانے()) missing_اشیاء += ", ";
                missing_اشیاء += eip->prettyNameQ();
                fullyCovered = جھوٹ;
                توڑو;
            }
        }
        اگر (!missing_اشیاء.خالی_خانے() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_اشیاء);
        }
        واپس fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقی checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->چوڑائی();
        برائے (uint32_t i = 0; i < عدد_Cases; ++i) {
            اگر (m_caseDetails.اندراجات[i].itemp) جاری;  // Covered case
            اگر (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            واپس جھوٹ;
        }

        // It's an exhaustive case statement
        واپس سچ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    خالی analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t عدد_اقدار = 1UL << nodep->exprp()->چوڑائی();
        // Clear case records
        برائے (uint32_t i = 0; i < عدد_اقدار; ++i) {
            m_caseDetails.اندراجات[i].itemp = خالی_اشاریہ;
            m_caseDetails.اندراجات[i].constp = خالی_اشاریہ;
            m_caseDetails.اندراجات[i].stmtsp = خالی_اشاریہ;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقی reportedOverlap = جھوٹ;
        منطقی hasDefault = جھوٹ;
        m_caseDetails.noOverlaps = سچ;
        برائے (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            // Default case
            اگر (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                برائے (uint32_t i = 0; i < عدد_اقدار; ++i) {
                    Case_ریکارڈ& case_ریکارڈ = m_caseDetails.اندراجات[i];
                    اگر (!case_ریکارڈ.itemp) {
                        case_ریکارڈ.itemp = itemp;
                        case_ریکارڈ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = سچ;
                جاری;
            }

            برائے (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                اگر (never_شے(nodep, iconstp)) جاری;

                ثابت خودکار& مطابقت = مطابقت_نمونہ(nodep, iconstp);
                ثابت uint32_t نقاب = مطابقت.پہلا.مقصد_UInt();
                ثابت uint32_t بٹ = مطابقت.دوسرا.مقصد_UInt();

                منطقی ملا_New_Case = جھوٹ;
                ثابت AstConst* پہلا_Overlap_Constp = خالی_اشاریہ;
                uint32_t پہلا_Overlap_قدر = 0;
                برائے (uint32_t i = 0; i < عدد_اقدار; ++i) {
                    اگر ((i & نقاب) != بٹ) جاری;

                    Case_ریکارڈ& case_ریکارڈ = m_caseDetails.اندراجات[i];

                    // If this is the first case that covers this value, record it
                    اگر (!case_ریکارڈ.itemp) {
                        case_ریکارڈ.itemp = itemp;
                        case_ریکارڈ.constp = iconstp;
                        case_ریکارڈ.stmtsp = itemp->stmtsp();
                        ملا_New_Case = سچ;
                        جاری;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    اگر (case_ریکارڈ.itemp == itemp) جاری;

                    // Otherwise record the first overlapping case
                    اگر (!پہلا_Overlap_Constp) {
                        پہلا_Overlap_Constp = case_ریکارڈ.constp;
                        پہلا_Overlap_قدر = i;
                        m_caseDetails.noOverlaps = جھوٹ;
                    }
                }

                // Only report first overlap
                اگر (reportedOverlap || !پہلا_Overlap_Constp) جاری;

                // Report first overlap
                اگر (nodep->ترجیح_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    اگر (!ملا_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << پہلا_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << پہلا_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = سچ;
                    }
                } ورنہ {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_نمونہ;
                    اگر (iconstp->عدد().isAnyXZ()) {
                        example_نمونہ << " (example pattern 0x" << std::hex << پہلا_Overlap_قدر
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_نمونہ.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << پہلا_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << پہلا_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = سچ;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = جھوٹ;
        m_caseDetails.exhaustive = hasDefault;
        اگر (!hasDefault) {
            اگر (ثابت AstEnumDType* ثابت enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ثابت منطقی exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } ورنہ {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = سچ;
    }

    خالی analyze_Decoder_نمونہ(AstCase* nodep) {
        // Check each LHS record
        برائے (خودکار it = m_case_Lhs_اندراجات.cbegin(); it != m_case_Lhs_اندراجات.cend();) {
            ثابت Lhs_ریکارڈ& lhs_ریکارڈ = it->دوسرا;

            // Delete records that have no assignments in any case item (only pre-defaults)
            اگر (!lhs_ریکارڈ.nCaseAssigns && !lhs_ریکارڈ.n_Case_تفویض_Dlys) {
                it = m_case_Lhs_اندراجات.erase(it);
                جاری;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            اگر (lhs_ریکارڈ.nCaseAssigns && lhs_ریکارڈ.n_Case_تفویض_Dlys) واپس;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            اگر (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                اگر (lhs_ریکارڈ.nCaseAssigns == m_case_NCase_اشیاء) جاری;
                اگر (lhs_ریکارڈ.n_Case_تفویض_Dlys == m_case_NCase_اشیاء) جاری;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_گرہ* ثابت preDefaultp = lhs_ریکارڈ.preDefaultp;
            اگر (!preDefaultp) واپس;
            // And the pre-default needs to be the same type
            اگر (lhs_ریکارڈ.nCaseAssigns && !VN_IS(preDefaultp, تفویض)) واپس;
            اگر (lhs_ریکارڈ.n_Case_تفویض_Dlys && !VN_IS(preDefaultp, تفویض_Dly)) واپس;
        }
        // All cases check out, can optimize if there are some entries left
        اگر (m_case_Lhs_اندراجات.خالی_خانے()) واپس;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_اندراجات.reserve(m_case_Lhs_اندراجات.سائز());
        برائے (ثابت خودکار& شے_3 : m_case_Lhs_اندراجات) m_case_Decoder_اندراجات.emplace_عقب(شے_3.دوسرا);
        std::ترتیب_دو(m_case_Decoder_اندراجات.begin(), m_case_Decoder_اندراجات.انتہا(),
                  [](ثابت Lhs_ریکارڈ& a, ثابت Lhs_ریکارڈ& b) {
                      // Sort by width, then id
                      ثابت صحیح a_چوڑائی = a.lhsp->چوڑائی();
                      ثابت صحیح b_چوڑائی = b.lhsp->چوڑائی();
                      اگر (a_چوڑائی != b_چوڑائی) واپس a_چوڑائی < b_چوڑائی;
                      واپس a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحیح case_چوڑائی = nodep->exprp()->چوڑائی();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت خودکار سماتا_حد_بندی = [&](سائز_t اندراج_چوڑائی, سائز_t max_چوڑائی) -> منطقی {
            سائز_t کل_چوڑائی = اندراج_چوڑائی;
            // Multiply cases - iterative to avoid overflow
            برائے (صحیح i = 0; i < case_چوڑائی; ++i) {
                کل_چوڑائی <<= 1;
                اگر (کل_چوڑائی > max_چوڑائی) واپس جھوٹ;
            }
            واپس سچ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_اندراج_چوڑائی = 0;
        برائے (Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_اندراجات) {
            lhs_ریکارڈ.انحراف = m_case_Decoder_اندراج_چوڑائی;
            m_case_Decoder_اندراج_چوڑائی += lhs_ریکارڈ.lhsp->چوڑائی();
        }
        // If it fits, we will pack it tightly
        اگر (سماتا_حد_بندی(m_case_Decoder_اندراج_چوڑائی, CASE_جدول_TINY_بٹ)) {
            m_case_جدول_چوڑائی = m_case_Decoder_اندراج_چوڑائی << case_چوڑائی;  // Can optimize
            واپس;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_اندراج_چوڑائی = 0;
        برائے (Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_اندراجات) {
            ثابت سائز_t چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت سائز_t lsb_لفظ = VL_BITWORD_E(m_case_Decoder_اندراج_چوڑائی);
            ثابت سائز_t msb_لفظ = VL_BITWORD_E(m_case_Decoder_اندراج_چوڑائی + چوڑائی - 1);
            اگر (lsb_لفظ != msb_لفظ) {
                m_case_Decoder_اندراج_چوڑائی = VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
            }
            lhs_ریکارڈ.انحراف = m_case_Decoder_اندراج_چوڑائی;
            m_case_Decoder_اندراج_چوڑائی += چوڑائی;
        }
        // Check the table fits max size
        ثابت سائز_t aligned_اندراج_چوڑائی = VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
        اگر (سماتا_حد_بندی(aligned_اندراج_چوڑائی, CASE_جدول_MAX_بٹ)) {
            m_case_جدول_چوڑائی = aligned_اندراج_چوڑائی << case_چوڑائی;  // Can optimize
            واپس;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    خالی analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = جھوٹ;
        m_caseHasDefault = جھوٹ;
        m_case_NCase_اشیاء = 0;
        m_caseNConditions = 0;
        m_case_Decoder_اندراجات.clear();
        m_case_Decoder_اندراج_چوڑائی = 0;
        m_case_جدول_چوڑائی = 0;
        m_case_Lhs_اندراجات.clear();
        m_caseDetailsValid = جھوٹ;

        Ast_گرہ* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        اگر (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = سچ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1_اندر_Use user1_اندر_Use;
            برائے (Ast_گرہ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_گرہ_تفویض* ثابت assp = check_Decoder_تفویض(prevp);
                اگر (!assp) توڑو;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقی مضرب_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    واپس refp->varScopep()->user1_سیٹ_Once();
                });
                اگر (مضرب_Assignments) توڑو;
                // Store pre-default assignment
                Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Lhs_اندراجات[*assp->lhsp()];
                lhs_ریکارڈ.lhsp = assp->lhsp();
                lhs_ریکارڈ.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقی canBeDecoder = سچ;
        برائے (Ast_Case_شے* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شے)) {
            // Check conditions
            برائے (Ast_گرہ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                اگر (!never_شے(nodep, VN_AS(condp, گرہ_اظہاریہ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                اگر (!VN_IS(condp, Const)) {
                    m_caseOpaque = سچ;
                    canBeDecoder = جھوٹ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            اگر (cip->isDefault()) m_caseHasDefault = سچ;
            // Count case items
            ++m_case_NCase_اشیاء;
            // Check if it fits the decoder pattern, if still possible
            اگر (canBeDecoder) canBeDecoder = analyze_Decoder_Case_شے(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        اگر (m_caseOpaque) واپس;

        // If small enough, analyse details
        اگر (caseExprp->چوڑائی() <= CASE_DETAILS_MAX_چوڑائی) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        اگر (canBeDecoder) analyze_Decoder_نمونہ(nodep);
    }

    Ast_گرہ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_گرہ_اظہاریہ* exprp, V3SharedTmps& tmps) {
        File_سطر* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        اگر (m_case_Decoder_اندراجات.سائز() == 1) {
            ثابت Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Decoder_اندراجات[0];
            ثابت صحیح چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            Ast_گرہ_اظہاریہ* ثابت rhsp
                = exprp->چوڑائی() == چوڑائی ? exprp : نیا AstSel{flp, exprp, 0, چوڑائی};
            Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp->clone_درخت_خالص(جھوٹ);
            اگر (lhs_ریکارڈ.nCaseAssigns) {
                واپس نیا Ast_تفویض{flp, lhsp, rhsp};
            } ورنہ اگر (lhs_ریکارڈ.n_Case_تفویض_Dlys) {
                واپس نیا Ast_تفویض_Dly{flp, lhsp, rhsp};
            } ورنہ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت عارضی_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_اندراج_چوڑائی);
        Ast_گرہ_اظہاریہ* ثابت عارضی_Writep = نیا AstVarRef{flp, عارضی_Vscp, VAccess::WRITE};
        Ast_گرہ_Stmt* ثابت resultp = نیا Ast_تفویض{flp, عارضی_Writep, exprp};

        // For each LHS, select out the result
        برائے (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_اندراجات) {
            ثابت صحیح چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت صحیح lsb = lhs_ریکارڈ.انحراف;
            Ast_گرہ_اظہاریہ* ثابت عارضی_Readp = نیا AstVarRef{flp, عارضی_Vscp, VAccess::READ};
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstSel{flp, عارضی_Readp, lsb, چوڑائی};
            Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp->clone_درخت_خالص(جھوٹ);
            اگر (lhs_ریکارڈ.nCaseAssigns) {
                resultp->جوڑو_اگلا(نیا Ast_تفویض{flp, lhsp, rhsp});
            } ورنہ اگر (lhs_ریکارڈ.n_Case_تفویض_Dlys) {
                resultp->جوڑو_اگلا(نیا Ast_تفویض_Dly{flp, lhsp, rhsp});
            } ورنہ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        واپس resultp;
    }

    Ast_گرہ_Stmt* convert_Case_جدول(AstCase* nodep) {
        // Create the table constant
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = نیا AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحیح>(m_case_جدول_چوڑائی), 0};
        ثابت uint32_t جدول_Entries = 1U << nodep->exprp()->چوڑائی();

        ثابت منطقی is_Tiny_جدول = m_case_جدول_چوڑائی <= CASE_جدول_TINY_بٹ;
        اگر (is_Tiny_جدول) {
            ++m_stats.case_جدول_Tiny;
        } ورنہ {
            ++m_stats.case_جدول_معمول;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t اندراج_چوڑائی = is_Tiny_جدول
                                        ? m_case_Decoder_اندراج_چوڑائی
                                        : VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
        برائے (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_اندراجات) {
            ثابت صحیح lhs_چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت صحیح lhs_انحراف = lhs_ریکارڈ.انحراف;

            // Broadcast the pre-default assignment
            اگر (lhs_ریکارڈ.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                برائے (uint32_t اشاریہ = 0; اشاریہ < جدول_Entries; ++اشاریہ) {
                    ثابت uint32_t جدول_انحراف = اشاریہ * اندراج_چوڑائی + lhs_انحراف;
                    tablep->عدد().opSelInto(rhsp->عدد(), جدول_انحراف, lhs_چوڑائی);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            برائے (Ast_Case_شے* cip = VN_AS(nodep->itemsp()->lastp(), Case_شے); cip;
                 cip = VN_AS(cip->prevp(), Case_شے)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    برائے (Ast_گرہ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_گرہ_تفویض* ثابت ap = VN_AS(stmtp, گرہ_تفویض);
                        اگر (lhs_ریکارڈ.lhsp->same_درخت(ap->lhsp())) واپس VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    واپس VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                اگر (cip->isDefault()) {
                    برائے (uint32_t اشاریہ = 0; اشاریہ < جدول_Entries; ++اشاریہ) {
                        ثابت uint32_t جدول_انحراف = اشاریہ * اندراج_چوڑائی + lhs_انحراف;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_انحراف, lhs_چوڑائی);
                    }
                    جاری;
                }

                // Iterate case conditions in reverse order
                برائے (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    اگر (never_شے(nodep, condp)) جاری;  // If item never matches, ignore it
                    ثابت خودکار& مطابقت = مطابقت_نمونہ(nodep, condp);
                    ثابت uint32_t مطابقت_نقاب = مطابقت.پہلا.مقصد_UInt();
                    ثابت uint32_t مطابقت_بٹ = مطابقت.دوسرا.مقصد_UInt();
                    ثابت uint32_t inverse_نقاب = ~مطابقت_نقاب & ((1U << condp->چوڑائی()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    برائے (uint32_t i = inverse_نقاب; سچ; i = (i - 1) & inverse_نقاب) {
                        ثابت uint32_t اشاریہ = i | مطابقت_بٹ;
                        ثابت uint32_t جدول_انحراف = اشاریہ * اندراج_چوڑائی + lhs_انحراف;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_انحراف, lhs_چوڑائی);
                        اگر (!i) توڑو;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_گرہ_اظہاریہ* ثابت جدول_Refp = [&]() -> Ast_گرہ_اظہاریہ* {
            اگر (is_Tiny_جدول) واپس tablep;
            AstVarRef* ثابت refp = V3ConstPool::ڈھونڈو(tablep);
            VL_DO_DANGLING(tablep->delete_درخت(), tablep);  // V3ConstPool::find clones
            واپس refp;
        }();

        // Create the lookup index
        Ast_گرہ_اظہاریہ* ثابت caseExprp
            = نیا AstExtend{flp, nodep->exprp()->clone_درخت_خالص(جھوٹ), 32};
        Ast_گرہ_اظہاریہ* ثابت scalep = نیا AstConst{flp, اندراج_چوڑائی};
        Ast_گرہ_اظہاریہ* ثابت جدول_Lsbp = نیا AstMul{flp, scalep, caseExprp};
        Ast_گرہ_اظہاریہ* ثابت جدول_Selp
            = نیا AstSel{flp, جدول_Refp, جدول_Lsbp, static_cast<صحیح>(m_case_Decoder_اندراج_چوڑائی)};

        // Connect outputs
        واپس connectDecoderOutputs(nodep, جدول_Selp, m_جدول_برآمد_Tmps);
    }

    Ast_گرہ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_سطر* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::سمتیہ<std::pair<AstConst*, Ast_گرہ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        برائے (Ast_Case_شے* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شے)) {
            اگر (cip->isDefault()) {
                clauses.emplace_عقب(خالی_اشاریہ, cip->stmtsp());
                جاری;
            }
            برائے (Ast_گرہ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                اگر (never_شے(nodep, iconstp)) جاری;
                clauses.emplace_عقب(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقی provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        اگر (clauses.عقب().پہلا && !provenExhaustive) clauses.emplace_عقب(خالی_اشاریہ, خالی_اشاریہ);

        // Number of entries in decoder table
        ثابت صحیح decoderEnries = clauses.سائز();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحیح cond_چوڑائی = nodep->exprp()->چوڑائی();
        ثابت صحیح مطابقت_چوڑائی = 2 * VL_الفاظ_I(cond_چوڑائی) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = نیا AstConst{flp, AstConst::Widthed_قدر{}, decoderEnries * مطابقت_چوڑائی, 0};
        برائے (صحیح i = 0; i < decoderEnries; ++i) {
            ثابت صحیح اندراج_Lsb = i * مطابقت_چوڑائی;

            // If the entry has a condition, use it's match bits and mask
            اگر (AstConst* ثابت condp = clauses[i].پہلا) {
                ثابت خودکار& مطابقت = مطابقت_نمونہ(nodep, condp);
                matchp->عدد().opSelInto(مطابقت.پہلا, اندراج_Lsb, cond_چوڑائی);
                matchp->عدد().opSelInto(مطابقت.دوسرا, اندراج_Lsb + مطابقت_چوڑائی / 2, cond_چوڑائی);
                جاری;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_نمبر عدد_Zero{flp, cond_چوڑائی, 0};
            matchp->عدد().opSelInto(عدد_Zero, اندراج_Lsb, cond_چوڑائی);
            matchp->عدد().opSelInto(عدد_Zero, اندراج_Lsb + مطابقت_چوڑائی / 2, cond_چوڑائی);
        }

        // Create the table initializer
        AstRange* ثابت rangep = نیا AstRange{flp, decoderEnries - 1, 0};
        Ast_گرہ_DType* ثابت اندراج_Dtypep = nodep->ڈھونڈو_ثنائی_ہندسہ_DType(
            m_case_Decoder_اندراج_چوڑائی, m_case_Decoder_اندراج_چوڑائی, VSigning::UNSIGNED);
        Ast_گرہ_DType* ثابت جدول_Dtypep = نیا AstUnpackArrayDType{flp, اندراج_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(جدول_Dtypep);
        AstConsArray* ثابت tablep = نیا AstConsArray{flp, جدول_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        برائے (صحیح i = 0; i < decoderEnries; ++i) {
            Ast_گرہ* ثابت stmtsp = clauses[i].دوسرا;
            AstConst* ثابت entryp = نیا AstConst{flp, AstConst::Widthed_قدر{},
                                                  static_cast<صحیح>(m_case_Decoder_اندراج_چوڑائی), 0};
            برائے (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_اندراجات) {
                Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = خالی_اشاریہ;
                برائے (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_گرہ_تفویض* ثابت assignp = VN_AS(stmtp, گرہ_تفویض);
                    اگر (!lhsp->same_درخت(assignp->lhsp())) جاری;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    توڑو;
                }
                // Not assigned in this clause, so use the pre-assigned default
                اگر (!valConstp) {
                    UASSERT_OBJ(lhs_ریکارڈ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                }
                entryp->عدد().opSelInto(valConstp->عدد(), lhs_ریکارڈ.انحراف, lhsp->چوڑائی());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت مطابقت_Refp = V3ConstPool::ڈھونڈو(matchp);
        AstVarRef* ثابت جدول_Refp = V3ConstPool::ڈھونڈو(tablep);
        VL_DO_DANGLING(matchp->delete_درخت(), matchp);
        VL_DO_DANGLING(tablep->delete_درخت(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_گرہ_اظہاریہ* ثابت caseExprp = nodep->exprp()->clone_درخت_خالص(جھوٹ);
        Ast_مطابقت_Masked* ثابت indexp = نیا Ast_مطابقت_Masked{flp, caseExprp, مطابقت_Refp};
        Ast_گرہ_اظہاریہ* ثابت entryp = نیا AstArraySel{flp, جدول_Refp, indexp};

        واپس connectDecoderOutputs(nodep, entryp, m_decoder_برآمد_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCaseFastRecurse(Ast_گرہ_اظہاریہ* cexprp, صحیح msb, uint32_t بالائی_قدر) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        اگر (msb < 0) واپس m_caseDetails.اندراجات[بالائی_قدر].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t بالائی_Value0 = بالائی_قدر;
        ثابت uint32_t بالائی_Value1 = بالائی_قدر | (1UL << msb);
        Ast_گرہ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, بالائی_Value0);
        Ast_گرہ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, بالائی_Value1);

        // If same logic on both sides, we can just return one of them
        اگر (tree0p == tree1p) واپس tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقی same = سچ;
            برائے (uint32_t a = بالائی_Value0, b = بالائی_Value1; a < بالائی_Value1; ++a, ++b) {
                اگر (m_caseDetails.اندراجات[a].stmtsp != m_caseDetails.اندراجات[b].stmtsp) {
                    same = جھوٹ;
                    توڑو;
                }
            }
            اگر (same) {
                VL_DO_DANGLING(tree1p->delete_درخت(), tree1p);
                واپس tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        اگر (tree0p && tree0p->backp()) tree0p = tree0p->clone_درخت(سچ);
        اگر (tree1p && tree1p->backp()) tree1p = tree1p->clone_درخت(سچ);
        // Create the If statement
        File_سطر* ثابت flp = cexprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت condp = نیا AstSel{flp, cexprp->clone_درخت_خالص(جھوٹ), msb, 1};
        AstIf* ثابت ifp = نیا AstIf{flp, condp, tree1p, tree0p};
        واپس ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحیح case_چوڑائی = nodep->exprp()->چوڑائی();
        Ast_گرہ* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), case_چوڑائی - 1, 0UL);
        واپس ifrootp && ifrootp->backp() ? ifrootp->clone_درخت(سچ) : ifrootp;
    }

    Ast_گرہ_اظہاریہ* newCaseCompare(ثابت AstCase* casep, Ast_گرہ_اظہاریہ* شے_Exprp) {
        File_سطر* ثابت flp = شے_Exprp->fileline();
        اگر (never_شے(casep, شے_Exprp)) {
            VL_DO_DANGLING(ڈالو_Deletep(شے_Exprp), شے_Exprp);
            واپس نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
        }

        اگر (AstCond* ثابت condp = VN_CAST(شے_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_گرہ_اظہاریہ* ثابت testp = condp->condp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_عقب());
            Ast_گرہ_اظہاریہ* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_عقب());
            VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, condp);
            واپس نیا AstCond{flp, testp, thenp, elsep};
        }

        Ast_گرہ_اظہاریہ* ثابت caseExprp = casep->exprp()->clone_درخت_خالص(جھوٹ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        اگر (AstInsideRange* ثابت شے_Rangep = VN_CAST(شے_Exprp, InsideRange)) {
            Ast_گرہ_اظہاریہ* ثابت resultp = شے_Rangep->newAndFromInside(
                caseExprp, شے_Rangep->lhsp()->unlink_Fr_عقب(), شے_Rangep->rhsp()->unlink_Fr_عقب());
            VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, شے_Rangep);
            واپس resultp;
        }

        اگر (AstConst* ثابت شے_Constp = VN_CAST(شے_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            اگر (شے_Constp->عدد().isFourState()) {
                ثابت خودکار& مطابقت = مطابقت_نمونہ(casep, شے_Constp);
                ثابت V3_نمبر& مطابقت_نقاب = مطابقت.پہلا;
                ثابت V3_نمبر& مطابقت_بٹ = مطابقت.دوسرا;
                VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, شے_Constp);
                واپس AstEq::newTyped(flp, نیا AstConst{flp, مطابقت_بٹ},
                                       نیا AstAnd{flp, caseExprp, نیا AstConst{flp, مطابقت_نقاب}});
            }
        }

        واپس AstEq::newTyped(flp, caseExprp, شے_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        منطقی hasDefault = جھوٹ;
        برائے (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            File_سطر* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            اگر (itemp->isDefault()) {
                itemp->جوڑو_Condsp(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}});
                hasDefault = سچ;
                جاری;
            }

            // Regular clause. Construct the condition expression.
            Ast_گرہ_اظہاریہ* newCondp = خالی_اشاریہ;
            برائے (Ast_گرہ_اظہاریہ *شے_Exprp = itemp->condsp(), *nextp; شے_Exprp; شے_Exprp = nextp) {
                nextp = VN_AS(شے_Exprp->nextp(), گرہ_اظہاریہ);
                شے_Exprp->unlink_Fr_عقب();

                // If case never matches, ignore it
                اگر (never_شے(nodep, شے_Exprp)) {
                    VL_DO_DANGLING(ڈالو_Deletep(شے_Exprp), شے_Exprp);
                    جاری;
                }

                // Compute the term to add to the condition expression
                Ast_گرہ_اظہاریہ* ثابت termp = newCaseCompare(nodep, شے_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? نیا AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            اگر (!newCondp) newCondp = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
            itemp->جوڑو_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        اگر (!hasDefault) {
            اگر (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_شے* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), Case_شے);
                ڈالو_Deletep(lastp->condsp()->unlink_Fr_عقب_With_اگلا());
                lastp->جوڑو_Condsp(نیا AstConst{lastp->fileline(), AstConst::ثنائی_ہندسہ_True{}});
            } ورنہ {
                // Otherwise needs an explicit empty default
                nodep->جوڑو_Itemsp(نیا Ast_Case_شے{
                    nodep->fileline(), نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_True{}},
                    خالی_اشاریہ});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحیح گہرائی = 0;
        AstIf* grouprootp = خالی_اشاریہ;
        AstIf* groupnextp = خالی_اشاریہ;
        AstIf* itemnextp = خالی_اشاریہ;
        برائے (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            // Grab the statements from this item. May be empty.
            Ast_گرہ* ثابت istmtsp = itemp->stmtsp();
            اگر (istmtsp) istmtsp->unlink_Fr_عقب_With_اگلا();

            // Expressioned clause
            Ast_گرہ_اظہاریہ* ثابت ifexprp = itemp->condsp()->unlink_Fr_عقب();
            {  // Prepare for next group
                اگر (++گہرائی > CASE_ENCODER_گروہ_گہرائی) گہرائی = 1;
                اگر (گہرائی == 1) {  // First group or starting new group
                    itemnextp = خالی_اشاریہ;
                    AstIf* ثابت newp = نیا AstIf{itemp->fileline(), ifexprp->clone_درخت_خالص(سچ)};
                    اگر (groupnextp) {
                        groupnextp->جوڑو_Elsesp(newp);
                    } ورنہ {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } ورنہ {  // Continue group, modify if condition to OR in this new condition
                    Ast_گرہ_اظہاریہ* ثابت condp = groupnextp->condp()->unlink_Fr_عقب();
                    groupnextp->condp(
                        نیا AstOr{ifexprp->fileline(), condp, ifexprp->clone_درخت_خالص(سچ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_گرہ_اظہاریہ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                اگر (گہرائی == CASE_ENCODER_گروہ_گہرائی) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_درخت(), itemexprp);
                    itemexprp = نیا AstConst{itemp->fileline(), AstConst::ثنائی_ہندسہ_True{}};
                }
                AstIf* ثابت newp = نیا AstIf{itemp->fileline(), itemexprp, istmtsp};
                اگر (itemnextp) {
                    itemnextp->جوڑو_Elsesp(newp);
                } ورنہ {
                    groupnextp->جوڑو_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        واپس grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقی use_جدول = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.f_Case_جدول()) واپس جھوٹ;
            // Not if analysis tells us we can't
            اگر (!m_case_جدول_چوڑائی) واپس جھوٹ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            اگر (m_case_جدول_چوڑائی <= CASE_جدول_TINY_بٹ) واپس سچ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت سائز_t branches = std::min<سائز_t>(nodep->exprp()->چوڑائی(), m_caseNConditions);
            اگر (branches < CASE_جدول_MIN_BRANCHES) واپس جھوٹ;
            واپس سچ;
        }();
        اگر (use_جدول) واپس convert_Case_جدول(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقی useDecoder = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.fCaseDecoder()) واپس جھوٹ;
            // Not if not a decoder pattern
            اگر (m_case_Decoder_اندراجات.خالی_خانے()) واپس جھوٹ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت سائز_t branches = std::min<سائز_t>(nodep->exprp()->چوڑائی(), m_caseNConditions);
            اگر (branches < CASE_جدول_MIN_BRANCHES) واپس جھوٹ;
            واپس سچ;
        }();
        اگر (useDecoder) واپس convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقی use_Fast_ثنائی_ہندسہ_درخت = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.f_Case_درخت()) واپس جھوٹ;
            // Can't do it without the detailed analysis
            اگر (!m_caseDetailsValid) واپس جھوٹ;
            // Can't do it if not exhaustive
            اگر (!m_caseDetails.exhaustive) واپس جھوٹ;
            // Not worth doing if there are few conditions
            اگر (m_caseNConditions <= 3) واپس جھوٹ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت سائز_t case_چوڑائی = nodep->exprp()->چوڑائی();
            اگر (case_چوڑائی >= 8 && m_caseNConditions <= (case_چوڑائی + 1)) واپس جھوٹ;
            // Otherwise use the bit tree
            واپس سچ;
        }();
        اگر (use_Fast_ثنائی_ہندسہ_درخت) واپس convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        اگر (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_رمز::LATCH, سچ);
        }
        واپس convertCaseGeneric(nodep);
    }

    // VISITORS
    خالی دورہ(AstCase* nodep) override {
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
        اگر (Ast_گرہ* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_عقب_With_اگلا();
            اگر (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_درخت(), stmtp);
            } ورنہ {
                nodep->جوڑو_اگلا_یہاں(stmtp);
            }
        }

        // Convert the case statement and replace the original
        اگر (Ast_گرہ* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::جوڑو_Stat("Optimizations, Cases table normal", m_stats.case_جدول_معمول);
        V3Stats::جوڑو_Stat("Optimizations, Cases table tiny", m_stats.case_جدول_Tiny);
        V3Stats::جوڑو_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::جوڑو_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::جوڑو_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

سائز_t CaseVisitor::Lhs_ریکارڈ::s_اگلا_Id = 0;

//######################################################################
// Case class functions

خالی V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("case", 0, dump_درخت_Either_Level() >= 3);
}
خالی V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
