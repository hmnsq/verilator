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
    ثابت Ast_گرہ* m_casep = خالی_سوچک;

    // METHODS
    سانچا <قسم_ناں Case_شے>
    ساكن فراغ detect_مضرب_Defaults(Case_شے* itemsp) {
        منطقي کامیابی_Default = خطأ;
        لكل (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            إذا (!itemp->isDefault()) استمر;
            إذا (کامیابی_Default) itemp->v3error("Multiple default statements in case statement.");
            کامیابی_Default = سچ;
        }
    }

    سانچا <قسم_ناں Case_شے>
    فراغ checkXZinNonCaseX(Ast_گرہ* casep, Ast_گرہ_اظہاریہ* exprp, Case_شے* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        لكل (Case_شے* itemp = itemsp; itemp; itemp = Ast_گرہ::as<Case_شے>(itemp->nextp())) {
            iterate_And_اگلا_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    فراغ ویکھو(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    فراغ ویکھو(AstCase* nodep) override {
        إذا (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_مضرب_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ ویکھو(AstConst* nodep) override {
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
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    واضح CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    واضح CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = افتراضي;

عام:
    ساكن فراغ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ساكن فراغ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

صنف CaseVisitor final : عام VNVisitor {
    // Maximum width for detailed analysis
    constexpr ساكن صحيح CASE_DETAILS_MAX_چوڑائی = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساكن صحيح CASE_ENCODER_گروہ_ڈونگھائی = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساكن ناپ_t CASE_جدول_TINY_بٹ = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساكن ناپ_t CASE_جدول_MAX_بٹ = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساكن ناپ_t CASE_جدول_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    بنية Case_ریکارڈ final {
        Ast_Case_شے* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_گرہ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    بنية Lhs_ریکارڈ final {
        Ast_گرہ_اظہاریہ* lhsp = خالی_سوچک;  // LHS of the assignment
        Ast_گرہ_سونپو* preDefaultp = خالی_سوچک;  // Default assignment *before the case statement*
        ناپ_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        ناپ_t n_Case_سونپو_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        ناپ_t وتھ = 0;  // Offset in the table for this LHS

        ساكن ناپ_t s_اگلا_Id;  // Static unique Id counter
        ناپ_t id = ++s_اگلا_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    بنية Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_جدول_عام;  // Cases using table method with normal table
        VDouble0 case_جدول_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت Ast_گرہ* m_alwaysp = خالی_سوچک;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_جدول_باہرلا_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_باہرلا_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = خالی_سوچک;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقي m_caseOpaque = خطأ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقي m_caseHasDefault = خطأ;  // Indicates the case statement has a default case
    ناپ_t m_case_NCase_اشیاء = 0;  // Number of AstCaseItems in the case statement
    ناپ_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_گرہ_اظہاریہ>, Lhs_ریکارڈ> m_case_Lhs_ریکارڈاں;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::سمتیہ<Lhs_ریکارڈ> m_case_Decoder_ریکارڈاں;
    ناپ_t m_case_Decoder_اندراج_چوڑائی = 0;  // Width of each entry in the decoder table
    ناپ_t m_case_جدول_چوڑائی = 0;  // Total width of the case table - 0 means can't optimize
    منطقي m_caseDetailsValid = خطأ;  // Indicates m_caseDetails is valid
    بنية final {
        منطقي exhaustive = خطأ;  // Proven exhaustive
        منطقي exhaustiveOverEnumOnly = خطأ;  // Exhaustive over enum values only
        منطقي noOverlaps = خطأ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_ریکارڈ, 1U << CASE_DETAILS_MAX_چوڑائی> ریکارڈاں;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساكن منطقي never_شے(ثابت AstCase* casep, ثابت Ast_گرہ_اظہاریہ* شے_Exprp) {
        ثابت AstConst* ثابت constp = VN_CAST(شے_Exprp, Const);
        إذا (!constp) أرجع خطأ;
        إذا (casep->casex() || casep->caseInside()) أرجع خطأ;
        إذا (casep->casez()) أرجع constp->عدد().isAnyX();
        أرجع constp->عدد().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساكن std::pair<V3_نمبر, V3_نمبر> میل_نمونہ(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_نمبر, V3_نمبر> pair_نقاب_بٹ{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->چوڑائی(), 0),
            std::forward_as_tuple(condp->fileline(), condp->چوڑائی(), 0)};
        pair_نقاب_بٹ.پہلا.op_بٹ_Non_XZ(condp->عدد());
        pair_نقاب_بٹ.دوجا.op_بٹ_One(condp->عدد());
        أرجع pair_نقاب_بٹ;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساكن Ast_گرہ_سونپو* check_Decoder_سونپو(Ast_گرہ* stmtp) {
        // Only Assign and AssignDly are supported
        إذا (!VN_IS(stmtp, سونپو) && !VN_IS(stmtp, سونپو_Dly)) أرجع خالی_سوچک;
        Ast_گرہ_سونپو* ثابت assp = VN_AS(stmtp, گرہ_سونپو);
        // Only if no timing control
        إذا (assp->timingControlp()) أرجع خالی_سوچک;
        // Only if assigning a constant
        إذا (!VN_IS(assp->rhsp(), Const)) أرجع خالی_سوچک;
        // Only if it's a packed value
        Ast_گرہ_DType* ثابت dtypep = assp->rhsp()->dtypep();
        إذا (dtypep->isString() || dtypep->isDouble()) أرجع خالی_سوچک;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        إذا (assp->lhsp()->exists([](AstVarRef* refp) { أرجع refp->access().isReadOrRW(); })) {
            أرجع خالی_سوچک;
        }
        // This is an assignment that fits the decoder pattern
        أرجع assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقي analyze_Decoder_Case_شے(Ast_Case_شے* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1_وچ_Use user1_وچ_Use;
        لكل (Ast_گرہ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_گرہ_سونپو* ثابت assp = check_Decoder_سونپو(stmtp);
            إذا (!assp) أرجع خطأ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                أرجع refp->varScopep()->user1_سیٹ_Once();
            });
            إذا (مضرب_Assignments) أرجع خطأ;
            // Update LhsRecord
            Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Lhs_ریکارڈاں[*assp->lhsp()];
            إذا (!lhs_ریکارڈ.lhsp) lhs_ریکارڈ.lhsp = assp->lhsp();
            lhs_ریکارڈ.nCaseAssigns += VN_IS(assp, سونپو);
            lhs_ریکارڈ.n_Case_سونپو_Dlys += VN_IS(assp, سونپو_Dly);
        }
        أرجع سچ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساكن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        إذا (!nodep->uniquePragma() && !nodep->unique0Pragma()) أرجع خالی_سوچک;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enumDtp) أرجع خالی_سوچک;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        إذا (!basicp) أرجع خالی_سوچک;  // Not simple type (perhaps IEEE illegal)
        أرجع enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقي checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->چوڑائی();
        منطقي fullyCovered = سچ;
        string missing_اشیاء;
        لكل (Ast_Enum_شے* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_شے)) {
            ثابت خودکار& میل = میل_نمونہ(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t نقاب = میل.پہلا.منزل_UInt();
            ثابت uint32_t بٹ = میل.دوجا.منزل_UInt();

            // Check all cases to see if they cover this enum value/pattern
            لكل (uint32_t i = 0; i < عدد_Cases; ++i) {
                إذا ((i & نقاب) != بٹ) استمر;  // This case is not for this enum value
                إذا (m_caseDetails.ریکارڈاں[i].itemp) استمر;  // Covered case
                إذا (!missing_اشیاء.خالی_تھاواں()) missing_اشیاء += ", ";
                missing_اشیاء += eip->prettyNameQ();
                fullyCovered = خطأ;
                اكسر;
            }
        }
        إذا (!missing_اشیاء.خالی_تھاواں() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_اشیاء);
        }
        أرجع fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقي checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t عدد_Cases = 1UL << nodep->exprp()->چوڑائی();
        لكل (uint32_t i = 0; i < عدد_Cases; ++i) {
            إذا (m_caseDetails.ریکارڈاں[i].itemp) استمر;  // Covered case
            إذا (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            أرجع خطأ;
        }

        // It's an exhaustive case statement
        أرجع سچ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    فراغ analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t عدد_اقدار = 1UL << nodep->exprp()->چوڑائی();
        // Clear case records
        لكل (uint32_t i = 0; i < عدد_اقدار; ++i) {
            m_caseDetails.ریکارڈاں[i].itemp = خالی_سوچک;
            m_caseDetails.ریکارڈاں[i].constp = خالی_سوچک;
            m_caseDetails.ریکارڈاں[i].stmtsp = خالی_سوچک;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقي reportedOverlap = خطأ;
        منطقي hasDefault = خطأ;
        m_caseDetails.noOverlaps = سچ;
        لكل (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            // Default case
            إذا (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                لكل (uint32_t i = 0; i < عدد_اقدار; ++i) {
                    Case_ریکارڈ& case_ریکارڈ = m_caseDetails.ریکارڈاں[i];
                    إذا (!case_ریکارڈ.itemp) {
                        case_ریکارڈ.itemp = itemp;
                        case_ریکارڈ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = سچ;
                استمر;
            }

            لكل (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                إذا (never_شے(nodep, iconstp)) استمر;

                ثابت خودکار& میل = میل_نمونہ(nodep, iconstp);
                ثابت uint32_t نقاب = میل.پہلا.منزل_UInt();
                ثابت uint32_t بٹ = میل.دوجا.منزل_UInt();

                منطقي لبھیا_New_Case = خطأ;
                ثابت AstConst* پہلا_Overlap_Constp = خالی_سوچک;
                uint32_t پہلا_Overlap_قدر = 0;
                لكل (uint32_t i = 0; i < عدد_اقدار; ++i) {
                    إذا ((i & نقاب) != بٹ) استمر;

                    Case_ریکارڈ& case_ریکارڈ = m_caseDetails.ریکارڈاں[i];

                    // If this is the first case that covers this value, record it
                    إذا (!case_ریکارڈ.itemp) {
                        case_ریکارڈ.itemp = itemp;
                        case_ریکارڈ.constp = iconstp;
                        case_ریکارڈ.stmtsp = itemp->stmtsp();
                        لبھیا_New_Case = سچ;
                        استمر;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    إذا (case_ریکارڈ.itemp == itemp) استمر;

                    // Otherwise record the first overlapping case
                    إذا (!پہلا_Overlap_Constp) {
                        پہلا_Overlap_Constp = case_ریکارڈ.constp;
                        پہلا_Overlap_قدر = i;
                        m_caseDetails.noOverlaps = خطأ;
                    }
                }

                // Only report first overlap
                إذا (reportedOverlap || !پہلا_Overlap_Constp) استمر;

                // Report first overlap
                إذا (nodep->ترجیح_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    إذا (!لبھیا_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << پہلا_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << پہلا_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = سچ;
                    }
                } وإلا {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_نمونہ;
                    إذا (iconstp->عدد().isAnyXZ()) {
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
        m_caseDetailsValid = سچ;
    }

    فراغ analyze_Decoder_نمونہ(AstCase* nodep) {
        // Check each LHS record
        لكل (خودکار it = m_case_Lhs_ریکارڈاں.cbegin(); it != m_case_Lhs_ریکارڈاں.cend();) {
            ثابت Lhs_ریکارڈ& lhs_ریکارڈ = it->دوجا;

            // Delete records that have no assignments in any case item (only pre-defaults)
            إذا (!lhs_ریکارڈ.nCaseAssigns && !lhs_ریکارڈ.n_Case_سونپو_Dlys) {
                it = m_case_Lhs_ریکارڈاں.erase(it);
                استمر;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            إذا (lhs_ریکارڈ.nCaseAssigns && lhs_ریکارڈ.n_Case_سونپو_Dlys) أرجع;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            إذا (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                إذا (lhs_ریکارڈ.nCaseAssigns == m_case_NCase_اشیاء) استمر;
                إذا (lhs_ریکارڈ.n_Case_سونپو_Dlys == m_case_NCase_اشیاء) استمر;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_گرہ* ثابت preDefaultp = lhs_ریکارڈ.preDefaultp;
            إذا (!preDefaultp) أرجع;
            // And the pre-default needs to be the same type
            إذا (lhs_ریکارڈ.nCaseAssigns && !VN_IS(preDefaultp, سونپو)) أرجع;
            إذا (lhs_ریکارڈ.n_Case_سونپو_Dlys && !VN_IS(preDefaultp, سونپو_Dly)) أرجع;
        }
        // All cases check out, can optimize if there are some entries left
        إذا (m_case_Lhs_ریکارڈاں.خالی_تھاواں()) أرجع;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_ریکارڈاں.reserve(m_case_Lhs_ریکارڈاں.ناپ());
        لكل (ثابت خودکار& شے_2 : m_case_Lhs_ریکارڈاں) m_case_Decoder_ریکارڈاں.emplace_پٹھ(شے_2.دوجا);
        std::ترتیب_دیو(m_case_Decoder_ریکارڈاں.begin(), m_case_Decoder_ریکارڈاں.سرا(),
                  [](ثابت Lhs_ریکارڈ& a, ثابت Lhs_ریکارڈ& b) {
                      // Sort by width, then id
                      ثابت صحيح a_چوڑائی = a.lhsp->چوڑائی();
                      ثابت صحيح b_چوڑائی = b.lhsp->چوڑائی();
                      إذا (a_چوڑائی != b_چوڑائی) أرجع a_چوڑائی < b_چوڑائی;
                      أرجع a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحيح case_چوڑائی = nodep->exprp()->چوڑائی();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت خودکار سماندا_حد_بندی = [&](ناپ_t اندراج_چوڑائی, ناپ_t max_چوڑائی) -> منطقي {
            ناپ_t کل_چوڑائی = اندراج_چوڑائی;
            // Multiply cases - iterative to avoid overflow
            لكل (صحيح i = 0; i < case_چوڑائی; ++i) {
                کل_چوڑائی <<= 1;
                إذا (کل_چوڑائی > max_چوڑائی) أرجع خطأ;
            }
            أرجع سچ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_اندراج_چوڑائی = 0;
        لكل (Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_ریکارڈاں) {
            lhs_ریکارڈ.وتھ = m_case_Decoder_اندراج_چوڑائی;
            m_case_Decoder_اندراج_چوڑائی += lhs_ریکارڈ.lhsp->چوڑائی();
        }
        // If it fits, we will pack it tightly
        إذا (سماندا_حد_بندی(m_case_Decoder_اندراج_چوڑائی, CASE_جدول_TINY_بٹ)) {
            m_case_جدول_چوڑائی = m_case_Decoder_اندراج_چوڑائی << case_چوڑائی;  // Can optimize
            أرجع;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_اندراج_چوڑائی = 0;
        لكل (Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_ریکارڈاں) {
            ثابت ناپ_t چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت ناپ_t lsb_لفظ = VL_BITWORD_E(m_case_Decoder_اندراج_چوڑائی);
            ثابت ناپ_t msb_لفظ = VL_BITWORD_E(m_case_Decoder_اندراج_چوڑائی + چوڑائی - 1);
            إذا (lsb_لفظ != msb_لفظ) {
                m_case_Decoder_اندراج_چوڑائی = VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
            }
            lhs_ریکارڈ.وتھ = m_case_Decoder_اندراج_چوڑائی;
            m_case_Decoder_اندراج_چوڑائی += چوڑائی;
        }
        // Check the table fits max size
        ثابت ناپ_t aligned_اندراج_چوڑائی = VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
        إذا (سماندا_حد_بندی(aligned_اندراج_چوڑائی, CASE_جدول_MAX_بٹ)) {
            m_case_جدول_چوڑائی = aligned_اندراج_چوڑائی << case_چوڑائی;  // Can optimize
            أرجع;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    فراغ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = خطأ;
        m_caseHasDefault = خطأ;
        m_case_NCase_اشیاء = 0;
        m_caseNConditions = 0;
        m_case_Decoder_ریکارڈاں.clear();
        m_case_Decoder_اندراج_چوڑائی = 0;
        m_case_جدول_چوڑائی = 0;
        m_case_Lhs_ریکارڈاں.clear();
        m_caseDetailsValid = خطأ;

        Ast_گرہ* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        إذا (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = سچ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1_وچ_Use user1_وچ_Use;
            لكل (Ast_گرہ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_گرہ_سونپو* ثابت assp = check_Decoder_سونپو(prevp);
                إذا (!assp) اكسر;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقي مضرب_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    أرجع refp->varScopep()->user1_سیٹ_Once();
                });
                إذا (مضرب_Assignments) اكسر;
                // Store pre-default assignment
                Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Lhs_ریکارڈاں[*assp->lhsp()];
                lhs_ریکارڈ.lhsp = assp->lhsp();
                lhs_ریکارڈ.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقي canBeDecoder = سچ;
        لكل (Ast_Case_شے* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شے)) {
            // Check conditions
            لكل (Ast_گرہ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                إذا (!never_شے(nodep, VN_AS(condp, گرہ_اظہاریہ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                إذا (!VN_IS(condp, Const)) {
                    m_caseOpaque = سچ;
                    canBeDecoder = خطأ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            إذا (cip->isDefault()) m_caseHasDefault = سچ;
            // Count case items
            ++m_case_NCase_اشیاء;
            // Check if it fits the decoder pattern, if still possible
            إذا (canBeDecoder) canBeDecoder = analyze_Decoder_Case_شے(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        إذا (m_caseOpaque) أرجع;

        // If small enough, analyse details
        إذا (caseExprp->چوڑائی() <= CASE_DETAILS_MAX_چوڑائی) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        إذا (canBeDecoder) analyze_Decoder_نمونہ(nodep);
    }

    Ast_گرہ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_گرہ_اظہاریہ* exprp, V3SharedTmps& tmps) {
        File_سطر* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        إذا (m_case_Decoder_ریکارڈاں.ناپ() == 1) {
            ثابت Lhs_ریکارڈ& lhs_ریکارڈ = m_case_Decoder_ریکارڈاں[0];
            ثابت صحيح چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            Ast_گرہ_اظہاریہ* ثابت rhsp
                = exprp->چوڑائی() == چوڑائی ? exprp : جديد AstSel{flp, exprp, 0, چوڑائی};
            Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp->clone_درخت_خالص(خطأ);
            إذا (lhs_ریکارڈ.nCaseAssigns) {
                أرجع جديد Ast_سونپو{flp, lhsp, rhsp};
            } وإلا إذا (lhs_ریکارڈ.n_Case_سونپو_Dlys) {
                أرجع جديد Ast_سونپو_Dly{flp, lhsp, rhsp};
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت عارضی_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_اندراج_چوڑائی);
        Ast_گرہ_اظہاریہ* ثابت عارضی_Writep = جديد AstVarRef{flp, عارضی_Vscp, VAccess::WRITE};
        Ast_گرہ_Stmt* ثابت resultp = جديد Ast_سونپو{flp, عارضی_Writep, exprp};

        // For each LHS, select out the result
        لكل (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_ریکارڈاں) {
            ثابت صحيح چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت صحيح lsb = lhs_ریکارڈ.وتھ;
            Ast_گرہ_اظہاریہ* ثابت عارضی_Readp = جديد AstVarRef{flp, عارضی_Vscp, VAccess::READ};
            Ast_گرہ_اظہاریہ* ثابت rhsp = جديد AstSel{flp, عارضی_Readp, lsb, چوڑائی};
            Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp->clone_درخت_خالص(خطأ);
            إذا (lhs_ریکارڈ.nCaseAssigns) {
                resultp->جوڑو_اگلا(جديد Ast_سونپو{flp, lhsp, rhsp});
            } وإلا إذا (lhs_ریکارڈ.n_Case_سونپو_Dlys) {
                resultp->جوڑو_اگلا(جديد Ast_سونپو_Dly{flp, lhsp, rhsp});
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        أرجع resultp;
    }

    Ast_گرہ_Stmt* convert_Case_جدول(AstCase* nodep) {
        // Create the table constant
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جديد AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحيح>(m_case_جدول_چوڑائی), 0};
        ثابت uint32_t جدول_Entries = 1U << nodep->exprp()->چوڑائی();

        ثابت منطقي is_Tiny_جدول = m_case_جدول_چوڑائی <= CASE_جدول_TINY_بٹ;
        إذا (is_Tiny_جدول) {
            ++m_stats.case_جدول_Tiny;
        } وإلا {
            ++m_stats.case_جدول_عام;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t اندراج_چوڑائی = is_Tiny_جدول
                                        ? m_case_Decoder_اندراج_چوڑائی
                                        : VL_الفاظ_I(m_case_Decoder_اندراج_چوڑائی) * VL_EDATASIZE;
        لكل (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_ریکارڈاں) {
            ثابت صحيح lhs_چوڑائی = lhs_ریکارڈ.lhsp->چوڑائی();
            ثابت صحيح lhs_وتھ = lhs_ریکارڈ.وتھ;

            // Broadcast the pre-default assignment
            إذا (lhs_ریکارڈ.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                لكل (uint32_t اشاریہ = 0; اشاریہ < جدول_Entries; ++اشاریہ) {
                    ثابت uint32_t جدول_وتھ = اشاریہ * اندراج_چوڑائی + lhs_وتھ;
                    tablep->عدد().opSelInto(rhsp->عدد(), جدول_وتھ, lhs_چوڑائی);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            لكل (Ast_Case_شے* cip = VN_AS(nodep->itemsp()->lastp(), Case_شے); cip;
                 cip = VN_AS(cip->prevp(), Case_شے)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    لكل (Ast_گرہ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_گرہ_سونپو* ثابت ap = VN_AS(stmtp, گرہ_سونپو);
                        إذا (lhs_ریکارڈ.lhsp->same_درخت(ap->lhsp())) أرجع VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    أرجع VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                إذا (cip->isDefault()) {
                    لكل (uint32_t اشاریہ = 0; اشاریہ < جدول_Entries; ++اشاریہ) {
                        ثابت uint32_t جدول_وتھ = اشاریہ * اندراج_چوڑائی + lhs_وتھ;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_وتھ, lhs_چوڑائی);
                    }
                    استمر;
                }

                // Iterate case conditions in reverse order
                لكل (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    إذا (never_شے(nodep, condp)) استمر;  // If item never matches, ignore it
                    ثابت خودکار& میل = میل_نمونہ(nodep, condp);
                    ثابت uint32_t میل_نقاب = میل.پہلا.منزل_UInt();
                    ثابت uint32_t میل_بٹ = میل.دوجا.منزل_UInt();
                    ثابت uint32_t inverse_نقاب = ~میل_نقاب & ((1U << condp->چوڑائی()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    لكل (uint32_t i = inverse_نقاب; سچ; i = (i - 1) & inverse_نقاب) {
                        ثابت uint32_t اشاریہ = i | میل_بٹ;
                        ثابت uint32_t جدول_وتھ = اشاریہ * اندراج_چوڑائی + lhs_وتھ;
                        tablep->عدد().opSelInto(rhsp->عدد(), جدول_وتھ, lhs_چوڑائی);
                        إذا (!i) اكسر;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_گرہ_اظہاریہ* ثابت جدول_Refp = [&]() -> Ast_گرہ_اظہاریہ* {
            إذا (is_Tiny_جدول) أرجع tablep;
            AstVarRef* ثابت refp = V3ConstPool::لبھو(tablep);
            VL_DO_DANGLING(tablep->delete_درخت(), tablep);  // V3ConstPool::find clones
            أرجع refp;
        }();

        // Create the lookup index
        Ast_گرہ_اظہاریہ* ثابت caseExprp
            = جديد AstExtend{flp, nodep->exprp()->clone_درخت_خالص(خطأ), 32};
        Ast_گرہ_اظہاریہ* ثابت scalep = جديد AstConst{flp, اندراج_چوڑائی};
        Ast_گرہ_اظہاریہ* ثابت جدول_Lsbp = جديد AstMul{flp, scalep, caseExprp};
        Ast_گرہ_اظہاریہ* ثابت جدول_Selp
            = جديد AstSel{flp, جدول_Refp, جدول_Lsbp, static_cast<صحيح>(m_case_Decoder_اندراج_چوڑائی)};

        // Connect outputs
        أرجع connectDecoderOutputs(nodep, جدول_Selp, m_جدول_باہرلا_Tmps);
    }

    Ast_گرہ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_سطر* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::سمتیہ<std::pair<AstConst*, Ast_گرہ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        لكل (Ast_Case_شے* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شے)) {
            إذا (cip->isDefault()) {
                clauses.emplace_پٹھ(خالی_سوچک, cip->stmtsp());
                استمر;
            }
            لكل (Ast_گرہ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                إذا (never_شے(nodep, iconstp)) استمر;
                clauses.emplace_پٹھ(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقي provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        إذا (clauses.پٹھ().پہلا && !provenExhaustive) clauses.emplace_پٹھ(خالی_سوچک, خالی_سوچک);

        // Number of entries in decoder table
        ثابت صحيح decoderEnries = clauses.ناپ();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحيح cond_چوڑائی = nodep->exprp()->چوڑائی();
        ثابت صحيح میل_چوڑائی = 2 * VL_الفاظ_I(cond_چوڑائی) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جديد AstConst{flp, AstConst::Widthed_قدر{}, decoderEnries * میل_چوڑائی, 0};
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            ثابت صحيح اندراج_Lsb = i * میل_چوڑائی;

            // If the entry has a condition, use it's match bits and mask
            إذا (AstConst* ثابت condp = clauses[i].پہلا) {
                ثابت خودکار& میل = میل_نمونہ(nodep, condp);
                matchp->عدد().opSelInto(میل.پہلا, اندراج_Lsb, cond_چوڑائی);
                matchp->عدد().opSelInto(میل.دوجا, اندراج_Lsb + میل_چوڑائی / 2, cond_چوڑائی);
                استمر;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_نمبر عدد_Zero{flp, cond_چوڑائی, 0};
            matchp->عدد().opSelInto(عدد_Zero, اندراج_Lsb, cond_چوڑائی);
            matchp->عدد().opSelInto(عدد_Zero, اندراج_Lsb + میل_چوڑائی / 2, cond_چوڑائی);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جديد AstRange{flp, decoderEnries - 1, 0};
        Ast_گرہ_DType* ثابت اندراج_Dtypep = nodep->لبھو_ثنائی_ہندسہ_DType(
            m_case_Decoder_اندراج_چوڑائی, m_case_Decoder_اندراج_چوڑائی, VSigning::UNSIGNED);
        Ast_گرہ_DType* ثابت جدول_Dtypep = جديد AstUnpackArrayDType{flp, اندراج_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(جدول_Dtypep);
        AstConsArray* ثابت tablep = جديد AstConsArray{flp, جدول_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            Ast_گرہ* ثابت stmtsp = clauses[i].دوجا;
            AstConst* ثابت entryp = جديد AstConst{flp, AstConst::Widthed_قدر{},
                                                  static_cast<صحيح>(m_case_Decoder_اندراج_چوڑائی), 0};
            لكل (ثابت Lhs_ریکارڈ& lhs_ریکارڈ : m_case_Decoder_ریکارڈاں) {
                Ast_گرہ_اظہاریہ* ثابت lhsp = lhs_ریکارڈ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = خالی_سوچک;
                لكل (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_گرہ_سونپو* ثابت assignp = VN_AS(stmtp, گرہ_سونپو);
                    إذا (!lhsp->same_درخت(assignp->lhsp())) استمر;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    اكسر;
                }
                // Not assigned in this clause, so use the pre-assigned default
                إذا (!valConstp) {
                    UASSERT_OBJ(lhs_ریکارڈ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_ریکارڈ.preDefaultp->rhsp(), Const);
                }
                entryp->عدد().opSelInto(valConstp->عدد(), lhs_ریکارڈ.وتھ, lhsp->چوڑائی());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت میل_Refp = V3ConstPool::لبھو(matchp);
        AstVarRef* ثابت جدول_Refp = V3ConstPool::لبھو(tablep);
        VL_DO_DANGLING(matchp->delete_درخت(), matchp);
        VL_DO_DANGLING(tablep->delete_درخت(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_گرہ_اظہاریہ* ثابت caseExprp = nodep->exprp()->clone_درخت_خالص(خطأ);
        Ast_میل_Masked* ثابت indexp = جديد Ast_میل_Masked{flp, caseExprp, میل_Refp};
        Ast_گرہ_اظہاریہ* ثابت entryp = جديد AstArraySel{flp, جدول_Refp, indexp};

        أرجع connectDecoderOutputs(nodep, entryp, m_decoder_باہرلا_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCaseFastRecurse(Ast_گرہ_اظہاریہ* cexprp, صحيح msb, uint32_t اتلا_قدر) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        إذا (msb < 0) أرجع m_caseDetails.ریکارڈاں[اتلا_قدر].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t اتلا_Value0 = اتلا_قدر;
        ثابت uint32_t اتلا_Value1 = اتلا_قدر | (1UL << msb);
        Ast_گرہ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, اتلا_Value0);
        Ast_گرہ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, اتلا_Value1);

        // If same logic on both sides, we can just return one of them
        إذا (tree0p == tree1p) أرجع tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقي same = سچ;
            لكل (uint32_t a = اتلا_Value0, b = اتلا_Value1; a < اتلا_Value1; ++a, ++b) {
                إذا (m_caseDetails.ریکارڈاں[a].stmtsp != m_caseDetails.ریکارڈاں[b].stmtsp) {
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
        إذا (tree0p && tree0p->backp()) tree0p = tree0p->clone_درخت(سچ);
        إذا (tree1p && tree1p->backp()) tree1p = tree1p->clone_درخت(سچ);
        // Create the If statement
        File_سطر* ثابت flp = cexprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت condp = جديد AstSel{flp, cexprp->clone_درخت_خالص(خطأ), msb, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp, tree1p, tree0p};
        أرجع ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحيح case_چوڑائی = nodep->exprp()->چوڑائی();
        Ast_گرہ* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), case_چوڑائی - 1, 0UL);
        أرجع ifrootp && ifrootp->backp() ? ifrootp->clone_درخت(سچ) : ifrootp;
    }

    Ast_گرہ_اظہاریہ* newCaseCompare(ثابت AstCase* casep, Ast_گرہ_اظہاریہ* شے_Exprp) {
        File_سطر* ثابت flp = شے_Exprp->fileline();
        إذا (never_شے(casep, شے_Exprp)) {
            VL_DO_DANGLING(ڈالو_Deletep(شے_Exprp), شے_Exprp);
            أرجع جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
        }

        إذا (AstCond* ثابت condp = VN_CAST(شے_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_گرہ_اظہاریہ* ثابت testp = condp->condp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_پٹھ());
            Ast_گرہ_اظہاریہ* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_پٹھ());
            VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, condp);
            أرجع جديد AstCond{flp, testp, thenp, elsep};
        }

        Ast_گرہ_اظہاریہ* ثابت caseExprp = casep->exprp()->clone_درخت_خالص(خطأ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        إذا (AstInsideRange* ثابت شے_Rangep = VN_CAST(شے_Exprp, InsideRange)) {
            Ast_گرہ_اظہاریہ* ثابت resultp = شے_Rangep->newAndFromInside(
                caseExprp, شے_Rangep->lhsp()->unlink_Fr_پٹھ(), شے_Rangep->rhsp()->unlink_Fr_پٹھ());
            VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, شے_Rangep);
            أرجع resultp;
        }

        إذا (AstConst* ثابت شے_Constp = VN_CAST(شے_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            إذا (شے_Constp->عدد().isFourState()) {
                ثابت خودکار& میل = میل_نمونہ(casep, شے_Constp);
                ثابت V3_نمبر& میل_نقاب = میل.پہلا;
                ثابت V3_نمبر& میل_بٹ = میل.دوجا;
                VL_DO_DANGLING2(ڈالو_Deletep(شے_Exprp), شے_Exprp, شے_Constp);
                أرجع AstEq::newTyped(flp, جديد AstConst{flp, میل_بٹ},
                                       جديد AstAnd{flp, caseExprp, جديد AstConst{flp, میل_نقاب}});
            }
        }

        أرجع AstEq::newTyped(flp, caseExprp, شے_Exprp);
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
        منطقي hasDefault = خطأ;
        لكل (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            File_سطر* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            إذا (itemp->isDefault()) {
                itemp->جوڑو_Condsp(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}});
                hasDefault = سچ;
                استمر;
            }

            // Regular clause. Construct the condition expression.
            Ast_گرہ_اظہاریہ* newCondp = خالی_سوچک;
            لكل (Ast_گرہ_اظہاریہ *شے_Exprp = itemp->condsp(), *nextp; شے_Exprp; شے_Exprp = nextp) {
                nextp = VN_AS(شے_Exprp->nextp(), گرہ_اظہاریہ);
                شے_Exprp->unlink_Fr_پٹھ();

                // If case never matches, ignore it
                إذا (never_شے(nodep, شے_Exprp)) {
                    VL_DO_DANGLING(ڈالو_Deletep(شے_Exprp), شے_Exprp);
                    استمر;
                }

                // Compute the term to add to the condition expression
                Ast_گرہ_اظہاریہ* ثابت termp = newCaseCompare(nodep, شے_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جديد AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            إذا (!newCondp) newCondp = جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
            itemp->جوڑو_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        إذا (!hasDefault) {
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_شے* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), Case_شے);
                ڈالو_Deletep(lastp->condsp()->unlink_Fr_پٹھ_With_اگلا());
                lastp->جوڑو_Condsp(جديد AstConst{lastp->fileline(), AstConst::ثنائی_ہندسہ_True{}});
            } وإلا {
                // Otherwise needs an explicit empty default
                nodep->جوڑو_Itemsp(جديد Ast_Case_شے{
                    nodep->fileline(), جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_True{}},
                    خالی_سوچک});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحيح ڈونگھائی = 0;
        AstIf* grouprootp = خالی_سوچک;
        AstIf* groupnextp = خالی_سوچک;
        AstIf* itemnextp = خالی_سوچک;
        لكل (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {

            // Grab the statements from this item. May be empty.
            Ast_گرہ* ثابت istmtsp = itemp->stmtsp();
            إذا (istmtsp) istmtsp->unlink_Fr_پٹھ_With_اگلا();

            // Expressioned clause
            Ast_گرہ_اظہاریہ* ثابت ifexprp = itemp->condsp()->unlink_Fr_پٹھ();
            {  // Prepare for next group
                إذا (++ڈونگھائی > CASE_ENCODER_گروہ_ڈونگھائی) ڈونگھائی = 1;
                إذا (ڈونگھائی == 1) {  // First group or starting new group
                    itemnextp = خالی_سوچک;
                    AstIf* ثابت newp = جديد AstIf{itemp->fileline(), ifexprp->clone_درخت_خالص(سچ)};
                    إذا (groupnextp) {
                        groupnextp->جوڑو_Elsesp(newp);
                    } وإلا {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وإلا {  // Continue group, modify if condition to OR in this new condition
                    Ast_گرہ_اظہاریہ* ثابت condp = groupnextp->condp()->unlink_Fr_پٹھ();
                    groupnextp->condp(
                        جديد AstOr{ifexprp->fileline(), condp, ifexprp->clone_درخت_خالص(سچ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_گرہ_اظہاریہ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                إذا (ڈونگھائی == CASE_ENCODER_گروہ_ڈونگھائی) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_درخت(), itemexprp);
                    itemexprp = جديد AstConst{itemp->fileline(), AstConst::ثنائی_ہندسہ_True{}};
                }
                AstIf* ثابت newp = جديد AstIf{itemp->fileline(), itemexprp, istmtsp};
                إذا (itemnextp) {
                    itemnextp->جوڑو_Elsesp(newp);
                } وإلا {
                    groupnextp->جوڑو_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        أرجع grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_گرہ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقي use_جدول = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_جدول()) أرجع خطأ;
            // Not if analysis tells us we can't
            إذا (!m_case_جدول_چوڑائی) أرجع خطأ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            إذا (m_case_جدول_چوڑائی <= CASE_جدول_TINY_بٹ) أرجع سچ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت ناپ_t branches = std::min<ناپ_t>(nodep->exprp()->چوڑائی(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع سچ;
        }();
        إذا (use_جدول) أرجع convert_Case_جدول(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقي useDecoder = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.fCaseDecoder()) أرجع خطأ;
            // Not if not a decoder pattern
            إذا (m_case_Decoder_ریکارڈاں.خالی_تھاواں()) أرجع خطأ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت ناپ_t branches = std::min<ناپ_t>(nodep->exprp()->چوڑائی(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع سچ;
        }();
        إذا (useDecoder) أرجع convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقي use_Fast_ثنائی_ہندسہ_درخت = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_درخت()) أرجع خطأ;
            // Can't do it without the detailed analysis
            إذا (!m_caseDetailsValid) أرجع خطأ;
            // Can't do it if not exhaustive
            إذا (!m_caseDetails.exhaustive) أرجع خطأ;
            // Not worth doing if there are few conditions
            إذا (m_caseNConditions <= 3) أرجع خطأ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت ناپ_t case_چوڑائی = nodep->exprp()->چوڑائی();
            إذا (case_چوڑائی >= 8 && m_caseNConditions <= (case_چوڑائی + 1)) أرجع خطأ;
            // Otherwise use the bit tree
            أرجع سچ;
        }();
        إذا (use_Fast_ثنائی_ہندسہ_درخت) أرجع convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        إذا (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_کوڈ::LATCH, سچ);
        }
        أرجع convertCaseGeneric(nodep);
    }

    // VISITORS
    فراغ ویکھو(AstCase* nodep) override {
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
        إذا (Ast_گرہ* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_پٹھ_With_اگلا();
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_درخت(), stmtp);
            } وإلا {
                nodep->جوڑو_اگلا_ایتھے(stmtp);
            }
        }

        // Convert the case statement and replace the original
        إذا (Ast_گرہ* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    فراغ ویکھو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::جوڑو_Stat("Optimizations, Cases table normal", m_stats.case_جدول_عام);
        V3Stats::جوڑو_Stat("Optimizations, Cases table tiny", m_stats.case_جدول_Tiny);
        V3Stats::جوڑو_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::جوڑو_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::جوڑو_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

ناپ_t CaseVisitor::Lhs_ریکارڈ::s_اگلا_Id = 0;

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
