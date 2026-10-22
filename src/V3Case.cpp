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
    ثابت Ast_ڳنڍ* m_casep = خالي_اشارو;

    // METHODS
    سانچو <قسم_نالو Case_شي>
    ساكن فراغ detect_گڻو_Defaults(Case_شي* itemsp) {
        منطقي ڪاميابي_Default = خطأ;
        لكل (Case_شي* itemp = itemsp; itemp; itemp = Ast_ڳنڍ::as<Case_شي>(itemp->nextp())) {
            إذا (!itemp->isDefault()) استمر;
            إذا (ڪاميابي_Default) itemp->v3error("Multiple default statements in case statement.");
            ڪاميابي_Default = سچ;
        }
    }

    سانچو <قسم_نالو Case_شي>
    فراغ checkXZinNonCaseX(Ast_ڳنڍ* casep, Ast_ڳنڍ_اظهار* exprp, Case_شي* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        لكل (Case_شي* itemp = itemsp; itemp; itemp = Ast_ڳنڍ::as<Case_شي>(itemp->nextp())) {
            iterate_And_ايندڙ_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    فراغ گهمو(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_گڻو_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    فراغ گهمو(AstCase* nodep) override {
        إذا (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_گڻو_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ گهمو(AstConst* nodep) override {
        إذا (!nodep->انگ().isFourState()) أرجع;

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
            إذا (nodep->انگ().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            أرجع;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

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
    constexpr ساكن صحيح CASE_DETAILS_MAX_ويڪر = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساكن صحيح CASE_ENCODER_گروهه_اونهائي = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساكن سائيز_t CASE_جدول_TINY_بٽ = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساكن سائيز_t CASE_جدول_MAX_بٽ = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساكن سائيز_t CASE_جدول_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    بنية Case_رڪارڊ final {
        Ast_Case_شي* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ڳنڍ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    بنية Lhs_رڪارڊ final {
        Ast_ڳنڍ_اظهار* lhsp = خالي_اشارو;  // LHS of the assignment
        Ast_ڳنڍ_سونپيو* preDefaultp = خالي_اشارو;  // Default assignment *before the case statement*
        سائيز_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        سائيز_t n_Case_سونپيو_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        سائيز_t وٿي = 0;  // Offset in the table for this LHS

        ساكن سائيز_t s_ايندڙ_Id;  // Static unique Id counter
        سائيز_t id = ++s_ايندڙ_Id;  // Unique Id for sorting
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
    ثابت Ast_ڳنڍ* m_alwaysp = خالي_اشارو;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_جدول_ٻاهريون_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_ٻاهريون_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = خالي_اشارو;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقي m_caseOpaque = خطأ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقي m_caseHasDefault = خطأ;  // Indicates the case statement has a default case
    سائيز_t m_case_NCase_شيون = 0;  // Number of AstCaseItems in the case statement
    سائيز_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ڳنڍ_اظهار>, Lhs_رڪارڊ> m_case_Lhs_رڪارڊون;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::ويڪٽر<Lhs_رڪارڊ> m_case_Decoder_رڪارڊون;
    سائيز_t m_case_Decoder_داخلا_ويڪر = 0;  // Width of each entry in the decoder table
    سائيز_t m_case_جدول_ويڪر = 0;  // Total width of the case table - 0 means can't optimize
    منطقي m_caseDetailsValid = خطأ;  // Indicates m_caseDetails is valid
    بنية final {
        منطقي exhaustive = خطأ;  // Proven exhaustive
        منطقي exhaustiveOverEnumOnly = خطأ;  // Exhaustive over enum values only
        منطقي noOverlaps = خطأ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_رڪارڊ, 1U << CASE_DETAILS_MAX_ويڪر> رڪارڊون;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساكن منطقي never_شي(ثابت AstCase* casep, ثابت Ast_ڳنڍ_اظهار* شي_Exprp) {
        ثابت AstConst* ثابت constp = VN_CAST(شي_Exprp, Const);
        إذا (!constp) أرجع خطأ;
        إذا (casep->casex() || casep->caseInside()) أرجع خطأ;
        إذا (casep->casez()) أرجع constp->انگ().isAnyX();
        أرجع constp->انگ().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساكن std::pair<V3_عدد, V3_عدد> ميلاپ_نمونو(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_عدد, V3_عدد> pair_نقاب_بٽ{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->ويڪر(), 0),
            std::forward_as_tuple(condp->fileline(), condp->ويڪر(), 0)};
        pair_نقاب_بٽ.پهريون.op_بٽ_Non_XZ(condp->انگ());
        pair_نقاب_بٽ.ٻيو.op_بٽ_One(condp->انگ());
        أرجع pair_نقاب_بٽ;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساكن Ast_ڳنڍ_سونپيو* check_Decoder_سونپيو(Ast_ڳنڍ* stmtp) {
        // Only Assign and AssignDly are supported
        إذا (!VN_IS(stmtp, سونپيو) && !VN_IS(stmtp, سونپيو_Dly)) أرجع خالي_اشارو;
        Ast_ڳنڍ_سونپيو* ثابت assp = VN_AS(stmtp, ڳنڍ_سونپيو);
        // Only if no timing control
        إذا (assp->timingControlp()) أرجع خالي_اشارو;
        // Only if assigning a constant
        إذا (!VN_IS(assp->rhsp(), Const)) أرجع خالي_اشارو;
        // Only if it's a packed value
        Ast_ڳنڍ_DType* ثابت dtypep = assp->rhsp()->dtypep();
        إذا (dtypep->isString() || dtypep->isDouble()) أرجع خالي_اشارو;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        إذا (assp->lhsp()->exists([](AstVarRef* refp) { أرجع refp->access().isReadOrRW(); })) {
            أرجع خالي_اشارو;
        }
        // This is an assignment that fits the decoder pattern
        أرجع assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقي analyze_Decoder_Case_شي(Ast_Case_شي* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        لكل (Ast_ڳنڍ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ڳنڍ_سونپيو* ثابت assp = check_Decoder_سونپيو(stmtp);
            إذا (!assp) أرجع خطأ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقي گڻو_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                أرجع refp->varScopep()->user1_سيٽ_Once();
            });
            إذا (گڻو_Assignments) أرجع خطأ;
            // Update LhsRecord
            Lhs_رڪارڊ& lhs_رڪارڊ = m_case_Lhs_رڪارڊون[*assp->lhsp()];
            إذا (!lhs_رڪارڊ.lhsp) lhs_رڪارڊ.lhsp = assp->lhsp();
            lhs_رڪارڊ.nCaseAssigns += VN_IS(assp, سونپيو);
            lhs_رڪارڊ.n_Case_سونپيو_Dlys += VN_IS(assp, سونپيو_Dly);
        }
        أرجع سچ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساكن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        إذا (!nodep->uniquePragma() && !nodep->unique0Pragma()) أرجع خالي_اشارو;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enumDtp) أرجع خالي_اشارو;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        إذا (!basicp) أرجع خالي_اشارو;  // Not simple type (perhaps IEEE illegal)
        أرجع enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقي checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t انگ_Cases = 1UL << nodep->exprp()->ويڪر();
        منطقي fullyCovered = سچ;
        string missing_شيون;
        لكل (Ast_Enum_شي* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_شي)) {
            ثابت خودڪار& ميلاپ = ميلاپ_نمونو(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t نقاب = ميلاپ.پهريون.منزل_UInt();
            ثابت uint32_t بٽ = ميلاپ.ٻيو.منزل_UInt();

            // Check all cases to see if they cover this enum value/pattern
            لكل (uint32_t i = 0; i < انگ_Cases; ++i) {
                إذا ((i & نقاب) != بٽ) استمر;  // This case is not for this enum value
                إذا (m_caseDetails.رڪارڊون[i].itemp) استمر;  // Covered case
                إذا (!missing_شيون.خالي_جايون()) missing_شيون += ", ";
                missing_شيون += eip->prettyNameQ();
                fullyCovered = خطأ;
                اكسر;
            }
        }
        إذا (!missing_شيون.خالي_جايون() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_شيون);
        }
        أرجع fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقي checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t انگ_Cases = 1UL << nodep->exprp()->ويڪر();
        لكل (uint32_t i = 0; i < انگ_Cases; ++i) {
            إذا (m_caseDetails.رڪارڊون[i].itemp) استمر;  // Covered case
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
        ثابت uint32_t انگ_قدرون = 1UL << nodep->exprp()->ويڪر();
        // Clear case records
        لكل (uint32_t i = 0; i < انگ_قدرون; ++i) {
            m_caseDetails.رڪارڊون[i].itemp = خالي_اشارو;
            m_caseDetails.رڪارڊون[i].constp = خالي_اشارو;
            m_caseDetails.رڪارڊون[i].stmtsp = خالي_اشارو;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقي reportedOverlap = خطأ;
        منطقي hasDefault = خطأ;
        m_caseDetails.noOverlaps = سچ;
        لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {

            // Default case
            إذا (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                لكل (uint32_t i = 0; i < انگ_قدرون; ++i) {
                    Case_رڪارڊ& case_رڪارڊ = m_caseDetails.رڪارڊون[i];
                    إذا (!case_رڪارڊ.itemp) {
                        case_رڪارڊ.itemp = itemp;
                        case_رڪارڊ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = سچ;
                استمر;
            }

            لكل (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                إذا (never_شي(nodep, iconstp)) استمر;

                ثابت خودڪار& ميلاپ = ميلاپ_نمونو(nodep, iconstp);
                ثابت uint32_t نقاب = ميلاپ.پهريون.منزل_UInt();
                ثابت uint32_t بٽ = ميلاپ.ٻيو.منزل_UInt();

                منطقي مليو_New_Case = خطأ;
                ثابت AstConst* پهريون_Overlap_Constp = خالي_اشارو;
                uint32_t پهريون_Overlap_قدر = 0;
                لكل (uint32_t i = 0; i < انگ_قدرون; ++i) {
                    إذا ((i & نقاب) != بٽ) استمر;

                    Case_رڪارڊ& case_رڪارڊ = m_caseDetails.رڪارڊون[i];

                    // If this is the first case that covers this value, record it
                    إذا (!case_رڪارڊ.itemp) {
                        case_رڪارڊ.itemp = itemp;
                        case_رڪارڊ.constp = iconstp;
                        case_رڪارڊ.stmtsp = itemp->stmtsp();
                        مليو_New_Case = سچ;
                        استمر;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    إذا (case_رڪارڊ.itemp == itemp) استمر;

                    // Otherwise record the first overlapping case
                    إذا (!پهريون_Overlap_Constp) {
                        پهريون_Overlap_Constp = case_رڪارڊ.constp;
                        پهريون_Overlap_قدر = i;
                        m_caseDetails.noOverlaps = خطأ;
                    }
                }

                // Only report first overlap
                إذا (reportedOverlap || !پهريون_Overlap_Constp) استمر;

                // Report first overlap
                إذا (nodep->ترجيح_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    إذا (!مليو_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << پهريون_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << پهريون_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = سچ;
                    }
                } وإلا {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_نمونو;
                    إذا (iconstp->انگ().isAnyXZ()) {
                        example_نمونو << " (example pattern 0x" << std::hex << پهريون_Overlap_قدر
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_نمونو.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << پهريون_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << پهريون_Overlap_Constp->warnContextSecondary());
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

    فراغ analyze_Decoder_نمونو(AstCase* nodep) {
        // Check each LHS record
        لكل (خودڪار it = m_case_Lhs_رڪارڊون.cbegin(); it != m_case_Lhs_رڪارڊون.cend();) {
            ثابت Lhs_رڪارڊ& lhs_رڪارڊ = it->ٻيو;

            // Delete records that have no assignments in any case item (only pre-defaults)
            إذا (!lhs_رڪارڊ.nCaseAssigns && !lhs_رڪارڊ.n_Case_سونپيو_Dlys) {
                it = m_case_Lhs_رڪارڊون.erase(it);
                استمر;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            إذا (lhs_رڪارڊ.nCaseAssigns && lhs_رڪارڊ.n_Case_سونپيو_Dlys) أرجع;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            إذا (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                إذا (lhs_رڪارڊ.nCaseAssigns == m_case_NCase_شيون) استمر;
                إذا (lhs_رڪارڊ.n_Case_سونپيو_Dlys == m_case_NCase_شيون) استمر;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ڳنڍ* ثابت preDefaultp = lhs_رڪارڊ.preDefaultp;
            إذا (!preDefaultp) أرجع;
            // And the pre-default needs to be the same type
            إذا (lhs_رڪارڊ.nCaseAssigns && !VN_IS(preDefaultp, سونپيو)) أرجع;
            إذا (lhs_رڪارڊ.n_Case_سونپيو_Dlys && !VN_IS(preDefaultp, سونپيو_Dly)) أرجع;
        }
        // All cases check out, can optimize if there are some entries left
        إذا (m_case_Lhs_رڪارڊون.خالي_جايون()) أرجع;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_رڪارڊون.reserve(m_case_Lhs_رڪارڊون.سائيز());
        لكل (ثابت خودڪار& شي_2 : m_case_Lhs_رڪارڊون) m_case_Decoder_رڪارڊون.emplace_پٺ(شي_2.ٻيو);
        std::ترتيب_ڏيو(m_case_Decoder_رڪارڊون.begin(), m_case_Decoder_رڪارڊون.پڇاڙي(),
                  [](ثابت Lhs_رڪارڊ& a, ثابت Lhs_رڪارڊ& b) {
                      // Sort by width, then id
                      ثابت صحيح a_ويڪر = a.lhsp->ويڪر();
                      ثابت صحيح b_ويڪر = b.lhsp->ويڪر();
                      إذا (a_ويڪر != b_ويڪر) أرجع a_ويڪر < b_ويڪر;
                      أرجع a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحيح case_ويڪر = nodep->exprp()->ويڪر();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت خودڪار سمائي_حد_بندي = [&](سائيز_t داخلا_ويڪر, سائيز_t max_ويڪر) -> منطقي {
            سائيز_t ڪل_ويڪر = داخلا_ويڪر;
            // Multiply cases - iterative to avoid overflow
            لكل (صحيح i = 0; i < case_ويڪر; ++i) {
                ڪل_ويڪر <<= 1;
                إذا (ڪل_ويڪر > max_ويڪر) أرجع خطأ;
            }
            أرجع سچ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_داخلا_ويڪر = 0;
        لكل (Lhs_رڪارڊ& lhs_رڪارڊ : m_case_Decoder_رڪارڊون) {
            lhs_رڪارڊ.وٿي = m_case_Decoder_داخلا_ويڪر;
            m_case_Decoder_داخلا_ويڪر += lhs_رڪارڊ.lhsp->ويڪر();
        }
        // If it fits, we will pack it tightly
        إذا (سمائي_حد_بندي(m_case_Decoder_داخلا_ويڪر, CASE_جدول_TINY_بٽ)) {
            m_case_جدول_ويڪر = m_case_Decoder_داخلا_ويڪر << case_ويڪر;  // Can optimize
            أرجع;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_داخلا_ويڪر = 0;
        لكل (Lhs_رڪارڊ& lhs_رڪارڊ : m_case_Decoder_رڪارڊون) {
            ثابت سائيز_t ويڪر = lhs_رڪارڊ.lhsp->ويڪر();
            ثابت سائيز_t lsb_لفظ = VL_BITWORD_E(m_case_Decoder_داخلا_ويڪر);
            ثابت سائيز_t msb_لفظ = VL_BITWORD_E(m_case_Decoder_داخلا_ويڪر + ويڪر - 1);
            إذا (lsb_لفظ != msb_لفظ) {
                m_case_Decoder_داخلا_ويڪر = VL_لفظ_I(m_case_Decoder_داخلا_ويڪر) * VL_EDATASIZE;
            }
            lhs_رڪارڊ.وٿي = m_case_Decoder_داخلا_ويڪر;
            m_case_Decoder_داخلا_ويڪر += ويڪر;
        }
        // Check the table fits max size
        ثابت سائيز_t aligned_داخلا_ويڪر = VL_لفظ_I(m_case_Decoder_داخلا_ويڪر) * VL_EDATASIZE;
        إذا (سمائي_حد_بندي(aligned_داخلا_ويڪر, CASE_جدول_MAX_بٽ)) {
            m_case_جدول_ويڪر = aligned_داخلا_ويڪر << case_ويڪر;  // Can optimize
            أرجع;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    فراغ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = خطأ;
        m_caseHasDefault = خطأ;
        m_case_NCase_شيون = 0;
        m_caseNConditions = 0;
        m_case_Decoder_رڪارڊون.clear();
        m_case_Decoder_داخلا_ويڪر = 0;
        m_case_جدول_ويڪر = 0;
        m_case_Lhs_رڪارڊون.clear();
        m_caseDetailsValid = خطأ;

        Ast_ڳنڍ* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        إذا (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = سچ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1_اندريون_Use user1_اندريون_Use;
            لكل (Ast_ڳنڍ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ڳنڍ_سونپيو* ثابت assp = check_Decoder_سونپيو(prevp);
                إذا (!assp) اكسر;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقي گڻو_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    أرجع refp->varScopep()->user1_سيٽ_Once();
                });
                إذا (گڻو_Assignments) اكسر;
                // Store pre-default assignment
                Lhs_رڪارڊ& lhs_رڪارڊ = m_case_Lhs_رڪارڊون[*assp->lhsp()];
                lhs_رڪارڊ.lhsp = assp->lhsp();
                lhs_رڪارڊ.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقي canBeDecoder = سچ;
        لكل (Ast_Case_شي* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شي)) {
            // Check conditions
            لكل (Ast_ڳنڍ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                إذا (!never_شي(nodep, VN_AS(condp, ڳنڍ_اظهار))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                إذا (!VN_IS(condp, Const)) {
                    m_caseOpaque = سچ;
                    canBeDecoder = خطأ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            إذا (cip->isDefault()) m_caseHasDefault = سچ;
            // Count case items
            ++m_case_NCase_شيون;
            // Check if it fits the decoder pattern, if still possible
            إذا (canBeDecoder) canBeDecoder = analyze_Decoder_Case_شي(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        إذا (m_caseOpaque) أرجع;

        // If small enough, analyse details
        إذا (caseExprp->ويڪر() <= CASE_DETAILS_MAX_ويڪر) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        إذا (canBeDecoder) analyze_Decoder_نمونو(nodep);
    }

    Ast_ڳنڍ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ڳنڍ_اظهار* exprp, V3SharedTmps& tmps) {
        File_سٽ* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        إذا (m_case_Decoder_رڪارڊون.سائيز() == 1) {
            ثابت Lhs_رڪارڊ& lhs_رڪارڊ = m_case_Decoder_رڪارڊون[0];
            ثابت صحيح ويڪر = lhs_رڪارڊ.lhsp->ويڪر();
            Ast_ڳنڍ_اظهار* ثابت rhsp
                = exprp->ويڪر() == ويڪر ? exprp : جديد AstSel{flp, exprp, 0, ويڪر};
            Ast_ڳنڍ_اظهار* ثابت lhsp = lhs_رڪارڊ.lhsp->clone_وڻ_خالص(خطأ);
            إذا (lhs_رڪارڊ.nCaseAssigns) {
                أرجع جديد Ast_سونپيو{flp, lhsp, rhsp};
            } وإلا إذا (lhs_رڪارڊ.n_Case_سونپيو_Dlys) {
                أرجع جديد Ast_سونپيو_Dly{flp, lhsp, rhsp};
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت عارضي_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_داخلا_ويڪر);
        Ast_ڳنڍ_اظهار* ثابت عارضي_Writep = جديد AstVarRef{flp, عارضي_Vscp, VAccess::WRITE};
        Ast_ڳنڍ_Stmt* ثابت resultp = جديد Ast_سونپيو{flp, عارضي_Writep, exprp};

        // For each LHS, select out the result
        لكل (ثابت Lhs_رڪارڊ& lhs_رڪارڊ : m_case_Decoder_رڪارڊون) {
            ثابت صحيح ويڪر = lhs_رڪارڊ.lhsp->ويڪر();
            ثابت صحيح lsb = lhs_رڪارڊ.وٿي;
            Ast_ڳنڍ_اظهار* ثابت عارضي_Readp = جديد AstVarRef{flp, عارضي_Vscp, VAccess::READ};
            Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstSel{flp, عارضي_Readp, lsb, ويڪر};
            Ast_ڳنڍ_اظهار* ثابت lhsp = lhs_رڪارڊ.lhsp->clone_وڻ_خالص(خطأ);
            إذا (lhs_رڪارڊ.nCaseAssigns) {
                resultp->جوڙيو_ايندڙ(جديد Ast_سونپيو{flp, lhsp, rhsp});
            } وإلا إذا (lhs_رڪارڊ.n_Case_سونپيو_Dlys) {
                resultp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{flp, lhsp, rhsp});
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        أرجع resultp;
    }

    Ast_ڳنڍ_Stmt* convert_Case_جدول(AstCase* nodep) {
        // Create the table constant
        File_سٽ* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جديد AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحيح>(m_case_جدول_ويڪر), 0};
        ثابت uint32_t جدول_Entries = 1U << nodep->exprp()->ويڪر();

        ثابت منطقي is_Tiny_جدول = m_case_جدول_ويڪر <= CASE_جدول_TINY_بٽ;
        إذا (is_Tiny_جدول) {
            ++m_stats.case_جدول_Tiny;
        } وإلا {
            ++m_stats.case_جدول_عام;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t داخلا_ويڪر = is_Tiny_جدول
                                        ? m_case_Decoder_داخلا_ويڪر
                                        : VL_لفظ_I(m_case_Decoder_داخلا_ويڪر) * VL_EDATASIZE;
        لكل (ثابت Lhs_رڪارڊ& lhs_رڪارڊ : m_case_Decoder_رڪارڊون) {
            ثابت صحيح lhs_ويڪر = lhs_رڪارڊ.lhsp->ويڪر();
            ثابت صحيح lhs_وٿي = lhs_رڪارڊ.وٿي;

            // Broadcast the pre-default assignment
            إذا (lhs_رڪارڊ.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhs_رڪارڊ.preDefaultp->rhsp(), Const);
                لكل (uint32_t اشاريو = 0; اشاريو < جدول_Entries; ++اشاريو) {
                    ثابت uint32_t جدول_وٿي = اشاريو * داخلا_ويڪر + lhs_وٿي;
                    tablep->انگ().opSelInto(rhsp->انگ(), جدول_وٿي, lhs_ويڪر);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            لكل (Ast_Case_شي* cip = VN_AS(nodep->itemsp()->lastp(), Case_شي); cip;
                 cip = VN_AS(cip->prevp(), Case_شي)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    لكل (Ast_ڳنڍ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ڳنڍ_سونپيو* ثابت ap = VN_AS(stmtp, ڳنڍ_سونپيو);
                        إذا (lhs_رڪارڊ.lhsp->same_وڻ(ap->lhsp())) أرجع VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    أرجع VN_AS(lhs_رڪارڊ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                إذا (cip->isDefault()) {
                    لكل (uint32_t اشاريو = 0; اشاريو < جدول_Entries; ++اشاريو) {
                        ثابت uint32_t جدول_وٿي = اشاريو * داخلا_ويڪر + lhs_وٿي;
                        tablep->انگ().opSelInto(rhsp->انگ(), جدول_وٿي, lhs_ويڪر);
                    }
                    استمر;
                }

                // Iterate case conditions in reverse order
                لكل (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    إذا (never_شي(nodep, condp)) استمر;  // If item never matches, ignore it
                    ثابت خودڪار& ميلاپ = ميلاپ_نمونو(nodep, condp);
                    ثابت uint32_t ميلاپ_نقاب = ميلاپ.پهريون.منزل_UInt();
                    ثابت uint32_t ميلاپ_بٽ = ميلاپ.ٻيو.منزل_UInt();
                    ثابت uint32_t inverse_نقاب = ~ميلاپ_نقاب & ((1U << condp->ويڪر()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    لكل (uint32_t i = inverse_نقاب; سچ; i = (i - 1) & inverse_نقاب) {
                        ثابت uint32_t اشاريو = i | ميلاپ_بٽ;
                        ثابت uint32_t جدول_وٿي = اشاريو * داخلا_ويڪر + lhs_وٿي;
                        tablep->انگ().opSelInto(rhsp->انگ(), جدول_وٿي, lhs_ويڪر);
                        إذا (!i) اكسر;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ڳنڍ_اظهار* ثابت جدول_Refp = [&]() -> Ast_ڳنڍ_اظهار* {
            إذا (is_Tiny_جدول) أرجع tablep;
            AstVarRef* ثابت refp = V3ConstPool::ڳوليو(tablep);
            VL_DO_DANGLING(tablep->delete_وڻ(), tablep);  // V3ConstPool::find clones
            أرجع refp;
        }();

        // Create the lookup index
        Ast_ڳنڍ_اظهار* ثابت caseExprp
            = جديد AstExtend{flp, nodep->exprp()->clone_وڻ_خالص(خطأ), 32};
        Ast_ڳنڍ_اظهار* ثابت scalep = جديد AstConst{flp, داخلا_ويڪر};
        Ast_ڳنڍ_اظهار* ثابت جدول_Lsbp = جديد AstMul{flp, scalep, caseExprp};
        Ast_ڳنڍ_اظهار* ثابت جدول_Selp
            = جديد AstSel{flp, جدول_Refp, جدول_Lsbp, static_cast<صحيح>(m_case_Decoder_داخلا_ويڪر)};

        // Connect outputs
        أرجع connectDecoderOutputs(nodep, جدول_Selp, m_جدول_ٻاهريون_Tmps);
    }

    Ast_ڳنڍ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_سٽ* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::ويڪٽر<std::pair<AstConst*, Ast_ڳنڍ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        لكل (Ast_Case_شي* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_شي)) {
            إذا (cip->isDefault()) {
                clauses.emplace_پٺ(خالي_اشارو, cip->stmtsp());
                استمر;
            }
            لكل (Ast_ڳنڍ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                إذا (never_شي(nodep, iconstp)) استمر;
                clauses.emplace_پٺ(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقي provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        إذا (clauses.پٺ().پهريون && !provenExhaustive) clauses.emplace_پٺ(خالي_اشارو, خالي_اشارو);

        // Number of entries in decoder table
        ثابت صحيح decoderEnries = clauses.سائيز();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحيح cond_ويڪر = nodep->exprp()->ويڪر();
        ثابت صحيح ميلاپ_ويڪر = 2 * VL_لفظ_I(cond_ويڪر) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جديد AstConst{flp, AstConst::Widthed_قدر{}, decoderEnries * ميلاپ_ويڪر, 0};
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            ثابت صحيح داخلا_Lsb = i * ميلاپ_ويڪر;

            // If the entry has a condition, use it's match bits and mask
            إذا (AstConst* ثابت condp = clauses[i].پهريون) {
                ثابت خودڪار& ميلاپ = ميلاپ_نمونو(nodep, condp);
                matchp->انگ().opSelInto(ميلاپ.پهريون, داخلا_Lsb, cond_ويڪر);
                matchp->انگ().opSelInto(ميلاپ.ٻيو, داخلا_Lsb + ميلاپ_ويڪر / 2, cond_ويڪر);
                استمر;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_عدد انگ_Zero{flp, cond_ويڪر, 0};
            matchp->انگ().opSelInto(انگ_Zero, داخلا_Lsb, cond_ويڪر);
            matchp->انگ().opSelInto(انگ_Zero, داخلا_Lsb + ميلاپ_ويڪر / 2, cond_ويڪر);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جديد AstRange{flp, decoderEnries - 1, 0};
        Ast_ڳنڍ_DType* ثابت داخلا_Dtypep = nodep->ڳوليو_ٻٽو_انگ_DType(
            m_case_Decoder_داخلا_ويڪر, m_case_Decoder_داخلا_ويڪر, VSigning::UNSIGNED);
        Ast_ڳنڍ_DType* ثابت جدول_Dtypep = جديد AstUnpackArrayDType{flp, داخلا_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(جدول_Dtypep);
        AstConsArray* ثابت tablep = جديد AstConsArray{flp, جدول_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            Ast_ڳنڍ* ثابت stmtsp = clauses[i].ٻيو;
            AstConst* ثابت entryp = جديد AstConst{flp, AstConst::Widthed_قدر{},
                                                  static_cast<صحيح>(m_case_Decoder_داخلا_ويڪر), 0};
            لكل (ثابت Lhs_رڪارڊ& lhs_رڪارڊ : m_case_Decoder_رڪارڊون) {
                Ast_ڳنڍ_اظهار* ثابت lhsp = lhs_رڪارڊ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = خالي_اشارو;
                لكل (Ast_ڳنڍ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ڳنڍ_سونپيو* ثابت assignp = VN_AS(stmtp, ڳنڍ_سونپيو);
                    إذا (!lhsp->same_وڻ(assignp->lhsp())) استمر;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    اكسر;
                }
                // Not assigned in this clause, so use the pre-assigned default
                إذا (!valConstp) {
                    UASSERT_OBJ(lhs_رڪارڊ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_رڪارڊ.preDefaultp->rhsp(), Const);
                }
                entryp->انگ().opSelInto(valConstp->انگ(), lhs_رڪارڊ.وٿي, lhsp->ويڪر());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت ميلاپ_Refp = V3ConstPool::ڳوليو(matchp);
        AstVarRef* ثابت جدول_Refp = V3ConstPool::ڳوليو(tablep);
        VL_DO_DANGLING(matchp->delete_وڻ(), matchp);
        VL_DO_DANGLING(tablep->delete_وڻ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ڳنڍ_اظهار* ثابت caseExprp = nodep->exprp()->clone_وڻ_خالص(خطأ);
        Ast_ميلاپ_Masked* ثابت indexp = جديد Ast_ميلاپ_Masked{flp, caseExprp, ميلاپ_Refp};
        Ast_ڳنڍ_اظهار* ثابت entryp = جديد AstArraySel{flp, جدول_Refp, indexp};

        أرجع connectDecoderOutputs(nodep, entryp, m_decoder_ٻاهريون_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ڳنڍ* convertCaseFastRecurse(Ast_ڳنڍ_اظهار* cexprp, صحيح msb, uint32_t مٿانهون_قدر) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        إذا (msb < 0) أرجع m_caseDetails.رڪارڊون[مٿانهون_قدر].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t مٿانهون_Value0 = مٿانهون_قدر;
        ثابت uint32_t مٿانهون_Value1 = مٿانهون_قدر | (1UL << msb);
        Ast_ڳنڍ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, مٿانهون_Value0);
        Ast_ڳنڍ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, مٿانهون_Value1);

        // If same logic on both sides, we can just return one of them
        إذا (tree0p == tree1p) أرجع tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقي same = سچ;
            لكل (uint32_t a = مٿانهون_Value0, b = مٿانهون_Value1; a < مٿانهون_Value1; ++a, ++b) {
                إذا (m_caseDetails.رڪارڊون[a].stmtsp != m_caseDetails.رڪارڊون[b].stmtsp) {
                    same = خطأ;
                    اكسر;
                }
            }
            إذا (same) {
                VL_DO_DANGLING(tree1p->delete_وڻ(), tree1p);
                أرجع tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        إذا (tree0p && tree0p->backp()) tree0p = tree0p->clone_وڻ(سچ);
        إذا (tree1p && tree1p->backp()) tree1p = tree1p->clone_وڻ(سچ);
        // Create the If statement
        File_سٽ* ثابت flp = cexprp->fileline();
        Ast_ڳنڍ_اظهار* ثابت condp = جديد AstSel{flp, cexprp->clone_وڻ_خالص(خطأ), msb, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp, tree1p, tree0p};
        أرجع ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ڳنڍ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحيح case_ويڪر = nodep->exprp()->ويڪر();
        Ast_ڳنڍ* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), case_ويڪر - 1, 0UL);
        أرجع ifrootp && ifrootp->backp() ? ifrootp->clone_وڻ(سچ) : ifrootp;
    }

    Ast_ڳنڍ_اظهار* newCaseCompare(ثابت AstCase* casep, Ast_ڳنڍ_اظهار* شي_Exprp) {
        File_سٽ* ثابت flp = شي_Exprp->fileline();
        إذا (never_شي(casep, شي_Exprp)) {
            VL_DO_DANGLING(ڌڪو_Deletep(شي_Exprp), شي_Exprp);
            أرجع جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}};
        }

        إذا (AstCond* ثابت condp = VN_CAST(شي_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ڳنڍ_اظهار* ثابت testp = condp->condp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_پٺ());
            Ast_ڳنڍ_اظهار* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_پٺ());
            VL_DO_DANGLING2(ڌڪو_Deletep(شي_Exprp), شي_Exprp, condp);
            أرجع جديد AstCond{flp, testp, thenp, elsep};
        }

        Ast_ڳنڍ_اظهار* ثابت caseExprp = casep->exprp()->clone_وڻ_خالص(خطأ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        إذا (AstInsideRange* ثابت شي_Rangep = VN_CAST(شي_Exprp, InsideRange)) {
            Ast_ڳنڍ_اظهار* ثابت resultp = شي_Rangep->newAndFromInside(
                caseExprp, شي_Rangep->lhsp()->unlink_Fr_پٺ(), شي_Rangep->rhsp()->unlink_Fr_پٺ());
            VL_DO_DANGLING2(ڌڪو_Deletep(شي_Exprp), شي_Exprp, شي_Rangep);
            أرجع resultp;
        }

        إذا (AstConst* ثابت شي_Constp = VN_CAST(شي_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            إذا (شي_Constp->انگ().isFourState()) {
                ثابت خودڪار& ميلاپ = ميلاپ_نمونو(casep, شي_Constp);
                ثابت V3_عدد& ميلاپ_نقاب = ميلاپ.پهريون;
                ثابت V3_عدد& ميلاپ_بٽ = ميلاپ.ٻيو;
                VL_DO_DANGLING2(ڌڪو_Deletep(شي_Exprp), شي_Exprp, شي_Constp);
                أرجع AstEq::newTyped(flp, جديد AstConst{flp, ميلاپ_بٽ},
                                       جديد AstAnd{flp, caseExprp, جديد AstConst{flp, ميلاپ_نقاب}});
            }
        }

        أرجع AstEq::newTyped(flp, caseExprp, شي_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ڳنڍ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        منطقي hasDefault = خطأ;
        لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {

            File_سٽ* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            إذا (itemp->isDefault()) {
                itemp->جوڙيو_Condsp(جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}});
                hasDefault = سچ;
                استمر;
            }

            // Regular clause. Construct the condition expression.
            Ast_ڳنڍ_اظهار* newCondp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار *شي_Exprp = itemp->condsp(), *nextp; شي_Exprp; شي_Exprp = nextp) {
                nextp = VN_AS(شي_Exprp->nextp(), ڳنڍ_اظهار);
                شي_Exprp->unlink_Fr_پٺ();

                // If case never matches, ignore it
                إذا (never_شي(nodep, شي_Exprp)) {
                    VL_DO_DANGLING(ڌڪو_Deletep(شي_Exprp), شي_Exprp);
                    استمر;
                }

                // Compute the term to add to the condition expression
                Ast_ڳنڍ_اظهار* ثابت termp = newCaseCompare(nodep, شي_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جديد AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            إذا (!newCondp) newCondp = جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}};
            itemp->جوڙيو_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        إذا (!hasDefault) {
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_شي* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), Case_شي);
                ڌڪو_Deletep(lastp->condsp()->unlink_Fr_پٺ_With_ايندڙ());
                lastp->جوڙيو_Condsp(جديد AstConst{lastp->fileline(), AstConst::ٻٽو_انگ_True{}});
            } وإلا {
                // Otherwise needs an explicit empty default
                nodep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                    nodep->fileline(), جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_True{}},
                    خالي_اشارو});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحيح اونهائي = 0;
        AstIf* grouprootp = خالي_اشارو;
        AstIf* groupnextp = خالي_اشارو;
        AstIf* itemnextp = خالي_اشارو;
        لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {

            // Grab the statements from this item. May be empty.
            Ast_ڳنڍ* ثابت istmtsp = itemp->stmtsp();
            إذا (istmtsp) istmtsp->unlink_Fr_پٺ_With_ايندڙ();

            // Expressioned clause
            Ast_ڳنڍ_اظهار* ثابت ifexprp = itemp->condsp()->unlink_Fr_پٺ();
            {  // Prepare for next group
                إذا (++اونهائي > CASE_ENCODER_گروهه_اونهائي) اونهائي = 1;
                إذا (اونهائي == 1) {  // First group or starting new group
                    itemnextp = خالي_اشارو;
                    AstIf* ثابت newp = جديد AstIf{itemp->fileline(), ifexprp->clone_وڻ_خالص(سچ)};
                    إذا (groupnextp) {
                        groupnextp->جوڙيو_Elsesp(newp);
                    } وإلا {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وإلا {  // Continue group, modify if condition to OR in this new condition
                    Ast_ڳنڍ_اظهار* ثابت condp = groupnextp->condp()->unlink_Fr_پٺ();
                    groupnextp->condp(
                        جديد AstOr{ifexprp->fileline(), condp, ifexprp->clone_وڻ_خالص(سچ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ڳنڍ_اظهار* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                إذا (اونهائي == CASE_ENCODER_گروهه_اونهائي) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_وڻ(), itemexprp);
                    itemexprp = جديد AstConst{itemp->fileline(), AstConst::ٻٽو_انگ_True{}};
                }
                AstIf* ثابت newp = جديد AstIf{itemp->fileline(), itemexprp, istmtsp};
                إذا (itemnextp) {
                    itemnextp->جوڙيو_Elsesp(newp);
                } وإلا {
                    groupnextp->جوڙيو_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        أرجع grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ڳنڍ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقي use_جدول = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_جدول()) أرجع خطأ;
            // Not if analysis tells us we can't
            إذا (!m_case_جدول_ويڪر) أرجع خطأ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            إذا (m_case_جدول_ويڪر <= CASE_جدول_TINY_بٽ) أرجع سچ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت سائيز_t branches = std::min<سائيز_t>(nodep->exprp()->ويڪر(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع سچ;
        }();
        إذا (use_جدول) أرجع convert_Case_جدول(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقي useDecoder = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.fCaseDecoder()) أرجع خطأ;
            // Not if not a decoder pattern
            إذا (m_case_Decoder_رڪارڊون.خالي_جايون()) أرجع خطأ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت سائيز_t branches = std::min<سائيز_t>(nodep->exprp()->ويڪر(), m_caseNConditions);
            إذا (branches < CASE_جدول_MIN_BRANCHES) أرجع خطأ;
            أرجع سچ;
        }();
        إذا (useDecoder) أرجع convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقي use_Fast_ٻٽو_انگ_وڻ = [&]() {
            // Not if disabled
            إذا (!v3Global.opt.f_Case_وڻ()) أرجع خطأ;
            // Can't do it without the detailed analysis
            إذا (!m_caseDetailsValid) أرجع خطأ;
            // Can't do it if not exhaustive
            إذا (!m_caseDetails.exhaustive) أرجع خطأ;
            // Not worth doing if there are few conditions
            إذا (m_caseNConditions <= 3) أرجع خطأ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت سائيز_t case_ويڪر = nodep->exprp()->ويڪر();
            إذا (case_ويڪر >= 8 && m_caseNConditions <= (case_ويڪر + 1)) أرجع خطأ;
            // Otherwise use the bit tree
            أرجع سچ;
        }();
        إذا (use_Fast_ٻٽو_انگ_وڻ) أرجع convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        إذا (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_ڪوڊ::LATCH, سچ);
        }
        أرجع convertCaseGeneric(nodep);
    }

    // VISITORS
    فراغ گهمو(AstCase* nodep) override {
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
        إذا (Ast_ڳنڍ* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_پٺ_With_ايندڙ();
            إذا (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_وڻ(), stmtp);
            } وإلا {
                nodep->جوڙيو_ايندڙ_هتي(stmtp);
            }
        }

        // Convert the case statement and replace the original
        إذا (Ast_ڳنڍ* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }

    فراغ گهمو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ گهمو(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::جوڙيو_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::جوڙيو_Stat("Optimizations, Cases table normal", m_stats.case_جدول_عام);
        V3Stats::جوڙيو_Stat("Optimizations, Cases table tiny", m_stats.case_جدول_Tiny);
        V3Stats::جوڙيو_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::جوڙيو_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::جوڙيو_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

سائيز_t CaseVisitor::Lhs_رڪارڊ::s_ايندڙ_Id = 0;

//######################################################################
// Case class functions

فراغ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("case", 0, dump_وڻ_Either_Level() >= 3);
}
فراغ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
