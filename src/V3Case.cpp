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
    ثابت Ast_گره* m_casep = اشاره_تهی;

    // METHODS
    قالب <نام_نوع Case_قلم>
    ساكن فراغ detect_مضرب_Defaults(Case_قلم* itemsp) {
        منطقي اصابت_Default = خطأ;
        لكل (Case_قلم* itemp = itemsp; itemp; itemp = Ast_گره::as<Case_قلم>(itemp->nextp())) {
            إذا (!itemp->isDefault()) استمر;
            إذا (اصابت_Default) itemp->v3error("Multiple default statements in case statement.");
            اصابت_Default = درست;
        }
    }

    قالب <نام_نوع Case_قلم>
    فراغ checkXZinNonCaseX(Ast_گره* casep, Ast_گره_عبارت* exprp, Case_قلم* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        لكل (Case_قلم* itemp = itemsp; itemp; itemp = Ast_گره::as<Case_قلم>(itemp->nextp())) {
            iterate_And_بعدی_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    فراغ بازدید(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    فراغ بازدید(AstCase* nodep) override {
        إذا (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ بازدید(AstConst* nodep) override {
        إذا (!nodep->رقم().isFourState()) أرجع;

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
            إذا (nodep->رقم().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            أرجع;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    صریح CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    صریح CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = افتراضي;

عام:
    ساكن فراغ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ساكن فراغ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

صنف CaseVisitor final : عام VNVisitor {
    // Maximum width for detailed analysis
    constexpr ساكن صحيح CASE_DETAILS_MAX_عرض = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساكن صحيح CASE_ENCODER_گروه_عمق = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساكن اندازه_t CASE_جدول_TINY_بیتها = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساكن اندازه_t CASE_جدول_MAX_بیتها = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساكن اندازه_t CASE_جدول_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    بنية Case_رکورد final {
        Ast_Case_قلم* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_گره* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    بنية Lhs_رکورد final {
        Ast_گره_عبارت* lhsp = اشاره_تهی;  // LHS of the assignment
        Ast_گره_تخصیص_بده* preDefaultp = اشاره_تهی;  // Default assignment *before the case statement*
        اندازه_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        اندازه_t n_Case_تخصیص_بده_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        اندازه_t جابه_جایی = 0;  // Offset in the table for this LHS

        ساكن اندازه_t s_بعدی_Id;  // Static unique Id counter
        اندازه_t id = ++s_بعدی_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    بنية Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_جدول_نرمال;  // Cases using table method with normal table
        VDouble0 case_جدول_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت Ast_گره* m_alwaysp = اشاره_تهی;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_جدول_بیرون_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_بیرون_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = اشاره_تهی;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقي m_caseOpaque = خطأ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقي m_caseHasDefault = خطأ;  // Indicates the case statement has a default case
    اندازه_t m_case_NCase_اقلام = 0;  // Number of AstCaseItems in the case statement
    اندازه_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_گره_عبارت>, Lhs_رکورد> m_case_Lhs_رکوردها;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::وکتور<Lhs_رکورد> m_case_Decoder_رکوردها;
    اندازه_t m_case_Decoder_مدخل_عرض = 0;  // Width of each entry in the decoder table
    اندازه_t m_case_جدول_عرض = 0;  // Total width of the case table - 0 means can't optimize
    منطقي m_caseDetailsValid = خطأ;  // Indicates m_caseDetails is valid
    بنية final {
        منطقي exhaustive = خطأ;  // Proven exhaustive
        منطقي exhaustiveOverEnumOnly = خطأ;  // Exhaustive over enum values only
        منطقي noOverlaps = خطأ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_رکورد, 1U << CASE_DETAILS_MAX_عرض> رکوردها;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساكن منطقي never_قلم(ثابت AstCase* casep, ثابت Ast_گره_عبارت* قلم_Exprp) {
        ثابت AstConst* ثابت constp = VN_CAST(قلم_Exprp, Const);
        إذا (!constp) أرجع خطأ;
        إذا (casep->casex() || casep->caseInside()) أرجع خطأ;
        إذا (casep->casez()) أرجع constp->رقم().isAnyX();
        أرجع constp->رقم().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساكن std::pair<V3_عدد, V3_عدد> تطابق_الگو(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_عدد, V3_عدد> pair_ماسک_بیتها{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->عرض(), 0),
            std::forward_as_tuple(condp->fileline(), condp->عرض(), 0)};
        pair_ماسک_بیتها.اولی.op_بیتها_Non_XZ(condp->رقم());
        pair_ماسک_بیتها.دومی.op_بیتها_One(condp->رقم());
        أرجع pair_ماسک_بیتها;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساكن Ast_گره_تخصیص_بده* check_Decoder_تخصیص_بده(Ast_گره* stmtp) {
        // Only Assign and AssignDly are supported
        إذا (!VN_IS(stmtp, تخصیص_بده) && !VN_IS(stmtp, تخصیص_بده_Dly)) أرجع اشاره_تهی;
        Ast_گره_تخصیص_بده* ثابت assp = VN_AS(stmtp, گره_تخصیص_بده);
        // Only if no timing control
        إذا (assp->timingControlp()) أرجع اشاره_تهی;
        // Only if assigning a constant
        إذا (!VN_IS(assp->rhsp(), Const)) أرجع اشاره_تهی;
        // Only if it's a packed value
        Ast_گره_DType* ثابت dtypep = assp->rhsp()->dtypep();
        إذا (dtypep->isString() || dtypep->isDouble()) أرجع اشاره_تهی;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        إذا (assp->lhsp()->exists([](AstVarRef* refp) { أرجع refp->access().isReadOrRW(); })) {
            أرجع اشاره_تهی;
        }
        // This is an assignment that fits the decoder pattern
        أرجع assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقي analyze_Decoder_Case_قلم(Ast_Case_قلم* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1_درون_Use user1_درون_Use;
        لكل (Ast_گره* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_گره_تخصیص_بده* ثابت assp = check_Decoder_تخصیص_بده(stmtp);
            إذا (!assp) أرجع خطأ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                أرجع refp->varScopep()->user1_مجموعه_Once();
            });
            إذا (مضرب_Assignments) أرجع خطأ;
            // Update LhsRecord
            Lhs_رکورد& lhs_رکورد = m_case_Lhs_رکوردها[*assp->lhsp()];
            إذا (!lhs_رکورد.lhsp) lhs_رکورد.lhsp = assp->lhsp();
            lhs_رکورد.nCaseAssigns += VN_IS(assp, تخصیص_بده);
            lhs_رکورد.n_Case_تخصیص_بده_Dlys += VN_IS(assp, تخصیص_بده_Dly);
        }
        أرجع درست;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساكن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        إذا (!nodep->uniquePragma() && !nodep->unique0Pragma()) أرجع اشاره_تهی;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        إذا (!enumDtp) أرجع اشاره_تهی;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        إذا (!basicp) أرجع اشاره_تهی;  // Not simple type (perhaps IEEE illegal)
        أرجع enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقي checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t رقم_Cases = 1UL << nodep->exprp()->عرض();
        منطقي fullyCovered = درست;
        string missing_اقلام;
        لكل (Ast_Enum_قلم* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_قلم)) {
            ثابت خودکار& تطابق = تطابق_الگو(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t ماسک = تطابق.اولی.مقصد_UInt();
            ثابت uint32_t بیتها = تطابق.دومی.مقصد_UInt();

            // Check all cases to see if they cover this enum value/pattern
            لكل (uint32_t i = 0; i < رقم_Cases; ++i) {
                إذا ((i & ماسک) != بیتها) استمر;  // This case is not for this enum value
                إذا (m_caseDetails.رکوردها[i].itemp) استمر;  // Covered case
                إذا (!missing_اقلام.خالی()) missing_اقلام += ", ";
                missing_اقلام += eip->prettyNameQ();
                fullyCovered = خطأ;
                اكسر;
            }
        }
        إذا (!missing_اقلام.خالی() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_اقلام);
        }
        أرجع fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقي checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t رقم_Cases = 1UL << nodep->exprp()->عرض();
        لكل (uint32_t i = 0; i < رقم_Cases; ++i) {
            إذا (m_caseDetails.رکوردها[i].itemp) استمر;  // Covered case
            إذا (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            أرجع خطأ;
        }

        // It's an exhaustive case statement
        أرجع درست;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    فراغ analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t رقم_مقادیر = 1UL << nodep->exprp()->عرض();
        // Clear case records
        لكل (uint32_t i = 0; i < رقم_مقادیر; ++i) {
            m_caseDetails.رکوردها[i].itemp = اشاره_تهی;
            m_caseDetails.رکوردها[i].constp = اشاره_تهی;
            m_caseDetails.رکوردها[i].stmtsp = اشاره_تهی;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقي reportedOverlap = خطأ;
        منطقي hasDefault = خطأ;
        m_caseDetails.noOverlaps = درست;
        لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {

            // Default case
            إذا (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                لكل (uint32_t i = 0; i < رقم_مقادیر; ++i) {
                    Case_رکورد& case_رکورد = m_caseDetails.رکوردها[i];
                    إذا (!case_رکورد.itemp) {
                        case_رکورد.itemp = itemp;
                        case_رکورد.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = درست;
                استمر;
            }

            لكل (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                إذا (never_قلم(nodep, iconstp)) استمر;

                ثابت خودکار& تطابق = تطابق_الگو(nodep, iconstp);
                ثابت uint32_t ماسک = تطابق.اولی.مقصد_UInt();
                ثابت uint32_t بیتها = تطابق.دومی.مقصد_UInt();

                منطقي یافت_New_Case = خطأ;
                ثابت AstConst* اولی_Overlap_Constp = اشاره_تهی;
                uint32_t اولی_Overlap_مقدار = 0;
                لكل (uint32_t i = 0; i < رقم_مقادیر; ++i) {
                    إذا ((i & ماسک) != بیتها) استمر;

                    Case_رکورد& case_رکورد = m_caseDetails.رکوردها[i];

                    // If this is the first case that covers this value, record it
                    إذا (!case_رکورد.itemp) {
                        case_رکورد.itemp = itemp;
                        case_رکورد.constp = iconstp;
                        case_رکورد.stmtsp = itemp->stmtsp();
                        یافت_New_Case = درست;
                        استمر;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    إذا (case_رکورد.itemp == itemp) استمر;

                    // Otherwise record the first overlapping case
                    إذا (!اولی_Overlap_Constp) {
                        اولی_Overlap_Constp = case_رکورد.constp;
                        اولی_Overlap_مقدار = i;
                        m_caseDetails.noOverlaps = خطأ;
                    }
                }

                // Only report first overlap
                إذا (reportedOverlap || !اولی_Overlap_Constp) استمر;

                // Report first overlap
                إذا (nodep->اولویت_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    إذا (!یافت_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << اولی_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << اولی_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = درست;
                    }
                } وإلا {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_الگو;
                    إذا (iconstp->رقم().isAnyXZ()) {
                        example_الگو << " (example pattern 0x" << std::hex << اولی_Overlap_مقدار
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_الگو.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << اولی_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << اولی_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = درست;
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
        m_caseDetailsValid = درست;
    }

    فراغ analyze_Decoder_الگو(AstCase* nodep) {
        // Check each LHS record
        لكل (خودکار it = m_case_Lhs_رکوردها.cbegin(); it != m_case_Lhs_رکوردها.cend();) {
            ثابت Lhs_رکورد& lhs_رکورد = it->دومی;

            // Delete records that have no assignments in any case item (only pre-defaults)
            إذا (!lhs_رکورد.nCaseAssigns && !lhs_رکورد.n_Case_تخصیص_بده_Dlys) {
                it = m_case_Lhs_رکوردها.erase(it);
                استمر;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            إذا (lhs_رکورد.nCaseAssigns && lhs_رکورد.n_Case_تخصیص_بده_Dlys) أرجع;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            إذا (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                إذا (lhs_رکورد.nCaseAssigns == m_case_NCase_اقلام) استمر;
                إذا (lhs_رکورد.n_Case_تخصیص_بده_Dlys == m_case_NCase_اقلام) استمر;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_گره* ثابت preDefaultp = lhs_رکورد.preDefaultp;
            إذا (!preDefaultp) أرجع;
            // And the pre-default needs to be the same type
            إذا (lhs_رکورد.nCaseAssigns && !VN_IS(preDefaultp, تخصیص_بده)) أرجع;
            إذا (lhs_رکورد.n_Case_تخصیص_بده_Dlys && !VN_IS(preDefaultp, تخصیص_بده_Dly)) أرجع;
        }
        // All cases check out, can optimize if there are some entries left
        إذا (m_case_Lhs_رکوردها.خالی()) أرجع;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_رکوردها.reserve(m_case_Lhs_رکوردها.اندازه());
        لكل (ثابت خودکار& قلم_2 : m_case_Lhs_رکوردها) m_case_Decoder_رکوردها.emplace_پشت(قلم_2.دومی);
        std::مرتب_سازی(m_case_Decoder_رکوردها.begin(), m_case_Decoder_رکوردها.انتها(),
                  [](ثابت Lhs_رکورد& a, ثابت Lhs_رکورد& b) {
                      // Sort by width, then id
                      ثابت صحيح a_عرض = a.lhsp->عرض();
                      ثابت صحيح b_عرض = b.lhsp->عرض();
                      إذا (a_عرض != b_عرض) أرجع a_عرض < b_عرض;
                      أرجع a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحيح case_عرض = nodep->exprp()->عرض();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت خودکار میگنجد_حد = [&](اندازه_t مدخل_عرض, اندازه_t max_عرض) -> منطقي {
            اندازه_t مجموع_عرض = مدخل_عرض;
            // Multiply cases - iterative to avoid overflow
            لكل (صحيح i = 0; i < case_عرض; ++i) {
                مجموع_عرض <<= 1;
                إذا (مجموع_عرض > max_عرض) أرجع خطأ;
            }
            أرجع درست;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_مدخل_عرض = 0;
        لكل (Lhs_رکورد& lhs_رکورد : m_case_Decoder_رکوردها) {
            lhs_رکورد.جابه_جایی = m_case_Decoder_مدخل_عرض;
            m_case_Decoder_مدخل_عرض += lhs_رکورد.lhsp->عرض();
        }
        // If it fits, we will pack it tightly
        إذا (میگنجد_حد(m_case_Decoder_مدخل_عرض, CASE_جدول_TINY_بیتها)) {
            m_case_جدول_عرض = m_case_Decoder_مدخل_عرض << case_عرض;  // Can optimize
            أرجع;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_مدخل_عرض = 0;
        لكل (Lhs_رکورد& lhs_رکورد : m_case_Decoder_رکوردها) {
            ثابت اندازه_t عرض = lhs_رکورد.lhsp->عرض();
            ثابت اندازه_t lsb_کلمه = VL_BITWORD_E(m_case_Decoder_مدخل_عرض);
            ثابت اندازه_t msb_کلمه = VL_BITWORD_E(m_case_Decoder_مدخل_عرض + عرض - 1);
            إذا (lsb_کلمه != msb_کلمه) {
                m_case_Decoder_مدخل_عرض = VL_واژگان_I(m_case_Decoder_مدخل_عرض) * VL_EDATASIZE;
            }
            lhs_رکورد.جابه_جایی = m_case_Decoder_مدخل_عرض;
            m_case_Decoder_مدخل_عرض += عرض;
        }
        // Check the table fits max size
        ثابت اندازه_t aligned_مدخل_عرض = VL_واژگان_I(m_case_Decoder_مدخل_عرض) * VL_EDATASIZE;
        إذا (میگنجد_حد(aligned_مدخل_عرض, CASE_جدول_MAX_بیتها)) {
            m_case_جدول_عرض = aligned_مدخل_عرض << case_عرض;  // Can optimize
            أرجع;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    فراغ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = خطأ;
        m_caseHasDefault = خطأ;
        m_case_NCase_اقلام = 0;
        m_caseNConditions = 0;
        m_case_Decoder_رکوردها.clear();
        m_case_Decoder_مدخل_عرض = 0;
        m_case_جدول_عرض = 0;
        m_case_Lhs_رکوردها.clear();
        m_caseDetailsValid = خطأ;

        Ast_گره* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        إذا (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = درست;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1_درون_Use user1_درون_Use;
            لكل (Ast_گره* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_گره_تخصیص_بده* ثابت assp = check_Decoder_تخصیص_بده(prevp);
                إذا (!assp) اكسر;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    أرجع refp->varScopep()->user1_مجموعه_Once();
                });
                إذا (مضرب_Assignments) اكسر;
                // Store pre-default assignment
                Lhs_رکورد& lhs_رکورد = m_case_Lhs_رکوردها[*assp->lhsp()];
                lhs_رکورد.lhsp = assp->lhsp();
                lhs_رکورد.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقي canBeDecoder = درست;
        لكل (Ast_Case_قلم* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_قلم)) {
            // Check conditions
            لكل (Ast_گره* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                إذا (!never_قلم(nodep, VN_AS(condp, گره_عبارت))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                إذا (!VN_IS(condp, Const)) {
                    m_caseOpaque = درست;
                    canBeDecoder = خطأ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            إذا (cip->isDefault()) m_caseHasDefault = درست;
            // Count case items
            ++m_case_NCase_اقلام;
            // Check if it fits the decoder pattern, if still possible
            إذا (canBeDecoder) canBeDecoder = analyze_Decoder_Case_قلم(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        إذا (m_caseOpaque) أرجع;

        // If small enough, analyse details
        إذا (caseExprp->عرض() <= CASE_DETAILS_MAX_عرض) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        إذا (canBeDecoder) analyze_Decoder_الگو(nodep);
    }

    Ast_گره_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_گره_عبارت* exprp, V3SharedTmps& tmps) {
        File_سطر* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        إذا (m_case_Decoder_رکوردها.اندازه() == 1) {
            ثابت Lhs_رکورد& lhs_رکورد = m_case_Decoder_رکوردها[0];
            ثابت صحيح عرض = lhs_رکورد.lhsp->عرض();
            Ast_گره_عبارت* ثابت rhsp
                = exprp->عرض() == عرض ? exprp : جديد AstSel{flp, exprp, 0, عرض};
            Ast_گره_عبارت* ثابت lhsp = lhs_رکورد.lhsp->clone_درخت_خالص(خطأ);
            إذا (lhs_رکورد.nCaseAssigns) {
                أرجع جديد Ast_تخصیص_بده{flp, lhsp, rhsp};
            } وإلا إذا (lhs_رکورد.n_Case_تخصیص_بده_Dlys) {
                أرجع جديد Ast_تخصیص_بده_Dly{flp, lhsp, rhsp};
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت موقت_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_مدخل_عرض);
        Ast_گره_عبارت* ثابت موقت_Writep = جديد AstVarRef{flp, موقت_Vscp, VAccess::WRITE};
        Ast_گره_Stmt* ثابت resultp = جديد Ast_تخصیص_بده{flp, موقت_Writep, exprp};

        // For each LHS, select out the result
        لكل (ثابت Lhs_رکورد& lhs_رکورد : m_case_Decoder_رکوردها) {
            ثابت صحيح عرض = lhs_رکورد.lhsp->عرض();
            ثابت صحيح lsb = lhs_رکورد.جابه_جایی;
            Ast_گره_عبارت* ثابت موقت_Readp = جديد AstVarRef{flp, موقت_Vscp, VAccess::READ};
            Ast_گره_عبارت* ثابت rhsp = جديد AstSel{flp, موقت_Readp, lsb, عرض};
            Ast_گره_عبارت* ثابت lhsp = lhs_رکورد.lhsp->clone_درخت_خالص(خطأ);
            إذا (lhs_رکورد.nCaseAssigns) {
                resultp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
            } وإلا إذا (lhs_رکورد.n_Case_تخصیص_بده_Dlys) {
                resultp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{flp, lhsp, rhsp});
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        أرجع resultp;
    }

    Ast_گره_Stmt* convert_Case_جدول(AstCase* nodep) {
        // Create the table constant
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جديد AstConst{flp, AstConst::Widthed_مقدار{}, static_cast<صحيح>(m_case_جدول_عرض), 0};
        ثابت uint32_t جدول_Entries = 1U << nodep->exprp()->عرض();

        ثابت منطقي is_Tiny_جدول = m_case_جدول_عرض <= CASE_جدول_TINY_بیتها;
        إذا (is_Tiny_جدول) {
            ++m_stats.case_جدول_Tiny;
        } وإلا {
            ++m_stats.case_جدول_نرمال;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t مدخل_عرض = is_Tiny_جدول
                                        ? m_case_Decoder_مدخل_عرض
                                        : VL_واژگان_I(m_case_Decoder_مدخل_عرض) * VL_EDATASIZE;
        لكل (ثابت Lhs_رکورد& lhs_رکورد : m_case_Decoder_رکوردها) {
            ثابت صحيح lhs_عرض = lhs_رکورد.lhsp->عرض();
            ثابت صحيح lhs_جابه_جایی = lhs_رکورد.جابه_جایی;

            // Broadcast the pre-default assignment
            إذا (lhs_رکورد.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhs_رکورد.preDefaultp->rhsp(), Const);
                لكل (uint32_t شاخص = 0; شاخص < جدول_Entries; ++شاخص) {
                    ثابت uint32_t جدول_جابه_جایی = شاخص * مدخل_عرض + lhs_جابه_جایی;
                    tablep->رقم().opSelInto(rhsp->رقم(), جدول_جابه_جایی, lhs_عرض);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            لكل (Ast_Case_قلم* cip = VN_AS(nodep->itemsp()->lastp(), Case_قلم); cip;
                 cip = VN_AS(cip->prevp(), Case_قلم)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    لكل (Ast_گره* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_گره_تخصیص_بده* ثابت ap = VN_AS(stmtp, گره_تخصیص_بده);
                        إذا (lhs_رکورد.lhsp->same_درخت(ap->lhsp())) أرجع VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    أرجع VN_AS(lhs_رکورد.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                إذا (cip->isDefault()) {
                    لكل (uint32_t شاخص = 0; شاخص < جدول_Entries; ++شاخص) {
                        ثابت uint32_t جدول_جابه_جایی = شاخص * مدخل_عرض + lhs_جابه_جایی;
                        tablep->رقم().opSelInto(rhsp->رقم(), جدول_جابه_جایی, lhs_عرض);
                    }
                    استمر;
                }

                // Iterate case conditions in reverse order
                لكل (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    إذا (never_قلم(nodep, condp)) استمر;  // If item never matches, ignore it
                    ثابت خودکار& تطابق = تطابق_الگو(nodep, condp);
                    ثابت uint32_t تطابق_ماسک = تطابق.اولی.مقصد_UInt();
                    ثابت uint32_t تطابق_بیتها = تطابق.دومی.مقصد_UInt();
                    ثابت uint32_t inverse_ماسک = ~تطابق_ماسک & ((1U << condp->عرض()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    لكل (uint32_t i = inverse_ماسک; درست; i = (i - 1) & inverse_ماسک) {
                        ثابت uint32_t شاخص = i | تطابق_بیتها;
                        ثابت uint32_t جدول_جابه_جایی = شاخص * مدخل_عرض + lhs_جابه_جایی;
                        tablep->رقم().opSelInto(rhsp->رقم(), جدول_جابه_جایی, lhs_عرض);
                        إذا (!i) اكسر;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_گره_عبارت* ثابت جدول_Refp = [&]() -> Ast_گره_عبارت* {
            إذا (is_Tiny_جدول) أرجع tablep;
            AstVarRef* ثابت refp = V3ConstPool::پیدا_کن(tablep);
            VL_DO_DANGLING(tablep->delete_درخت(), tablep);  // V3ConstPool::find clones
            أرجع refp;
        }();

        // Create the lookup index
        Ast_گره_عبارت* ثابت caseExprp
            = جديد AstExtend{flp, nodep->exprp()->clone_درخت_خالص(خطأ), 32};
        Ast_گره_عبارت* ثابت scalep = جديد AstConst{flp, مدخل_عرض};
        Ast_گره_عبارت* ثابت جدول_Lsbp = جديد AstMul{flp, scalep, caseExprp};
        Ast_گره_عبارت* ثابت جدول_Selp
            = جديد AstSel{flp, جدول_Refp, جدول_Lsbp, static_cast<صحيح>(m_case_Decoder_مدخل_عرض)};

        // Connect outputs
        أرجع connectDecoderOutputs(nodep, جدول_Selp, m_جدول_بیرون_Tmps);
    }

    Ast_گره_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_سطر* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::وکتور<std::pair<AstConst*, Ast_گره*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        لكل (Ast_Case_قلم* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_قلم)) {
            إذا (cip->isDefault()) {
                clauses.emplace_پشت(اشاره_تهی, cip->stmtsp());
                استمر;
            }
            لكل (Ast_گره* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                إذا (never_قلم(nodep, iconstp)) استمر;
                clauses.emplace_پشت(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقي provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        إذا (clauses.پشت().اولی && !provenExhaustive) clauses.emplace_پشت(اشاره_تهی, اشاره_تهی);

        // Number of entries in decoder table
        ثابت صحيح decoderEnries = clauses.اندازه();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحيح cond_عرض = nodep->exprp()->عرض();
        ثابت صحيح تطابق_عرض = 2 * VL_واژگان_I(cond_عرض) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جديد AstConst{flp, AstConst::Widthed_مقدار{}, decoderEnries * تطابق_عرض, 0};
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            ثابت صحيح مدخل_Lsb = i * تطابق_عرض;

            // If the entry has a condition, use it's match bits and mask
            إذا (AstConst* ثابت condp = clauses[i].اولی) {
                ثابت خودکار& تطابق = تطابق_الگو(nodep, condp);
                matchp->رقم().opSelInto(تطابق.اولی, مدخل_Lsb, cond_عرض);
                matchp->رقم().opSelInto(تطابق.دومی, مدخل_Lsb + تطابق_عرض / 2, cond_عرض);
                استمر;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_عدد رقم_Zero{flp, cond_عرض, 0};
            matchp->رقم().opSelInto(رقم_Zero, مدخل_Lsb, cond_عرض);
            matchp->رقم().opSelInto(رقم_Zero, مدخل_Lsb + تطابق_عرض / 2, cond_عرض);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جديد AstRange{flp, decoderEnries - 1, 0};
        Ast_گره_DType* ثابت مدخل_Dtypep = nodep->پیدا_کن_بیت_DType(
            m_case_Decoder_مدخل_عرض, m_case_Decoder_مدخل_عرض, VSigning::UNSIGNED);
        Ast_گره_DType* ثابت جدول_Dtypep = جديد AstUnpackArrayDType{flp, مدخل_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(جدول_Dtypep);
        AstConsArray* ثابت tablep = جديد AstConsArray{flp, جدول_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            Ast_گره* ثابت stmtsp = clauses[i].دومی;
            AstConst* ثابت entryp = جديد AstConst{flp, AstConst::Widthed_مقدار{},
                                                  static_cast<صحيح>(m_case_Decoder_مدخل_عرض), 0};
            لكل (ثابت Lhs_رکورد& lhs_رکورد : m_case_Decoder_رکوردها) {
                Ast_گره_عبارت* ثابت lhsp = lhs_رکورد.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = اشاره_تهی;
                لكل (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_گره_تخصیص_بده* ثابت assignp = VN_AS(stmtp, گره_تخصیص_بده);
                    إذا (!lhsp->same_درخت(assignp->lhsp())) استمر;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    اكسر;
                }
                // Not assigned in this clause, so use the pre-assigned default
                إذا (!valConstp) {
                    UASSERT_OBJ(lhs_رکورد.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_رکورد.preDefaultp->rhsp(), Const);
                }
                entryp->رقم().opSelInto(valConstp->رقم(), lhs_رکورد.جابه_جایی, lhsp->عرض());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت تطابق_Refp = V3ConstPool::پیدا_کن(matchp);
        AstVarRef* ثابت جدول_Refp = V3ConstPool::پیدا_کن(tablep);
        VL_DO_DANGLING(matchp->delete_درخت(), matchp);
        VL_DO_DANGLING(tablep->delete_درخت(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_گره_عبارت* ثابت caseExprp = nodep->exprp()->clone_درخت_خالص(خطأ);
        Ast_تطابق_Masked* ثابت indexp = جديد Ast_تطابق_Masked{flp, caseExprp, تطابق_Refp};
        Ast_گره_عبارت* ثابت entryp = جديد AstArraySel{flp, جدول_Refp, indexp};

        أرجع connectDecoderOutputs(nodep, entryp, m_decoder_بیرون_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCaseFastRecurse(Ast_گره_عبارت* cexprp, صحيح msb, uint32_t فوقانی_مقدار) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        إذا (msb < 0) أرجع m_caseDetails.رکوردها[فوقانی_مقدار].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t فوقانی_Value0 = فوقانی_مقدار;
        ثابت uint32_t فوقانی_Value1 = فوقانی_مقدار | (1UL << msb);
        Ast_گره* tree0p = convertCaseFastRecurse(cexprp, msb - 1, فوقانی_Value0);
        Ast_گره* tree1p = convertCaseFastRecurse(cexprp, msb - 1, فوقانی_Value1);

        // If same logic on both sides, we can just return one of them
        إذا (tree0p == tree1p) أرجع tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقي same = درست;
            لكل (uint32_t a = فوقانی_Value0, b = فوقانی_Value1; a < فوقانی_Value1; ++a, ++b) {
                إذا (m_caseDetails.رکوردها[a].stmtsp != m_caseDetails.رکوردها[b].stmtsp) {
                    same = خطأ;
                    اكسر;
                }
            }
            إذا (same) {
                VL_DO_DANGLING(tree1p->delete_درخت(), tree1p);
                أرجع tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        إذا (tree0p && tree0p->backp()) tree0p = tree0p->clone_درخت(درست);
        إذا (tree1p && tree1p->backp()) tree1p = tree1p->clone_درخت(درست);
        // Create the If statement
        File_سطر* ثابت flp = cexprp->fileline();
        Ast_گره_عبارت* ثابت condp = جديد AstSel{flp, cexprp->clone_درخت_خالص(خطأ), msb, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp, tree1p, tree0p};
        أرجع ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحيح case_عرض = nodep->exprp()->عرض();
        Ast_گره* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), case_عرض - 1, 0UL);
        أرجع ifrootp && ifrootp->backp() ? ifrootp->clone_درخت(درست) : ifrootp;
    }

    Ast_گره_عبارت* newCaseCompare(ثابت AstCase* casep, Ast_گره_عبارت* قلم_Exprp) {
        File_سطر* ثابت flp = قلم_Exprp->fileline();
        إذا (never_قلم(casep, قلم_Exprp)) {
            VL_DO_DANGLING(افزودن_Deletep(قلم_Exprp), قلم_Exprp);
            أرجع جديد AstConst{flp, AstConst::بیت_False{}};
        }

        إذا (AstCond* ثابت condp = VN_CAST(قلم_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_گره_عبارت* ثابت testp = condp->condp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_پشت());
            Ast_گره_عبارت* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_پشت());
            VL_DO_DANGLING2(افزودن_Deletep(قلم_Exprp), قلم_Exprp, condp);
            أرجع جديد AstCond{flp, testp, thenp, elsep};
        }

        Ast_گره_عبارت* ثابت caseExprp = casep->exprp()->clone_درخت_خالص(خطأ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        إذا (AstInsideRange* ثابت قلم_Rangep = VN_CAST(قلم_Exprp, InsideRange)) {
            Ast_گره_عبارت* ثابت resultp = قلم_Rangep->newAndFromInside(
                caseExprp, قلم_Rangep->lhsp()->unlink_Fr_پشت(), قلم_Rangep->rhsp()->unlink_Fr_پشت());
            VL_DO_DANGLING2(افزودن_Deletep(قلم_Exprp), قلم_Exprp, قلم_Rangep);
            أرجع resultp;
        }

        إذا (AstConst* ثابت قلم_Constp = VN_CAST(قلم_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            إذا (قلم_Constp->رقم().isFourState()) {
                ثابت خودکار& تطابق = تطابق_الگو(casep, قلم_Constp);
                ثابت V3_عدد& تطابق_ماسک = تطابق.اولی;
                ثابت V3_عدد& تطابق_بیتها = تطابق.دومی;
                VL_DO_DANGLING2(افزودن_Deletep(قلم_Exprp), قلم_Exprp, قلم_Constp);
                أرجع AstEq::newTyped(flp, جديد AstConst{flp, تطابق_بیتها},
                                       جديد AstAnd{flp, caseExprp, جديد AstConst{flp, تطابق_ماسک}});
            }
        }

        أرجع AstEq::newTyped(flp, caseExprp, قلم_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        منطقي hasDefault = خطأ;
        لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {

            File_سطر* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            إذا (itemp->isDefault()) {
                itemp->جمع_کن_Condsp(جديد AstConst{flp, AstConst::بیت_True{}});
                hasDefault = درست;
                استمر;
            }

            // Regular clause. Construct the condition expression.
            Ast_گره_عبارت* newCondp = اشاره_تهی;
            لكل (Ast_گره_عبارت *قلم_Exprp = itemp->condsp(), *nextp; قلم_Exprp; قلم_Exprp = nextp) {
                nextp = VN_AS(قلم_Exprp->nextp(), گره_عبارت);
                قلم_Exprp->unlink_Fr_پشت();

                // If case never matches, ignore it
                إذا (never_قلم(nodep, قلم_Exprp)) {
                    VL_DO_DANGLING(افزودن_Deletep(قلم_Exprp), قلم_Exprp);
                    استمر;
                }

                // Compute the term to add to the condition expression
                Ast_گره_عبارت* ثابت termp = newCaseCompare(nodep, قلم_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جديد AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            إذا (!newCondp) newCondp = جديد AstConst{flp, AstConst::بیت_False{}};
            itemp->جمع_کن_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        إذا (!hasDefault) {
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_قلم* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), Case_قلم);
                افزودن_Deletep(lastp->condsp()->unlink_Fr_پشت_With_بعدی());
                lastp->جمع_کن_Condsp(جديد AstConst{lastp->fileline(), AstConst::بیت_True{}});
            } وإلا {
                // Otherwise needs an explicit empty default
                nodep->جمع_کن_Itemsp(جديد Ast_Case_قلم{
                    nodep->fileline(), جديد AstConst{nodep->fileline(), AstConst::بیت_True{}},
                    اشاره_تهی});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحيح عمق = 0;
        AstIf* grouprootp = اشاره_تهی;
        AstIf* groupnextp = اشاره_تهی;
        AstIf* itemnextp = اشاره_تهی;
        لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {

            // Grab the statements from this item. May be empty.
            Ast_گره* ثابت istmtsp = itemp->stmtsp();
            إذا (istmtsp) istmtsp->unlink_Fr_پشت_With_بعدی();

            // Expressioned clause
            Ast_گره_عبارت* ثابت ifexprp = itemp->condsp()->unlink_Fr_پشت();
            {  // Prepare for next group
                إذا (++عمق > CASE_ENCODER_گروه_عمق) عمق = 1;
                إذا (عمق == 1) {  // First group or starting new group
                    itemnextp = اشاره_تهی;
                    AstIf* ثابت newp = جديد AstIf{itemp->fileline(), ifexprp->clone_درخت_خالص(درست)};
                    إذا (groupnextp) {
                        groupnextp->جمع_کن_Elsesp(newp);
                    } وإلا {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وإلا {  // Continue group, modify if condition to OR in this new condition
                    Ast_گره_عبارت* ثابت condp = groupnextp->condp()->unlink_Fr_پشت();
                    groupnextp->condp(
                        جديد AstOr{ifexprp->fileline(), condp, ifexprp->clone_درخت_خالص(درست)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_گره_عبارت* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                إذا (عمق == CASE_ENCODER_گروه_عمق) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_درخت(), itemexprp);
                    itemexprp = جديد AstConst{itemp->fileline(), AstConst::بیت_True{}};
                }
                AstIf* ثابت newp = جديد AstIf{itemp->fileline(), itemexprp, istmtsp};
                إذا (itemnextp) {
                    itemnextp->جمع_کن_Elsesp(newp);
                } وإلا {
                    groupnextp->جمع_کن_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        أرجع grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقي use_جدول = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_جدول()) أرجع خطأ;
            // Not if analysis tells us we can't
            إذا (!m_case_جدول_عرض) أرجع خطأ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            إذا (m_case_جدول_عرض <= CASE_جدول_TINY_بیتها) أرجع درست;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت اندازه_t branches = std::min<اندازه_t>(nodep->exprp()->عرض(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع درست;
        }();
        إذا (use_جدول) أرجع convert_Case_جدول(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقي useDecoder = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.fCaseDecoder()) أرجع خطأ;
            // Not if not a decoder pattern
            إذا (m_case_Decoder_رکوردها.خالی()) أرجع خطأ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت اندازه_t branches = std::min<اندازه_t>(nodep->exprp()->عرض(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع درست;
        }();
        إذا (useDecoder) أرجع convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقي use_Fast_بیت_درخت = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_درخت()) أرجع خطأ;
            // Can't do it without the detailed analysis
            إذا (!m_caseDetailsValid) أرجع خطأ;
            // Can't do it if not exhaustive
            إذا (!m_caseDetails.exhaustive) أرجع خطأ;
            // Not worth doing if there are few conditions
            إذا (m_caseNConditions <= 3) أرجع خطأ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت اندازه_t case_عرض = nodep->exprp()->عرض();
            إذا (case_عرض >= 8 && m_caseNConditions <= (case_عرض + 1)) أرجع خطأ;
            // Otherwise use the bit tree
            أرجع درست;
        }();
        إذا (use_Fast_بیت_درخت) أرجع convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        إذا (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_کد::LATCH, درست);
        }
        أرجع convertCaseGeneric(nodep);
    }

    // VISITORS
    فراغ بازدید(AstCase* nodep) override {
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
        إذا (Ast_گره* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_پشت_With_بعدی();
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_درخت(), stmtp);
            } وإلا {
                nodep->جمع_کن_بعدی_اینجا(stmtp);
            }
        }

        // Convert the case statement and replace the original
        إذا (Ast_گره* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وإلا {
            nodep->unlink_Fr_پشت();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ بازدید(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::جمع_کن_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::جمع_کن_Stat("Optimizations, Cases table normal", m_stats.case_جدول_نرمال);
        V3Stats::جمع_کن_Stat("Optimizations, Cases table tiny", m_stats.case_جدول_Tiny);
        V3Stats::جمع_کن_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::جمع_کن_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::جمع_کن_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

اندازه_t CaseVisitor::Lhs_رکورد::s_بعدی_Id = 0;

//######################################################################
// Case class functions

فراغ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("case", 0, dump_درخت_Either_Level() >= 3);
}
فراغ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
